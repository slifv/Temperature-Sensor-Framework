/**
 * @file adc_channel.hpp
 * @brief ADC通道封装 — 电压转换、多次采样平均辅助功能
 */

#pragma once

#include "../hal_interface.hpp"
#include "../../feature_config.hpp"
#include "../../compiler_features.hpp"

#if SENSOR_FEATURE_HAL_ADC

#include <cstdint>

/**
 * @class ADCChannelWrapper
 * @brief ADC通道包装器：自动多次采样取平均，简化电压读取
 */
class ADCChannelWrapper {
public:
    ADCChannelWrapper(IADCChannel& channel)
        : channel_(channel)
    {}

    IADCChannel& getRaw() { return channel_; }

    /**
     * @brief 多次采样取平均值
     * @param samples 采样次数
     * @return 平均电压值(V)
     */
    float readVoltageAveraged(uint8_t samples = 4) {
        float sum = 0.0f;
        for (uint8_t i = 0; i < samples; ++i) {
            sum += channel_.readVoltage();
        }
        return sum / static_cast<float>(samples);
    }

    /**
     * @brief 多次采样取原始值平均
     */
    uint16_t readRawAveraged(uint8_t samples = 4) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < samples; ++i) {
            sum += channel_.readRaw();
        }
        return static_cast<uint16_t>(sum / samples);
    }

    /**
     * @brief 读取电压（带超量程检测）
     * @param outVoltage 输出电压值
     * @return 数据有效返回 true（未超量程）
     */
    bool readVoltageSafe(float& outVoltage) {
        outVoltage = channel_.readVoltage();
        float ref = channel_.getReferenceVoltage();
        return outVoltage >= 0.0f && outVoltage <= ref;
    }

    /**
     * @brief 将原始ADC值转换为电压值
     * @param raw 原始ADC值
     * @return 电压(V)
     */
    float rawToVoltage(uint16_t raw) const {
        uint16_t maxRaw = (1U << channel_.getResolution()) - 1;
        return static_cast<float>(raw) * channel_.getReferenceVoltage()
               / static_cast<float>(maxRaw);
    }

    /// 单次读取电压
    float readVoltage() { return channel_.readVoltage(); }

    /// 单次读取原始值
    uint16_t readRaw() { return channel_.readRaw(); }

private:
    IADCChannel& channel_;
};

#endif // SENSOR_FEATURE_HAL_ADC
