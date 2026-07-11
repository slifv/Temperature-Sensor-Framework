/**
 * @file mqtt_sender.hpp
 * @brief MQTT发送订阅者 — 订阅传感器数据并通过MQTT上报云端
 * @details 将传感器数据序列化为JSON并通过MQTT发布到指定Topic。
 *          支持数据缓冲和批量上报以降低网络开销。
 */

#pragma once

#include "../core/event_bus.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

#include <cstdint>
#include <cstring>

/**
 * @class MqttSenderSubscriber
 * @brief MQTT数据上报订阅者
 */
class MqttSenderSubscriber : public IDataSubscriber {
public:
    /// MQTT发布函数原型: (topic, payload, payload_len) -> bool
    typedef bool (*MqttPublishFunc)(const char* topic, const char* payload, uint16_t len);

    /**
     * @param name        订阅者名称
     * @param baseTopic   MQTT Topic前缀 (如 "sensors/temperature")
     * @param publishFunc MQTT发布函数（平台相关实现）
     */
    MqttSenderSubscriber(const char* name, const char* baseTopic,
                         MqttPublishFunc publishFunc)
        : name_(name), baseTopic_(baseTopic)
        , publishFunc_(publishFunc)
        , sendCount_(0), failCount_(0)
        , batchInterval_(1), batchCount_(0)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        ++batchCount_;

        // 批处理：每 batchInterval_ 次才发送一次
        if (batchCount_ < batchInterval_) {
            return;
        }
        batchCount_ = 0;

        // 构建JSON payload
        char payload[128];
        int len = buildJsonPayload(data, payload, sizeof(payload));

        if (len > 0 && publishFunc_) {
            if (publishFunc_(baseTopic_, payload, static_cast<uint16_t>(len))) {
                ++sendCount_;
            } else {
                ++failCount_;
            }
        }

        lastData_ = data;
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    /// 设置批处理间隔（每N次数据发送1次）
    void setBatchInterval(uint8_t interval) { batchInterval_ = interval; }

    uint32_t getSendCount() const { return sendCount_; }
    uint32_t getFailCount() const { return failCount_; }
    const SensorData& getLastData() const { return lastData_; }

private:
    static int buildJsonPayload(const SensorData& data, char* buf, size_t bufSize) {
        // 简易JSON构建（嵌入式环境中避免sprintf大库依赖）
        const char* prefix = "{\"id\":";
        const char* mid1  = ",\"val\":";
        const char* mid2  = ",\"raw\":";
        const char* mid3  = ",\"ts\":";
        const char* suffix = "}";

        int pos = 0;

        // Copy prefix
        for (const char* p = prefix; *p && pos < (int)bufSize - 1; ++p) buf[pos++] = *p;
        pos += uintToStr(data.sensorId, buf + pos, bufSize - pos);
        for (const char* p = mid1; *p && pos < (int)bufSize - 1; ++p) buf[pos++] = *p;
        pos += floatToStr(data.value, buf + pos, bufSize - pos);
        for (const char* p = mid2; *p && pos < (int)bufSize - 1; ++p) buf[pos++] = *p;
        pos += floatToStr(data.rawValue, buf + pos, bufSize - pos);
        for (const char* p = mid3; *p && pos < (int)bufSize - 1; ++p) buf[pos++] = *p;
        pos += uintToStr(data.timestampMs, buf + pos, bufSize - pos);
        for (const char* p = suffix; *p && pos < (int)bufSize - 1; ++p) buf[pos++] = *p;

        buf[pos] = '\0';
        return pos;
    }

    static int uintToStr(uint32_t val, char* buf, size_t bufSize) {
        if (bufSize < 2) return 0;
        char temp[11];
        uint8_t len = 0;
        if (val == 0) temp[len++] = '0';
        while (val > 0 && len < 10) {
            temp[len++] = '0' + (val % 10);
            val /= 10;
        }
        uint8_t actualLen = len;
        if (actualLen >= (int)bufSize) actualLen = (uint8_t)bufSize - 1;
        for (uint8_t i = 0; i < actualLen; ++i) {
            buf[i] = temp[len - 1 - i];
        }
        return actualLen;
    }

    static int floatToStr(float val, char* buf, size_t bufSize) {
        if (bufSize < 2) return 0;
        // 简化: 整数部分 + "." + 1位小数
        int32_t intPart = static_cast<int32_t>(val);
        int32_t fracPart = static_cast<int32_t>((val - intPart) * 10.0f + 0.5f);
        if (fracPart < 0) fracPart = -fracPart;
        if (fracPart > 9) fracPart = 9;

        int pos = uintToStr(static_cast<uint32_t>(intPart < 0 ? -intPart : intPart), buf, bufSize);
        if (intPart < 0) {
            // shift and add minus
            for (int i = pos; i > 0; --i) buf[i] = buf[i-1];
            buf[0] = '-';
            ++pos;
        }
        if (pos < (int)bufSize - 2) {
            buf[pos++] = '.';
            buf[pos++] = '0' + fracPart;
        }
        buf[pos] = '\0';
        return pos;
    }

    const char*     name_;
    const char*     baseTopic_;
    MqttPublishFunc publishFunc_;
    uint32_t        sendCount_;
    uint32_t        failCount_;
    uint8_t         batchInterval_;
    uint8_t         batchCount_;
    SensorData      lastData_;
};

#endif // SENSOR_FEATURE_EVENTBUS
