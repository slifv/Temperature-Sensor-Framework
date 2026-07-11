/**
 * @file test_fixed_filter.cpp
 * @brief 定点数均值滤波器白盒单元测试
 * @details 测试 FixedMeanFilter 的定点数运算正确性和与浮点版本的一致性
 */

// 启用定点数数学库
#define SENSOR_FEATURE_FIXED_POINT_MATH 1

#include "../../test_framework/test_fixture.hpp"
#include "../../fixed_point/fixed_mean_filter.hpp"
#include "../../filter/mean_filter.hpp"

// ============================================================
//  基本功能测试
// ============================================================

TEST(FixedMeanFilter, EmptyOnInit) {
    FixedMeanFilter<4> filter;
    TEST_ASSERT_TRUE(!filter.isReady(), "Should not be ready on init");
    TEST_ASSERT_EQ(filter.getCount(), 0, "Count should be 0 initially");
    return true;
}

TEST(FixedMeanFilter, ReadyWhenFull) {
    FixedMeanFilter<4> filter;
    for (uint8_t i = 0; i < 4; ++i) {
        filter.update(10.0f);
    }
    TEST_ASSERT_TRUE(filter.isReady(), "Should be ready after 4 updates");
    TEST_ASSERT_EQ(filter.getCount(), 4, "Count should be 4");
    return true;
}

TEST(FixedMeanFilter, PassThroughBeforeFull) {
    FixedMeanFilter<4> filter;

    float r1 = filter.update(99.0f);
    TEST_ASSERT_FLOAT_EQ(r1, 99.0f, 0.01f, "First value should pass through");

    float r2 = filter.update(100.0f);
    TEST_ASSERT_FLOAT_EQ(r2, 100.0f, 0.01f, "Second value should pass through");
    return true;
}

// ============================================================
//  均值计算测试
// ============================================================

TEST(FixedMeanFilter, BasicMean) {
    FixedMeanFilter<4> filter;

    filter.update(10.0f);
    filter.update(20.0f);
    filter.update(30.0f);
    float result = filter.update(40.0f);

    // (10+20+30+40)/4 = 25
    TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.1f, "Mean of [10,20,30,40] should be 25");
    return true;
}

TEST(FixedMeanFilter, UniformValues) {
    FixedMeanFilter<4> filter;

    for (uint8_t i = 0; i < 4; ++i) {
        filter.update(25.0f);
    }
    float result = filter.update(25.0f);

    TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.1f, "Mean of all 25s should be 25");
    return true;
}

TEST(FixedMeanFilter, NegativeValues) {
    FixedMeanFilter<4> filter;

    filter.update(-5.0f);
    filter.update(-15.0f);
    filter.update(-25.0f);
    float result = filter.update(-35.0f);

    // (-5 + -15 + -25 + -35)/4 = -20
    TEST_ASSERT_FLOAT_EQ(result, -20.0f, 0.2f, "Mean of negatives should be -20");
    return true;
}

// ============================================================
//  reset 测试
// ============================================================

TEST(FixedMeanFilter, ResetClearsState) {
    FixedMeanFilter<4> filter;

    for (uint8_t i = 0; i < 4; ++i) {
        filter.update(10.0f);
    }
    TEST_ASSERT_TRUE(filter.isReady(), "Should be ready before reset");

    filter.reset();
    TEST_ASSERT_TRUE(!filter.isReady(), "Should NOT be ready after reset");
    TEST_ASSERT_EQ(filter.getCount(), 0, "Count should be 0 after reset");
    return true;
}

// ============================================================
//  窗口大小测试
// ============================================================

TEST(FixedMeanFilter, WindowSizeSmall) {
    FixedMeanFilter<2> filter;

    filter.update(10.0f);
    float result = filter.update(30.0f);

    // (10+30)/2 = 20
    TEST_ASSERT_FLOAT_EQ(result, 20.0f, 0.1f, "Mean of [10,30] window=2 should be 20");
    return true;
}

TEST(FixedMeanFilter, WindowSizeLarge) {
    FixedMeanFilter<8> filter;

    for (uint8_t i = 0; i < 8; ++i) {
        filter.update(20.0f);
    }
    TEST_ASSERT_TRUE(filter.isReady(), "Should be ready after 8 updates");
    TEST_ASSERT_FLOAT_EQ(filter.update(20.0f), 20.0f, 0.1f, "Mean of all 20s should be 20");
    return true;
}

// ============================================================
//  与浮点版本对比
// ============================================================

TEST(FixedMeanFilter, CompareWithFloatMean) {
    FixedMeanFilter<4> fixedFilter;
    MeanFilter<4> floatFilter;

    float inputs[] = {25.1f, 25.8f, 24.9f, 25.2f, 25.0f, 25.5f, 24.7f, 25.3f};

    for (uint8_t i = 0; i < 8; ++i) {
        float fixedResult = fixedFilter.update(inputs[i]);
        float floatResult = floatFilter.update(inputs[i]);

        if (fixedFilter.isReady() && floatFilter.isReady()) {
            float diff = fixedResult - floatResult;
            if (diff < 0.0f) diff = -diff;
            TEST_ASSERT_TRUE(diff < 0.05f, "Fixed and float mean should differ by < 0.05");
        }
    }

    return true;
}

// ============================================================
//  获取原始缓冲区
// ============================================================

TEST(FixedMeanFilter, GetRawBuffer) {
    FixedMeanFilter<4> filter;

    filter.update(1.0f);
    filter.update(2.0f);
    filter.update(3.0f);
    filter.update(4.0f);

    const q15_16_t* buf = filter.getRawBuffer();
    TEST_ASSERT_TRUE(buf != NULL, "Raw buffer should not be NULL");

    // 验证缓冲区中的定点数可以转回原值
    float v1 = qmath::toFloat(buf[0]);
    float v2 = qmath::toFloat(buf[1]);
    TEST_ASSERT_FLOAT_EQ(v1, 1.0f, 0.01f, "Buffer[0] should be ~1.0");
    TEST_ASSERT_FLOAT_EQ(v2, 2.0f, 0.01f, "Buffer[1] should be ~2.0");

    return true;
}

// ============================================================
//  便捷别名测试
// ============================================================

TEST(FixedMeanFilter, ConvenienceAliases) {
    FixedMeanFilter4 f4;
    FixedMeanFilter8 f8;

    TEST_ASSERT_EQ(f4.getWindowSize(), 4, "FixedMeanFilter4 should have window=4");
    TEST_ASSERT_EQ(f8.getWindowSize(), 8, "FixedMeanFilter8 should have window=8");
    return true;
}

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
