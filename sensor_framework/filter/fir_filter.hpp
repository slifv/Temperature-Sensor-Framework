/**
 * @file fir_filter.hpp
 * @brief FIR低通滤波 — 有限脉冲响应数字滤波器
 * @details 线性相位，稳定性好。基于窗口设计法生成滤波器系数。
 *          阶数编译期确定，适用于需要精确频域特性的场景。
 *          参考: 需求评审报告 第6.1.6节
 */

#pragma once

#include "../feature_config.hpp"
#include "filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_FIR

#include <cstdint>
#include <cstring>
#include <cmath>

/**
 * @class FIRFilter
 * @brief FIR低通滤波器（基于滑动窗口缓冲）
 * @tparam Order FIR阶数（即抽头数-1，滤波器长度为 Order+1）
 */
template <uint8_t Order>
class FIRFilter : public IFilter {
    static_assert(Order >= 2, "FIR order must be >= 2");
    static_assert(Order <= 64, "FIR order too large for embedded platform");

public:
    FIRFilter() { reset(); }

    /**
     * @brief 使用预计算系数初始化
     * @param coefficients 滤波器系数数组（长度 Order+1）
     */
    explicit FIRFilter(const float* coefficients) {
        setCoefficients(coefficients);
        reset();
    }

    /**
     * @brief 使用汉明窗设计低通滤波器
     * @param cutoffFreq     归一化截止频率 (0, 0.5) — 相对于采样率
     * @param generateCoefs  是否自动生成系数
     */
    FIRFilter(float cutoffFreq) {
        designLowPass(cutoffFreq);
        reset();
    }

    float update(float input) SENSOR_OVERRIDE {
        // 将新数据移入缓冲区
        for (uint8_t i = Order; i > 0; --i) {
            buffer_[i] = buffer_[i - 1];
        }
        buffer_[0] = input;

        // FIR卷积
        float output = 0.0f;
        for (uint8_t i = 0; i <= Order; ++i) {
            output += coefs_[i] * buffer_[i];
        }

        if (!isFinite(output)) {
            output = input;
        }

        lastOutput_ = output;
        return output;
    }

    void reset() SENSOR_OVERRIDE {
        for (uint8_t i = 0; i <= Order; ++i) {
            buffer_[i] = 0.0f;
        }
        lastOutput_ = 0.0f;
    }

    const char* getName() const SENSOR_OVERRIDE { return "FIR"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::FIR; }

    /**
     * @brief 设置滤波器系数
     */
    void setCoefficients(const float* coefficients) {
        for (uint8_t i = 0; i <= Order; ++i) {
            coefs_[i] = coefficients[i];
        }
    }

    /**
     * @brief 获取滤波器系数（只读）
     */
    const float* getCoefficients() const { return coefs_; }

    /**
     * @brief 获取FIR阶数
     */
    uint8_t getOrder() const { return Order; }

    /**
     * @brief 使用汉明窗设计低通FIR滤波器
     * @param fc 归一化截止频率 (0, 0.45) — fc = cutoffFreq / sampleRate
     */
    void designLowPass(float fc) {
        if (fc <= 0.0f) fc = 0.05f;
        if (fc >= 0.45f) fc = 0.45f;

        int8_t M = static_cast<int8_t>(Order);
        float sum = 0.0f;

        for (int8_t n = 0; n <= M; ++n) {
            if (n == M / 2) {
                coefs_[n] = 2.0f * fc;  // 中心抽头
            } else {
                float k = static_cast<float>(n) - static_cast<float>(M) / 2.0f;
                coefs_[n] = sinf(2.0f * 3.14159265f * fc * k) / (3.14159265f * k);
                // 汉明窗
                float w = 0.54f - 0.46f * cosf(2.0f * 3.14159265f * static_cast<float>(n) / static_cast<float>(M));
                coefs_[n] *= w;
            }
            sum += coefs_[n];
        }

        // 归一化增益
        if (sum > 0.0f) {
            for (uint8_t i = 0; i <= Order; ++i) {
                coefs_[i] /= sum;
            }
        }
    }

private:
    float buffer_[Order + 1];  ///< 输入延迟线
    float coefs_[Order + 1];   ///< 滤波器系数
};

#endif // SENSOR_FEATURE_FILTER_FIR
