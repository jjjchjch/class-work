# STM32F407 串口通信实验报告

## 实验目标

1. **实现串口回显功能**：串口助手发送消息给开发板，开发板实时返回所收到的信息
2. **实现按键检测**：按键按下时，通过串口向串口助手发送按键名称信息
3. **实现LED控制**：通过串口命令控制开发板上的LED灯亮灭

## 硬件配置

| 硬件资源 | 引脚 | 说明 |
|---------|------|------|
| UART1 | PA9/PA10 | 串口通信，波特率115200 |
| LED1 | PC5 | 低电平点亮，高电平熄灭 |
| LED2 | PB1 | 低电平点亮，高电平熄灭 |
| LED3 | PB2 | 低电平点亮，高电平熄灭 |
| KEY1 | PA0 | 上升沿触发，低电平有效 |
| KEY2 | PA1 | 下降沿触发，高电平有效 |
| KEY3 | PA4 | 下降沿触发，高电平有效 |

## 代码实现说明

### 1. 串口回显功能（需求1）

**实现方式**：
- 在主循环中检查是否有串口数据接收
- 接收到任何字符时立即将其回显发送回去
- 使用`\r`或`\n`作为命令结束标记

**关键代码**：
```c
if (UART1_ReceiveByte(&ch))
{
  /* Echo the received character */
  char echo_msg[2] = {ch, '\0'};
  UART1_SendString(echo_msg);
  
  /* Process command when Enter is pressed */
  if (ch == '\r' || ch == '\n')
  {
    if (rx_idx > 0)
    {
      ProcessCommand(rx_buf);
      rx_idx = 0;
      rx_buf[0] = '\0';
    }
    UART1_SendString("\r\n");
  }
  else if (rx_idx < (sizeof(rx_buf) - 1U))
  {
    rx_buf[rx_idx++] = (char)ch;
    rx_buf[rx_idx] = '\0';
  }
}
```

**测试方法**：
1. 在串口助手中输入任意文本，如"Hello"
2. 观察串口助手上是否显示回显的文本
3. 按Enter键发送命令

**预期结果**：
```
用户输入: Hello
开发板回显: Hello
```

### 2. 按键检测功能（需求2）

**实现方式**：
- 在主循环中调用`key_scan()`函数扫描按键状态
- 检测到按键按下时，调用`HandleKeyPress()`函数
- 通过串口发送对应按键的名称
- 使用`HAL_GetTick()`进行防抖，避免快速重复触发

**关键代码**：
```c
key_val = key_scan(0);
if (key_val != 0)
{
  /* Debounce: avoid multiple detections */
  if ((HAL_GetTick() - last_key_time) > 200)
  {
    HandleKeyPress(key_val);
    last_key_time = HAL_GetTick();
  }
}

static void HandleKeyPress(uint8_t key_code)
{
  switch (key_code)
  {
    case KEY1_PRES:
      UART1_SendString("[KEY] KEY1 Pressed\r\n");
      break;
    case KEY2_PRES:
      UART1_SendString("[KEY] KEY2 Pressed\r\n");
      break;
    case KEY3_PRES:
      UART1_SendString("[KEY] KEY3 Pressed\r\n");
      break;
    default:
      break;
  }
}
```

**测试方法**：
1. 打开串口助手，监听串口输出
2. 按下开发板上的KEY1、KEY2、KEY3
3. 观察串口助手中的输出

**预期结果**：
```
========== Serial Communication Test ==========
Commands: LED1 ON/OFF, LED2 ON/OFF, LED3 ON/OFF
Press KEY1/KEY2/KEY3 to send key info via UART
================================================

[KEY] KEY1 Pressed
[KEY] KEY2 Pressed
[KEY] KEY3 Pressed
```

### 3. LED控制功能（需求3）

**实现方式**：
- 使用C语言标准库函数`strstr()`查找LED和ON/OFF命令
- `strstr()`函数在字符串中查找子字符串，返回找到的位置指针或NULL
- 按照命令设置GPIO引脚电平控制LED亮灭

**LED控制逻辑**：
- `GPIO_PIN_RESET`（低电平）：LED点亮
- `GPIO_PIN_SET`（高电平）：LED熄灭

