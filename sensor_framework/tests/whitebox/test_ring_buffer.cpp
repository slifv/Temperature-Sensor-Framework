/**
 * @file test_ring_buffer.cpp
 * @brief 环形缓冲区白盒单元测试
 * @details 测试 RingBuffer 的所有接口：push/pop/peek/newest/operator[]/
 *          size/is_empty/is_full/is_ready/capacity/clear/reset/data/copyTo
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../utils/ring_buffer.hpp"

// ============================================================
//  环形缓冲区基础功能测试
// ============================================================

TEST(RingBuffer, EmptyOnInit) {
    RingBuffer<int, 4> buf;

    TEST_ASSERT_EQ(buf.size(), (uint8_t)0, "Size should be 0 on init");
    TEST_ASSERT_TRUE(buf.is_empty(), "Buffer should be empty on init");
    TEST_ASSERT_FALSE(buf.is_full(), "Buffer should not be full on init");
    TEST_ASSERT_FALSE(buf.is_ready(), "Buffer should not be ready on init");
    return true;
}

TEST(RingBuffer, PushAndSize) {
    RingBuffer<int, 4> buf;

    buf.push(10);
    TEST_ASSERT_EQ(buf.size(), (uint8_t)1, "Size should be 1 after 1 push");

    buf.push(20);
    buf.push(30);
    TEST_ASSERT_EQ(buf.size(), (uint8_t)3, "Size should be 3 after 3 pushes");
    TEST_ASSERT_FALSE(buf.is_full(), "Should not be full at 3/4");

    return true;
}

TEST(RingBuffer, PeekOldest) {
    RingBuffer<int, 4> buf;

    buf.push(100);
    buf.push(200);
    buf.push(300);

    TEST_ASSERT_EQ(buf.peek(), 100, "peek() should return first pushed element (100)");

    // peek() 不应移除元素
    TEST_ASSERT_EQ(buf.size(), (uint8_t)3, "Size should still be 3 after peek");
    TEST_ASSERT_EQ(buf.peek(), 100, "peek() should still return 100");

    return true;
}

TEST(RingBuffer, NewestElement) {
    RingBuffer<int, 4> buf;

    buf.push(11);
    buf.push(22);
    buf.push(33);

    TEST_ASSERT_EQ(buf.newest(), 33, "newest() should return last pushed element (33)");

    buf.push(44);
    TEST_ASSERT_EQ(buf.newest(), 44, "newest() should update after another push");

    return true;
}

TEST(RingBuffer, PopRemovesOldest) {
    RingBuffer<int, 4> buf;

    buf.push(10);
    buf.push(20);
    buf.push(30);

    int val = buf.pop();
    TEST_ASSERT_EQ(val, 10, "pop() should return oldest element (10)");
    TEST_ASSERT_EQ(buf.size(), (uint8_t)2, "Size should be 2 after pop");

    val = buf.pop();
    TEST_ASSERT_EQ(val, 20, "Second pop() should return 20");
    TEST_ASSERT_EQ(buf.size(), (uint8_t)1, "Size should be 1 after 2 pops");

    val = buf.pop();
    TEST_ASSERT_EQ(val, 30, "Third pop() should return 30");
    TEST_ASSERT_TRUE(buf.is_empty(), "Buffer should be empty after 3 pops");

    return true;
}

TEST(RingBuffer, FullDetection) {
    RingBuffer<int, 4> buf;

    buf.push(1);
    buf.push(2);
    buf.push(3);
    TEST_ASSERT_FALSE(buf.is_full(), "Should not be full at 3/4");

    buf.push(4);
    TEST_ASSERT_TRUE(buf.is_full(), "Should be full after 4 pushes");
    TEST_ASSERT_TRUE(buf.is_ready(), "Should be ready when full");
    TEST_ASSERT_EQ(buf.size(), (uint8_t)4, "Size should be 4 when full");

    return true;
}

TEST(RingBuffer, CircularWrap) {
    RingBuffer<int, 4> buf;

    // 填充到满: [1, 2, 3, 4]
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.push(4);
    TEST_ASSERT_TRUE(buf.is_full(), "Should be full at 4/4");

    // 再推入2个，覆盖最旧的: [5, 6, 3, 4]（逻辑顺序: 3, 4, 5, 6）
    buf.push(5);
    buf.push(6);

    TEST_ASSERT_EQ(buf.size(), (uint8_t)4, "Size should still be 4 after wrap");
    TEST_ASSERT_TRUE(buf.is_full(), "Should remain full after wrap");

    // 弹出验证: 最旧的应为被覆盖后剩余的最旧值
    // 逻辑索引0=最旧: 3, 1=4, 2=5, 3=6
    TEST_ASSERT_EQ(buf.pop(), 3, "After wrap, oldest should be 3");
    TEST_ASSERT_EQ(buf.pop(), 4, "Next should be 4");
    TEST_ASSERT_EQ(buf.pop(), 5, "Next should be 5");
    TEST_ASSERT_EQ(buf.pop(), 6, "Next should be 6");
    TEST_ASSERT_TRUE(buf.is_empty(), "Should be empty after popping all");

    return true;
}

TEST(RingBuffer, RandomAccess) {
    RingBuffer<int, 4> buf;

    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.push(40);

    // operator[] 索引从最旧(0)到最新(size-1)
    TEST_ASSERT_EQ(buf[0], 10, "Index 0 should be oldest (10)");
    TEST_ASSERT_EQ(buf[1], 20, "Index 1 should be 20");
    TEST_ASSERT_EQ(buf[2], 30, "Index 2 should be 30");
    TEST_ASSERT_EQ(buf[3], 40, "Index 3 should be newest (40)");

    return true;
}

TEST(RingBuffer, ClearEmpties) {
    RingBuffer<int, 4> buf;

    buf.push(1);
    buf.push(2);
    buf.push(3);
    TEST_ASSERT_EQ(buf.size(), (uint8_t)3, "Should have 3 elements before clear");

    buf.clear();

    TEST_ASSERT_EQ(buf.size(), (uint8_t)0, "Size should be 0 after clear");
    TEST_ASSERT_TRUE(buf.is_empty(), "Should be empty after clear");
    TEST_ASSERT_FALSE(buf.is_full(), "Should not be full after clear");
    TEST_ASSERT_FALSE(buf.is_ready(), "Should not be ready after clear");

    return true;
}

TEST(RingBuffer, ResetWithDefault) {
    RingBuffer<int, 4> buf;

    buf.reset(42);

    // 验证所有元素被填充为默认值
    const int* ptr = buf.data();
    for (uint8_t i = 0; i < 4; ++i) {
        TEST_ASSERT_EQ(ptr[i], 42, "All elements should be 42 after reset(42)");
    }

    // reset() 清空缓冲区状态（head=tail=0, full=false）
    TEST_ASSERT_EQ(buf.size(), (uint8_t)0, "Size should be 0 after reset");
    TEST_ASSERT_TRUE(buf.is_empty(), "Should be empty after reset");
    TEST_ASSERT_FALSE(buf.is_ready(), "Should not be ready after reset");

    return true;
}

TEST(RingBuffer, CopyTo) {
    RingBuffer<int, 4> buf;

    buf.push(10);
    buf.push(20);
    buf.push(30);

    int out[4] = {0, 0, 0, 0};

    // 复制最多 2 个
    buf.copyTo(out, 2);
    TEST_ASSERT_EQ(out[0], 10, "copyTo[0] should be 10");
    TEST_ASSERT_EQ(out[1], 20, "copyTo[1] should be 20");
    TEST_ASSERT_EQ(out[2], 0,  "copyTo[2] should be untouched (0)");

    // 复制全部
    int out2[4] = {0, 0, 0, 0};
    buf.copyTo(out2, 10);  // maxCount > size
    TEST_ASSERT_EQ(out2[0], 10, "copyTo all: [0] should be 10");
    TEST_ASSERT_EQ(out2[1], 20, "copyTo all: [1] should be 20");
    TEST_ASSERT_EQ(out2[2], 30, "copyTo all: [2] should be 30");
    TEST_ASSERT_EQ(out2[3], 0,  "copyTo all: [3] should be untouched (0)");

    return true;
}

TEST(RingBuffer, MinimumCapacity) {
    // static_assert(Capacity >= 2) — 测试最小容量边角情况
    RingBuffer<int, 2> buf;

    TEST_ASSERT_TRUE(buf.is_empty(), "Min cap: should be empty on init");
    TEST_ASSERT_EQ(buf.size(), (uint8_t)0, "Min cap: size should be 0 on init");

    buf.push(1);
    buf.push(2);
    TEST_ASSERT_TRUE(buf.is_full(), "Min cap: should be full after 2 pushes");
    TEST_ASSERT_EQ(buf.size(), (uint8_t)2, "Min cap: size should be 2 when full");

    // 覆盖：推入第3个，最旧的1被覆盖
    buf.push(3);
    TEST_ASSERT_EQ(buf.size(), (uint8_t)2, "Min cap: size should still be 2 after wrap");
    TEST_ASSERT_EQ(buf.peek(), 2, "Min cap: after wrap, oldest should be 2");
    TEST_ASSERT_EQ(buf.newest(), 3, "Min cap: newest should be 3");

    // 按顺序弹出
    TEST_ASSERT_EQ(buf.pop(), 2, "Min cap: pop should return 2");
    TEST_ASSERT_EQ(buf.pop(), 3, "Min cap: pop should return 3");
    TEST_ASSERT_TRUE(buf.is_empty(), "Min cap: should be empty after popping all");

    // 单元素操作
    buf.push(99);
    TEST_ASSERT_EQ(buf.pop(), 99, "Min cap: single push/pop should work");

    return true;
}

TEST(RingBuffer, DataPointer) {
    RingBuffer<int, 4> buf;

    buf.push(10);
    buf.push(20);

    const int* ptr = buf.data();
    TEST_ASSERT_TRUE(ptr != ((const int*)0), "data() should return non-null pointer");

    // buffer_ 是内部物理存储，物理索引0和1处应有我们推入的值
    // （在未发生环绕时，物理顺序 = 逻辑顺序）
    TEST_ASSERT_EQ(ptr[0], 10, "Physical slot 0 should be 10 (first pushed)");
    TEST_ASSERT_EQ(ptr[1], 20, "Physical slot 1 should be 20 (second pushed)");

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
