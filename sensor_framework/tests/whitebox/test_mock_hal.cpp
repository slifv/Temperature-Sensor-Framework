/**
 * @file test_mock_hal.cpp
 * @brief Mock HAL 层白盒单元测试
 * @details 测试所有 Mock HAL 实现的行为：
 *          MockI2CBus、MockTimeProvider、MockADCChannel
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../test_framework/mock_hal.hpp"

// ============================================================
//  MockI2CBus 测试
// ============================================================

TEST(MockI2C, WriteSucceeds) {
    MockI2CBus i2c;

    uint8_t data[] = {0x01, 0x02, 0x03};
    bool result = i2c.write(0x48, data, 3);

    TEST_ASSERT_TRUE(result, "Write to any address should return true");
    return true;
}

TEST(MockI2C, ReadWithPresetData) {
    MockI2CBus i2c;

    uint8_t preset[] = {0xAA, 0xBB, 0xCC};
    i2c.setDeviceResponse(0x48, preset, 3);

    uint8_t buffer[3] = {0};
    bool result = i2c.read(0x48, buffer, 3);

    TEST_ASSERT_TRUE(result, "Read with preset data should return true");
    TEST_ASSERT_EQ(buffer[0], 0xAA, "buffer[0] should match preset[0]");
    TEST_ASSERT_EQ(buffer[1], 0xBB, "buffer[1] should match preset[1]");
    TEST_ASSERT_EQ(buffer[2], 0xCC, "buffer[2] should match preset[2]");
    return true;
}

TEST(MockI2C, NackReturnsFalse) {
    MockI2CBus i2c;

    i2c.setNackOnAddress(0x50);
    uint8_t data[] = {0x01};
    bool result = i2c.write(0x50, data, 1);

    TEST_ASSERT_FALSE(result, "Write to NACK address should return false");
    return true;
}

TEST(MockI2C, ClearNackRestores) {
    MockI2CBus i2c;

    i2c.setNackOnAddress(0x50);
    i2c.clearNackOnAddress(0x50);

    uint8_t data[] = {0x01};
    bool result = i2c.write(0x50, data, 1);

    TEST_ASSERT_TRUE(result, "Write should succeed after clearing NACK");
    return true;
}

TEST(MockI2C, WriteRecordHistory) {
    MockI2CBus i2c;

    uint8_t data[] = {0x10, 0x20, 0x30};
    i2c.write(0x48, data, 3);

    TEST_ASSERT_EQ(i2c.getWriteCount(), static_cast<uint8_t>(1),
                   "Write count should be 1 after single write");

    const MockI2CBus::WriteRecord& record = i2c.getWriteRecord(0);
    TEST_ASSERT_EQ(record.addr, static_cast<uint8_t>(0x48),
                   "Recorded address should be 0x48");
    TEST_ASSERT_EQ(record.len, static_cast<size_t>(3),
                   "Recorded length should be 3");
    TEST_ASSERT_EQ(record.data[0], 0x10, "record.data[0] should be 0x10");
    TEST_ASSERT_EQ(record.data[1], 0x20, "record.data[1] should be 0x20");
    TEST_ASSERT_EQ(record.data[2], 0x30, "record.data[2] should be 0x30");
    return true;
}

TEST(MockI2C, MultipleWrites) {
    MockI2CBus i2c;

    uint8_t data[] = {0x01};
    i2c.write(0x48, data, 1);
    i2c.write(0x49, data, 1);
    i2c.write(0x4A, data, 1);

    TEST_ASSERT_EQ(i2c.getWriteCount(), static_cast<uint8_t>(3),
                   "Write count should be 3 after 3 writes");
    return true;
}

TEST(MockI2C, ReadWithoutPresetFails) {
    MockI2CBus i2c;

    uint8_t buffer[4] = {0};
    bool result = i2c.read(0x48, buffer, 4);

    TEST_ASSERT_FALSE(result,
                      "Read without setDeviceResponse should return false");
    return true;
}

TEST(MockI2C, ClearAllResets) {
    MockI2CBus i2c;

    uint8_t preset[] = {0x01, 0x02};
    i2c.setDeviceResponse(0x48, preset, 2);

    uint8_t wdata[] = {0x10};
    i2c.write(0x48, wdata, 1);
    i2c.write(0x49, wdata, 1);

    i2c.clearAll();

    TEST_ASSERT_EQ(i2c.getWriteCount(), static_cast<uint8_t>(0),
                   "Write count should be 0 after clearAll");
    TEST_ASSERT_EQ(i2c.getReadCount(), static_cast<uint8_t>(0),
                   "Read count should be 0 after clearAll");
    return true;
}

TEST(MockI2C, WriteReadCombined) {
    MockI2CBus i2c;

    uint8_t preset[] = {0xBE, 0xEF};
    i2c.setDeviceResponse(0x48, preset, 2);

    uint8_t tx[] = {0x00};  // register address
    uint8_t rx[2] = {0};
    bool result = i2c.writeRead(0x48, tx, 1, rx, 2);

    TEST_ASSERT_TRUE(result, "writeRead should return true with preset response");
    TEST_ASSERT_EQ(rx[0], 0xBE, "rx[0] should be 0xBE");
    TEST_ASSERT_EQ(rx[1], 0xEF, "rx[1] should be 0xEF");
    return true;
}

// ============================================================
//  MockTimeProvider 测试
// ============================================================

TEST(MockTime, InitialZero) {
    MockTimeProvider time;

    TEST_ASSERT_EQ(time.getTickMs(), static_cast<uint32_t>(0),
                   "getTickMs should be 0 after construction");
    TEST_ASSERT_EQ(time.getTickUs(), static_cast<uint64_t>(0),
                   "getTickUs should be 0 after construction");
    return true;
}

TEST(MockTime, AdvanceTime) {
    MockTimeProvider time;

    time.advance(100);

    TEST_ASSERT_EQ(time.getTickMs(), static_cast<uint32_t>(100),
                   "getTickMs should be 100 after advance(100)");
    return true;
}

TEST(MockTime, SetTick) {
    MockTimeProvider time;

    time.setTick(500);
    time.advance(100);

    TEST_ASSERT_EQ(time.getTickMs(), static_cast<uint32_t>(600),
                   "getTickMs should be 600 after setTick(500) + advance(100)");
    return true;
}

TEST(MockTime, DelayMsAdvances) {
    MockTimeProvider time;

    time.delayMs(50);

    TEST_ASSERT_EQ(time.getTickMs(), static_cast<uint32_t>(50),
                   "getTickMs should be 50 after delayMs(50)");
    return true;
}

TEST(MockTime, TickUsConversion) {
    MockTimeProvider time;

    time.advance(1);  // 1ms

    TEST_ASSERT_EQ(time.getTickUs(), static_cast<uint64_t>(1000),
                   "getTickUs should be 1000 after advance(1) ms");
    return true;
}

// ============================================================
//  MockADCChannel 测试
// ============================================================

#if SENSOR_FEATURE_HAL_ADC

TEST(MockADC, SetRaw) {
    MockADCChannel adc;

    adc.setRaw(2048);

    TEST_ASSERT_EQ(adc.readRaw(), static_cast<uint16_t>(2048),
                   "readRaw should return 2048 after setRaw(2048)");
    return true;
}

TEST(MockADC, SetVoltage) {
    MockADCChannel adc;

    adc.setVoltage(1.65f);

    float voltage = adc.readVoltage();
    TEST_ASSERT_FLOAT_EQ(voltage, 1.65f, 0.01f,
                         "readVoltage should be approximately 1.65 after setVoltage(1.65)");
    return true;
}

TEST(MockADC, ReferenceVoltage) {
    MockADCChannel adc;

    float ref = adc.getReferenceVoltage();

    TEST_ASSERT_FLOAT_EQ(ref, 3.3f, 0.01f,
                         "getReferenceVoltage should return default 3.3");
    return true;
}

TEST(MockADC, Resolution) {
    MockADCChannel adc;

    uint8_t res = adc.getResolution();

    TEST_ASSERT_EQ(res, static_cast<uint8_t>(12),
                   "getResolution should return default 12-bit");
    return true;
}

#endif // SENSOR_FEATURE_HAL_ADC

// ============================================================
//  测试入口
// ============================================================

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