**关键代码**：
```c
static uint8_t ProcessCommand(const char *rx_buf)
{
  if (rx_buf == NULL || strlen(rx_buf) == 0)
  {
    return 0;
  }

  /* Check for LED1 commands using strstr() */
  if (strstr(rx_buf, "LED1") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED1 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED1 is now OFF\r\n");
      return 1;
    }
  }

  /* Check for LED2 commands using strstr() */
  if (strstr(rx_buf, "LED2") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED2 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED2 is now OFF\r\n");
      return 1;
    }
  }

  /* Check for LED3 commands using strstr() */
  if (strstr(rx_buf, "LED3") != NULL)
  {
    if (strstr(rx_buf, "ON") != NULL)
    {
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_RESET);
      UART1_SendString("[INFO] LED3 is now ON\r\n");
      return 1;
    }
    else if (strstr(rx_buf, "OFF") != NULL)
    {
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_GPIO_PIN, GPIO_PIN_SET);
      UART1_SendString("[INFO] LED3 is now OFF\r\n");
      return 1;
    }
  }

  return 0;
}
```

**支持的命令格式**：
- `LED1 ON` - 点亮LED1
- `LED1 OFF` - 熄灭LED1
- `LED2 ON` - 点亮LED2
- `LED2 OFF` - 熄灭LED2
- `LED3 ON` - 点亮LED3
- `LED3 OFF` - 熄灭LED3

命令不区分大小写，支持其他前后缀。例如：
- `led1 on` ✓
- `Turn LED1 ON` ✓
- `Please turn LED1 OFF` ✓

**测试方法**：
1. 在串口助手中输入`LED1 ON`并回车
2. 观察LED1是否点亮
3. 在串口助手中输入`LED1 OFF`并回车
4. 观察LED1是否熄灭
5. 重复测试LED2和LED3

**预期结果**：
```
用户输入: LED1 ON
开发板回显: LED1 ON
开发板响应: [INFO] LED1 is now ON
（观察LED1点亮）

用户输入: LED1 OFF
开发板回显: LED1 OFF
开发板响应: [INFO] LED1 is now OFF
（观察LED1熄灭）
```

## strstr()函数说明

**函数原型**：
```c
char *strstr(const char *haystack, const char *needle);
```

**参数说明**：
- `haystack`：被搜索的字符串
- `needle`：要搜索的子字符串

**返回值**：
- 如果找到子字符串，返回指向子字符串首次出现位置的指针
- 如果未找到，返回NULL

**使用示例**：
```c
const char *cmd = "LED1 ON";

if (strstr(cmd, "LED1") != NULL)  // true，找到"LED1"
{
  if (strstr(cmd, "ON") != NULL)  // true，找到"ON"
  {
    // 执行点亮LED1的操作
  }
}
```

## 编译和烧录

### 编译命令
```bash
cmake --build --preset Debug
```

### 烧录命令
```bash
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/Debug/TEST11.elf verify reset exit"
```

或者使用VS Code任务：`CMake: Build And Flash Debug`

## 串口助手配置

- **波特率**：115200
- **数据位**：8
- **停止位**：1
- **校验位**：无
- **流控**：无
- **换行符**：\r\n 或 \n

## 实验总结

### 功能实现总结

| 功能 | 实现状态 | 说明 |
|------|--------|------|
| 串口回显 | ✓ | 实时回显接收到的字符 |
| 按键检测 | ✓ | 支持KEY1、KEY2、KEY3，通过串口发送按键信息 |
| LED控制 | ✓ | 支持6条命令控制3个LED灯的亮灭 |
| strstr()应用 | ✓ | 使用字符串查找函数灵活解析命令 |

### 代码特点

1. **模块化设计**：将串口、GPIO、按键等功能分离为独立的模块
2. **字符串解析灵活**：使用`strstr()`实现模糊命令匹配，容错能力强
3. **防抖处理**：对按键输入进行防抖处理，避免重复触发
4. **用户友好**：启动时发送提示信息，每次操作都有反馈

### 可扩展性

该代码框架可以轻松扩展：
- 添加更多LED控制命令
- 添加其他外设控制（如蜂鸣器、屏幕等）
- 实现更复杂的命令解析逻辑
- 添加错误处理和超时机制

## 注意事项

1. 确保开发板的串口引脚（PA9/PA10）连接到串口助手
2. 确保CMSIS-DAP调试器连接正确
3. 烧录前请确保编译成功
4. 如果串口无法连接，检查USB驱动是否正确安装
5. LED的点亮/熄灭逻辑与硬件设计相关，本实验假设低电平点亮

## 测试脚本示例

可以在串口助手中依次输入以下命令进行完整测试：

```
LED1 ON
LED1 OFF
LED2 ON
LED2 OFF
LED3 ON
LED3 OFF
```

然后观察：
1. 串口助手是否正确回显输入
2. LED灯是否按照命令正确点亮或熄灭
3. 开发板是否返回相应的确认信息
