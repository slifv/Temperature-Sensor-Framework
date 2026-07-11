/**
 * @file ds18b20.hpp
 * @brief DS18B20 温度传感器驱动 — OneWire单总线, ±0.5°C精度
 * @details 支持寄生供电模式，可级联多点测温（通过ROM ID区分）。
 *          分辨率9/10/11/12位可配置。
 *          参考: Maxim DS18B20 Datasheet
 */

#pragma once

#include "../../feature_config.hpp"
#include "../../core/sensor_base.hpp"
#include "../../core/sensor_config.hpp"
#include "../../hal/hal_interface.hpp"

#if SENSOR_FEATURE_DS18B20 && SENSOR_FEATURE_HAL_ONEWIRE

#include <cstdint>

/**
 * @class DS18B20Sensor
 * @brief DS18B20 数字温度传感器
 * @details 通过 OneWire 单总线协议通信。
 *          默认12位分辨率，转换时间最长750ms。
 */
class DS18B20Sensor : public SensorBase<TemperatureData> {
public:
    /// DS18B20 ROM命令
    enum RomCmd : uint8_t {
        ROM_READ        = 0x33,    ///< 读ROM（仅单设备时有效）
        ROM_MATCH       = 0x55,    ///< 匹配ROM
        ROM_SKIP        = 0xCC,    ///< 跳过ROM（单设备广播）
        ROM_SEARCH      = 0xF0,    ///< 搜索ROM
        ROM_ALARM       = 0xEC     ///< 告警搜索
    };

    /// DS18B20 功能命令
    enum FuncCmd : uint8_t {
        FUNC_CONVERT_T      = 0x44,    ///< 启动温度转换
        FUNC_WRITE_SCRATCH  = 0x4E,    ///< 写暂存器
        FUNC_READ_SCRATCH   = 0xBE,    ///< 读暂存器
        FUNC_COPY_SCRATCH   = 0x48,    ///< 复制暂存器到EEPROM
        FUNC_RECALL_EE      = 0xB8,    ///< 从EEPROM调出配置
        FUNC_READ_POWER     = 0xB4     ///< 读供电模式
    };

    /// 分辨率配置
    enum Resolution : uint8_t {
        RES_9_BIT  = 0,     ///< 9位: 转换时间93.75ms
        RES_10_BIT = 1,     ///< 10位: 187.5ms
        RES_11_BIT = 2,     ///< 11位: 375ms
        RES_12_BIT = 3      ///< 12位: 750ms (默认)
    };

    /**
     * @param id      传感器唯一ID
     * @param oneWire OneWire总线接口
     */
    DS18B20Sensor(uint32_t id, IOneWireBus& oneWire)
        : SensorBase<TemperatureData>(id, SensorType::TEMPERATURE)
        , oneWire_(oneWire)
        , resolution_(RES_12_BIT)
    {}

    // ─── 生命周期 ────────────────────────────────────────

    bool init() SENSOR_OVERRIDE {
        // 1. 检测总线复位和设备存在脉冲
        if (!oneWire_.reset()) {
            setStatus(SensorStatus::COMM_ERROR);
            return false;
        }

        // 2. 读取当前分辨率配置
        readScratchpad();

        setStatus(SensorStatus::READY);
        resetErrorCount();
        return true;
    }

    bool start() SENSOR_OVERRIDE {
        if (getStatus() != SensorStatus::READY &&
            getStatus() != SensorStatus::STOPPED) {
            return false;
        }

        // 写入分辨率配置
        writeResolution(resolution_);

        setStatus(SensorStatus::RUNNING);
        return true;
    }

    bool stop() SENSOR_OVERRIDE {
        setStatus(SensorStatus::STOPPED);
        return true;
    }

    bool reset() SENSOR_OVERRIDE {
        oneWire_.reset();
        SensorBase::reset();
        setStatus(SensorStatus::READY);
        return true;
    }

    // ─── 数据获取 ────────────────────────────────────────

