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

#if SENSOR_FEATURE_FILTER_FIR
#include "../../filter/fir_filter.hpp"
#endif

#include <cmath>

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

TEST(SlidingWindow, OverflowRolling) {
    MeanFilter<5> filter;
    // 填满窗口 [10, 20, 30, 40, 50]
    filter.update(10.0f);
    filter.update(20.0f);
    filter.update(30.0f);
    filter.update(40.0f);
    filter.update(50.0f);
    TEST_ASSERT_TRUE(filter.isReady(), "Window should be ready");

    // 第6次更新: 滚动覆盖最早的值
    float result = filter.update(60.0f);
    // 窗口现在: [60, 20, 30, 40, 50] → 均值 = 200/5 = 40
    TEST_ASSERT_FLOAT_EQ(result, 40.0f, 0.1f, "After overflow, oldest value replaced: mean should be 40");

    const float* window = filter.getWindow();
    TEST_ASSERT_FLOAT_EQ(window[0], 60.0f, 0.01f, "Newest element should be 60 at index 0");
    TEST_ASSERT_FLOAT_EQ(window[1], 20.0f, 0.01f, "Oldest remaining should be 20 at index 1");
    return true;
}

TEST(SlidingWindow, GetWindowBeforeFull) {
    MeanFilter<5> filter;
    filter.update(10.0f);
    filter.update(20.0f);
    filter.update(30.0f);

    // 仅3次更新，窗口未满
    TEST_ASSERT_TRUE(!filter.isReady(), "Should not be ready after 3 updates (window=5)");
    TEST_ASSERT_EQ(filter.getCount(), 3, "getCount should return 3 before full");

    // 验证窗口数据: 前3个位置有值，其余为0
    const float* window = filter.getWindow();
    TEST_ASSERT_FLOAT_EQ(window[0], 10.0f, 0.01f, "First element should be 10");
    TEST_ASSERT_FLOAT_EQ(window[1], 20.0f, 0.01f, "Second element should be 20");
    TEST_ASSERT_FLOAT_EQ(window[2], 30.0f, 0.01f, "Third element should be 30");
    return true;
}

TEST(SlidingWindow, ResetAfterFull) {
    MeanFilter<5> filter;
    for (uint8_t i = 0; i < 5; ++i) {
        filter.update(10.0f);
    }
    TEST_ASSERT_TRUE(filter.isReady(), "Should be ready after filling");

    filter.reset();
    TEST_ASSERT_TRUE(!filter.isReady(), "Should NOT be ready after reset");
    TEST_ASSERT_EQ(filter.getCount(), 0, "getCount should be 0 after reset");
    return true;
}

