/**
 * @file i2c_bus.hpp
 * @brief I2C总线封装 — 在HAL接口上添加超时/重试/日志功能
 */

#pragma once

#include "../hal_interface.hpp"
#include "../../feature_config.hpp"
#include "../../compiler_features.hpp"

#if SENSOR_FEATURE_HAL_I2C

#include <cstdint>
#include <cstring>

/**
 * @class I2CBusWrapper
 * @brief I2C总线包装器：在原始II2CBus上附加超时控制和重试逻辑
 */
class I2CBusWrapper {
public:
    /**
     * @param bus      底层I2C总线实现
     * @param timeoutMs 操作超时时间(ms)，0表示无超时
     */
    I2CBusWrapper(II2CBus& bus, uint32_t timeoutMs = 50)
        : bus_(bus), timeoutMs_(timeoutMs)
    {}

    /// 获取底层总线引用
    II2CBus& getRaw() { return bus_; }

    /**
     * @brief 带重试的写操作
     * @param addr       从设备地址
     * @param data       数据缓冲区
     * @param len        数据长度
     * @param maxRetries 最大重试次数
     * @return 成功返回 true
     */
    bool write(uint8_t addr, const uint8_t* data, size_t len,
               uint8_t maxRetries = 3)
    {
        for (uint8_t retry = 0; retry <= maxRetries; ++retry) {
            if (bus_.write(addr, data, len)) {
                return true;
            }
            if (retry < maxRetries) {
                delayRetry();
            }
        }
        return false;
    }

    /**
     * @brief 带重试的读操作
     */
    bool read(uint8_t addr, uint8_t* data, size_t len,
              uint8_t maxRetries = 3)
    {
        for (uint8_t retry = 0; retry <= maxRetries; ++retry) {
            if (bus_.read(addr, data, len)) {
                return true;
            }
            if (retry < maxRetries) {
                delayRetry();
            }
        }
        return false;
    }

    /**
     * @brief 带重试的组合写-读操作
     */
    bool writeRead(uint8_t addr,
                   const uint8_t* tx, size_t txLen,
                   uint8_t* rx, size_t rxLen,
                   uint8_t maxRetries = 3)
    {
        for (uint8_t retry = 0; retry <= maxRetries; ++retry) {
            if (bus_.writeRead(addr, tx, txLen, rx, rxLen)) {
                return true;
            }
            if (retry < maxRetries) {
                delayRetry();
            }
        }
        return false;
    }

    /**
     * @brief 写入16位寄存器命令并读取数据
     *        常见模式: 发送2字节命令 → 读取N字节响应
     */
    bool writeCmd16Read(uint8_t addr, uint16_t command,
                        uint8_t* rx, size_t rxLen,
                        uint8_t maxRetries = 3)
    {
        uint8_t cmd[2] = {
            static_cast<uint8_t>((command >> 8) & 0xFF),
            static_cast<uint8_t>(command & 0xFF)
        };
        return writeRead(addr, cmd, 2, rx, rxLen, maxRetries);
    }

    /**
     * @brief 检测设备是否存在（发送地址并检查ACK）
     */
    bool probeDevice(uint8_t addr) {
        uint8_t dummy = 0;
        return bus_.write(addr, &dummy, 0);
    }

    /// 设置超时时间
    void setTimeout(uint32_t timeoutMs) { timeoutMs_ = timeoutMs; }

private:
    void delayRetry() {
        if (timeoutMs_ > 0) {
            // 简单忙等待（嵌入式环境）或系统延时
            volatile uint32_t count = timeoutMs_ * 1000;
            while (count--) { /* spin */ }
        }
    }

    II2CBus&    bus_;
    uint32_t    timeoutMs_;
};

#endif // SENSOR_FEATURE_HAL_I2C
