/**
 * @file test_reporter.hpp
 * @brief 测试报告生成器 — 生成结构化测试报告
 * @details 支持纯文本、JSON、JUnit XML格式。
 *          可配置输出目标（控制台/文件/串口）。
 *          参考: 需求评审报告 FR-AT-07
 */

#pragma once

#include "../feature_config.hpp"
#include "test_fixture.hpp"

#if SENSOR_FEATURE_TEST_FRAMEWORK

#include <cstdint>

/**
 * @enum ReportFormat
 * @brief 测试报告输出格式
 */
enum class ReportFormat : uint8_t {
    TEXT    = 0,    ///< 纯文本/控制台
    JSON    = 1,    ///< JSON格式（可被CI工具解析）
    JUNIT   = 2     ///< JUnit XML格式（Jenkins兼容）
};

/**
 * @class TestReporter
 * @brief 测试报告生成器
 */
class TestReporter {
public:
    TestReporter() : format_(ReportFormat::TEXT) {}

    void setFormat(ReportFormat fmt) { format_ = fmt; }

    /**
     * @brief 生成测试报告
     */
    void generate() {
        TestStats& stats = getTestStats();
        TestRegistry& registry = getTestRegistry();

        switch (format_) {
            case ReportFormat::TEXT:  generateText(stats, registry);  break;
            case ReportFormat::JSON:  generateJson(stats, registry);  break;
            case ReportFormat::JUNIT: generateJUnit(stats, registry); break;
        }
    }

private:
    ReportFormat format_;

    void generateText(const TestStats& stats, const TestRegistry& registry) {
        testPrint("\n");
        testPrint("==============================================\n");
        testPrint("  Sensor Framework Test Report\n");
        testPrint("==============================================\n");
        testPrint("  Total:    %u\n", stats.total);
        testPrint("  Passed:   %u\n", stats.passed);
        testPrint("  Failed:   %u\n", stats.failed);
        testPrint("  Skipped:  %u\n", stats.skipped);

        if (stats.total > 0) {
            uint32_t passRate = (stats.passed * 100) / stats.total;
            testPrint("  PassRate: %u%%\n", passRate);
        }

        testPrint("----------------------------------------------\n");

        // 列出测试套件
        const char* currentSuite = "";
        for (uint8_t i = 0; i < registry.count; ++i) {
            const TestCase& tc = registry.cases[i];
            if (std::strcmp(tc.suiteName, currentSuite) != 0) {
                testPrint("  Suite: %s\n", tc.suiteName);
                currentSuite = tc.suiteName;
            }
            testPrint("    - %s\n", tc.testName);
        }

        testPrint("==============================================\n");

        if (stats.failed > 0) {
            testPrint("  *** %u TEST(S) FAILED ***\n", stats.failed);
        } else {
            testPrint("  All tests passed!\n");
        }
    }

    void generateJson(const TestStats& stats, const TestRegistry& registry) {
        testPrint("{\n");
        testPrint("  \"testFramework\": \"SensorFramework\",\n");
        testPrint("  \"summary\": {\n");
        testPrint("    \"total\": %u,\n", stats.total);
        testPrint("    \"passed\": %u,\n", stats.passed);
        testPrint("    \"failed\": %u,\n", stats.failed);
        testPrint("    \"skipped\": %u\n", stats.skipped);
        testPrint("  },\n");
        testPrint("  \"testCases\": [\n");

        for (uint8_t i = 0; i < registry.count; ++i) {
            const TestCase& tc = registry.cases[i];
            testPrint("    {\"suite\": \"%s\", \"name\": \"%s\"}%s\n",
                      tc.suiteName, tc.testName,
                      (i < registry.count - 1) ? "," : "");
        }

        testPrint("  ]\n");
        testPrint("}\n");
    }

    void generateJUnit(const TestStats& stats, const TestRegistry& registry) {
        testPrint("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        testPrint("<testsuite name=\"SensorFramework\" ");
        testPrint("tests=\"%u\" failures=\"%u\" errors=\"0\" ", stats.total, stats.failed);
        testPrint("skipped=\"%u\">\n", stats.skipped);

        for (uint8_t i = 0; i < registry.count; ++i) {
            const TestCase& tc = registry.cases[i];
            testPrint("  <testcase classname=\"%s\" name=\"%s\" />\n",
                      tc.suiteName, tc.testName);
        }

        testPrint("</testsuite>\n");
    }
};

#endif // SENSOR_FEATURE_TEST_FRAMEWORK
