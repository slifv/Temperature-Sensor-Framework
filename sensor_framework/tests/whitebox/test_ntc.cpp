/**
 * @file test_ntc.cpp
 * @brief NTC热敏电阻温度传感器 白盒单元测试
 * @details 使用 MockADCChannel 预设ADC读数，覆盖构造、初始化、
 *          数据读取、生命周期状态机和参数配置等场景。
 *          NTC采用分压电路: VCC → R_fixed → ADC → NTC → GND
 *          使用 Steinhart-Hart 方程: 1/T = 1/T0 + (1/B) * ln(R/R0)
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../test_framework/mock_hal.hpp"
#include "../../sensors/temperature/ntc_thermistor.hpp"

// ============================================================
//  ADC原始值预计算（12-bit, VCC=3.3V, rFixed=10k, R0=10k）
// ============================================================

// 25°C 时: R_ntc = R0 = 10kΩ
// V_adc = VCC * R_ntc / (rFixed + R_ntc) = 3.3 * 10k / 20k = 1.65V
// raw = 1.65 / 3.3 * 4095 = 2047.5 → 2048
static const uint16_t kADCAt25C = 2048;

// 0°C 时: R_ntc = R0 * exp(B*(1/273.15 - 1/298.15))
//       = 10000 * exp(3950 * 0.000307) ≈ 33630Ω
// V_adc = 3.3 * 33630 / (10000 + 33630) ≈ 2.5436V
// raw = 2.5436 / 3.3 * 4095 ≈ 3157
static const uint16_t kADCAt0C = 3157;

// ============================================================
//  构造函数测试
// ============================================================

TEST(NTC, ConstructionDefaults) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    TEST_ASSERT_EQ(sensor.getSensorId(), 1u, "Sensor ID should be 1");
    TEST_ASSERT_EQ(sensor.getType(), SensorType::TEMPERATURE,
                   "Type should be TEMPERATURE");
    TEST_ASSERT_TRUE(
        (strcmp(sensor.getName(), "NTC_Thermistor") == 0),
        "Name should be NTC_Thermistor");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::UNINIT,
                   "Status should be UNINIT");
    return true;
}

// ============================================================
//  默认参数测试
// ============================================================

TEST(NTC, DefaultParams) {
    NTCThermistorSensor::NTCParams params =
        NTCThermistorSensor::NTCParams::defaultParams();

    TEST_ASSERT_FLOAT_EQ(params.r0, 10000.0f, 0.01f,
                         "Default R0 should be 10000");
    TEST_ASSERT_FLOAT_EQ(params.bValue, 3950.0f, 0.01f,
                         "Default B value should be 3950");
    TEST_ASSERT_FLOAT_EQ(params.rFixed, 10000.0f, 0.01f,
                         "Default rFixed should be 10000");
    TEST_ASSERT_FLOAT_EQ(params.t0, 298.15f, 0.01f,
                         "Default T0 should be 298.15K (25C)");
    TEST_ASSERT_FLOAT_EQ(params.vcc, 3.3f, 0.01f,
                         "Default VCC should be 3.3V");
    TEST_ASSERT_EQ(params.adcResolution, static_cast<uint8_t>(12),
                   "Default ADC resolution should be 12");
    return true;
}

// ============================================================
//  初始化测试
// ============================================================

TEST(NTC, InitSucceeds) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    bool ok = sensor.init();

    TEST_ASSERT_TRUE(ok, "init() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "Status should be READY after init");
    return true;
}

// ============================================================
//  数据读取测试
// ============================================================

TEST(NTC, ReadAt25C) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    // 预设ADC值为 2048（对应 1.65V，即 25°C 分压点）
    adc.setRaw(kADCAt25C);

    // init + start 使状态进入 RUNNING
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed");

    TemperatureData data = sensor.readRaw();

    // 25°C 时 R_ntc == R0，ln(R/R0) = 0 → T = T0 = 298.15K → 25.0°C
    TEST_ASSERT_FLOAT_EQ(data.value, 25.0f, 1.0f,
                         "Temperature should be ~25.0 C");
    TEST_ASSERT_EQ(data.sensorId, 1u, "Sensor ID should be 1");
    TEST_ASSERT_EQ(data.type, SensorType::TEMPERATURE,
                   "Data type should be TEMPERATURE");
    TEST_ASSERT_EQ(data.status, SensorStatus::RUNNING,
                   "Status after readRaw should be RUNNING");

    return true;
}

TEST(NTC, ReadAt0C) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    // 预设ADC值为 3157（对应约 2.544V，即 0°C 分压点）
    adc.setRaw(kADCAt0C);

    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed");

    TemperatureData data = sensor.readRaw();

    // 0°C 时 R_ntc ≈ 33630Ω，通过 Steinhart-Hart 反算约 0°C
    // 使用较宽的 epsilon (1.5°C) 因计算过程有累积误差
    TEST_ASSERT_FLOAT_EQ(data.value, 0.0f, 1.5f,
                         "Temperature should be ~0.0 C");
    TEST_ASSERT_EQ(data.sensorId, 1u, "Sensor ID should be 1");
    TEST_ASSERT_EQ(data.type, SensorType::TEMPERATURE,
                   "Data type should be TEMPERATURE");

    return true;
}

// ============================================================
//  生命周期状态机测试
// ============================================================

TEST(NTC, StartStopLifecycle) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    // UNINIT → READY
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "After init: READY");

    // READY → RUNNING
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed when READY");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::RUNNING,
                   "After start: RUNNING");

    // RUNNING → STOPPED
    TEST_ASSERT_TRUE(sensor.stop(), "stop() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::STOPPED,
                   "After stop: STOPPED");

    return true;
}

// ============================================================
//  参数配置测试
// ============================================================

TEST(NTC, SetParams) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    // 构造自定义参数
    NTCThermistorSensor::NTCParams customParams;
    customParams.rFixed        = 4700.0f;
    customParams.r0            = 4700.0f;
    customParams.t0            = 300.0f;
    customParams.bValue        = 4200.0f;
    customParams.vcc           = 5.0f;
    customParams.adcResolution = 10;

    sensor.setParams(customParams);

    const NTCThermistorSensor::NTCParams& params = sensor.getParams();
    TEST_ASSERT_FLOAT_EQ(params.rFixed, 4700.0f, 0.01f,
                         "rFixed should be 4700");
    TEST_ASSERT_FLOAT_EQ(params.r0, 4700.0f, 0.01f,
                         "R0 should be 4700");
    TEST_ASSERT_FLOAT_EQ(params.t0, 300.0f, 0.01f,
                         "T0 should be 300.0K");
    TEST_ASSERT_FLOAT_EQ(params.bValue, 4200.0f, 0.01f,
                         "B value should be 4200");
    TEST_ASSERT_FLOAT_EQ(params.vcc, 5.0f, 0.01f,
                         "VCC should be 5.0V");
    TEST_ASSERT_EQ(params.adcResolution, static_cast<uint8_t>(10),
                   "ADC resolution should be 10");

    return true;
}

// ============================================================
//  复位测试
// ============================================================

TEST(NTC, ResetClearsState) {
    MockADCChannel adc;
    NTCThermistorSensor sensor(1, adc);

    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::RUNNING,
                   "After start: RUNNING");

    // reset() 调用 SensorBase::reset() + setStatus(READY)
    TEST_ASSERT_TRUE(sensor.reset(), "reset() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "After reset: READY");

    return true;
}

// ============================================================
//  主入口
// ============================================================

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
