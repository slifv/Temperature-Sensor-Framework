/**
 * @file sliding_window.hpp
 * @brief 通用滑动窗口滤波框架基类
 * @details 将"滑动窗口数据管理"与"窗口算子逻辑"分离。
 *          维护固定大小的环形缓冲区，提供纯虚方法 apply()
 *          供子类实现具体窗口算子（均值/中值/截尾/自定义）。
 *          参考: 需求评审报告 第6.1.7节
 */

#pragma once

#include "../feature_config.hpp"
#include "../compiler_features.hpp"
#include "filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_SLIDING_WINDOW

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * @class SlidingWindowFilter
 * @brief 滑动窗口滤波基类 — 管理环形缓冲区，算子由子类实现
 * @tparam N 窗口大小（编译期常量，推荐 ≤ SENSOR_CONSTRAINT_MAX_WINDOW）
 */
template <uint8_t N>
class SlidingWindowFilter : public IFilter {
    static_assert(N >= 3, "SlidingWindowFilter: window size must be >= 3");
    static_assert(N <= SENSOR_CONSTRAINT_MAX_WINDOW,
                  "Window size exceeds platform limit SENSOR_CONSTRAINT_MAX_WINDOW");

public:
    SlidingWindowFilter() { reset(); }

    /**
     * @brief 插入数据并返回窗口算子结果
     * @param input 新采样数据
     * @return 窗口未填满时透传原始值，填满后返回 apply() 结果
     */
    float update(float input) SENSOR_OVERRIDE final {
        buffer_[writeIdx_ % N] = input;
        ++writeIdx_;

        if (writeIdx_ < N) {
            lastOutput_ = input;
            return input;  // 窗口未填满时透传
        }

        lastOutput_ = apply();
        return lastOutput_;
    }

    /// 重置缓冲区
    void reset() SENSOR_OVERRIDE {
        writeIdx_ = 0;
        // 用零填充缓冲区
        for (uint8_t i = 0; i < N; ++i) {
            buffer_[i] = 0.0f;
        }
        lastOutput_ = 0.0f;
    }

    // ─── 查询接口 ───────────────────────────────────────

    /// 获取窗口原始数据指针（只读）
    const float* getWindow() const { return buffer_; }

    /// 获取窗口大小
    uint8_t getWindowSize() const { return N; }

    /// 窗口是否已填满（就绪状态）
    bool isReady() const { return writeIdx_ >= N; }

    /// 当前缓冲区中已填入的数据点数量
    uint8_t getCount() const {
        return (writeIdx_ < N) ? static_cast<uint8_t>(writeIdx_) : N;
    }

protected:
    /**
     * @brief 纯虚方法：子类实现具体的窗口算子
     * @details 当窗口填满后每次 update() 都会调用此方法
     * @return 窗口算子的计算结果
     */
    virtual float apply() = 0;

    /**
     * @brief 获取排序后的窗口数据副本
     * @details 供需要排序的算子（中值、截尾均值）使用。
     *          原窗口数据不被修改。
     * @param out 输出缓冲区（需预分配 N 个 float）
     */
    void getSortedCopy(float* out) const {
        std::memcpy(out, buffer_, N * sizeof(float));
        // 使用插入排序（小窗口N≤32时比快排更快，且无递归栈开销）
        for (uint8_t i = 1; i < N; ++i) {
            float key = out[i];
            int16_t j = static_cast<int16_t>(i) - 1;
            while (j >= 0 && out[j] > key) {
                out[j + 1] = out[j];
                --j;
            }
            out[j + 1] = key;
        }
    }

    /**
     * @brief 获取窗口数据可写副本（供高级算子修改）
     */
    void getCopy(float* out) const {
        std::memcpy(out, buffer_, N * sizeof(float));
    }

    float   buffer_[N];
    uint8_t writeIdx_;
};

#endif // SENSOR_FEATURE_FILTER_SLIDING_WINDOW
