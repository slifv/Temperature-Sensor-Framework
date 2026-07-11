/**
 * @file business_logic.hpp
 * @brief 业务逻辑订阅者 — 阈值告警、安全联动等
 * @details 订阅传感器数据并执行业务逻辑:
 *          - 温度超限告警
 *          - PID控制计算
 *          - 安全联动（如加热器关闭）
 */

#pragma once

#include "../core/event_bus.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

#include <cstdint>

/**
 * @class AlarmSubscriber
 * @brief 阈值告警订阅者 — 温度超限时触发告警
 */
class AlarmSubscriber : public IDataSubscriber {
public:
    typedef void (*AlarmCallback)(uint32_t sensorId, float value,
                                  float threshold, bool isHigh);

    AlarmSubscriber(const char* name,
                    float highThreshold = 35.0f,
                    float lowThreshold = 15.0f,
                    AlarmCallback callback = NULL)
        : name_(name)
        , highThreshold_(highThreshold)
        , lowThreshold_(lowThreshold)
        , callback_(callback)
        , alarmActive_(false), alarmCount_(0)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        bool wasActive = alarmActive_;
        alarmActive_ = false;

        if (data.value > highThreshold_) {
            alarmActive_ = true;
            if (callback_) {
                callback_(data.sensorId, data.value, highThreshold_, true);
            }
            ++alarmCount_;
        } else if (data.value < lowThreshold_) {
            alarmActive_ = true;
            if (callback_) {
                callback_(data.sensorId, data.value, lowThreshold_, false);
            }
            ++alarmCount_;
        }

        if (!alarmActive_ && wasActive) {
            // 告警解除
            if (callback_) {
                callback_(data.sensorId, data.value, 0.0f, false);
            }
        }

        lastData_ = data;
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    void setThresholds(float high, float low) {
        highThreshold_ = high;
        lowThreshold_ = low;
    }

    bool isAlarmActive() const { return alarmActive_; }
    uint32_t getAlarmCount() const { return alarmCount_; }
    const SensorData& getLastData() const { return lastData_; }

private:
    const char*     name_;
    float           highThreshold_;
    float           lowThreshold_;
    AlarmCallback   callback_;
    bool            alarmActive_;
    uint32_t        alarmCount_;
    SensorData      lastData_;
};

/**
 * @class SafetySubscriber
 * @brief 安全联动订阅者 — 检测异常时执行保护动作
 * @details 典型场景: 温度过高 → 自动关闭加热器
 */
class SafetySubscriber : public IDataSubscriber {
public:
    typedef void (*SafetyAction)(uint32_t sensorId, bool activate);

    SafetySubscriber(const char* name, float dangerThreshold,
                     SafetyAction action)
        : name_(name)
        , dangerThreshold_(dangerThreshold)
        , action_(action)
        , safetyActive_(false)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        if (data.value > dangerThreshold_ && !safetyActive_) {
            safetyActive_ = true;
            if (action_) {
                action_(data.sensorId, true);  // 激活保护
            }
        } else if (data.value <= dangerThreshold_ * 0.8f && safetyActive_) {
            // 滞后80%恢复（防止抖动）
            safetyActive_ = false;
            if (action_) {
                action_(data.sensorId, false); // 解除保护
            }
        }

        lastData_ = data;
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    bool isSafetyActive() const { return safetyActive_; }
    const SensorData& getLastData() const { return lastData_; }

private:
    const char*     name_;
    float           dangerThreshold_;
    SafetyAction    action_;
    bool            safetyActive_;
    SensorData      lastData_;
};

/**
 * @class PIDController
 * @brief 简易PID控制器 — 从传感器数据计算控制输出
 */
class PIDController {
public:
    PIDController(float Kp = 1.0f, float Ki = 0.0f, float Kd = 0.0f,
                  float setpoint = 25.0f)
        : Kp_(Kp), Ki_(Ki), Kd_(Kd)
        , setpoint_(setpoint)
        , integral_(0.0f), lastError_(0.0f)
        , outputMin_(-100.0f), outputMax_(100.0f)
    {}

    /**
     * @brief 输入当前测量值，计算PID输出
     * @param measurement 当前测量值
     * @param dt          距上次计算的时间间隔(秒)
     * @return 控制输出值
     */
    float compute(float measurement, float dt) {
        float error = setpoint_ - measurement;

        // 比例项
        float pTerm = Kp_ * error;

        // 积分项（带抗饱和）
        integral_ += error * dt;
        float iTerm = Ki_ * integral_;

        // 微分项
        float derivative = (dt > 0.001f) ? (error - lastError_) / dt : 0.0f;
        float dTerm = Kd_ * derivative;

        lastError_ = error;

        // 输出限幅
        float output = pTerm + iTerm + dTerm;
        return clamp(output, outputMin_, outputMax_);
    }

    void setSetpoint(float sp) { setpoint_ = sp; }
    void setTunings(float Kp, float Ki, float Kd) {
        Kp_ = Kp; Ki_ = Ki; Kd_ = Kd;
    }
    void setOutputLimits(float min, float max) {
        outputMin_ = min; outputMax_ = max;
    }

    void reset() {
        integral_ = 0.0f;
        lastError_ = 0.0f;
    }

    float getSetpoint() const { return setpoint_; }
    float getIntegral() const { return integral_; }

private:
    static float clamp(float val, float min, float max) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }

    float Kp_, Ki_, Kd_;
    float setpoint_;
    float integral_;
    float lastError_;
    float outputMin_, outputMax_;
};

#endif // SENSOR_FEATURE_EVENTBUS
