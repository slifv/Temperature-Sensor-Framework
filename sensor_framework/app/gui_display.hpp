/**
 * @file gui_display.hpp
 * @brief GUI显示订阅者 — 订阅传感器数据并在屏幕上显示
 * @details 实现IDataSubscriber接口，将传感器数据推送到LCD/OLED显示。
 *          可按显示类型（数值/曲线/仪表盘）定制。
 */

#pragma once

#include "../core/event_bus.hpp"
#include "../core/sensor_config.hpp"

#if SENSOR_FEATURE_EVENTBUS

/**
 * @class GuiDisplaySubscriber
 * @brief GUI显示订阅者 — 将传感器数据渲染到屏幕
 */
class GuiDisplaySubscriber : public IDataSubscriber {
public:
    typedef void (*DisplayFunc)(const char* label, float value, const char* unit);

    GuiDisplaySubscriber(const char* name, DisplayFunc displayFunc)
        : name_(name), displayFunc_(displayFunc), updateCount_(0)
    {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        ++updateCount_;

        if (displayFunc_) {
            char label[32];
            formatLabel(data, label, sizeof(label));
            displayFunc_(label, data.value, getUnit(data.type));
        }

        lastData_ = data;
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    uint32_t getUpdateCount() const { return updateCount_; }
    const SensorData& getLastData() const { return lastData_; }

private:
    static const char* getUnit(SensorType type) {
        switch (type) {
            case SensorType::TEMPERATURE: return "\xb0\x43";  // °C
            case SensorType::HUMIDITY:    return "%";
            case SensorType::PRESSURE:    return "hPa";
            case SensorType::VOLTAGE:     return "V";
            case SensorType::CURRENT:     return "A";
            default:                      return "";
        }
    }

    static void formatLabel(const SensorData& data, char* buf, size_t len) {
        const char* typeNames[] = {
            "?", "Temp", "Humi", "Pres", "Light",
            "Accel", "Gyro", "Mag", "Prox", "Gas", "VOC",
            "Curr", "Volt"
        };
        uint8_t typeIdx = static_cast<uint8_t>(data.type);
        const char* typeName = (typeIdx < 13) ? typeNames[typeIdx] : "??";

        // 使用snprintf风格的格式化（如果可用）
        int written = 0;
        for (const char* p = typeName; *p && written < (int)len - 10; ++p) {
            buf[written++] = *p;
        }
        buf[written++] = '#';
        // 简单数字转字符串
        uint32_t id = data.sensorId;
        char idBuf[6];
        uint8_t idLen = 0;
        do {
            idBuf[idLen++] = '0' + (id % 10);
            id /= 10;
        } while (id > 0 && idLen < 5);
        while (idLen > 0) {
            buf[written++] = idBuf[--idLen];
        }
        buf[written] = '\0';
    }

    const char*     name_;
    DisplayFunc     displayFunc_;
    uint32_t        updateCount_;
    SensorData      lastData_;
};

#endif // SENSOR_FEATURE_EVENTBUS
