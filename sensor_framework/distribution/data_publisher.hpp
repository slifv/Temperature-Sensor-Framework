/**
 * @file data_publisher.hpp
 * @brief 数据发布者接口 — 将传感器数据发布到EventBus的便捷包装
 */

#pragma once

#include "../core/event_bus.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

/**
 * @class DataPublisher
 * @brief 数据发布者 — 将采样数据便捷地发布到EventBus
 */
class DataPublisher {
public:
    DataPublisher(EventBus<>& bus) : bus_(bus) {}

    /**
     * @brief 发布传感器数据到总线
     */
    void publish(const SensorData& data) {
        bus_.publish(data);
    }

    /**
     * @brief 构建并发布传感器数据
     */
    void publishData(uint32_t sensorId, SensorType type,
                     float rawValue, float filteredValue,
                     uint32_t timestampMs, SensorStatus status)
    {
        SensorData data;
        data.sensorId       = sensorId;
        data.type           = type;
        data.rawValue       = rawValue;
        data.value          = filteredValue;
        data.timestampMs    = timestampMs;
        data.status         = status;
        bus_.publish(data);
    }

private:
    EventBus<>& bus_;
};

#endif // SENSOR_FEATURE_EVENTBUS
