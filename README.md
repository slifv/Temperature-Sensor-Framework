# Sensor Framework — 使用说明

> 跨MCU平台传感器数据采集框架 | C++11 | 8/16/32位全平台支持

---

## 快速开始

### 1. 选择Profile

```bash
# 标准配置（默认推荐）
cmake .. -DSENSOR_PROFILE=STANDARD

# 全功能配置
cmake .. -DSENSOR_PROFILE=ADVANCED

# 极简配置（<2KB ROM）
cmake .. -DSENSOR_PROFILE=MINIMAL

# 8位MCU配置
cmake .. -DSENSOR_PROFILE=MINIMAL_8BIT
```

### 2. 包含头文件

```cpp
#include "sensor_framework.hpp"
```

### 3. 5分钟示例：温度监控

```cpp
#include "sensor_framework.hpp"

int main() {
    // 1. 创建SHT30温度传感器
    SHT30Sensor tempSensor(0x01, i2cBus1, 0x44);

    // 2. 配置滤波：中值去毛刺 → 低通平滑
    tempSensor.addFilter(new MedianFilter<5>());
    tempSensor.addFilter(new LowPassFilter(0.1f));

    // 3. 初始化并启动（10Hz采样）
    tempSensor.init();
    tempSensor.start();

    // 4. 订阅数据到GUI显示和告警
    auto& bus = EventBus<>::getInstance();
    GuiDisplaySubscriber gui("LCD", &myDisplayFunc);
    AlarmSubscriber alarm("Alarm", 35.0f, 15.0f);
    bus.subscribe(0x01, &gui);
    bus.subscribe(0x01, &alarm);

    // 5. 主循环：每100ms采样分发
    while (true) {
        SensorData data = tempSensor.readFiltered();
        bus.publish(data);
        delay(100);
    }
}
```

---

## 构建与测试

### 前置条件

