/**
 * @file static_vector.hpp
 * @brief 静态分配容器 — 替代 std::vector 的嵌入式安全方案
 * @details 所有内存在编译期静态分配，无堆分配。
 *          C++11兼容。模板参数使用 uint8_t 适配8/16位MCU。
 *          适用于传感器注册表、订阅者列表等场景。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

/**
 * @class StaticVector
 * @brief 固定编译期容量的连续容器
 * @tparam T 存储元素类型
 * @tparam Capacity 最大容量
 */
template <typename T, uint8_t Capacity>
class StaticVector {
    static_assert(Capacity > 0, "StaticVector capacity must be > 0");

public:
    typedef T           value_type;
    typedef T*          iterator;
    typedef const T*    const_iterator;
    typedef uint8_t     size_type;

    StaticVector() : size_(0) {}

    // ─── 元素访问 ────────────────────────────────────────

    T& operator[](size_type index) {
        return reinterpret_cast<T&>(storage_[index]);
    }

    const T& operator[](size_type index) const {
        return reinterpret_cast<const T&>(storage_[index]);
    }

    T& front() { return (*this)[0]; }
    const T& front() const { return (*this)[0]; }

    T& back() { return (*this)[size_ - 1]; }
    const T& back() const { return (*this)[size_ - 1]; }

    T* data() {
        return reinterpret_cast<T*>(storage_);
    }

    const T* data() const {
        return reinterpret_cast<const T*>(storage_);
    }

    // ─── 迭代器 ──────────────────────────────────────────

    iterator begin() { return data(); }
    const_iterator begin() const { return data(); }
    iterator end() { return data() + size_; }
    const_iterator end() const { return data() + size_; }

    // ─── 容量 ────────────────────────────────────────────

    size_type size() const { return size_; }
    static uint8_t capacity() { return Capacity; }
    bool is_empty() const { return size_ == 0; }
    bool is_full() const { return size_ >= Capacity; }

    // ─── 修改操作 ────────────────────────────────────────

    /**
     * @brief 在尾部添加元素（如已满则忽略）
     * @return 添加成功返回 true，已满返回 false
     */
    bool push_back(const T& value) {
        if (is_full()) return false;
        new (&storage_[size_]) T(value);
        ++size_;
        return true;
    }

    /**
     * @brief 移除尾部元素
     */
    void pop_back() {
        if (size_ > 0) {
            --size_;
            reinterpret_cast<T&>(storage_[size_]).~T();
        }
    }

    /**
     * @brief 按索引移除元素（后续元素前移）
     * @param index 要移除的元素索引
     * @return 成功返回 true
     */
    bool remove(size_type index) {
        if (index >= size_) return false;

        reinterpret_cast<T&>(storage_[index]).~T();

        // 前移后续元素
        for (size_type i = index; i < size_ - 1; ++i) {
            new (&storage_[i]) T(reinterpret_cast<T&>(storage_[i + 1]));
            reinterpret_cast<T&>(storage_[i + 1]).~T();
        }

        --size_;
        return true;
    }

    /**
     * @brief 按值查找并移除第一个匹配元素
     * @return 找到并移除返回 true
     */
    bool removeValue(const T& value) {
        for (size_type i = 0; i < size_; ++i) {
            if ((*this)[i] == value) {
                return remove(i);
            }
        }
        return false;
    }

    /**
     * @brief 查找元素索引
     * @return 找到返回索引，未找到返回 Capacity
     */
    size_type indexOf(const T& value) const {
        for (size_type i = 0; i < size_; ++i) {
            if ((*this)[i] == value) {
                return i;
            }
        }
        return Capacity;  // 哨兵值
    }

    /// 清空所有元素
    void clear() {
        for (size_type i = 0; i < size_; ++i) {
            reinterpret_cast<T&>(storage_[i]).~T();
        }
        size_ = 0;
    }

private:
    // 使用 aligned storage 适配非平凡类型
    typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type
        storage_[Capacity];
    size_type size_;
};
