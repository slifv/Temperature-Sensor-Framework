/**
 * @file exception_logger.hpp
 * @brief 异常日志记录 — 环形缓冲区，内存安全
 * @details 存储最近N条异常事件，支持按严重级别过滤查询。
 *          参考: 需求评审报告 第16.7节
 */

#pragma once

#include "exception_types.hpp"

#if SENSOR_FEATURE_EXCEPTION_LOGGER

#include <cstdint>

/**
 * @class ExceptionLogger
 * @brief 异常日志记录器（环形缓冲区）
 * @tparam MaxEntries 最大日志条目数（典型值: 32/64）
 */
template <uint8_t MaxEntries = 64>
class ExceptionLogger {
public:
    ExceptionLogger()
        : writeIndex_(0), totalCount_(0)
    {}

    /// 记录异常事件
    void log(const ExceptionEvent& event) {
        buffer_[writeIndex_ % MaxEntries] = event;
        ++writeIndex_;
        ++totalCount_;
    }

    /// 获取第N条日志（0 = 最新）
    const ExceptionEvent& get(uint8_t index) const {
        if (totalCount_ == 0 || index >= MaxEntries) {
            return invalidEvent_;
        }
        uint8_t actualIndex = (writeIndex_ - 1 - index) % MaxEntries;
        return buffer_[actualIndex];
    }

    /// 获取最近一条日志
    const ExceptionEvent& last() const {
        if (totalCount_ == 0) return invalidEvent_;
        return buffer_[(writeIndex_ - 1) % MaxEntries];
    }

    /// 获取最近N条日志（按发生时间倒序）
    uint8_t getRecent(ExceptionEvent* out, uint8_t maxCount,
                      ExceptionSeverity minSeverity = ExceptionSeverity::INFO) const
    {
        uint8_t count = 0;
        uint8_t available = (totalCount_ < MaxEntries) ? totalCount_ : MaxEntries;
        uint8_t start = writeIndex_;

        for (uint8_t i = 0; i < available && count < maxCount; ++i) {
            uint8_t idx = (start - 1 - i) % MaxEntries;
            if (buffer_[idx].severity >= minSeverity) {
                out[count++] = buffer_[idx];
            }
        }
        return count;
    }

    /// 统计各级别异常数量
    void getStats(uint32_t counts[4]) const {
        for (uint8_t i = 0; i < 4; ++i) counts[i] = 0;

        uint8_t available = (totalCount_ < MaxEntries) ? totalCount_ : MaxEntries;
        for (uint8_t i = 0; i < available; ++i) {
            uint8_t idx = (writeIndex_ - 1 - i) % MaxEntries;
            uint8_t sev = static_cast<uint8_t>(buffer_[idx].severity);
            if (sev < 4) {
                counts[sev]++;
            }
        }
    }

    /// 异常总数（含已覆盖的旧日志）
    uint32_t getTotalCount() const { return totalCount_; }

    /// 当前缓冲区中的条目数
    uint8_t getCount() const {
        return (totalCount_ < MaxEntries) ? totalCount_ : MaxEntries;
    }

    /// 清空日志
    void clear() {
        writeIndex_ = 0;
        totalCount_ = 0;
    }

private:
    ExceptionEvent   buffer_[MaxEntries];
    uint8_t          writeIndex_;
    uint32_t         totalCount_;
    ExceptionEvent   invalidEvent_;  // 空日志时的哨兵
};

#endif // SENSOR_FEATURE_EXCEPTION_LOGGER
