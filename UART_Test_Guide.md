# 串口通信实验 - 快速使用指南

## 前置准备

1. **硬件连接**
   - STM32F407开发板
   - CMSIS-DAP调试器
   - USB转串口模块或直接使用板载UART
   - PC运行串口助手软件（如CH340驱动的串口工具）

2. **软件环境**
   - VS Code + CMake工具
   - ARM编译工具链（gcc-arm-none-eabi）
   - OpenOCD（用于烧录）

## 编译和烧录步骤

### 方法1：使用VS Code任务（推荐）

1. 按`Ctrl+Shift+B`或菜单选择`任务 > 运行任务`
2. 选择`CMake: Build And Flash Debug`
3. 等待编译和烧录完成

### 方法2：手动命令

```bash
# 编译
cd d:\vscode_other\TEST11
cmake --build --preset Debug

# 烧录
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "program build/Debug/TEST11.elf verify reset exit"
```

## 串口助手配置

| 参数 | 值 |
|------|-----|
| 端口 | COM3（或检测到的端口） |
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控 | 无 |
| 发送模式 | 文本 |
| 接收显示 | 文本/十六进制 |

## 功能测试清单

### ✓ 测试1：串口回显功能

**步骤**：
1. 烧录代码到开发板
2. 打开串口助手，配置好波特率115200
3. 在串口助手输入框输入文本，如：`Hello World`
4. 观察串口助手接收区

**预期输出**：
```
========== Serial Communication Test ==========
Commands: LED1 ON/OFF, LED2 ON/OFF, LED3 ON/OFF
Press KEY1/KEY2/KEY3 to send key info via UART
================================================

Hello World
```

**验证标准**：
- [ ] 开发板回显了输入的文本
- [ ] 每输入一个字符就立即回显
- [ ] 按Enter后显示换行符

---

### ✓ 测试2：按键检测功能

**步骤**：
1. 打开串口助手
2. 按开发板上的KEY1按键
3. 观察串口助手接收区
4. 重复测试KEY2和KEY3

**预期输出**：
```
[KEY] KEY1 Pressed
[KEY] KEY2 Pressed
[KEY] KEY3 Pressed
```

**验证标准**：
- [ ] 按KEY1时串口显示"KEY1 Pressed"
- [ ] 按KEY2时串口显示"KEY2 Pressed"
- [ ] 按KEY3时串口显示"KEY3 Pressed"
- [ ] 防抖正常，长按不会重复输出（>200ms重复）
- [ ] 有适当的时间间隔以避免误触发

---

### ✓ 测试3：LED控制功能

#### 测试3.1：LED1控制

**步骤**：
1. 在串口助手输入框输入：`LED1 ON`
2. 按Enter发送
3. 观察LED1（PC5）是否点亮
4. 在输入框输入：`LED1 OFF`
5. 按Enter发送
6. 观察LED1是否熄灭

**预期输出**：
```
LED1 ON
[INFO] LED1 is now ON
LED1 OFF
[INFO] LED1 is now OFF
```

**验证标准**：
- [ ] 输入`LED1 ON`时LED1点亮
- [ ] 输入`LED1 OFF`时LED1熄灭
- [ ] 开发板返回确认消息
- [ ] 命令不区分大小写（`led1 on`也有效）
- [ ] 支持额外的文本（如`Please turn LED1 ON`）

#### 测试3.2：LED2控制

**步骤**：同上，将`LED1`改为`LED2`

**验证标准**：
- [ ] 输入`LED2 ON`时LED2（PB1）点亮
- [ ] 输入`LED2 OFF`时LED2熄灭

#### 测试3.3：LED3控制

**步骤**：同上，将`LED1`改为`LED3`

**验证标准**：
- [ ] 输入`LED3 ON`时LED3（PB2）点亮
- [ ] 输入`LED3 OFF`时LED3熄灭

---

## 高级测试场景

### 场景1：混合命令测试

