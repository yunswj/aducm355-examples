/**
 * @file CV_Example.c
 * @brief 循环伏安法串口控制示例代码
 * @details 演示如何通过串口实时控制循环伏安法测量参数
 * @author ADI Application Team
 * @date 2024
 */

#include "CV_Controller.h"
#include "UART_Parser.h"
#include "ad5940.h"
#include <stdio.h>
#include <string.h>

/* 全局变量 */
static char g_ResponseBuffer[512];
static BoolFlag g_MeasurementActive = bFALSE;

/* 函数声明 */
void CV_ExampleInit(void);
void CV_ExampleLoop(void);
void CV_ProcessUartData(void);
void CV_ProcessMeasurementData(void);
void CV_SendMeasurementData(float voltage, float current);
void CV_PrintWelcomeMessage(void);
void CV_PrintParameterStatus(void);

/* main函数已移至main.c文件 */

/**
 * @brief 示例初始化
 */
void CV_ExampleInit(void)
{
    /* 初始化AD5940 */
    AD5940_MCUResourceInit(0);

    /* 初始化循环伏安法控制器 */
    CV_Init();

    /* 初始化串口解析器 */
    UART_ParserInit();

    /* 设置默认参数 */
    CV_SetVoltageRange(-1.0f, 1.0f);          /* 电压范围: -1V 到 +1V */
    CV_SetCurrentRange(10);                   /* 电流档: RTIA_INT_10K */
    CV_SetScanRate(0.1f);                     /* 扫速: 100 mV/s */
    CV_SetScanParams(200, 1000);              /* 步数: 200, 持续时间: 1000ms */
    CV_SetMeasurementMode(CV_MODE_FAST_SCAN); /* 快速扫描模式 */

    printf("CV Example initialized successfully!\n");
}

/**
 * @brief 主循环
 */
void CV_ExampleLoop(void)
{
    /* 处理串口数据 */
    CV_ProcessUartData();

    /* 处理测量数据 */
    if (g_MeasurementActive)
    {
        CV_ProcessMeasurementData();
    }

    /* 延时 */
    AD5940_Delay10us(100); /* 1ms延时 */
}

/**
 * @brief 处理串口数据
 */
void CV_ProcessUartData(void)
{
    /* 模拟从串口接收字符 */
    /* 实际应用中，这里应该从UART中断或DMA中获取数据 */

    /* 检查是否有完整命令 */
    if (UART_ProcessReadyCommand(g_ResponseBuffer))
    {
        /* 发送响应 */
        printf("%s", g_ResponseBuffer);
    }
}

/**
 * @brief 处理测量数据
 */
void CV_ProcessMeasurementData(void)
{
    static uint32_t lastDataTime = 0;
    uint32_t currentTime = AD5940_GetMCUIntFlag(); /* 获取当前时间戳 */

    /* 检查是否有新的测量数据 */
    if (currentTime - lastDataTime > 10)
    { /* 每10ms检查一次 */
        lastDataTime = currentTime;

        /* 模拟获取测量数据 */
        float voltage, current;
        if (CV_GetMeasurementData(&voltage, &current) == AD5940ERR_OK)
        {
            CV_SendMeasurementData(voltage, current);
        }

        /* 检查测量是否完成 */
        if (CV_GetMeasurementState() == CV_STATE_IDLE)
        {
            g_MeasurementActive = bFALSE;
            printf("$INFO,Measurement completed\r\n");
        }
    }
}

/**
 * @brief 发送测量数据
 */
void CV_SendMeasurementData(float voltage, float current)
{
    char dataResponse[128];
    uint32_t timestamp = AD5940_GetMCUIntFlag();

    UART_GenerateDataResponse(voltage, current, timestamp, dataResponse);
    printf("%s", dataResponse);
}

/**
 * @brief 打印欢迎信息
 */
