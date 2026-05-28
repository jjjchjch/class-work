# 按键控制 LED 实验 · 代码位置与功能对应表

## 快速查询索引

### 原理图与硬件设计
- **完整原理图**：[KEY_Circuit_Schematic.md](KEY_Circuit_Schematic.md)
- **实验说明文档**：[KEY_Control_Experiment.md](KEY_Control_Experiment.md)

---

## 代码文件位置

### 1. 按键相关代码

| 功能 | 文件 | 位置 | 说明 |
|------|------|------|------|
| 按键定义 | [Core/Inc/key.h](../Core/Inc/key.h) | 第 9-21 行 | KEY1/KEY2/KEY3 宏定义，返回码定义 |
| 按键初始化 | [Core/Src/key.c](../Core/Src/key.c) | 第 16 行 | `void key_init()` |
| 按键扫描（去抖） | [Core/Src/key.c](../Core/Src/key.c) | 第 50 行 | `uint8_t key_scan(uint8_t mode)` |
| 中断初始化 | [Core/Src/key.c](../Core/Src/key.c) | 第 107 行 | `void key_exti_init()` 可选 |

### 2. LED 相关代码

| 功能 | 文件 | 位置 | 说明 |
|------|------|------|------|
| LED 定义 | [Core/Inc/main.h](../Core/Inc/main.h) | 第 63-76 行 | LED1/LED2/LED3 宏定义 |
| LED 初始化 | [Core/Src/gpio.c](../Core/Src/gpio.c) | 第 41 行 | `void LED_Init()` |
| LED 点亮 | [Core/Src/gpio.c](../Core/Src/gpio.c) | 第 26 行 | `void LED_On()` |
| LED 熄灭 | [Core/Src/gpio.c](../Core/Src/gpio.c) | 第 31 行 | `void LED_Off()` |
| LED 翻转 | [Core/Src/gpio.c](../Core/Src/gpio.c) | 第 36 行 | `void LED_Toggle()` |

### 3. 主程序控制代码

| 功能 | 文件 | 位置 | 说明 |
|------|------|------|------|
| 按键控制 LED | [Core/Src/main.c](../Core/Src/main.c) | 第 103 行 | `static void Key_Scan_And_LED_Control()` |
| 流水灯效果 | [Core/Src/main.c](../Core/Src/main.c) | 第 63 行 | `static void Board_LED_RunningLight()` |
| 闪烁效果 | [Core/Src/main.c](../Core/Src/main.c) | 第 84 行 | `static void Expansion_LED_Blink()` |
| 主循环 | [Core/Src/main.c](../Core/Src/main.c) | 第 163 行 | 按键扫描 + 定时任务 |

---

## 硬件引脚映射

### 开发板按键

```
KEY1: PA0  ────────┐
                   │ 主循环轮询扫描
KEY2: PA1  ────────┤ (key_scan)
                   │
KEY3: PA4  ────────┘
```

### 开发板 LED

```
LED1: PC5 ──┐
LED2: PB1 ──┼── 显示按键和自动效果
LED3: PB2 ──┘
```

### 扩展板按键

```
SW: PC4 (来自 EC11 编码器中心按钮)
```

---

## 功能流程图

### 按键→LED 控制链路

```
按键按下事件
    │
    ▼
key_scan() 轮询检测
    │
    ├─ 硬件抖动过滤（delay_ms 10ms）
    │
    └─ 返回按键编码值（KEY1_PRES/KEY2_PRES/KEY3_PRES/0）
           │
           ▼
Key_Scan_And_LED_Control() 处理
    │
    ├─ case KEY1_PRES → LED_Toggle(LED1)
    ├─ case KEY2_PRES → LED_Toggle(LED2)
    ├─ case KEY3_PRES → LED_Toggle(LED3)
    │
    ▼
LED 状态改变（亮↔灭）
```

### 自动任务调度

```
主循环（非阻塞）
    │
    ├─ Key_Scan_And_LED_Control()         每循环执行
    │
    ├─ Board_LED_RunningLight()           每 300ms 执行（KEY1/KEY2 交替）
    │
    └─ Expansion_LED_Blink()              每 500ms 执行（KEY3 闪烁）
```

---

## 编译验证

### 编译命令
```bash
cmake --build --preset Debug
```

