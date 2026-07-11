/**
 * @file exception_detector.hpp
 * @brief 异常检测器 — 通信异常/数据异常/滤波异常三类检测器
 * @details 实现 IExceptionDetector 接口的三类检测器。
 *          检测结果以 ExceptionEvent 结构输出。
 *          参考: 需求评审报告 第16.4节
 */

#pragma once

#include "exception_types.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EXCEPTION

#include <cmath>
#include <cstring>

// ============================================================
//  异常检测器接口
// ============================================================

/**
 * @class IExceptionDetector
 * @brief 异常检测器抽象接口
 */
class IExceptionDetector {
public:
    virtual ~IExceptionDetector() {}

    /**
     * @brief 检测传感器数据和状态中是否存在异常
     * @param data   传感器数据
     * @param status 传感器当前状态
     * @param event  [输出] 异常事件详情（仅在检测到异常时填充）
     * @return 检测到异常返回 true
     */
    virtual bool detect(const SensorData& data,
                        const SensorStatus& status,
                        ExceptionEvent& event) = 0;

    /// 重置检测器状态
    virtual void reset() = 0;

    /// 获取检测器名称
    virtual const char* getName() const = 0;
};

// ============================================================
//  通信异常检测器
// ============================================================

#if SENSOR_FEATURE_EXCEPTION_COMM_DETECT

class CommExceptionDetector : public IExceptionDetector {
public:
    CommExceptionDetector(uint32_t maxRetries = 3, uint32_t retryIntervalMs = 100)
        : maxRetries_(maxRetries)
        , retryIntervalMs_(retryIntervalMs)
        , consecutiveErrors_(0)
    {}

    bool detect(const SensorData& data, const SensorStatus& status,
                ExceptionEvent& event) SENSOR_OVERRIDE
    {
        // 检查通信失败标志
        if (status == SensorStatus::COMM_ERROR || status == SensorStatus::OFFLINE) {
            ++consecutiveErrors_;

            event.sensorId      = data.sensorId;
            event.timestampMs   = data.timestampMs;
            event.category      = ExceptionCategory::COMMUNICATION;
            event.severity      = (consecutiveErrors_ >= maxRetries_)
                                    ? ExceptionSeverity::ERROR
                                    : ExceptionSeverity::WARNING;
            event.errorCode     = ERR_I2C_NACK;  // 可扩展为具体错误码
            event.consecutiveCount = consecutiveErrors_;
            event.contextValue  = 0.0f;
            event.recoveryAction = RecoveryAction::RETRY;
            return true;
        }

        // 正常通信 → 重置计数
        consecutiveErrors_ = 0;
        return false;
    }

    void reset() SENSOR_OVERRIDE { consecutiveErrors_ = 0; }
    const char* getName() const SENSOR_OVERRIDE { return "CommDetector"; }

    uint32_t getConsecutiveErrors() const { return consecutiveErrors_; }

private:
    uint32_t maxRetries_;
    uint32_t retryIntervalMs_;
    uint32_t consecutiveErrors_;
};

#endif // SENSOR_FEATURE_EXCEPTION_COMM_DETECT

// ============================================================
//  数据异常检测器
// ============================================================

#if SENSOR_FEATURE_EXCEPTION_DATA_DETECT

class DataExceptionDetector : public IExceptionDetector {
public:
    DataExceptionDetector(float minValid = -40.0f, float maxValid = 125.0f,
                          float maxChangeRate = 30.0f,
                          uint32_t frozenThresholdMs = 10000)
        : minValid_(minValid), maxValid_(maxValid)
        , maxChangeRate_(maxChangeRate)
        , frozenThresholdMs_(frozenThresholdMs)
        , lastValue_(SENSOR_INVALID_FLOAT)
        , lastChangeTime_(0)
    {}

    bool detect(const SensorData& data, const SensorStatus& status,
                ExceptionEvent& event) SENSOR_OVERRIDE
    {
        (void)status;

        // 1. 超出量程检测
        if (data.value < minValid_ || data.value > maxValid_) {
            fillEvent(event, data, ExceptionSeverity::ERROR,
                      ERR_OUT_OF_RANGE, RecoveryAction::USE_LAST_VALID);
            return true;
        }

        // 2. 数据冻结检测（长时间不变）
        if (lastValue_ != SENSOR_INVALID_FLOAT &&
            data.value == lastValue_)
        {
            if (data.timestampMs - lastChangeTime_ > frozenThresholdMs_) {
                fillEvent(event, data, ExceptionSeverity::ERROR,
                          ERR_DATA_FROZEN, RecoveryAction::RESET_SENSOR);
                return true;
            }
        } else {
            lastChangeTime_ = data.timestampMs;
        }

        // 3. 突变检测（相邻采样差值过大）
        if (lastValue_ != SENSOR_INVALID_FLOAT) {
            float delta = (data.value > lastValue_)
                        ? (data.value - lastValue_)
                        : (lastValue_ - data.value);
            if (delta > maxChangeRate_) {
                fillEvent(event, data, ExceptionSeverity::WARNING,
                          ERR_SPIKE_DETECTED, RecoveryAction::NONE);
                event.contextValue = delta;
                lastValue_ = data.value;
                return true;
            }
        }

        lastValue_ = data.value;
        return false;
    }

