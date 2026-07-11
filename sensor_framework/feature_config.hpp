/**
 * @file feature_config.hpp
 * @brief 编译时特性配置主开关 — 框架裁剪入口
 * @details 通过宏定义控制每个模块的编译行为。每个特性宏为 0(禁用) 或 1(启用)。
 *          支持通过 CMake -D 选项或直接修改此文件进行配置。
 *          裁剪后零残留: 未启用的模块不产生任何代码段和数据段。
 *          参考: 需求评审报告 第17.2节
 */

#pragma once

#include "compiler_features.hpp"

// ============================================================
//  1. Profile 选择 (优先级最高)
// ============================================================
// 通过 CMake -DSENSOR_PROFILE_XXX 或直接定义宏来选择Profile
// Profile会自动覆盖下面的各项配置
// 如果定义了 SENSOR_PROFILE_CUSTOM 或不定义任何PROFILE宏，
// 则完全由下面的手动配置决定
#include "sensor_profile.hpp"

// ============================================================
//  2. 传感器驱动层
// ============================================================
#ifndef SENSOR_FEATURE_DS18B20
    #define SENSOR_FEATURE_DS18B20      0   ///< DS18B20 OneWire 温度传感器
#endif
#ifndef SENSOR_FEATURE_SHT30
    #define SENSOR_FEATURE_SHT30        1   ///< SHT30 I2C 温度传感器
#endif
#ifndef SENSOR_FEATURE_NTC
    #define SENSOR_FEATURE_NTC          0   ///< NTC热敏电阻 ADC
#endif
#ifndef SENSOR_FEATURE_TMP117
    #define SENSOR_FEATURE_TMP117       0   ///< TMP117 高精度I2C
#endif

// ============================================================
//  3. 滤波器模块（每个滤波器独立开关）
// ============================================================
#ifndef SENSOR_FEATURE_FILTER_MEAN
    #define SENSOR_FEATURE_FILTER_MEAN          1   ///< 滑动均值滤波
#endif
#ifndef SENSOR_FEATURE_FILTER_MEDIAN
    #define SENSOR_FEATURE_FILTER_MEDIAN        1   ///< 中值滤波
#endif
#ifndef SENSOR_FEATURE_FILTER_LOWPASS
    #define SENSOR_FEATURE_FILTER_LOWPASS       1   ///< 一阶低通滤波
#endif
#ifndef SENSOR_FEATURE_FILTER_KALMAN
    #define SENSOR_FEATURE_FILTER_KALMAN        1   ///< 一维卡尔曼滤波
#endif
#ifndef SENSOR_FEATURE_FILTER_WEIGHTED_MEAN
    #define SENSOR_FEATURE_FILTER_WEIGHTED_MEAN 0   ///< 加权移动平均
#endif
#ifndef SENSOR_FEATURE_FILTER_FIR
    #define SENSOR_FEATURE_FILTER_FIR           0   ///< FIR低通滤波
#endif
#ifndef SENSOR_FEATURE_FILTER_SLIDING_WINDOW
    #define SENSOR_FEATURE_FILTER_SLIDING_WINDOW 1  ///< ★ 滑动窗口滤波框架基类
#endif
#ifndef SENSOR_FEATURE_FILTER_TRIMMED_MEAN
    #define SENSOR_FEATURE_FILTER_TRIMMED_MEAN  1   ///< ★ 截尾均值滤波(Olympic Average)
#endif

// ============================================================
//  4. 滤波器 Pipeline (依赖至少一个滤波器模块开启)
// ============================================================
#ifndef SENSOR_FEATURE_FILTER_PIPELINE
    #define SENSOR_FEATURE_FILTER_PIPELINE      1   ///< 滤波器链Pipeline
#endif

// ============================================================
//  5. 事件总线 / 发布-订阅
// ============================================================
#ifndef SENSOR_FEATURE_EVENTBUS
    #define SENSOR_FEATURE_EVENTBUS             1   ///< 事件总线
#endif

