/**
 * @file fixed_mean_filter.hpp
 * @brief 定点数均值滤波器 — 无浮点运算的滑动均值滤波
 * @details 输入/输出接口保持float（与传感器接口一致），
 *          内部全部使用Q15.16定点数进行计算。
 *          适用于8位/16位无FPU的MCU平台。
 *          参考: 需求评审报告 第18.3节
 */

#pragma once

#include "../feature_config.hpp"
#include "q_math.hpp"

#if SENSOR_FEATURE_FILTER_MEAN && SENSOR_FEATURE_FIXED_POINT_MATH

#include <cstdint>

/**
 * @class FixedMeanFilter
 * @brief 定点数滑动均值滤波器
 * @tparam N 窗口大小（8位平台建议 ≤4，16位平台建议 ≤8）
 */
template <uint8_t N>
class FixedMeanFilter {
    static_assert(N >= 2, "Window size must be >= 2");
    static_assert(N <= SENSOR_CONSTRAINT_MAX_WINDOW,
                  "Window size exceeds platform limit");

public:
    FixedMeanFilter() : writeIdx_(0) { reset(); }

    /**
     * @brief 输入浮点值，内部转定点计算后返回浮点结果
     * @param input 采样值（float，与传感器接口一致）
     * @return 滤波后的值（float）
     */
    float update(float input) {
        q15_16_t fixedInput = qmath::fromFloat(input);

        // 写入环形缓冲区
        buffer_[writeIdx_ % N] = fixedInput;
        ++writeIdx_;

        if (writeIdx_ < N) {
            return input;  // 窗口未满，透传
        }

        // 定点数均值计算（纯整数运算）
        q15_16_t avg = qmath::mean(buffer_, N);
        return qmath::toFloat(avg);
    }

    /// 重置
    void reset() {
        writeIdx_ = 0;
        for (uint8_t i = 0; i < N; ++i) {
            buffer_[i] = 0;
        }
    }

    /// 是否已就绪（窗口填满）
    bool isReady() const { return writeIdx_ >= N; }

    /// 当前数据点数量
    uint8_t getCount() const {
        return (writeIdx_ < N) ? writeIdx_ : N;
    }

    /// 获取窗口大小
    uint8_t getWindowSize() const { return N; }

    /// 获取原始定点数缓冲区（供调试/测试）
    const q15_16_t* getRawBuffer() const { return buffer_; }

private:
    q15_16_t buffer_[N];    ///< 定点数环形缓冲区
    uint8_t  writeIdx_;     ///< 写入索引（uint8_t节省1字节 vs size_t）
};

// ─── 便捷别名 ──────────────────────────────────────────────

/// 4点定点均值滤波（8位MCU推荐）
typedef FixedMeanFilter<4> FixedMeanFilter4;

/// 8点定点均值滤波（16位MCU推荐）
typedef FixedMeanFilter<8> FixedMeanFilter8;

#endif // SENSOR_FEATURE_FILTER_MEAN && SENSOR_FEATURE_FIXED_POINT_MATH
