/**
 * @file event_bus.hpp
 * @brief 事件总线 — 发布-订阅模式核心，传感器数据分发枢纽
 * @details 支持多对多绑定：多个传感器 → 多个订阅者。
 *          裁剪时 publish() 转为空操作，保持接口一致。
 *          参考: 需求评审报告 第7.1节, 第17.4.3节
 */

#pragma once

#include "../feature_config.hpp"
#include "../compiler_features.hpp"
#include "../utils/static_vector.hpp"
#include "sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

/**
 * @class IDataSubscriber
 * @brief 数据订阅者抽象接口
 */
class IDataSubscriber {
public:
    virtual ~IDataSubscriber() {}

    /// 接收到传感器数据时的回调
    virtual void onDataReceived(const SensorData& data) = 0;

    /// 获取订阅者名称（调试用）
    virtual const char* getName() const = 0;
};

/**
 * @class EventBus
 * @brief 事件总线 — 发布-订阅模式核心
 * @tparam MaxSubscribersPerSensor 每个传感器最大订阅者数
 */
template <uint8_t MaxSubscribersPerSensor = SENSOR_CONSTRAINT_MAX_SUBSCRIBERS>
class EventBus {
public:
    /// 获取单例
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    /**
     * @brief 订阅某个传感器的数据
     * @param sensorId   要订阅的传感器ID
     * @param subscriber 订阅者指针（不接管所有权）
     */
    void subscribe(uint32_t sensorId, IDataSubscriber* subscriber) {
        if (!subscriber) return;

        Binding* binding = findOrCreateBinding(sensorId);
        if (binding) {
            binding->subscribers.push_back(subscriber);
        }
    }

    /**
     * @brief 取消订阅
     * @param sensorId   传感器ID
     * @param subscriber 要移除的订阅者
     */
    void unsubscribe(uint32_t sensorId, IDataSubscriber* subscriber) {
        Binding* binding = findBinding(sensorId);
        if (binding) {
            binding->subscribers.removeValue(subscriber);
        }
    }

    /**
     * @brief 发布数据给所有订阅者
     * @details 遍历该传感器的所有订阅者并调用onDataReceived()
     */
    void publish(const SensorData& data) {
        Binding* binding = findBinding(data.sensorId);
        if (!binding) return;

        for (uint8_t i = 0; i < binding->subscribers.size(); ++i) {
            IDataSubscriber* sub = binding->subscribers[i];
            if (sub) {
                sub->onDataReceived(data);
            }
        }
    }

    /**
     * @brief 取消某个传感器的所有订阅
     */
    void unsubscribeAll(uint32_t sensorId) {
        for (uint8_t i = 0; i < bindings_.size(); ++i) {
            if (bindings_[i].sensorId == sensorId) {
                bindings_[i].subscribers.clear();
                return;
            }
        }
    }

    /// 获取某个传感器的订阅者数量
    uint8_t getSubscriberCount(uint32_t sensorId) const {
        const Binding* binding = findBinding(sensorId);
        if (binding) return binding->subscribers.size();
        return 0;
    }

    /// 获取总绑定数
    uint8_t getBindingCount() const { return bindings_.size(); }

private:
    EventBus() {}
    ~EventBus() {}

    // 禁止拷贝
    EventBus(const EventBus&) SENSOR_DELETE_FUNC;
    EventBus& operator=(const EventBus&) SENSOR_DELETE_FUNC;

    struct Binding {
        uint32_t sensorId;
        StaticVector<IDataSubscriber*, MaxSubscribersPerSensor> subscribers;

        Binding() : sensorId(0) {}
        explicit Binding(uint32_t id) : sensorId(id) {}
    };

    Binding* findBinding(uint32_t sensorId) {
        for (uint8_t i = 0; i < bindings_.size(); ++i) {
            if (bindings_[i].sensorId == sensorId) {
                return &bindings_[i];
            }
        }
        return NULL;
    }

    const Binding* findBinding(uint32_t sensorId) const {
        for (uint8_t i = 0; i < bindings_.size(); ++i) {
            if (bindings_[i].sensorId == sensorId) {
                return &bindings_[i];
            }
        }
        return NULL;
    }

    Binding* findOrCreateBinding(uint32_t sensorId) {
        Binding* existing = findBinding(sensorId);
        if (existing) return existing;

        if (bindings_.is_full()) return NULL;

        Binding newBinding(sensorId);
        if (bindings_.push_back(newBinding)) {
            return &bindings_[bindings_.size() - 1];
        }
        return NULL;
    }

    StaticVector<Binding, SENSOR_CONSTRAINT_MAX_SENSORS> bindings_;
};

#else
// ─── EventBus关闭时：空实现，保持接口一致 ────────────────

class IDataSubscriber {
public:
    virtual ~IDataSubscriber() {}
    virtual void onDataReceived(const SensorData& data) = 0;
    virtual const char* getName() const = 0;
};

class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    void subscribe(uint32_t, IDataSubscriber*) { /* no-op */ }
    void unsubscribe(uint32_t, IDataSubscriber*) { /* no-op */ }
    void publish(const SensorData&) { /* no-op: 数据不分发 */ }
    void unsubscribeAll(uint32_t) { /* no-op */ }
    uint8_t getSubscriberCount(uint32_t) const { return 0; }
    uint8_t getBindingCount() const { return 0; }

private:
    EventBus() {}
    ~EventBus() {}
    EventBus(const EventBus&) SENSOR_DELETE_FUNC;
    EventBus& operator=(const EventBus&) SENSOR_DELETE_FUNC;
};

#endif // SENSOR_FEATURE_EVENTBUS
