/**
 * @file c_binding.h
 * @brief C语言胶水层 — 为不支持C++的编译器(SDCC/XC8)提供纯C接口
 * @details 使用不透明指针(void*)隐藏C++对象布局。
 *          通过 extern "C" 函数封装核心操作。
 *          参考: 需求评审报告 第18.5节
 */

#ifndef SENSOR_FRAMEWORK_C_BINDING_H
#define SENSOR_FRAMEWORK_C_BINDING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
//  不透明句柄类型
// ============================================================

/// 传感器实例句柄（C侧不感知内部结构）
typedef void* sensor_handle_t;

/// 滤波器实例句柄
typedef void* filter_handle_t;

/// 事件总线句柄
typedef void* eventbus_handle_t;

// ============================================================
//  传感器类型枚举 (C 兼容)
// ============================================================

enum c_sensor_type_e {
    C_SENSOR_TEMP       = 1,
    C_SENSOR_HUMIDITY   = 2,
    C_SENSOR_PRESSURE   = 3,
    C_SENSOR_LIGHT      = 4,
    C_SENSOR_UNKNOWN    = 0
};

enum c_sensor_status_e {
    C_STATUS_UNINIT     = 0,
    C_STATUS_READY      = 1,
    C_STATUS_RUNNING    = 2,
    C_STATUS_STOPPED    = 3,
    C_STATUS_ERROR      = 4,
    C_STATUS_COMM_ERROR = 5,
    C_STATUS_OFFLINE    = 6
};

// ============================================================
//  API 函数声明
// ============================================================

// ─── 传感器管理 ─────────────────────────────────────────

/// 创建传感器实例
/// @param sensor_type 传感器类型 (1=SHT30, 2=DS18B20, 3=NTC)
/// @param bus_id      总线ID（平台相关）
/// @return 传感器句柄，失败返回 NULL
sensor_handle_t sensor_create(uint8_t sensor_type, uint8_t bus_id);

/// 初始化传感器
/// @return 1=成功, 0=失败
uint8_t sensor_init(sensor_handle_t handle);

/// 启动采样
uint8_t sensor_start(sensor_handle_t handle);

/// 停止采样
uint8_t sensor_stop(sensor_handle_t handle);

/// 读取原始数据
/// @param handle 传感器句柄
/// @return 传感器读取的浮点值
float sensor_read_raw(sensor_handle_t handle);

/// 读取滤波后数据
float sensor_read_filtered(sensor_handle_t handle);

/// 获取传感器状态
uint8_t sensor_get_status(sensor_handle_t handle);

/// 销毁传感器实例
void sensor_destroy(sensor_handle_t handle);

// ─── 配置 ──────────────────────────────────────────────

/// 设置采样率 (Hz)
void sensor_set_sample_rate(sensor_handle_t handle, float rateHz);

/// 获取采样率 (Hz)
float sensor_get_sample_rate(sensor_handle_t handle);

// ─── 滤波器 ────────────────────────────────────────────

/// 创建均值滤波器
/// @param window_size 窗口大小
filter_handle_t filter_create_mean(uint8_t window_size);

/// 创建低通滤波器
/// @param alpha 滤波系数 (0, 1]
filter_handle_t filter_create_lowpass(float alpha);

/// 输入数据到滤波器
float filter_update(filter_handle_t handle, float input);

/// 重置滤波器
void filter_reset(filter_handle_t handle);

/// 销毁滤波器实例
void filter_destroy(filter_handle_t handle);

// ─── 事件总线 ──────────────────────────────────────────

/// 获取事件总线单例句柄
eventbus_handle_t eventbus_get_instance(void);

/// 订阅传感器数据
/// @param bus      总线句柄
/// @param sensorId 传感器ID
/// @param callback 回调函数（收到数据时调用）
/// @param context  用户上下文指针
void eventbus_subscribe(eventbus_handle_t bus,
                        uint32_t sensorId,
                        void (*callback)(float value, uint32_t timestamp, void* context),
                        void* context);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_FRAMEWORK_C_BINDING_H
