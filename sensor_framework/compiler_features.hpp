/**
 * @file compiler_features.hpp
 * @brief C++版本检测宏体系 — 兼容C++11/14/17/20多标准编译
 * @details 根据 __cplusplus 宏自动检测编译器支持的C++标准版本，
 *          提供统一的能力检测宏和兼容性适配宏。
 *          参考: 需求评审报告 附录C.2
 */

#pragma once

// ============================================================
//  C++标准版本检测
// ============================================================

// C++11 (最低要求)
#if __cplusplus >= 201103L
    #define SENSOR_HAS_CPP11          1
#else
    #error "Sensor Framework requires C++11 or later"
#endif

// C++14
#if __cplusplus >= 201402L
    #define SENSOR_HAS_CPP14          1
    #define SENSOR_HAS_MAKE_UNIQUE    1
    #define SENSOR_HAS_CONSTEXPR14    1
#else
    #define SENSOR_HAS_CPP14          0
    #define SENSOR_HAS_MAKE_UNIQUE    0
    #define SENSOR_HAS_CONSTEXPR14    0
#endif

// C++17
#if __cplusplus >= 201703L
    #define SENSOR_HAS_CPP17              1
    #define SENSOR_HAS_IF_CONSTEXPR       1
    #define SENSOR_HAS_OPTIONAL           1
    #define SENSOR_HAS_STRING_VIEW        1
    #define SENSOR_HAS_NODISCARD          1
    #define SENSOR_HAS_INLINE_VARIABLES   1
#else
    #define SENSOR_HAS_CPP17              0
    #define SENSOR_HAS_IF_CONSTEXPR       0
    #define SENSOR_HAS_OPTIONAL           0
    #define SENSOR_HAS_STRING_VIEW        0
    #define SENSOR_HAS_NODISCARD          0
    #define SENSOR_HAS_INLINE_VARIABLES   0
#endif

// C++20
#if __cplusplus >= 202002L
    #define SENSOR_HAS_CPP20          1
    #define SENSOR_HAS_CONCEPTS       1
    #define SENSOR_HAS_CONSTEXPR20    1
#else
    #define SENSOR_HAS_CPP20          0
    #define SENSOR_HAS_CONCEPTS       0
    #define SENSOR_HAS_CONSTEXPR20    0
#endif

// ============================================================
//  编译器检测
// ============================================================

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC)
    #define SENSOR_COMPILER_GCC     1
    #define SENSOR_COMPILER_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
    #define SENSOR_COMPILER_GCC     0
#endif

#if defined(__clang__)
    #define SENSOR_COMPILER_CLANG   1
    #define SENSOR_COMPILER_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
    #define SENSOR_COMPILER_CLANG   0
#endif

#if defined(__IAR_SYSTEMS_ICC__)
    #define SENSOR_COMPILER_IAR     1
#else
    #define SENSOR_COMPILER_IAR     0
#endif

#if defined(__SDCC)
    #define SENSOR_COMPILER_SDCC    1
#else
    #define SENSOR_COMPILER_SDCC    0
#endif

// ============================================================
//  平台特性检测
// ============================================================

// FPU检测
#if defined(__ARM_FP) || defined(__FPU_PRESENT) || defined(__FPU_USED)
    #define SENSOR_HAS_FPU          1
#else
    #define SENSOR_HAS_FPU          0
#endif

// RTTI检测
#if defined(__cpp_rtti) || !defined(__GXX_RTTI)
    #define SENSOR_RTTI_ENABLED     1
#else
    #define SENSOR_RTTI_ENABLED     0
#endif

#if defined(__EXCEPTIONS) || !defined(_HAS_EXCEPTIONS)
    #define SENSOR_EXCEPTIONS_ENABLED 1
#else
    #define SENSOR_EXCEPTIONS_ENABLED 0
#endif

// MCU位宽检测
#if defined(__AVR__) || defined(__SDCC_mcs51) || defined(__STM8__)
    #define SENSOR_MCU_8BIT         1
#else
    #define SENSOR_MCU_8BIT         0
#endif

#if defined(__MSP430__) || defined(__PIC24F__) || defined(__dsPIC33__)
    #define SENSOR_MCU_16BIT        1
#else
    #define SENSOR_MCU_16BIT        0
#endif

#if defined(__ARM_ARCH) || defined(__XTENSA__) || defined(__riscv)
    #define SENSOR_MCU_32BIT        1
#else
    #define SENSOR_MCU_32BIT        0
#endif

// ============================================================
//  C++11兼容性适配宏
// ============================================================

/**
 * @def SENSOR_MAKE_UNIQUE(T, ...)
 * @brief std::make_unique 的C++11替代
 *        C++14+ 使用 std::make_unique, C++11 降级为 new
 */
#if SENSOR_HAS_MAKE_UNIQUE
    #include <memory>
    #define SENSOR_MAKE_UNIQUE(T, ...) std::make_unique<T>(__VA_ARGS__)
#else
    #include <memory>
    #define SENSOR_MAKE_UNIQUE(T, ...) std::unique_ptr<T>(new T(__VA_ARGS__))
#endif

/**
 * @def SENSOR_IF_CONSTEXPR(cond)
 * @brief if constexpr 的兼容宏
 *        C++17 使用 if constexpr, C++11/14 降级为普通 if (依赖编译器优化)
 */
#if SENSOR_HAS_IF_CONSTEXPR
    #define SENSOR_IF_CONSTEXPR(cond) if constexpr (cond)
#else
    #define SENSOR_IF_CONSTEXPR(cond) if (cond)
#endif

/**
 * @def SENSOR_NODISCARD
 * @brief [[nodiscard]] 属性兼容
 */
#if SENSOR_HAS_NODISCARD
    #define SENSOR_NODISCARD [[nodiscard]]
#else
    #if SENSOR_COMPILER_GCC || SENSOR_COMPILER_CLANG
        #define SENSOR_NODISCARD __attribute__((warn_unused_result))
    #else
        #define SENSOR_NODISCARD
    #endif
#endif

/**
 * @def SENSOR_OVERRIDE
 * @brief override 关键字 (C++11起支持)
 */
#if SENSOR_HAS_CPP11
    #define SENSOR_OVERRIDE override
#else
    #define SENSOR_OVERRIDE
#endif

/**
 * @def SENSOR_NOEXCEPT
 * @brief noexcept 兼容 (C++11起支持)
 */
#if SENSOR_HAS_CPP11
    #define SENSOR_NOEXCEPT noexcept
#else
    #define SENSOR_NOEXCEPT throw()
#endif

/**
 * @def SENSOR_DELETE_FUNC
 * @brief = delete 兼容 (C++11起支持)
 */
#if SENSOR_HAS_CPP11
    #define SENSOR_DELETE_FUNC = delete
#else
    #define SENSOR_DELETE_FUNC
#endif

/**
 * @def SENSOR_FALLTHROUGH
 * @brief [[fallthrough]] 兼容
 */
#if SENSOR_HAS_CPP17
    #define SENSOR_FALLTHROUGH [[fallthrough]]
#else
    #if SENSOR_COMPILER_GCC && (__GNUC__ >= 7)
        #define SENSOR_FALLTHROUGH __attribute__((fallthrough))
    #else
        #define SENSOR_FALLTHROUGH /* fallthrough */
    #endif
#endif

// ============================================================
//  辅助宏
// ============================================================

/// 未使用的变量抑制警告
#define SENSOR_UNUSED(x) (void)(x)

/// 数组元素个数 (编译期)
#define SENSOR_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
