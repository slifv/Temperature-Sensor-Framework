/**
 * @file test_sensor_manager.cpp
 * @brief SensorManager 白盒单元测试
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../core/sensor_manager.hpp"
#include "../../core/sensor_base.hpp"

// ─── 测试用传感器桩 ─────────────────────────────────────

class StubSensor : public SensorBase<SensorData> {
public:
    StubSensor(uint32_t id, SensorType type = SensorType::TEMPERATURE)
        : SensorBase<SensorData>(id, type)
        , initCalled_(false), startCalled_(false), stopCalled_(false)
    {}

    bool init() SENSOR_OVERRIDE {
        initCalled_ = true;
        setStatus(SensorStatus::READY);
        return true;
    }

    bool start() SENSOR_OVERRIDE {
        startCalled_ = true;
        setStatus(SensorStatus::RUNNING);
        return true;
    }

    bool stop() SENSOR_OVERRIDE {
        stopCalled_ = true;
        setStatus(SensorStatus::STOPPED);
        return true;
    }

    SensorData readRaw() SENSOR_OVERRIDE {
        SensorData data;
        data.sensorId = sensorId_;
        data.value = 25.0f;
        data.status = getStatus();
        return data;
    }

    SensorData readFiltered() SENSOR_OVERRIDE {
        return readRaw();
    }

    const char* getName() const SENSOR_OVERRIDE { return "StubSensor"; }

    bool reset() SENSOR_OVERRIDE { SensorBase::reset(); return true; }

    bool initCalled_;
    bool startCalled_;
    bool stopCalled_;
};

// ============================================================
//  传感器注册/注销测试
// ============================================================

TEST(SensorManager, RegisterSensor) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* sensor = new StubSensor(0x01);
    bool ok = manager.registerSensor(sensor);

    TEST_ASSERT_TRUE(ok, "Register should succeed");
    TEST_ASSERT_TRUE(manager.getSensorCount() == 1, "Should have 1 sensor");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, FindById) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s1 = new StubSensor(0x10);
    StubSensor* s2 = new StubSensor(0x20);

    manager.registerSensor(s1);
    manager.registerSensor(s2);

    SensorBase<SensorData>* found = manager.findSensorById(0x20);
    TEST_ASSERT_TRUE(found != NULL, "Should find sensor 0x20");
    TEST_ASSERT_TRUE(found->getSensorId() == 0x20, "Found sensor should have ID 0x20");

    found = manager.findSensorById(0x99);
    TEST_ASSERT_TRUE(found == NULL, "Should NOT find non-existent sensor");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, DuplicateIdRejected) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s1 = new StubSensor(0x01);
    StubSensor* s2 = new StubSensor(0x01); // 同ID

    bool ok1 = manager.registerSensor(s1);
    TEST_ASSERT_TRUE(ok1, "First register should succeed");

    bool ok2 = manager.registerSensor(s2);
    TEST_ASSERT_TRUE(!ok2, "Duplicate ID register should fail");

    delete s2;  // 未被管理器接管的需要手动删除
    manager.destroyAll();
    return true;
}

// ============================================================
//  生命周期管理测试
// ============================================================

TEST(SensorManager, BatchInit) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s1 = new StubSensor(1);
    StubSensor* s2 = new StubSensor(2);

    manager.registerSensor(s1);
    manager.registerSensor(s2);

    uint8_t count = manager.initAll();
    TEST_ASSERT_TRUE(count == 2, "Both sensors should init");
    TEST_ASSERT_TRUE(s1->initCalled_ && s2->initCalled_, "init() should be called");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, BatchStart) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s = new StubSensor(1);
    manager.registerSensor(s);
    manager.initAll();

    uint8_t count = manager.startAll();
    TEST_ASSERT_TRUE(count == 1, "Sensor should start");
    TEST_ASSERT_TRUE(s->getStatus() == SensorStatus::RUNNING, "Status should be RUNNING");

    manager.destroyAll();
    return true;
}

// ============================================================
//  按类型查找测试
// ============================================================

TEST(SensorManager, FindByType) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* tempSensor = new StubSensor(1, SensorType::TEMPERATURE);
    StubSensor* humiSensor = new StubSensor(2, SensorType::HUMIDITY);

    manager.registerSensor(tempSensor);
    manager.registerSensor(humiSensor);

    SensorBase<SensorData>* found = manager.findSensorByType(SensorType::HUMIDITY);
    TEST_ASSERT_TRUE(found != NULL, "Should find humidity sensor");
    TEST_ASSERT_TRUE(found->getSensorId() == 2, "Found sensor should be ID 2");

    manager.destroyAll();
    return true;
}

// ============================================================
//  边界条件 / 错误路径测试
// ============================================================

TEST(SensorManager, NullSensorRejected) {
    auto& manager = SensorManager<>::getInstance();
    bool ok = manager.registerSensor(NULL);
    TEST_ASSERT_TRUE(!ok, "NULL sensor should be rejected");
    manager.destroyAll();
    return true;
}

TEST(SensorManager, UnregisterSensor) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s = new StubSensor(0x30);
    manager.registerSensor(s);
    TEST_ASSERT_TRUE(manager.getSensorCount() == 1, "Should have 1 sensor");

    bool removed = manager.unregisterSensor(0x30);
    TEST_ASSERT_TRUE(removed, "Unregister should succeed");
    TEST_ASSERT_TRUE(manager.getSensorCount() == 0, "Should have 0 sensors after unregister");
    return true;
}

TEST(SensorManager, StopAll) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s1 = new StubSensor(1);
    StubSensor* s2 = new StubSensor(2);
    manager.registerSensor(s1);
    manager.registerSensor(s2);
    manager.initAll();
    manager.startAll();

    manager.stopAll();
    TEST_ASSERT_TRUE(s1->getStatus() == SensorStatus::STOPPED, "Sensor1 should be STOPPED");
    TEST_ASSERT_TRUE(s2->getStatus() == SensorStatus::STOPPED, "Sensor2 should be STOPPED");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, GetSensorAtOutOfBounds) {
    auto& manager = SensorManager<>::getInstance();

    SensorBase<SensorData>* found = manager.getSensorAt(0);
    TEST_ASSERT_TRUE(found == NULL, "getSensorAt on empty manager should return NULL");

    found = manager.getSensorAt(255);
    TEST_ASSERT_TRUE(found == NULL, "getSensorAt 255 should return NULL");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, GetCountByStatus) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s = new StubSensor(1);
    manager.registerSensor(s);

    uint8_t uninit = manager.getCountByStatus(SensorStatus::UNINIT);
    TEST_ASSERT_TRUE(uninit == 1, "Should have 1 UNINIT sensor");

    manager.initAll();
    uint8_t ready = manager.getCountByStatus(SensorStatus::READY);
    TEST_ASSERT_TRUE(ready == 1, "Should have 1 READY sensor after init");

    manager.startAll();
    uint8_t running = manager.getCountByStatus(SensorStatus::RUNNING);
    TEST_ASSERT_TRUE(running == 1, "Should have 1 RUNNING sensor after start");

    manager.destroyAll();
    return true;
}

// 全局变量用于 sampleAll 回调测试（C++11 lambda 无法转为函数指针）
static uint8_t g_sampleCallCount = 0;
static void sampleCallback(const SensorData& data) {
    ++g_sampleCallCount;
    (void)data;
}

TEST(SensorManager, SampleAllCallback) {
    auto& manager = SensorManager<>::getInstance();

    StubSensor* s = new StubSensor(1);
    manager.registerSensor(s);
    manager.initAll();
    manager.startAll();

    g_sampleCallCount = 0;
    manager.sampleAll(sampleCallback);

    TEST_ASSERT_TRUE(g_sampleCallCount == 1, "sampleAll should invoke callback once for running sensor");

    manager.destroyAll();
    return true;
}

TEST(SensorManager, EmptyManagerOperations) {
    auto& manager = SensorManager<>::getInstance();

    TEST_ASSERT_TRUE(manager.getSensorCount() == 0, "Empty manager: count=0");
    TEST_ASSERT_TRUE(manager.findSensorById(1) == NULL, "Empty manager: findById returns NULL");
    TEST_ASSERT_TRUE(manager.findSensorByType(SensorType::TEMPERATURE) == NULL, "Empty manager: findByType returns NULL");
    TEST_ASSERT_TRUE(manager.initAll() == 0, "Empty manager: initAll returns 0");
    TEST_ASSERT_TRUE(manager.startAll() == 0, "Empty manager: startAll returns 0");
    // stopAll + sampleAll on empty manager should not crash
    manager.stopAll();
    manager.sampleAll(NULL);

    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
