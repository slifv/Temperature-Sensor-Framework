/**
 * @file sensor_profile.hpp
 * @brief Sensor Profile 预设配置 — 四档预定义能力集
 * @details 通过定义 SENSOR_PROFILE_MINIMAL / STANDARD / ADVANCED / CUSTOM
 *          之一来自动配置 feature_config.hpp 中的各项开关。
 *          Minimal:  仅原始采集，无滤波/无总线/无异常
 *          Standard: 基础滤波 + 事件总线
 *          Advanced: 全滤波 + 异常监控 + 熔断
 *          Custom:   用户手动配置 feature_config.hpp
 *          参考: 需求评审报告 第17.3节
 */

#pragma once

// ============================================================
//  Profile 枚举定义
// ============================================================

/// Sensor Profile 级别
enum SensorProfileLevel {
    PROFILE_MINIMAL  = 0,   ///< 极简: 原始采集，ROM < 2KB
    PROFILE_STANDARD = 1,   ///< 标准: 基础滤波 + 事件总线，ROM < 8KB
    PROFILE_ADVANCED = 2,   ///< 高级: 全功能，ROM < 16KB
    PROFILE_CUSTOM   = 3    ///< 自定义: 用户自由组合
};

// ============================================================
//  Minimal Profile (极简)
// ============================================================
#if defined(SENSOR_PROFILE_MINIMAL)

    // 驱动层: 默认全部关闭，按需手动开启一个
    #ifndef SENSOR_FEATURE_SHT30
        #define SENSOR_FEATURE_SHT30            1
    #endif

    // 滤波模块: 全部关闭
    #define SENSOR_FEATURE_FILTER_MEAN              0
    #define SENSOR_FEATURE_FILTER_MEDIAN            0
    #define SENSOR_FEATURE_FILTER_LOWPASS           0
    #define SENSOR_FEATURE_FILTER_KALMAN            0
    #define SENSOR_FEATURE_FILTER_WEIGHTED_MEAN     0
    #define SENSOR_FEATURE_FILTER_FIR               0
    #define SENSOR_FEATURE_FILTER_SLIDING_WINDOW    0
    #define SENSOR_FEATURE_FILTER_TRIMMED_MEAN      0
    #define SENSOR_FEATURE_FILTER_PIPELINE          0

    // 事件总线: 关闭
    #define SENSOR_FEATURE_EVENTBUS                 0

    // 异常模块: 全部关闭
    #define SENSOR_FEATURE_EXCEPTION                0
    #define SENSOR_FEATURE_EXCEPTION_COMM_DETECT    0
    #define SENSOR_FEATURE_EXCEPTION_DATA_DETECT    0
    #define SENSOR_FEATURE_EXCEPTION_FILTER_DETECT  0
    #define SENSOR_FEATURE_CIRCUIT_BREAKER          0
    #define SENSOR_FEATURE_HEARTBEAT                0
    #define SENSOR_FEATURE_EXCEPTION_LOGGER         0

    // 测试框架: 关闭
    #define SENSOR_FEATURE_TEST_FRAMEWORK           0

    // HAL: 按需
    #ifndef SENSOR_FEATURE_HAL_I2C
        #define SENSOR_FEATURE_HAL_I2C              1
    #endif

// ============================================================
//  Standard Profile (标准)
// ============================================================
#elif defined(SENSOR_PROFILE_STANDARD)

    // 驱动层: 开启常用
    #ifndef SENSOR_FEATURE_SHT30
        #define SENSOR_FEATURE_SHT30            1
    #endif

    // 滤波模块: 基础滤波
    #define SENSOR_FEATURE_FILTER_MEAN              1
    #define SENSOR_FEATURE_FILTER_MEDIAN            0
    #define SENSOR_FEATURE_FILTER_LOWPASS           1
    #define SENSOR_FEATURE_FILTER_KALMAN            0
    #define SENSOR_FEATURE_FILTER_WEIGHTED_MEAN     0
    #define SENSOR_FEATURE_FILTER_FIR               0
    #define SENSOR_FEATURE_FILTER_SLIDING_WINDOW    0
    #define SENSOR_FEATURE_FILTER_TRIMMED_MEAN      0
    #define SENSOR_FEATURE_FILTER_PIPELINE          1

    // 事件总线: 开启
    #define SENSOR_FEATURE_EVENTBUS                 1

    // 异常模块: 基础
    #define SENSOR_FEATURE_EXCEPTION                1
    #define SENSOR_FEATURE_EXCEPTION_COMM_DETECT    1
    #define SENSOR_FEATURE_EXCEPTION_DATA_DETECT    1
    #define SENSOR_FEATURE_EXCEPTION_FILTER_DETECT  0
    #define SENSOR_FEATURE_CIRCUIT_BREAKER          0
    #define SENSOR_FEATURE_HEARTBEAT                0
    #define SENSOR_FEATURE_EXCEPTION_LOGGER         0

    // HAL: 常用总线
    #ifndef SENSOR_FEATURE_HAL_I2C
        #define SENSOR_FEATURE_HAL_I2C              1
    #endif

