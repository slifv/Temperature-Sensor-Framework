/**
 * @file hal_interface.hpp
 * @brief 硬件抽象层统一接口定义
 * @details 定义所有平台无关的HAL接口: 时间、互斥锁、I2C/SPI/ADC总线抽象。
 *          各平台通过实现这些接口完成平台适配。
 *          参考: 需求评审报告 第8.1节
 */

#pragma once

#include "../compiler_features.hpp"
#include <cstdint>
#include <cstddef>

// ============================================================
//  时间抽象接口
// ============================================================

/**
 * @class ITimeProvider
 * @brief 系统时间提供者接口
 */
class ITimeProvider {
public:
    virtual ~ITimeProvider() {}

    /// 获取系统滴答（毫秒），自系统启动计
    virtual uint32_t getTickMs() = 0;

    /// 获取系统滴答（微秒），自系统启动计
    virtual uint64_t getTickUs() = 0;

    /// 毫秒级阻塞延时
    virtual void delayMs(uint32_t ms) = 0;

    /// 微秒级阻塞延时
    virtual void delayUs(uint32_t us) = 0;
};

// ============================================================
//  互斥锁抽象接口
// ============================================================

/**
 * @class IMutex
 * @brief 互斥锁抽象接口
 * @details 在 Bare-metal 无RTOS环境下可实现为关中断/开中断。
 *          RTOS环境下映射到 osMutex / FreeRTOS Mutex。
 */
class IMutex {
public:
    virtual ~IMutex() {}

    /// 获取锁（阻塞）
    virtual void lock() = 0;

    /// 释放锁
    virtual void unlock() = 0;

    /// 尝试获取锁（非阻塞）
    /// @return 获取成功返回 true
    virtual bool tryLock() = 0;
};

// ============================================================
//  I2C 总线抽象接口
// ============================================================

/**
 * @class II2CBus
 * @brief I2C总线抽象接口
 */
class II2CBus {
public:
    virtual ~II2CBus() {}

    /**
     * @brief 向从设备写入数据
     * @param addr 7位从设备地址
     * @param data 数据缓冲区
     * @param len  数据长度
     * @return 写入成功返回 true（收到ACK）
     */
    virtual bool write(uint8_t addr, const uint8_t* data, size_t len) = 0;

    /**
     * @brief 从从设备读取数据
     * @param addr 7位从设备地址
     * @param data 接收缓冲区
     * @param len  期望读取长度
     * @return 读取成功返回 true
     */
    virtual bool read(uint8_t addr, uint8_t* data, size_t len) = 0;

    /**
     * @brief 组合写-读操作（典型I2C寄存器访问模式）
     * @param addr  7位从设备地址
     * @param tx    发送缓冲区（寄存器地址/命令）
     * @param txLen 发送长度
     * @param rx    接收缓冲区（读取到的数据）
     * @param rxLen 期望读取长度
     * @return 操作成功返回 true
     */
    virtual bool writeRead(uint8_t addr,
                           const uint8_t* tx, size_t txLen,
                           uint8_t* rx, size_t rxLen) = 0;

    /**
     * @brief 设置I2C总线速率
     * @param freqHz 频率(Hz)，典型值: 100000(标准), 400000(快速)
     */
    virtual void setFrequency(uint32_t freqHz) = 0;
};

// ============================================================
//  SPI 总线抽象接口
// ============================================================

/**
 * @class ISPIBus
 * @brief SPI总线抽象接口
 */
class ISPIBus {
public:
    virtual ~ISPIBus() {}

    /**
     * @brief 全双工SPI传输
     * @param tx  发送缓冲区（可为nullptr仅读取）
     * @param rx  接收缓冲区（可为nullptr仅写入）
     * @param len 传输字节数
     * @return 传输成功返回 true
     */
    virtual bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) = 0;

    /// 控制片选信号
    virtual void setCsLevel(bool high) = 0;

    /// 设置SPI时钟频率(Hz)
    virtual void setFrequency(uint32_t freqHz) = 0;

    /// 设置SPI模式 (CPOL/CPHA)
    /// @param mode 0-3 (mode 0: CPOL=0,CPHA=0; mode 3: CPOL=1,CPHA=1)
    virtual void setMode(uint8_t mode) = 0;
};

// ============================================================
//  ADC 通道抽象接口
// ============================================================

/**
 * @class IADCChannel
 * @brief ADC模数转换通道抽象接口
 */
class IADCChannel {
public:
    virtual ~IADCChannel() {}

    /// 读取电压值（V）
    virtual float readVoltage() = 0;

    /// 读取原始ADC值
    virtual uint16_t readRaw() = 0;

    /// 获取基准电压（V）
    virtual float getReferenceVoltage() const = 0;

    /// 获取ADC分辨率（位数）
    virtual uint8_t getResolution() const = 0;
};

// ============================================================
//  OneWire 总线抽象接口
// ============================================================

#if SENSOR_FEATURE_HAL_ONEWIRE
/**
 * @class IOneWireBus
 * @brief OneWire单总线抽象接口
 */
class IOneWireBus {
public:
    virtual ~IOneWireBus() {}

    /// 复位总线并检测从设备存在脉冲
    /// @return 检测到设备返回 true
    virtual bool reset() = 0;

    /// 写入一个字节
    virtual void writeByte(uint8_t data) = 0;

    /// 读取一个字节
    virtual uint8_t readByte() = 0;

    /// 写入一个bit
    virtual void writeBit(bool bit) = 0;

    /// 读取一个bit
    virtual bool readBit() = 0;
};
#endif // SENSOR_FEATURE_HAL_ONEWIRE

// ============================================================
//  GPIO 抽象接口
// ============================================================

/**
 * @class IGPIO
 * @brief 通用GPIO抽象接口
 */
class IGPIO {
public:
    virtual ~IGPIO() {}

    /// 设置输出电平
    virtual void write(bool high) = 0;

    /// 读取输入电平
    virtual bool read() = 0;

    /// 翻转输出电平
    virtual void toggle() = 0;
};
