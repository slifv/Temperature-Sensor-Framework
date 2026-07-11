/**
 * @file test_q_math.cpp
 * @brief Q-format 定点数数学库白盒单元测试
 * @details 测试 qmath 命名空间下的自由函数、Q15_16 类、Q7_8 类的所有操作。
 *          覆盖转换、基本运算、统计运算、操作符重载、边界条件。
 */

// 在包含 feature_config.hpp 之前启用定点数学和测试框架
#define SENSOR_FEATURE_FIXED_POINT_MATH  1
#define SENSOR_FEATURE_TEST_FRAMEWORK    1

#include "../../test_framework/test_fixture.hpp"
#include "../../fixed_point/q_math.hpp"

// ============================================================
//  qmath 自由函数测试
// ============================================================

TEST(QMath, FromFloatZero) {
    q15_16_t v = qmath::fromFloat(0.0f);
    TEST_ASSERT_EQ(v, static_cast<q15_16_t>(0), "fromFloat(0.0f) should be 0");
    return true;
}

TEST(QMath, FromFloatOne) {
    q15_16_t v = qmath::fromFloat(1.0f);
    TEST_ASSERT_EQ(v, qmath::Q15_16_ONE, "fromFloat(1.0f) should be Q15_16_ONE (65536)");
    return true;
}

TEST(QMath, FromFloatHalf) {
    q15_16_t v = qmath::fromFloat(0.5f);
    TEST_ASSERT_EQ(v, qmath::Q15_16_HALF, "fromFloat(0.5f) should be Q15_16_HALF (32768)");
    return true;
}

TEST(QMath, FromFloatNegative) {
    q15_16_t v = qmath::fromFloat(-1.0f);
    TEST_ASSERT_EQ(v, static_cast<q15_16_t>(-65536), "fromFloat(-1.0f) should be -65536");
    return true;
}

TEST(QMath, ToFloatRoundTrip) {
    float values[] = {0.0f, 1.0f, -1.0f, 3.14159f, 2.71828f};
    const char* names[] = {"0", "1", "-1", "3.14159", "2.71828"};
    for (int i = 0; i < 5; ++i) {
        float result = qmath::toFloat(qmath::fromFloat(values[i]));
        TEST_ASSERT_FLOAT_EQ(result, values[i], 0.001f, names[i]);
    }
    return true;
}

TEST(QMath, FromInt) {
    q15_16_t v = qmath::fromInt(5);
    TEST_ASSERT_EQ(qmath::toInt(v), static_cast<int32_t>(5), "fromInt(5) >> 16 should give 5");
    TEST_ASSERT_FLOAT_EQ(qmath::toFloat(v), 5.0f, 0.001f, "toFloat(fromInt(5)) should be 5.0");
    return true;
}

TEST(QMath, ToInt) {
    q15_16_t v = qmath::fromFloat(3.7f);
    TEST_ASSERT_EQ(qmath::toInt(v), static_cast<int32_t>(3), "toInt(fromFloat(3.7f)) should give 3 (truncation)");
    return true;
}

TEST(QMath, Add) {
    q15_16_t a = qmath::fromFloat(1.5f);
    q15_16_t b = qmath::fromFloat(2.5f);
    float result = qmath::toFloat(qmath::add(a, b));
    TEST_ASSERT_FLOAT_EQ(result, 4.0f, 0.001f, "1.5 + 2.5 should be 4.0");
    return true;
}

TEST(QMath, Sub) {
    q15_16_t a = qmath::fromFloat(5.0f);
    q15_16_t b = qmath::fromFloat(2.0f);
    float result = qmath::toFloat(qmath::sub(a, b));
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "5.0 - 2.0 should be 3.0");
    return true;
}

TEST(QMath, Mul) {
    q15_16_t a = qmath::fromFloat(3.0f);
    q15_16_t b = qmath::fromFloat(2.0f);
    float result = qmath::toFloat(qmath::mul(a, b));
    TEST_ASSERT_FLOAT_EQ(result, 6.0f, 0.001f, "3.0 * 2.0 should be 6.0");
    return true;
}

TEST(QMath, MulFraction) {
    q15_16_t a = qmath::fromFloat(0.5f);
    q15_16_t b = qmath::fromFloat(0.5f);
    float result = qmath::toFloat(qmath::mul(a, b));
    TEST_ASSERT_FLOAT_EQ(result, 0.25f, 0.001f, "0.5 * 0.5 should be 0.25");
    return true;
}

