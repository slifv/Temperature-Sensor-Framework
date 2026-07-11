/**
 * @file exception_types.hpp
 * @brief 异常分类体系 — 类型/级别/事件结构定义
 * @details 定义通信异常/数据异常/滤波异常/系统异常四大类，
 *          INFO/WARNING/ERROR/FATAL四级严重度，
 *          以及统一的 ExceptionEvent 事件结构体。
 *          参考: 需求评审报告 第16.1-16.3节
 */

#pragma once

#include "../feature_config.hpp"
#include "../compiler_features.hpp"

#if SENSOR_FEATURE_EXCEPTION

#include <cstdint>

// ============================================================
//  异常类别
// ============================================================

/// 异常大类
enum class ExceptionCategory : uint8_t {
    COMMUNICATION   = 0,    ///< 通信异常 (I2C NACK/SPI超时/OneWire无应答/CRC)
    DATA_VALIDITY   = 1,    ///< 数据有效性异常 (超出量程/数据冻结/突变)
    FILTER_ANOMALY  = 2,    ///< 滤波异常 (NaN/Inf/发散)
    SYSTEM_RESOURCE = 3     ///< 系统资源异常 (内存不足/栈溢出)
};

/// 异常严重级别
enum class ExceptionSeverity : uint8_t {
    INFO    = 0,    ///< L0: 通知 — 偶发重试成功、轻微噪声
    WARNING = 1,    ///< L1: 警告 — 连续重试2-3次成功、数据接近边界
    ERROR   = 2,    ///< L2: 错误 — 连续失败超阈值、数据越界、需降级
    FATAL   = 3     ///< L3: 致命 — 熔断触发、硬件损坏、需人工/系统复位
};

// ============================================================
//  错误码
// ============================================================

/// 具体错误码（高8位=类别, 低8位=子类型）
enum ErrorCode : uint16_t {
    // 通信异常 (0x01xx)
    ERR_I2C_NACK        = 0x0101,  ///< I2C无应答
    ERR_I2C_BUS_BUSY    = 0x0102,  ///< I2C总线忙
    ERR_SPI_TIMEOUT     = 0x0103,  ///< SPI超时
    ERR_SPI_DMA_ERROR   = 0x0104,  ///< SPI DMA错误
    ERR_ONEWIRE_NO_DEV  = 0x0105,  ///< OneWire无应答
    ERR_CRC_MISMATCH    = 0x0106,  ///< CRC校验失败
    ERR_BUS_GENERAL     = 0x010F,  ///< 通用通信错误

    // 数据异常 (0x02xx)
    ERR_OUT_OF_RANGE    = 0x0201,  ///< 超出量程
    ERR_DATA_FROZEN     = 0x0202,  ///< 数据冻结（长时间不变）
    ERR_SPIKE_DETECTED  = 0x0203,  ///< 突变检测（跳变过大）
    ERR_NEGATIVE_VALUE  = 0x0204,  ///< 非法负值

    // 滤波异常 (0x03xx)
    ERR_FILTER_NAN          = 0x0301,  ///< 滤波输出NaN
    ERR_FILTER_INF          = 0x0302,  ///< 滤波输出Inf
    ERR_FILTER_DIVERGENCE   = 0x0303,  ///< 滤波发散

    // 系统异常 (0x04xx)
    ERR_SYS_OOM             = 0x0401,  ///< 内存不足
    ERR_SYS_STACK_OVERFLOW  = 0x0402,  ///< 栈溢出
    ERR_SYS_WATCHDOG        = 0x0403,  ///< 看门狗超时

    ERR_NONE = 0
};

// ============================================================
//  恢复动作
// ============================================================

/// 异常恢复动作
enum class RecoveryAction : uint8_t {
    NONE                = 0,    ///< 无动作（仅记录）
    RETRY               = 1,    ///< 重试通信
    USE_LAST_VALID      = 2,    ///< 使用上次有效值（降级）
    USE_DEFAULT         = 3,    ///< 使用默认值（降级）
    RESET_SENSOR        = 4,    ///< 复位传感器
    STOP_SAMPLING       = 5,    ///< 停止采样
    SYSTEM_ALERT        = 6     ///< 触发系统告警
};

// ============================================================
//  异常事件结构体
// ============================================================

/**
 * @struct ExceptionEvent
 * @brief 统一异常事件结构
 */
struct ExceptionEvent {
    uint32_t            timestampMs;        ///< 异常发生时间戳
    uint32_t            sensorId;           ///< 相关传感器ID
    ExceptionCategory   category;           ///< 异常类别
    ExceptionSeverity   severity;           ///< 严重级别
    uint16_t            errorCode;          ///< 具体错误码
    float               contextValue;       ///< 异常发生时的数据值
    uint32_t            consecutiveCount;   ///< 连续异常次数
    RecoveryAction      recoveryAction;     ///< 推荐恢复动作

    ExceptionEvent()
        : timestampMs(0), sensorId(0)
        , category(ExceptionCategory::COMMUNICATION)
        , severity(ExceptionSeverity::INFO)
        , errorCode(ERR_NONE)
        , contextValue(0.0f), consecutiveCount(0)
        , recoveryAction(RecoveryAction::NONE)
    {}
};

#endif // SENSOR_FEATURE_EXCEPTION
