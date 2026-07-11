/**
 * @file test_fixture.hpp
 * @brief 测试夹具基类 — 提供SetUp/TearDown模板 + 断言宏
 * @details 轻量级测试框架，不依赖外部库。
 *          支持PC端和MCU端（通过串口输出测试结果）。
 */

#pragma once

#include "../feature_config.hpp"

#if SENSOR_FEATURE_TEST_FRAMEWORK

#include <cstdint>
#include <cstring>

// ============================================================
//  断言宏 (轻量级实现，不依赖GoogleTest/Catch2)
// ============================================================

/// 测试统计
struct TestStats {
    uint32_t total;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;

    TestStats() : total(0), passed(0), failed(0), skipped(0) {}
};

/// 全局测试统计（单例）
inline TestStats& getTestStats() {
    static TestStats stats;
    return stats;
}

/// 简单断言宏
#define TEST_ASSERT(cond, msg) \
    do { \
        getTestStats().total++; \
        if (!(cond)) { \
            getTestStats().failed++; \
            testReportFailure(__FILE__, __LINE__, msg); \
            return false; \
        } else { \
            getTestStats().passed++; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg)   TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg)   TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_TRUE(cond, msg) TEST_ASSERT(cond, msg)
#define TEST_ASSERT_FALSE(cond, msg) TEST_ASSERT(!(cond), msg)

/// 浮点数近似相等断言
#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, msg) \
    do { \
        getTestStats().total++; \
        float diff = ((a) > (b)) ? ((a) - (b)) : ((b) - (a)); \
        if (diff > (epsilon)) { \
            getTestStats().failed++; \
            testReportFailure(__FILE__, __LINE__, msg); \
            return false; \
        } else { \
            getTestStats().passed++; \
        } \
    } while(0)

// ============================================================
//  测试用例注册宏
// ============================================================

/// 测试函数类型
typedef bool (*TestFunc)();

/// 测试用例结构
struct TestCase {
    const char* suiteName;
    const char* testName;
    TestFunc    func;
};

/// 最大可注册的测试用例数
#define TEST_MAX_CASES 128

/// 全局测试用例注册表
struct TestRegistry {
    TestCase    cases[TEST_MAX_CASES];
    uint8_t     count;

    TestRegistry() : count(0) {}

    void add(const char* suite, const char* name, TestFunc func) {
        if (count < TEST_MAX_CASES) {
            cases[count].suiteName = suite;
            cases[count].testName  = name;
            cases[count].func      = func;
            ++count;
        }
    }
};

inline TestRegistry& getTestRegistry() {
    static TestRegistry registry;
    return registry;
}

/**
 * @def TEST(suite, name)
 * @brief 定义一个测试用例
 *
 * 使用示例:
 *   TEST(Filter, MeanFilter_Basic) {
 *       MeanFilter<4> filter;
 *       float result = filter.update(25.0f);
 *       TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.01f, "First value should pass through");
 *       return true;
 *   }
 */
#define TEST(suite, name) \
    static bool test_##suite##_##name(); \
    static struct TestRegistrar_##suite##_##name { \
        TestRegistrar_##suite##_##name() { \
            getTestRegistry().add(#suite, #name, test_##suite##_##name); \
        } \
    } registrar_##suite##_##name; \
    static bool test_##suite##_##name()

// ============================================================
//  测试运行器
// ============================================================

/**
 * @brief 运行所有已注册的测试用例
 * @return 通过的测试数量
 */
inline uint32_t runAllTests() {
    TestRegistry& registry = getTestRegistry();
    TestStats& stats = getTestStats();

    stats = TestStats();  // Reset

    for (uint8_t i = 0; i < registry.count; ++i) {
        const TestCase& tc = registry.cases[i];
        bool passed = tc.func();

        const char* status = passed ? "PASS" : "FAIL";
        testPrint("[%s] %s::%s\n", status, tc.suiteName, tc.testName);
    }

    return stats.passed;
}

// ============================================================
//  测试输出函数（平台相关）
// ============================================================

/// 报告测试失败
inline void testReportFailure(const char* file, int line, const char* msg) {
    testPrint("  FAIL at %s:%d - %s\n", file, line, msg);
}

/// 测试输出 — 默认 printf, 嵌入式平台可重写
inline void testPrint(const char* fmt, ...) {
    // 简化: 直接使用 printf（PC端）
    // 嵌入式平台: 替换为 UART printf
#if !defined(SENSOR_MCU_8BIT) && !defined(SENSOR_MCU_16BIT)
    extern int printf(const char* fmt, ...);
    // Note: variadic forwarding simplified here
    // In practice, use vfprintf or platform-specific serial output
#endif
}

/// 获取测试摘要
inline void printTestSummary() {
    TestStats& stats = getTestStats();
    testPrint("\n========================================\n");
    testPrint("  Test Summary\n");
    testPrint("  Total:  %u\n", stats.total);
    testPrint("  Passed: %u\n", stats.passed);
    testPrint("  Failed: %u\n", stats.failed);
    testPrint("========================================\n");
}

#endif // SENSOR_FEATURE_TEST_FRAMEWORK
