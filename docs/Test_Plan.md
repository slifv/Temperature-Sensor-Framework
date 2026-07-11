# Sensor Framework — 全量测试计划

> 版本: v1.0 | 日期: 2026-07-11 | 作者: Framework Team
>
> 覆盖范围: 白盒单元测试 · 黑盒集成测试 · 平台兼容性测试 · 性能测试 · 回归测试

---

## 目录

1. [测试策略概述](#1-测试策略概述)
2. [测试环境与前置条件](#2-测试环境与前置条件)
3. [白盒单元测试 —— 滤波引擎](#3-白盒单元测试--滤波引擎)
4. [白盒单元测试 —— 框架核心](#4-白盒单元测试--框架核心)
5. [白盒单元测试 —— 异常监控](#5-白盒单元测试--异常监控)
6. [白盒单元测试 —— HAL 硬件抽象层](#6-白盒单元测试--hal-硬件抽象层)
7. [白盒单元测试 —— 工具类](#7-白盒单元测试--工具类)
8. [白盒单元测试 —— 传感器驱动](#8-白盒单元测试--传感器驱动)
9. [白盒单元测试 —— 应用层](#9-白盒单元测试--应用层)
10. [白盒单元测试 —— 定点数数学库](#10-白盒单元测试--定点数数学库)
11. [黑盒集成测试](#11-黑盒集成测试)
12. [平台兼容性测试](#12-平台兼容性测试)
13. [性能基准测试](#13-性能基准测试)
14. [回归测试策略](#14-回归测试策略)
15. [测试覆盖率矩阵](#15-测试覆盖率矩阵)
16. [测试执行计划](#16-测试执行计划)

---

## 1. 测试策略概述

### 1.1 测试金字塔

```
           ╱  E2E  ╲          平台兼容性 / 全链路
          ╱──────────╲         ———————————————
         ╱  集成测试   ╲       黑盒 / 多模块协作
        ╱──────────────╲       ———————————————
       ╱   单元测试 (N)  ╲     白盒 / 每模块 1-N 文件
      ╱──────────────────╲     ———————————————
```

### 1.2 测试维度

| 维度 | 说明 | 通过标准 |
|------|------|---------|
| **功能正确性** | 输入 → 输出是否与预期一致 | 100% 用例通过 |
| **边界条件** | 极值、空值、满容、溢出 | 无崩溃，行为定义明确 |
| **错误路径** | 无效参数、资源耗尽、超时 | 优雅降级，无未定义行为 |
| **资源约束** | ROM/RAM 占用、栈深度 | 满足 Profile 限制 |
| **并发安全** | 中断上下文、多订阅者 | 无竞态、无死锁 |
| **平台兼容** | 8/16/32 位编译器 | 零警告，行为一致 |

---

## 2. 测试环境与前置条件

### 2.1 PC 端测试环境

| 组件 | 要求 |
|------|------|
| OS | Windows 10+ / Linux (Ubuntu 22.04+) / macOS 13+ |
| CMake | ≥ 3.10 |
| 编译器 | GCC 9+ / Clang 12+ / MSVC 2019+ |
| C++ 标准 | C++11（默认），可选 C++14/17/20 |
| 构建命令 | 见 [README.md](../README.md#构建与测试) |

### 2.2 嵌入式目标平台（平台兼容性验证）

| 平台 | MCU | 编译器 | Profile |
|------|-----|--------|---------|
| ARM Cortex-M4 | STM32F407 | arm-none-eabi-gcc 12+ | ADVANCED |
| ARM Cortex-M0+ | STM32G070 | arm-none-eabi-gcc 12+ | STANDARD |
| Xtensa LX7 | ESP32-S3 | xtensa-esp32s3-elf-gcc 12+ | ADVANCED |
| AVR 8-bit | ATmega328P | avr-gcc 7+ | MINIMAL_8BIT |
| MSP430 16-bit | MSP430FR5994 | msp430-elf-gcc 9+ | MINIMAL |

### 2.3 测试框架

| 层级 | 框架 | 说明 |
|------|------|------|
| PC 端 | 内置 `test_framework/` | 无外部依赖，`TEST()` 宏 + `runAllTests()` |
| PC 端（扩展） | Google Test / Catch2 | 可选，用于 IDE 集成和 CI 报告 |
| 嵌入式 | 内置框架 + UART 输出 | `testPrint()` → 串口重定向 |

---

## 3. 白盒单元测试 —— 滤波引擎

> 源文件: `sensor_framework/filter/` | 现有测试: `tests/whitebox/test_filter.cpp` (16 用例)
>
> 目标覆盖率: **≥ 95%**

### 3.1 SlidingWindowFilter（滑动窗口基类）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| FW-01 | 初始化状态 | 构造 `MeanFilter<5>` | `isReady() == false` | ✅ 已覆盖 |
| FW-02 | 窗口填满触发就绪 | 连续 5 次 `update(25.0)` | `isReady() == true` | ✅ 已覆盖 |
| FW-03 | 窗口数据完整性 | 输入 `[10,20,30,40,50]` | `getWindow()[]` 匹配 | ✅ 已覆盖 |
| FW-04 | 窗口溢出滚动 | 输入 6 次 → 查窗口 | 最早值被移除 | 🔲 待实现 |
| FW-05 | 满窗前 getWindow | 3 次 update 后 `getWindow()` | 仅 3 个有效值 | 🔲 待实现 |
| FW-06 | reset 后状态 | 填满 → `reset()` | `isReady() == false`, 窗口空 | 🔲 待实现 |
| FW-07 | 零窗口容量 | `MeanFilter<1>` | 单值即满，均值=输入 | 🔲 待实现 |
| FW-08 | 大窗口容量 | `MeanFilter<32>`（32位上限） | 正常滚动 | 🔲 待实现 |

### 3.2 MeanFilter（滑动均值滤波）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| MF-01 | 基本均值 | `[20,30,40,30]` 窗口4 | 30.0 ±0.1 | ✅ 已覆盖 |
| MF-02 | 未满窗口透传 | 连续 `update(99)` → `update(100)` | 逐值原样返回 | ✅ 已覆盖 |
| MF-03 | reset 清空状态 | 填满 → `reset()` | 未就绪，计数归零 | ✅ 已覆盖 |
| MF-04 | 全零值 | `[0,0,0,0]` | 0.0 | 🔲 待实现 |
| MF-05 | 负值均值 | `[-10,-20,-30,-20]` | -20.0 | 🔲 待实现 |
| MF-06 | 正负混合 | `[-5,5,-5,5]` | 0.0 | 🔲 待实现 |
| MF-07 | 极值范围 | `[1e6, -1e6, 1e6, -1e6]` | 0.0（不溢出） | 🔲 待实现 |

### 3.3 MedianFilter（中值滤波）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| MDF-01 | 脉冲噪声去除 | `[25,26,25.5,26.5,99]` 窗口5 | 26.0（中值） | ✅ 已覆盖 |
| MDF-02 | 偶数窗口 | `[10,20,30,40]` 窗口4 | 25.0（中间两值平均） | ✅ 已覆盖 |
| MDF-03 | 窗口3基础 | `[1,3,2]` 窗口3 | 2.0 | 🔲 待实现 |
| MDF-04 | 全相同值 | `[5,5,5,5,5]` | 5.0 | 🔲 待实现 |
| MDF-05 | 单值窗口 | `MedianFilter<1>` | 输入=输出 | 🔲 待实现 |

### 3.4 LowPassFilter（一阶低通滤波）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| LPF-01 | α=1.0 全透传 | `25→30`, α=1.0 | 30.0 | ✅ 已覆盖 |
| LPF-02 | α=0.1 强平滑 | `25→30`, α=0.1 | 25.5 | ✅ 已覆盖 |
| LPF-03 | α=0.0 不变 | `25→30`, α=0.0 | 25.0（完全过滤） | 🔲 待实现 |
| LPF-04 | α 边界值 | α=1.1 / α=-0.1 | 限幅到 [0,1] 或断言 | 🔲 待实现 |
| LPF-05 | 阶跃响应曲线 | 0→100 多次迭代 | 指数趋近，无超调 | 🔲 待实现 |
| LPF-06 | reset 后重收敛 | 稳定→reset→0→新值 | 从初始值重新开始 | 🔲 待实现 |

### 3.5 KalmanFilter（一维卡尔曼滤波）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| KF-01 | 噪声收敛 | 10 次含噪测量(真值25.0) | 收敛到 25.0±0.3 | ✅ 已覆盖 |
| KF-02 | 参数读写 | `setParameter("Q",0.05)`, `getParameter("Q")` | 读写一致 | ✅ 已覆盖 |
| KF-03 | 高过程噪声 Q=1.0 | 快速跟踪阶跃变化 | 收敛速度 > 低 Q 时 | 🔲 待实现 |
| KF-04 | 高测量噪声 R=10.0 | 强平滑 | 输出方差 < 输入方差 | 🔲 待实现 |
| KF-05 | 参数边界 | Q=0 / R=0 / Q<0 | 拒绝或限幅 | 🔲 待实现 |
| KF-06 | NaN 输入保护 | `update(NaN)` | 保持上次有效值 | 🔲 待实现 |
| KF-07 | Inf 输入保护 | `update(Inf)` | 保持上次有效值 | 🔲 待实现 |
| KF-08 | 协方差下界保护 | 大量相同输入 → `P_` | `P_ >= Q_ * 0.1` | 🔲 待实现 |
| KF-09 | 未识别参数 | `setParameter("X", 1.0)` | `false` | 🔲 待实现 |

### 3.6 TrimmedMeanFilter（截尾均值 / Olympic Average）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| TMF-01 | 去异常值 | `[25.1,24.8,99.9,25.0,25.3,0.1,25.2]` trim=1 | 25.08 ±0.05 | ✅ 已覆盖 |
| TMF-02 | 均匀值不变 | 7 次 25.0 | 25.0 | ✅ 已覆盖 |
| TMF-03 | trim=0 退化为均值 | trim=0 | 结果=MeanFilter | 🔲 待实现 |
| TMF-04 | trim 边界 | trim≥窗口一半 | 行为定义（拒绝或退化） | 🔲 待实现 |
| TMF-05 | 运行时 trim 调整 | 构造后改trim | 后续计算用新trim | 🔲 待实现 |

### 3.7 FilterPipeline（滤波器链）

| ID | 测试用例 | 输入 | 预期输出 | 状态 |
|----|---------|------|---------|------|
| FP-01 | 中值→低通级联 | `[中值,低通]` + 稳定输入 | 输出合理 | ✅ 已覆盖 |
| FP-02 | 添加/移除滤波器 | 添加2→移除2 | 计数正确 | ✅ 已覆盖 |
| FP-03 | 空 Pipeline | 无滤波器 | `update()` = 输入 | 🔲 待实现 |
| FP-04 | 多滤波器级联(4级) | M→LP→K→LP | 正确级联 | 🔲 待实现 |
| FP-05 | reset 全链重置 | pipeline.reset() | 所有滤波器重置 | 🔲 待实现 |
| FP-06 | 容量上限 | 添加 > MAX 个滤波器 | 拒绝，返回 false | 🔲 待实现 |
| FP-07 | 指定位置删除 | removeAt(0), removeAt(最后) | 边界正确 | 🔲 待实现 |

### 3.8 WeightedMeanFilter（加权移动平均）🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| WMF-01 | 权重递减 | 近期权重大 → 响应快 |
| WMF-02 | 权重均匀 | 退化为均值 |
| WMF-03 | 零权重处理 | 某权重=0 的正确行为 |

### 3.9 FIRFilter（FIR 有限脉冲响应）🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| FIR-01 | 简单系数 | `[0.25,0.25,0.25,0.25]` → 均值 |
| FIR-02 | 低通系数 | 截止频率验证 |
| FIR-03 | 系数归一化 | 非归一化系数的处理 |

---

## 4. 白盒单元测试 —— 框架核心

> 源文件: `sensor_framework/core/` | 现有测试: `tests/whitebox/test_event_bus.cpp` (4 用例), `tests/whitebox/test_sensor_manager.cpp` (6 用例)
>
> 目标覆盖率: **≥ 90%**

### 4.1 EventBus（事件总线）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| EB-01 | 单订阅者接收 | 1 传感器 → 1 订阅者 | ✅ 已覆盖 |
| EB-02 | 多订阅者广播 | 1 传感器 → 3 订阅者 | ✅ 已覆盖 |
| EB-03 | 取消订阅停止投递 | unsubscribe 后不再回调 | ✅ 已覆盖 |
| EB-04 | 传感器间隔离 | Sensor1 发布不影响 Sensor2 订阅者 | ✅ 已覆盖 |
| EB-05 | 空订阅发布 | 无订阅者的传感器发布 | 🔲 待实现 |
| EB-06 | NULL 订阅者保护 | `subscribe(id, NULL)` | 🔲 待实现 |
| EB-07 | 订阅者上限 | 超过 `SENSOR_CONSTRAINT_MAX_SUBSCRIBERS` | 🔲 待实现 |
| EB-08 | 重复订阅 | 同一订阅者 subscribe 两次 | 🔲 待实现 |
| EB-09 | unsubscribeAll | 清除某传感器所有订阅 | 🔲 待实现 |
| EB-10 | 绑定数上限 | 传感器数 > `SENSOR_CONSTRAINT_MAX_SENSORS` | 🔲 待实现 |

### 4.2 SensorManager（传感器管理器）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| SM-01 | 注册传感器 | `registerSensor()` 成功 | ✅ 已覆盖 |
| SM-02 | ID 查找 | `findSensorById()` | ✅ 已覆盖 |
| SM-03 | 重复 ID 拒绝 | 同 ID 注册两次 | ✅ 已覆盖 |
| SM-04 | 批量初始化 | `initAll()` 调用全部 init | ✅ 已覆盖 |
| SM-05 | 批量启动 | `startAll()` + 状态验证 | ✅ 已覆盖 |
| SM-06 | 按类型查找 | `findSensorByType()` | ✅ 已覆盖 |
| SM-07 | NULL 传感器注册 | `registerSensor(NULL)` | 🔲 待实现 |
| SM-08 | 容量上限 | 注册 > MaxSensors 个 | 🔲 待实现 |
| SM-09 | 注销传感器 | `unregisterSensor()` → 计数减少 | 🔲 待实现 |
| SM-10 | 批量停止 | `stopAll()` 全部 STOPPED | 🔲 待实现 |
| SM-11 | sampleAll 回调 | 运行中传感器 → callback | 🔲 待实现 |
| SM-12 | 按状态计数 | `getCountByStatus(RUNNING)` | 🔲 待实现 |
| SM-13 | 按类型查找多个 | `findSensorsByType()` | 🔲 待实现 |
| SM-14 | 索引越界访问 | `getSensorAt(999)` → NULL | 🔲 待实现 |
| SM-15 | 空管理器操作 | 无传感器时各种操作 | 🔲 待实现 |

### 4.3 SensorBase（传感器基类）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| SB-01 | 构造初始状态 | 新建 Sensor → UNINIT | 🔲 待实现 |
| SB-02 | readFiltered 滤波流程 | 配置滤波 → 读值经 Pipeline | 🔲 待实现 |
| SB-03 | readFiltered 无滤波 | 关闭滤波 → 读原始值 | 🔲 待实现 |
| SB-04 | 序列号递增 | 多次 `readFiltered()` | 🔲 待实现 |
| SB-05 | configure 配置 | `configure(cfg)` → `getConfig()` 一致 | 🔲 待实现 |
| SB-06 | addFilter 自动创建 | 无 Pipeline → addFilter → 自动 new | 🔲 待实现 |
| SB-07 | filterPipeline_ 所有权 | 析构时 Pipeline 被 delete | 🔲 待实现 |
| SB-08 | reset 重置序列号 | reset() → sequenceNum=0 | 🔲 待实现 |
| SB-09 | 错误计数 | incrementError → getErrorCount | 🔲 待实现 |

### 4.4 SensorConfig / SensorData（数据结构）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| SC-01 | defaultConfig 默认值 | 各字段预设值验证 | 🔲 待实现 |
| SC-02 | SensorData 默认构造 | 零初始化 | 🔲 待实现 |
| SC-03 | TemperatureData 继承 | type == TEMPERATURE | 🔲 待实现 |

---

## 5. 白盒单元测试 —— 异常监控

> 源文件: `sensor_framework/exception/` | 现有测试: `tests/whitebox/test_exception.cpp` (7 用例)
>
> 目标覆盖率: **≥ 90%**

### 5.1 CircuitBreaker（熔断器）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| CB-01 | 初始状态 CLOSED | 构造 → `getState()==CLOSED` | ✅ 已覆盖 |
| CB-02 | 达到阈值 OPEN | 3 次失败(threshold=3) → OPEN | ✅ 已覆盖 |
| CB-03 | 成功清零失败计数 | 失败→成功 → `getConsecutiveFailures()==0` | ✅ 已覆盖 |
| CB-04 | 手动复位 | OPEN → `reset()` → CLOSED | ✅ 已覆盖 |
| CB-05 | HALF_OPEN 探测 | OPEN冷却后 → allowRequest → HALF_OPEN | 🔲 待实现 |
| CB-06 | 探测成功恢复 | HALF_OPEN → recordSuccess → CLOSED | 🔲 待实现 |
| CB-07 | 探测失败重新熔断 | HALF_OPEN → recordFailure → OPEN | 🔲 待实现 |
| CB-08 | 冷却时间未到 | OPEN 后立即 allowRequest | 🔲 待实现 |
| CB-09 | 冷却时间刚过 | 注入 tickProvider → 时间前进 | 🔲 待实现 |
| CB-10 | 阈值动态修改 | `setThreshold()` 后行为 | 🔲 待实现 |
| CB-11 | 冷却时间动态修改 | `setCooldown()` 后行为 | 🔲 待实现 |
| CB-12 | 总计数器 | `getTotalSuccesses()`, `getTotalFailures()` | 🔲 待实现 |

### 5.2 ExceptionLogger（异常日志）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| EL-01 | 记录与最后一条 | log → `last()` 匹配 | ✅ 已覆盖 |
| EL-02 | 环形覆盖 | 写 6 条(MaxEntries=4) → 保留最后4 | ✅ 已覆盖 |
| EL-03 | 严重级别过滤 | INFO+WARNING+ERROR → `getRecent(>=WARNING)` | ✅ 已覆盖 |
| EL-04 | 空日志查询 | 无记录 → `last()` 返回 invalidEvent | 🔲 待实现 |
| EL-05 | get 索引访问 | `get(0)`=最新, `get(1)`=次新 | 🔲 待实现 |
| EL-06 | getStats 统计 | 各级别计数 | 🔲 待实现 |
| EL-07 | clear 清空 | clear → `getTotalCount()==0` | 🔲 待实现 |
| EL-08 | 大容量(64) | ExceptionLogger<64> 正常 | 🔲 待实现 |
| EL-09 | 最小容量(1) | ExceptionLogger<1> 不崩溃 | 🔲 待实现 |

### 5.3 ExceptionHandler / ExceptionDetector 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| EH-01 | 通信异常检测 | 连续 NACK → 触发 COMM 异常 |
| EH-02 | 数据异常检测 | 越界值 → 触发 DATA 异常 |
| EH-03 | 滤波异常检测 | NaN 输出 → 触发 FILTER 异常 |
| EH-04 | 恢复动作推荐 | 各异常类型 → 对应 RecoveryAction |
| EH-05 | SafeSensorWrapper | 异常传感器 → safeRead 降级 |
| EH-06 | HeartbeatMonitor | 超时未采样 → 心跳告警 |

### 5.4 ExceptionTypes（枚举定义）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| ET-01 | ExceptionEvent 默认构造 | 零值/默认枚举 | 🔲 待实现 |
| ET-02 | ErrorCode 分类宏 | 高 8 位 = 类别 | 🔲 待实现 |
| ET-03 | ExceptionSeverity 比较 | INFO < WARNING < ERROR < FATAL | 🔲 待实现 |

---

## 6. 白盒单元测试 —— HAL 硬件抽象层

> 源文件: `sensor_framework/hal/` | 现有测试: 无独立测试文件
>
> 目标覆盖率: **≥ 85%**

### 6.1 MockI2CBus（已在 test_sensor_pipeline 中使用）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| HAL-I2C-01 | 基本读写 | write → read，数据匹配 | ✅ 已覆盖 |
| HAL-I2C-02 | NACK 行为 | 设置 NACK 地址 → write 失败 | ✅ 已覆盖 |
| HAL-I2C-03 | NACK 清除恢复 | clearNack → 恢复通信 | ✅ 已覆盖 |
| HAL-I2C-04 | 设备响应数据 | 预设响应 → read 正确 | 🔲 待实现 |
| HAL-I2C-05 | 多设备共存 | 多个设备地址独立响应 | 🔲 待实现 |
| HAL-I2C-06 | 未注册设备 | 未预设设备的地址 → NACK | 🔲 待实现 |
| HAL-I2C-07 | 读写计数统计 | `getWriteCount()`, `getReadCount()` | 🔲 待实现 |
| HAL-I2C-08 | 地址扫描 | scan 返回已注册地址列表 | 🔲 待实现 |

### 6.2 MockSPIBus 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| HAL-SPI-01 | 基本传输 | write + read 全双工 |
| HAL-SPI-02 | 片选管理 | CS 引脚状态验证 |
| HAL-SPI-03 | 模式配置 | Mode 0/1/2/3 |
| HAL-SPI-04 | 频率设置 | 时钟分频 |

### 6.3 MockADCChannel 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| HAL-ADC-01 | 模拟电压读取 | 预设电压 → read 返回 |
| HAL-ADC-02 | 分辨率位宽 | 8/10/12/16 位 |
| HAL-ADC-03 | 参考电压 | 不同 Vref → 换算正确 |

### 6.4 MockOneWireBus 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| HAL-OW-01 | 设备搜索 | 返回 ROM 码列表 |
| HAL-OW-02 | 读写时序 | bit/byte 级别 |
| HAL-OW-03 | CRC 校验 | 正确/错误 CRC |

### 6.5 HAL Interface 抽象层 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| HAL-IF-01 | ITimeProvider 注入 | 自定义 tick 函数 → 时间正确 |
| HAL-IF-02 | Platform Selector | 平台宏 → 正确 platform 类 |

---

## 7. 白盒单元测试 —— 工具类

> 源文件: `sensor_framework/utils/` | 现有测试: 无独立测试文件
>
> 目标覆盖率: **≥ 90%**

### 7.1 StaticVector（静态容器）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| SV-01 | 初始空 | size=0, is_empty=true | 🔲 待实现 |
| SV-02 | push_back | 添加 → size++, 索引访问 | 🔲 待实现 |
| SV-03 | pop_back | 移除尾部 → size-- | 🔲 待实现 |
| SV-04 | 满容拒绝 | push 超过 Capacity → false | 🔲 待实现 |
| SV-05 | remove 按索引 | 中间删除 → 后续前移 | 🔲 待实现 |
| SV-06 | removeValue | 按值删除第一个匹配 | 🔲 待实现 |
| SV-07 | clear 清空 | clear → size=0, 析构调用 | 🔲 待实现 |
| SV-08 | front/back 访问 | 首尾元素正确 | 🔲 待实现 |
| SV-09 | 迭代器遍历 | begin → end 顺序正确 | 🔲 待实现 |
| SV-10 | indexOf 查找 | 存在/不存在 → 索引/Capacity | 🔲 待实现 |
| SV-11 | 容量1边界 | StaticVector<int, 1> 全部操作 | 🔲 待实现 |
| SV-12 | 非平凡类型 | 含析构函数的元素 → 正确析构 | 🔲 待实现 |

### 7.2 RingBuffer（环形缓冲区）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| RB-01 | 基本读写 | 写 → 读 → 数据一致 | 🔲 待实现 |
| RB-02 | 环形回绕 | 写到尾部 → 回绕到头部 | 🔲 待实现 |
| RB-03 | 空读保护 | 空 buffer → read 返回哨兵 | 🔲 待实现 |
| RB-04 | 满写保护 | 满 buffer → write 拒绝 | 🔲 待实现 |
| RB-05 | 可用空间 | available() 计算正确 | 🔲 待实现 |

### 7.3 TimeProvider（时间提供者）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| TP-01 | tick 单调递增 | 连续调用 → 返回值递增 | 🔲 待实现 |
| TP-02 | 自定义 tick 函数 | 注入函数 → 返回注入值 | 🔲 待实现 |
| TP-03 | 溢出处理 | uint32_max → 回绕 | 🔲 待实现 |
| TP-04 | 时间差计算 | `elapsed(a, b)` 考虑回绕 | 🔲 待实现 |

---

## 8. 白盒单元测试 —— 传感器驱动

> 源文件: `sensor_framework/sensors/temperature/` | 现有测试: 无独立测试文件
>
> 目标覆盖率: **≥ 80%**（依赖 Mock HAL）

### 8.1 SHT30（I2C 温湿度传感器）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| SHT30-01 | 单次测量 | Mock I2C → readRaw 返回温度 | 🔲 待实现 |
| SHT30-02 | CRC 校验正确 | 合法 CRC → 数据有效 | 🔲 待实现 |
| SHT30-03 | CRC 校验失败 | 非法 CRC → 重试/报错 | 🔲 待实现 |
| SHT30-04 | 周期测量模式 | 设置周期 → 定时读数 | 🔲 待实现 |
| SHT30-05 | 加热器控制 | 启用/禁用 → 状态变化 | 🔲 待实现 |
| SHT30-06 | I2C NACK 处理 | 通信失败 → 异常处理 | 🔲 待实现 |
| SHT30-07 | 地址选择 | ADDR 引脚 → 0x44/0x45 | 🔲 待实现 |

### 8.2 DS18B20（OneWire 温度传感器）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| DS18-01 | ROM 搜索 | Mock OW → 发现设备 | 🔲 待实现 |
| DS18-02 | 温度转换 | 启动转换 → 等待 → 读数 | 🔲 待实现 |
| DS18-03 | 分辨率配置 | 9/10/11/12 bit | 🔲 待实现 |
| DS18-04 | 多设备总线 | 多个 ROM 码 → 轮流读取 | 🔲 待实现 |
| DS18-05 | 寄生供电模式 | 寄生供电 → 强上拉 | 🔲 待实现 |

### 8.3 NTC Thermistor（NTC 热敏电阻）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| NTC-01 | ADC 值 → 温度换算 | Steinhart-Hart / B 参数方程 | 🔲 待实现 |
| NTC-02 | 分压电路 | R1/R2 + Vref → 电阻值 | 🔲 待实现 |
| NTC-03 | 查表插值 | LUT 方法 → 温度 | 🔲 待实现 |
| NTC-04 | ADC 噪声 | 含噪 ADC → 滤波后温度 | 🔲 待实现 |

---

## 9. 白盒单元测试 —— 应用层

> 源文件: `sensor_framework/app/` | 现有测试: 无独立测试文件
>
> 目标覆盖率: **≥ 75%**

### 9.1 GuiDisplaySubscriber

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| GUI-01 | 接收数据回调 | onDataReceived → display 函数被调用 | 🔲 待实现 |
| GUI-02 | 格式化输出 | 温度值 → "25.0°C" 格式 | 🔲 待实现 |
| GUI-03 | NULL 回调保护 | displayFunc=NULL → 不崩溃 | 🔲 待实现 |

### 9.2 MqttSenderSubscriber

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| MQTT-01 | 数据序列化 | SensorData → JSON/二进制 | 🔲 待实现 |
| MQTT-02 | 发送节流 | 高频采样 → 按间隔发送 | 🔲 待实现 |
| MQTT-03 | 断线重连 | MQTT 断开 → 缓冲 → 重连发送 | 🔲 待实现 |

### 9.3 BusinessLogicSubscriber

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| BL-01 | 温度告警 | 超上限 → 触发告警 | 🔲 待实现 |
| BL-02 | 温度告警恢复 | 回落到正常 → 解除告警 | 🔲 待实现 |
| BL-03 | 死区/迟滞 | 阈值 ± 回差 | 🔲 待实现 |
| BL-04 | PID 控制输出 | 偏差 → 控制量 | 🔲 待实现 |

---

## 10. 白盒单元测试 —— 定点数数学库

> 源文件: `sensor_framework/fixed_point/` | 目标覆盖率: **≥ 90%**

### 10.1 Q-Math（Q15.16 定点运算）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| QM-01 | 整数 → 定点转换 | `toFixed(1.5)` → Q15.16 值 | 🔲 待实现 |
| QM-02 | 定点 → 浮点转换 | `toFloat(q)` → 原值 ±1 LSB | 🔲 待实现 |
| QM-03 | 定点加法 | a + b，无溢出 | 🔲 待实现 |
| QM-04 | 定点减法 | a - b | 🔲 待实现 |
| QM-05 | 定点乘法 | a × b → 64bit 中间 → 移位 | 🔲 待实现 |
| QM-06 | 定点除法 | a ÷ b → 先移位再除 | 🔲 待实现 |
| QM-07 | 饱和加法 | 溢出 → INT32_MAX/INT32_MIN | 🔲 待实现 |
| QM-08 | 绝对值 | abs(x) | 🔲 待实现 |
| QM-09 | 平方根 | sqrt → 迭代法 | 🔲 待实现 |
| QM-10 | 边界值 | 0, ±1, INT32_MAX, INT32_MIN | 🔲 待实现 |

### 10.2 FixedMeanFilter（定点均值滤波）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| FMF-01 | 定点均值 | Q15.16 输入 → 均值 | 🔲 待实现 |
| FMF-02 | 与浮点版本对比 | 误差 < 1 LSB | 🔲 待实现 |
| FMF-03 | 全整数路径 | 无浮点运算（验证反汇编） | 🔲 待实现 |

---

## 11. 黑盒集成测试

> 现有测试: `tests/blackbox/test_sensor_pipeline.cpp` (5 用例)
>
> 目标: 覆盖跨模块数据流和典型使用场景

### 11.1 传感器全链路（已有基础覆盖）

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| BB-01 | 均值→低通 Pipeline | 25.0 稳定输入 → 收敛 | ✅ 已覆盖 |
| BB-02 | Mock I2C 读写 | 预设响应 → 读写正确 | ✅ 已覆盖 |
| BB-03 | Mock I2C NACK | NACK → 通信失败 | ✅ 已覆盖 |
| BB-04 | EventBus 3 订阅者 | 广播 10 次 → 各收到 10 次 | ✅ 已覆盖 |
| BB-05 | 多种滤波组合对比 | α=0.1 vs α=0.3 → 平滑程度 | ✅ 已覆盖 |

### 11.2 新增集成场景 🔲 全部待实现

| ID | 测试用例 | 描述 |
|----|---------|------|
| BB-06 | 完整采集→滤波→分发链路 | SensorManager + Pipeline + EventBus + 3 订阅者 |
| BB-07 | 异常→熔断→恢复链路 | 注入错误 → 熔断 → 冷却 → 恢复 |
| BB-08 | 多传感器并发 | 3 传感器同时采样 → 数据不串扰 |
| BB-09 | 动态滤波器切换 | 运行时 addFilter / removeFilter |
| BB-10 | 极端数据场景 | 全零/全负/突变/持续漂移 |
| BB-11 | 长时间运行稳定性 | 10万次采样无内存泄漏 |
| BB-12 | Mock HAL 多设备拓扑 | I2C(2设备) + SPI(1设备) + ADC(2通道) |
| BB-13 | Profile 切换行为 | 同代码 → MINIMAL vs ADVANCED 编译差异 |

---

## 12. 平台兼容性测试

> 目标: 5 个目标平台 0 编译错误，行为一致

### 12.1 编译兼容性矩阵

| 平台 | 编译器 | C++11 | C++14 | C++17 | C++20 | RTTI OFF | EXCEPT OFF |
|------|--------|-------|-------|-------|-------|----------|------------|
| x86_64 Windows | GCC 16 (MinGW) | ✅ | 🔲 | 🔲 | 🔲 | ✅ | 🔲 |
| x86_64 Linux | GCC 14 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| x86_64 Linux | Clang 18 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| x86_64 Windows | MSVC 2022 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| ARM Cortex-M4 | arm-none-eabi-gcc 12 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| ARM Cortex-M0+ | arm-none-eabi-gcc 12 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| Xtensa LX7 | xtensa-esp32s3-elf-gcc | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| AVR 8-bit | avr-gcc 7 | 🔲 | 🔲 | N/A | N/A | 🔲 | 🔲 |
| MSP430 16-bit | msp430-elf-gcc 9 | 🔲 | 🔲 | 🔲 | N/A | 🔲 | 🔲 |

### 12.2 Profile × 平台兼容矩阵

| Profile | ARM M4 | ARM M0+ | ESP32 | AVR | MSP430 |
|---------|--------|---------|-------|-----|--------|
| MINIMAL | 🔲 | 🔲 | 🔲 | 🔲 | ✅ 预期 |
| STANDARD | 🔲 | 🔲 | 🔲 | 🔲 | 🔲 |
| ADVANCED | 🔲 | 🔲 | 🔲 | ❌ ROM超限 | ❌ ROM超限 |
| MINIMAL_8BIT | 🔲 | 🔲 | 🔲 | ✅ 预期 | ✅ 预期 |

### 12.3 行为一致性验证

| ID | 测试用例 | 描述 |
|----|---------|------|
| PT-01 | 相同输入 → 相同输出 | 跨平台：同一组传感器数据 → 同一滤波结果 |
| PT-02 | 浮点精度差异 | IEEE 754 vs 软件浮点 → 误差 < 0.1% |
| PT-03 | ROM/RAM 测量 | 各 Profile → 各平台 → 记录实际占用 |
| PT-04 | 中断安全 | ISR 中 update() → 无竞态 |
| PT-05 | 栈深度测量 | 最大调用深度 < 平台限制 |

---

## 13. 性能基准测试

### 13.1 滤波器性能（32位平台 @ 72MHz）

| 滤波器 | 目标耗时 (μs) | 目标 ROM (bytes) | 目标 RAM (bytes) |
|--------|-------------|-----------------|-----------------|
| MeanFilter<4> | < 5 | < 200 | < 64 |
| MeanFilter<32> | < 15 | < 400 | < 256 |
| MedianFilter<5> | < 50 | < 500 | < 128 |
| MedianFilter<9> | < 150 | < 800 | < 256 |
| LowPassFilter | < 3 | < 150 | < 32 |
| KalmanFilter | < 10 | < 300 | < 64 |
| TrimmedMeanFilter<7,1> | < 80 | < 600 | < 256 |
| FIRFilter<16> | < 30 | < 500 | < 256 |
| FilterPipeline<4> | < 200 | < 800 | < 512 |

### 13.2 8位平台性能（AVR @ 16MHz）

| 滤波器 | 目标耗时 (ms) | 目标 ROM (bytes) |
|--------|-------------|-----------------|
| FixedMeanFilter<4> (定点) | < 1 | < 300 |
| FixedMeanFilter<8> (定点) | < 2 | < 500 |
| MeanFilter<4> (软件浮点) | < 5 | < 800 |

### 13.3 性能回归检查

| ID | 测试用例 | 描述 | 状态 |
|----|---------|------|------|
| PERF-01 | 滤波器 benchmark | 1000 次 update → 平均耗时 | 🔲 |
| PERF-02 | EventBus publish | 8 订阅者 → 总耗时 | 🔲 |
| PERF-03 | SensorManager broadcast | 8 传感器 × sampleAll | 🔲 |
| PERF-04 | Memory 分配 | 0 次 malloc/new（除显式 new） | 🔲 |
| PERF-05 | 代码体积 | 各 Profile → ROM 不超限 | 🔲 |

---

## 14. 回归测试策略

### 14.1 提交前检查（Pre-commit）

```bash
# 每次 git commit 前执行
cmake .. -DSENSOR_PROFILE=ADVANCED -DBUILD_TESTS=ON
mingw32-make -j$(nproc)
ctest --output-on-failure
```

| 检查项 | 阻断条件 |
|--------|---------|
| 编译 (ADVANCED) | 任何错误 |
| 全部 5 个测试可执行文件 | 任何失败 |
| 编译警告 | > 0 个（-Wall -Wextra） |

### 14.2 CI Pipeline（建议配置）

```yaml
# .github/workflows/test.yml
strategy:
  matrix:
    profile: [MINIMAL, STANDARD, ADVANCED, MINIMAL_8BIT]
    compiler: [gcc, clang]
    std: [c++11, c++14, c++17]
```

### 14.3 夜间构建

- 全 Profile × 全编译器 × 全 C++ 标准组合（24 个配置）
- Valgrind / AddressSanitizer 内存检查
- UndefinedBehaviorSanitizer 未定义行为检查
- 代码覆盖率报告（gcov/lcov）

---

## 15. 测试覆盖率矩阵

### 15.1 模块覆盖率汇总

| 模块 | 总用例数 | 已实现 | 待实现 | 覆盖率 |
|------|---------|--------|--------|--------|
| **滤波引擎** (filter/) | 56 | 16 | 40 | 29% |
| **框架核心** (core/) | 37 | 10 | 27 | 27% |
| **异常监控** (exception/) | 30 | 7 | 23 | 23% |
| **HAL 层** (hal/) | 19 | 3 | 16 | 16% |
| **工具类** (utils/) | 21 | 0 | 21 | 0% |
| **传感器驱动** (sensors/) | 16 | 0 | 16 | 0% |
| **应用层** (app/) | 10 | 0 | 10 | 0% |
| **定点数库** (fixed_point/) | 13 | 0 | 13 | 0% |
| **集成测试** (blackbox/) | 13 | 5 | 8 | 38% |
| **性能测试** | 5 | 0 | 5 | 0% |
| **平台兼容** | 5 | 0 | 5 | 0% |
| **总计** | **225** | **41** | **184** | **18%** |

### 15.2 优先级分类

| 优先级 | 数量 | 说明 |
|--------|------|------|
| **P0 (阻塞)** | 0 | 必须立即实现 |
| **P1 (高)** | 43 | 核心功能覆盖（已标注 🔲 的核心路径） |
| **P2 (中)** | 87 | 边界条件、错误路径 |
| **P3 (低)** | 54 | 性能、平台兼容、极端场景 |

---

## 16. 测试执行计划

### Phase 1: 补齐核心单元测试 (Week 1-2)

```
目标: 滤波引擎 + 框架核心 + 异常监控 覆盖率达 85%

□ filter/        +40 用例 (P1: 24, P2: 16)
□ core/          +27 用例 (P1: 15, P2: 12)
□ exception/     +23 用例 (P1: 10, P2: 13)
```

### Phase 2: 工具类 + HAL 层 (Week 3)

```
目标: utils/ + hal/ 覆盖率达 85%

□ utils/         +21 用例
□ hal/           +16 用例 (Mock 扩展)
```

### Phase 3: 传感器驱动 + 应用层 (Week 4)

```
目标: 驱动层可 Mock 测试

□ sensors/       +16 用例 (需完整 Mock HAL)
□ app/           +10 用例
□ fixed_point/   +13 用例
```

### Phase 4: 集成测试 + 性能基准 (Week 5)

```
目标: 端到端场景 + 性能数据

□ blackbox/      +8 用例
□ 性能 benchmark  +5 用例
```

### Phase 5: 平台兼容性验证 (Week 6)

```
目标: 5 平台 × 多编译器 = 全绿

□ ARM M4 编译 + 运行
□ ARM M0+ 编译
□ ESP32 编译 + 运行
□ AVR 8-bit 编译 + ROM 检查
□ MSP430 编译 + ROM 检查
□ CI Pipeline 搭建
```

---

## 附录 A: 测试文件命名规范

```
tests/
├── whitebox/
│   ├── test_filter.cpp           # 滤波器白盒
│   ├── test_event_bus.cpp        # EventBus 白盒
│   ├── test_sensor_manager.cpp   # SensorManager 白盒
│   ├── test_exception.cpp        # 异常监控白盒
│   ├── test_static_vector.cpp    # StaticVector (待创建)
│   ├── test_ring_buffer.cpp      # RingBuffer (待创建)
│   ├── test_sensor_base.cpp      # SensorBase (待创建)
│   ├── test_sht30.cpp            # SHT30 (待创建)
│   ├── test_ds18b20.cpp          # DS18B20 (待创建)
│   ├── test_ntc.cpp              # NTC (待创建)
│   ├── test_q_math.cpp           # Q-Math 定点数 (待创建)
│   ├── test_app_subscribers.cpp  # 应用层订阅者 (待创建)
│   └── test_fixed_filter.cpp     # 定点滤波器 (待创建)
├── blackbox/
│   ├── test_sensor_pipeline.cpp  # 全链路集成 (已有)
│   ├── test_multi_sensor.cpp     # 多传感器并发 (待创建)
│   ├── test_fault_recovery.cpp   # 故障恢复 (待创建)
│   └── test_long_run.cpp         # 长时间运行 (待创建)
└── performance/
    ├── bench_filter.cpp          # 滤波器性能 (待创建)
    └── bench_memory.cpp          # 内存占用 (待创建)
```

## 附录 B: 测试用例模板

```cpp
/**
 * @brief 测试套件: <模块名>
 * @details 覆盖: 功能正确性 / 边界条件 / 错误路径
 */

#include "../../test_framework/test_fixture.hpp"
#include "../../<module>/<header>.hpp"

// ============================================================
//  功能正确性
// ============================================================

TEST(<SuiteName>, <DescriptiveName>) {
    // Arrange: 构造测试对象和输入数据

    // Act: 执行被测操作

    // Assert: 验证输出
    TEST_ASSERT_EQ(actual, expected, "描述信息");
    TEST_ASSERT_TRUE(condition, "描述信息");
    TEST_ASSERT_FLOAT_EQ(actual, expected, epsilon, "描述信息");

    return true;
}

// ============================================================
//  边界条件
// ============================================================

TEST(<SuiteName>, ZeroCapacity) { ... }
TEST(<SuiteName>, MaxCapacity) { ... }
TEST(<SuiteName>, NullInput) { ... }

// ============================================================
//  错误路径
// ============================================================

TEST(<SuiteName>, InvalidParameter) { ... }
TEST(<SuiteName>, OutOfRange) { ... }

int main() {
    runAllTests();
    printTestSummary();
    TestStats& stats = getTestStats();
    return (stats.failed > 0) ? 1 : 0;
}
```

---

> 文档版本: v1.0 | 最后更新: 2026-07-11
>
> 计划用例总数: **225** | 已实现: **41** (18%) | 待实现: **184**
