/**
 * @file test_static_vector.cpp
 * @brief StaticVector 白盒单元测试
 * @details 测试所有 StaticVector<T, Capacity> 的操作：
 *          基本操作、边界条件、迭代器、非平凡类型、指针存储
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../utils/static_vector.hpp"

// ============================================================
//  辅助类型定义
// ============================================================

/// 带析构计数的辅助结构体，用于测试非平凡类型的构造/析构
struct Counter {
    static int alive;
    int value;

    Counter(int v = 0) : value(v) { ++alive; }
    Counter(const Counter& other) : value(other.value) { ++alive; }
    ~Counter() { --alive; }

    bool operator==(const Counter& other) const { return value == other.value; }
};
int Counter::alive = 0;

// ============================================================
//  基本操作测试
// ============================================================

TEST(StaticVector, EmptyOnInit) {
    StaticVector<int, 8> vec;
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(0), "Size should be 0 on init");
    TEST_ASSERT_TRUE(vec.is_empty(), "Should be empty on init");
    TEST_ASSERT_FALSE(vec.is_full(), "Should not be full on init");
    return true;
}

TEST(StaticVector, PushBack) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(3), "Size should be 3 after 3 pushes");
    TEST_ASSERT_EQ(vec[0], 10, "vec[0] should be 10");
    TEST_ASSERT_EQ(vec[1], 20, "vec[1] should be 20");
    TEST_ASSERT_EQ(vec[2], 30, "vec[2] should be 30");
    return true;
}

TEST(StaticVector, PushBackReturnsTrue) {
    StaticVector<int, 8> vec;
    bool result = vec.push_back(42);
    TEST_ASSERT_TRUE(result, "push_back should return true on success");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(1), "Size should be 1");
    return true;
}

TEST(StaticVector, FrontBack) {
    StaticVector<int, 8> vec;
    vec.push_back(100);
    vec.push_back(200);
    vec.push_back(300);

    TEST_ASSERT_EQ(vec.front(), 100, "front() should be first element");
    TEST_ASSERT_EQ(vec.back(), 300, "back() should be last element");
    return true;
}

TEST(StaticVector, PopBack) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    vec.pop_back();
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(2), "Size should be 2 after pop");
    TEST_ASSERT_EQ(vec.back(), 20, "back() should now be 20");
    return true;
}

TEST(StaticVector, FullCapacity) {
    StaticVector<int, 4> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    TEST_ASSERT_FALSE(vec.is_full(), "Should not be full yet");

    vec.push_back(4);
    TEST_ASSERT_TRUE(vec.is_full(), "Should be full at capacity");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(4), "Size should equal capacity");
    return true;
}

TEST(StaticVector, PushBackWhenFull) {
    StaticVector<int, 4> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    TEST_ASSERT_TRUE(vec.is_full(), "Should be full");

    bool result = vec.push_back(5);
    TEST_ASSERT_FALSE(result, "push_back should return false when full");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(4), "Size should remain 4");
    TEST_ASSERT_EQ(vec[3], 4, "Last element should still be 4");
    return true;
}

TEST(StaticVector, Clear) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    vec.clear();
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(0), "Size should be 0 after clear");
    TEST_ASSERT_TRUE(vec.is_empty(), "Should be empty after clear");
    return true;
}

// ============================================================
//  查找与移除测试
// ============================================================

TEST(StaticVector, IndexOf) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    // 存在的值
    TEST_ASSERT_EQ(vec.indexOf(20), static_cast<uint8_t>(1), "indexOf(20) should return 1");

    // 不存在的值 → 返回 Capacity 作为哨兵
    TEST_ASSERT_EQ(vec.indexOf(99), static_cast<uint8_t>(8), "indexOf(99) should return Capacity (8)");

    return true;
}

TEST(StaticVector, RemoveAtFront) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    bool result = vec.remove(0);
    TEST_ASSERT_TRUE(result, "remove(0) should return true");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(2), "Size should be 2 after remove");
    // 后续元素应前移
    TEST_ASSERT_EQ(vec[0], 20, "vec[0] should now be 20 (shifted)");
    TEST_ASSERT_EQ(vec[1], 30, "vec[1] should now be 30 (shifted)");
    return true;
}

TEST(StaticVector, RemoveAtMiddle) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    bool result = vec.remove(1);
    TEST_ASSERT_TRUE(result, "remove(1) should return true");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(2), "Size should be 2");
    TEST_ASSERT_EQ(vec[0], 10, "vec[0] should still be 10");
    TEST_ASSERT_EQ(vec[1], 30, "vec[1] should be 30 (was shifted from index 2)");
    return true;
}

TEST(StaticVector, RemoveAtEnd) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    bool result = vec.remove(2); // 移除最后一个
    TEST_ASSERT_TRUE(result, "remove(2) should return true");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(2), "Size should be 2");
    TEST_ASSERT_EQ(vec[0], 10, "vec[0] should still be 10");
    TEST_ASSERT_EQ(vec[1], 20, "vec[1] should still be 20");
    return true;
}

TEST(StaticVector, RemoveAtIndexOutOfBounds) {
    StaticVector<int, 8> vec;
    vec.push_back(10);

    // 移除索引 == size
    bool result = vec.remove(1);
    TEST_ASSERT_FALSE(result, "remove(1) on size=1 vec should return false");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(1), "Size should be unchanged");
    TEST_ASSERT_EQ(vec[0], 10, "Element should be intact");
    return true;
}

TEST(StaticVector, RemoveValue) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    vec.push_back(20); // 重复值

    bool result = vec.removeValue(20);
    TEST_ASSERT_TRUE(result, "removeValue(20) should return true (found and removed)");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(3), "Size should be 3");
    // 应移除第一个匹配的20
    TEST_ASSERT_EQ(vec[0], 10, "vec[0] should be 10");
    TEST_ASSERT_EQ(vec[1], 30, "vec[1] should be 30 (shifted)");
    TEST_ASSERT_EQ(vec[2], 20, "vec[2] should be 20 (shifted from index 3)");

    // 移除不存在的值
    bool result2 = vec.removeValue(99);
    TEST_ASSERT_FALSE(result2, "removeValue(99) should return false (not found)");
    return true;
}

// ============================================================
//  迭代器测试
// ============================================================

TEST(StaticVector, IteratorTraversal) {
    StaticVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    int expected[] = {10, 20, 30};
    int i = 0;
    for (StaticVector<int, 8>::iterator it = vec.begin(); it != vec.end(); ++it) {
        TEST_ASSERT_EQ(*it, expected[i], "Iterator value mismatch");
        ++i;
    }
    TEST_ASSERT_EQ(i, 3, "Should have iterated 3 elements");
    return true;
}

// ============================================================
//  边界条件测试
// ============================================================

TEST(StaticVector, Capacity1) {
    StaticVector<int, 1> vec;

    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(0), "Capacity-1: size should be 0");
    TEST_ASSERT_TRUE(vec.is_empty(), "Capacity-1: should be empty");
    TEST_ASSERT_FALSE(vec.is_full(), "Capacity-1: should not be full");

    bool pushed = vec.push_back(42);
    TEST_ASSERT_TRUE(pushed, "Capacity-1: push_back should succeed");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(1), "Capacity-1: size should be 1");
    TEST_ASSERT_TRUE(vec.is_full(), "Capacity-1: should be full after 1 push");
    TEST_ASSERT_EQ(vec.front(), 42, "Capacity-1: front should be 42");
    TEST_ASSERT_EQ(vec.back(), 42, "Capacity-1: back should be 42");

    // push_back 在满时失败
    bool pushed2 = vec.push_back(99);
    TEST_ASSERT_FALSE(pushed2, "Capacity-1: push when full should return false");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(1), "Capacity-1: size should stay 1");

    // pop 后再 push
    vec.pop_back();
    TEST_ASSERT_TRUE(vec.is_empty(), "Capacity-1: should be empty after pop");
    vec.push_back(77);
    TEST_ASSERT_EQ(vec[0], 77, "Capacity-1: element should be 77 after re-push");

    return true;
}

// ============================================================
//  非平凡类型测试
// ============================================================

TEST(StaticVector, NonTrivialType) {
    Counter::alive = 0;

    {
        StaticVector<Counter, 4> vec;

        // push 3 个 Counter
        vec.push_back(Counter(10));
        vec.push_back(Counter(20));
        vec.push_back(Counter(30));

        TEST_ASSERT_EQ(Counter::alive, 3, "Should have 3 alive Counters");
        TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(3), "Size should be 3");
        TEST_ASSERT_EQ(vec[0].value, 10, "First Counter value should be 10");
        TEST_ASSERT_EQ(vec[1].value, 20, "Second Counter value should be 20");

        // pop_back 触发析构
        vec.pop_back();
        TEST_ASSERT_EQ(Counter::alive, 2, "Should have 2 alive after pop_back");

        // remove 触发析构 + 拷贝构造
        vec.remove(0);
        TEST_ASSERT_EQ(Counter::alive, 1, "Should have 1 alive after remove(0)");

        // clear 析构所有剩余元素
        vec.clear();
        TEST_ASSERT_EQ(Counter::alive, 0, "Should have 0 alive after clear");
    }

    // 离开作用域后确保无泄漏
    TEST_ASSERT_EQ(Counter::alive, 0, "No Counter should be alive after vector destruction");
    return true;
}

// ============================================================
//  指针存储测试
// ============================================================

TEST(StaticVector, PointerStorage) {
    int a = 10, b = 20, c = 30;

    StaticVector<int*, 4> vec;
    vec.push_back(&a);
    vec.push_back(&b);
    vec.push_back(&c);

    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(3), "Size should be 3");
    TEST_ASSERT_EQ(*vec[0], 10, "Dereferenced pointer at [0] should be 10");
    TEST_ASSERT_EQ(*vec[1], 20, "Dereferenced pointer at [1] should be 20");
    TEST_ASSERT_EQ(*vec[2], 30, "Dereferenced pointer at [2] should be 30");

    // 修改原变量，通过 vector 验证
    a = 100;
    TEST_ASSERT_EQ(*vec[0], 100, "Pointer should reflect external change");

    // removeValue 通过指针比较
    bool removed = vec.removeValue(&b);
    TEST_ASSERT_TRUE(removed, "removeValue should find matching pointer");
    TEST_ASSERT_EQ(vec.size(), static_cast<uint8_t>(2), "Size should be 2 after removeValue");

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