// ============================================================
//  6. 异常监控与处理（每个子模块独立开关）
// ============================================================
#ifndef SENSOR_FEATURE_EXCEPTION
    #define SENSOR_FEATURE_EXCEPTION                1   ///< 异常监控总开关
#endif
#ifndef SENSOR_FEATURE_EXCEPTION_COMM_DETECT
    #define SENSOR_FEATURE_EXCEPTION_COMM_DETECT    1   ///< 通信异常检测
#endif
#ifndef SENSOR_FEATURE_EXCEPTION_DATA_DETECT
    #define SENSOR_FEATURE_EXCEPTION_DATA_DETECT    1   ///< 数据异常检测
#endif
#ifndef SENSOR_FEATURE_EXCEPTION_FILTER_DETECT
    #define SENSOR_FEATURE_EXCEPTION_FILTER_DETECT  1   ///< 滤波异常检测
#endif
#ifndef SENSOR_FEATURE_CIRCUIT_BREAKER
    #define SENSOR_FEATURE_CIRCUIT_BREAKER          1   ///< 熔断保护器
#endif
#ifndef SENSOR_FEATURE_HEARTBEAT
    #define SENSOR_FEATURE_HEARTBEAT                0   ///< 心跳监控
#endif
#ifndef SENSOR_FEATURE_EXCEPTION_LOGGER
    #define SENSOR_FEATURE_EXCEPTION_LOGGER         1   ///< 异常日志记录
#endif

// ============================================================
//  7. HAL 平台适配
// ============================================================
#ifndef SENSOR_FEATURE_HAL_I2C
    #define SENSOR_FEATURE_HAL_I2C              1   ///< I2C总线
#endif
#ifndef SENSOR_FEATURE_HAL_SPI
    #define SENSOR_FEATURE_HAL_SPI              0   ///< SPI总线
#endif
#ifndef SENSOR_FEATURE_HAL_ONEWIRE
    #define SENSOR_FEATURE_HAL_ONEWIRE          0   ///< OneWire总线
#endif
#ifndef SENSOR_FEATURE_HAL_ADC
    #define SENSOR_FEATURE_HAL_ADC              0   ///< ADC通道
#endif

// ============================================================
//  8. 测试框架（仅在测试构建中开启）
// ============================================================
#ifndef SENSOR_FEATURE_TEST_FRAMEWORK
    #define SENSOR_FEATURE_TEST_FRAMEWORK       0   ///< 测试支撑框架
#endif
#ifndef SENSOR_FEATURE_HIL_BRIDGE
    #define SENSOR_FEATURE_HIL_BRIDGE           0   ///< HIL桥接器
#endif

// ============================================================
//  9. 定点数数学库（8位/16位无FPU平台推荐启用）
// ============================================================
#ifndef SENSOR_FEATURE_FIXED_POINT_MATH
    #define SENSOR_FEATURE_FIXED_POINT_MATH     0   ///< 定点数Q15.16数学库
#endif

// ============================================================
//  10. 平台约束配置
// ============================================================
#ifndef SENSOR_CONSTRAINT_MAX_WINDOW
    #if SENSOR_MCU_8BIT
        #define SENSOR_CONSTRAINT_MAX_WINDOW    4   ///< 8位平台窗口上限
    #elif SENSOR_MCU_16BIT
        #define SENSOR_CONSTRAINT_MAX_WINDOW    8   ///< 16位平台窗口上限
    #else
        #define SENSOR_CONSTRAINT_MAX_WINDOW    32  ///< 32位平台窗口上限
    #endif
#endif

#ifndef SENSOR_CONSTRAINT_MAX_FILTERS
    #if SENSOR_MCU_8BIT
        #define SENSOR_CONSTRAINT_MAX_FILTERS   2   ///< 8位平台滤波器数量上限
    #elif SENSOR_MCU_16BIT
        #define SENSOR_CONSTRAINT_MAX_FILTERS   4
    #else
        #define SENSOR_CONSTRAINT_MAX_FILTERS   8
    #endif
#endif

