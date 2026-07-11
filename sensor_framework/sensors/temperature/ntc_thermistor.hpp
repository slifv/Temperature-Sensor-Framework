/**
 * @file ntc_thermistor.hpp
 * @brief NTC热敏电阻温度传感器 — ADC电压分压方式, 成本极低
 * @details 通过ADC读取分压电压 → 计算电阻 → Steinhart-Hart方程 → 温度。
 *          支持用户自定义B值/参考电阻/温度系数。
 *          典型精度 ±1°C（线性化后可到 ±0.5°C）。
 */

#pragma once

#include "../../feature_config.hpp"
#include "../../core/sensor_base.hpp"
#include "../../core/sensor_config.hpp"
#include "../../hal/hal_interface.hpp"

#if SENSOR_FEATURE_NTC && SENSOR_FEATURE_HAL_ADC

#include <cmath>
#include <cstdint>

/**
 * @class NTCThermistorSensor
 * @brief NTC热敏电阻温度传感器
 * @details 采用分压电路: VCC → R_fixed → ADC → NTC → GND
 *          使用 Steinhart-Hart 方程: 1/T = 1/T0 + (1/B) * ln(R/R0)
 */
class NTCThermistorSensor : public SensorBase<TemperatureData> {
public:
    /**
     * @brief NTC参数配置
     */
    struct NTCParams {
        float   rFixed;         ///< 固定分压电阻值(Ω)，典型10kΩ
        float   r0;             ///< NTC在T0时的电阻值(Ω)，典型10kΩ
        float   t0;             ///< 参考温度(K)，典型298.15K (25°C)
        float   bValue;         ///< B值系数，典型3950
        float   vcc;            ///< 供电电压(V)，典型3.3V
        uint8_t adcResolution;  ///< ADC分辨率(位)，典型10/12

        /// 默认参数 (10kΩ NTC, B=3950, 10kΩ分压电阻)
        static NTCParams defaultParams() {
            NTCParams p;
            p.rFixed        = 10000.0f;
            p.r0            = 10000.0f;
            p.t0            = 298.15f;   // 25°C in Kelvin
            p.bValue        = 3950.0f;
            p.vcc           = 3.3f;
            p.adcResolution = 12;
            return p;
        }
    };

    /**
     * @param id     传感器唯一ID
     * @param adc    ADC通道接口
     * @param params NTC参数（默认10kΩ/B3950）
     */
    NTCThermistorSensor(uint32_t id, IADCChannel& adc,
                        const NTCParams& params = NTCParams::defaultParams())
        : SensorBase<TemperatureData>(id, SensorType::TEMPERATURE)
        , adc_(adc), params_(params)
    {}

    // ─── 生命周期 ────────────────────────────────────────

    bool init() SENSOR_OVERRIDE {
        setStatus(SensorStatus::READY);
        return true;
    }

    bool start() SENSOR_OVERRIDE {
        setStatus(SensorStatus::RUNNING);
        return true;
    }

    bool stop() SENSOR_OVERRIDE {
        setStatus(SensorStatus::STOPPED);
        return true;
    }

    bool reset() SENSOR_OVERRIDE {
        SensorBase::reset();
        setStatus(SensorStatus::READY);
        return true;
    }

    // ─── 数据获取 ────────────────────────────────────────

    TemperatureData readRaw() SENSOR_OVERRIDE {
        TemperatureData data;
        data.sensorId    = sensorId_;
        data.type        = SensorType::TEMPERATURE;
        data.timestampMs = 0;
        data.status      = SensorStatus::RUNNING;

        // 1. 读取原始ADC值
        uint16_t rawADC = adc_.readRaw();

        // 2. 计算分压电压
        uint16_t adcMax = (1U << params_.adcResolution) - 1;
        float voltage = params_.vcc * static_cast<float>(rawADC) / static_cast<float>(adcMax);

        // 3. 计算NTC电阻值 (分压公式)
        //    V_adc = VCC * R_ntc / (R_fixed + R_ntc)
        //    R_ntc = R_fixed * V_adc / (VCC - V_adc)
        float rNtc = params_.rFixed * voltage / (params_.vcc - voltage + 0.001f);  // +0.001防止除零

        // 4. Steinhart-Hart 简化方程 (B参数模型)
        //    1/T = 1/T0 + (1/B) * ln(R/R0)
        //    T = 1 / (1/T0 + (1/B) * ln(R/R0))
        float lnRatio = std::log(rNtc / params_.r0);
        float invT = (1.0f / params_.t0) + (1.0f / params_.bValue) * lnRatio;
        float tempKelvin = 1.0f / invT;
        float tempCelsius = tempKelvin - 273.15f;

        data.value    = tempCelsius;
        data.rawValue = static_cast<float>(rawADC);
        data.status   = SensorStatus::RUNNING;

        return data;
    }

    // ─── 配置 ────────────────────────────────────────────

    const char* getName() const SENSOR_OVERRIDE { return "NTC_Thermistor"; }

    /// 更新NTC参数（运行时校准）
    void setParams(const NTCParams& params) { params_ = params; }

    const NTCParams& getParams() const { return params_; }

    /**
     * @brief 多点线性化校准
     * @param refTemps  已知参考温度点数组(K)
     * @param refValues 对应ADC读数
     * @param count     校准点数量
     * @return 拟合后的B值和R0（通过参数引用返回）
     */
    static bool calibrate(const float* refTemps, const float* refValues,
                          uint8_t count, float& outB, float& outR0)
    {
        if (count < 2) return false;

        // 使用两点校准: ln(R1/R2) = B * (1/T1 - 1/T2)
        float t1 = refTemps[0];
        float t2 = refTemps[count - 1];
        float r1 = refValues[0];
        float r2 = refValues[count - 1];

        outR0 = r1;
        outB = std::log(r1 / r2) / (1.0f / t1 - 1.0f / t2);

        return true;
    }

private:
    IADCChannel&    adc_;
    NTCParams       params_;
};

#endif // SENSOR_FEATURE_NTC