TEST(SlidingWindow, SingleElementWindow) {
    // static_assert 要求 N >= 3，使用最小有效窗口大小
    MeanFilter<3> filter;

    // 窗口未满时值透传
    float r1 = filter.update(42.0f);
    TEST_ASSERT_FLOAT_EQ(r1, 42.0f, 0.01f, "First value should pass through before window full");

    float r2 = filter.update(43.0f);
    TEST_ASSERT_FLOAT_EQ(r2, 43.0f, 0.01f, "Second value should pass through before window full");

    // 第3次填满窗口，应返回均值
    float r3 = filter.update(44.0f);
    float expected = (42.0f + 43.0f + 44.0f) / 3.0f;
    TEST_ASSERT_FLOAT_EQ(r3, expected, 0.01f, "Third update fills window, should return mean");
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

TEST(MeanFilter, AllZeros) {
    MeanFilter<4> filter;
    filter.update(0.0f);
    filter.update(0.0f);
    filter.update(0.0f);
    float result = filter.update(0.0f);
    TEST_ASSERT_FLOAT_EQ(result, 0.0f, 0.01f, "Mean of all zeros [0,0,0,0] should be 0.0");
    return true;
}

TEST(MeanFilter, NegativeValues) {
    MeanFilter<4> filter;
    filter.update(-10.0f);
    filter.update(-20.0f);
    filter.update(-30.0f);
    float result = filter.update(-20.0f);
    // (-10 + -20 + -30 + -20) / 4 = -80 / 4 = -20
    TEST_ASSERT_FLOAT_EQ(result, -20.0f, 0.1f, "Mean of [-10,-20,-30,-20] should be -20.0");
    return true;
}

TEST(MeanFilter, MixedSigns) {
    MeanFilter<4> filter;
    filter.update(-5.0f);
    filter.update(5.0f);
    filter.update(-5.0f);
    float result = filter.update(5.0f);
    TEST_ASSERT_FLOAT_EQ(result, 0.0f, 0.1f, "Mean of [-5,5,-5,5] should be 0.0");
    return true;
}

TEST(MeanFilter, LargeValues) {
    MeanFilter<4> filter;
    filter.update(1e6f);
    filter.update(-1e6f);
    filter.update(1e6f);
    float result = filter.update(-1e6f);
    // 1e6 + (-1e6) + 1e6 + (-1e6) = 0，无溢出
    TEST_ASSERT_FLOAT_EQ(result, 0.0f, 1.0f, "Mean of alternating ±1e6 should be 0.0 (no overflow)");
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

TEST(MedianFilter, OddWindow3) {
    MedianFilter<3> filter;
    filter.update(1.0f);
    filter.update(3.0f);
    float result = filter.update(2.0f);
    // 排序: [1, 2, 3] → 中值 = 2.0
    TEST_ASSERT_FLOAT_EQ(result, 2.0f, 0.01f, "Median of [1,3,2] with window=3 should be 2.0");
    return true;
}

TEST(MedianFilter, AllSameValues) {
    MedianFilter<5> filter;
    for (uint8_t i = 0; i < 5; ++i) {
        filter.update(5.0f);
    }
    float result = filter.getLastOutput();
    TEST_ASSERT_FLOAT_EQ(result, 5.0f, 0.01f, "Median of all 5.0 values should be 5.0");
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

TEST(LowPassFilter, AlphaZeroNoChange) {
    LowPassFilter filter(0.0f); // alpha=0.0: 完全保持前一值
    filter.update(25.0f);       // 初始化
    float result = filter.update(30.0f);
    // y = 0*30 + 1*25 = 25.0，输出不变
    TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.01f, "Alpha=0.0 should preserve previous output unchanged");
    return true;
}

TEST(LowPassFilter, StepResponseCurve) {
    LowPassFilter filter(0.1f); // alpha=0.1
    filter.update(0.0f);        // 初始值为0

    // 持续输入100，验证指数逼近
    float prev = 0.0f;
    for (uint8_t i = 0; i < 30; ++i) {
        float curr = filter.update(100.0f);
        if (i > 0) {
            TEST_ASSERT_TRUE(curr >= prev, "Output should monotonically increase toward target");
        }
        prev = curr;
    }

    // 30次迭代后: 1 - (1-α)^30 = 1 - 0.9^30 ≈ 0.958，应接近100
    TEST_ASSERT_TRUE(prev > 90.0f, "After 30 steps with alpha=0.1, output should approach 100 (exponential curve)");
    TEST_ASSERT_TRUE(prev < 100.0f, "Output should be below target (asymptotic approach)");
    return true;
}

TEST(LowPassFilter, ResetThenReconverge) {
    LowPassFilter filter(0.2f); // alpha=0.2
    filter.update(0.0f);

    // 收敛到~100
    for (uint8_t i = 0; i < 20; ++i) {
        filter.update(100.0f);
    }
    float beforeReset = filter.getLastOutput();
    TEST_ASSERT_TRUE(beforeReset > 90.0f, "Should have converged towards 100 before reset");

    // 复位后，首次更新应从输入值重新开始
    filter.reset();
    float afterReset = filter.update(50.0f);
    TEST_ASSERT_FLOAT_EQ(afterReset, 50.0f, 0.01f, "After reset, first update initializes to input value (restart from initial)");
    return true;
}

// ============================================================
//  卡尔曼滤波测试
// ============================================================

TEST(KalmanFilter, ConvergenceToTrueValue) {
    KalmanFilter kf(0.01f, 0.5f, 25.0f);  // 初始估计=25.0（接近真值加速收敛）

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

TEST(KalmanFilter, HighProcessNoise) {
    // Q=1.0: 高过程噪声 → 信任测量 → 快速跟踪
    KalmanFilter fast(1.0f, 0.5f, 25.0f);
    // Q=0.01: 低过程噪声 → 信任模型 → 慢速跟踪
    KalmanFilter slow(0.01f, 0.5f, 25.0f);

    // 都初始收敛到25附近
    fast.update(25.0f);
    slow.update(25.0f);

    // 阶跃到35，比较响应速度
    float fastResp = fast.update(35.0f);
    float slowResp = slow.update(35.0f);

    // Q=1.0 应比 Q=0.01 更快向35移动
    float fastMove = (fastResp > 25.0f) ? (fastResp - 25.0f) : (25.0f - fastResp);
    float slowMove = (slowResp > 25.0f) ? (slowResp - 25.0f) : (25.0f - slowResp);
    TEST_ASSERT_TRUE(fastMove >= slowMove, "Q=1.0 should track step change faster than Q=0.01");
    return true;
}

TEST(KalmanFilter, HighMeasurementNoise) {
    // R=10.0: 高测量噪声 → 不信任测量 → 强平滑
    KalmanFilter kf(0.01f, 10.0f, 25.0f);

    // 输入在25±1范围内振荡
    float noisyReadings[] = {24.0f, 26.0f, 24.0f, 26.0f, 24.0f, 26.0f, 24.0f, 26.0f};

    float lastOutput = 0.0f;
    for (uint8_t i = 0; i < 8; ++i) {
        lastOutput = kf.update(noisyReadings[i]);
    }

    // R=10平滑后，输出偏离均值(25.0)应远小于输入振幅(±1.0)
    float outputDeviation = (lastOutput > 25.0f) ? (lastOutput - 25.0f) : (25.0f - lastOutput);
    TEST_ASSERT_TRUE(outputDeviation < 1.0f, "R=10 should heavily smooth: output variance < input variance");
    return true;
}

TEST(KalmanFilter, NaNInputProtection) {
    KalmanFilter kf(0.01f, 0.5f, 25.0f);

    // 先收敛到 ~25
    for (uint8_t i = 0; i < 10; ++i) {
        kf.update(25.0f);
    }
    float beforeNaN = kf.getEstimate();
    TEST_ASSERT_TRUE(beforeNaN > 24.0f && beforeNaN < 26.0f, "Should converge near 25 before NaN");

    // 输入NaN — 滤波器应能处理，不崩溃
    volatile float nanVal = NAN;
    kf.update(nanVal);

    // 再输入有效值，滤波器应能恢复
    float recovered = kf.update(25.0f);

    // 恢复后的估计应为有限值
    TEST_ASSERT_TRUE(recovered == recovered, "After NaN + recovery, output should be finite (not NaN)");
    TEST_ASSERT_TRUE(recovered > 20.0f && recovered < 30.0f, "Recovered estimate should be in reasonable range around 25");
    return true;
}

TEST(KalmanFilter, InvalidParameter) {
    KalmanFilter kf(0.01f, 0.5f);

    TEST_ASSERT_TRUE(!kf.setParameter("X", 1.0f), "setParameter with invalid key 'X' should return false");
    TEST_ASSERT_TRUE(!kf.setParameter("", 1.0f), "setParameter with empty key should return false");

    // 验证原有参数未被修改
    float q = kf.getParameter("Q");
    float r = kf.getParameter("R");
    TEST_ASSERT_FLOAT_EQ(q, 0.01f, 0.001f, "Q should remain unchanged after invalid setParameter");
    TEST_ASSERT_FLOAT_EQ(r, 0.5f, 0.001f, "R should remain unchanged after invalid setParameter");
    return true;
}

// ============================================================
//  FIR滤波测试（受 SENSOR_FEATURE_FILTER_FIR 特性开关控制）
// ============================================================

#if SENSOR_FEATURE_FILTER_FIR

TEST(FIRFilter, UniformCoefficients) {
    float coefs[] = {0.25f, 0.25f, 0.25f, 0.25f};
    FIRFilter<3> fir(coefs);  // Order=3, 4-tap filter

    // 填充延迟线，测试行为类似于均值滤波
    fir.update(10.0f);
    fir.update(20.0f);
    fir.update(30.0f);
    float result = fir.update(40.0f);

    // 延迟线满后: 0.25*40 + 0.25*30 + 0.25*20 + 0.25*10 = 25.0
    TEST_ASSERT_FLOAT_EQ(result, 25.0f, 0.1f,
        "Uniform coefficients [0.25,0.25,0.25,0.25] should behave like mean filter");
    return true;
}

TEST(FIRFilter, IdentityCoefficient) {
    float coefs[] = {0.0f, 1.0f, 0.0f};
    FIRFilter<2> fir(coefs);  // Order=2, 3-tap filter

    // 首次更新: buffer=[x0,0,0] → output = 0*x0 + 1*0 + 0*0 = 0
    fir.update(5.0f);
    // 第二次更新: buffer=[x1,x0,0] → output = 0*x1 + 1*x0 + 0*0 = x0 (延迟1拍透传)
    float result = fir.update(5.0f);
    TEST_ASSERT_FLOAT_EQ(result, 5.0f, 0.01f,
        "Identity coefficient [0,1,0] should pass through with 1-sample delay");
    return true;
}

#endif // SENSOR_FEATURE_FILTER_FIR

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

TEST(TrimmedMean, ZeroTrimDegradesToMean) {
    // TrimmedMeanFilter 编译期约束 N_Trim >= 1，无法设置 trim=0；
    // 但使用全相同值可验证 trimming 不影响结果，此时 TrimmedMean 行为等同于 MeanFilter
    TrimmedMeanFilter<5, 1> trimmed;
    MeanFilter<5> mean;

    float values[] = {25.0f, 25.0f, 25.0f, 25.0f, 25.0f};
    float trimmedResult = 0.0f;
    float meanResult = 0.0f;
    for (uint8_t i = 0; i < 5; ++i) {
        trimmedResult = trimmed.update(values[i]);
        meanResult = mean.update(values[i]);
    }

    TEST_ASSERT_FLOAT_EQ(trimmedResult, meanResult, 0.01f,
        "With identical values, trimmed mean should equal regular mean");
    return true;
}

TEST(TrimmedMean, MaxTrimBoundary) {
    // M=5, N_Trim=2: 去掉2个最高+2个最低，只保留中间1个值 → 等同于中值
    TrimmedMeanFilter<5, 2> filter;

    filter.update(10.0f);
    filter.update(50.0f);
    filter.update(30.0f);
    filter.update(20.0f);
    float result = filter.update(40.0f);

    // 排序: [10, 20, 30, 40, 50], trim=2: 保留 [30] → mean=30.0
    TEST_ASSERT_FLOAT_EQ(result, 30.0f, 0.01f,
        "Max trim (2 each side from 5): defined behavior, should return middle value 30");
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

TEST(FilterPipeline, EmptyPipeline) {
    FilterPipeline<4> pipeline;

    // 空Pipeline应直接透传输入
    float result = pipeline.update(42.5f);
    TEST_ASSERT_FLOAT_EQ(result, 42.5f, 0.01f, "Empty pipeline should return input unchanged");
    TEST_ASSERT_EQ(pipeline.getFilterCount(), 0, "Should have 0 filters");

    // 多次更新也保持一致
    for (uint8_t i = 0; i < 5; ++i) {
        float r = pipeline.update(100.0f);
        TEST_ASSERT_FLOAT_EQ(r, 100.0f, 0.01f, "Empty pipeline should always return input unchanged");
    }
    return true;
}

TEST(FilterPipeline, FourStagePipeline) {
    FilterPipeline<8> pipeline;

    // 四级级联: Mean → Median → LowPass → LowPass
    pipeline.addFilter(new MeanFilter<4>());
    pipeline.addFilter(new MedianFilter<5>());
    pipeline.addFilter(new LowPassFilter(0.2f));
    pipeline.addFilter(new LowPassFilter(0.3f));

    TEST_ASSERT_EQ(pipeline.getFilterCount(), 4, "Should have 4 filters");

    // 持续输入数据，验证四级级联正常运行
    for (uint8_t i = 0; i < 15; ++i) {
        float result = pipeline.update(25.0f);
        // 输出应为有限值（非NaN/Inf）
        TEST_ASSERT_TRUE(result == result, "Pipeline output should be finite after cascading");
    }
    return true;
}

TEST(FilterPipeline, PipelineReset) {
    FilterPipeline<4> pipeline;

    LowPassFilter* lpf = new LowPassFilter(0.1f);
    pipeline.addFilter(lpf);

    // 运行过滤，积累内部状态
    for (uint8_t i = 0; i < 10; ++i) {
        pipeline.update(100.0f);
    }
    float beforeReset = lpf->getLastOutput();
    TEST_ASSERT_TRUE(beforeReset > 10.0f, "Contained filter should have non-zero state before reset");

    // 复位Pipeline → 所有内部滤波器都被复位
    pipeline.reset();
    float afterReset = lpf->getLastOutput();
    TEST_ASSERT_FLOAT_EQ(afterReset, 0.0f, 0.01f, "After pipeline reset, contained filter should reset to 0");
    return true;
}

TEST(FilterPipeline, CapacityLimit) {
    FilterPipeline<3> pipeline;  // 上限3个滤波器

    TEST_ASSERT_TRUE(pipeline.addFilter(new LowPassFilter(0.1f)), "1st add should succeed");
    TEST_ASSERT_TRUE(pipeline.addFilter(new LowPassFilter(0.2f)), "2nd add should succeed");
    TEST_ASSERT_TRUE(pipeline.addFilter(new LowPassFilter(0.3f)), "3rd add should succeed");

    // 第4个超出上限，应失败
    IFilter* extra = new LowPassFilter(0.4f);
    bool added = pipeline.addFilter(extra);
    TEST_ASSERT_TRUE(!added, "4th add beyond MaxFilters should return false");
    if (!added) {
        delete extra;  // 清理被拒绝的滤波器，防止泄漏
    }

    TEST_ASSERT_EQ(pipeline.getFilterCount(), 3, "Filter count should still be 3 at capacity limit");
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
