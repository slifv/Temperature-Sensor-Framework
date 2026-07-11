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

    void reset() SENSOR_OVERRIDE { SensorBase::reset(); }

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

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
