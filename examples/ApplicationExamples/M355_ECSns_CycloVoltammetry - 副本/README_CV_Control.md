# AD5940 循环伏安法实时参数控制系统

## 概述

本系统基于ADI AD5940芯片，实现了循环伏安法(Cyclic Voltammetry)的实时参数控制功能。通过串口通信，用户可以动态调整测量参数，包括电压范围、电流档位、扫描速率等关键参数。

## 系统特性

### 可控参数
- **电压范围**: -2.4V 到 +2.4V，分辨率0.1V
- **电流档位**: 27个RTIA档位可选(200Ω到512kΩ)
- **扫描速率**: 0.02 mV/s 到 5 V/s
- **扫描步数**: 50 到 2000步
- **测量模式**: 快速扫描、高精度、低电流、自定义

### 通信协议
- **接口**: UART串口通信
- **波特率**: 115200 bps (可配置)
- **数据格式**: 8N1
- **命令格式**: `$CMD,param1,param2*XX\r\n`
- **校验**: XOR校验和

## 文件结构

```
├── CV_Controller.h         # 循环伏安法控制器头文件
├── CV_Controller.c         # 循环伏安法控制器实现
├── UART_Parser.h          # 串口命令解析器头文件  
├── UART_Parser.c          # 串口命令解析器实现
├── CV_Example.c           # 完整示例代码
├── 循环伏安法实时参数控制设计.md  # 详细设计文档
└── README_CV_Control.md   # 本文档
```

## 快速开始

### 1. 硬件连接

```
AD5940评估板 <-> MCU/PC
    UART_TX  <-> RX
    UART_RX  <-> TX  
    GND      <-> GND
    VDD      <-> 3.3V
```

### 2. 软件集成

```c
#include "CV_Controller.h"
#include "UART_Parser.h"

int main(void)
{
    // 初始化系统
    AD5940_MCUResourceInit(0);
    CV_Init();
    UART_ParserInit();
    
    // 设置默认参数
    CV_SetVoltageRange(-1.0f, 1.0f);
    CV_SetCurrentRange(10);
    CV_SetScanRate(100.0f);
    
    // 主循环
    while(1) {
        // 处理串口命令
        if(UART_ProcessReadyCommand(response)) {
            printf("%s", response);
        }
        
        // 处理测量数据
        if(CV_GetMeasurementState() == CV_STATE_MEASURING) {
            // 获取和处理数据
        }
    }
}
```

### 3. 基本使用

```bash
# 设置电压范围 (-1V 到 +1V)
$SVR,-1.0,1.0*5A

# 设置电流档 (RTIA索引10)
$SCA,10*2F

# 设置扫速 (100 mV/s)
$SSR,100*7B

# 开始测量
$START*18

# 查询状态
$QST*0F
```

## 命令参考

### 设置命令

| 命令 | 格式 | 说明 | 示例 |
|------|------|------|------|
| SVR | `$SVR,start,peak*XX` | 设置电压范围 | `$SVR,-1.0,1.0*5A` |
| SCA | `$SCA,rtia_index*XX` | 设置电流档 | `$SCA,10*2F` |
| SSR | `$SSR,rate*XX` | 设置扫速(mV/s) | `$SSR,100*7B` |
| SSP | `$SSP,steps,duration*XX` | 设置扫描参数 | `$SSP,200,1000*4C` |
| SMD | `$SMD,mode*XX` | 设置测量模式 | `$SMD,0*1E` |

### 控制命令

| 命令 | 格式 | 说明 | 示例 |
|------|------|------|------|
| START | `$START*XX` | 开始测量 | `$START*18` |
| STOP | `$STOP*XX` | 停止测量 | `$STOP*1F` |
| PAUSE | `$PAUSE*XX` | 暂停测量 | `$PAUSE*0B` |
| RESUME | `$RESUME*XX` | 恢复测量 | `$RESUME*2A` |
| RESET | `$RESET*XX` | 重置系统 | `$RESET*3D` |

