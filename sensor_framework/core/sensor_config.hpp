/**
 * @file sensor_config.hpp
 * @brief 传感器配置结构 — SensorType/SensorStatus/SensorConfig/SensorData
 * @details 定义框架核心数据结构。所有传感器类型、状态、配置、
 *          以及采样数据包都在此文件中定义。
 *          参考: 需求评审报告 第7.1节, 第10.1节
 */

#pragma once

#include <cstdint>

// ============================================================
//  传感器类型枚举
// ============================================================

/// 传感器物理量类型
enum class SensorType : uint8_t {
    UNKNOWN         = 0,
    TEMPERATURE     = 1,    ///< 温度
    HUMIDITY        = 2,    ///< 湿度
    PRESSURE        = 3,    ///< 气压
    LIGHT           = 4,    ///< 光照
    ACCELEROMETER   = 5,    ///< 加速度
    GYROSCOPE       = 6,    ///< 陀螺仪
    MAGNETOMETER    = 7,    ///< 磁力计
    PROXIMITY       = 8,    ///< 接近
    GAS             = 9,    ///< 气体
    VOC             = 10,   ///< 挥发性有机物
    CURRENT         = 11,   ///< 电流
    VOLTAGE         = 12,   ///< 电压
    CUSTOM_BASE     = 100   ///< 自定义类型起始值
};

// ============================================================
//  传感器状态枚举
// ============================================================

/// 传感器运行状态
enum class SensorStatus : uint8_t {
    UNINIT      = 0,    ///< 未初始化
    READY       = 1,    ///< 就绪（已初始化，未启动）
    RUNNING     = 2,    ///< 运行中（正在采样）
    STOPPED     = 3,    ///< 已停止
    ERROR       = 4,    ///< 通用错误
    COMM_ERROR  = 5,    ///< 通信错误
    OFFLINE     = 6,    ///< 离线
    DEGRADED    = 7,    ///< 降级运行
    DESTROYED   = 8     ///< 已销毁
};

// ============================================================
//  采样模式枚举
// ============================================================

/// 采样模式
enum class SampleMode : uint8_t {
    PERIODIC    = 0,    ///< 定时采样（周期性）
    TRIGGERED   = 1,    ///< 触发采样（事件驱动）
    CONTINUOUS  = 2     ///< 连续采样（高速数据流）
};

// ============================================================
//  传感器配置结构
// ============================================================

/// 传感器配置参数
struct SensorConfig {
    float       sampleRateHz;       ///< 采样率 (Hz)
    SampleMode  sampleMode;         ///< 采样模式
    uint16_t    sampleIntervalMs;   ///< 采样间隔 (ms) — 与 sampleRateHz 二选一
    float       rangeMin;           ///< 量程下限
    float       rangeMax;           ///< 量程上限
    uint8_t     resolution;         ///< 分辨率（位数）
    bool        autoStart;          ///< 初始化后是否自动启动
    bool        enableFiltering;    ///< 是否启用滤波

    // 配置预设工厂方法
    static SensorConfig defaultConfig() {
        SensorConfig cfg;
        cfg.sampleRateHz        = 10.0f;
        cfg.sampleMode          = SampleMode::PERIODIC;
        cfg.sampleIntervalMs    = 100;
        cfg.rangeMin            = -40.0f;
        cfg.rangeMax            = 125.0f;
        cfg.resolution          = 16;
        cfg.autoStart           = false;
        cfg.enableFiltering     = true;
        return cfg;
    }
};

// ============================================================
//  传感器数据包结构
// ============================================================

/// 传感器采样数据包
struct SensorData {
    uint32_t        sensorId;       ///< 传感器唯一ID
    SensorType      type;           ///< 传感器类型
    float           value;          ///< 滤波后数值
    float           rawValue;       ///< 原始数值
    uint32_t        timestampMs;    ///< 毫秒时间戳（自系统启动）
    SensorStatus    status;         ///< 传感器状态
    uint8_t         channel;        ///< 通道号（多通道传感器）
    uint16_t        sequenceNum;    ///< 采样序列号
    uint16_t        reserved;       ///< 保留对齐

    SensorData()
        : sensorId(0), type(SensorType::UNKNOWN)
        , value(0.0f), rawValue(0.0f)
        , timestampMs(0), status(SensorStatus::UNINIT)
        , channel(0), sequenceNum(0), reserved(0)
    {}
};

/// 温度传感器特化数据（继承SensorData + 温度专用字段）
struct TemperatureData : public SensorData {
    TemperatureData() {
        type = SensorType::TEMPERATURE;
    }
    // 温度传感器无额外字段（保留扩展点）
};
