/**
 * @file test_sht30.cpp
 * @brief SHT30 温度传感器 白盒单元测试
 * @details 使用 MockI2CBus 预设I2C总线行为，覆盖构造、初始化、
 *          数据读取、生命周期状态机和备用地址等场景。
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../test_framework/mock_hal.hpp"
#include "../../sensors/temperature/sht30.hpp"

// ============================================================
//  CRC-8 辅助函数 (SHT30多项式: x^8 + x^5 + x^4 + 1)
// ============================================================

static uint8_t sht30CRC8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x80) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x31);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

// ============================================================
//  预计算的测试数据
// ============================================================

/// 状态寄存器响应: {0x00, 0x00} + CRC-8 = 0x81
static const uint8_t kStatusResponse[] = {0x00, 0x00, 0x81};

/// 温湿度响应: temp ~25.0 degC (raw=0x6666, CRC=0x93), humi ~50% (raw=0x7FFF, CRC=0x8F)
static const uint8_t kTempHumiResponse[] = {0x66, 0x66, 0x93, 0x7F, 0xFF, 0x8F};

// ============================================================
//  构造函数测试
// ============================================================

TEST(SHT30, ConstructionDefaults) {
    MockI2CBus i2c;
    SHT30Sensor sensor(1, i2c);

    TEST_ASSERT_EQ(sensor.getSensorId(), 1u, "Sensor ID should be 1");
    TEST_ASSERT_EQ(sensor.getType(), SensorType::TEMPERATURE, "Type should be TEMPERATURE");
    TEST_ASSERT_TRUE(
        (strcmp(sensor.getName(), "SHT30") == 0),
        "Name should be SHT30");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::UNINIT, "Status should be UNINIT");
    return true;
}

// ============================================================
//  初始化测试
// ============================================================

TEST(SHT30, InitProbesDevice) {
    MockI2CBus i2c;

    // init() 内部流程:
    //   probeDevice() -> readStatus: write(0xF3,0x2D) + read(3 bytes)
    //   softReset():   write(0x30,0xA2)
    //   readStatus():  write(0xF3,0x2D) + read(3 bytes)
    // 共需 2 次 read 响应
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);

    SHT30Sensor sensor(1, i2c);
    bool ok = sensor.init();

    TEST_ASSERT_TRUE(ok, "init() should succeed with valid I2C responses");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "Status should be READY after init");
    TEST_ASSERT_EQ(sensor.getErrorCount(), 0u,
                   "Error count should be 0 after successful init");

    // 验证 I2C 写/读调用次数
    TEST_ASSERT_TRUE(i2c.getWriteCount() >= 3,
                     "At least 3 writes during init (2 status probes + 1 soft reset)");
    TEST_ASSERT_EQ(i2c.getReadCount(), 2u,
                   "2 reads during init (2 status reads)");

    return true;
}

TEST(SHT30, InitFailsOnNack) {
    MockI2CBus i2c;
    i2c.setNackOnAddress(0x44);

    SHT30Sensor sensor(1, i2c);
    bool ok = sensor.init();

    TEST_ASSERT_FALSE(ok, "init() should fail when device NACKs");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::COMM_ERROR,
                   "Status should be COMM_ERROR after NACK");
    return true;
}

// ============================================================
//  数据读取测试
// ============================================================

TEST(SHT30, ReadRawTemperature) {
    MockI2CBus i2c;

    // init() 需要 2 次 read 响应
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    // readRaw() 需要 1 次 read 响应 (6 bytes 温湿度数据)
    i2c.setDeviceResponse(0x44, kTempHumiResponse, 6);

    SHT30Sensor sensor(1, i2c);
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");

    TemperatureData data = sensor.readRaw();

    // T[C] = -45 + 175 * 26214 / 65535 = 25.0 degC
    float expectedTemp = -45.0f + 175.0f * 26214.0f / 65535.0f;
    TEST_ASSERT_FLOAT_EQ(data.value, expectedTemp, 0.1f,
                         "Temperature should be ~25.0 C");
    TEST_ASSERT_EQ(data.sensorId, 1u, "Sensor ID should be 1");
    TEST_ASSERT_EQ(data.type, SensorType::TEMPERATURE,
                   "Data type should be TEMPERATURE");
    TEST_ASSERT_EQ(data.status, SensorStatus::RUNNING,
                   "Status after successful readRaw should be RUNNING");

    // 验证原始 ADC 访问器
    TEST_ASSERT_EQ(sensor.getRawTempADC(), static_cast<uint16_t>(0x6666),
                   "Raw temp ADC should be 0x6666");
    TEST_ASSERT_EQ(sensor.getRawHumiADC(), static_cast<uint16_t>(0x7FFF),
                   "Raw humi ADC should be 0x7FFF");

    return true;
}

TEST(SHT30, MultipleReads) {
    MockI2CBus i2c;

    // init(): 2 次 read
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    // 两次 readRaw(): 2 次 read
    i2c.setDeviceResponse(0x44, kTempHumiResponse, 6);
    i2c.setDeviceResponse(0x44, kTempHumiResponse, 6);

    SHT30Sensor sensor(1, i2c);
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");

    TemperatureData data1 = sensor.readRaw();
    TemperatureData data2 = sensor.readRaw();

    float expectedTemp = -45.0f + 175.0f * 26214.0f / 65535.0f;
    TEST_ASSERT_FLOAT_EQ(data1.value, expectedTemp, 0.1f,
                         "First read should be ~25.0 C");
    TEST_ASSERT_FLOAT_EQ(data2.value, expectedTemp, 0.1f,
                         "Second read should be ~25.0 C");

    // 两次读取应返回相同值（注入了相同数据）
    TEST_ASSERT_FLOAT_EQ(data1.value, data2.value, 0.001f,
                         "Two reads should return identical values");

    return true;
}

// ============================================================
//  生命周期状态机测试
// ============================================================

TEST(SHT30, StartStopLifecycle) {
    MockI2CBus i2c;

    // 为 init() 预备
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);

    SHT30Sensor sensor(1, i2c);
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "After init: READY");

    // start(): READY -> RUNNING
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed when READY");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::RUNNING,
                   "After start: RUNNING");

    // stop(): RUNNING -> STOPPED
    TEST_ASSERT_TRUE(sensor.stop(), "stop() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::STOPPED,
                   "After stop: STOPPED");

    return true;
}

TEST(SHT30, ResetClearsState) {
    MockI2CBus i2c;

    // 为 init() 预备
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);
    i2c.setDeviceResponse(0x44, kStatusResponse, 3);

    SHT30Sensor sensor(1, i2c);
    TEST_ASSERT_TRUE(sensor.init(), "init() should succeed");
    TEST_ASSERT_TRUE(sensor.start(), "start() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::RUNNING,
                   "After start: RUNNING");

    // reset(): 发送 softReset + SensorBase::reset() + setStatus(READY)
    TEST_ASSERT_TRUE(sensor.reset(), "reset() should succeed");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "After reset: READY");

    return true;
}

// ============================================================
//  备用地址测试
// ============================================================

TEST(SHT30, AlternateAddress) {
    MockI2CBus i2c;

    // 在默认地址 0x44 设置 NACK，验证 SHT30 使用 0x45 不受影响
    i2c.setNackOnAddress(0x44);

    // 为 init() 在地址 0x45 上预备响应
    i2c.setDeviceResponse(0x45, kStatusResponse, 3);
    i2c.setDeviceResponse(0x45, kStatusResponse, 3);

    SHT30Sensor sensor(2, i2c, SHT30Sensor::ALT_ADDR);
    TEST_ASSERT_EQ(sensor.getSensorId(), 2u, "Sensor ID should be 2");

    // 因为 0x45 未被 NACK，init() 应成功
    bool ok = sensor.init();
    TEST_ASSERT_TRUE(ok, "init() should succeed on alternate address 0x45");
    TEST_ASSERT_EQ(sensor.getStatus(), SensorStatus::READY,
                   "Status should be READY after init on alt addr");

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
