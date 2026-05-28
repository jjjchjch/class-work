# STM32F407 按键电路原理图与控制

## 一、开发板按键电路连接原理图

### 1.1 硬件连接拓扑

```
┌─────────────────────────────────────────────────────────────┐
│                    STM32F407 MCU                             │
│                                                              │
│  PA0 ──────────────┐        ┌────────────── PC5 (LED1)      │
│                    │        │                               │
│                 KEY1      LED1                              │
│              (普通按键)   (低电平点亮)                       │
│                    │        │                               │
│  PA1 ──────────────┐        ├────────────── PB1 (LED2)      │
│                    │        │                               │
│                 KEY2      LED2                              │
│          (普通按键，下拉)  (低电平点亮)                      │
│                    │        │                               │
│  PA4 ──────────────┐        ├────────────── PB2 (LED3)      │
│                    │        │                               │
│                 KEY3      LED3                              │
│          (普通按键，下拉)  (低电平点亮)                      │
│                    │        │                               │
└────────────────────┼────────┴────────────────────────────────┘
                     │
            GND/+3.3V 接入
```

### 1.2 开发板按键详细设计

#### **KEY1: PA0（高电平点亮）**
```
           +3.3V
             │
             ├─[R上拉]── 100kΩ
             │
      ┌──────┴──────┐
      │              │
      R(限流)    按键开关
      100Ω           │
      │              │
      ├──────────────┤
             │
            PA0 (GPIO_MODE_INPUT, GPIO_PULLDOWN)
             │
            GND

工作原理：按键未按下 → PA0 = 低电平(0V)，GPIO读为 GPIO_PIN_RESET
        按键被按下 → PA0 = 高电平(3.3V)，GPIO读为 GPIO_PIN_SET
        
扫描检测：if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)  // 高电平按下
```

#### **KEY2: PA1（低电平按下）**
```
           +3.3V
             │
             ├─[R上拉]── 100kΩ
             │
      ┌──────┴──────┐
      │              │
      R(限流)    按键开关
      100Ω           │
      │              │
      ├──────┬───────┘
             │
            PA1 (GPIO_MODE_INPUT, GPIO_PULLUP)
             │
            GND

工作原理：按键未按下 → PA1 = 高电平(3.3V)，GPIO读为 GPIO_PIN_SET
        按键被按下 → PA1 = 低电平(0V)，GPIO读为 GPIO_PIN_RESET
        
扫描检测：if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)  // 低电平按下
```

#### **KEY3: PA4（低电平按下）**
```
           +3.3V
             │
             ├─[R上拉]── 100kΩ
             │
      ┌──────┴──────┐
      │              │
      R(限流)    按键开关
      100Ω           │
      │              │
      ├──────┬───────┘
             │
            PA4 (GPIO_MODE_INPUT, GPIO_PULLUP)
             │
            GND

工作原理：按键未按下 → PA4 = 高电平(3.3V)，GPIO读为 GPIO_PIN_SET
        按键被按下 → PA4 = 低电平(0V)，GPIO读为 GPIO_PIN_RESET
        
扫描检测：if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)  // 低电平按下
```

### 1.3 按键初始化配置

| 按键 | GPIO引脚 | 工作模式 | 上/下拉 | 按下触发 | 功能 |
|------|---------|--------|--------|--------|------|
| KEY1 | PA0 | 输入 | 下拉 | 高电平 | 控制 LED1（开发板红灯） |
| KEY2 | PA1 | 输入 | 上拉 | 低电平 | 控制 LED2（开发板绿灯） |
| KEY3 | PA4 | 输入 | 上拉 | 低电平 | 控制 LED3（开发板蓝灯） |

---

## 二、扩展板按键电路连接原理图

### 2.1 编码器 SW 按键（扩展板）

```
┌─────────────────────────────────────────────────┐
│        EC11 旋转编码器                           │
│                                                  │
│  ┌─────────────────────────────────┐            │
│  │  CLK(B脚) ── PA7 ─┐             │            │
│  │  DT(A脚)  ── PA6 ─┤ STM32F407   │            │
│  │  SW(中脚) ── PC4 ─┤ MCU         │            │
│  │  GND      ── GND  │             │            │
│  │  +3.3V    ── VCC  │             │            │
│  │                   │             │            │
│  └─────────────────────────────────┘            │
│                                                  │
│  SW 按键工作逻辑（低电平点亮）:                  │
│  按键未按下 → PC4 = 高电平(3.3V)                │
│  按键被按下 → PC4 = 低电平(0V)                  │
│                                                  │
└─────────────────────────────────────────────────┘
```

### 2.2 扩展板按键详细设计

```
        编码器 SW 脚
             │
             └─[内部上拉] 100kΩ
             │
      ┌──────┴──────┐
      │              │
      R(限流)    按键开关
      100Ω           │
      │              │
      ├──────┬───────┘
             │
            PC4 (GPIO_MODE_IT_FALLING, GPIO_PULLUP)
             │
            GND
            
外部中断配置：EXTI4_IRQn，优先级 2，下降沿触发
            
控制输出：扩展板 LED（如果存在），通常在 GND/+3.3V 之间
         或通过译码控制其他外设
```