void CV_PrintWelcomeMessage(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  ADI AD5940 Cyclic Voltammetry Demo   \n");
    printf("     Real-time Parameter Control       \n");
    printf("========================================\n");
    printf("\n");
    printf("Available Commands:\n");
    printf("  $SVR,start,peak*XX     - Set Voltage Range\n");
    printf("  $SCA,rtia_index*XX     - Set Current Range\n");
    printf("  $SSR,rate*XX           - Set Scan Rate\n");
    printf("  $SSP,steps,duration*XX - Set Scan Parameters\n");
    printf("  $SMD,mode*XX           - Set Measurement Mode\n");
    printf("  $START*XX              - Start Measurement\n");
    printf("  $STOP*XX               - Stop Measurement\n");
    printf("  $PAUSE*XX              - Pause Measurement\n");
    printf("  $RESUME*XX             - Resume Measurement\n");
    printf("  $QVR*XX                - Query Voltage Range\n");
    printf("  $QCA*XX                - Query Current Range\n");
    printf("  $QSR*XX                - Query Scan Rate\n");
    printf("  $QSP*XX                - Query Scan Parameters\n");
    printf("  $QST*XX                - Query Status\n");
    printf("  $QALL*XX               - Query All Parameters\n");
    printf("  $RESET*XX              - Reset System\n");
    printf("  $HELP*XX               - Show Help\n");
    printf("\n");
    printf("Parameter Ranges:\n");
    printf("  Voltage: %.1f V to %.1f V\n", CV_VOLTAGE_MIN, CV_VOLTAGE_MAX);
    printf("  Scan Rate: %.3f mV/s to %.1f V/s\n", CV_SCAN_RATE_MIN, CV_SCAN_RATE_MAX);
    printf("  RTIA Index: 0-26 (see manual for values)\n");
    printf("  Steps: %d to %d\n", CV_STEP_MIN, CV_STEP_MAX);
    printf("  Duration: %d ms to %d ms\n", CV_DURATION_MIN, CV_DURATION_MAX);
    printf("\n");

    /* 打印当前参数状态 */
    CV_PrintParameterStatus();

    printf("Ready for commands...\n");
    printf("----------------------------------------\n");
}

/**
 * @brief 打印参数状态
 */
void CV_PrintParameterStatus(void)
{
    CV_Params_Type params;

    if (CV_GetAllParams(&params) == AD5940ERR_OK)
    {
        printf("Current Parameters:\n");
        printf("  Voltage Range: %.1f V to %.1f V\n", params.startVolt, params.peakVolt);
        printf("  Current Range: RTIA Index %u\n", params.rtiaIndex);
        printf("  Scan Rate: %.3f mV/s\n", params.scanRate);
        printf("  Scan Steps: %u\n", params.stepNumber);
        printf("  Duration: %u ms\n", params.duration);
        printf("  Status: %s\n", (params.state == CV_STATE_IDLE) ? "Idle" : (params.state == CV_STATE_CONFIGURED) ? "Configured"
                                                                        : (params.state == CV_STATE_MEASURING)    ? "Measuring"
                                                                        : (params.state == CV_STATE_PAUSED)       ? "Paused"
                                                                                                                  : "Error");
    }
    printf("\n");
}

/* 串口中断处理函数示例 */
/**
 * @brief UART接收中断处理函数
 * @note 这是一个示例函数，实际实现需要根据具体的MCU和UART配置
 */
void UART_RxInterruptHandler(void)
{
    char receivedChar;

    /* 从UART硬件寄存器读取字符 */
    /* receivedChar = UART_ReadChar(); */

    /* 处理接收到的字符 */
    UART_ProcessChar(receivedChar);
}

/* 测量完成回调函数示例 */
/**
 * @brief 测量完成回调函数
 * @param pData 测量数据指针
 * @param DataCount 数据点数量
 */
void CV_MeasurementCompleteCallback(fImpPol_Type *pData, uint32_t DataCount)
{
    /* 处理测量完成事件 */
    g_MeasurementActive = bFALSE;

    /* 发送完成通知 */
    printf("$INFO,Measurement completed with %u data points\r\n", DataCount);

    /* 可以在这里保存数据到文件或发送到上位机 */
    for (uint32_t i = 0; i < DataCount; i++)
    {
        float voltage = pData[i].Magnitude; /* 电压值 */
        float current = pData[i].Phase;     /* 电流值 */

        /* 发送数据点 */
        CV_SendMeasurementData(voltage, current);
    }
}

/* 错误处理函数示例 */
/**
 * @brief 错误处理函数
 * @param errorCode 错误代码
 */
