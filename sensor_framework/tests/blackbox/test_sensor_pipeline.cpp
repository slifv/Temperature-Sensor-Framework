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
#include "../../core/sensor_manager.hpp"
#include "../../filter/kalman_filter.hpp"
#include "../../filter/median_filter.hpp"

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

// ============================================================
//  辅助类: 用于集成测试的最小传感器实现
// ============================================================

namespace {

class MinimalSensor : public SensorBase<SensorData> {
public:
    MinimalSensor(uint32_t id) : SensorBase<SensorData>(id, SensorType::TEMPERATURE) {}

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

    SensorData readRaw() SENSOR_OVERRIDE {
        SensorData d;
        d.sensorId = sensorId_;
        d.value = 25.0f;
        d.status = getStatus();
        return d;
    }

    const char* getName() const SENSOR_OVERRIDE { return "Minimal"; }
};

/// sampleAll回调 — 记录每个传感器是否被采样
struct SampleTracker {
    bool sampled1;
    bool sampled2;
    bool sampled3;

    SampleTracker() : sampled1(false), sampled2(false), sampled3(false) {}

    static void onSample(const SensorData& data) {
        // 使用文件静态变量中转（回调签名限制为无上下文指针）
        if (data.sensorId == 0x01) instance().sampled1 = true;
        if (data.sensorId == 0x02) instance().sampled2 = true;
        if (data.sensorId == 0x03) instance().sampled3 = true;
    }

    static SampleTracker& instance() {
        static SampleTracker tracker;
        return tracker;
    }

    void reset() { sampled1 = sampled2 = sampled3 = false; }
};

} // anonymous namespace

// ============================================================
//  多传感器并发采样测试
// ============================================================

TEST(BlackBox, MultiSensorConcurrency) {
    SensorManager<>& mgr = SensorManager<>::getInstance();

    MinimalSensor* s1 = new MinimalSensor(0x01);
    MinimalSensor* s2 = new MinimalSensor(0x02);
    MinimalSensor* s3 = new MinimalSensor(0x03);

    TEST_ASSERT_TRUE(mgr.registerSensor(s1), "Register sensor 0x01");
    TEST_ASSERT_TRUE(mgr.registerSensor(s2), "Register sensor 0x02");
    TEST_ASSERT_TRUE(mgr.registerSensor(s3), "Register sensor 0x03");
    TEST_ASSERT_TRUE(mgr.getSensorCount() == 3, "Should have 3 registered sensors");

    // 批量初始化
    uint8_t initCount = mgr.initAll();
    TEST_ASSERT_TRUE(initCount == 3, "All 3 sensors should init successfully");

    // 批量启动
    uint8_t startCount = mgr.startAll();
    TEST_ASSERT_TRUE(startCount == 3, "All 3 sensors should start successfully");

    // 批量采样（带回调验证）
    SampleTracker::instance().reset();
    mgr.sampleAll(SampleTracker::onSample);

    TEST_ASSERT_TRUE(SampleTracker::instance().sampled1, "Sensor 0x01 should be sampled");
    TEST_ASSERT_TRUE(SampleTracker::instance().sampled2, "Sensor 0x02 should be sampled");
    TEST_ASSERT_TRUE(SampleTracker::instance().sampled3, "Sensor 0x03 should be sampled");

    // 批量销毁
    mgr.destroyAll();
    TEST_ASSERT_TRUE(mgr.getSensorCount() == 0, "All sensors should be destroyed");

    return true;
}

// ============================================================
//  动态滤波器切换测试
// ============================================================

TEST(BlackBox, DynamicFilterSwitch) {
    MinimalSensor* sensor = new MinimalSensor(0x10);
    sensor->init();
    sensor->start();

    // 添加均值滤波器
    bool addedMean = sensor->addFilter(new MeanFilter<4>());
    TEST_ASSERT_TRUE(addedMean, "Add MeanFilter should succeed");
    TEST_ASSERT_TRUE(sensor->getFilterPipeline() != NULL, "Pipeline should exist after first add");

    // 采样若干次让滤波器稳定
    for (uint8_t i = 0; i < 5; ++i) {
        SensorData d = sensor->readFiltered();
        TEST_ASSERT_TRUE(d.value > 0.0f, "Filtered value should be positive");
    }

    // 动态增加卡尔曼滤波器
    bool addedKalman = sensor->addFilter(new KalmanFilter(0.01f, 0.5f));
    TEST_ASSERT_TRUE(addedKalman, "Add KalmanFilter should succeed");
    TEST_ASSERT_TRUE(sensor->getFilterPipeline()->getFilterCount() == 2,
        "Pipeline should have 2 filters after adding Kalman");

    // 移除均值滤波器
    bool removed = sensor->getFilterPipeline()->removeFilter("Mean");
    TEST_ASSERT_TRUE(removed, "Remove MeanFilter should succeed");
    TEST_ASSERT_TRUE(sensor->getFilterPipeline()->getFilterCount() == 1,
        "Pipeline should have 1 filter after removing Mean");

    // 清理
    sensor->getFilterPipeline()->clear();
    delete sensor;

    return true;
}