// ============================================================
//  Advanced Profile (高级)
// ============================================================
#elif defined(SENSOR_PROFILE_ADVANCED)

    // 驱动层: 全部开启
    #ifndef SENSOR_FEATURE_SHT30
        #define SENSOR_FEATURE_SHT30            1
    #endif
    #ifndef SENSOR_FEATURE_DS18B20
        #define SENSOR_FEATURE_DS18B20          1
    #endif
    #ifndef SENSOR_FEATURE_NTC
        #define SENSOR_FEATURE_NTC              1
    #endif
    #ifndef SENSOR_FEATURE_TMP117
        #define SENSOR_FEATURE_TMP117           1
    #endif

    // 滤波模块: 全部滤波器
    #define SENSOR_FEATURE_FILTER_MEAN              1
    #define SENSOR_FEATURE_FILTER_MEDIAN            1
    #define SENSOR_FEATURE_FILTER_LOWPASS           1
    #define SENSOR_FEATURE_FILTER_KALMAN            1
    #define SENSOR_FEATURE_FILTER_WEIGHTED_MEAN     1
    #define SENSOR_FEATURE_FILTER_FIR               1
    #define SENSOR_FEATURE_FILTER_SLIDING_WINDOW    1
    #define SENSOR_FEATURE_FILTER_TRIMMED_MEAN      1
    #define SENSOR_FEATURE_FILTER_PIPELINE          1

    // 事件总线: 开启
    #define SENSOR_FEATURE_EVENTBUS                 1

    // 异常模块: 全部开启
    #define SENSOR_FEATURE_EXCEPTION                1
    #define SENSOR_FEATURE_EXCEPTION_COMM_DETECT    1
    #define SENSOR_FEATURE_EXCEPTION_DATA_DETECT    1
    #define SENSOR_FEATURE_EXCEPTION_FILTER_DETECT  1
    #define SENSOR_FEATURE_CIRCUIT_BREAKER          1
    #define SENSOR_FEATURE_HEARTBEAT                1
    #define SENSOR_FEATURE_EXCEPTION_LOGGER         1

    // HAL: 全部总线
    #ifndef SENSOR_FEATURE_HAL_I2C
        #define SENSOR_FEATURE_HAL_I2C              1
    #endif
    #ifndef SENSOR_FEATURE_HAL_SPI
        #define SENSOR_FEATURE_HAL_SPI              1
    #endif
    #ifndef SENSOR_FEATURE_HAL_ONEWIRE
        #define SENSOR_FEATURE_HAL_ONEWIRE          1
    #endif
    #ifndef SENSOR_FEATURE_HAL_ADC
        #define SENSOR_FEATURE_HAL_ADC              1
    #endif

// ============================================================
//  Minimal-8Bit Profile (极简8位)
// ============================================================
#elif defined(SENSOR_PROFILE_MINIMAL_8BIT)

    // 仅1路传感器
    #define SENSOR_FEATURE_SHT30                    1
    #define SENSOR_FEATURE_DS18B20                  0
    #define SENSOR_FEATURE_NTC                      0
    #define SENSOR_FEATURE_TMP117                   0

    // 仅1种定点滤波
    #define SENSOR_FEATURE_FILTER_MEAN              1
    #define SENSOR_FEATURE_FILTER_MEDIAN            0
    #define SENSOR_FEATURE_FILTER_LOWPASS           0
    #define SENSOR_FEATURE_FILTER_KALMAN            0
    #define SENSOR_FEATURE_FILTER_WEIGHTED_MEAN     0
    #define SENSOR_FEATURE_FILTER_FIR               0
    #define SENSOR_FEATURE_FILTER_SLIDING_WINDOW    0
    #define SENSOR_FEATURE_FILTER_TRIMMED_MEAN      0
    #define SENSOR_FEATURE_FILTER_PIPELINE          0

    // 关闭所有高级特性
    #define SENSOR_FEATURE_EVENTBUS                 0
    #define SENSOR_FEATURE_EXCEPTION                0
    #define SENSOR_FEATURE_EXCEPTION_COMM_DETECT    0
    #define SENSOR_FEATURE_EXCEPTION_DATA_DETECT    0
    #define SENSOR_FEATURE_EXCEPTION_FILTER_DETECT  0
    #define SENSOR_FEATURE_CIRCUIT_BREAKER          0
    #define SENSOR_FEATURE_HEARTBEAT                0
    #define SENSOR_FEATURE_EXCEPTION_LOGGER         0
    #define SENSOR_FEATURE_TEST_FRAMEWORK           0

    // 开启定点数
    #define SENSOR_FEATURE_FIXED_POINT_MATH         1

    // 8位严格约束
    #ifndef SENSOR_CONSTRAINT_MAX_WINDOW
        #define SENSOR_CONSTRAINT_MAX_WINDOW        4
    #endif

    // HAL: 仅必要总线
    #ifndef SENSOR_FEATURE_HAL_I2C
        #define SENSOR_FEATURE_HAL_I2C              1
    #endif

// ============================================================
//  Custom Profile (自定义)
// ============================================================
#elif defined(SENSOR_PROFILE_CUSTOM)
    // 不做任何覆盖，完全由用户在 feature_config.hpp 中手动配置
    // feature_config.hpp 中的 #ifndef 将使用用户提前定义的宏值

#endif
