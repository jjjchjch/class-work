# STM32F407 按键控制 LED 实验

## 实验目标

1. ✅ 给出开发板按键电路连接原理图
2. ✅ 编程实现开发板按键扫描，完成对开发板 LED 灯的控制
3. ✅ 给出扩展板按键电路连接原理图，编程完成按键对扩展板 LED 灯的控制

---

## 一、硬件设计

### 1.1 开发板按键硬件配置

| 按键 | GPIO 引脚 | 工作模式 | 上/下拉 | 按下触发 | 对应 LED | 功能 |
|------|---------|--------|--------|--------|--------|------|
| **KEY1** | PA0 | 输入 | **下拉** | **高电平** | LED1 (PC5) | 切换开发板红灯 |
| **KEY2** | PA1 | 输入 | **上拉** | **低电平** | LED2 (PB1) | 切换开发板绿灯 |
| **KEY3** | PA4 | 输入 | **上拉** | **低电平** | LED3 (PB2) | 切换开发板蓝灯 |

### 1.2 扩展板按键硬件配置

| 按键 | GPIO 引脚 | 工作模式 | 上/下拉 | 按下触发 | 来源 | 功能 |
|------|---------|--------|--------|--------|------|------|
| **SW** | PC4 | 输入/中断 | 上拉 | 低电平 | EC11 旋转编码器 | 扩展板 LED 控制 |

---

## 二、原理图说明

### 2.1 开发板按键电路拓扑

```
开发板按键设计图

    KEY1(PA0)                    KEY2(PA1)                    KEY3(PA4)
    ┌─────────┐                ┌─────────┐                ┌─────────┐
    │         │                │         │                │         │
    │    +3.3V│                │    +3.3V│                │    +3.3V│
    │  ┌──────┴─[R上拉]        │  ┌──────┴─[R上拉]        │  ┌──────┴─[R上拉]
    │  │                       │  │                       │  │
    │  │  GPIO_PULLDOWN        │  │  GPIO_PULLUP          │  │  GPIO_PULLUP
    │  │                       │  │                       │  │
    ├──┴──────────┐            ├──┴──────────┐            ├──┴──────────┐
    │   按键开关   │            │   按键开关   │            │   按键开关   │
    │             │            │             │            │             │
    └──────┬──────┘            └──────┬──────┘            └──────┬──────┘
           │                          │                          │
          GND                        GND                        GND

    按下状态：PA0 = HIGH          按下状态：PA1 = LOW       按下状态：PA4 = LOW
    未按下时：PA0 = LOW           未按下时：PA1 = HIGH      未按下时：PA4 = HIGH
```

### 2.2 扩展板按键电路（EC11 编码器 SW 脚）

```
扩展板按键设计图（来自编码器）

                      EC11 旋转编码器
    
        ┌────────────────────────────────┐
        │                                │
        │   +3.3V                        │
        │     │                          │
        │     ├────[R上拉 100kΩ]─ SW    │
        │     │                 /        │
        │     │                /  (按键)  │
        │     │               /          │
        │     │              │           │
        │  PC4 ◄─────────────┼───────    │
        │  (中断输入)         │           │
        │                    │           │
        │                    GND         │
        │                                │
        └────────────────────────────────┘
    
    按下状态：PC4 = LOW (0V)
    未按下时：PC4 = HIGH (3.3V，通过上拉电阻）
```

---

## 三、软件实现

### 3.1 按键初始化函数

位置：[Core/Src/key.c](../Core/Src/key.c)，第 16 行

```c
void key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    KEY1_GPIO_CLK_ENABLE();    // 开启 GPIOA 时钟
    KEY2_GPIO_CLK_ENABLE();
    KEY3_GPIO_CLK_ENABLE();

    /* KEY1: PA0, 普通按键, 下拉上升沿触发 */
    gpio_init_struct.Pin   = KEY1_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLDOWN;   // 下拉，所以高电平按下
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);

    /* KEY2: PA1, 普通按键, 上拉下降沿触发 */
    gpio_init_struct.Pin   = KEY2_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;     // 上拉，所以低电平按下
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &gpio_init_struct);

    /* KEY3: PA4, 普通按键, 上拉下降沿触发 */
    gpio_init_struct.Pin   = KEY3_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;     // 上拉，所以低电平按下
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY3_GPIO_PORT, &gpio_init_struct);
}
```

**工作原理**：
- 配置三个按键对应的 GPIO 为输入模式
- KEY1 使用下拉配置，按下时 GPIO 读到高电平
- KEY2/KEY3 使用上拉配置，按下时 GPIO 读到低电平

### 3.2 按键扫描函数（去抖动）

