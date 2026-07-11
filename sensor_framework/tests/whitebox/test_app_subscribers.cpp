/**
 * @file test_app_subscribers.cpp
 * @brief 应用层订阅者白盒单元测试 — 告警、安全联动、PID、GUI显示、MQTT
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../app/business_logic.hpp"
#include "../../app/gui_display.hpp"
#include "../../app/mqtt_sender.hpp"

// ============================================================
//  测试辅助：构建 SensorData
// ============================================================

static SensorData makeData(uint32_t id, float value) {
    SensorData d;
    d.sensorId = id;
    d.value = value;
    d.type = SensorType::TEMPERATURE;
    return d;
}

// ============================================================
//  全局回调追踪变量
// ============================================================

// ─── Alarm 回调追踪 ───
static uint32_t g_alarmSensorId = 0;
static float    g_alarmValue = 0.0f;
static float    g_alarmThreshold = 0.0f;
static bool     g_alarmIsHigh = false;
static int      g_alarmCallbackCount = 0;

static void alarmCallback(uint32_t id, float value, float threshold, bool isHigh) {
    g_alarmSensorId = id;
    g_alarmValue = value;
    g_alarmThreshold = threshold;
    g_alarmIsHigh = isHigh;
    ++g_alarmCallbackCount;
}

// ─── Safety 回调追踪 ───
static bool     g_safetyActive = false;
static uint32_t g_safetySensorId = 0;

static void safetyCallback(uint32_t id, bool activate) {
    g_safetyActive = activate;
    g_safetySensorId = id;
}

// ─── GuiDisplay 回调追踪 ───
static int g_displayCallCount = 0;

static void displayFunc(const char* label, float value, const char* unit) {
    ++g_displayCallCount;
    (void)label;
    (void)value;
    (void)unit;
}

// ─── MQTT 回调追踪 ───
static int  g_mqttCallCount = 0;
static bool g_mqttReturnValue = true;

static bool mqttPublishFunc(const char* topic, const char* payload, uint16_t len) {
    ++g_mqttCallCount;
    (void)topic;
    (void)payload;
    (void)len;
    return g_mqttReturnValue;
}

// ============================================================
//  全局变量重置
// ============================================================

static void resetGlobals() {
    g_alarmCallbackCount = 0;
    g_alarmSensorId = 0;
    g_alarmValue = 0.0f;
    g_alarmThreshold = 0.0f;
    g_alarmIsHigh = false;

    g_safetyActive = false;
    g_safetySensorId = 0;

    g_displayCallCount = 0;

    g_mqttCallCount = 0;
    g_mqttReturnValue = true;
}

// ============================================================
//  AlarmSubscriber 测试
// ============================================================

TEST(AlarmSubscriber, NoAlarmInRange) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    SensorData data = makeData(1, 25.0f);
    alarm.onDataReceived(data);

    TEST_ASSERT_FALSE(alarm.isAlarmActive(), "Alarm should NOT be active for value 25.0 in [15, 35]");
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 0u, "Alarm count should be 0");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 0, "Callback should NOT have been called");
    return true;
}

TEST(AlarmSubscriber, HighAlarmTriggered) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    SensorData data = makeData(1, 40.0f);
    alarm.onDataReceived(data);

    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Alarm should be active for value 40.0 > 35");
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 1u, "Alarm count should be 1");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 1, "Callback should have been called once");
    TEST_ASSERT_EQ(g_alarmSensorId, 1u, "Callback sensorId should be 1");
    TEST_ASSERT_FLOAT_EQ(g_alarmValue, 40.0f, 0.01f, "Callback value should be 40.0");
    TEST_ASSERT_FLOAT_EQ(g_alarmThreshold, 35.0f, 0.01f, "Callback threshold should be 35.0 (high)");
    TEST_ASSERT_TRUE(g_alarmIsHigh, "Callback isHigh should be true");
    return true;
}

TEST(AlarmSubscriber, LowAlarmTriggered) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    SensorData data = makeData(1, 10.0f);
    alarm.onDataReceived(data);

    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Alarm should be active for value 10.0 < 15");
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 1u, "Alarm count should be 1");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 1, "Callback should have been called once");
    TEST_ASSERT_FLOAT_EQ(g_alarmThreshold, 15.0f, 0.01f, "Callback threshold should be 15.0 (low)");
    TEST_ASSERT_FALSE(g_alarmIsHigh, "Callback isHigh should be false for low alarm");
    return true;
}

TEST(AlarmSubscriber, AlarmCountIncrements) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    // 第一次高告警
    SensorData data1 = makeData(1, 40.0f);
    alarm.onDataReceived(data1);
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 1u, "Alarm count should be 1 after first high alarm");

    // 回到范围内 → 解除
    SensorData data2 = makeData(1, 25.0f);
    alarm.onDataReceived(data2);
    TEST_ASSERT_FALSE(alarm.isAlarmActive(), "Alarm should be cleared");
    // alarmCount 不增加（解除不算告警）
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 1u, "Alarm count should remain 1 (clear doesn't count)");

    // 第二次高告警
    SensorData data3 = makeData(1, 38.0f);
    alarm.onDataReceived(data3);
    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Alarm should be active again");
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 2u, "Alarm count should be 2 after second high alarm");
    return true;
}

TEST(AlarmSubscriber, AlarmClearsWhenBackInRange) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    // 触发高告警
    SensorData data1 = makeData(1, 40.0f);
    alarm.onDataReceived(data1);
    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Alarm should be active");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 1, "Callback should be called once for alarm");

    // 回到正常范围 — 重置全局变量以单独验证清除回调参数
    resetGlobals();
    SensorData data2 = makeData(1, 25.0f);
    alarm.onDataReceived(data2);
    TEST_ASSERT_FALSE(alarm.isAlarmActive(), "Alarm should NOT be active after returning to range");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 1, "Callback should be called once for clear");
    TEST_ASSERT_FLOAT_EQ(g_alarmThreshold, 0.0f, 0.01f, "Clear callback threshold should be 0.0");
    TEST_ASSERT_FALSE(g_alarmIsHigh, "Clear callback isHigh should be false");
    return true;
}

TEST(AlarmSubscriber, SetThresholds) {
    resetGlobals();
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, alarmCallback);

    // 新阈值：高=30, 低=20
    alarm.setThresholds(30.0f, 20.0f);

    // value=25.0 在新范围[20,30]内 → 不告警
    SensorData data1 = makeData(1, 25.0f);
    alarm.onDataReceived(data1);
    TEST_ASSERT_FALSE(alarm.isAlarmActive(), "Should NOT alarm: 25.0 in [20,30]");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 0, "Callback should NOT be called");

    // value=32.0 > 30 → 高告警
    SensorData data2 = makeData(1, 32.0f);
    alarm.onDataReceived(data2);
    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Should alarm: 32.0 > 30");
    TEST_ASSERT_FLOAT_EQ(g_alarmThreshold, 30.0f, 0.01f, "Callback threshold should be new high=30.0");
    TEST_ASSERT_TRUE(g_alarmIsHigh, "Should be high alarm");

    // value=18.0 < 20 → 低告警
    resetGlobals();
    SensorData data3 = makeData(1, 18.0f);
    alarm.onDataReceived(data3);
    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Should alarm: 18.0 < 20");
    TEST_ASSERT_FLOAT_EQ(g_alarmThreshold, 20.0f, 0.01f, "Callback threshold should be new low=20.0");
    TEST_ASSERT_FALSE(g_alarmIsHigh, "Should be low alarm");
    return true;
}

TEST(AlarmSubscriber, NullCallbackSafe) {
    resetGlobals();
    // NULL callback, 不应崩溃
    AlarmSubscriber alarm("TestAlarm", 35.0f, 15.0f, NULL);

    SensorData data = makeData(1, 40.0f);
    alarm.onDataReceived(data);

    TEST_ASSERT_TRUE(alarm.isAlarmActive(), "Alarm should be active even without callback");
    TEST_ASSERT_EQ(alarm.getAlarmCount(), 1u, "Alarm count should still increment");
    TEST_ASSERT_EQ(g_alarmCallbackCount, 0, "Global callback should NOT have been called");
    return true;
}

// ============================================================
//  SafetySubscriber 测试
// ============================================================

TEST(SafetySubscriber, BelowThresholdSafe) {
    resetGlobals();
    SafetySubscriber safety("TestSafety", 100.0f, safetyCallback);

    SensorData data = makeData(1, 80.0f);
    safety.onDataReceived(data);

    TEST_ASSERT_FALSE(safety.isSafetyActive(), "Safety should NOT be active when below danger threshold");
    TEST_ASSERT_FALSE(g_safetyActive, "Callback should NOT have been called with activate=true");
    return true;
}

TEST(SafetySubscriber, AboveThresholdActivates) {
    resetGlobals();
    SafetySubscriber safety("TestSafety", 100.0f, safetyCallback);

    SensorData data = makeData(1, 120.0f);
    safety.onDataReceived(data);

    TEST_ASSERT_TRUE(safety.isSafetyActive(), "Safety should be active when above danger threshold");
    TEST_ASSERT_TRUE(g_safetyActive, "Callback should have been called with activate=true");
    TEST_ASSERT_EQ(g_safetySensorId, 1u, "Callback sensorId should be 1");
    return true;
}

TEST(SafetySubscriber, Hysteresis) {
    resetGlobals();
    SafetySubscriber safety("TestSafety", 100.0f, safetyCallback);

    // 触发安全联动：值超过100
    SensorData data1 = makeData(1, 110.0f);
    safety.onDataReceived(data1);
    TEST_ASSERT_TRUE(safety.isSafetyActive(), "Safety should be active at 110 > 100");
    TEST_ASSERT_TRUE(g_safetyActive, "Callback should report activate=true");

    // 值降到90（仍高于80%滞后值 80） → 不解除
    SensorData data2 = makeData(1, 90.0f);
    safety.onDataReceived(data2);
    TEST_ASSERT_TRUE(safety.isSafetyActive(), "Safety should remain active at 90 > 80 (80% of 100)");

    // 值降到75（低于80%滞后值 80） → 解除
    SensorData data3 = makeData(1, 75.0f);
    safety.onDataReceived(data3);
    TEST_ASSERT_FALSE(safety.isSafetyActive(), "Safety should deactivate at 75 <= 80");
    TEST_ASSERT_FALSE(g_safetyActive, "Callback should report activate=false");

    // 值再次升高超过100 → 重新激活
    SensorData data4 = makeData(1, 105.0f);
    safety.onDataReceived(data4);
    TEST_ASSERT_TRUE(safety.isSafetyActive(), "Safety should reactivate at 105 > 100");
    TEST_ASSERT_TRUE(g_safetyActive, "Callback should report activate=true again");
    return true;
}

TEST(SafetySubscriber, ExactlyAtThresholdNotActive) {
    resetGlobals();
    SafetySubscriber safety("TestSafety", 100.0f, safetyCallback);

    SensorData data = makeData(1, 100.0f);
    safety.onDataReceived(data);

    // 条件为 data.value > dangerThreshold_，等于时不应激活
    TEST_ASSERT_FALSE(safety.isSafetyActive(), "Safety should NOT be active when value exactly equals threshold");
    return true;
}

TEST(SafetySubscriber, NullActionSafe) {
    resetGlobals();
    SafetySubscriber safety("TestSafety", 100.0f, NULL);

    SensorData data = makeData(1, 120.0f);
    safety.onDataReceived(data);

    // NULL action，不应崩溃
    TEST_ASSERT_TRUE(safety.isSafetyActive(), "Safety should still track state with NULL action");
    TEST_ASSERT_FALSE(g_safetyActive, "Global callback should NOT have been modified");
    return true;
}

// ============================================================
//  PIDController 测试
// ============================================================

TEST(PIDController, ProportionalOnly) {
    PIDController pid(1.0f, 0.0f, 0.0f, 25.0f);
    // 测量值20.0，设定值25.0，误差=5.0，Kp=1.0
    float output = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(output, 5.0f, 0.01f, "Proportional output should be Kp * error = 5.0");
    return true;
}

TEST(PIDController, IntegralAccumulation) {
    PIDController pid(0.0f, 1.0f, 0.0f, 25.0f);

    // 第一次：误差=5.0, integral=5.0, output=5.0
    float out1 = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(out1, 5.0f, 0.01f, "First integral output should be 5.0");

    // 第二次：误差=5.0, integral=10.0, output=10.0
    float out2 = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(out2, 10.0f, 0.01f, "Second integral output should be 10.0 (accumulated)");
    return true;
}

TEST(PIDController, OutputClamping) {
    PIDController pid(1.0f, 0.0f, 0.0f, 25.0f);
    pid.setOutputLimits(-1.0f, 1.0f);

    // 测量值0.0，误差=25.0，Kp=1.0 → 输出=25.0，应被钳制到1.0
    float output = pid.compute(0.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(output, 1.0f, 0.01f, "Output should be clamped to max=1.0");

    // 测量值50.0，误差=-25.0，Kp=1.0 → 输出=-25.0，应被钳制到-1.0
    float output2 = pid.compute(50.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(output2, -1.0f, 0.01f, "Output should be clamped to min=-1.0");
    return true;
}

TEST(PIDController, Reset) {
    PIDController pid(0.0f, 1.0f, 0.0f, 25.0f);

    // 积累积分
    pid.compute(20.0f, 1.0f);
    pid.compute(20.0f, 1.0f);
    float integralBefore = pid.getIntegral();
    TEST_ASSERT_TRUE(integralBefore > 1.0f, "Integral should have accumulated");

    // 重置
    pid.reset();
    TEST_ASSERT_FLOAT_EQ(pid.getIntegral(), 0.0f, 0.001f, "Integral should be 0 after reset");

    // 重置后重新计算 → 积分从0开始
    float outAfterReset = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(outAfterReset, 5.0f, 0.01f, "After reset, integral should restart from 0");
    return true;
}

TEST(PIDController, DifferentialTerm) {
    PIDController pid(0.0f, 0.0f, 1.0f, 25.0f);

    // 第一次：误差=5.0, lastError=0, derivative=5.0/1.0=5, output=5.0
    float out1 = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(out1, 5.0f, 0.01f, "First D-term: derivative = (5-0)/1 = 5");

    // 第二次：测量值15.0, 误差=10.0, lastError=5.0, derivative=(10-5)/1=5, output=5
    float out2 = pid.compute(15.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(out2, 5.0f, 0.01f, "Second D-term: derivative = (10-5)/1 = 5");

    return true;
}

TEST(PIDController, SetSetpoint) {
    PIDController pid(1.0f, 0.0f, 0.0f, 25.0f);
    TEST_ASSERT_FLOAT_EQ(pid.getSetpoint(), 25.0f, 0.01f, "Initial setpoint should be 25.0");

    pid.setSetpoint(30.0f);
    TEST_ASSERT_FLOAT_EQ(pid.getSetpoint(), 30.0f, 0.01f, "Setpoint should be updated to 30.0");

    // 测量值20.0, 新设定值30.0, 误差=10.0, Kp=1.0
    float output = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(output, 10.0f, 0.01f, "Output should reflect new setpoint: 10.0");
    return true;
}

TEST(PIDController, SetTunings) {
    PIDController pid(1.0f, 0.0f, 0.0f, 25.0f);

    pid.setTunings(2.0f, 0.5f, 0.1f);

    // 测量值20.0, 误差=5.0
    // P = Kp*error = 2*5 = 10
    // I = Ki*integral = 0.5*(5*1) = 2.5
    // D = Kd*derivative = 0.1*(5/1) = 0.5
    // total = 10 + 2.5 + 0.5 = 13.0
    float output = pid.compute(20.0f, 1.0f);
    TEST_ASSERT_FLOAT_EQ(output, 13.0f, 0.01f, "Output should reflect new tunings: P+I+D = 10+2.5+0.5 = 13.0");
    return true;
}

// ============================================================
//  GuiDisplaySubscriber 测试
// ============================================================

TEST(GuiDisplay, ReceivesData) {
    resetGlobals();
    GuiDisplaySubscriber gui("TestGui", displayFunc);

    SensorData data = makeData(2, 30.5f);
    gui.onDataReceived(data);

    TEST_ASSERT_EQ(gui.getUpdateCount(), 1u, "Update count should be 1 after receiving data");
    TEST_ASSERT_FLOAT_EQ(gui.getLastData().value, 30.5f, 0.01f, "Last data value should be 30.5");
    TEST_ASSERT_EQ(g_displayCallCount, 1, "Display callback should have been called");
    return true;
}

TEST(GuiDisplay, NullCallbackSafe) {
    resetGlobals();
    GuiDisplaySubscriber gui("TestGui", NULL);

    SensorData data = makeData(3, 50.0f);
    gui.onDataReceived(data);

    // NULL display callback，不应崩溃
    TEST_ASSERT_EQ(gui.getUpdateCount(), 1u, "Update count should still increment with NULL callback");
    TEST_ASSERT_FLOAT_EQ(gui.getLastData().value, 50.0f, 0.01f, "Last data should still be stored");
    TEST_ASSERT_EQ(g_displayCallCount, 0, "Global display callback should NOT have been called");
    return true;
}

TEST(GuiDisplay, MultipleUpdatesCountCorrectly) {
    resetGlobals();
    GuiDisplaySubscriber gui("TestGui", displayFunc);

    gui.onDataReceived(makeData(1, 10.0f));
    gui.onDataReceived(makeData(1, 20.0f));
    gui.onDataReceived(makeData(1, 30.0f));

    TEST_ASSERT_EQ(gui.getUpdateCount(), 3u, "Update count should be 3 after 3 data receives");
    TEST_ASSERT_EQ(g_displayCallCount, 3, "Display callback should have been called 3 times");
    return true;
}

// ============================================================
//  MqttSenderSubscriber 测试
// ============================================================

TEST(MqttSender, ReceivesData) {
    resetGlobals();
    MqttSenderSubscriber mqtt("TestMqtt", "sensors/test", mqttPublishFunc);

    SensorData data = makeData(4, 55.5f);
    mqtt.onDataReceived(data);

    TEST_ASSERT_EQ(mqtt.getSendCount(), 1u, "Send count should be 1 after receiving data");
    TEST_ASSERT_EQ(mqtt.getFailCount(), 0u, "Fail count should be 0 on successful publish");
    TEST_ASSERT_EQ(g_mqttCallCount, 1, "Publish callback should have been called once");
    TEST_ASSERT_FLOAT_EQ(mqtt.getLastData().value, 55.5f, 0.01f, "Last data value should be 55.5");
    return true;
}

TEST(MqttSender, NullPublishFunc) {
    resetGlobals();
    MqttSenderSubscriber mqtt("TestMqtt", "sensors/test", NULL);

    SensorData data = makeData(5, 60.0f);
    mqtt.onDataReceived(data);

    // NULL publish func，不应崩溃
    TEST_ASSERT_EQ(mqtt.getSendCount(), 0u, "Send count should remain 0 with NULL publish func");
    TEST_ASSERT_EQ(mqtt.getFailCount(), 0u, "Fail count should be 0 (no publish attempted)");
    TEST_ASSERT_FLOAT_EQ(mqtt.getLastData().value, 60.0f, 0.01f, "Last data should still be stored");
    TEST_ASSERT_EQ(g_mqttCallCount, 0, "Global MQTT callback should NOT have been called");
    return true;
}

TEST(MqttSender, PublishFailureIncrementsFailCount) {
    resetGlobals();
    g_mqttReturnValue = false; // 模拟发布失败
    MqttSenderSubscriber mqtt("TestMqtt", "sensors/test", mqttPublishFunc);

    SensorData data = makeData(6, 70.0f);
    mqtt.onDataReceived(data);

    TEST_ASSERT_EQ(mqtt.getSendCount(), 0u, "Send count should be 0 on publish failure");
    TEST_ASSERT_EQ(mqtt.getFailCount(), 1u, "Fail count should increment on publish failure");
    TEST_ASSERT_EQ(g_mqttCallCount, 1, "Publish callback should have been called once");
    return true;
}

TEST(MqttSender, BatchSending) {
    resetGlobals();
    MqttSenderSubscriber mqtt("TestMqtt", "sensors/test", mqttPublishFunc);
    mqtt.setBatchInterval(3); // 每3条数据发送1次

    // 第1条：batchCount_ = 1 (< 3)，不发送
    mqtt.onDataReceived(makeData(1, 10.0f));
    TEST_ASSERT_EQ(mqtt.getSendCount(), 0u, "Send count should be 0 at batch count 1/3");
    TEST_ASSERT_EQ(g_mqttCallCount, 0, "Callback should NOT be called at batch count 1/3");

    // 第2条：batchCount_ = 2 (< 3)，不发送
    mqtt.onDataReceived(makeData(1, 20.0f));
    TEST_ASSERT_EQ(mqtt.getSendCount(), 0u, "Send count should be 0 at batch count 2/3");

    // 第3条：batchCount_ = 3 (>= 3)，发送并重置
    mqtt.onDataReceived(makeData(1, 30.0f));
    TEST_ASSERT_EQ(mqtt.getSendCount(), 1u, "Send count should be 1 at batch count 3/3");
    TEST_ASSERT_EQ(g_mqttCallCount, 1, "Callback should be called once at batch interval");

    // 第4条：batchCount_ = 1 (< 3)，不发送
    mqtt.onDataReceived(makeData(1, 40.0f));
    TEST_ASSERT_EQ(mqtt.getSendCount(), 1u, "Send count should still be 1 at new batch count 1/3");

    return true;
}

TEST(MqttSender, SetBatchInterval) {
    resetGlobals();
    MqttSenderSubscriber mqtt("TestMqtt", "sensors/test", mqttPublishFunc);

    mqtt.setBatchInterval(5);

    // 前4条不发送
    for (int i = 0; i < 4; ++i) {
        mqtt.onDataReceived(makeData(1, 10.0f));
    }
    TEST_ASSERT_EQ(mqtt.getSendCount(), 0u, "No sends after 4 data with batchInterval=5");

    // 第5条发送
    mqtt.onDataReceived(makeData(1, 10.0f));
    TEST_ASSERT_EQ(mqtt.getSendCount(), 1u, "One send after 5th data with batchInterval=5");
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