    TemperatureData readRaw() SENSOR_OVERRIDE {
        TemperatureData data;
        data.sensorId    = sensorId_;
        data.type        = SensorType::TEMPERATURE;
        data.timestampMs = 0;
        data.status      = getStatus();

        // 1. 启动温度转换
        if (!startConversion()) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // 2. 等待转换完成
        delayMs(getConversionTime());

        // 3. 读取暂存器
        uint8_t scratchpad[9];
        if (!readScratchpad(scratchpad)) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // 4. CRC校验
        if (!checkCRC8(scratchpad, 8, scratchpad[8])) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // 5. 解析温度值
        int16_t rawTemp = (static_cast<int16_t>(scratchpad[1]) << 8) | scratchpad[0];
        float temperature = static_cast<float>(rawTemp) / 16.0f;  // DS18B20: 1 LSB = 0.0625°C

        data.value    = temperature;
        data.rawValue = static_cast<float>(rawTemp);
        data.status   = SensorStatus::RUNNING;

        resetErrorCount();
        return data;
    }

    // ─── 配置 ────────────────────────────────────────────

    const char* getName() const SENSOR_OVERRIDE { return "DS18B20"; }

    /**
     * @brief 设置分辨率
     */
    void setResolution(Resolution res) {
        resolution_ = res;
        if (getStatus() == SensorStatus::RUNNING) {
            writeResolution(res);
        }
    }

    Resolution getResolution() const { return resolution_; }

    /// 获取当前分辨率的转换时间(ms)
    uint16_t getConversionTime() const {
        switch (resolution_) {
            case RES_9_BIT:  return 94;
            case RES_10_BIT: return 188;
            case RES_11_BIT: return 375;
            case RES_12_BIT: return 750;
            default:         return 750;
        }
    }

private:
    /// 启动温度转换
    bool startConversion() {
        if (!oneWire_.reset()) return false;
        oneWire_.writeByte(ROM_SKIP);
        oneWire_.writeByte(FUNC_CONVERT_T);
        return true;
    }

    /// 读取暂存器（9字节: [Temp_LSB, Temp_MSB, TH, TL, Config, Reserved*3, CRC]）
    bool readScratchpad(uint8_t* buf = NULL) {
        uint8_t localBuf[9];
        uint8_t* out = buf ? buf : localBuf;

        if (!oneWire_.reset()) return false;
        oneWire_.writeByte(ROM_SKIP);
        oneWire_.writeByte(FUNC_READ_SCRATCH);

        for (uint8_t i = 0; i < 9; ++i) {
            out[i] = oneWire_.readByte();
        }

        // 解析分辨率
        if (!buf) {
            uint8_t config = out[4];
            resolution_ = static_cast<Resolution>((config >> 5) & 0x03);
        }

        return checkCRC8(out, 8, out[8]);
    }

    /// 写入分辨率配置
    bool writeResolution(Resolution res) {
        if (!oneWire_.reset()) return false;

        oneWire_.writeByte(ROM_SKIP);
        oneWire_.writeByte(FUNC_WRITE_SCRATCH);

        // TH寄存器 (告警上限, 默认75°C)
        oneWire_.writeByte(0x4B);
        // TL寄存器 (告警下限, 默认-10°C)
        oneWire_.writeByte(0xF6);
        // 配置寄存器 (R1,R0=分辨率, 其余为1)
        uint8_t config = 0x1F | (static_cast<uint8_t>(res) << 5);
        oneWire_.writeByte(config);

        return true;
    }

    /// Dallas/Maxim CRC-8 (多项式: x^8 + x^5 + x^4 + 1)
    static bool checkCRC8(const uint8_t* data, uint8_t len, uint8_t expected) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < len; ++i) {
            uint8_t inByte = data[i];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                uint8_t mix = (crc ^ inByte) & 0x01;
                crc >>= 1;
                if (mix) crc ^= 0x8C;
                inByte >>= 1;
            }
        }
        return crc == expected;
    }

    void delayMs(uint32_t ms) {
        for (volatile uint32_t i = 0; i < ms * 500; ++i) {
            __asm__ volatile("nop");
        }
    }

    IOneWireBus&    oneWire_;
    Resolution      resolution_;
};

#endif // SENSOR_FEATURE_DS18B20