#ifndef SENSOR_CONSTRAINT_MAX_SUBSCRIBERS
    #if SENSOR_MCU_8BIT
        #define SENSOR_CONSTRAINT_MAX_SUBSCRIBERS 2
    #elif SENSOR_MCU_16BIT
        #define SENSOR_CONSTRAINT_MAX_SUBSCRIBERS 4
    #else
        #define SENSOR_CONSTRAINT_MAX_SUBSCRIBERS 8
    #endif
#endif

#ifndef SENSOR_CONSTRAINT_MAX_SENSORS
    #if SENSOR_MCU_8BIT
        #define SENSOR_CONSTRAINT_MAX_SENSORS   1
    #elif SENSOR_MCU_16BIT
        #define SENSOR_CONSTRAINT_MAX_SENSORS   3
    #else
        #define SENSOR_CONSTRAINT_MAX_SENSORS   8
    #endif
#endif

// ============================================================
//  11. 调试与诊断
// ============================================================
#ifndef SENSOR_FEATURE_PERF_STATS
    #define SENSOR_FEATURE_PERF_STATS           0   ///< 性能统计
#endif
#ifndef SENSOR_FEATURE_TRACE_LOG
    #define SENSOR_FEATURE_TRACE_LOG            0   ///< 跟踪日志
#endif

// ============================================================
//  12. 聚合开关：便捷条件编译宏
// ============================================================

/// 是否有任何滤波器启用
#define SENSOR_HAS_ANY_FILTER ( \
    SENSOR_FEATURE_FILTER_MEAN || \
    SENSOR_FEATURE_FILTER_MEDIAN || \
    SENSOR_FEATURE_FILTER_LOWPASS || \
    SENSOR_FEATURE_FILTER_KALMAN || \
    SENSOR_FEATURE_FILTER_WEIGHTED_MEAN || \
    SENSOR_FEATURE_FILTER_FIR || \
    SENSOR_FEATURE_FILTER_TRIMMED_MEAN \
)

/// 是否有任何异常检测器启用
#define SENSOR_HAS_ANY_EXCEPTION_DETECTOR ( \
    SENSOR_FEATURE_EXCEPTION_COMM_DETECT || \
    SENSOR_FEATURE_EXCEPTION_DATA_DETECT || \
    SENSOR_FEATURE_EXCEPTION_FILTER_DETECT \
)

// ============================================================
//  13. 条件编译便捷宏
// ============================================================

#if SENSOR_FEATURE_EXCEPTION
    #define IF_EXCEPTION_ENABLED(code)   code
    #define IF_EXCEPTION_DISABLED(code)
#else
    #define IF_EXCEPTION_ENABLED(code)
    #define IF_EXCEPTION_DISABLED(code)  code
#endif

#if SENSOR_FEATURE_EVENTBUS
    #define IF_EVENTBUS_ENABLED(code)    code
    #define IF_EVENTBUS_DISABLED(code)
#else
    #define IF_EVENTBUS_ENABLED(code)
    #define IF_EVENTBUS_DISABLED(code)   code
#endif

#if SENSOR_FEATURE_FILTER_PIPELINE
    #define IF_FILTER_PIPELINE_ENABLED(code)  code
    #define IF_FILTER_PIPELINE_DISABLED(code)
#else
    #define IF_FILTER_PIPELINE_ENABLED(code)
    #define IF_FILTER_PIPELINE_DISABLED(code) code
#endif

#if SENSOR_FEATURE_CIRCUIT_BREAKER
    #define IF_CIRCUIT_BREAKER_ENABLED(code)  code
    #define IF_CIRCUIT_BREAKER_DISABLED(code)
#else
    #define IF_CIRCUIT_BREAKER_ENABLED(code)
    #define IF_CIRCUIT_BREAKER_DISABLED(code) code
#endif

#if SENSOR_FEATURE_EXCEPTION_LOGGER
    #define IF_EXCEPTION_LOGGER_ENABLED(code)  code
    #define IF_EXCEPTION_LOGGER_DISABLED(code)
#else
    #define IF_EXCEPTION_LOGGER_ENABLED(code)
    #define IF_EXCEPTION_LOGGER_DISABLED(code) code
#endif
