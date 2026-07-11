/**
 * @file filter_interface.hpp
 * @brief 滤波器统一抽象接口 + 滤波器类型枚举
 * @details 所有滤波器实现 IFilter 接口。通过 FilterType 枚举支持
 *          运行时配置滤波器类型选择。
 *          参考: 需求评审报告 第9.2节
 */

#pragma once

#include "../feature_config.hpp"

// ============================================================
//  滤波器类型枚举
// ============================================================

/// 滤波器类型标识（用于运行时配置、序列化、日志输出）
enum class FilterType : uint8_t {
    NONE            = 0,
    MEAN            = 1,    ///< 滑动均值滤波
    MEDIAN          = 2,    ///< 中值滤波
    LOW_PASS        = 3,    ///< 一阶低通滤波 (IIR)
    KALMAN          = 4,    ///< 一维卡尔曼滤波
    WEIGHTED_MEAN   = 5,    ///< 加权移动平均
    FIR             = 6,    ///< FIR有限脉冲响应低通
    TRIMMED_MEAN    = 7,    ///< 截尾均值滤波 (Olympic Average)

    // 自定义扩展ID起始值
    CUSTOM_BASE     = 100
};

// ============================================================
//  滤波器统一接口
// ============================================================

/**
 * @class IFilter
 * @brief 滤波器统一抽象接口
 * @details 所有滤波器必须实现此接口。采用策略模式，
 *          不同滤波算法可互相替换。
 */
class IFilter {
public:
    virtual ~IFilter() {}

    /**
     * @brief 输入一个数据点，返回滤波结果
     * @param input 原始输入值
     * @return 滤波后的输出值
     */
    virtual float update(float input) = 0;

    /**
     * @brief 重置滤波器状态
     * @details 清空内部缓冲区/历史数据，恢复到初始状态
     */
    virtual void reset() = 0;

    /**
     * @brief 获取滤波器名称（用于调试/日志）
     * @return C风格字符串
     */
    virtual const char* getName() const = 0;

    /**
     * @brief 获取滤波器类型枚举
     */
    virtual FilterType getType() const = 0;

    /**
     * @brief 设置滤波器参数（键值对方式）
     * @param key   参数名
     * @param value 参数值
     * @return 成功返回 true（不支持的参数返回 false）
     */
    virtual bool setParameter(const char* key, float value) {
        (void)key;
        (void)value;
        return false;
    }

    /**
     * @brief 获取滤波器参数
     * @param key 参数名
     * @return 参数值，不支持的参数返回 0.0f
     */
    virtual float getParameter(const char* key) const {
        (void)key;
        return 0.0f;
    }

    /**
     * @brief 获取最后输出值（不产生新输入）
     */
    virtual float getLastOutput() const { return lastOutput_; }

protected:
    float lastOutput_ = 0.0f;

    /// 检测浮点值是否有效（非NaN，非Inf）
    static bool isFinite(float value) {
        // C++11兼容方式检测 NaN/Inf（避免 std::isnan 可能存在的方法重载问题）
        volatile float v = value;
        return (v == v) && (v * 0.0f == 0.0f);
    }

    /// 限幅保护
    static float clamp(float value, float minVal, float maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }
};

// ============================================================
//  哨兵值常量
// ============================================================

/// 无效浮点值（作为哨兵标记）
#define SENSOR_INVALID_FLOAT (-3.4028235e+38f)