---

## 三、按键扫描原理与去抖动

### 3.1 软件去抖动方案

```c
static uint8_t key_up = 1;  // 按键释放标志位

uint8_t key_scan(uint8_t mode)
{
    // 模式 1：支持连续按压检测，每次读取时重置标志位
    if (mode == 1)
        key_up = 1;
    
    // 检测按键是否被按下
    if (key_up && (某个按键被按下))
    {
        delay_ms(10);           // 延时 10ms，进行抖动过滤
        key_up = 0;             // 设置标志位，防止重复识别
        
        if (某个按键仍然被按下)
        {
            return 该按键的编码值;
        }
    }
    
    // 检测按键是否全部释放（用于重置状态）
    if (所有按键都释放了)
    {
        key_up = 1;
    }
    
    return 0;  // 无按键被按下
}
```

### 3.2 去抖动工作流程

```
按键按下时刻              ← 产生硬件抖动（典型 10-30ms）
    │
    ├─ 检测到按键电平变化
    │
    ├─ 延时 10ms 等待抖动稳定
    │
    ├─ 再次检测按键电平
    │
    ├─ 确认电平稳定，执行按键事件处理
    │
    └─ 设置标志位 key_up=0，防止重复检测

按键释放时刻
    │
    ├─ 检测到按键电平释放
    │
    └─ 设置标志位 key_up=1，允许下次检测
```

---

## 四、按键与LED控制对应关系

### 4.1 开发板按键控制逻辑

| 按键 | 按下状态 | LED 控制 | 实现方式 |
|------|--------|--------|--------|
| KEY1 | 高电平 | 切换 LED1（PC5）亮度 | `LED_Toggle(LED1_GPIO_PORT, LED1_GPIO_PIN)` |
| KEY2 | 低电平 | 切换 LED2（PB1）亮度 | `LED_Toggle(LED2_GPIO_PORT, LED2_GPIO_PIN)` |
| KEY3 | 低电平 | 切换 LED3（PB2）亮度 | `LED_Toggle(LED3_GPIO_PORT, LED3_GPIO_PIN)` |

### 4.2 扩展板按键控制逻辑

| 按键 | 按下状态 | 功能 | 实现方式 |
|------|--------|------|--------|
| SW（PC4） | 低电平 | 切换扩展板 LED | 中断处理 + `LED_Toggle()` |

---

## 五、中断处理配置

### 5.1 开发板按键中断配置（可选）

```c
void key_exti_init(void)
{
    GPIO_InitTypeDef gpio_init;
    
    // KEY1: PA0，上升沿触发 (EXTI0_IRQn)
    gpio_init.Pin   = GPIO_PIN_0;
    gpio_init.Mode  = GPIO_MODE_IT_RISING;      // 高电平按下
    gpio_init.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    // KEY2: PA1，下降沿触发 (EXTI1_IRQn)
    gpio_init.Pin   = GPIO_PIN_1;
    gpio_init.Mode  = GPIO_MODE_IT_FALLING;     // 低电平按下
    gpio_init.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    
    // KEY3: PA4，下降沿触发 (EXTI4_IRQn)
    gpio_init.Pin   = GPIO_PIN_4;
    gpio_init.Mode  = GPIO_MODE_IT_FALLING;     // 低电平按下
    gpio_init.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}
```

### 5.2 中断处理函数

```c
// KEY1 中断处理 (EXTI0_IRQHandler)
void EXTI0_IRQHandler(void)
{
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
        LED_Toggle(LED1_GPIO_PORT, LED1_GPIO_PIN);  // 切换 LED1
    }
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// KEY2 中断处理 (EXTI1_IRQHandler)
void EXTI1_IRQHandler(void)
{
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        LED_Toggle(LED2_GPIO_PORT, LED2_GPIO_PIN);  // 切换 LED2
    }
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

// KEY3 中断处理 (EXTI4_IRQHandler 或 EXTI9_5_IRQHandler)
void EXTI4_IRQHandler(void)
{
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
    {
        LED_Toggle(LED3_GPIO_PORT, LED3_GPIO_PIN);  // 切换 LED3
    }
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}
```

---

## 总结

- **开发板按键**：3个按键（KEY1~KEY3），分别控制 3 个 LED，支持轮询和中断两种扫描方式
- **扩展板按键**：编码器 SW 脚（PC4），可作为额外的控制按键
- **去抖动策略**：软件延时 10ms，确保电平稳定后再判断
- **工作模式**：KEY1 = 高电平按下，KEY2/KEY3/SW = 低电平按下
- **HAL 初始化**：`key_init()` 用于轮询模式，`key_exti_init()` 用于中断模式