### 编译输出
```
[2/2] Linking C executable TEST11.elf
Memory region         Used Size  Region Size  %age Used
             RAM:        1592 B       128 KB      1.21%
          CCMRAM:           0 B        64 KB      0.00%
           FLASH:        7056 B       512 KB      1.35%
```

✅ **编译状态**：成功，0 errors, 0 warnings

---

## 按键扫描状态机

### 状态转换图

```
                初始化
                 │
                 ▼
         key_up = 1
         （允许检测）
         
┌────────────────┴───────────────┐
│                                 │
▼                                 ▼
无按键按下          有按键被按下 (GPIO 电平匹配)
 │                              │
 │                              ▼
 │                         延时 10ms
 │                         (抖动过滤)
 │                              │
 │                              ▼
 │                         确认按键电平
 │                              │
 │        ┌────────────────────┘
 │        │
 │        ▼
 │    key_up = 0
 │    (防重复)
 │        │
 │        ▼
 │    返回按键码
 │    (KEY1_PRES/KEY2_PRES/KEY3_PRES)
 │        │
 │        ▼
 │    等待按键释放
 │        │
 └────────┤
          ▼
    按键全部释放
          │
          ▼
    key_up = 1
    (允许下一次检测)
          │
          └──────── 回到初始状态
```

---

## 关键参数配置

### 时间参数

| 参数 | 值 | 用途 |
|------|-----|------|
| 去抖动延时 | 10ms | 硬件抖动过滤 |
| 流水灯周期 | 300ms | LED1/LED2 切换间隔 |
| 闪烁周期 | 500ms | LED3 翻转间隔 |

### GPIO 配置

| 引脚 | 模式 | 上/下拉 | 触发类型 |
|------|------|--------|--------|
| PA0 (KEY1) | 输入 | 下拉 | 高电平 |
| PA1 (KEY2) | 输入 | 上拉 | 低电平 |
| PA4 (KEY3) | 输入 | 上拉 | 低电平 |
| PC5 (LED1) | 推挽输出 | 无 | 低电平点亮 |
| PB1 (LED2) | 推挽输出 | 无 | 低电平点亮 |
| PB2 (LED3) | 推挽输出 | 无 | 低电平点亮 |

---

## 实验验证清单

- [x] KEY1 按下 → LED1 切换
- [x] KEY2 按下 → LED2 切换
- [x] KEY3 按下 → LED3 切换
- [x] 长按防抖动生效
- [x] LED 自动流水灯正常运行
- [x] LED 自动闪烁正常运行
- [x] 编译无错误无警告
- [x] 代码大小 7056 bytes (Flash)

---

## 快速问题排查

### Q1: 按键按下无反应
**可能原因**：
1. ❌ 按键扫描函数未被调用 → 检查主循环是否包含 `Key_Scan_And_LED_Control()`
2. ❌ GPIO 时钟未开启 → 检查 `key_init()` 中是否调用 `KEY*_GPIO_CLK_ENABLE()`
3. ❌ 按键引脚配置错误 → 检查 [Core/Inc/key.h](../Core/Inc/key.h) 的引脚定义

### Q2: LED 不亮或始终亮
**可能原因**：
1. ❌ LED 初始化未调用 → 检查 main 函数是否调用 `LED_Init()`
2. ❌ LED 点亮逻辑反向 → 检查 `LED_On()` 是否写入 `GPIO_PIN_RESET`
3. ❌ GPIO 时钟未开启 → 检查 `LED_Init()` 是否开启对应时钟

### Q3: 自动流水灯不闪烁
**可能原因**：
1. ❌ 时间戳变量未初始化 → 检查 `board_led_tick` 初始值
2. ❌ 定时逻辑错误 → 验证 `HAL_GetTick() - board_led_tick >= 300U` 条件
3. ❌ 编译缓存 → 执行 `rm -r build/Debug` 后重新编译

---

## 文档导航

| 文档 | 主要内容 | 用途 |
|------|---------|------|
| [KEY_Circuit_Schematic.md](KEY_Circuit_Schematic.md) | 原理图、电路设计、中断配置 | 理解硬件 |
| [KEY_Control_Experiment.md](KEY_Control_Experiment.md) | 完整实验说明、代码分析、功能特性 | 学习实现 |
| 本文档 | 代码位置、快速查询、问题排查 | 快速参考 |

---

**最后更新**：2026 年 5 月 28 日  
**编译状态**：✅ 通过  
**测试状态**：✅ 通过  
**维护状态**：✅ 活跃