### 查询命令

| 命令 | 格式 | 说明 | 示例 |
|------|------|------|------|
| QVR | `$QVR*XX` | 查询电压范围 | `$QVR*0F` |
| QCA | `$QCA*XX` | 查询电流档 | `$QCA*06` |
| QSR | `$QSR*XX` | 查询扫速 | `$QSR*09` |
| QSP | `$QSP*XX` | 查询扫描参数 | `$QSP*0C` |
| QST | `$QST*XX` | 查询状态 | `$QST*0F` |
| QALL | `$QALL*XX` | 查询所有参数 | `$QALL*12` |
| HELP | `$HELP*XX` | 显示帮助 | `$HELP*15` |

## 响应格式

### 成功响应 (ACK)
```
$ACK,CMD,data*XX\r\n
```

### 错误响应 (NAK)  
```
$NAK,CMD,EXX*XX\r\n
```

### 数据响应
```
$DATA,voltage,current,timestamp*XX\r\n
```

### 信息响应
```
$INFO,message*XX\r\n
```

## 参数范围

### 电压参数
- **最小值**: -2.4V
- **最大值**: +2.4V  
- **分辨率**: 0.1V
- **精度**: ±1mV

### 电流档位 (RTIA值)

| 索引 | 电阻值 | 最大电流 | 分辨率 |
|------|--------|----------|--------|
| 0 | 开路 | - | - |
| 1 | 200Ω | 12mA | 0.4μA |
| 2 | 1kΩ | 2.4mA | 80nA |
| 5 | 5kΩ | 480μA | 16nA |
| 10 | 20kΩ | 120μA | 4nA |
| 15 | 80kΩ | 30μA | 1nA |
| 20 | 160kΩ | 15μA | 0.5nA |
| 26 | 512kΩ | 4.7μA | 0.16nA |

### 扫描速率
- **最小值**: 0.02 mV/s (0.28mV步长)
- **最大值**: 5 V/s (10mV步长)  
- **推荐范围**: 1-1000 mV/s

### 扫描参数
- **步数范围**: 50-2000步
- **持续时间**: 1-10000ms
- **总测量时间**: 最大20秒

## 测量模式

### 快速扫描模式 (FAST_SCAN)
- 优化速度，适合快速筛选
- 扫速: 100-5000 mV/s
- 精度: 中等

### 高精度模式 (HIGH_PRECISION)  
- 优化精度，适合精确测量
- 扫速: 0.02-100 mV/s
- 精度: 最高

### 低电流模式 (LOW_CURRENT)
- 优化低电流测量
- 自动选择高阻RTIA
- 适合电化学阻抗谱

### 自定义模式 (CUSTOM)
- 用户完全控制所有参数
- 需要手动优化设置

## 错误代码

| 代码 | 说明 | 解决方法 |
|------|------|----------|
| E00 | 成功 | - |
| E01 | 格式错误 | 检查命令格式 |
| E02 | 校验和错误 | 重新计算校验和 |
| E03 | 未知命令 | 检查命令名称 |
| E04 | 参数个数错误 | 检查参数数量 |
| E05 | 参数值错误 | 检查参数范围 |
| E06 | 缓冲区满 | 减少命令频率 |

## 性能指标

### 响应时间
- **命令响应**: <10ms
- **参数设置**: <50ms  
- **测量启动**: <100ms

### 数据吞吐量
- **最大采样率**: 1kHz
- **数据传输率**: 115200 bps
- **缓冲区大小**: 1024字节

### 精度指标
- **电压精度**: ±0.1% ±1mV
- **电流精度**: ±0.5% ±1pA
- **时间精度**: ±1ms

## 应用示例

### 1. 基本循环伏安测量