// ============================================================
//  全零输入极端场景测试
// ============================================================

TEST(BlackBox, AllZeroInput) {
    MeanFilter<4>* meanFilter = new MeanFilter<4>();
    LowPassFilter* lowpassFilter = new LowPassFilter(0.2f);

    FilterPipeline<4> pipeline;
    pipeline.addFilter(meanFilter);
    pipeline.addFilter(lowpassFilter);

    // 连续输入全零，验证输出保持为零
    for (uint8_t i = 0; i < 20; ++i) {
        float result = pipeline.update(0.0f);
        TEST_ASSERT_FLOAT_EQ(result, 0.0f, 0.001f,
            "All-zero input should produce zero output");
    }

    return true;
}

// ============================================================
//  故障恢复测试
// ============================================================

TEST(BlackBox, FaultRecovery) {
    MockI2CBus mockI2c;

    // Phase 1: 注入NACK故障
    mockI2c.setNackOnAddress(0x44);

    uint8_t cmd[] = {0x2C, 0x06};
    uint8_t readBuf[3];
    bool result = mockI2c.writeRead(0x44, cmd, 2, readBuf, 3);
    TEST_ASSERT_FALSE(result, "Phase 1: writeRead with NACK should fail");

    // Phase 2: 恢复 — 清除故障，预设响应
    mockI2c.clearNackOnAddress(0x44);
    uint8_t response[] = {0x66, 0x66, 0xA3};
    mockI2c.setDeviceResponse(0x44, response, 3);

    // Phase 3: 验证恢复成功
    result = mockI2c.writeRead(0x44, cmd, 2, readBuf, 3);
    TEST_ASSERT_TRUE(result, "Phase 3: writeRead after recovery should succeed");
    TEST_ASSERT_TRUE(readBuf[0] == 0x66, "Phase 3: first byte should match expected after recovery");
    TEST_ASSERT_TRUE(readBuf[1] == 0x66, "Phase 3: second byte should match expected after recovery");

    return true;
}

// ============================================================
//  多滤波链对比测试
// ============================================================

TEST(BlackBox, FilterChainComparison) {
    // 滤波链1: 中值(窗口5) + 低通(α=0.3)
    MedianFilter<5>* medianFilter = new MedianFilter<5>();
    LowPassFilter* lpFilter = new LowPassFilter(0.3f);
    FilterPipeline<4> pipeline1;
    pipeline1.addFilter(medianFilter);
    pipeline1.addFilter(lpFilter);

    // 滤波链2: 均值(窗口4) + 卡尔曼(Q=0.01, R=0.5)
    MeanFilter<4>* meanFilter = new MeanFilter<4>();
    KalmanFilter* kalmanFilter = new KalmanFilter(0.01f, 0.5f);
    FilterPipeline<4> pipeline2;
    pipeline2.addFilter(meanFilter);
    pipeline2.addFilter(kalmanFilter);

    // 同一含噪声的输入序列（25.0基准 + 脉冲噪点）
    float inputs[] = {25.0f, 25.0f, 30.0f, 25.0f, 25.0f, 35.0f, 25.0f, 25.0f};

    float out1 = 0.0f;
    float out2 = 0.0f;
    for (uint8_t i = 0; i < 8; ++i) {
        out1 = pipeline1.update(inputs[i]);
        out2 = pipeline2.update(inputs[i]);
    }

    // 两条链的输出都应接近基准值25.0
    TEST_ASSERT_FLOAT_EQ(out1, 25.0f, 5.0f,
        "Chain1 (Median+LowPass) output should be near baseline 25.0");
    TEST_ASSERT_FLOAT_EQ(out2, 25.0f, 5.0f,
        "Chain2 (Mean+Kalman) output should be near baseline 25.0");

    // 两条链应产生不同的输出
    float diff = (out1 > out2) ? (out1 - out2) : (out2 - out1);
    TEST_ASSERT_TRUE(diff > 0.0f,
        "Different filter chains should produce different outputs");

    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
