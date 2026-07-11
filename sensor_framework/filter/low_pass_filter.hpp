/**
 * @file low_pass_filter.hpp
 * @brief 一阶低通滤波 (IIR) — y[n] = α·x[n] + (1-α)·y[n-1]
 * @details 计算复杂度 O(1)，内存仅需保存上一次输出值。
 *          系数α越小平滑效果越强。适用于温度等慢变化信号。
 *          参考: 需求评审报告 第6.1.3节
 */

#pragma once

#include "../feature_config.hpp"
#include "filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_LOWPASS

#include <cmath>

class LowPassFilter : public IFilter {
public:
    /**
     * @param alpha 滤波系数 (0, 1]
     *        alpha → 1.0: 几乎不平滑（快速响应）
     *        alpha → 0.0: 强平滑（响应慢）
     *        典型值: 0.05 ~ 0.3
     */
    explicit LowPassFilter(float alpha = 0.1f)
        : alpha_(alpha), yPrev_(0.0f), initialized_(false)
    {}

    float update(float input) SENSOR_OVERRIDE {
        if (!initialized_) {
            yPrev_ = input;
            initialized_ = true;
        } else {
            yPrev_ = alpha_ * input + (1.0f - alpha_) * yPrev_;
        }

        if (!isFinite(yPrev_)) {
            reset();
            yPrev_ = input;
        }

        lastOutput_ = yPrev_;
        return yPrev_;
    }

    void reset() SENSOR_OVERRIDE {
        initialized_ = false;
        yPrev_ = 0.0f;
        lastOutput_ = 0.0f;
    }

    const char* getName() const SENSOR_OVERRIDE { return "LowPass"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::LOW_PASS; }

    bool setParameter(const char* key, float value) SENSOR_OVERRIDE {
        if (key && key[0] == 'a' && key[1] == '\0') {
            if (value > 0.0f && value <= 1.0f) {
                alpha_ = value;
                return true;
            }
        }
        return false;
    }

    float getParameter(const char* key) const SENSOR_OVERRIDE {
        if (key && key[0] == 'a' && key[1] == '\0') return alpha_;
        if (key && key[0] == 'f' && key[1] == 'c' && key[2] == '\0') {
            // 估算截止频率（近似，假设固定采样率）
            return alpha_ / (2.0f * 3.14159f);
        }
        return 0.0f;
    }

    /**
     * @brief 根据采样周期和期望截止频率计算α值
     * @param dt         采样周期 (秒)
     * @param cutoffFreq 截止频率 (Hz)
     * @return alpha系数
     */
    static float calcAlpha(float dt, float cutoffFreq) {
        float rc = 1.0f / (2.0f * 3.14159f * cutoffFreq);
        float alpha = dt / (rc + dt);
        return clamp(alpha, 0.001f, 1.0f);
    }

private:
    float alpha_;
    float yPrev_;
    bool  initialized_;
};

#endif // SENSOR_FEATURE_FILTER_LOWPASS