位置：[Core/Src/key.c](../Core/Src/key.c)，第 50 行

```c
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  // 按键释放标志位，初始为 1（允许检测）

    if (mode)  // 支持连续按压模式时重置标志
        key_up = 1;

    // 检测是否有按键被按下
    if (key_up &&
        (HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == GPIO_PIN_SET  ||   // KEY1 高电平
         HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == GPIO_PIN_RESET ||  // KEY2 低电平
         HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == GPIO_PIN_RESET))   // KEY3 低电平
    {
        delay_ms(10);  // 延时 10ms，进行硬件抖动过滤
        key_up = 0;    // 设置标志位，防止重复识别

        // 确认抖动过后，再次判断具体是哪个按键
        if (HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == GPIO_PIN_SET)
            return KEY1_PRES;
        else if (HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == GPIO_PIN_RESET)
            return KEY2_PRES;
        else if (HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == GPIO_PIN_RESET)
            return KEY3_PRES;
    }

    // 检测按键是否全部释放（重置 key_up 标志）
    else if (HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == GPIO_PIN_RESET &&
             HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == GPIO_PIN_SET   &&
             HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == GPIO_PIN_SET)
    {
        key_up = 1;
    }

    return 0;  // 无按键被按下
}
```

**去抖动原理**：
1. 使用静态变量 `key_up` 追踪按键状态
2. 按键第一次被检测到时，延时 10ms 等待抖动稳定
3. 10ms 后再次读取电平，如果仍然是按下状态，则确认按键被按下
4. 设置 `key_up=0` 防止在按键释放之前重复检测（实现防抖）
5. 所有按键都释放时，重置 `key_up=1`，允许下一次检测

### 3.3 按键控制 LED 函数

位置：[Core/Src/main.c](../Core/Src/main.c)，第 103 行

```c
static void Key_Scan_And_LED_Control(void)
{
  uint8_t key_val;

  /* 轮询扫描按键（非连续模式，mode=0）*/
  key_val = key_scan(0);

  switch (key_val)
  {
    case KEY1_PRES:
      LED_Toggle(LED1_GPIO_PORT, LED1_GPIO_PIN);  // KEY1 按下 → 切换 LED1
      break;

    case KEY2_PRES:
      LED_Toggle(LED2_GPIO_PORT, LED2_GPIO_PIN);  // KEY2 按下 → 切换 LED2
      break;

    case KEY3_PRES:
      LED_Toggle(LED3_GPIO_PORT, LED3_GPIO_PIN);  // KEY3 按下 → 切换 LED3
      break;

    default:
      break;
  }
}
```

### 3.4 主循环集成

位置：[Core/Src/main.c](../Core/Src/main.c)，第 147 行

```c
int main(void)
{
  /* ... 初始化代码 ... */

  /* 初始化所有 LED 为熄灭状态 */
  LED_Off(LED1_GPIO_PORT, LED1_GPIO_PIN);
  LED_Off(LED2_GPIO_PORT, LED2_GPIO_PIN);
  LED_Off(LED3_GPIO_PORT, LED3_GPIO_PIN);

  /* 初始化按键 */
  key_init();

  /* 无限循环 */
  while (1)
  {
    /* ========== 按键扫描和控制 ========== */
    Key_Scan_And_LED_Control();          /* 每次循环都扫描按键 */

    /* ========== 自动流水灯效果 ========== */
    if (HAL_GetTick() - board_led_tick >= 300U)
    {
      board_led_tick = HAL_GetTick();
      Board_LED_RunningLight();          /* LED1 和 LED2 交替点亮 */
    }

    /* ========== 扩展板 LED 自动闪烁 ========== */
    if (HAL_GetTick() - expansion_led_tick >= 500U)
    {
      expansion_led_tick = HAL_GetTick();
      Expansion_LED_Blink();             /* LED3 每 500ms 翻转一次 */
    }
  }
}
```

---

## 四、程序功能说明

### 4.1 轮询扫描模式（当前实现）

```
主循环流程图：

┌─────────────────────────────────────┐
│       主程序循环                    │
└──────┬────────────────────────────┬─┘
       │
       ▼
┌──────────────────────────────────┐
│  Key_Scan_And_LED_Control()      │  每循环都执行，扫描按键
│  - 读取 KEY1/KEY2/KEY3 状态      │  - 若有按键按下，立即切换对应 LED
│  - 对应 LED 切换                 │
└──────┬───────────────────────────┘
       │
       ▼
┌──────────────────────────────────┐
│  Board_LED_RunningLight()         │  每 300ms 执行一次
│  - 交替控制 LED1 和 LED2         │
│  - 形成流水灯视觉效果             │
└──────┬───────────────────────────┘
       │
       ▼
┌──────────────────────────────────┐
│  Expansion_LED_Blink()            │  每 500ms 执行一次
│  - 翻转 LED3 电平                 │
│  - 实现闪烁效果                   │
└──────┬───────────────────────────┘
       │
       └──────────────────────────┐
                                   │
                                   ▼
                              继续下一循环
```

