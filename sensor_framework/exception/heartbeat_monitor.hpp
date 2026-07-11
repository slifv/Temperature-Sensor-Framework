/**
 * @file heartbeat_monitor.hpp
 * @brief 心跳监控 — 每个传感器定时上报"存活"状态
 * @details 超时未上报则标记传感器为离线状态。
 *          适合有RTOS定时器的场景。
 *          参考: 需求评审报告 FR-EH-06
 */

#pragma once

#include "../feature_config.hpp"

#if SENSOR_FEATURE_HEARTBEAT

#include <cstdint>

/**
 * @class HeartbeatMonitor
 * @brief 传感器心跳监控器
 * @tparam MaxSensors 最大监控传感器数量
 */
template <uint8_t MaxSensors = SENSOR_CONSTRAINT_MAX_SENSORS>
class HeartbeatMonitor {
public:
    /**
     * @param timeoutMs 心跳超时时间（ms），超过此时间未上报视为离线
     */
    explicit HeartbeatMonitor(uint32_t timeoutMs = 5000)
        : timeoutMs_(timeoutMs), count_(0)
    {}

    /**
     * @brief 注册传感器到心跳监控
     */
    void registerSensor(uint32_t sensorId) {
        for (uint8_t i = 0; i < count_; ++i) {
            if (entries_[i].sensorId == sensorId) {
                return;  // 已注册
            }
        }
        if (count_ < MaxSensors) {
            entries_[count_].sensorId = sensorId;
            entries_[count_].lastBeatMs = getCurrentTick();
            entries_[count_].offline = false;
            ++count_;
        }
    }

    /**
     * @brief 传感器上报心跳（通常在采样成功时调用）
     */
    void beat(uint32_t sensorId) {
        for (uint8_t i = 0; i < count_; ++i) {
            if (entries_[i].sensorId == sensorId) {
                entries_[i].lastBeatMs = getCurrentTick();
                entries_[i].offline = false;
                entries_[i].missedBeats = 0;
                return;
            }
        }
    }

    /**
     * @brief 检查所有传感器心跳状态（应周期性调用）
     * @return 返回超时离线的传感器数量
     */
    uint8_t checkAll() {
        uint8_t offlineCount = 0;
        uint32_t now = getCurrentTick();

        for (uint8_t i = 0; i < count_; ++i) {
            if (!entries_[i].offline) {
                if (now - entries_[i].lastBeatMs > timeoutMs_) {
                    entries_[i].offline = true;
                    entries_[i].missedBeats++;
                }
            }
            if (entries_[i].offline) {
                ++offlineCount;
            }
        }

        return offlineCount;
    }

    /// 查询某传感器是否在线
    bool isOnline(uint32_t sensorId) const {
        for (uint8_t i = 0; i < count_; ++i) {
            if (entries_[i].sensorId == sensorId) {
                return !entries_[i].offline;
            }
        }
        return false;
    }

    /// 获取丢失心跳次数
    uint32_t getMissedBeats(uint32_t sensorId) const {
        for (uint8_t i = 0; i < count_; ++i) {
            if (entries_[i].sensorId == sensorId) {
                return entries_[i].missedBeats;
            }
        }
        return 0;
    }

    void setTimeout(uint32_t ms) { timeoutMs_ = ms; }

    void setTickProvider(uint32_t (*tickFunc)()) { tickProvider_ = tickFunc; }

private:
    uint32_t getCurrentTick() const {
        return tickProvider_ ? tickProvider_() : 0;
    }

    struct HeartbeatEntry {
        uint32_t sensorId;
        uint32_t lastBeatMs;
        uint32_t missedBeats;
        bool     offline;
    };

    HeartbeatEntry  entries_[MaxSensors];
    uint32_t        timeoutMs_;
    uint8_t         count_;
    uint32_t        (*tickProvider_)();
};

#endif // SENSOR_FEATURE_HEARTBEAT
