/**
 * @file median_filter.hpp
 * @brief 中值滤波 — 基于 SlidingWindowFilter 基类
 * @details 对窗口内N个值排序后取中位数，有效抑制偶发脉冲噪声。
 *          参考: 需求评审报告 第6.1.2节
 */

#pragma once

#include "../feature_config.hpp"
#include "sliding_window.hpp"

#if SENSOR_FEATURE_FILTER_MEDIAN && SENSOR_FEATURE_FILTER_SLIDING_WINDOW

template <uint8_t N>
class MedianFilter : public SlidingWindowFilter<N> {
public:
    const char* getName() const SENSOR_OVERRIDE { return "Median"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::MEDIAN; }

    bool setParameter(const char* key, float value) SENSOR_OVERRIDE {
        (void)key;
        (void)value;
        return false;
    }

    float getParameter(const char* key) const SENSOR_OVERRIDE {
        if (key && key[0] == 'N' && key[1] == '\0') return static_cast<float>(N);
        return 0.0f;
    }

protected:
    float apply() SENSOR_OVERRIDE {
        float sorted[N];
        this->getSortedCopy(sorted);

        uint8_t mid = N / 2;
        if (N % 2 == 1) {
            return sorted[mid];                      // 奇数窗口: 中间值
        } else {
            return (sorted[mid - 1] + sorted[mid]) / 2.0f;  // 偶数窗口: 两中间值平均
        }
    }
};

#endif // SENSOR_FEATURE_FILTER_MEDIAN
