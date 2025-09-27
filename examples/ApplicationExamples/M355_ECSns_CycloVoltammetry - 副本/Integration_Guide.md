# M355_ECSns_CycloVoltammetry 串口控制集成指南

## 概述

本指南说明如何在原始的M355_ECSns_CycloVoltammetry项目中集成串口实时参数控制功能。

## 文件结构

项目现在包含以下新增文件：

```
M355_ECSns_CycloVoltammetry/
├── CV_Controller.h         # 循环伏安法控制器头文件
├── CV_Controller.c         # 循环伏安法控制器实现
├── UART_Parser.h          # 串口命令解析器头文件  
├── UART_Parser.c          # 串口命令解析器实现
├── CV_Example.c           # 完整示例代码（可选参考）
├── README_CV_Control.md   # 详细使用文档
├── Integration_Guide.md   # 本集成指南
├── main.c                 # 已修改的主文件
├── Ramp.h                 # 原始文件（未修改）
├── Ramp.c                 # 原始文件（未修改）
└── AD5940Main.c           # 原始文件（未修改）
```

## 主要修改

### 1. main.c 文件修改

**新增包含文件**:
```c
#include "CV_Controller.h"
#include "UART_Parser.h"
```

**新增全局变量**:
```c
static char g_ResponseBuffer[512];
static char g_RxChar;
static BoolFlag g_CharReceived = bFALSE;
```

**主函数修改**:
- 添加了串口控制系统初始化
- 设置了默认测量参数
- 修改为循环处理模式，同时处理串口命令和原始测量逻辑

**UART配置修改**:
- 波特率从230400改为115200（更好的兼容性）
- 添加了UART接收中断配置
- 新增UART中断处理函数

### 2. 新增功能模块

**CV_Controller模块**:
- 提供循环伏安法参数的动态配置接口
- 支持电压范围、电流档、扫速等参数的实时调整
- 包含参数验证和状态管理功能

**UART_Parser模块**:
- 实现完整的串口命令解析协议
- 支持XOR校验和验证
- 提供17个标准命令的处理

## 使用方法

### 1. 编译配置

确保在项目设置中包含新增的源文件：
- CV_Controller.c
- UART_Parser.c

### 2. 硬件连接

```
AD5940评估板 <-> PC/MCU
    P0.10 (TX) <-> RX
    P0.11 (RX) <-> TX  
    GND        <-> GND
```

### 3. 串口设置

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **流控**: 无

### 4. 基本命令

```bash
# 查看帮助
$HELP*15

# 设置电压范围 (-1V 到 +1V)
$SVR,-1.0,1.0*5A

# 设置电流档 (RTIA索引10)
$SCA,10*2F

# 设置扫速 (0.1 V/s)
$SSR,0.100*75

# 开始测量
$START*18

# 查询状态
$QST*0F

# 停止测量
$STOP*1F
```

## 与原项目的兼容性

### 保持兼容的部分

1. **原始测量逻辑**: AD5940Main.c中的测量逻辑完全保持不变
2. **Ramp模块**: Ramp.h和Ramp.c文件未做任何修改
3. **硬件配置**: AD5940的硬件配置保持原有设置
4. **数据格式**: 测量数据的格式和处理方式不变

### 新增的功能

1. **实时参数控制**: 可在测量过程中动态调整参数
2. **串口通信**: 通过标准化命令协议进行控制
3. **状态监控**: 实时查询系统状态和参数
4. **错误处理**: 完善的错误检测和报告机制

## 参数映射关系

串口控制参数与原项目参数的对应关系：

| 串口参数 | 原项目参数 | 说明 |
|----------|------------|------|
| startVolt | AppRAMPCfg.RampStartVolt | 起始电压 |
| peakVolt | AppRAMPCfg.RampPeakVolt | 峰值电压 |
| rtiaIndex | AppRAMPCfg.LPTIARtiaSel | RTIA电阻选择 |
| scanRate | 计算得出 | 基于步数和持续时间 |
| stepNum | AppRAMPCfg.StepNumber | 扫描步数 |
| duration | AppRAMPCfg.RampDuration | 扫描持续时间 |

## 调试和故障排除

### 1. 串口通信问题

**症状**: 发送命令无响应
**解决方法**:
- 检查串口连接和配置
- 验证波特率设置（115200）
- 确认命令格式和校验和

### 2. 参数设置失败

**症状**: 收到NAK响应
**解决方法**:
- 检查参数范围是否在有效区间内
- 确认系统当前状态允许参数修改
- 查看错误代码获取具体原因

### 3. 测量异常

**症状**: 测量数据异常或无数据
**解决方法**:
- 检查电极连接
- 验证电解液和样品
- 调整测量参数（电流档、扫速等）

## 扩展开发

### 1. 添加自定义命令

在UART_Parser.c中的g_CmdTable数组中添加新命令：

```c
{"MYCMD", UART_CMD_MYCMD, 1, 1, UART_HandleMyCommand, "My Command: $MYCMD,param*CS"}
```

### 2. 修改参数范围

在CV_Controller.h中修改相应的宏定义：

```c
#define CV_VOLTAGE_MIN    -3.0f  // 扩展电压范围
#define CV_VOLTAGE_MAX    +3.0f
```

### 3. 添加数据处理

可以在ProcessUartCommands()函数中添加数据处理逻辑：

```c
void ProcessUartCommands(void)
{
    // 原有代码...
    
    // 添加数据处理
    if (CV_GetMeasurementState() == CV_STATE_MEASURING) {
        // 处理测量数据
        ProcessMeasurementData();
    }
}
```

## 性能考虑

### 1. 中断处理

UART中断处理函数应保持简洁，避免长时间阻塞：

```c
void UART_Int_Handler(void)
{
    // 快速读取数据，设置标志
    g_RxChar = UrtRx(pADI_UART0);
    g_CharReceived = bTRUE;
    // 实际处理在主循环中进行
}
```

### 2. 缓冲区管理

合理设置缓冲区大小，避免溢出：

```c
#define UART_CMD_MAX_LENGTH    128  // 命令缓冲区
static char g_ResponseBuffer[512];  // 响应缓冲区
```

### 3. 实时性

对于实时性要求高的应用，可以考虑：
- 提高UART波特率
- 优化命令处理逻辑
- 使用DMA传输

## 版本兼容性

本集成方案兼容以下版本：
- ADuCM355 SDK v1.0及以上
- AD5940 Library v1.0及以上
- Keil MDK-ARM v5.0及以上
- IAR EWARM v8.0及以上

## 技术支持

如遇到问题，请参考：
1. README_CV_Control.md - 详细使用文档
2. CV_Example.c - 完整示例代码
3. ADI技术支持论坛
4. 项目源代码注释

## 更新日志

- v1.0.0: 初始集成版本
- v1.0.1: 修复UART波特率配置
- v1.0.2: 优化参数验证逻辑