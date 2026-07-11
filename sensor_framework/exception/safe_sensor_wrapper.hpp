/**
 * @file safe_sensor_wrapper.hpp
 * @brief 安全传感器包装器 — 集成异常检测+熔断+降级的采样入口
 * @details 装饰器模式：包装原始传感器，在采样前后自动执行
 *          异常检测、熔断检查、恢复策略、异常日志记录。
 *          是应用层使用异常监控能力的推荐入口。
 *          参考: 需求评审报告 第16.7节
 */

#pragma once

#include "../feature_config.hpp"
#include "exception_types.hpp"
#include "exception_detector.hpp"
#include "exception_handler.hpp"
#include "circuit_breaker.hpp"
#include "exception_logger.hpp"
#include "../core/sensor_base.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EXCEPTION

#include <memory>

/**
 * @class SafeSensorWrapper
 * @brief 安全传感器包装器 — 带异常保护的传感器数据读取
 * @details 每次 safeRead() 执行:
 *          1. 熔断检查 → 拒绝则返回缓存值
 *          2. 读取原始数据
 *          3. 逐检测器扫描异常
 *          4. 异常时: 记录→熔断→执行恢复策略
 *          5. 正常时: 记录成功→更新缓存→返回
 */
template <uint8_t MaxDetectors = 4>
class SafeSensorWrapper {
public:
    SafeSensorWrapper()
        : sensor_(NULL), handler_(NULL), initialized_(false)
        , lastValidValue_(0.0f)
    {}

    /**
     * @brief 绑定传感器
     */
    void bindSensor(SensorBase<SensorData>* sensor) {
        sensor_ = sensor;
        if (handler_) {
            handler_->reset();
        }
    }

    /**
     * @brief 添加异常检测器
     */
    bool addDetector(IExceptionDetector* detector) {
        if (detectorCount_ < MaxDetectors) {
            detectors_[detectorCount_++] = detector;
            return true;
        }
        return false;
    }

    /**
     * @brief 设置异常处理器
     */
    void setHandler(ExceptionHandler* handler) {
        handler_ = handler;
    }

    /**
     * @brief 安全读取传感器数据
     * @details 带熔断保护、异常检测、自动降级的采样方法
     * @return 传感器数据（异常时可能为缓存值/默认值）
     */
    SensorData safeRead() {
        SensorData result;

        if (!sensor_) {
            return result;
        }

        // 1. 熔断检查
        IF_CIRCUIT_BREAKER_ENABLED(
            if (!breaker_.allowRequest()) {
                result.value = lastValidValue_;
                result.status = SensorStatus::DEGRADED;
                result.sensorId = sensor_->getSensorId();
                return result;
            }
        )

        // 2. 读取原始数据
        result = sensor_->readFiltered();
        SensorStatus status = sensor_->getStatus();

        // 3. 逐检测器扫描异常
        ExceptionEvent event;
        bool hasException = false;

        for (uint8_t i = 0; i < detectorCount_; ++i) {
            if (detectors_[i]->detect(result, status, event)) {
                hasException = true;

                // 记录失败到熔断器
                IF_CIRCUIT_BREAKER_ENABLED(
                    breaker_.recordFailure();
                )

                // 记录日志
                IF_EXCEPTION_LOGGER_ENABLED(
                    logger_.log(event);
                )

                // 执行恢复策略
                if (handler_) {
                    RecoveryResult recovery = handler_->handle(event);
                    result = executeRecovery(recovery, result);
                }

                break;  // 一次只处理第一个检测到的异常
            }
        }

        // 4. 一切正常
        if (!hasException) {
            IF_CIRCUIT_BREAKER_ENABLED(
                breaker_.recordSuccess();
            )
            if (handler_) {
                handler_->updateValidValue(result.value);
            }
            lastValidValue_ = result.value;
        }

        return result;
    }

    /// 获取熔断器（供外部查询状态）
    CircuitBreaker* getCircuitBreaker() {
        IF_CIRCUIT_BREAKER_ENABLED(
            return &breaker_;
        )
        return NULL;
    }

    /// 获取日志记录器
    ExceptionLogger<>* getLogger() {
        IF_EXCEPTION_LOGGER_ENABLED(
            return &logger_;
        )
        return NULL;
    }

    /// 获取最后有效值
    float getLastValidValue() const { return lastValidValue_; }

    /// 重置所有状态
    void reset() {
        for (uint8_t i = 0; i < detectorCount_; ++i) {
            if (detectors_[i]) detectors_[i]->reset();
        }
        IF_CIRCUIT_BREAKER_ENABLED(breaker_.reset();)
        IF_EXCEPTION_LOGGER_ENABLED(logger_.clear();)
    }

    /// 注入时间提供者（供熔断器和心跳使用）
    void setTickProvider(uint32_t (*tickFunc)()) {
        IF_CIRCUIT_BREAKER_ENABLED(breaker_.setTickProvider(tickFunc);)
    }

private:
    SensorData executeRecovery(const RecoveryResult& recovery,
                               const SensorData& original)
    {
        SensorData result = original;

        switch (recovery.action) {
        case RecoveryAction::USE_LAST_VALID:
            result.value = recovery.fallbackValue;
            result.status = SensorStatus::DEGRADED;
            break;

        case RecoveryAction::USE_DEFAULT:
            result.value = recovery.fallbackValue;
            result.status = SensorStatus::DEGRADED;
            break;

        case RecoveryAction::RESET_SENSOR:
            if (sensor_) sensor_->reset();
            result.value = lastValidValue_;
            result.status = SensorStatus::ERROR;
            break;

        case RecoveryAction::STOP_SAMPLING:
            if (sensor_) sensor_->stop();
            result.value = recovery.fallbackValue;
            result.status = SensorStatus::ERROR;
            break;

        case RecoveryAction::SYSTEM_ALERT:
            result.value = lastValidValue_;
            result.status = SensorStatus::ERROR;
            break;

        default:
            break;
        }

        return result;
    }

    SensorBase<SensorData>*         sensor_;
    IExceptionDetector*             detectors_[MaxDetectors];
    uint8_t                         detectorCount_;
    ExceptionHandler*               handler_;
    bool                            initialized_;
    float                           lastValidValue_;

    IF_CIRCUIT_BREAKER_ENABLED(CircuitBreaker breaker_;)
    IF_EXCEPTION_LOGGER_ENABLED(ExceptionLogger<> logger_;)
};

#endif // SENSOR_FEATURE_EXCEPTION
