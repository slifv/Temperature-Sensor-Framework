/**
 * @file ring_buffer.hpp
 * @brief 静态环形缓冲区 — 固定容量FIFO队列
 * @details C++11兼容，所有内存在编译期静态分配。
 *          适用于嵌入式平台（无动态内存分配）。
 *          作为滑动窗口滤波器、异常日志等模块的底层数据结构。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * @class RingBuffer
 * @brief 固定容量环形缓冲区（静态分配）
 * @tparam T 存储元素类型
 * @tparam Capacity 最大容量（编译期常量）
 */
template <typename T, uint8_t Capacity>
class RingBuffer {
    static_assert(Capacity >= 2, "RingBuffer capacity must be >= 2");

public:
    typedef T           value_type;
    typedef T&          reference;
    typedef const T&    const_reference;
    typedef uint8_t     size_type;  ///< 8位索引，适配小容量

    RingBuffer()
        : head_(0), tail_(0), full_(false)
    {}

    /**
     * @brief 向缓冲区尾部推入一个元素
     * @details 若缓冲区已满，自动覆盖最旧元素
     */
    void push(const T& item) {
        buffer_[tail_] = item;
        tail_ = static_cast<size_type>((tail_ + 1) % Capacity);

        if (full_) {
            head_ = static_cast<size_type>((head_ + 1) % Capacity);
        }

        if (tail_ == head_) {
            full_ = true;
        }
    }

    /**
     * @brief 从缓冲区头部弹出最旧元素
     * @return 弹出的元素（按值返回）
     * @note 调用前需确保非空（is_empty() == false）
     */
    T pop() {
        T item = buffer_[head_];
        full_ = false;
        head_ = static_cast<size_type>((head_ + 1) % Capacity);
        return item;
    }

    /// 查看头部元素（不弹出）
    const T& peek() const {
        return buffer_[head_];
    }

    /// 查看尾部（最新）元素
    const T& newest() const {
        if (tail_ == 0) {
            return buffer_[Capacity - 1];
        }
        return buffer_[tail_ - 1];
    }

    /// 按逻辑索引访问（0 = 最旧，size()-1 = 最新）
    const T& operator[](size_type index) const {
        return buffer_[(head_ + index) % Capacity];
    }

    T& operator[](size_type index) {
        return buffer_[(head_ + index) % Capacity];
    }

    /// 当前缓冲区中元素数量
    size_type size() const {
        if (full_) return Capacity;
        if (tail_ >= head_) return static_cast<size_type>(tail_ - head_);
        return static_cast<size_type>(tail_ + Capacity - head_);
    }

    /// 是否为空
    bool is_empty() const {
        return (!full_) && (head_ == tail_);
    }

    /// 是否已满
    bool is_full() const {
        return full_;
    }

    /// 缓冲区是否已填满（窗口就绪）
    bool is_ready() const {
        return full_;
    }

    /// 最大容量
    static uint8_t capacity() { return Capacity; }

    /// 清空缓冲区
    void clear() {
        head_ = 0;
        tail_ = 0;
        full_ = false;
    }

    /// 重置缓冲区（清空 + 填充指定值）
    void reset(const T& defaultValue = T()) {
        for (size_type i = 0; i < Capacity; ++i) {
            buffer_[i] = defaultValue;
        }
        head_ = 0;
        tail_ = 0;
        full_ = false;
    }

    /// 获取原始数据指针（连续块需要特殊处理）
    const T* data() const { return buffer_; }

    /// 复制全部有效数据到外部数组（按时间顺序：最旧→最新）
    void copyTo(T* out, size_type maxCount) const {
        size_type count = (maxCount < size()) ? maxCount : size();
        for (size_type i = 0; i < count; ++i) {
            out[i] = (*this)[i];
        }
    }

private:
    T           buffer_[Capacity];
    size_type   head_;
    size_type   tail_;
    bool        full_;
};
