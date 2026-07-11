/**
 * @file sht30.hpp
 * @brief SHT30 温度传感器驱动 — I2C接口, ±0.3°C精度
 * @details 支持单次测量和周期测量模式，内置CRC校验。
 *          参考: 需求评审报告 第10.2节, Sensirion SHT30 Datasheet
 */

#pragma once

#include "../../feature_config.hpp"
#include "../../core/sensor_base.hpp"
#include "../../core/sensor_config.hpp"
#include "../../hal/hal_interface.hpp"

#if SENSOR_FEATURE_SHT30 && SENSOR_FEATURE_HAL_I2C

#include <cstdint>

/**
 * @class SHT30Sensor
 * @brief SHT30 温湿度传感器（仅温度部分）
 * @details 基于 SensorBase<TemperatureData>，通过I2C总线通信。
 *          默认地址 0x44 (ADDR引脚接地), 可选 0x45 (ADDR接VDD)。
 */
class SHT30Sensor : public SensorBase<TemperatureData> {
public:
    /// SHT30 默认I2C地址
    static const uint8_t DEFAULT_ADDR = 0x44;
    static const uint8_t ALT_ADDR     = 0x45;

    /**
     * @param id   传感器唯一ID
     * @param i2c  I2C总线接口
     * @param addr I2C从设备地址（0x44或0x45）
     */
    SHT30Sensor(uint32_t id, II2CBus& i2c, uint8_t addr = DEFAULT_ADDR)
        : SensorBase<TemperatureData>(id, SensorType::TEMPERATURE)
        , i2c_(i2c), addr_(addr)
        , rawTemp_(0), rawHumi_(0)
    {}

    // ─── 生命周期 ────────────────────────────────────────

    bool init() SENSOR_OVERRIDE {
        // 1. 检测设备是否存在
        if (!probeDevice()) {
            setStatus(SensorStatus::COMM_ERROR);
            return false;
        }

        // 2. 发送软件复位命令
        if (!softReset()) {
            setStatus(SensorStatus::ERROR);
            return false;
        }

        // 3. 读取状态寄存器（验证工作正常）
        uint16_t statusReg;
        if (!readStatus(statusReg)) {
            setStatus(SensorStatus::ERROR);
            return false;
        }

        // 4. 标记就绪
        setStatus(SensorStatus::READY);
        resetErrorCount();
        return true;
    }

    bool start() SENSOR_OVERRIDE {
        if (getStatus() != SensorStatus::READY &&
            getStatus() != SensorStatus::STOPPED) {
            return false;
        }
        setStatus(SensorStatus::RUNNING);
        return true;
    }

    bool stop() SENSOR_OVERRIDE {
        setStatus(SensorStatus::STOPPED);
        return true;
    }

    bool reset() SENSOR_OVERRIDE {
        softReset();
        SensorBase::reset();
        setStatus(SensorStatus::READY);
        return true;
    }

    // ─── 数据获取 ────────────────────────────────────────

    TemperatureData readRaw() SENSOR_OVERRIDE {
        TemperatureData data;
        data.sensorId    = sensorId_;
        data.type        = SensorType::TEMPERATURE;
        data.timestampMs = 0;  // 由调用者或定时器填充
        data.status      = getStatus();

        // 发送高精度测量命令 (高重复性 + 时钟拉伸使能)
        uint8_t cmd[] = {0x2C, 0x06};
        if (!i2c_.write(addr_, cmd, 2)) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // 等待测量完成 (典型15ms @ 高重复性)
        delayMs(15);

        // 读取6字节数据: [Temp_MSB, Temp_LSB, Temp_CRC, Humi_MSB, Humi_LSB, Humi_CRC]
        uint8_t buf[6];
        if (!i2c_.read(addr_, buf, 6)) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // CRC校验
        if (!checkCRC(&buf[0], 2, buf[2]) ||
            !checkCRC(&buf[3], 2, buf[5])) {
            incrementError();
            data.status = SensorStatus::COMM_ERROR;
            setStatus(SensorStatus::COMM_ERROR);
            return data;
        }

        // 解析温度值: T[°C] = -45 + 175 * (Raw_T / 65535)
        rawTemp_ = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
        rawHumi_ = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];

        float temperature = -45.0f + 175.0f *
            static_cast<float>(rawTemp_) / 65535.0f;

        data.value    = temperature;
        data.rawValue = static_cast<float>(rawTemp_);
        data.status   = SensorStatus::RUNNING;

        resetErrorCount();
        return data;
    }

    // ─── 配置 ────────────────────────────────────────────

    const char* getName() const SENSOR_OVERRIDE { return "SHT30"; }

    /**
     * @brief 同步读取温湿度（在需要湿度数据的场景）
     * @param outHumidity [输出] 湿度值 (%RH)
     * @return 温度数据
     */
    TemperatureData readBoth(float& outHumidity) {
        TemperatureData data = readRaw();

        // 解析湿度: RH[%] = 100 * (Raw_H / 65535)
        outHumidity = 100.0f *
            static_cast<float>(rawHumi_) / 65535.0f;

        return data;
    }

    /// 获取最后一次原始温度ADC值
    uint16_t getRawTempADC() const { return rawTemp_; }

    /// 获取最后一次原始湿度ADC值
    uint16_t getRawHumiADC() const { return rawHumi_; }

private:
    // ─── SHT30 命令 ──────────────────────────────────────

    /// 软件复位
    bool softReset() {
        uint8_t cmd[] = {0x30, 0xA2};
        return i2c_.write(addr_, cmd, 2);
    }

    /// 读取状态寄存器
    bool readStatus(uint16_t& status) {
        uint8_t cmd[] = {0xF3, 0x2D};
        if (!i2c_.write(addr_, cmd, 2)) return false;

        uint8_t buf[3];
        if (!i2c_.read(addr_, buf, 3)) return false;

        status = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
        return true;
    }

    /// 检测设备是否存在
    bool probeDevice() {
        // SHT30不支持I2C地址扫描ACK（需要发送命令后检查）
        // 使用状态寄存器读取作为存在性检测
        uint16_t status;
        return readStatus(status);
    }

    /// CRC-8校验 (多项式: x^8 + x^5 + x^4 + 1)
    static bool checkCRC(const uint8_t* data, uint8_t len, uint8_t expectedCRC) {
        uint8_t crc = 0xFF;
        for (uint8_t i = 0; i < len; ++i) {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if (crc & 0x80) {
                    crc = static_cast<uint8_t>((crc << 1) ^ 0x31);
                } else {
                    crc = static_cast<uint8_t>(crc << 1);
                }
            }
        }
        return crc == expectedCRC;
    }

    /// 简易延时（嵌入式平台替换为HAL延时）
    void delayMs(uint32_t ms) {
        // 简单忙等待（嵌入式环境）
        for (volatile uint32_t i = 0; i < ms * 500; ++i) {
            __asm__ volatile("nop");
        }
    }

    II2CBus&    i2c_;
    uint8_t     addr_;
    uint16_t    rawTemp_;
    uint16_t    rawHumi_;
};

#endif // SENSOR_FEATURE_SHT30
