/**
 * @file sensor_data.hpp
 * @brief 传感器数据结构 — 统一的数据包格式
 * @details 重新导出 core/sensor_config.hpp 中的 SensorData / TemperatureData，
 *          作为 distribution 子系统的独立入口。
 */

#pragma once

#include "../core/sensor_config.hpp"

// SensorData, TemperatureData, SensorType, SensorStatus 等
// 统一定义在 core/sensor_config.hpp 中
// 本文件作为 distribution 命名空间下的便捷包含入口