    void reset() SENSOR_OVERRIDE {
        lastValue_ = SENSOR_INVALID_FLOAT;
        lastChangeTime_ = 0;
    }

    const char* getName() const SENSOR_OVERRIDE { return "DataDetector"; }

    bool setParameter(const char* key, float value) {
        if (!key) return false;
        if (std::strcmp(key, "min") == 0) { minValid_ = value; return true; }
        if (std::strcmp(key, "max") == 0) { maxValid_ = value; return true; }
        if (std::strcmp(key, "rate") == 0) { maxChangeRate_ = value; return true; }
        return false;
    }

private:
    void fillEvent(ExceptionEvent& event, const SensorData& data,
                   ExceptionSeverity sev, ErrorCode code,
                   RecoveryAction action)
    {
        event.sensorId       = data.sensorId;
        event.timestampMs    = data.timestampMs;
        event.category       = ExceptionCategory::DATA_VALIDITY;
        event.severity       = sev;
        event.errorCode      = static_cast<uint16_t>(code);
        event.contextValue   = data.value;
        event.recoveryAction = action;
    }

    float    minValid_;
    float    maxValid_;
    float    maxChangeRate_;
    uint32_t frozenThresholdMs_;
    float    lastValue_;
    uint32_t lastChangeTime_;
};

#endif // SENSOR_FEATURE_EXCEPTION_DATA_DETECT

// ============================================================
//  滤波异常检测器
// ============================================================

#if SENSOR_FEATURE_EXCEPTION_FILTER_DETECT

class FilterExceptionDetector : public IExceptionDetector {
public:
    FilterExceptionDetector()
        : lastDeviation_(0.0f), divergenceCount_(0)
    {}

    bool detect(const SensorData& data, const SensorStatus& status,
                ExceptionEvent& event) SENSOR_OVERRIDE
    {
        (void)status;

        float filtered = data.value;
        float raw = data.rawValue;

        // 1. NaN/Inf 检测
        if (filtered != filtered || filtered * 0.0f != 0.0f) {
            event.sensorId       = data.sensorId;
            event.timestampMs    = data.timestampMs;
            event.category       = ExceptionCategory::FILTER_ANOMALY;
            event.severity       = ExceptionSeverity::ERROR;
            event.errorCode      = (filtered != filtered)
                                    ? ERR_FILTER_NAN : ERR_FILTER_INF;
            event.contextValue   = filtered;
            event.recoveryAction = RecoveryAction::USE_LAST_VALID;
            return true;
        }

        // 2. 滤波发散检测: 滤波后偏差持续增大
        float deviation = (filtered > raw) ? (filtered - raw) : (raw - filtered);
        if (deviation > lastDeviation_ * 3.0f) {
            ++divergenceCount_;
        } else {
            divergenceCount_ = 0;
        }
        lastDeviation_ = deviation;

        if (divergenceCount_ >= DIVERGENCE_THRESHOLD) {
            event.sensorId       = data.sensorId;
            event.timestampMs    = data.timestampMs;
            event.category       = ExceptionCategory::FILTER_ANOMALY;
            event.severity       = ExceptionSeverity::ERROR;
            event.errorCode      = ERR_FILTER_DIVERGENCE;
            event.contextValue   = deviation;
            event.consecutiveCount = divergenceCount_;
            event.recoveryAction = RecoveryAction::RESET_SENSOR;
            return true;
        }

        return false;
    }

    void reset() SENSOR_OVERRIDE {
        lastDeviation_ = 0.0f;
        divergenceCount_ = 0;
    }

    const char* getName() const SENSOR_OVERRIDE { return "FilterDetector"; }

private:
    static const uint32_t DIVERGENCE_THRESHOLD = 5;
    float    lastDeviation_;
    uint32_t divergenceCount_;
};

#endif // SENSOR_FEATURE_EXCEPTION_FILTER_DETECT

#endif // SENSOR_FEATURE_EXCEPTION
