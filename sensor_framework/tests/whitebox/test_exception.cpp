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

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
