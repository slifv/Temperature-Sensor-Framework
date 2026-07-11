/**
 * @file test_sensor_pipeline.cpp
 * @brief 传感器全链路黑盒集成测试
 * @details 模拟完整的传感器→滤波→分发数据流，
 *          使用Mock HAL验证端到端正确性。
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../test_framework/mock_hal.hpp"
#include "../../core/sensor_base.hpp"
#include "../../core/sensor_config.hpp"
#include "../../core/event_bus.hpp"
#include "../../filter/mean_filter.hpp"
#include "../../filter/low_pass_filter.hpp"
#include "../../filter/filter_pipeline.hpp"
#include "../../hal/hal_interface.hpp"

// ============================================================
//  完整Pipeline集成测试
// ============================================================

TEST(BlackBox, FullPipelineRawToFiltered) {
    // 构建滤波链: 均值(4点) + 低通(α=0.2)
    MeanFilter<4>* meanFilter = new MeanFilter<4>();
    LowPassFilter* lowpassFilter = new LowPassFilter(0.2f);

    FilterPipeline<4> pipeline;
    pipeline.addFilter(meanFilter);
    pipeline.addFilter(lowpassFilter);

    // 注入已知输入序列: 全部25.0
    for (uint8_t i = 0; i < 8; ++i) {
        float result = pipeline.update(25.0f);
        TEST_ASSERT_TRUE(result > 0.0f, "Pipeline output should be positive");
    }

    // Pipeline稳定后输出应接近25.0
    float output = pipeline.update(25.0f);
    TEST_ASSERT_FLOAT_EQ(output, 25.0f, 1.0f, "Pipeline output should converge to 25.0");

    return true;
}

// ============================================================
//  Mock I2C 集成测试
// ============================================================

TEST(BlackBox, MockI2CWriteAndRead) {
    MockI2CBus mockI2c;

    // 预设响应数据
    uint8_t response[] = {0x66, 0x66, 0xA3};  // 模拟SHT30温度数据+CRC
    mockI2c.setDeviceResponse(0x44, response, 3);

    // 写入命令
    uint8_t cmd[] = {0x2C, 0x06};
    bool writeOk = mockI2c.write(0x44, cmd, 2);
    TEST_ASSERT_TRUE(writeOk, "I2C write should succeed");

    // 读取响应
    uint8_t readBuf[3];
    bool readOk = mockI2c.read(0x44, readBuf, 3);
    TEST_ASSERT_TRUE(readOk, "I2C read should succeed");
    TEST_ASSERT_TRUE(readBuf[0] == 0x66, "First byte should be 0x66");
    TEST_ASSERT_TRUE(readBuf[1] == 0x66, "Second byte should be 0x66");

    return true;
}

TEST(BlackBox, MockI2CNackBehavior) {
    MockI2CBus mockI2c;

    // 设置NACK
    mockI2c.setNackOnAddress(0x44);

    uint8_t data[] = {0x00};
    bool result = mockI2c.write(0x44, data, 1);
    TEST_ASSERT_TRUE(!result, "Write to NACK address should fail");

    // 清除NACK
    mockI2c.clearNackOnAddress(0x44);

    // 再次尝试（需要预设响应）
    uint8_t response[] = {0xAA};
    mockI2c.setDeviceResponse(0x44, response, 1);
    result = mockI2c.write(0x44, data, 1);
    TEST_ASSERT_TRUE(result, "Write after clearing NACK should succeed");

    return true;
}

// ============================================================
//  EventBus + 多订阅者集成测试
// ============================================================

namespace {

class CountingSubscriber : public IDataSubscriber {
public:
    CountingSubscriber(const char* name) : name_(name), count_(0) {}
    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE { ++count_; lastData_ = data; }
    const char* getName() const SENSOR_OVERRIDE { return name_; }
    uint32_t count_;
    SensorData lastData_;
private:
    const char* name_;
};

} // anonymous namespace

TEST(BlackBox, EventBusMultiSubscriberIntegration) {
    EventBus<>& bus = EventBus<>::getInstance();
    CountingSubscriber sub1("GUI"), sub2("MQTT"), sub3("Logic");

    bus.subscribe(0x01, &sub1);
    bus.subscribe(0x01, &sub2);
    bus.subscribe(0x01, &sub3);

    // 模拟10次采样数据发布
    for (uint8_t i = 0; i < 10; ++i) {
        SensorData data;
        data.sensorId = 0x01;
        data.value = 25.0f + static_cast<float>(i) * 0.1f;
        data.timestampMs = static_cast<uint32_t>(i) * 100;
        bus.publish(data);
    }

    TEST_ASSERT_TRUE(sub1.count_ == 10, "Sub1 should receive all 10 updates");
    TEST_ASSERT_TRUE(sub2.count_ == 10, "Sub2 should receive all 10 updates");
    TEST_ASSERT_TRUE(sub3.count_ == 10, "Sub3 should receive all 10 updates");

    bus.unsubscribeAll(0x01);
    return true;
}

// ============================================================
//  多滤波器组合测试
// ============================================================

TEST(BlackBox, FilterCombinations) {
    // 测试多种滤波组合
    LowPassFilter lp1(0.1f);
    LowPassFilter lp2(0.3f);

    // 组合1: 纯低通 α=0.1
    lp1.update(25.0f);
    float result1 = lp1.update(30.0f);
    // y = 0.1*30 + 0.9*25 = 25.5

    // 组合2: 纯低通 α=0.3
    lp2.update(25.0f);
    float result2 = lp2.update(30.0f);
    // y = 0.3*30 + 0.7*25 = 26.5

    TEST_ASSERT_TRUE(result1 < result2,
        "alpha=0.1 should give more smoothing (smaller change) than alpha=0.3");
    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