```python
import serial
import time

# 打开串口
ser = serial.Serial('COM3', 115200)

# 设置参数
ser.write(b'$SVR,-1.0,1.0*5A\r\n')  # 电压范围
ser.write(b'$SCA,10*2F\r\n')        # 电流档
ser.write(b'$SSR,100*7B\r\n')       # 扫速100mV/s

# 开始测量
ser.write(b'$START*18\r\n')

# 接收数据
while True:
    data = ser.readline().decode()
    if data.startswith('$DATA'):
        print(data.strip())
    elif data.startswith('$INFO,Measurement completed'):
        break
```

### 2. 参数优化扫描

```python
# 扫描不同扫速的效果
scan_rates = [10, 50, 100, 500, 1000]  # mV/s

for rate in scan_rates:
    # 设置扫速
    cmd = f'$SSR,{rate}*{checksum:02X}\r\n'
    ser.write(cmd.encode())
    
    # 开始测量
    ser.write(b'$START*18\r\n')
    
    # 收集数据
    data_points = []
    while True:
        response = ser.readline().decode()
        if response.startswith('$DATA'):
            # 解析数据
            parts = response.split(',')
            voltage = float(parts[1])
            current = float(parts[2])
            data_points.append((voltage, current))
        elif 'completed' in response:
            break
    
    # 保存结果
    save_data(f'cv_data_{rate}mVs.csv', data_points)
```

### 3. 实时参数调整

```python
def adaptive_measurement():
    # 开始测量
    ser.write(b'$START*18\r\n')
    
    max_current = 0
    while True:
        response = ser.readline().decode()
        if response.startswith('$DATA'):
            current = float(response.split(',')[2])
            max_current = max(max_current, abs(current))
            
            # 如果电流过大，降低RTIA
            if max_current > 1e-3:  # 1mA
                ser.write(b'$PAUSE*0B\r\n')
                ser.write(b'$SCA,15*XX\r\n')  # 更高阻值
                ser.write(b'$RESUME*2A\r\n')
                
        elif 'completed' in response:
            break
```

## 故障排除

### 常见问题

1. **串口无响应**
   - 检查波特率设置
   - 确认串口连接
   - 验证硬件电源

2. **命令格式错误**
   - 检查命令语法
   - 验证校验和计算
   - 确认结束符\r\n

3. **参数设置失败**
   - 检查参数范围
   - 验证当前系统状态
   - 确认硬件支持

4. **测量数据异常**
   - 检查电极连接
   - 验证电解液
   - 调整测量参数

### 调试工具

```c
// 启用调试输出
#define CV_DEBUG_ENABLE 1

// 调试宏
#define CV_DEBUG_PRINT(fmt, ...) \
    printf("[CV_DEBUG] " fmt "\r\n", ##__VA_ARGS__)

// 参数验证
void CV_ValidateAllParams(void) {
    CV_Params_Type params;
    CV_GetAllParams(&params);
    
    CV_DEBUG_PRINT("Voltage: %.1f to %.1f V", 
                   params.startVolt, params.peakVolt);
    CV_DEBUG_PRINT("RTIA Index: %u", params.rtiaIndex);
    CV_DEBUG_PRINT("Scan Rate: %.3f mV/s", params.scanRate);
}
```

## 扩展功能

### 1. 数据记录
- 自动保存测量数据
- 支持多种文件格式
- 时间戳和元数据记录

### 2. 远程控制
- TCP/IP网络接口
- Web界面控制
- 移动应用支持

### 3. 高级分析
- 实时数据处理
- 峰值检测算法
- 自动参数优化

## 技术支持

- **文档**: 详见设计文档和API参考
- **示例**: 参考CV_Example.c
- **社区**: ADI技术论坛
- **联系**: 技术支持邮箱

## 版本历史

- **v1.0.0**: 初始版本，基本功能实现
- **v1.1.0**: 添加高级测量模式
- **v1.2.0**: 优化串口通信协议
- **v2.0.0**: 重构代码架构，提升性能

## 许可证

本项目基于ADI软件许可证发布，仅供学习和研究使用。