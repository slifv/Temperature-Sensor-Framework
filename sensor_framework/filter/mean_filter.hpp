/**
 * @file mean_filter.hpp
 * @brief 滑动均值滤波 — 基于 SlidingWindowFilter 基类
 * @details 窗口大小N编译期确定，计算复杂度 O(1)（维护累积和）。
 *          参考: 需求评审报告 第6.1.1节
 */

#pragma once

#include "../feature_config.hpp"
#include "sliding_window.hpp"

#if SENSOR_FEATURE_FILTER_MEAN && SENSOR_FEATURE_FILTER_SLIDING_WINDOW

template <uint8_t N>
class MeanFilter : public SlidingWindowFilter<N> {
public:
    const char* getName() const SENSOR_OVERRIDE { return "Mean"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::MEAN; }

    bool setParameter(const char* key, float value) SENSOR_OVERRIDE {
        (void)key;
        (void)value;
        return false;  // 均值滤波器无可调参数
    }

    float getParameter(const char* key) const SENSOR_OVERRIDE {
        if (key && key[0] == 'N' && key[1] == '\0') return static_cast<float>(N);
        return 0.0f;
    }

protected:
    float apply() SENSOR_OVERRIDE {
        float sum = 0.0f;
        const float* window = this->getWindow();
        for (uint8_t i = 0; i < N; ++i) {
            sum += window[i];
        }
        return sum / static_cast<float>(N);
    }
};

#endif // SENSOR_FEATURE_FILTER_MEAN
