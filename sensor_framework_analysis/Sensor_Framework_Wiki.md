# 跨MCU平台Sensor框架 — 完整Wiki

> **版本**: v1.0  
> **更新日期**: 2026-07-11  
> **基于**: 《需求分析与评审报告》v1.5（评审通过）  
> **实现语言**: C++11（最低）/ C++14/17/20（可选特性通过宏启用）  
> **目标平台**: 8位 / 16位 / 32位 MCU

---

## 目录

1. [项目概述](#1-项目概述)
2. [快速开始](#2-快速开始)
3. [架构设计](#3-架构设计)
4. [目录结构](#4-目录结构)
5. [Profile预设](#5-profile预设)
6. [核心模块详解](#6-核心模块详解)
   - 6.1 [编译时特性配置](#61-编译时特性配置)
   - 6.2 [滤波引擎](#62-滤波引擎)
   - 6.3 [框架核心](#63-框架核心)
   - 6.4 [事件总线](#64-事件总线)
   - 6.5 [异常监控](#65-异常监控)
   - 6.6 [硬件抽象层](#66-硬件抽象层)
   - 6.7 [温度传感器](#67-温度传感器)
   - 6.8 [定点数数学库](#68-定点数数学库)
   - 6.9 [测试框架](#69-测试框架)
7. [API参考](#7-api参考)
8. [平台移植指南](#8-平台移植指南)
9. [裁剪指南](#9-裁剪指南)
10. [滤波器选型指南](#10-滤波器选型指南)
11. [异常处理策略](#11-异常处理策略)
12. [测试指南](#12-测试指南)
13. [FAQ](#13-faq)

---

## 1. 项目概述

### 1.1 背景

在嵌入式IoT产品开发中，传感器数据采集是核心功能。不同MCU平台在硬件抽象、驱动模型、RTOS支持等方面差异显著，导致传感器代码难以复用。

### 1.2 核心目标

| 目标 | 说明 |
|------|------|
| **平台无关** | 核心逻辑与硬件解耦，通过HAL适配层支持多MCU |
| **可扩展** | 新增传感器只需实现标准接口，无需修改框架 |
| **灵活滤波** | 支持8种滤波算法，可动态组合Pipeline |
| **应用解耦** | 通过EventBus发布-订阅模式分发数据 |
| **类型安全** | C++模板 + 接口抽象，编译期保证类型安全 |
| **可裁剪** | 编译时模块裁剪，零残留 |
| **全平台** | 8位AVR到32位Cortex-M7统一接口 |

### 1.3 兼容性矩阵

| 平台 | 位宽 | 典型MCU | C++编译器 | Profile推荐 |
|------|------|---------|-----------|-------------|
| ARM Cortex-M4/M7 | 32-bit | STM32F4/H7 | GCC/ARMCC/IAR | ADVANCED |
| ARM Cortex-M0+ | 32-bit | STM32G0 | GCC | STANDARD |
| Xtensa LX6/LX7 | 32-bit | ESP32-S3 | GCC | ADVANCED |
| ARM Cortex-M4 | 32-bit | nRF52840 | GCC | ADVANCED |
| RISC-V | 32-bit | GD32VF103 | GCC | STANDARD |
| MSP430 | 16-bit | MSP430FR | GCC/IAR | MINIMAL_8BIT |
| PIC24/dsPIC | 16-bit | PIC24FJ | XC16 | MINIMAL_8BIT |
| AVR ATmega | 8-bit | ATmega328P | AVR-GCC | MINIMAL_8BIT |
| STM8 | 8-bit | STM8S | IAR/SDCC | MINIMAL_8BIT (C binding) |
| PIC16 | 8-bit | PIC16F | XC8 (C only) | MINIMAL_8BIT (C binding) |

---

## 2. 快速开始

### 2.1 包含头文件

```cpp
// 方式一：包含统一入口（推荐）
#include "sensor_framework.hpp"

// 方式二：按需包含（更精确的依赖控制）
#include "core/sensor_base.hpp"
#include "filter/mean_filter.hpp"
#include "core/event_bus.hpp"
```

### 2.2 CMake构建

```bash
# Standard Profile（默认）
cmake .. -DSENSOR_PROFILE=STANDARD

# 全功能Advanced Profile
cmake .. -DSENSOR_PROFILE=ADVANCED

# 极简Minimal Profile
cmake .. -DSENSOR_PROFILE=MINIMAL

# 8位MCU极简
cmake .. -DSENSOR_PROFILE=MINIMAL_8BIT

# 自定义配置
cmake .. -DSENSOR_PROFILE=CUSTOM -DSENSOR_FEATURE_FILTER_KALMAN=OFF

# 构建并运行测试
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

### 2.3 5分钟示例：温度监控

```cpp
#include "sensor_framework.hpp"

// 1. 创建SHT30传感器
SHT30Sensor tempSensor(0x01, i2cBus1, 0x44);

// 2. 配置滤波链：中值去毛刺 → 低通平滑
tempSensor.addFilter(new MedianFilter<5>());
tempSensor.addFilter(new LowPassFilter(0.1f));

// 3. 创建订阅者
GuiDisplaySubscriber gui("LCD", &displayTemperature);
AlarmSubscriber alarm("Alarm", 35.0f, 15.0f);

// 4. 订阅数据
auto& bus = EventBus<>::getInstance();
bus.subscribe(0x01, &gui);
bus.subscribe(0x01, &alarm);

// 5. 启动采集（10Hz）
tempSensor.init();
tempSensor.configure(SensorConfig::defaultConfig());
tempSensor.start();

// 6. 主循环：每隔100ms采样一次
while (true) {
    SensorData data = tempSensor.readFiltered();
    bus.publish(data);
    delay(100);
}
```

---

## 3. 架构设计

### 3.1 分层架构图

```
┌──────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                       │
│    GUI显示 ←→ MQTT发送 ←→ 业务逻辑 (告警/PID/安全联动)        │
├──────────────────────────────────────────────────────────────┤
│                    框架核心层 (Core)                          │
│  ┌────────────┐  ┌─────────────┐  ┌──────────────────────┐  │
│  │ EventBus   │  │SensorManager│  │   异常监控引擎         │  │
│  │ 发布-订阅   │  │ 注册/生命周期 │  │ 检测器+熔断+恢复      │  │
│  └─────┬──────┘  └──────┬──────┘  └──────────┬───────────┘  │
│        │                │                     │              │
│  ┌─────▼────────────────▼─────────────────────▼───────────┐  │
│  │              滤波引擎 (Filter Engine)                    │  │
│  │  均值|中值|低通|卡尔曼|FIR|加权|截尾均值|滑动窗口框架      │  │
│  │              Pipeline 可动态组合                          │  │
│  └──────────────────────────┬──────────────────────────────┘  │
├──────────────────────────────┼───────────────────────────────┤
│                    硬件抽象层 (HAL)                           │
│  I2C | SPI | ADC | OneWire | GPIO | Time | Mutex            │
├──────────────────────────────────────────────────────────────┤
│                    平台驱动层                                 │
│  STM32 HAL | ESP-IDF | nRF SDK | GD32 | AVR | MSP430 | ... │
└──────────────────────────────────────────────────────────────┘

         编译时配置裁剪层 → feature_config.hpp → #if FEATURE_XXX
```

### 3.2 设计模式

| 模式 | 应用场景 |
|------|----------|
| **策略模式** | 不同滤波算法可互相替换 |
| **观察者模式** | EventBus发布-订阅数据分发 |
| **模板方法** | SensorBase定义采集流程，子类实现硬件细节 |
| **装饰器模式** | FilterPipeline级联 + SafeSensorWrapper异常保护 |
| **单例模式** | SensorManager / EventBus 全局唯一 |
| **适配器模式** | HAL接口适配不同MCU平台 |

### 3.3 数据流

```
定时器/RTOS任务
      │
      ▼
Sensor::readRaw()  ─── HAL读取硬件寄存器
      │
      ▼
FilterPipeline     ─── 多级滤波级联处理
      │
      ▼
EventBus::publish() ─── 广播到所有订阅者
      │
  ┌───┼───┐
  ▼   ▼   ▼
GUI  MQTT 业务逻辑
```

---

## 4. 目录结构

```
sensor_framework/
├── sensor_framework.hpp              # ★ 统一入口头文件
├── CMakeLists.txt                    # CMake构建配置
│
├── compiler_features.hpp             # C++版本+编译器检测宏
├── feature_config.hpp                # ★ 编译时特性开关（裁剪入口）
├── sensor_profile.hpp                # Profile预设配置
│
├── utils/
│   ├── ring_buffer.hpp               # 静态环形缓冲区
│   ├── static_vector.hpp             # 静态分配容器
│   └── time_provider.hpp             # 时间提供者接口
│
├── hal/
│   ├── hal_interface.hpp             # HAL统一接口（ITime/IMutex/IBus）
│   ├── bus/
│   │   ├── i2c_bus.hpp               # I2C总线封装（超时+重试）
│   │   ├── spi_bus.hpp               # SPI总线封装（CS管理）
│   │   └── adc_channel.hpp           # ADC通道封装（多次采样平均）
│   └── platform/
│       └── platform_selector.hpp     # 编译时平台自动选择
│
├── filter/
│   ├── filter_interface.hpp          # IFilter统一接口 + FilterType枚举
│   ├── filter_pipeline.hpp           # 滤波器链Pipeline
│   ├── sliding_window.hpp            # ★ 滑动窗口框架基类
│   ├── mean_filter.hpp               # 滑动均值滤波
│   ├── median_filter.hpp             # 中值滤波
│   ├── low_pass_filter.hpp           # 一阶低通滤波(IIR)
│   ├── kalman_filter.hpp             # 一维卡尔曼滤波
│   ├── weighted_mean_filter.hpp      # 加权移动平均
│   ├── fir_filter.hpp                # FIR数字滤波
│   └── trimmed_mean_filter.hpp       # ★ 截尾均值/Olympic Average
│
├── core/
│   ├── sensor_config.hpp             # SensorType/Status/Config/Data
│   ├── sensor_base.hpp               # SensorBase模板基类
│   ├── sensor_manager.hpp            # 传感器管理器（单例）
│   └── event_bus.hpp                 # EventBus发布-订阅（单例）
│
├── distribution/
│   ├── sensor_data.hpp               # 数据结构重导出
│   ├── data_subscriber.hpp           # 订阅者接口+内置实现
│   └── data_publisher.hpp            # 发布者便捷包装
│
├── exception/
│   ├── exception_types.hpp           # 异常分类/分级/错误码
│   ├── exception_detector.hpp        # 3类异常检测器
│   ├── exception_handler.hpp         # 恢复策略映射
│   ├── circuit_breaker.hpp           # 熔断保护器（三态状态机）
│   ├── heartbeat_monitor.hpp         # 心跳监控
│   ├── exception_logger.hpp          # 环形缓冲区日志
│   └── safe_sensor_wrapper.hpp       # ★ 安全采样包装器
│
├── sensors/temperature/
│   ├── sht30.hpp                     # SHT30 I2C (±0.3°C)
│   ├── ds18b20.hpp                   # DS18B20 OneWire (±0.5°C)
│   └── ntc_thermistor.hpp            # NTC热敏电阻 ADC (±1°C)
│
├── app/
│   ├── gui_display.hpp               # GUI显示订阅者
│   ├── mqtt_sender.hpp               # MQTT上报订阅者
│   └── business_logic.hpp            # 告警/安全联动/PID控制器
│
├── test_framework/
│   ├── mock_hal.hpp                  # Mock I2C/Time/ADC
│   ├── test_fixture.hpp              # 测试夹具+断言宏
│   └── test_reporter.hpp             # 报告生成器(Text/JSON/JUnit)
│
├── fixed_point/
│   ├── q_math.hpp                    # Q15.16/Q7.8定点数库
│   ├── fixed_mean_filter.hpp         # 定点数均值滤波器
│   └── c_binding.h                   # C语言胶水层
│
└── tests/
    ├── whitebox/
    │   ├── test_filter.cpp           # 滤波器白盒测试(18用例)
    │   ├── test_event_bus.cpp        # EventBus白盒测试(4用例)
    │   ├── test_sensor_manager.cpp   # SensorManager白盒测试(6用例)
    │   └── test_exception.cpp        # 异常处理白盒测试(6用例)
    └── blackbox/
        └── test_sensor_pipeline.cpp  # 全链路黑盒集成测试(5用例)
```

---

## 5. Profile预设

### 5.1 四档Profile对比

| Profile | ROM | RAM | 滤波器 | EventBus | 异常监控 | 熔断 | 适用场景 |
|---------|-----|-----|--------|----------|----------|------|----------|
| **Minimal** | <2KB | <512B | 0 | ❌ | ❌ | ❌ | 超低功耗传感器节点 |
| **Standard** | <8KB | <2KB | 均值+低通 | ✅ | 通信+数据 | ❌ | 常规IoT终端 |
| **Advanced** | <16KB | <4KB | 全部8种 | ✅ | 完整3类 | ✅ | 工业控制器/网关 |
| **Minimal-8Bit** | <2KB | <256B | 定点均值 | ❌ | ❌ | ❌ | 8位AVR/PIC/STM8 |
| **Custom** | 按需 | 按需 | 自定义 | 自定义 | 自定义 | 自定义 | 特殊需求 |

### 5.2 使用方式

```bash
# CMake方式
cmake .. -DSENSOR_PROFILE=STANDARD

# 直接定义宏方式
g++ -DSENSOR_PROFILE_STANDARD -std=c++11 ...

# 代码中检测
#include "sensor_profile.hpp"
// SENSOR_PROFILE_STANDARD 宏已定义
```

### 5.3 自定义Profile

```cpp
// feature_config.hpp 中手动配置
#define SENSOR_FEATURE_FILTER_MEAN       1
#define SENSOR_FEATURE_FILTER_MEDIAN     1
#define SENSOR_FEATURE_FILTER_LOWPASS    0   // 关闭不需要的
#define SENSOR_FEATURE_FILTER_KALMAN     0
#define SENSOR_FEATURE_EVENTBUS          1
#define SENSOR_FEATURE_EXCEPTION         0   // 关闭异常监控
```

---

## 6. 核心模块详解

### 6.1 编译时特性配置

框架的核心裁剪机制，每个模块通过 `SENSOR_FEATURE_XXX` 宏控制编译。

**关键文件**: `feature_config.hpp`

**条件编译模式**:
```cpp
// 头文件级别裁剪
#if SENSOR_FEATURE_FILTER_KALMAN
class KalmanFilter : public IFilter { /* 完整实现 */ };
#else
struct KalmanFilter {
    // 静态断言：防止误用
    KalmanFilter(float, float) {
        static_assert(false, "Enable SENSOR_FEATURE_FILTER_KALMAN");
    }
};
#endif

// 函数体内裁剪（零开销）
template <typename TData>
TData SensorBase<TData>::readFiltered() {
    TData raw = readRaw();
#if SENSOR_FEATURE_FILTER_PIPELINE
    if (filterPipeline_) {
        raw.value = filterPipeline_->update(raw.value);
    }
#endif
    return raw;
}
```

**裁剪验证**:
```bash
# 检查Minimal Profile下无卡尔曼滤波符号
nm build/minimal/sensor_framework.elf | grep -i "kalman"
# 期望输出：空（零残留）

# 对比各Profile的ROM占用
arm-none-eabi-size build/*/sensor_framework.elf
```

### 6.2 滤波引擎

#### 6.2.1 IFilter统一接口

```cpp
class IFilter {
public:
    virtual float update(float input) = 0;     // 输入→滤波→输出
    virtual void reset() = 0;                  // 重置状态
    virtual const char* getName() const = 0;    // 名称标识
    virtual FilterType getType() const = 0;     // 类型枚举
    virtual bool setParameter(const char* key, float value);   // 参数设置
    virtual float getParameter(const char* key) const;         // 参数查询
};
```

#### 6.2.2 8种滤波器对比

| 滤波器 | 复杂度 | 内存 | 延迟 | 抗脉冲 | 平滑度 | 跟踪速度 | 最佳场景 |
|--------|--------|------|------|--------|--------|----------|----------|
| **均值** | O(1) | N×4B | (N-1)/2周期 | 差 | 中 | 慢 | 周期性噪声抑制 |
| **中值** | O(N log N) | N×4B | (N-1)/2周期 | 优 | 中 | 慢 | 脉冲毛刺 |
| **低通** | O(1) | 8B | — | 中 | 优(可调) | 中(可调) | 通用慢变化信号 |
| **卡尔曼** | O(1) | 20B | — | 中 | 自适应 | 优 | 已知噪声模型 |
| **加权均值** | O(N) | N×4B | (N-1)/2周期 | 中 | 中 | 中 | 兼顾平滑+响应 |
| **FIR** | O(N) | N×8B | (N-1)/2周期 | 好 | 优 | 慢 | 精确频域特性 |
| **截尾均值** | O(M log M) | M×4B | (M-1)/2周期 | 优 | 好 | 慢 | 体育评分/抗异常值 |

#### 6.2.3 滑动窗口框架

将"窗口管理"与"算子逻辑"分离的设计：

```cpp
template <uint8_t N>
class SlidingWindowFilter : public IFilter {
public:
    float update(float input) final {
        buffer_[writeIdx_ % N] = input;
        writeIdx_++;
        if (writeIdx_ < N) return input;  // 窗口未满透传
        return apply();                    // 委托子类算子
    }
protected:
    virtual float apply() = 0;  // ★ 子类实现具体算法
    void getSortedCopy(float* out) const;  // 供排序算子使用
};
```

**基于滑动窗口实现的滤波器**: MeanFilter, MedianFilter, WeightedMeanFilter, TrimmedMeanFilter

#### 6.2.4 滤波器Pipeline

```cpp
// 温度传感器推荐滤波链
FilterPipeline<4> pipeline;
pipeline.addFilter(new MedianFilter<5>());       // 第1级: 去脉冲
pipeline.addFilter(new TrimmedMeanFilter<7,1>()); // 第2级: 截尾均值
pipeline.addFilter(new LowPassFilter(0.1f));      // 第3级: 平滑
pipeline.addFilter(new KalmanFilter(0.01f,0.5f)); // 第4级: 最优估计

// 运行时增删
pipeline.removeFilter("Median");
pipeline.setFilterEnabled("Kalman", false);
```

#### 6.2.5 截尾均值滤波 (Olympic Average)

```
窗口原始值: [25.1, 24.8, 99.9, 25.0, 25.3, 0.1, 25.2]
    排序后:  [0.1, 24.8, 25.0, 25.1, 25.2, 25.3, 99.9]
    └─trim─┘                              └─trim─┘
    保留:                      [25.0, 25.1, 25.2]
    结果:                      mean = 25.1  (去除了异常高/低值)
```

### 6.3 框架核心

#### 6.3.1 SensorBase模板基类

```cpp
template <typename TData>
class SensorBase {
public:
    // ─── 生命周期（纯虚方法，子类必须实现） ───
    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    // ─── 数据获取 ───
    virtual TData readRaw() = 0;
    virtual TData readFiltered();   // Raw → FilterPipeline → Output

    // ─── 配置 ───
    virtual bool configure(const SensorConfig& cfg);

    // ─── 滤波管理 ───
    void setFilterPipeline(FilterPipeline<>* pipeline);
    bool addFilter(IFilter* filter);

    // ─── 状态查询 ───
    SensorStatus getStatus() const;
    uint32_t getSensorId() const;
};
```

#### 6.3.2 SensorManager 单例

```cpp
auto& manager = SensorManager<>::getInstance();

// 注册
SHT30Sensor* sensor = new SHT30Sensor(0x01, i2cBus);
manager.registerSensor(sensor);

// 查找
SensorBase<SensorData>* found = manager.findSensorById(0x01);
SensorBase<SensorData>* temp = manager.findSensorByType(SensorType::TEMPERATURE);

// 批量操作
manager.initAll();    // 初始化全部传感器
manager.startAll();   // 启动全部
manager.sampleAll();  // 全部采样一次
manager.stopAll();    // 停止全部
```

### 6.4 事件总线

#### 6.4.1 发布-订阅模式

```cpp
auto& bus = EventBus<>::getInstance();

// 订阅
bus.subscribe(sensorId, &mySubscriber);

// 发布
SensorData data = sensor.readFiltered();
bus.publish(data);

// 取消订阅
bus.unsubscribe(sensorId, &mySubscriber);
bus.unsubscribeAll(sensorId);
```

#### 6.4.2 内置订阅者

```cpp
// 函数回调订阅者（快速绑定）
LambdaSubscriber sub("MySub", [](const SensorData& d, void* ctx) {
    printf("Value: %.2f\n", d.value);
});

// 过滤订阅者（仅当值超过阈值）
FilteredSubscriber filtered(&sub, thresholdExceededFilter);
```

#### 6.4.3 裁剪行为

当 `SENSOR_FEATURE_EVENTBUS=0` 时，`EventBus::publish()` 变为空操作，`subscribe/unsubscribe` 保持接口兼容，上层代码无需修改。

### 6.5 异常监控

#### 6.5.1 异常分类体系

| 类别 | 检测内容 | 典型错误 |
|------|----------|----------|
| **通信异常** | I2C NACK / SPI超时 / OneWire无应答 / CRC失败 | ERR_I2C_NACK |
| **数据异常** | 超出量程 / 数据冻结 / 突变(Spike) | ERR_OUT_OF_RANGE |
| **滤波异常** | 输出NaN/Inf / 滤波发散 | ERR_FILTER_NAN |
| **系统异常** | 内存不足 / 栈溢出 / 看门狗 | ERR_SYS_OOM |

#### 6.5.2 熔断保护器 三态状态机

```
CLOSED(正常) → 连续失败超阈值 → OPEN(熔断)
OPEN(熔断)  → 冷却时间结束   → HALF_OPEN(探测)
HALF_OPEN  → 探测成功       → CLOSED(恢复)
HALF_OPEN  → 探测失败       → OPEN(重新熔断)
```

#### 6.5.3 SafeSensorWrapper — 安全采样入口

```cpp
SafeSensorWrapper<4> safeSensor;
safeSensor.bindSensor(&mySensor);
safeSensor.addDetector(new CommExceptionDetector(3, 100));
safeSensor.addDetector(new DataExceptionDetector(-40, 125, 30, 10000));
safeSensor.setHandler(new ExceptionHandler(25.0f));

// 安全读取 — 自动执行: 熔断检查 → 采样 → 异常检测 → 降级/恢复
SensorData data = safeSensor.safeRead();
// 异常时 data.value 可能为上次有效值或默认值
```

### 6.6 硬件抽象层

#### 6.6.1 接口定义

```cpp
class II2CBus {
    virtual bool write(uint8_t addr, const uint8_t* data, size_t len) = 0;
    virtual bool read(uint8_t addr, uint8_t* data, size_t len) = 0;
    virtual bool writeRead(uint8_t addr, const uint8_t* tx, size_t txLen,
                           uint8_t* rx, size_t rxLen) = 0;
};

class ISPIBus {
    virtual bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) = 0;
    virtual void setCsLevel(bool high) = 0;
};

class IADCChannel {
    virtual float readVoltage() = 0;
    virtual uint16_t readRaw() = 0;
};

class ITimeProvider {
    virtual uint32_t getTickMs() = 0;
    virtual void delayMs(uint32_t ms) = 0;
};

class IMutex {
    virtual void lock() = 0;
    virtual void unlock() = 0;
};
```

#### 6.6.2 总线包装器

框架在HAL接口之上提供了带重试/超时的包装器：

```cpp
I2CBusWrapper i2c(rawI2CBus, /*timeout=*/50);
// 写入16位命令并读取数据（自动重试3次）
i2c.writeCmd16Read(0x44, 0x2C06, rxBuf, 6, /*maxRetries=*/3);

SPIBusWrapper spi(rawSPIBus);
spi.writeRegister(0x20, data, 4);  // CS自动管理

ADCChannelWrapper adc(rawADC);
float avgVoltage = adc.readVoltageAveraged(8);  // 8次采样取平均
```

### 6.7 温度传感器

#### 6.7.1 支持的驱动芯片

| 芯片 | 接口 | 精度 | 量程 | 实现文件 |
|------|------|------|------|----------|
| SHT30 | I2C (0x44/0x45) | ±0.3°C | -40~125°C | `sht30.hpp` |
| DS18B20 | OneWire | ±0.5°C | -55~125°C | `ds18b20.hpp` |
| NTC热敏电阻 | ADC | ±1°C | -40~105°C | `ntc_thermistor.hpp` |

#### 6.7.2 SHT30使用

```cpp
SHT30Sensor sht30(0x01, i2cBus1, 0x44);
sht30.init();
sht30.start();

// 仅读温度
TemperatureData temp = sht30.readRaw();

// 同时读温湿度
float humidity;
TemperatureData temp2 = sht30.readBoth(humidity);
```

#### 6.7.3 DS18B20使用

```cpp
DS18B20Sensor ds18b20(0x02, oneWireBus);
ds18b20.init();
ds18b20.setResolution(DS18B20Sensor::RES_12_BIT);  // 最高精度 (750ms)
ds18b20.start();

TemperatureData temp = ds18b20.readRaw();
// 转换时间: 9位=94ms, 10位=188ms, 11位=375ms, 12位=750ms
```

#### 6.7.4 NTC热敏电阻使用

```cpp
NTCThermistorSensor::NTCParams params;
params.rFixed = 10000.0f;  // 10kΩ分压电阻
params.r0     = 10000.0f;  // 25°C时阻值
params.bValue = 3950.0f;   // B值

NTCThermistorSensor ntc(0x03, adcChannel, params);
ntc.init();
ntc.start();

TemperatureData temp = ntc.readRaw();
```

### 6.8 定点数数学库

#### 6.8.1 Q15.16格式

```
编码:  value_int32 = round(real × 2^16)
范围:  [-32768.0, 32767.99998]
精度:  1/65536 ≈ 0.000015
内存:  4字节（与float相同）

示例:
  float 1.5  → Q15.16 = 98304 (0x18000)
  float π    → Q15.16 = 205887
  Q15.16 65536 → float = 1.0
```

#### 6.8.2 运算API

```cpp
// C风格函数（轻量）
q15_16_t a = qmath::fromFloat(1.5f);
q15_16_t b = qmath::fromFloat(2.0f);
q15_16_t c = qmath::mul(a, b);    // 1.5 * 2.0 = 3.0 (用int64_t防溢出)
float result = qmath::toFloat(c); // → 3.0

// C++操作符重载类（自然语法）
Q15_16 a(1.5f);
Q15_16 b(2.0f);
Q15_16 c = a * b;  // 完全像float一样使用

// 窗口统计
q15_16_t avg = qmath::mean(buffer, count);       // 定点数均值
q15_16_t med = qmath::median(sorted, count);     // 定点数中值
q15_16_t tri = qmath::trimmedMean(data, 7, 1);   // 奥林匹克平均
```

#### 6.8.3 定点数滤波器

```cpp
FixedMeanFilter<4> filter;  // 4点定点数均值滤波
float output = filter.update(25.5f);  // 接口保持float一致
// 内部全整数运算，无浮点软库开销
```

### 6.9 测试框架

#### 6.9.1 Mock HAL

```cpp
MockI2CBus mockI2c;
MockTimeProvider mockTime;

// 预设传感器响应
uint8_t sht30Response[] = {0x66, 0x66, 0xA3};
mockI2c.setDeviceResponse(0x44, sht30Response, 3);

// 注入错误
mockI2c.setNackOnAddress(0x44);  // 模拟I2C通信失败

// 控制时间
mockTime.advance(100);  // 推进100ms

// 验证写入历史
const auto& record = mockI2c.getWriteRecord(0);
assert(record.addr == 0x44);
```

#### 6.9.2 测试用例编写

```cpp
#include "test_framework/test_fixture.hpp"

TEST(Filter, MeanFilter_BasicAverage) {
    MeanFilter<4> filter;
    filter.update(20.0f);
    filter.update(30.0f);
    filter.update(40.0f);
    float result = filter.update(30.0f);

    TEST_ASSERT_FLOAT_EQ(result, 30.0f, 0.1f, "Mean of [20,30,40,30] should be 30");
    return true;
}

// 运行所有测试
int main() {
    runAllTests();
    printTestSummary();
    return (getTestStats().failed > 0) ? 1 : 0;
}
```

---

## 7. API参考

### 7.1 传感器管理API

| 方法 | 说明 |
|------|------|
| `SensorManager::getInstance()` | 获取单例 |
| `registerSensor(sensor*)` | 注册传感器（接管所有权） |
| `unregisterSensor(id)` | 注销并销毁传感器 |
| `findSensorById(id)` | 按ID查找 |
| `findSensorByType(type)` | 按类型查找 |
| `initAll()` | 批量初始化 |
| `startAll()` | 批量启动 |
| `stopAll()` | 批量停止 |
| `sampleAll(callback)` | 批量采样+回调 |
| `destroyAll()` | 销毁全部 |

### 7.2 EventBus API

| 方法 | 说明 |
|------|------|
| `EventBus::getInstance()` | 获取单例 |
| `subscribe(sensorId, subscriber*)` | 订阅 |
| `unsubscribe(sensorId, subscriber*)` | 取消订阅 |
| `publish(SensorData)` | 发布数据 |
| `unsubscribeAll(sensorId)` | 取消全部订阅 |

### 7.3 滤波器API

```cpp
float update(float input);                      // 核心接口
void reset();                                   // 重置
const char* getName() const;                    // 名称
FilterType getType() const;                     // 类型枚举
bool setParameter("key", value);                // 动态调参
float getParameter("key") const;                // 参数查询
```

**各滤波器参数**:

| 滤波器 | 参数Key | 说明 |
|--------|---------|------|
| Mean/Median/TrimmedMean | `"N"` | 窗口大小（只读） |
| LowPass | `"a"` | alpha系数 (0, 1] |
| Kalman | `"Q"` `"R"` | 过程噪声/测量噪声 |
| TrimmedMean | `"m"` | 运行时截尾数 |
| FIR | — | 通过 `setCoefficients()` |

### 7.4 异常处理API

```cpp
// 熔断器
CircuitBreaker cb(5, 30000);     // 5次失败→熔断, 30s冷却
cb.recordSuccess();
cb.recordFailure();
bool ok = cb.allowRequest();      // 是否允许请求
State state = cb.getState();      // CLOSED/OPEN/HALF_OPEN

// 异常日志
ExceptionLogger<64> logger;
logger.log(event);
const ExceptionEvent& last = logger.last();
uint8_t count = logger.getRecent(out, 10, ExceptionSeverity::WARNING);

// 安全包装器
SafeSensorWrapper<4> safe;
safe.bindSensor(&sensor);
safe.addDetector(new DataExceptionDetector(...));
SensorData data = safe.safeRead();  // 自动保护
```

---

## 8. 平台移植指南

### 8.1 移植步骤（32位MCU）

1. **实现HAL接口**:
```cpp
class MySTM32I2C : public II2CBus {
    bool write(uint8_t addr, const uint8_t* data, size_t len) override {
        return HAL_I2C_Master_Transmit(&hi2c1, addr<<1, (uint8_t*)data, len, 100) == HAL_OK;
    }
    // ... 其他方法
};
```

2. **配置平台宏**:
```cpp
#define MCU_STM32
#include "hal/platform/platform_selector.hpp"
// 自动设置 SENSOR_PLATFORM_NAME = "STM32"
```

3. **选择Profile**:
```bash
cmake .. -DSENSOR_PROFILE=ADVANCED -DMCU_STM32=ON
```

### 8.2 移植到8位MCU

8位MCU需额外注意：

1. **启用定点数数学库**: `#define SENSOR_FEATURE_FIXED_POINT_MATH 1`
2. **使用Minimal-8Bit Profile**: `-DSENSOR_PROFILE=MINIMAL_8BIT`
3. **uint8_t模板参数**: 窗口大小等使用 `uint8_t` 而非 `size_t`
4. **禁止堆分配**: 全部使用 `StaticVector` / `RingBuffer`
5. **可选C胶水层**: SDCC/XC8不支持C++，使用 `c_binding.h`

### 8.3 各平台HAL映射

| 抽象接口 | STM32 | ESP32 | AVR | MSP430 |
|----------|-------|-------|-----|--------|
| `ITimeProvider::getTickMs()` | `HAL_GetTick()` | `esp_timer_get_time()/1000` | `millis()` | `TA0CCR0` |
| `IMutex::lock()` | `osMutexAcquire()` | `xSemaphoreTake()` | `cli()` | `__disable_interrupt()` |
| `II2CBus::write()` | `HAL_I2C_Master_Transmit()` | `i2c_master_write_to_device()` | TWI寄存器 | USCI-B I2C |
| `IADCChannel::readRaw()` | `HAL_ADC_GetValue()` | `adc1_get_raw()` | ADC寄存器 | ADC12MEM0 |

---

## 9. 裁剪指南

### 9.1 按模块裁剪

```cpp
// feature_config.hpp

// 关闭不使用的总线
#define SENSOR_FEATURE_HAL_SPI        0
#define SENSOR_FEATURE_HAL_ONEWIRE    0

// 关闭不使用的滤波器
#define SENSOR_FEATURE_FILTER_KALMAN  0
#define SENSOR_FEATURE_FILTER_FIR     0

// 关闭不使用的传感器驱动
#define SENSOR_FEATURE_DS18B20        0
#define SENSOR_FEATURE_NTC            0

// 关闭异常监控（省~3KB ROM）
#define SENSOR_FEATURE_EXCEPTION      0
```

### 9.2 裁剪效果验证

```bash
# 构建各Profile
cmake .. -DSENSOR_PROFILE=MINIMAL && make
arm-none-eabi-size sensor_framework.elf  # text < 2KB

cmake .. -DSENSOR_PROFILE=STANDARD && make
arm-none-eabi-size sensor_framework.elf  # text < 8KB

# 验证零残留
nm sensor_framework_minimal.elf | grep -i "kalman"     # 期望空
nm sensor_framework_minimal.elf | grep -i "circuit"    # 期望空
```

### 9.3 资源优化技巧

| 技巧 | 效果 | 适用场景 |
|------|------|----------|
| 用 `uint8_t` 替代 `size_t` | 省1-2B/变量 | 8位平台 |
| 滑动窗口用插入排序 | 省栈空间 | 小窗口N≤32 |
| 禁止异常+RTTI | 省代码 | 所有嵌入式平台 |
| 定点数替代浮点 | 省软浮点库(~2KB) | 无FPU平台 |

---

## 10. 滤波器选型指南

### 10.1 按信号特征选择

| 信号特征 | 推荐滤波器 | Pipeline组合建议 |
|----------|------------|-------------------|
| 偶发脉冲毛刺 | 中值(5点) | 中值 → 低通 |
| 随机白噪声 | 均值(8点) + 低通 | 均值 → 卡尔曼 |
| 工频50/60Hz干扰 | FIR(16阶陷波) | FIR → 低通 |
| 传感器自身噪声大 | 卡尔曼(Q小,R大) | 卡尔曼（单级即可） |
| 偶发异常值 | 截尾均值(7选5) | 截尾 → 低通 |
| 需要同时快响应+平滑 | 卡尔曼(Q=0.1) | 卡尔曼 |

### 10.2 温度传感器推荐配置

```cpp
// 通用场景（室温监控）
config.filter_pipeline = {TRIMMED_MEAN(7,1), LOW_PASS(0.1)};

// 高精度场景（实验室）
config.filter_pipeline = {MEDIAN(5), KALMAN(0.01,0.1)};

// 快速响应场景（PID控制）
config.filter_pipeline = {LOW_PASS(0.3)};  // 单级低通

// 极端噪声环境（工厂车间）
config.filter_pipeline = {TRIMMED_MEAN(9,2), MEDIAN(5), KALMAN(0.01,0.5)};

// 8位MCU极简场景
config.filter_pipeline = {FIXED_MEAN(4)};   // 4点定点均值
```

---

## 11. 异常处理策略

### 11.1 四级严重度

| 级别 | 名称 | 触发条件 | 自动动作 |
|------|------|----------|----------|
| L0 INFO | 通知 | 偶发重试成功 | 仅日志记录 |
| L1 WARNING | 警告 | 连续2-3次重试成功 | 日志 + EventBus通知 |
| L2 ERROR | 错误 | 连续5次失败/数据越界 | 降级运行 + 使用缓存值 |
| L3 FATAL | 致命 | 熔断触发/硬件损坏 | 停止采样 + 系统告警 |

### 11.2 恢复策略

```
检测到异常
    │
    ├─ L0/L1 → 记录日志 → 继续运行
    │
    ├─ L2(通信) → 自动重试 → 成功? → 恢复
    │                        └── 失败 → 降级(使用缓存值)
    │
    ├─ L2(数据) → 使用上次有效值 → 累计 → 触发传感器复位
    │
    └─ L3 → 熔断 → 停止采样 → 冷却30s → 探测 → 恢复/保持熔断
```

### 11.3 熔断参数配置建议

| 场景 | failureThreshold | cooldownMs | 说明 |
|------|-----------------|------------|------|
| 关键传感器 | 3 | 10000 | 快速熔断, 快速恢复 |
| 普通传感器 | 5 | 30000 | 默认值 |
| 非关键传感器 | 10 | 60000 | 容忍更多失败 |
| 8位MCU | 3 | 30000 | 资源有限 |

---

## 12. 测试指南

### 12.1 运行测试

```bash
# 构建并运行全部测试
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure

# 运行单个测试
./test_filter

# 生成覆盖率报告
gcovr --xml coverage.xml
```

### 12.2 编写新测试

```cpp
#include "test_framework/test_fixture.hpp"

TEST(MySuite, MyTestCase) {
    // Arrange
    MyClass obj;

    // Act
    bool result = obj.doSomething();

    // Assert
    TEST_ASSERT_TRUE(result, "Should succeed");
    TEST_ASSERT_EQ(obj.getValue(), 42, "Value should be 42");

    return true;
}

// 自动注册，无需手动添加
```

### 12.3 Mock HAL使用

```cpp
// 准备Mock
MockI2CBus mockI2c;
uint8_t response[] = {0x66, 0x66, 0xA3};
mockI2c.setDeviceResponse(0x44, response, 3);

// 测试传感器驱动
SHT30Sensor sensor(1, mockI2c);
sensor.init();
auto data = sensor.readRaw();

// 验证
TEST_ASSERT_TRUE(data.status == SensorStatus::RUNNING, "Sensor should be running");

// 注入故障
mockI2c.setNackOnAddress(0x44);
auto errorData = sensor.readRaw();
TEST_ASSERT_TRUE(errorData.status == SensorStatus::COMM_ERROR, "Should detect NACK");
```

---

## 13. FAQ

### Q1: 如何新增一种传感器类型？

1. 继承 `SensorBase<YourDataType>`
2. 实现 `init()` / `start()` / `stop()` / `readRaw()`
3. 在 `SensorType` 枚举中注册新类型（或使用 `CUSTOM_BASE`）
4. 在 `feature_config.hpp` 添加对应的 `SENSOR_FEATURE_YOUR_SENSOR` 开关

### Q2: 新增一种滤波器？

1. 继承 `IFilter`（无窗口）或 `SlidingWindowFilter<N>`（有窗口）
2. 实现 `getName()` / `getType()` / `apply()`（如果是滑动窗口）
3. 在 `FilterType` 枚举中注册（或使用 `CUSTOM_BASE`）
4. 在 `feature_config.hpp` 添加开关

### Q3: 如何在Bare-metal (无RTOS) 环境使用？

- `IMutex` 实现为 `cli()/sei()` 关全局中断
- 采样用定时器ISR触发
- 确保所有缓冲区静态分配
- 禁用 `SENSOR_FEATURE_HEARTBEAT`（心跳需要RTOS定时器）

### Q4: 内存不够怎么进一步优化？

- 减小 `SENSOR_CONSTRAINT_MAX_WINDOW`（如从32降到8）
- 减小 `SENSOR_CONSTRAINT_MAX_FILTERS`（Pipeline级数）
- 减小 `SENSOR_CONSTRAINT_MAX_SENSORS` / `MAX_SUBSCRIBERS`
- 用 `FixedMeanFilter` 替代 `MeanFilter`（定点数节省浮点库）
- 使用 Minimal Profile

### Q5: 如何调试裁剪后仍有残留符号的问题？

```bash
# 检查目标符号
nm build/sensor_framework.elf | grep -i "kalman"

# 如果仍有残留，检查是否有死代码路径引用了被裁剪的模块
# 确保使用 #if FEATURE_XXX 包裹，而不是普通的 if()
```

### Q6: 定点数精度不够怎么办？

- 使用 Q15.16（32-bit）代替 Q7.8（16-bit）
- 关键路径用 `int64_t` 中间结果避免累积误差
- 乘法后立即右移16位防止溢出
- 长Pipeline考虑每N级用float校准一次

### Q7: C++11约束下如何优雅处理？

框架已内置兼容宏，直接使用即可：
- `SENSOR_MAKE_UNIQUE(T, args...)` 替代 `std::make_unique`
- `SENSOR_IF_CONSTEXPR(cond)` 替代 `if constexpr`
- `SENSOR_NODISCARD` 替代 `[[nodiscard]]`
- 使用 `typedef` 而非 `using` 声明类型别名
- 使用 `.hpp` 头文件扩展名

---

> **文档版本**: v1.0  
> **基于框架版本**: v1.0.0  
> **维护者**: 开发团队  
> **最后更新**: 2026-07-11