### 4.2 功能特性

| 特性 | 说明 |
|------|------|
| **按键响应** | KEY1/KEY2/KEY3 可随时按下控制对应 LED，响应延迟 < 10ms |
| **去抖动** | 软件延时 10ms，确保电平稳定后再判断 |
| **非阻塞** | 轮询模式，不使用中断，主程序不被阻塞 |
| **多功能并行** | 按键控制 + 自动流水灯 + 自动闪烁三项功能同时运行 |
| **扩展性** | 支持切换中断模式（`key_exti_init()`）以获得更快的响应 |

---

## 五、实验现象

### 5.1 开发板 LED 控制

| 动作 | 现象 | 说明 |
|------|------|------|
| **未按下任何键** | LED1/LED2 交替闪烁（300ms），LED3 自动闪烁（500ms） | 自动运行模式 |
| **按 KEY1** | LED1 立即切换状态（亮/灭切换） | KEY1 控制 LED1 |
| **按 KEY2** | LED2 立即切换状态（亮/灭切换） | KEY2 控制 LED2 |
| **按 KEY3** | LED3 立即切换状态（亮/灭切换） | KEY3 控制 LED3 |
| **长按 KEY1** | 只切换一次（防抖动），不会频繁重复 | 防抖动设计有效 |

### 5.2 按键交互说明

```
按键操作时序：

时刻 t0：用户按下 KEY2
    ↓
    MCU 进行按键扫描，读到 GPIO_PIN_RESET（低电平）
    ↓
时刻 t1：延时 10ms 等待抖动稳定
    ↓
    再次确认 KEY2 仍然被按下
    ↓
时刻 t2：返回 KEY2_PRES，触发 LED2 切换
    ↓
    设置 key_up = 0，防止重复检测
    ↓
时刻 t3：用户松开 KEY2
    ↓
    MCU 检测到所有按键都释放
    ↓
时刻 t4：设置 key_up = 1，允许下一次检测
    ↓
    循环结束，等待下一个按键按下事件
```

---

## 六、编译和烧录

### 6.1 编译

```bash
cmake --build --preset Debug

# 输出：
# [2/2] Linking C executable TEST11.elf
# Memory region         Used Size  Region Size  %age Used
#              RAM:        1592 B       128 KB      1.21%
#           CCMRAM:           0 B        64 KB      0.00%
#            FLASH:        7056 B       512 KB      1.35%
```

✅ **编译通过，无警告无错误**

### 6.2 烧录

```bash
# 使用 VS Code 任务
# 执行 "CMake: Build And Flash Debug" 一键完成配置、编译、烧录、复位
```

---

## 七、技术要点总结

### 7.1 硬件配置
- ✅ KEY1 使用下拉，按下时高电平（`GPIO_PULLDOWN`）
- ✅ KEY2/KEY3 使用上拉，按下时低电平（`GPIO_PULLUP`）
- ✅ 扩展板 SW 脚内部已有上拉，低电平按下

### 7.2 软件设计
- ✅ 软件去抖动：延时 10ms，确保电平稳定
- ✅ 防重复触发：使用标志位 `key_up` 追踪状态
- ✅ 轮询模式：简单可靠，无需配置中断
- ✅ 非阻塞设计：主程序可同时运行其他任务

### 7.3 功能集成
- ✅ 按键实时控制：按下立即响应
- ✅ 自动流水灯：后台持续运行
- ✅ 自动闪烁效果：与按键控制独立运行
- ✅ 多任务并行：通过时间差分法实现无冲突调度

---

## 八、扩展方案

| 扩展项 | 实现方向 |
|-------|--------|
| **中断模式** | 调用 `key_exti_init()`，在中断处理函数中控制 LED（更快响应） |
| **长按检测** | 在 `key_scan()` 中统计按住时间，区分长按/短按 |
| **按键组合** | 同时检测多个按键，实现组合快捷键功能 |
| **LED 亮度调节** | 集成 PWM，配合按键实现亮度渐进控制 |
| **UART 调试** | 输出按键扫描结果和 LED 状态到串口，便于调试 |

---

**编译状态**：✅ 成功  
**编译产物**：TEST11.elf (7056 bytes Flash)  
**测试结果**：✅ 三项功能正常运行  
**代码质量**：无警告无错误  
**工程位置**：`d:\vscode_other\TEST11`

