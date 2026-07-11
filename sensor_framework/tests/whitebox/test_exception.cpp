/**
 * @file test_exception.cpp
 * @brief 异常监控白盒单元测试
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../exception/exception_types.hpp"
#include "../../exception/circuit_breaker.hpp"
#include "../../exception/exception_logger.hpp"

// ============================================================
//  熔断器测试
// ============================================================

TEST(CircuitBreaker, InitiallyClosed) {
    CircuitBreaker cb(5, 1000);
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::CLOSED, "Initial state should be CLOSED");
    TEST_ASSERT_TRUE(cb.allowRequest(), "Should allow requests when CLOSED");
    return true;
}

TEST(CircuitBreaker, OpensAfterThreshold) {
    CircuitBreaker cb(3, 1000);

    // 2次失败 → 保持CLOSED
    cb.recordFailure();
    cb.recordFailure();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::CLOSED, "Should stay CLOSED after 2 failures");

    // 第3次失败 → 转为OPEN
    cb.recordFailure();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::OPEN, "Should OPEN after 3 consecutive failures");
    TEST_ASSERT_TRUE(!cb.allowRequest(), "Should NOT allow requests when OPEN");
    return true;
}

TEST(CircuitBreaker, SuccessResetsFailureCount) {
    CircuitBreaker cb(3, 1000);

    cb.recordFailure();
    cb.recordFailure();
    cb.recordSuccess();  // 中断连续失败

    TEST_ASSERT_TRUE(cb.getConsecutiveFailures() == 0, "Failure count should reset after success");
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::CLOSED, "Should remain CLOSED");
    return true;
}

TEST(CircuitBreaker, ManualReset) {
    CircuitBreaker cb(2, 1000);

    cb.recordFailure();
    cb.recordFailure();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::OPEN, "Should be OPEN");

    cb.reset();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::CLOSED, "Should be CLOSED after reset");
    return true;
}

// ============================================================
//  异常日志测试
// ============================================================

TEST(ExceptionLogger, LogAndRetrieve) {
    ExceptionLogger<8> logger;

    ExceptionEvent event;
    event.sensorId = 1;
    event.category = ExceptionCategory::DATA_VALIDITY;
    event.severity = ExceptionSeverity::ERROR;
    event.errorCode = ERR_OUT_OF_RANGE;
    event.contextValue = 999.0f;

    logger.log(event);

    const ExceptionEvent& last = logger.last();
    TEST_ASSERT_TRUE(last.sensorId == 1, "Last event should have sensorId=1");
    TEST_ASSERT_TRUE(last.severity == ExceptionSeverity::ERROR, "Last event should be ERROR");
    TEST_ASSERT_TRUE(logger.getTotalCount() == 1, "Total count should be 1");
    return true;
}

TEST(ExceptionLogger, CircularOverwrite) {
    ExceptionLogger<4> logger;

    for (uint8_t i = 0; i < 6; ++i) {
        ExceptionEvent event;
        event.sensorId = i;
        event.severity = ExceptionSeverity::WARNING;
        logger.log(event);
    }

    TEST_ASSERT_TRUE(logger.getTotalCount() == 6, "Total count should be 6");
    TEST_ASSERT_TRUE(logger.getCount() == 4, "Buffer count should be capped at 4");
    return true;
}

TEST(ExceptionLogger, SeverityFiltering) {
    ExceptionLogger<8> logger;

    ExceptionEvent infoEvent;
    infoEvent.severity = ExceptionSeverity::INFO;
    infoEvent.sensorId = 1;
    logger.log(infoEvent);

    ExceptionEvent warnEvent;
    warnEvent.severity = ExceptionSeverity::WARNING;
    warnEvent.sensorId = 2;
    logger.log(warnEvent);

    ExceptionEvent errorEvent;
    errorEvent.severity = ExceptionSeverity::ERROR;
    errorEvent.sensorId = 3;
    logger.log(errorEvent);

    // 按WARNING以上过滤查询
    ExceptionEvent filtered[8];
    uint8_t count = logger.getRecent(filtered, 8, ExceptionSeverity::WARNING);

    TEST_ASSERT_TRUE(count >= 2, "Should get at least 2 events >= WARNING");
    return true;
}

// ============================================================
//  熔断器 HALF_OPEN / 恢复流程测试
// ============================================================

// 使用可注入时间戳的熔断器进行 HALF_OPEN 测试
static uint32_t mockTick = 0;
static uint32_t mockTickProvider() { return mockTick; }

TEST(CircuitBreaker, HalfOpenAfterCooldown) {
    CircuitBreaker cb(2, 100);
    cb.setTickProvider(mockTickProvider);
    mockTick = 0;

    // 触发熔断
    cb.recordFailure();
    cb.recordFailure();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::OPEN, "Should be OPEN after threshold failures");

    // 冷却时间过后 → 允许一次探测
    mockTick = 200;
    TEST_ASSERT_TRUE(cb.allowRequest(), "Should allow probe after cooldown");
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::HALF_OPEN, "Should be HALF_OPEN after cooldown");

    return true;
}

TEST(CircuitBreaker, HalfOpenSuccessRecovery) {
    CircuitBreaker cb(2, 100);
    cb.setTickProvider(mockTickProvider);
    mockTick = 0;

    cb.recordFailure();
    cb.recordFailure();  // OPEN
    mockTick = 200;
    cb.allowRequest();    // → HALF_OPEN
    cb.recordSuccess();   // 探测成功
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::CLOSED, "Should recover to CLOSED on probe success");

    return true;
}

TEST(CircuitBreaker, HalfOpenFailureReopens) {
    CircuitBreaker cb(2, 100);
    cb.setTickProvider(mockTickProvider);
    mockTick = 0;

    cb.recordFailure();
    cb.recordFailure();  // OPEN
    mockTick = 200;
    cb.allowRequest();    // → HALF_OPEN
    cb.recordFailure();   // 探测失败
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::OPEN, "Should reopen on probe failure");

    return true;
}

TEST(CircuitBreaker, CooldownNotExpired) {
    CircuitBreaker cb(2, 100);
    cb.setTickProvider(mockTickProvider);
    mockTick = 0;

    cb.recordFailure();
    cb.recordFailure();  // OPEN at t=0

    mockTick = 50;  // 冷却时间未到
    TEST_ASSERT_TRUE(!cb.allowRequest(), "Should NOT allow request before cooldown expires");

    return true;
}

TEST(CircuitBreaker, SetThreshold) {
    CircuitBreaker cb(5, 1000);
    cb.setThreshold(2);

    cb.recordFailure();
    cb.recordFailure();
    TEST_ASSERT_TRUE(cb.getState() == CircuitBreaker::OPEN, "Should OPEN at new threshold=2");

    return true;
}

TEST(CircuitBreaker, SetCooldown) {
    CircuitBreaker cb(2, 100);
    cb.setTickProvider(mockTickProvider);
    cb.setCooldown(500);

    mockTick = 0;
    cb.recordFailure();
    cb.recordFailure();  // OPEN

    mockTick = 200;  // < 500
    TEST_ASSERT_TRUE(!cb.allowRequest(), "Should not allow request before new cooldown");

    mockTick = 600;  // >= 500
    TEST_ASSERT_TRUE(cb.allowRequest(), "Should allow after new cooldown");

    return true;
}

TEST(CircuitBreaker, TotalCounters) {
    CircuitBreaker cb(5, 1000);

    cb.recordFailure();
    cb.recordFailure();
    cb.recordSuccess();
    cb.recordFailure();

    TEST_ASSERT_TRUE(cb.getTotalFailures() == 3, "Total failures should be 3");
    TEST_ASSERT_TRUE(cb.getTotalSuccesses() == 1, "Total successes should be 1");

    return true;
}

// ============================================================
//  异常日志扩展测试
// ============================================================

TEST(ExceptionLogger, EmptyLogQuery) {
    ExceptionLogger<4> logger;
    const ExceptionEvent& evt = logger.last();
    // 空日志返回 invalidEvent，sensorId=0
    TEST_ASSERT_TRUE(evt.sensorId == 0, "Empty logger last() should return invalid event");
    TEST_ASSERT_TRUE(logger.getTotalCount() == 0, "Total count should be 0");

    return true;
}

TEST(ExceptionLogger, GetByIndex) {
    ExceptionLogger<8> logger;

    ExceptionEvent e1, e2;
    e1.sensorId = 1; e1.severity = ExceptionSeverity::INFO;
    e2.sensorId = 2; e2.severity = ExceptionSeverity::WARNING;
    logger.log(e1);
    logger.log(e2);

    // get(0) = 最新 (e2), get(1) = 次新 (e1)
    TEST_ASSERT_TRUE(logger.get(0).sensorId == 2, "get(0) should be most recent");
    TEST_ASSERT_TRUE(logger.get(1).sensorId == 1, "get(1) should be second most recent");

    return true;
}

TEST(ExceptionLogger, GetStats) {
    ExceptionLogger<8> logger;

    ExceptionEvent info;
    info.severity = ExceptionSeverity::INFO; info.sensorId = 1;
    logger.log(info);

    ExceptionEvent warn;
    warn.severity = ExceptionSeverity::WARNING; warn.sensorId = 2;
    logger.log(warn);

    ExceptionEvent error;
    error.severity = ExceptionSeverity::ERROR; error.sensorId = 3;
    logger.log(error);
    logger.log(error);  // 2 errors

    uint32_t counts[4];
    logger.getStats(counts);

    TEST_ASSERT_TRUE(counts[0] == 1, "INFO count should be 1");
    TEST_ASSERT_TRUE(counts[1] == 1, "WARNING count should be 1");
    TEST_ASSERT_TRUE(counts[2] == 2, "ERROR count should be 2");
    TEST_ASSERT_TRUE(counts[3] == 0, "FATAL count should be 0");

    return true;
}

TEST(ExceptionLogger, ClearLog) {
    ExceptionLogger<8> logger;

    ExceptionEvent e;
    e.sensorId = 1; e.severity = ExceptionSeverity::INFO;
    logger.log(e);

    logger.clear();
    TEST_ASSERT_TRUE(logger.getTotalCount() == 0, "Total count should be 0 after clear");
    TEST_ASSERT_TRUE(logger.getCount() == 0, "Buffer count should be 0 after clear");

    return true;
}

TEST(ExceptionLogger, LargeCapacity) {
    ExceptionLogger<64> logger;

    for (uint8_t i = 0; i < 64; ++i) {
        ExceptionEvent e;
        e.sensorId = i;
        e.severity = ExceptionSeverity::INFO;
        logger.log(e);
    }

    TEST_ASSERT_TRUE(logger.getTotalCount() == 64, "Total count should be 64");
    TEST_ASSERT_TRUE(logger.getCount() == 64, "Buffer count should be 64 (not capped)");

    return true;
}

TEST(ExceptionLogger, MinimumCapacity) {
    ExceptionLogger<1> logger;

    ExceptionEvent e1, e2;
    e1.sensorId = 1; e1.severity = ExceptionSeverity::INFO;
    e2.sensorId = 2; e2.severity = ExceptionSeverity::WARNING;
    logger.log(e1);
    logger.log(e2);

    TEST_ASSERT_TRUE(logger.getTotalCount() == 2, "Total count should be 2");
    TEST_ASSERT_TRUE(logger.getCount() == 1, "Buffer count should be capped at 1");
    TEST_ASSERT_TRUE(logger.last().sensorId == 2, "Last should be most recent event");

    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
