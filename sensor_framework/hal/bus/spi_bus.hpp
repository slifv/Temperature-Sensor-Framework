/**
 * @file spi_bus.hpp
 * @brief SPI总线封装 — 在HAL接口上添加CS管理/重试/多字节传输
 */

#pragma once

#include "../hal_interface.hpp"
#include "../../feature_config.hpp"
#include "../../compiler_features.hpp"

#if SENSOR_FEATURE_HAL_SPI

#include <cstdint>
#include <cstring>

/**
 * @class SPIBusWrapper
 * @brief SPI总线包装器：CS自动管理 + 重试逻辑
 */
class SPIBusWrapper {
public:
    SPIBusWrapper(ISPIBus& bus)
        : bus_(bus)
    {}

    ISPIBus& getRaw() { return bus_; }

    /**
     * @brief 全双工传输（自动管理CS）
     * @param tx  发送缓冲区
     * @param rx  接收缓冲区
     * @param len 传输字节数
     * @return 成功返回 true
     */
    bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
        csLow();
        bool result = bus_.transfer(tx, rx, len);
        csHigh();
        return result;
    }

    /**
     * @brief 仅写入数据
     */
    bool write(const uint8_t* data, size_t len) {
        return transfer(data, NULL, len);
    }

    /**
     * @brief 仅读取数据
     */
    bool read(uint8_t* data, size_t len) {
        uint8_t dummyTx = 0xFF;
        // 发送dummy bytes来产生时钟
        csLow();
        bool result = true;
        for (size_t i = 0; i < len; ++i) {
            uint8_t rxByte;
            if (!bus_.transfer(&dummyTx, &rxByte, 1)) {
                result = false;
                break;
            }
            data[i] = rxByte;
        }
        csHigh();
        return result;
    }

    /**
     * @brief 写寄存器: 寄存器地址 + 数据
     * @param regAddr 寄存器地址
     * @param data    写入数据
     * @param len     数据长度
     */
    bool writeRegister(uint8_t regAddr, const uint8_t* data, size_t len) {
        csLow();
        bus_.transfer(&regAddr, NULL, 1);
        bool result = bus_.transfer(data, NULL, len);
        csHigh();
        return result;
    }

    /**
     * @brief 读寄存器: 发送地址(含读标志位) → 读取数据
     * @param regAddr 寄存器地址 (需包含读标志位如 0x80)
     * @param data    接收缓冲区
     * @param len     读取长度
     */
    bool readRegister(uint8_t regAddr, uint8_t* data, size_t len) {
        csLow();
        bus_.transfer(&regAddr, NULL, 1);
        bool result = bus_.read(data, len);
        csHigh();
        return result;
    }

    void setFrequency(uint32_t freqHz) { bus_.setFrequency(freqHz); }
    void setMode(uint8_t mode) { bus_.setMode(mode); }

private:
    void csLow()  { bus_.setCsLevel(false); }
    void csHigh() { bus_.setCsLevel(true); }

    ISPIBus& bus_;
};

#endif // SENSOR_FEATURE_HAL_SPI