- **CMake** ≥ 3.10
- **C++11 编译器**: GCC / Clang / MSVC
- **Windows 推荐**: 安装 [MSYS2](https://www.msys2.org/) (`pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake`)

### 构建

```bash
cd sensor_framework
mkdir build && cd build

# 配置（ADVANCED profile 包含全部滤波器，适合测试）
cmake .. -G "MinGW Makefiles" \
  -DSENSOR_PROFILE=ADVANCED \
  -DBUILD_TESTS=ON \
  -DSENSOR_NO_EXCEPTIONS=OFF \
  -DSENSOR_NO_RTTI=OFF

# 编译（使用所有CPU核心）
mingw32-make -j$(nproc)
```

> **Linux/macOS**: 使用 `make` 替代 `mingw32-make`，省略 `-G` 参数。
>
> **MSVC**: 使用 `-G "Visual Studio 17 2022"`，`cmake --build . --config Release`。

### 运行测试

```bash
# 运行全部测试（带失败详情）
ctest --output-on-failure

# 或单独运行
./test_filter.exe
./test_event_bus.exe
./test_sensor_manager.exe
./test_exception.exe
./test_sensor_pipeline.exe
```

### Profile 选项

| Profile | 命令 | 滤波器 | 事件总线 | 异常监控 |
|---------|------|--------|---------|---------|
| **MINIMAL** | `-DSENSOR_PROFILE=MINIMAL` | — | — | — |
| **STANDARD** | `-DSENSOR_PROFILE=STANDARD` | 均值+低通 | ✅ | 基础 |
| **ADVANCED** | `-DSENSOR_PROFILE=ADVANCED` | 全8种 | ✅ | 全功能 |
| **MINIMAL_8BIT** | `-DSENSOR_PROFILE=MINIMAL_8BIT` | 定点均值 | — | — |

---

## 核心概念

### 架构分层

```
应用层      GUI显示 · MQTT上报 · 业务逻辑(告警/PID)
   ↕ EventBus (发布-订阅)
核心层      SensorManager · EventBus · 异常监控
   ↕
滤波引擎    8种滤波器 + Pipeline级联
   ↕
HAL层      I2C · SPI · ADC · OneWire (平台抽象)
   ↕
平台驱动    STM32 · ESP32 · nRF52 · GD32 · AVR · MSP430
```

### 4个Profile

| Profile | ROM | 场景 |
|---------|-----|------|
| **Minimal** (<2KB) | 原始采集 | 超低功耗节点 |
| **Standard** (<8KB) | 基础滤波+事件总线 | 常规IoT |
| **Advanced** (<16KB) | 全部功能 | 工业控制器 |
| **Minimal-8Bit** (<2KB) | 1路定点滤波 | 8位AVR/PIC |

### 8种滤波器

| 滤波器 | 一句话描述 |
|--------|-----------|
| **均值** | 窗口平均，抑制周期性噪声 |
| **中值** | 排序取中间，去脉冲毛刺 |
| **低通** | α平滑，慢变化信号首选 |
| **卡尔曼** | 最优估计，自适应噪声 |
| **加权均值** | 近期权重大，兼顾平滑+响应 |
| **FIR** | 线性相位，精确频域特性 |
| **截尾均值** | 去最高最低取平均，体育评分算法 |
| **定点均值** | 纯整数运算，无FPU平台 |

---

## 常用操作

### 传感器管理

```cpp
auto& mgr = SensorManager<>::getInstance();

// 注册
mgr.registerSensor(new SHT30Sensor(0x01, i2cBus));

// 查找
auto* sensor = mgr.findSensorById(0x01);

// 批量操作
mgr.initAll();
mgr.startAll();
mgr.sampleAll([](const SensorData& d) { /* 处理 */ });
```

### 滤波配置

```cpp
// 方式1: Pipeline组合
tempSensor.addFilter(new MedianFilter<5>());
tempSensor.addFilter(new KalmanFilter(0.01f, 0.5f));

// 方式2: 手动Pipeline
FilterPipeline<4> pipeline;
pipeline.addFilter(new TrimmedMeanFilter<7, 1>());
pipeline.addFilter(new LowPassFilter(0.15f));
float result = pipeline.update(rawValue);
```

### 事件总线

```cpp
auto& bus = EventBus<>::getInstance();

// 订阅
bus.subscribe(sensorId, &subscriber);

// 发布
bus.publish(sensorData);

// 取消
bus.unsubscribe(sensorId, &subscriber);
```

### 异常保护

```cpp
SafeSensorWrapper<4> safeSensor;
safeSensor.bindSensor(&mySensor);
safeSensor.addDetector(new CommExceptionDetector(3, 100));
safeSensor.addDetector(new DataExceptionDetector(-40, 125, 30, 10000));

SensorData data = safeSensor.safeRead();  // 自动检测→降级→恢复
```

---

## 支持平台

| 平台 | MCU | Profile推荐 |
|------|-----|-------------|
| ARM Cortex-M4/M7 | STM32F4/H7 | Advanced |
| Xtensa | ESP32-S3 | Advanced |
| ARM Cortex-M4 | nRF52840 | Advanced |
| ARM Cortex-M0+ | STM32G0 | Standard |
| MSP430 | MSP430FR | Minimal-8Bit |
| AVR 8-bit | ATmega328P | Minimal-8Bit |

---

## 模块裁剪

在 `feature_config.hpp` 中将不需要的模块设为 `0`：

```cpp
#define SENSOR_FEATURE_FILTER_KALMAN      0   // 关闭卡尔曼滤波
#define SENSOR_FEATURE_FILTER_FIR         0   // 关闭FIR
#define SENSOR_FEATURE_EXCEPTION          0   // 关闭异常监控
#define SENSOR_FEATURE_DS18B20            0   // 关闭DS18B20驱动
```

裁剪后相关代码和数据零残留。

---

## 目录结构

```
sensor_framework/
├── sensor_framework.hpp         # 统一入口
├── CMakeLists.txt               # 构建配置
├── compiler_features.hpp        # 编译器检测
├── feature_config.hpp           # 特性裁剪开关 ★
├── sensor_profile.hpp           # Profile预设
├── utils/                       # 工具类
├── hal/                         # 硬件抽象层
├── filter/                      # 滤波引擎(10个文件)
├── core/                        # 框架核心(4个文件)
├── distribution/                # 数据分发
├── exception/                   # 异常监控(7个文件)
├── sensors/temperature/         # 温度传感器(3款)
├── app/                         # 应用层订阅者
├── test_framework/              # 测试支撑框架
├── fixed_point/                 # 定点数数学库
└── tests/                       # 测试用例(5个文件)
```

---

## 详细文档

完整Wiki文档请参见: [sensor_framework_analysis/Sensor_Framework_Wiki.md](sensor_framework_analysis/Sensor_Framework_Wiki.md)

需求分析报告: [sensor_framework_analysis/需求分析与评审报告.md](sensor_framework_analysis/需求分析与评审报告.md)

---

> Framework Version: v1.0.0 | C++11 | 2026-07-11
