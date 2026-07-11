/**
 * @file exception_handler.hpp
 * @brief 异常处理策略 — 根据异常级别映射恢复动作
 * @details 根据异常严重级别(L0-L3)返回对应的恢复动作:
 *          L0→NONE, L1→NONE+记录, L2→降级/重试, L3→停止/告警
 *          参考: 需求评审报告 第16.2-16.3节
 */

#pragma once

#include "exception_types.hpp"

#if SENSOR_FEATURE_EXCEPTION

/**
 * @struct RecoveryResult
 * @brief 恢复处理结果
 */
struct RecoveryResult {
    RecoveryAction  action;             ///< 推荐执行的动作
    bool            success;            ///< 恢复是否成功
    float           fallbackValue;      ///< 降级时使用的回退值

    RecoveryResult()
        : action(RecoveryAction::NONE)
        , success(false)
        , fallbackValue(0.0f)
    {}
};

/**
 * @class ExceptionHandler
 * @brief 默认异常处理器 — 基于严重级别的策略映射
 */
class ExceptionHandler {
public:
    ExceptionHandler(float defaultFallback = 25.0f)
        : defaultFallback_(defaultFallback)
        , lastValidValue_(defaultFallback)
    {}

    /**
     * @brief 根据异常事件决定恢复动作
     */
    RecoveryResult handle(const ExceptionEvent& event) {
        RecoveryResult result;

        switch (event.severity) {
        case ExceptionSeverity::INFO:
            result.action  = RecoveryAction::NONE;
            result.success = true;
            break;

        case ExceptionSeverity::WARNING:
            result.action  = RecoveryAction::NONE;  // 警告不阻断
            result.success = true;
            break;

        case ExceptionSeverity::ERROR:
            // 错误级别：使用降级策略
            if (event.category == ExceptionCategory::COMMUNICATION) {
                result.action = RecoveryAction::RETRY;
            } else {
                result.action = RecoveryAction::USE_LAST_VALID;
                result.fallbackValue = lastValidValue_;
            }
            result.success = false;
            break;

        case ExceptionSeverity::FATAL:
            // 致命级别：停止采样 + 系统告警
            result.action  = RecoveryAction::STOP_SAMPLING;
            result.fallbackValue = defaultFallback_;
            result.success = false;
            break;
        }

        return result;
    }

    /// 更新最近一次有效值（正常数据通过时调用）
    void updateValidValue(float value) {
        lastValidValue_ = value;
    }

    /// 设置默认回退值
    void setDefaultFallback(float value) { defaultFallback_ = value; }

    /// 获取最近有效值
    float getLastValidValue() const { return lastValidValue_; }

private:
    float defaultFallback_;
    float lastValidValue_;
};

#endif // SENSOR_FEATURE_EXCEPTION
