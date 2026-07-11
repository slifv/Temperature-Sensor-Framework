/**
 * @file sensor_manager.hpp
 * @brief 传感器管理器 — 单例模式，统一管理所有传感器实例
 * @details 提供传感器注册/注销/查找/生命周期批量控制。
 *          所有传感器实例通过此管理器进行中央调度。
 *          参考: 需求评审报告 第5.1节, 第2.1节
 */

#pragma once

#include "../feature_config.hpp"
#include "../utils/static_vector.hpp"
#include "sensor_base.hpp"
#include "sensor_config.hpp"

/**
 * @class SensorManager
 * @brief 传感器管理器（单例模式）
 * @tparam MaxSensors 最大管理传感器数量
 */
template <uint8_t MaxSensors = SENSOR_CONSTRAINT_MAX_SENSORS>
class SensorManager {
public:
    /// 获取单例
    static SensorManager& getInstance() {
        static SensorManager instance;
        return instance;
    }

    /**
     * @brief 注册传感器实例
     * @param sensor 传感器指针（管理器接管所有权）
     * @return 注册成功返回 true（ID冲突或已满返回 false）
     */
    bool registerSensor(SensorBase<SensorData>* sensor) {
        if (!sensor) return false;
        if (sensors_.is_full()) return false;

        // 检查ID冲突
        uint32_t newId = sensor->getSensorId();
        if (findSensorById(newId) != NULL) {
            return false;
        }

        return sensors_.push_back(sensor);
    }

    /**
     * @brief 注销并销毁传感器实例
     * @param sensorId 传感器唯一ID
     * @return 找到并销毁返回 true
     */
    bool unregisterSensor(uint32_t sensorId) {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->getSensorId() == sensorId) {
                delete sensors_[i];
                sensors_.remove(i);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 按ID查找传感器
     * @return 找到返回指针，未找到返回 NULL
     */
    SensorBase<SensorData>* findSensorById(uint32_t sensorId) {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->getSensorId() == sensorId) {
                return sensors_[i];
            }
        }
        return NULL;
    }

    /**
     * @brief 按类型查找传感器
     * @return 找到返回指针，未找到返回 NULL
     */
    SensorBase<SensorData>* findSensorByType(SensorType type) {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->getType() == type) {
                return sensors_[i];
            }
        }
        return NULL;
    }

    /**
     * @brief 获取所有指定类型的传感器
     * @param type  传感器类型
     * @param out   输出数组
     * @param maxOut 输出数组容量
     * @return 找到的数量
     */
    uint8_t findSensorsByType(SensorType type,
                              SensorBase<SensorData>** out,
                              uint8_t maxOut)
    {
        uint8_t count = 0;
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->getType() == type && count < maxOut) {
                out[count++] = sensors_[i];
            }
        }
        return count;
    }

    /**
     * @brief 初始化所有传感器
     * @return 成功初始化的数量
     */
    uint8_t initAll() {
        uint8_t successCount = 0;
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->init()) {
                ++successCount;
            }
        }
        return successCount;
    }

    /**
     * @brief 启动所有传感器采样
     * @return 成功启动的数量
     */
    uint8_t startAll() {
        uint8_t successCount = 0;
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->start()) {
                ++successCount;
            }
        }
        return successCount;
    }

    /**
     * @brief 停止所有传感器采样
     */
    void stopAll() {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            sensors_[i]->stop();
        }
    }

    /**
     * @brief 执行一次全部传感器采样
     * @details 遍历所有运行中的传感器，执行 readFiltered() 并回调
     * @param callback 每采样一个传感器后调用的回调（可为NULL）
     */
    void sampleAll(void (*callback)(const SensorData&) = NULL) {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            SensorBase<SensorData>* sensor = sensors_[i];
            if (sensor->getStatus() == SensorStatus::RUNNING) {
                SensorData data = sensor->readFiltered();
                if (callback) {
                    callback(data);
                }
            }
        }
    }

    /// 按状态获取传感器数量
    uint8_t getCountByStatus(SensorStatus status) const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            if (sensors_[i]->getStatus() == status) {
                ++count;
            }
        }
        return count;
    }

    /// 获取已注册传感器总数
    uint8_t getSensorCount() const { return sensors_.size(); }

    /// 获取最大可管理传感器数量
    static uint8_t getMaxSensors() { return MaxSensors; }

    /// 按索引访问传感器
    SensorBase<SensorData>* getSensorAt(uint8_t index) {
        if (index < sensors_.size()) {
            return sensors_[index];
        }
        return NULL;
    }

    /// 销毁所有传感器
    void destroyAll() {
        for (uint8_t i = 0; i < sensors_.size(); ++i) {
            delete sensors_[i];
        }
        sensors_.clear();
    }

private:
    SensorManager() {}
    ~SensorManager() { destroyAll(); }

    // 禁止拷贝
    SensorManager(const SensorManager&) SENSOR_DELETE_FUNC;
    SensorManager& operator=(const SensorManager&) SENSOR_DELETE_FUNC;

    StaticVector<SensorBase<SensorData>*, MaxSensors> sensors_;
};
