/**
 * @file sensor_framework.hpp
 * @brief 跨MCU平台Sensor框架 — 统一入口头文件
 * @details 包含此单个头文件即可使用框架全部功能。
 *          实际编译行为由 feature_config.hpp 中的特性开关控制。
 *
 * 使用方式:
 *   1. 通过 CMake -DSENSOR_PROFILE=STANDARD 选择Profile
 *   2. 或直接修改 feature_config.hpp 手动配置
 *   3. #include "sensor_framework.hpp" 即可
 *
 * 版本: v1.0
 * 最低C++标准: C++11
 * 支持平台: 8位(AVR/STM8/PIC16/8051) / 16位(MSP430/PIC24) /
 *           32位(STM32/ESP32/nRF52/GD32)
 */

#pragma once

// ============================================================
//  基础设施
// ============================================================
#include "compiler_features.hpp"
#include "feature_config.hpp"
#include "sensor_profile.hpp"

// ============================================================
//  工具类
// ============================================================
#include "utils/ring_buffer.hpp"
#include "utils/static_vector.hpp"
#include "utils/time_provider.hpp"

// ============================================================
//  硬件抽象层
// ============================================================
#include "hal/hal_interface.hpp"
#include "hal/platform/platform_selector.hpp"

#if SENSOR_FEATURE_HAL_I2C
    #include "hal/bus/i2c_bus.hpp"
#endif
#if SENSOR_FEATURE_HAL_SPI
    #include "hal/bus/spi_bus.hpp"
#endif
#if SENSOR_FEATURE_HAL_ADC
    #include "hal/bus/adc_channel.hpp"
#endif

// ============================================================
//  滤波引擎
// ============================================================
#include "filter/filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_PIPELINE
    #include "filter/filter_pipeline.hpp"
#endif
#if SENSOR_FEATURE_FILTER_SLIDING_WINDOW
    #include "filter/sliding_window.hpp"
#endif
#if SENSOR_FEATURE_FILTER_MEAN
    #include "filter/mean_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_MEDIAN
    #include "filter/median_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_LOWPASS
    #include "filter/low_pass_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_KALMAN
    #include "filter/kalman_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_WEIGHTED_MEAN
    #include "filter/weighted_mean_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_FIR
    #include "filter/fir_filter.hpp"
#endif
#if SENSOR_FEATURE_FILTER_TRIMMED_MEAN
    #include "filter/trimmed_mean_filter.hpp"
#endif

// ============================================================
//  框架核心
// ============================================================
#include "core/sensor_config.hpp"
#include "core/sensor_base.hpp"
#include "core/sensor_manager.hpp"
#include "core/event_bus.hpp"

// ============================================================
//  数据分发
// ============================================================
#include "distribution/sensor_data.hpp"
#include "distribution/data_subscriber.hpp"
#include "distribution/data_publisher.hpp"

// ============================================================
//  异常监控
// ============================================================
#if SENSOR_FEATURE_EXCEPTION
    #include "exception/exception_types.hpp"
    #include "exception/exception_handler.hpp"

    #if SENSOR_FEATURE_EXCEPTION_COMM_DETECT || \
        SENSOR_FEATURE_EXCEPTION_DATA_DETECT || \
        SENSOR_FEATURE_EXCEPTION_FILTER_DETECT
        #include "exception/exception_detector.hpp"
    #endif

    #if SENSOR_FEATURE_CIRCUIT_BREAKER
        #include "exception/circuit_breaker.hpp"
    #endif
    #if SENSOR_FEATURE_HEARTBEAT
        #include "exception/heartbeat_monitor.hpp"
    #endif
    #if SENSOR_FEATURE_EXCEPTION_LOGGER
        #include "exception/exception_logger.hpp"
    #endif

    #include "exception/safe_sensor_wrapper.hpp"
#endif

// ============================================================
//  传感器驱动
// ============================================================
#if SENSOR_FEATURE_SHT30
    #include "sensors/temperature/sht30.hpp"
#endif
#if SENSOR_FEATURE_DS18B20
    #include "sensors/temperature/ds18b20.hpp"
#endif
#if SENSOR_FEATURE_NTC
    #include "sensors/temperature/ntc_thermistor.hpp"
#endif

// ============================================================
//  应用层
// ============================================================
#if SENSOR_FEATURE_EVENTBUS
    #include "app/gui_display.hpp"
    #include "app/mqtt_sender.hpp"
    #include "app/business_logic.hpp"
#endif

// ============================================================
//  定点数数学库（按需）
// ============================================================
#if SENSOR_FEATURE_FIXED_POINT_MATH
    #include "fixed_point/q_math.hpp"
    #include "fixed_point/fixed_mean_filter.hpp"
#endif

// ============================================================
//  版本信息
// ============================================================

#define SENSOR_FRAMEWORK_VERSION_MAJOR  1
#define SENSOR_FRAMEWORK_VERSION_MINOR  0
#define SENSOR_FRAMEWORK_VERSION_PATCH  0

/// 版本字符串: "1.0.0"
#define SENSOR_FRAMEWORK_VERSION_STRING "1.0.0"
