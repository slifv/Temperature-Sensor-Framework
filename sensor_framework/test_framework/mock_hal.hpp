/**
 * @file mock_hal.hpp
 * @brief Mock HAL层 — 白盒测试的核心依赖
 * @details 提供 Mock 实现替代真实硬件，支持预设数据、错误注入、
 *          行为录制（记录写入数据/函数调用历史供断言使用）。
 *          参考: 需求评审报告 第15.2.2节
 */

#pragma once

#include "../feature_config.hpp"

#if SENSOR_FEATURE_TEST_FRAMEWORK

#include "../hal/hal_interface.hpp"
#include <cstdint>
#include <cstring>
#include <queue>

// ============================================================
//  Mock I2C 总线
// ============================================================

/**
 * @class MockI2CBus
 * @brief Mock I2C总线 — 预设设备响应，支持错误注入
 */
class MockI2CBus : public II2CBus {
public:
    /// 记录写操作历史
    struct WriteRecord {
        uint8_t addr;
        uint8_t data[32];
        size_t  len;
    };

    MockI2CBus() : writeCount_(0), readCount_(0), nackCount_(0) {}

    // ─── 预设设备行为 ──────────────────────────────────

    /// 预设设备返回数据（FIFO队列）
    void setDeviceResponse(uint8_t addr, const uint8_t* response, size_t len) {
        ResponseEntry entry;
        entry.addr = addr;
        entry.dataLen = (len < 64) ? len : 64;
        for (size_t i = 0; i < entry.dataLen; ++i) {
            entry.data[i] = response[i];
        }
        responses_.push(entry);
    }

    /// 设置某地址总是返回NACK
    void setNackOnAddress(uint8_t addr) {
        for (uint8_t i = 0; i < nackCount_; ++i) {
            if (nackAddrs_[i] == addr) return;
        }
        if (nackCount_ < 8) {
            nackAddrs_[nackCount_++] = addr;
        }
    }

    /// 清除某地址的NACK设置
    void clearNackOnAddress(uint8_t addr) {
        for (uint8_t i = 0; i < nackCount_; ++i) {
            if (nackAddrs_[i] == addr) {
                nackAddrs_[i] = nackAddrs_[--nackCount_];
                return;
            }
        }
    }

    /// 清除所有预设
    void clearAll() {
        while (!responses_.empty()) responses_.pop();
        nackCount_ = 0;
        writeCount_ = 0;
        readCount_ = 0;
    }

    // ─── II2CBus 接口实现 ──────────────────────────────

    bool write(uint8_t addr, const uint8_t* data, size_t len) SENSOR_OVERRIDE {
        if (isNacked(addr)) return false;

        // 记录写入历史
        if (writeCount_ < MAX_RECORDS) {
            writeRecords_[writeCount_].addr = addr;
            writeRecords_[writeCount_].len = (len < 32) ? len : 32;
            for (size_t i = 0; i < writeRecords_[writeCount_].len; ++i) {
                writeRecords_[writeCount_].data[i] = data[i];
            }
            ++writeCount_;
        }
        return true;
    }

    bool read(uint8_t addr, uint8_t* data, size_t len) SENSOR_OVERRIDE {
        if (isNacked(addr)) return false;
        ++readCount_;

        // 返回预设数据
        if (!responses_.empty()) {
            ResponseEntry& entry = responses_.front();
            size_t copyLen = (len < entry.dataLen) ? len : entry.dataLen;
            for (size_t i = 0; i < copyLen; ++i) {
                data[i] = entry.data[i];
            }
            responses_.pop();
            return true;
        }
        return false;
    }

    bool writeRead(uint8_t addr,
                   const uint8_t* tx, size_t txLen,
                   uint8_t* rx, size_t rxLen) SENSOR_OVERRIDE
    {
        if (!write(addr, tx, txLen)) return false;
        return read(addr, rx, rxLen);
    }

    void setFrequency(uint32_t) SENSOR_OVERRIDE { /* no-op */ }

    // ─── 断言辅助 ──────────────────────────────────────

    uint8_t getWriteCount() const { return writeCount_; }
    uint8_t getReadCount() const { return readCount_; }

    const WriteRecord& getWriteRecord(uint8_t index) const {
        return writeRecords_[index];
    }

private:
    bool isNacked(uint8_t addr) const {
        for (uint8_t i = 0; i < nackCount_; ++i) {
            if (nackAddrs_[i] == addr) return true;
        }
        return false;
    }

    struct ResponseEntry {
        uint8_t addr;
        uint8_t data[64];
        size_t  dataLen;
    };

    static const uint8_t MAX_RECORDS = 16;

    std::queue<ResponseEntry>   responses_;
    uint8_t                     nackAddrs_[8];
    uint8_t                     nackCount_;
    WriteRecord                 writeRecords_[MAX_RECORDS];
    uint8_t                     writeCount_;
    uint8_t                     readCount_;
};

// ============================================================
//  Mock 时间提供者
// ============================================================

/**
 * @class MockTimeProvider
 * @brief Mock 时间提供者 — 完全可控的时间推进
 */
class MockTimeProvider : public ITimeProvider {
public:
    MockTimeProvider() : tickMs_(0), tickUs_(0) {}

    void advance(uint32_t ms) {
        tickMs_ += ms;
        tickUs_ += static_cast<uint64_t>(ms) * 1000ULL;
    }

    void setTick(uint32_t ms) {
        tickMs_ = ms;
        tickUs_ = static_cast<uint64_t>(ms) * 1000ULL;
    }

    uint32_t getTickMs() SENSOR_OVERRIDE { return tickMs_; }
    uint64_t getTickUs() SENSOR_OVERRIDE { return tickUs_; }

    void delayMs(uint32_t ms) SENSOR_OVERRIDE {
        advance(ms);
    }

    void delayUs(uint32_t us) SENSOR_OVERRIDE {
        tickUs_ += us;
        tickMs_ += us / 1000;
    }

private:
    uint32_t tickMs_;
    uint64_t tickUs_;
};

// ============================================================
//  Mock ADC 通道
// ============================================================

#if SENSOR_FEATURE_HAL_ADC
class MockADCChannel : public IADCChannel {
public:
    MockADCChannel() : raw_(0), voltage_(0.0f), refVoltage_(3.3f), resolution_(12) {}

    void setRaw(uint16_t raw) {
        raw_ = raw;
        uint16_t maxRaw = (1U << resolution_) - 1;
        voltage_ = static_cast<float>(raw) * refVoltage_ / static_cast<float>(maxRaw);
    }

    void setVoltage(float v) { voltage_ = v; }

    float readVoltage() SENSOR_OVERRIDE { return voltage_; }
    uint16_t readRaw() SENSOR_OVERRIDE { return raw_; }
    float getReferenceVoltage() const SENSOR_OVERRIDE { return refVoltage_; }
    uint8_t getResolution() const SENSOR_OVERRIDE { return resolution_; }

private:
    uint16_t raw_;
    float    voltage_;
    float    refVoltage_;
    uint8_t  resolution_;
};
#endif

#endif // SENSOR_FEATURE_TEST_FRAMEWORK