void CV_ErrorHandler(AD5940Err errorCode)
{
    printf("CV Error occurred: ");

    switch (errorCode)
    {
    case AD5940ERR_OK:
        printf("No error\n");
        break;
    case AD5940ERR_ERROR:
        printf("General error\n");
        break;
    case AD5940ERR_PARA:
        printf("Parameter error\n");
        break;
    case AD5940ERR_NULLP:
        printf("Null pointer error\n");
        break;
    case AD5940ERR_BUSY:
        printf("Device busy\n");
        break;
    case AD5940ERR_NOTINIT:
        printf("Device not initialized\n");
        break;
    default:
        printf("Unknown error (code: %d)\n", errorCode);
        break;
    }

    /* 停止当前测量 */
    CV_StopMeasurement();

    /* 重置状态 */
    g_MeasurementActive = bFALSE;
}

/* 参数验证示例函数 */
/* CV_ValidateVoltageRange和CV_ValidateScanRate函数已移至CV_Controller.c */

/**
 * @brief 计算最优扫描参数
 * @param scanRate 扫速 (mV/s)
 * @param voltageRange 电压范围 (V)
 * @param pSteps 输出步数
 * @param pDuration 输出持续时间
 * @return 计算结果
 */
AD5940Err CV_CalculateOptimalParams(float scanRate, float voltageRange,
                                    uint32_t *pSteps, uint32_t *pDuration)
{
    if (pSteps == NULL || pDuration == NULL)
    {
        return AD5940ERR_NULLP;
    }

    /* 根据扫速和电压范围计算最优参数 */
    float totalTime = (voltageRange * 1000.0f) / scanRate; /* 总时间 (ms) */

    /* 选择合适的步数 */
    if (scanRate <= 1.0f)
    {
        *pSteps = (uint32_t)(voltageRange * 1000.0f / 0.28f); /* 0.28mV步长 */
    }
    else if (scanRate <= 100.0f)
    {
        *pSteps = (uint32_t)(voltageRange * 1000.0f / 1.0f); /* 1mV步长 */
    }
    else
    {
        *pSteps = (uint32_t)(voltageRange * 1000.0f / 10.0f); /* 10mV步长 */
    }

    /* 限制步数范围 */
    if (*pSteps < CV_STEP_MIN)
        *pSteps = CV_STEP_MIN;
    if (*pSteps > CV_STEP_MAX)
        *pSteps = CV_STEP_MAX;

    /* 计算每步持续时间 */
    *pDuration = (uint32_t)(totalTime / (*pSteps));

    /* 限制持续时间范围 */
    if (*pDuration < CV_DURATION_MIN)
        *pDuration = CV_DURATION_MIN;
    if (*pDuration > CV_DURATION_MAX)
        *pDuration = CV_DURATION_MAX;

    printf("$INFO,Calculated params - Steps: %u, Duration: %u ms\r\n", *pSteps, *pDuration);

    return AD5940ERR_OK;
}

/* 数据处理示例函数 */
/**
 * @brief 处理原始测量数据
 * @param pRawData 原始数据
 * @param dataCount 数据点数
 * @param pProcessedData 处理后数据
 * @return 处理结果
 */
AD5940Err CV_ProcessRawData(fImpPol_Type *pRawData, uint32_t dataCount,
                            CV_DataPoint_Type *pProcessedData)
{
    if (pRawData == NULL || pProcessedData == NULL)
    {
        return AD5940ERR_NULLP;
    }

    for (uint32_t i = 0; i < dataCount; i++)
    {
        /* 电压转换 */
        pProcessedData[i].voltage = pRawData[i].Magnitude;

        /* 电流转换和校准 */
        pProcessedData[i].current = pRawData[i].Phase;

        /* 时间戳 */
        pProcessedData[i].timestamp = i * 10; /* 假设10ms间隔 */

        /* 数据质量检查 */
        if (pProcessedData[i].current > 1e-3f || pProcessedData[i].current < -1e-3f)
        {
            pProcessedData[i].quality = CV_DATA_QUALITY_SATURATED;
        }
        else if (pProcessedData[i].current > 1e-9f || pProcessedData[i].current < -1e-9f)
        {
            pProcessedData[i].quality = CV_DATA_QUALITY_GOOD;
        }
        else
        {
            pProcessedData[i].quality = CV_DATA_QUALITY_NOISY;
        }
    }

    return AD5940ERR_OK;
}