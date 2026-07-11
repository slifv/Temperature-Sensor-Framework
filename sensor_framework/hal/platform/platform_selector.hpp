/**
 * @file platform_selector.hpp
 * @brief 编译时平台选择 — 通过预编译宏自动选择对应HAL实现
 * @details 根据 MCU_STM32 / MCU_ESP32 / MCU_NRF52 / MCU_GD32 / MCU_AVR 等
 *          宏定义自动 include 对应平台的HAL实现，并通过 using 别名统一引用。
 *          参考: 需求评审报告 第8.3节
 */

#pragma once

#include "../../feature_config.hpp"
#include "../../compiler_features.hpp"

// ============================================================
//  32位MCU平台
// ============================================================

#if defined(MCU_STM32)
    // STM32 HAL: 使用 STM32Cube HAL 库
    // #include "stm32/stm32_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "STM32"
    #define SENSOR_PLATFORM_32BIT

#elif defined(MCU_ESP32)
    // ESP32: 使用 ESP-IDF
    // #include "esp32/esp32_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "ESP32"
    #define SENSOR_PLATFORM_32BIT

#elif defined(MCU_NRF52)
    // nRF52: 使用 nRF SDK
    // #include "nrf52/nrf52_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "nRF52"
    #define SENSOR_PLATFORM_32BIT

#elif defined(MCU_GD32)
    // GD32: 使用 GD32 HAL
    // #include "gd32/gd32_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "GD32"
    #define SENSOR_PLATFORM_32BIT

// ============================================================
//  16位MCU平台
// ============================================================

#elif defined(MCU_MSP430)
    // MSP430: 使用 MSP430-GCC
    // #include "msp430/msp430_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "MSP430"
    #define SENSOR_PLATFORM_16BIT

#elif defined(MCU_PIC24)
    // PIC24/dsPIC33: 使用 XC16
    // #include "pic24/pic24_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "PIC24"
    #define SENSOR_PLATFORM_16BIT

// ============================================================
//  8位MCU平台
// ============================================================

#elif defined(MCU_AVR)
    // AVR ATmega/ATtiny: 使用 AVR-GCC
    // #include "avr/avr_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "AVR"
    #define SENSOR_PLATFORM_8BIT

#elif defined(MCU_STM8)
    // STM8: 使用 IAR EWSTM8 / SDCC
    // #include "stm8/stm8_hal.hpp"
    #define SENSOR_PLATFORM_NAME    "STM8"
    #define SENSOR_PLATFORM_8BIT

#elif defined(MCU_PIC16)
    // PIC16: 使用 XC8 (C only, C binding)
    // #include "pic16/pic16_hal.h"
    #define SENSOR_PLATFORM_NAME    "PIC16"
    #define SENSOR_PLATFORM_8BIT

#elif defined(MCU_8051)
    // 8051: 使用 SDCC (C only, C binding)
    // #include "8051/8051_hal.h"
    #define SENSOR_PLATFORM_NAME    "8051"
    #define SENSOR_PLATFORM_8BIT

#else
    // 未指定平台时使用PC模拟环境（用于单元测试）
    #define SENSOR_PLATFORM_NAME    "PC-SIM"
    #define SENSOR_PLATFORM_32BIT
#endif

// ============================================================
//  平台能力宏
// ============================================================

/// 平台是否支持浮点硬件 (FPU)
#if defined(SENSOR_PLATFORM_8BIT) || defined(SENSOR_PLATFORM_16BIT)
    // 8/16位平台通常无FPU，除非明确声明
    #ifdef SENSOR_PLATFORM_HAS_FPU
        #define SENSOR_PLATFORM_FPU  1
    #else
        #define SENSOR_PLATFORM_FPU  0
    #endif
#else
    // 32位平台通常有FPU（Cortex-M4F/M7，ESP32，nRF52）
    #define SENSOR_PLATFORM_FPU  1
#endif

/// 平台推荐使用定点数
#if SENSOR_PLATFORM_FPU == 0
    #define SENSOR_PLATFORM_RECOMMEND_FIXED_POINT  1
#else
    #define SENSOR_PLATFORM_RECOMMEND_FIXED_POINT  0
#endif

// ============================================================
//  平台类型别名
// ============================================================

/// 平台推荐使用的数值类型
#if SENSOR_PLATFORM_RECOMMEND_FIXED_POINT
    // 无FPU平台：推荐使用定点数类型
    typedef int32_t sensor_float_t;     ///< Q15.16 定点数
#else
    // 有FPU平台：直接使用 float
    typedef float   sensor_float_t;
#endif
