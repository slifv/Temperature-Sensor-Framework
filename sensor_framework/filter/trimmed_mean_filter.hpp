/**
 * @file trimmed_mean_filter.hpp
 * @brief 截尾均值滤波 (Trimmed Mean / Olympic Average)
 * @details 取窗口内M个值，排序后去掉最小N_Trim个和最大N_Trim个，
 *          剩余(M-2N_Trim)个值取算术平均。
 *          比中值滤波保留更多分布信息，比均值滤波更抗干扰。
 *          参考: 需求评审报告 第6.1.8节
 */

#pragma once

#include "../feature_config.hpp"
#include "sliding_window.hpp"

#if SENSOR_FEATURE_FILTER_TRIMMED_MEAN && SENSOR_FEATURE_FILTER_SLIDING_WINDOW

#include <cstring>

/**
 * @class TrimmedMeanFilter
 * @brief 截尾均值滤波器
 * @tparam M      窗口大小
 * @tparam N_Trim 每侧截尾数量（总共去掉 2*N_Trim 个值）
 * @note 约束: M > 2 * N_Trim，N_Trim >= 1
 */
template <uint8_t M, uint8_t N_Trim = 1>
class TrimmedMeanFilter : public SlidingWindowFilter<M> {
    static_assert(M > 2 * N_Trim, "TrimmedMeanFilter: M must be > 2 * N_Trim");
    static_assert(N_Trim >= 1, "TrimmedMeanFilter: N_Trim must be >= 1");

public:
    TrimmedMeanFilter()
        : runtimeTrim_(N_Trim)
    {}

    const char* getName() const SENSOR_OVERRIDE { return "TrimmedMean"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::TRIMMED_MEAN; }

    bool setParameter(const char* key, float value) SENSOR_OVERRIDE {
        if (key && key[0] == 'm' && key[1] == '\0') {
            uint8_t newTrim = static_cast<uint8_t>(value);
            if (newTrim >= 1 && M > 2 * newTrim) {
                runtimeTrim_ = newTrim;
                return true;
            }
        }
        return false;
    }

    float getParameter(const char* key) const SENSOR_OVERRIDE {
        if (!key) return 0.0f;
        if (key[0] == 'M' && key[1] == '\0') return static_cast<float>(M);
        if (key[0] == 'm' && key[1] == '\0') return static_cast<float>(runtimeTrim_);
        if (key[0] == 'e' && key[1] == '\0') {
            return static_cast<float>(M - 2 * runtimeTrim_);
        }
        return 0.0f;
    }

    /// 获取有效样本数（去掉截尾后的保留数量）
    uint8_t getEffectiveCount() const {
        return M - 2 * runtimeTrim_;
    }

protected:
    float apply() SENSOR_OVERRIDE {
        float sorted[M];
        this->getSortedCopy(sorted);

        uint8_t trim = runtimeTrim_;
        float sum = 0.0f;
        uint8_t count = M - 2 * trim;

        for (uint8_t i = trim; i < M - trim; ++i) {
            sum += sorted[i];
        }

        return sum / static_cast<float>(count);
    }

private:
    uint8_t runtimeTrim_;  ///< 运行时可调整的截尾数（不超过编译期约束）
};

// ─── 便捷别名 ──────────────────────────────────────────────

/// 奥林匹克平均：去掉1个最高+1个最低（M选M-2）— 体育评分标准
template <uint8_t M = 7>
using OlympicFilter = TrimmedMeanFilter<M, 1>;

/// 重度截尾均值：去掉2个最高+2个最低 — 更激进去噪
template <uint8_t M = 7>
using HeavyTrimmedFilter = TrimmedMeanFilter<M, 2>;

#endif // SENSOR_FEATURE_FILTER_TRIMMED_MEAN
