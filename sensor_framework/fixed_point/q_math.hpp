/**
 * @file q_math.hpp
 * @brief Q-format 定点数数学库 — 无FPU平台的浮点替代方案
 * @details Q15.16格式（32-bit有符号整数，低16位为小数部分）：
 *          value_int32 = round(real_value × 2^16)
 *          范围: [-32768.0, 32767.99998]  精度: 1/65536 ≈ 0.000015
 *
 *          Q7.8格式（16-bit有符号整数，低8位为小数部分）：
 *          范围: [-128.0, 127.996]  精度: 1/256 ≈ 0.004
 *          适用于8位MCU超低资源场景。
 *
 *          参考: 需求评审报告 第18.2节
 */

#pragma once

#include "../feature_config.hpp"
#include "../compiler_features.hpp"
#include <cstdint>

#if SENSOR_FEATURE_FIXED_POINT_MATH

// ============================================================
//  类型定义
// ============================================================

typedef int32_t q15_16_t;   ///< Q15.16定点数 (32-bit)
typedef int16_t q7_8_t;     ///< Q7.8定点数 (16-bit, 8位平台推荐)

// ============================================================
//  Q15.16 常量
// ============================================================

namespace qmath {

const q15_16_t Q15_16_ONE     = 65536;     ///< 1.0 in Q15.16
const q15_16_t Q15_16_HALF    = 32768;     ///< 0.5 in Q15.16
const q15_16_t Q15_16_ZERO    = 0;
const q15_16_t Q15_16_PI      = 205887;    ///< π ≈ 3.14159
const q15_16_t Q15_16_E       = 178145;    ///< e ≈ 2.71828
const q15_16_t Q15_16_MAX     = 0x7FFFFFFF;
const q15_16_t Q15_16_MIN     = 0x80000000;

// ─── 转换 ───────────────────────────────────────────────

/// float -> Q15.16（带四舍五入）
inline q15_16_t fromFloat(float v) {
    return static_cast<q15_16_t>(v * 65536.0f + (v >= 0.0f ? 0.5f : -0.5f));
}

/// Q15.16 -> float
inline float toFloat(q15_16_t v) {
    return static_cast<float>(v) / 65536.0f;
}

/// int -> Q15.16
inline q15_16_t fromInt(int32_t v) {
    return static_cast<q15_16_t>(v) << 16;
}

/// Q15.16 -> int（截断）
inline int32_t toInt(q15_16_t v) {
    return v >> 16;
}

// ─── 基本运算（防溢出） ─────────────────────────────────

/// 加法（直接使用 int32_t 加法，结果正确）
inline q15_16_t add(q15_16_t a, q15_16_t b) {
    return a + b;
}

/// 减法
inline q15_16_t sub(q15_16_t a, q15_16_t b) {
    return a - b;
}

/// 乘法: (a * b) >> 16（用int64_t防溢出）
inline q15_16_t mul(q15_16_t a, q15_16_t b) {
    return static_cast<q15_16_t>(
        (static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> 16
    );
}

/// 除法: (a << 16) / b（用int64_t防溢出）
inline q15_16_t div(q15_16_t a, q15_16_t b) {
    if (b == 0) return (a >= 0) ? Q15_16_MAX : Q15_16_MIN;
    return static_cast<q15_16_t>(
        (static_cast<int64_t>(a) << 16) / static_cast<int64_t>(b)
    );
}

/// 绝对值
inline q15_16_t abs(q15_16_t v) {
    return (v < 0) ? -v : v;
}

// ─── 统计运算 ───────────────────────────────────────────

/// 均值（窗口内求和后除法）
/// @param data  数据数组
/// @param count 元素个数
inline q15_16_t mean(const q15_16_t* data, uint8_t count) {
    if (count == 0) return 0;
    int32_t sum = 0;
    for (uint8_t i = 0; i < count; ++i) {
        sum += data[i];
    }
    return sum / static_cast<int32_t>(count);
}

/// 中值（原地排序，破环性）
/// @param data  数据数组（会被修改排序）
/// @param count 元素个数
inline q15_16_t median(q15_16_t* data, uint8_t count) {
    if (count == 0) return 0;
    // 冒泡排序（小窗口N≤7最快，无递归栈开销）
    for (uint8_t i = 0; i < count - 1; ++i) {
        bool swapped = false;
        for (uint8_t j = 0; j < count - 1 - i; ++j) {
            if (data[j] > data[j + 1]) {
                q15_16_t tmp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = tmp;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
    uint8_t mid = count / 2;
    if (count % 2 == 1) {
        return data[mid];
    } else {
        return (data[mid - 1] + data[mid]) / 2;
    }
}

/// 截尾均值（Olympic Average）
/// @param data  数据数组（会被排序）
/// @param count 元素个数
/// @param trim  每侧去掉的数量
inline q15_16_t trimmedMean(q15_16_t* data, uint8_t count, uint8_t trim) {
    if (count <= 2 * trim) return 0;
    median(data, count);  // 排序后 data 已有序

    int32_t sum = 0;
    uint8_t effective = count - 2 * trim;
    for (uint8_t i = trim; i < count - trim; ++i) {
        sum += data[i];
    }
    return sum / static_cast<int32_t>(effective);
}

} // namespace qmath

// ============================================================
//  Q15_16 类 — C++11操作符重载包装
// ============================================================

/**
 * @class Q15_16
 * @brief Q15.16定点数C++封装 — 操作符重载，自然使用
 * @details 用法: Q15_16 a(1.5f); Q15_16 b(2.0f); Q15_16 c = a * b;
 *          与 float 接口一致，内部纯整数运算。
 */
class Q15_16 {
public:
    Q15_16() : v_(0) {}
    explicit Q15_16(q15_16_t raw) : v_(raw) {}
    explicit Q15_16(float f) : v_(qmath::fromFloat(f)) {}

    /// 从整数构造的静态工厂（避免与 q15_16_t=int32_t 冲突）
    static Q15_16 fromInt(int32_t i) { return Q15_16(qmath::fromInt(i)); }

    float toFloat() const { return qmath::toFloat(v_); }
    q15_16_t raw() const { return v_; }

    // ─── 算术运算符 ──────────────────────────────────

    Q15_16 operator+(Q15_16 other) const {
        return Q15_16(v_ + other.v_);
    }
    Q15_16 operator-(Q15_16 other) const {
        return Q15_16(v_ - other.v_);
    }
    Q15_16 operator*(Q15_16 other) const {
        return Q15_16(qmath::mul(v_, other.v_));
    }
    Q15_16 operator/(Q15_16 other) const {
        return Q15_16(qmath::div(v_, other.v_));
    }

    Q15_16& operator+=(Q15_16 other) { v_ += other.v_; return *this; }
    Q15_16& operator-=(Q15_16 other) { v_ -= other.v_; return *this; }
    Q15_16& operator*=(Q15_16 other) { v_ = qmath::mul(v_, other.v_); return *this; }
    Q15_16& operator/=(Q15_16 other) { v_ = qmath::div(v_, other.v_); return *this; }

    Q15_16 operator-() const { return Q15_16(-v_); }

    // ─── 比较运算符 ──────────────────────────────────

    bool operator==(Q15_16 other) const { return v_ == other.v_; }
    bool operator!=(Q15_16 other) const { return v_ != other.v_; }
    bool operator<(Q15_16 other) const  { return v_ < other.v_; }
    bool operator>(Q15_16 other) const  { return v_ > other.v_; }
    bool operator<=(Q15_16 other) const { return v_ <= other.v_; }
    bool operator>=(Q15_16 other) const { return v_ >= other.v_; }

    /// 转换为float（隐式，方便printf）
    operator float() const { return toFloat(); }

private:
    q15_16_t v_;
};

// ============================================================
//  Q7.8 类型（16-bit，8位MCU推荐）
// ============================================================

/**
 * @class Q7_8
 * @brief Q7.8定点数C++封装（16-bit，适合8位MCU）
 * @details 范围: [-128.0, 127.996]  精度: 1/256 ≈ 0.004
 */
class Q7_8 {
public:
    Q7_8() : v_(0) {}
    explicit Q7_8(q7_8_t raw) : v_(raw) {}
    explicit Q7_8(float f) : v_(static_cast<q7_8_t>(f * 256.0f + (f >= 0 ? 0.5f : -0.5f))) {}

    float toFloat() const { return static_cast<float>(v_) / 256.0f; }
    q7_8_t raw() const { return v_; }

    Q7_8 operator+(Q7_8 other) const { return Q7_8(static_cast<q7_8_t>(v_ + other.v_)); }
    Q7_8 operator-(Q7_8 other) const { return Q7_8(static_cast<q7_8_t>(v_ - other.v_)); }
    Q7_8 operator*(Q7_8 other) const {
        return Q7_8(static_cast<q7_8_t>(
            (static_cast<int32_t>(v_) * static_cast<int32_t>(other.v_)) >> 8
        ));
    }

    bool operator<(Q7_8 other) const { return v_ < other.v_; }
    bool operator>(Q7_8 other) const { return v_ > other.v_; }

private:
    q7_8_t v_;
};

#endif // SENSOR_FEATURE_FIXED_POINT_MATH
