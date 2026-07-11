/**
 * @file data_subscriber.hpp
 * @brief 数据订阅者接口和内置订阅者实现
 * @details 应用层通过实现 IDataSubscriber 接口来消费传感器数据。
 *          包含若干内置订阅者类型便于快速开发。
 *          参考: 需求评审报告 第7.1节
 */

#pragma once

#include "../core/event_bus.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

#include <cstdint>

/// 重新导出（统一使用 distribution 命名空间）
using ::IDataSubscriber;

/**
 * @class LambdaSubscriber
 * @brief 函数指针/函数对象订阅者 — 用于快速绑定回调
 */
class LambdaSubscriber : public IDataSubscriber {
public:
    typedef void (*Callback)(const SensorData& data, void* context);

    LambdaSubscriber(const char* name, Callback callback, void* context = NULL)
        : name_(name), callback_(callback), context_(context)
        , callCount_(0)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        ++callCount_;
        if (callback_) {
            callback_(data, context_);
        }
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    /// 获取回调调用次数
    uint32_t getCallCount() const { return callCount_; }

    /// 获取最后接收的数据
    const SensorData& getLastData() const { return lastData_; }

private:
    const char* name_;
    Callback    callback_;
    void*       context_;
    uint32_t    callCount_;
    SensorData  lastData_;
};

/**
 * @class FilteredSubscriber
 * @brief 带数据过滤的订阅者装饰器
 * @details 仅当数据满足条件时才通知内部订阅者
 */
class FilteredSubscriber : public IDataSubscriber {
public:
    typedef bool (*FilterFunc)(const SensorData& data);

    FilteredSubscriber(IDataSubscriber* inner, FilterFunc filter)
        : inner_(inner), filter_(filter)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        if (filter_ && filter_(data) && inner_) {
            inner_->onDataReceived(data);
        }
    }

    const char* getName() const SENSOR_OVERRIDE {
        return inner_ ? inner_->getName() : "Filtered";
    }

private:
    IDataSubscriber* inner_;
    FilterFunc filter_;
};

/// 静态方法：创建仅当数据超过阈值时才通知的过滤器
static bool thresholdExceededFilter(const SensorData& data, float threshold) {
    return data.value > threshold;
}

#endif // SENSOR_FEATURE_EVENTBUS
