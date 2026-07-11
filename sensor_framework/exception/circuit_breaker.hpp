/**
 * @file circuit_breaker.hpp
 * @brief 熔断保护器 — 连续异常超过阈值时停止采样
 * @details 三态状态机: CLOSED(正常) → OPEN(熔断) → HALF_OPEN(探测恢复)
 *          防止系统资源被持续异常的传感器耗尽。
 *          参考: 需求评审报告 第16.5节
 */

#pragma once

#include "../feature_config.hpp"

#if SENSOR_FEATURE_CIRCUIT_BREAKER

#include <cstdint>

/**
 * @class CircuitBreaker
 * @brief 熔断保护器（三态状态机）
 */
class CircuitBreaker {
public:
    /// 熔断器状态
    enum State : uint8_t {
        CLOSED      = 0,    ///< 正常: 允许所有请求
        OPEN        = 1,    ///< 熔断: 拒绝所有请求
        HALF_OPEN   = 2     ///< 半开: 允许探测请求（测试是否恢复）
    };

    /**
     * @param failureThreshold 连续失败阈值（默认5次后熔断）
     * @param cooldownMs      冷却时间（默认30s后尝试恢复）
     */
    CircuitBreaker(uint32_t failureThreshold = 5, uint32_t cooldownMs = 30000)
        : state_(CLOSED)
        , failureThreshold_(failureThreshold)
        , cooldownMs_(cooldownMs)
        , consecutiveFailures_(0)
        , openedAt_(0)
        , totalSuccesses_(0)
        , totalFailures_(0)
        , tickProvider_(NULL)
    {}

    /// 记录一次操作成功
    void recordSuccess() {
        consecutiveFailures_ = 0;
        ++totalSuccesses_;

        if (state_ == HALF_OPEN) {
            state_ = CLOSED;  // 探测成功 → 恢复正常
        }
    }

    /// 记录一次操作失败
    void recordFailure() {
        ++consecutiveFailures_;
        ++totalFailures_;

        if (state_ == HALF_OPEN) {
            // 探测失败 → 重新熔断
            state_ = OPEN;
            openedAt_ = getCurrentTick();
        } else if (consecutiveFailures_ >= failureThreshold_) {
            // 连续失败超阈值 → 熔断
            state_ = OPEN;
            openedAt_ = getCurrentTick();
        }
    }

    /// 是否允许执行请求
    bool allowRequest() {
        if (state_ == CLOSED) return true;

        if (state_ == OPEN) {
            uint32_t now = getCurrentTick();
            if (now - openedAt_ > cooldownMs_) {
                state_ = HALF_OPEN;   // 冷却结束 → 进入探测状态
                return true;           // 允许一次探测
            }
            return false;  // 仍在冷却中
        }

        // HALF_OPEN: 允许探测请求
        return true;
    }

    /// 获取当前状态
    State getState() const { return state_; }

    /// 获取连续失败次数
    uint32_t getConsecutiveFailures() const { return consecutiveFailures_; }

    /// 获取总成功/失败次数
    uint32_t getTotalSuccesses() const { return totalSuccesses_; }
    uint32_t getTotalFailures() const { return totalFailures_; }

    /// 获取状态名称
    const char* getStateName() const {
        switch (state_) {
            case CLOSED:    return "CLOSED";
            case OPEN:      return "OPEN";
            case HALF_OPEN: return "HALF_OPEN";
            default:        return "UNKNOWN";
        }
    }

    /// 配置更新
    void setThreshold(uint32_t threshold) { failureThreshold_ = threshold; }
    void setCooldown(uint32_t ms) { cooldownMs_ = ms; }

    /// 手动复位
    void reset() {
        state_ = CLOSED;
        consecutiveFailures_ = 0;
    }

    /**
     * @brief 设置时间获取函数（供不同平台注入）
     * @param tickFunc 返回当前毫秒滴答的函数指针
     */
    void setTickProvider(uint32_t (*tickFunc)()) {
        tickProvider_ = tickFunc;
    }

private:
    uint32_t getCurrentTick() const {
        if (tickProvider_) {
            return tickProvider_();
        }
        return 0;  // 默认（需平台注入）
    }

    State       state_;
    uint32_t    failureThreshold_;
    uint32_t    cooldownMs_;
    uint32_t    consecutiveFailures_;
    uint32_t    openedAt_;
    uint32_t    totalSuccesses_;
    uint32_t    totalFailures_;
    uint32_t    (*tickProvider_)();  ///< 可注入的时间获取函数
};

#endif // SENSOR_FEATURE_CIRCUIT_BREAKER
