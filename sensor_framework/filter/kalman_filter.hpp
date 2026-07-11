/**
 * @file kalman_filter.hpp
 * @brief 一维卡尔曼滤波 — 基于状态空间模型的最优递归估计
 * @details 预测: x̂⁻=x̂, P⁻=P+Q; 更新: K=P⁻/(P⁻+R), x̂=x̂⁻+K(z-x̂⁻), P=(1-K)P⁻
 *          计算复杂度 O(1)，自适应跟踪噪声环境下的真值。
 *          参考: 需求评审报告 第6.1.4节
 */

#pragma once

#include "../feature_config.hpp"
#include "filter_interface.hpp"

#if SENSOR_FEATURE_FILTER_KALMAN

class KalmanFilter : public IFilter {
public:
    /**
     * @param Q 过程噪声协方差 — 越大越信任模型预测（跟踪越快）
     *          典型值: 0.001 ~ 0.1
     * @param R 测量噪声协方差 — 越大越不信任测量值（平滑越强）
     *          典型值: 0.01 ~ 1.0
     * @param initialEstimate 初始估计值
     * @param initialError 初始误差协方差
     */
    KalmanFilter(float Q = 0.01f, float R = 0.5f,
                 float initialEstimate = 0.0f,
                 float initialError = 1.0f)
        : Q_(Q), R_(R)
        , xHat_(initialEstimate)
        , P_(initialError)
    {}

    float update(float input) SENSOR_OVERRIDE {
        // ── 预测步骤 ──
        float xHatMinus = xHat_;
        float PMinus = P_ + Q_;

        // ── 更新步骤 ──
        float K = PMinus / (PMinus + R_);       // 卡尔曼增益
        xHat_ = xHatMinus + K * (input - xHatMinus);
        P_ = (1.0f - K) * PMinus;

        // 安全保护
        if (!isFinite(xHat_)) {
            xHat_ = input;
            P_ = initialP_;
        }

        // 协方差下界保护（防止P_过小导致滤波器"僵硬"）
        if (P_ < Q_ * 0.1f) {
            P_ = Q_ * 0.1f;
        }

        lastOutput_ = xHat_;
        return xHat_;
    }

    void reset() SENSOR_OVERRIDE {
        xHat_ = 0.0f;
        P_ = 1.0f;
        lastOutput_ = 0.0f;
    }

    const char* getName() const SENSOR_OVERRIDE { return "Kalman"; }
    FilterType getType() const SENSOR_OVERRIDE { return FilterType::KALMAN; }

    bool setParameter(const char* key, float value) SENSOR_OVERRIDE {
        if (!key) return false;
        if (key[0] == 'Q' && key[1] == '\0') { Q_ = value; return true; }
        if (key[0] == 'R' && key[1] == '\0') { R_ = value; return true; }
        return false;
    }

    float getParameter(const char* key) const SENSOR_OVERRIDE {
        if (!key) return 0.0f;
        if (key[0] == 'Q' && key[1] == '\0') return Q_;
        if (key[0] == 'R' && key[1] == '\0') return R_;
        if (key[0] == 'K' && key[1] == '\0') {
            // 返回当前卡尔曼增益
            float PMinus = P_ + Q_;
            return PMinus / (PMinus + R_);
        }
        return 0.0f;
    }

    /// 获取当前估计值
    float getEstimate() const { return xHat_; }

    /// 获取当前误差协方差
    float getErrorCovariance() const { return P_; }

private:
    float Q_;       ///< 过程噪声协方差
    float R_;       ///< 测量噪声协方差
    float xHat_;    ///< 当前状态估计
    float P_;       ///< 当前误差协方差
    float initialP_ = 1.0f;
};

#endif // SENSOR_FEATURE_FILTER_KALMAN
