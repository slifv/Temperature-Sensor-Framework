/**
 * @file weighted_mean_filter.hpp
 * @brief 加权移动平均 — 近期数据权重更大，兼顾平滑性与响应速度
 * @details y[n] = Σ(w[i]·x[n-i]) / Σw[i]
 *          权重数组编译期确定，可按线性/指数/自定义分布。
 *          参考: 需求评审报告 第6.1.5节
 */

#pragma once

#include "../feature_config.hpp"
#include "sliding_window.hpp"

#if SENSOR_FEATURE_FILTER_WEIGHTED_MEAN && SENSOR_FEATURE_FILTER_SLIDING_WINDOW

/**
 * @class WeightedMeanFilter
 * @brief 加权移动平均滤波器
 * @tparam N  窗口大小
 * @tparam W0..W[N-1] 权重数组（模板参数，编译期确定，越靠前越旧）
 * @note 默认权重为线性递减: 最新值权重=N, 最旧值权重=1
 */
template <uint8_t N, uint8_t... Weights>
class WeightedMeanFilter : public SlidingWindowFilter<N> {
public:
    const char* getName() const SENSOR_OVERRIDE { return "WeightedMean"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::WEIGHTED_MEAN; }

protected:
    float apply() SENSOR_OVERRIDE {
        const float* window = this->getWindow();
        const uint8_t w[N] = { Weights... };

        float sum = 0.0f;
        float weightSum = 0.0f;

        for (uint8_t i = 0; i < N; ++i) {
            uint8_t idx = (this->writeIdx_ - N + i) % N;
            sum += window[i] * static_cast<float>(w[i]);
            weightSum += static_cast<float>(w[i]);
        }

        return (weightSum > 0.0f) ? (sum / weightSum) : 0.0f;
    }
};

// ─── 默认线性权重特化 ────────────────────────────────────

/**
 * @brief 线性加权移动平均（默认权重: 最新=N, ..., 最旧=1）
 */
template <uint8_t N>
class LinearWeightedFilter : public SlidingWindowFilter<N> {
public:
    const char* getName() const SENSOR_OVERRIDE { return "LinearWeighted"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::WEIGHTED_MEAN; }

protected:
    float apply() SENSOR_OVERRIDE {
        const float* window = this->getWindow();
        float sum = 0.0f;
        float weightSum = 0.0f;

        // 线性权重: 窗口中最旧=1, ..., 最新=N
        for (uint8_t i = 0; i < N; ++i) {
            float w = static_cast<float>(i + 1);
            sum += window[i] * w;
            weightSum += w;
        }

        return sum / weightSum;
    }
};

#endif // SENSOR_FEATURE_FILTER_WEIGHTED_MEAN
