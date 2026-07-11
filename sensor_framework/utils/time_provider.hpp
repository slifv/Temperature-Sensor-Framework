/**
 * @file time_provider.hpp
 * @brief 时间提供者抽象接口
 * @details 抽象系统时间源，供传感器采样打时间戳使用。
 *          支持 ms 和 us 精度，以及延时操作。
 *          不同平台通过实现 ITimeProvider 接口适配。
 */

#pragma once

#include <cstdint>

/**
 * @class ITimeProvider
 * @brief 时间提供者抽象接口
 */
class ITimeProvider {
public:
    virtual ~ITimeProvider() {}

    /// 获取系统滴答（毫秒）
    virtual uint32_t getTickMs() = 0;

    /// 获取系统滴答（微秒）
    virtual uint64_t getTickUs() = 0;

    /// 毫秒级延时（阻塞）
    virtual void delayMs(uint32_t ms) = 0;

    /// 微秒级延时（阻塞）
    virtual void delayUs(uint32_t us) = 0;
};

/**
 * @class DefaultTimeProvider
 * @brief 默认时间提供者（PC端使用 std::chrono）
 * @note 嵌入式平台应替换为对应HAL实现
 */
#if !defined(SENSOR_MCU_8BIT) && !defined(SENSOR_MCU_16BIT)
#include <chrono>
#include <thread>

class DefaultTimeProvider : public ITimeProvider {
public:
    uint32_t getTickMs() SENSOR_OVERRIDE {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
    }

    uint64_t getTickUs() SENSOR_OVERRIDE {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()
            ).count()
        );
    }

    void delayMs(uint32_t ms) SENSOR_OVERRIDE {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void delayUs(uint32_t us) SENSOR_OVERRIDE {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
};
#endif
