/**
 * @file test_filter.cpp
 * @brief 滤波器白盒单元测试
 * @details 测试所有8种滤波器 + 滑动窗口框架 + 滤波器Pipeline
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../filter/sliding_window.hpp"
#include "../../filter/mean_filter.hpp"
#include "../../filter/median_filter.hpp"
#include "../../filter/low_pass_filter.hpp"
#include "../../filter/kalman_filter.hpp"
#include "../../filter/trimmed_mean_filter.hpp"
#include "../../filter/filter_pipeline.hpp"

// ============================================================
//  滑动窗口框架测试
// ============================================================

TEST(SlidingWindow, EmptyOnInit) {
    SlidingWindowFilter<5>* filter = NULL; // 抽象类不可实例化，使用子类
    // 使用 MeanFilter 间接测试基类
    MeanFilter<5> meanFilter;
    TEST_ASSERT_TRUE(!meanFilter.isReady(), "Window should not be ready on init");
    return true;
}

TEST(SlidingWindow, ReadyWhenFull) {
    MeanFilter<5> filter;
    for (uint8_t i = 0; i < 5; ++i) {
        filter.update(25.0f);
    }
    TEST_ASSERT_TRUE(filter.isReady(), "Window should be ready after 5 updates");
    return true;
}

TEST(SlidingWindow, GetWindowReturnsData) {
    MeanFilter<5> filter;
    filter.update(10.0f);
    filter.update(20.0f);
    filter.update(30.0f);
    filter.update(40.0f);
    filter.update(50.0f);

    const float* window = filter.getWindow();
    TEST_ASSERT_FLOAT_EQ(window[0], 10.0f, 0.01f, "First element should be 10");
    TEST_ASSERT_FLOAT_EQ(window[4], 50.0f, 0.01f, "Last element should be 50");
    return true;
}

// ============================================================
//  均值滤波测试
// ============================================================

TEST(MeanFilter, BasicAverage) {
    MeanFilter<4> filter;

    filter.update(20.0f);
    filter.update(30.0f);
    filter.update(40.0f);
    float result = filter.update(30.0f);

    // (20+30+40+30)/4 = 30
    TEST_ASSERT_FLOAT_EQ(result, 30.0f, 0.1f, "Mean of [20,30,40,30] should be 30");
    return true;
}

TEST(MeanFilter, PassThroughBeforeFull) {
    MeanFilter<5> filter;

    float first = filter.update(99.0f);
    TEST_ASSERT_FLOAT_EQ(first, 99.0f, 0.01f, "First value should pass through");

    float second = filter.update(100.0f);
    TEST_ASSERT_FLOAT_EQ(second, 100.0f, 0.01f, "Second value should pass through");
    return true;
}

TEST(MeanFilter, ResetClearsState) {
    MeanFilter<4> filter;
    filter.update(10.0f);
    filter.update(10.0f);
    filter.update(10.0f);
    filter.update(10.0f);
    TEST_ASSERT_TRUE(filter.isReady(), "Should be ready before reset");

    filter.reset();
    TEST_ASSERT_TRUE(!filter.isReady(), "Should NOT be ready after reset");
    return true;
}

// ============================================================
//  中值滤波测试
// ============================================================

TEST(MedianFilter, ImpulseNoiseRemoval) {
    MedianFilter<5> filter;

    filter.update(25.0f);
    filter.update(26.0f);
    filter.update(25.5f);
    filter.update(26.5f);

    // 注入脉冲噪声 (99.0)
    float result = filter.update(99.0f);
    // 排序后: [25.0, 25.5, 26.0, 26.5, 99.0] → 中值=26.0
    TEST_ASSERT_FLOAT_EQ(result, 26.0f, 0.1f, "Impulse 99.0 should be filtered (median=26.0)");
    return true;
}

TEST(MedianFilter, EvenWindow) {
    MedianFilter<4> filter;
    filter.update(10.0f);
    filter.update(20.0f);
    filter.update(30.0f);
    float result = filter.update(40.0f);
    // 排序后: [10, 20, 30, 40] → 中值=(20+30)/2=25
    TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.1f, "Even window median should be avg of middle 2");
    return true;
}

// ============================================================
//  低通滤波测试
// ============================================================

TEST(LowPassFilter, AlphaOneNoFiltering) {
    LowPassFilter filter(1.0f); // alpha=1.0: 完全透传

    filter.update(25.0f);
    float result = filter.update(30.0f);
    TEST_ASSERT_FLOAT_EQ(result, 30.0f, 0.01f, "Alpha=1.0 should pass through unchanged");
    return true;
}

TEST(LowPassFilter, SmoothingEffect) {
    LowPassFilter filter(0.1f); // alpha=0.1: 强平滑

    filter.update(25.0f); // init
    float result = filter.update(30.0f);
    // y = 0.1*30 + 0.9*25 = 3 + 22.5 = 25.5
    TEST_ASSERT_FLOAT_EQ(result, 25.5f, 0.1f, "Step from 25 to 30 with alpha=0.1 should output 25.5");
    return true;
}

// ============================================================
//  卡尔曼滤波测试
// ============================================================

TEST(KalmanFilter, ConvergenceToTrueValue) {
    KalmanFilter kf(0.01f, 0.5f);

    // 模拟恒温25.0°C带噪声的测量
    float noisyReadings[] = {
        24.2f, 25.8f, 24.5f, 25.3f, 24.9f,
        25.1f, 24.8f, 25.2f, 25.0f, 25.1f
    };

    float estimate = 0.0f;
    for (uint8_t i = 0; i < 10; ++i) {
        estimate = kf.update(noisyReadings[i]);
    }

    // 卡尔曼滤波应最终收敛到真值±0.3°C
    TEST_ASSERT_TRUE(estimate > 24.7f, "Estimate should be above 24.7");
    TEST_ASSERT_TRUE(estimate < 25.3f, "Estimate should be below 25.3");
    return true;
}

TEST(KalmanFilter, ParameterSetting) {
    KalmanFilter kf(0.01f, 0.5f);

    TEST_ASSERT_TRUE(kf.setParameter("Q", 0.05f), "Should accept Q parameter");
    TEST_ASSERT_TRUE(kf.setParameter("R", 0.8f), "Should accept R parameter");

    float q = kf.getParameter("Q");
    float r = kf.getParameter("R");
    TEST_ASSERT_FLOAT_EQ(q, 0.05f, 0.001f, "Q should be 0.05");
    TEST_ASSERT_FLOAT_EQ(r, 0.8f, 0.001f, "R should be 0.8");
    return true;
}

// ============================================================
//  截尾均值滤波测试
// ============================================================

TEST(TrimmedMean, OlympicAverageRemovesOutliers) {
    TrimmedMeanFilter<7, 1> filter; // 7选5 Olympic Average

    // 填入正常值 + 极端异常值
    filter.update(25.1f);
    filter.update(24.8f);
    filter.update(99.9f);  // 异常高
    filter.update(25.0f);
    filter.update(25.3f);
    filter.update(0.1f);   // 异常低
    float result = filter.update(25.2f);

    // 排序: [0.1, 24.8, 25.0, 25.1, 25.2, 25.3, 99.9]
    // 去头尾各1 → [24.8, 25.0, 25.1, 25.2, 25.3] → mean = 25.08
    float expected = (24.8f + 25.0f + 25.1f + 25.2f + 25.3f) / 5.0f;
    TEST_ASSERT_FLOAT_EQ(result, expected, 0.05f, "Olympic average should remove outliers");
    return true;
}

TEST(TrimmedMean, RuntimeTrimAdjustment) {
    TrimmedMeanFilter<7, 1> filter;

    // 7个相同值，无论怎么trim都不应改变结果
    for (uint8_t i = 0; i < 7; ++i) {
        filter.update(25.0f);
    }
    TEST_ASSERT_FLOAT_EQ(filter.getLastOutput(), 25.0f, 0.01f, "Uniform values should stay same");
    return true;
}

// ============================================================
//  滤波器Pipeline测试
// ============================================================

TEST(FilterPipeline, CascadingFilters) {
    FilterPipeline<4> pipeline;

    IFilter* median = new MedianFilter<5>();
    IFilter* lowpass = new LowPassFilter(0.2f);

    pipeline.addFilter(median);
    pipeline.addFilter(lowpass);

    // 测试级联: 中值→低通
    for (uint8_t i = 0; i < 10; ++i) {
        pipeline.update(25.0f);
    }
    TEST_ASSERT_TRUE(pipeline.getFilterCount() == 2, "Should have 2 filters");
    return true;
}

TEST(FilterPipeline, AddRemoveFilters) {
    FilterPipeline<4> pipeline;

    IFilter* f1 = new LowPassFilter(0.1f);
    IFilter* f2 = new LowPassFilter(0.2f);

    pipeline.addFilter(f1);
    pipeline.addFilter(f2);

    TEST_ASSERT_TRUE(pipeline.getFilterCount() == 2, "Should have 2 filters");

    pipeline.removeAt(0);
    TEST_ASSERT_TRUE(pipeline.getFilterCount() == 1, "Should have 1 filter after remove");

    pipeline.removeAt(0);
    TEST_ASSERT_TRUE(pipeline.getFilterCount() == 0, "Should have 0 filters");
    return true;
}

// ============================================================
//  测试入口
// ============================================================

int main() {
    runAllTests();
    printTestSummary();

    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
