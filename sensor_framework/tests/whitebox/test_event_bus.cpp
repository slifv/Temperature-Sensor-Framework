/**
 * @file test_event_bus.cpp
 * @brief EventBus 白盒单元测试
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../core/event_bus.hpp"

// ─── 测试用订阅者 ───────────────────────────────────────

class TestSubscriber : public IDataSubscriber {
public:
    TestSubscriber(const char* name) : name_(name), callCount_(0), lastValue_(0.0f) {}

    void onDataReceived(const SensorData& data) SENSOR_OVERRIDE {
        ++callCount_;
        lastData_ = data;
        lastValue_ = data.value;
    }

    const char* getName() const SENSOR_OVERRIDE { return name_; }

    uint32_t getCallCount() const { return callCount_; }
    float getLastValue() const { return lastValue_; }
    const SensorData& getLastData() const { return lastData_; }

    void reset() { callCount_ = 0; lastValue_ = 0.0f; }

private:
    const char* name_;
    uint32_t    callCount_;
    float       lastValue_;
    SensorData  lastData_;
};

// ============================================================
//  单订阅者测试
// ============================================================

TEST(EventBus, SingleSubscriberReceivesData) {
    EventBus<>& bus = EventBus<>::getInstance();
    TestSubscriber sub("TestSub");

    bus.subscribe(1, &sub);

    SensorData data;
    data.sensorId = 1;
    data.value = 25.5f;
    bus.publish(data);

    TEST_ASSERT_TRUE(sub.getCallCount() == 1, "Subscriber should receive exactly 1 callback");
    TEST_ASSERT_FLOAT_EQ(sub.getLastValue(), 25.5f, 0.01f, "Subscriber should receive value 25.5");

    bus.unsubscribe(1, &sub);
    return true;
}

// ============================================================
//  多订阅者测试
// ============================================================

TEST(EventBus, MultipleSubscribersReceiveData) {
    EventBus<>& bus = EventBus<>::getInstance();
    TestSubscriber sub1("Sub1"), sub2("Sub2"), sub3("Sub3");

    bus.subscribe(100, &sub1);
    bus.subscribe(100, &sub2);
    bus.subscribe(100, &sub3);

    SensorData data;
    data.sensorId = 100;
    data.value = 30.0f;
    bus.publish(data);

    TEST_ASSERT_TRUE(sub1.getCallCount() == 1, "Sub1 should receive data");
    TEST_ASSERT_TRUE(sub2.getCallCount() == 1, "Sub2 should receive data");
    TEST_ASSERT_TRUE(sub3.getCallCount() == 1, "Sub3 should receive data");

    bus.unsubscribeAll(100);
    return true;
}

// ============================================================
//  取消订阅测试
// ============================================================

TEST(EventBus, UnsubscribeStopsDelivery) {
    EventBus<>& bus = EventBus<>::getInstance();
    TestSubscriber sub("UnsubTest");

    bus.subscribe(200, &sub);

    SensorData data;
    data.sensorId = 200;
    data.value = 10.0f;
    bus.publish(data);
    TEST_ASSERT_TRUE(sub.getCallCount() == 1, "Should receive before unsubscribe");

    bus.unsubscribe(200, &sub);

    data.value = 20.0f;
    bus.publish(data);
    TEST_ASSERT_TRUE(sub.getCallCount() == 1, "Should NOT receive after unsubscribe");
    return true;
}

// ============================================================
//  不同传感器隔离测试
// ============================================================

TEST(EventBus, DifferentSensorsIsolated) {
    EventBus<>& bus = EventBus<>::getInstance();
    TestSubscriber sub1("Sensor1_Sub");
    TestSubscriber sub2("Sensor2_Sub");

    bus.subscribe(1, &sub1);
    bus.subscribe(2, &sub2);

    // 发布传感器1的数据
    SensorData data1;
    data1.sensorId = 1;
    data1.value = 40.0f;
    bus.publish(data1);

    TEST_ASSERT_TRUE(sub1.getCallCount() == 1, "Sensor1 subscriber should receive");
    TEST_ASSERT_TRUE(sub2.getCallCount() == 0, "Sensor2 subscriber should NOT receive");

    // 发布传感器2的数据
    SensorData data2;
    data2.sensorId = 2;
    data2.value = 60.0f;
    bus.publish(data2);

    TEST_ASSERT_TRUE(sub2.getCallCount() == 1, "Sensor2 subscriber should now receive");
    TEST_ASSERT_TRUE(sub1.getCallCount() == 1, "Sensor1 subscriber should still be 1");

    bus.unsubscribeAll(1);
    bus.unsubscribeAll(2);
    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