TEST(QMath, Div) {
    q15_16_t a = qmath::fromFloat(6.0f);
    q15_16_t b = qmath::fromFloat(2.0f);
    float result = qmath::toFloat(qmath::div(a, b));
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "6.0 / 2.0 should be 3.0");
    return true;
}

TEST(QMath, DivByZero) {
    q15_16_t a = qmath::fromFloat(5.0f);
    q15_16_t result = qmath::div(a, 0);
    // 正数除以零应返回 MAX
    TEST_ASSERT_EQ(result, qmath::Q15_16_MAX, "div(positive, 0) should return Q15_16_MAX");

    // 负数除以零应返回 MIN
    q15_16_t neg_a = qmath::fromFloat(-5.0f);
    q15_16_t result2 = qmath::div(neg_a, 0);
    TEST_ASSERT_EQ(result2, qmath::Q15_16_MIN, "div(negative, 0) should return Q15_16_MIN");
    return true;
}

TEST(QMath, AbsPositive) {
    q15_16_t v = qmath::fromFloat(5.0f);
    float result = qmath::toFloat(qmath::abs(v));
    TEST_ASSERT_FLOAT_EQ(result, 5.0f, 0.001f, "abs(5.0) should be 5.0");
    return true;
}

TEST(QMath, AbsNegative) {
    q15_16_t v = qmath::fromFloat(-5.0f);
    float result = qmath::toFloat(qmath::abs(v));
    TEST_ASSERT_FLOAT_EQ(result, 5.0f, 0.001f, "abs(-5.0) should be 5.0");
    return true;
}

TEST(QMath, Mean) {
    q15_16_t data[4] = {
        qmath::fromFloat(1.0f),
        qmath::fromFloat(2.0f),
        qmath::fromFloat(3.0f),
        qmath::fromFloat(4.0f)
    };
    float result = qmath::toFloat(qmath::mean(data, 4));
    TEST_ASSERT_FLOAT_EQ(result, 2.5f, 0.001f, "mean of [1,2,3,4] should be 2.5");
    return true;
}

TEST(QMath, Median) {
    q15_16_t data[5] = {
        qmath::fromFloat(1.0f),
        qmath::fromFloat(5.0f),
        qmath::fromFloat(3.0f),
        qmath::fromFloat(2.0f),
        qmath::fromFloat(4.0f)
    };
    float result = qmath::toFloat(qmath::median(data, 5));
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "median of [1,5,3,2,4] should be 3.0");
    return true;
}

TEST(QMath, MedianEven) {
    q15_16_t data[4] = {
        qmath::fromFloat(1.0f),
        qmath::fromFloat(3.0f),
        qmath::fromFloat(2.0f),
        qmath::fromFloat(4.0f)
    };
    float result = qmath::toFloat(qmath::median(data, 4));
    TEST_ASSERT_FLOAT_EQ(result, 2.5f, 0.001f, "median of [1,3,2,4] should be 2.5");
    return true;
}

TEST(QMath, TrimmedMean) {
    q15_16_t data[5] = {
        qmath::fromFloat(0.0f),
        qmath::fromFloat(1.0f),
        qmath::fromFloat(2.0f),
        qmath::fromFloat(3.0f),
        qmath::fromFloat(100.0f)
    };
    float result = qmath::toFloat(qmath::trimmedMean(data, 5, 1));
    TEST_ASSERT_FLOAT_EQ(result, 2.0f, 0.001f, "trimmedMean trim=1 of [0,1,2,3,100] should be 2.0");
    return true;
}

TEST(QMath, Constants) {
    float pi = qmath::toFloat(qmath::Q15_16_PI);
    TEST_ASSERT_FLOAT_EQ(pi, 3.14159f, 0.001f, "Q15_16_PI should be approximately 3.14159");

    float e = qmath::toFloat(qmath::Q15_16_E);
    TEST_ASSERT_FLOAT_EQ(e, 2.71828f, 0.001f, "Q15_16_E should be approximately 2.71828");
    return true;
}

// ============================================================
//  Q15_16 类测试
// ============================================================

TEST(Q15_16Class, DefaultZero) {
    Q15_16 v;
    TEST_ASSERT_FLOAT_EQ(v.toFloat(), 0.0f, 0.001f, "Default-constructed Q15_16 should be 0");
    return true;
}