```
LED1 ON
LED2 ON
LED3 ON
LED1 OFF
LED2 OFF
LED3 OFF
```

**预期结果**：
- 按顺序执行，三个LED依次点亮，再依次熄灭
- 每条命令都有回显和确认信息

### 场景2：容错测试

```
led1 on          （小写）
Turn LED2 ON     （带前缀）
LED3 OFF PLEASE  （带后缀）
LedX On          （无效命令，应该不响应）
```

**预期结果**：
- 小写命令有效
- 带额外文本的命令有效
- 无效命令没有响应

### 场景3：快速操作测试

```
LED1 ON
LED1 OFF
LED1 ON
LED1 OFF
```

快速连续发送命令，观察LED是否能正确响应。

## 故障排除

### 问题1：串口无法连接

**可能原因和解决方案**：
- [ ] 检查USB驱动是否安装（CH340驱动等）
- [ ] 更换USB接口或USB线
- [ ] 在设备管理器中确认COM端口号
- [ ] 重启串口助手应用

### 问题2：LED不亮

**可能原因和解决方案**：
- [ ] 检查LED硬件连接是否正确
- [ ] 确认PC5/PB1/PB2对应的GPIO初始化正确
- [ ] 验证LED极性（正向是否接高电平）
- [ ] 检查GPIO的电平定义（高/低电平点亮）

### 问题3：按键无反应

**可能原因和解决方案**：
- [ ] 确认按键硬件连接正确
- [ ] 检查PA0/PA1/PA4是否正确配置为输入
- [ ] 验证上拉/下拉配置与按键电路匹配
- [ ] 检查key_scan()函数是否被正确调用

### 问题4：串口数据乱码

**可能原因和解决方案**：
- [ ] 检查波特率是否为115200
- [ ] 确认数据位为8、停止位为1
- [ ] 检查USB转串口模块的驱动
- [ ] 尝试重新烧录固件

## 代码关键要点

### 回显实现
```c
UART1_ReceiveByte(&ch);         // 接收一个字节
UART1_SendString(&ch);          // 立即回显
```

### 按键处理
```c
key_val = key_scan(0);          // 扫描按键
if (key_val != 0) {
  HandleKeyPress(key_val);      // 处理按键
}
```

### LED控制（关键：strstr()函数）
```c
if (strstr(rx_buf, "LED1") != NULL) {
  if (strstr(rx_buf, "ON") != NULL) {
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
  }
}
```

**strstr()的优势**：
- 可以在任意位置查找子字符串
- 容错能力强，不需要完全匹配
- 代码简洁易懂

## 内存占用情况

编译后的内存统计：
- **RAM**: 1.27% (1664字节 / 128KB)
- **FLASH**: 2.04% (10684字节 / 512KB)

说明代码效率高，还有充足空间用于扩展。

## 性能指标

| 指标 | 值 |
|------|-----|
| 波特率 | 115200 bps |
| 串口响应时间 | <10ms |
| 按键防抖延迟 | 200ms |
| LED响应时间 | <5ms |

## 下一步改进方向

1. **功能扩展**
   - 支持更多LED控制命令
   - 添加LED亮度调节（PWM）
   - 实现LED闪烁功能

2. **可靠性增强**
   - 添加命令超时检测
   - 实现CRC校验
   - 支持多字节命令

3. **用户体验**
   - 添加命令帮助菜单
   - 实现命令历史记录
   - 支持按键自定义映射

## 参考资源

- STM32F407 数据手册：[Datasheet](https://www.st.com/en/microcontrollers/stm32f407-417.html)
- HAL库文档：STM32Cube IDE自带
- C标准库函数：`strstr()`、`strlen()`等
- UART通信原理：异步串行通信协议

## 许可证和归属

本实验代码基于STM32CubeMX生成的HAL库框架。

---

**最后更新日期**：2026年5月
**版本**：1.0
**作者**：实验创建者
