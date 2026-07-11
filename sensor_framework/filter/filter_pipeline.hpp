/**
 * @file filter_pipeline.hpp
 * @brief 滤波器链 (Pipeline) — 多级滤波器串联执行
 * @details 前一级输出为后一级输入。运行时可通过配置动态增删滤波器。
 *          采用装饰器模式，Pipeline本身也是IFilter。
 *          参考: 需求评审报告 第6.2节
 */

#pragma once

#include "../feature_config.hpp"
#include "../utils/static_vector.hpp"
#include "filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_PIPELINE

#include <cstdint>

/**
 * @class FilterPipeline
 * @brief 滤波器链 — 级联多个滤波器
 * @tparam MaxFilters 最大滤波器数量（受平台约束）
 */
template <uint8_t MaxFilters = SENSOR_CONSTRAINT_MAX_FILTERS>
class FilterPipeline : public IFilter {
public:
    FilterPipeline() {}

    /**
     * @brief 添加滤波器到链尾
     * @param filter 滤波器指针（Pipeline接管所有权）
     * @return 添加成功返回 true（链满返回 false）
     */
    bool addFilter(IFilter* filter) {
        if (!filter) return false;
        return filters_.push_back(filter);
    }

    /**
     * @brief 按名称移除滤波器
     * @param name 滤波器名称
     * @return 找到并移除返回 true
     */
    bool removeFilter(const char* name) {
        for (uint8_t i = 0; i < filters_.size(); ++i) {
            const char* filterName = filters_[i]->getName();
            if (filterName) {
                // 简单字符串比较
                const char* a = name;
                const char* b = filterName;
                while (*a && *b && (*a == *b)) { ++a; ++b; }
                if (*a == '\0' && *b == '\0') {
                    delete filters_[i];
                    return filters_.remove(i);
                }
            }
        }
        return false;
    }

    /**
     * @brief 按索引移除滤波器
     */
    bool removeAt(uint8_t index) {
        if (index < filters_.size()) {
            delete filters_[index];
            return filters_.remove(index);
        }
        return false;
    }

    /// 清空所有滤波器
    void clear() {
        for (uint8_t i = 0; i < filters_.size(); ++i) {
            delete filters_[i];
        }
        filters_.clear();
    }

    /**
     * @brief 级联处理: 数据依次经过所有滤波器
     */
    float update(float input) SENSOR_OVERRIDE {
        float value = input;
        for (uint8_t i = 0; i < filters_.size(); ++i) {
            value = filters_[i]->update(value);
            // 有效性检查
            if (!isFinite(value)) {
                return input;  // 滤波异常 → 退回原始值
            }
        }
        lastOutput_ = value;
        return value;
    }

    void reset() SENSOR_OVERRIDE {
        for (uint8_t i = 0; i < filters_.size(); ++i) {
            filters_[i]->reset();
        }
        lastOutput_ = 0.0f;
    }

    const char* getName() const SENSOR_OVERRIDE { return "Pipeline"; }

    FilterType getType() const SENSOR_OVERRIDE { return FilterType::NONE; }

    /// 获取滤波器数量
    uint8_t getFilterCount() const { return filters_.size(); }

    /// 按索引访问滤波器
    IFilter* getFilter(uint8_t index) const {
        if (index < filters_.size()) {
            return filters_[index];
        }
        return NULL;
    }

    /// 启用/禁用单个滤波器（通过名称）
    bool setFilterEnabled(const char* name, bool enabled) {
        for (uint8_t i = 0; i < filters_.size(); ++i) {
            const char* filterName = filters_[i]->getName();
            // 简化比较
            const char* a = name;
            const char* b = filterName;
            while (*a && *b && (*a == *b)) { ++a; ++b; }
            if (*a == '\0' && *b == '\0') {
                enabled_[i] = enabled;
                return true;
            }
        }
        return false;
    }

    ~FilterPipeline() {
        clear();
    }

private:
    StaticVector<IFilter*, MaxFilters>  filters_;
    bool enabled_[MaxFilters];  ///< 逐滤波器启用标志（预留，C++11无内联初始化）
    uint8_t _padding[1];        // alignment padding
};

#endif // SENSOR_FEATURE_FILTER_PIPELINE