TEST(Q15_16Class, ConstructFromFloat) {
    Q15_16 v(1.5f);
    TEST_ASSERT_FLOAT_EQ(v.toFloat(), 1.5f, 0.001f, "Q15_16(1.5f) should be approximately 1.5");
    return true;
}

TEST(Q15_16Class, AddOperator) {
    Q15_16 a(1.0f);
    Q15_16 b(2.0f);
    float result = a + b;
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "Q15_16(1) + Q15_16(2) should be 3.0");
    return true;
}

TEST(Q15_16Class, SubOperator) {
    Q15_16 a(5.0f);
    Q15_16 b(2.0f);
    float result = a - b;
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "Q15_16(5) - Q15_16(2) should be 3.0");
    return true;
}

TEST(Q15_16Class, MulOperator) {
    Q15_16 a(3.0f);
    Q15_16 b(2.0f);
    float result = a * b;
    TEST_ASSERT_FLOAT_EQ(result, 6.0f, 0.001f, "Q15_16(3) * Q15_16(2) should be 6.0");
    return true;
}

TEST(Q15_16Class, DivOperator) {
    Q15_16 a(6.0f);
    Q15_16 b(2.0f);
    float result = a / b;
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.001f, "Q15_16(6) / Q15_16(2) should be 3.0");
    return true;
}

TEST(Q15_16Class, NegateOperator) {
    Q15_16 v(3.0f);
    float result = -v;
    TEST_ASSERT_FLOAT_EQ(result, -3.0f, 0.001f, "-Q15_16(3.0f) should be -3.0");
    return true;
}

TEST(Q15_16Class, Comparison) {
    Q15_16 one(1.0f);
    Q15_16 two(2.0f);
    Q15_16 also_one(1.0f);

    TEST_ASSERT_TRUE(one < two, "Q15_16(1) < Q15_16(2) should be true");
    TEST_ASSERT_TRUE(two > one, "Q15_16(2) > Q15_16(1) should be true");
    TEST_ASSERT_TRUE(one <= two, "Q15_16(1) <= Q15_16(2) should be true");
    TEST_ASSERT_TRUE(two >= one, "Q15_16(2) >= Q15_16(1) should be true");
    TEST_ASSERT_TRUE(one <= also_one, "Q15_16(1) <= Q15_16(1) should be true");
    TEST_ASSERT_TRUE(one >= also_one, "Q15_16(1) >= Q15_16(1) should be true");
    return true;
}

TEST(Q15_16Class, Equality) {
    Q15_16 a(1.5f);
    Q15_16 b(1.5f);
    Q15_16 c(2.0f);

    TEST_ASSERT_TRUE(a == b, "Q15_16(1.5f) == Q15_16(1.5f) should be true");
    TEST_ASSERT_FALSE(a == c, "Q15_16(1.5f) == Q15_16(2.0f) should be false");
    TEST_ASSERT_TRUE(a != c, "Q15_16(1.5f) != Q15_16(2.0f) should be true");
    TEST_ASSERT_FALSE(a != b, "Q15_16(1.5f) != Q15_16(1.5f) should be false");
    return true;
}

// ============================================================
//  Q7_8 类测试
// ============================================================

TEST(Q7_8Class, ConstructFromFloat) {
    Q7_8 v(1.5f);
    // Q7.8 精度约为 0.004，使用 0.01 的 epsilon
    TEST_ASSERT_FLOAT_EQ(v.toFloat(), 1.5f, 0.01f, "Q7_8(1.5f) should be approximately 1.5");
    return true;
}

TEST(Q7_8Class, AddOperator) {
    Q7_8 a(1.0f);
    Q7_8 b(2.0f);
    float result = (a + b).toFloat();
    TEST_ASSERT_FLOAT_EQ(result, 3.0f, 0.01f, "Q7_8(1.0) + Q7_8(2.0) should be 3.0");
    return true;
}

TEST(Q7_8Class, MulOperator) {
    Q7_8 a(2.0f);
    Q7_8 b(3.0f);
    float result = (a * b).toFloat();
    TEST_ASSERT_FLOAT_EQ(result, 6.0f, 0.01f, "Q7_8(2.0) * Q7_8(3.0) should be 6.0");
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
