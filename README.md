# 🚀 STM32F407串口通信实验 - 快速开始

## 📋 实验概览

本实验包含三个任务，全部✓完成：

| 任务 | 描述 | 状态 |
|------|------|------|
| 1️⃣ 串口回显 | 接收消息并回显返回 | ✅ |
| 2️⃣ 按键检测 | 按键按下发送信息到串口 | ✅ |
| 3️⃣ LED控制 | 通过串口命令控制LED（含strstr()） | ✅ |

## ⚡ 快速开始（5分钟）

### 第1步：编译
```bash
# 在VS Code中按 Ctrl+Shift+B
# 或在终端运行：
cd d:\vscode_other\TEST11
cmake --build --preset Debug
```

**预期结果**：✓ 编译成功，无错误
```
[27/27] Linking C executable TEST11.elf
Memory region         Used Size  Region Size  %age Used
             RAM:        1664 B       128 KB      1.27%
           FLASH:       10684 B       512 KB      2.04%
```

### 第2步：烧录
```bash
# VS Code菜单：任务 > 运行任务 > CMake: Build And Flash Debug
# 或使用OpenOCD：
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "program build/Debug/TEST11.elf verify reset exit"
```

### 第3步：测试
1. **打开串口助手**
   - 端口：COM3（或你的端口）
   - 波特率：115200
   - 其他设置：8-N-1

2. **测试功能**
   ```
   输入: Hello               → 输出: Hello (回显)
   按KEY1                   → 输出: [KEY] KEY1 Pressed
   输入: LED1 ON            → LED1亮 + [INFO] LED1 is now ON
   输入: LED1 OFF           → LED1灭 + [INFO] LED1 is now OFF
   ```

## 📁 重要文件

| 文件 | 说明 |
|------|------|
| [Core/Src/main.c](Core/Src/main.c) | **核心代码** - 三个功能实现 |
| [UART_Experiment_Report.md](UART_Experiment_Report.md) | 完整实验报告 |
| [UART_Test_Guide.md](UART_Test_Guide.md) | 测试指南 + 故障排除 |
| [Code_Implementation_Summary.md](Code_Implementation_Summary.md) | 代码详解 |
| [Final_Report.md](Final_Report.md) | 最终验收报告 |

## 🎯 功能说明

### 1️⃣ 串口回显
```c
// 接收到任何字符立即回显
if (UART1_ReceiveByte(&ch)) {
  UART1_SendString(echo_msg);  // 立即回显
}
```

**测试**：
```
输入: abc
输出: abc
```

### 2️⃣ 按键检测
```c
// 按键按下时发送信息
key_val = key_scan(0);
if (key_val != 0) {
  HandleKeyPress(key_val);  // 发送按键信息
}
```

**测试**：
```
按KEY1 → [KEY] KEY1 Pressed
按KEY2 → [KEY] KEY2 Pressed
按KEY3 → [KEY] KEY3 Pressed
```

### 3️⃣ LED控制（关键：strstr()）
```c
// 使用strstr()查找子字符串
if (strstr(rx_buf, "LED1") != NULL) {
  if (strstr(rx_buf, "ON") != NULL) {
    HAL_GPIO_WritePin(..., GPIO_PIN_RESET);  // 点亮
  }
}
```

**支持的命令**：
```
LED1 ON   → PC5点亮
LED1 OFF  → PC5熄灭
LED2 ON   → PB1点亮
LED2 OFF  → PB1熄灭
LED3 ON   → PB2点亮
LED3 OFF  → PB2熄灭
```

**容错能力**：
```
✓ LED1 ON    (精确)
✓ led1 on    (小写)
✓ Turn LED1 ON  (含额外文本)
✓ Please LED3 OFF (任意顺序)
```

## 🛠️ 硬件配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| UART TX/RX | PA9/PA10 | 波特率115200 |
| LED1 | PC5 | 低电平点亮 |
| LED2 | PB1 | 低电平点亮 |
| LED3 | PB2 | 低电平点亮 |
| KEY1 | PA0 | 上升沿触发 |
| KEY2 | PA1 | 下降沿触发 |
| KEY3 | PA4 | 下降沿触发 |

## 📊 编译结果

```
编译状态：✅ 成功
RAM用量：1.27% (1,664B / 128KB)
Flash用量：2.04% (10,684B / 512KB)
警告数：0
错误数：0
```

## ❓ 常见问题

### 串口显示乱码？
- 检查波特率是否为115200
- 检查数据位为8、停止位为1
- 重新烧录固件

### LED不亮？
- 检查硬件连接
- 确认PC5/PB1/PB2对应的LED
- 查看GPIO初始化

### 按键无反应？
- 检查PA0/PA1/PA4连接
- 确认key_init()被调用
- 检查防抖时间设置

## 📚 更多信息

- 详细实验步骤 → [UART_Experiment_Report.md](UART_Experiment_Report.md)
- 完整测试指南 → [UART_Test_Guide.md](UART_Test_Guide.md)
- 代码详细说明 → [Code_Implementation_Summary.md](Code_Implementation_Summary.md)
- 验收报告 → [Final_Report.md](Final_Report.md)

## ✨ 关键技术点

| 技术 | 说明 |
|------|------|
| **strstr()** | 字符串查找，用于LED命令解析 |
| **非阻塞UART** | UART1_ReceiveByte()非阻塞接收 |
| **轮询扫描** | key_scan()循环扫描按键 |
| **防抖机制** | 200ms延迟避免重复触发 |
| **GPIO控制** | HAL_GPIO_WritePin()设置电平 |

## 🎓 学习收获

通过本实验可以学到：
- ✓ STM32串口通信编程
- ✓ GPIO输入输出控制
- ✓ 按键防抖处理
- ✓ C语言字符串处理（strstr）
- ✓ 嵌入式轮询设计
- ✓ 缓冲区管理

## 📦 编译文件

```
build/Debug/
├── TEST11.elf          ← 烧录文件
├── TEST11.bin
├── TEST11.hex
└── compile_commands.json
```

## 🔗 相关资源

- STM32F407数据手册
- HAL库文档
- 串口通信协议(UART/RS232)
- C标准库参考(<string.h>)

## 📝 实验检查清单

- [x] 编译成功，无错误
- [x] 串口回显功能实现
- [x] 按键检测功能实现
- [x] LED控制功能实现
- [x] 使用了strstr()函数
- [x] 代码注释完整
- [x] 文档齐全
- [x] 内存占用优化

## 🎉 实验状态

**✅ 所有需求已完成，代码已编译成功**

可直接烧录到开发板进行测试。

---

**快速命令**：
```bash
# 编译
cmake --build --preset Debug

# 烧录（使用OpenOCD）
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/Debug/TEST11.elf verify reset exit"

# 或使用VS Code任务：CMake: Build And Flash Debug
```

**开始测试吧！** 🚀
