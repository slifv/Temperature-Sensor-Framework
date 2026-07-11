/**
 * @file sensor_base.hpp
 * @brief 传感器抽象基类 — 模板方法模式，定义通用采集流程
 * @details 所有传感器必须继承 SensorBase<TData> 并实现硬件相关细节。
 *          通过模板参数TData区分不同传感器数据类型。
 *          参考: 需求评审报告 第9.1节
 */

#pragma once

#include "../feature_config.hpp"
#include "../compiler_features.hpp"
#include "../filter/filter_interface.hpp"
#include "../filter/filter_pipeline.hpp"
#include "sensor_config.hpp"

#if SENSOR_FEATURE_FILTER_PIPELINE
#include <memory>
#endif

/**
 * @class SensorBase
 * @brief 传感器抽象基类（模板方法模式）
 * @tparam TData 传感器数据类型（必须继承自 SensorData）
 */
template <typename TData>
class SensorBase {
public:
    SensorBase(uint32_t sensorId, SensorType type)
        : sensorId_(sensorId)
        , type_(type)
        , status_(SensorStatus::UNINIT)
        , config_(SensorConfig::defaultConfig())
        , sequenceNum_(0)
        , errorCount_(0)
    {
        IF_FILTER_PIPELINE_ENABLED(
            filterPipeline_ = NULL;
        )
    }

    virtual ~SensorBase() {
        IF_FILTER_PIPELINE_ENABLED(
            destroyPipeline();
        )
    }

    // ============================================================
    //  生命周期（纯虚方法 — 子类必须实现硬件细节）
    // ============================================================

    /// 初始化传感器（发送配置命令、检测硬件）
    virtual bool init() = 0;

    /// 启动采样
    virtual bool start() = 0;

    /// 停止采样
    virtual bool stop() = 0;

    /// 复位传感器（软件复位 + 清空滤波器）
    virtual bool reset() {
        IF_FILTER_PIPELINE_ENABLED(
            if (filterPipeline_) {
                filterPipeline_->reset();
            }
        )
        sequenceNum_ = 0;
        return true;
    }

    // ============================================================
    //  数据获取（纯虚方法）
    // ============================================================

    /// 读取原始数据（直接与硬件交互）
    virtual TData readRaw() = 0;

    /// 读取滤波后的数据
    virtual TData readFiltered() {
        TData raw = readRaw();

        IF_FILTER_PIPELINE_ENABLED(
            if (filterPipeline_ && config_.enableFiltering) {
                raw.value = filterPipeline_->update(raw.value);
            }
        )

        raw.sequenceNum = sequenceNum_++;
        return raw;
    }

    // ============================================================
    //  配置
    // ============================================================

    /// 设置传感器配置
    virtual bool configure(const SensorConfig& cfg) {
        config_ = cfg;
        return true;
    }

    /// 获取当前配置
    virtual SensorConfig getConfig() const {
        return config_;
    }

    // ============================================================
    //  状态
    // ============================================================

    /// 获取当前运行状态
    SensorStatus getStatus() const { return status_; }

    /// 获取传感器唯一ID
    uint32_t getSensorId() const { return sensorId_; }

    /// 获取传感器类型
    SensorType getType() const { return type_; }

    /// 获取当前采样序列号
    uint16_t getSequenceNum() const { return sequenceNum_; }

    // ============================================================
    //  滤波管理
    // ============================================================

#if SENSOR_FEATURE_FILTER_PIPELINE
    /**
     * @brief 设置滤波管道
     * @param pipeline 滤波管道（基类接管所有权）
     */
    void setFilterPipeline(FilterPipeline<SENSOR_CONSTRAINT_MAX_FILTERS>* pipeline) {
        destroyPipeline();
        filterPipeline_ = pipeline;
    }

    /// 获取滤波管道（可能为 NULL）
    FilterPipeline<SENSOR_CONSTRAINT_MAX_FILTERS>* getFilterPipeline() {
        return filterPipeline_;
    }

    /// 向当前管道添加滤波器（无管道时自动创建）
    bool addFilter(IFilter* filter) {
        if (!filterPipeline_) {
            filterPipeline_ = new FilterPipeline<SENSOR_CONSTRAINT_MAX_FILTERS>();
        }
        return filterPipeline_->addFilter(filter);
    }
#else
    void setFilterPipeline(void*) {}
    void* getFilterPipeline() { return NULL; }
    bool addFilter(IFilter*) { return false; }
#endif

    // ============================================================
    //  诊断
    // ============================================================

    /// 获取传感器名称（用于调试）
    virtual const char* getName() const { return "SensorBase"; }

    /// 获取通信错误计数
    uint32_t getErrorCount() const { return errorCount_; }

protected:
    /// 设置状态（供子类使用）
    void setStatus(SensorStatus status) { status_ = status; }

    /// 增加错误计数
    void incrementError() { ++errorCount_; }

    /// 重置错误计数
    void resetErrorCount() { errorCount_ = 0; }

    /// 获取配置的可写引用（供子类修改）
    SensorConfig& mutableConfig() { return config_; }

#if SENSOR_FEATURE_FILTER_PIPELINE
    void destroyPipeline() {
        if (filterPipeline_) {
            delete filterPipeline_;
            filterPipeline_ = NULL;
        }
    }
#endif

    // ─── 成员变量 ────────────────────────────────────────

    uint32_t        sensorId_;
    SensorType      type_;
    SensorStatus    status_;
    SensorConfig    config_;
    uint16_t        sequenceNum_;
    uint32_t        errorCount_;

#if SENSOR_FEATURE_FILTER_PIPELINE
    FilterPipeline<SENSOR_CONSTRAINT_MAX_FILTERS>* filterPipeline_;
#endif
};

/**
 * @def SENSOR_DECLARE_LIFECYCLE
 * @brief 便捷宏：声明标准的传感器生命周期方法签名
 */
#define SENSOR_DECLARE_LIFECYCLE() \
    bool init() SENSOR_OVERRIDE;   \
    bool start() SENSOR_OVERRIDE;  \
    bool stop() SENSOR_OVERRIDE;   \
    bool reset() SENSOR_OVERRIDE;
