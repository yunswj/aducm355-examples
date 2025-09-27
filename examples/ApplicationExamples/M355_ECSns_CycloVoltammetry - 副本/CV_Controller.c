/**
 * @file CV_Controller.c
 * @brief 循环伏安法实时参数控制器实现文件
 * @details 实现电压范围、电流档、扫速等关键参数的动态配置功能
 * @author ADI Application Team
 * @date 2024
 */

#include "CV_Controller.h"
#include <string.h>
#include <math.h>

/* 全局变量定义 */
CV_Params_Type g_CVParams = {0};

/* RTIA电阻值表 */
static const uint32_t g_RtiaValueTable[] = {
    0,      // LPTIARTIA_OPEN
    200,    // LPTIARTIA_200R
    1000,   // LPTIARTIA_1K
    2000,   // LPTIARTIA_2K
    3000,   // LPTIARTIA_3K
    4000,   // LPTIARTIA_4K
    6000,   // LPTIARTIA_6K
    8000,   // LPTIARTIA_8K
    10000,  // LPTIARTIA_10K
    12000,  // LPTIARTIA_12K
    16000,  // LPTIARTIA_16K
    20000,  // LPTIARTIA_20K
    24000,  // LPTIARTIA_24K
    30000,  // LPTIARTIA_30K
    32000,  // LPTIARTIA_32K
    40000,  // LPTIARTIA_40K
    48000,  // LPTIARTIA_48K
    64000,  // LPTIARTIA_64K
    85000,  // LPTIARTIA_85K
    96000,  // LPTIARTIA_96K
    100000, // LPTIARTIA_100K
    120000, // LPTIARTIA_120K
    128000, // LPTIARTIA_128K
    160000, // LPTIARTIA_160K
    196000, // LPTIARTIA_196K
    256000, // LPTIARTIA_256K
    512000  // LPTIARTIA_512K
};

/* 电流档位信息表 */
const CV_CurrentRange_Type g_CurrentRangeTable[] = {
    {0, 0, 0.0f, "OPEN"},
    {1, 200, 9.0f, "200R (±9mA)"},
    {2, 1000, 1.8f, "1K (±1.8mA)"},
    {3, 2000, 0.9f, "2K (±900μA)"},
    {4, 3000, 0.6f, "3K (±600μA)"},
    {5, 4000, 0.45f, "4K (±450μA)"},
    {6, 6000, 0.3f, "6K (±300μA)"},
    {7, 8000, 0.225f, "8K (±225μA)"},
    {8, 10000, 0.18f, "10K (±180μA)"},
    {9, 12000, 0.15f, "12K (±150μA)"},
    {10, 16000, 0.1125f, "16K (±112.5μA)"},
    {11, 20000, 0.09f, "20K (±90μA)"},
    {12, 24000, 0.075f, "24K (±75μA)"},
    {13, 30000, 0.06f, "30K (±60μA)"},
    {14, 32000, 0.056f, "32K (±56μA)"},
    {15, 40000, 0.045f, "40K (±45μA)"},
    {16, 48000, 0.0375f, "48K (±37.5μA)"},
    {17, 64000, 0.028f, "64K (±28μA)"},
    {18, 85000, 0.021f, "85K (±21μA)"},
    {19, 96000, 0.019f, "96K (±19μA)"},
    {20, 100000, 0.018f, "100K (±18μA)"},
    {21, 120000, 0.015f, "120K (±15μA)"},
    {22, 128000, 0.014f, "128K (±14μA)"},
    {23, 160000, 0.011f, "160K (±11μA)"},
    {24, 196000, 0.009f, "196K (±9μA)"},
    {25, 256000, 0.007f, "256K (±7μA)"},
    {26, 512000, 0.0035f, "512K (±3.5μA)"}};

/* 错误描述字符串表 */
static const char *g_ErrorStrings[] = {
    "No Error",
    "Parameter Out of Range",
    "Parameter Format Error",
    "Reserved",
    "System Busy",
    "Reserved",
    "Reserved",
    "Reserved",
    "Not Initialized",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Measurement Active"};

/* 私有函数声明 */
static AD5940Err CV_ApplyParametersToRamp(void);

/**
 * @brief 初始化循环伏安法控制器
 */
AD5940Err CV_Init(void)
{
    /* 初始化参数结构体 */
    memset(&g_CVParams, 0, sizeof(CV_Params_Type));

    /* 设置默认参数 */
    g_CVParams.startVolt = -500.0f; /* -500mV */
    g_CVParams.peakVolt = 500.0f;   /* +500mV */
    g_CVParams.rtiaIndex = 11;      /* 20K (LPTIARTIA_20K) */
    g_CVParams.stepNumber = 400;    /* 400步 */
    g_CVParams.duration = 10000;    /* 10秒 */
    g_CVParams.sampleDelay = 2.0f;  /* 2ms */
    g_CVParams.scanRate = CV_CalculateScanRate(g_CVParams.startVolt,
                                               g_CVParams.peakVolt,
                                               g_CVParams.duration);
    g_CVParams.state = CV_STATE_CONFIGURED;
    g_CVParams.errorCode = CV_ERR_NONE;

    return AD5940ERR_OK;
}

/**
 * @brief 反初始化循环伏安法控制器
 */
AD5940Err CV_DeInit(void)
{
    /* 停止测量 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        CV_StopMeasurement();
    }

    /* 重置参数 */
    memset(&g_CVParams, 0, sizeof(CV_Params_Type));
    g_CVParams.state = CV_STATE_IDLE;

    return AD5940ERR_OK;
}

/**
 * @brief 设置电压范围
 */
AD5940Err CV_SetVoltageRange(float startVolt, float peakVolt)
{
    /* 参数验证 */
    if (!CV_ValidateVoltageRange(startVolt, peakVolt))
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    /* 更新参数 */
    g_CVParams.startVolt = startVolt;
    g_CVParams.peakVolt = peakVolt;

    /* 重新计算扫速 */
    g_CVParams.scanRate = CV_CalculateScanRate(startVolt, peakVolt, g_CVParams.duration);

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 设置电流档位
 */
AD5940Err CV_SetCurrentRange(uint32_t rtiaIndex)
{
    /* 参数验证 */
    if (!CV_ValidateCurrentRange(rtiaIndex))
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    /* 更新参数 */
    g_CVParams.rtiaIndex = rtiaIndex;

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 设置扫速
 */
AD5940Err CV_SetScanRate(float scanRate)
{
    /* 参数验证 */
    if (!CV_ValidateScanRate(scanRate))
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    /* 根据扫速计算持续时间 */
    float voltageRange = fabs(g_CVParams.peakVolt - g_CVParams.startVolt);
    uint32_t newDuration = (uint32_t)(voltageRange / scanRate);

    /* 验证计算结果 */
    if (!CV_ValidateScanParams(g_CVParams.stepNumber, newDuration))
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 更新参数 */
    g_CVParams.scanRate = scanRate;
    g_CVParams.duration = newDuration;

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 设置扫描参数
 */
AD5940Err CV_SetScanParams(uint32_t stepNum, uint32_t duration)
{
    /* 参数验证 */
    if (!CV_ValidateScanParams(stepNum, duration))
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    /* 更新参数 */
    g_CVParams.stepNumber = stepNum;
    g_CVParams.duration = duration;

    /* 重新计算扫速 */
    g_CVParams.scanRate = CV_CalculateScanRate(g_CVParams.startVolt,
                                               g_CVParams.peakVolt,
                                               duration);

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 设置采样延迟
 */
AD5940Err CV_SetSampleDelay(float sampleDelay)
{
    /* 参数验证 */
    if (sampleDelay < CV_SAMPLE_DELAY_MIN)
    {
        g_CVParams.errorCode = CV_ERR_PARAM_RANGE;
        return AD5940ERR_PARA;
    }

    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    /* 更新参数 */
    g_CVParams.sampleDelay = sampleDelay;

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 应用预定义测量模式
 */
AD5940Err CV_SetMeasurementMode(CV_MeasMode_Type mode)
{
    /* 检查测量状态 */
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        g_CVParams.errorCode = CV_ERR_MEASUREMENT_ACTIVE;
        return AD5940ERR_BUSY;
    }

    switch (mode)
    {
    case CV_MODE_FAST_SCAN:
        g_CVParams.startVolt = -1000.0f;
        g_CVParams.peakVolt = 1000.0f;
        g_CVParams.rtiaIndex = 11; /* 20K */
        g_CVParams.stepNumber = 200;
        g_CVParams.duration = 2000; /* 2秒 */
        g_CVParams.sampleDelay = 1.0f;
        break;

    case CV_MODE_HIGH_PRECISION:
        g_CVParams.startVolt = -500.0f;
        g_CVParams.peakVolt = 500.0f;
        g_CVParams.rtiaIndex = 20; /* 100K */
        g_CVParams.stepNumber = 1000;
        g_CVParams.duration = 50000; /* 50秒 */
        g_CVParams.sampleDelay = 5.0f;
        break;

    case CV_MODE_LOW_CURRENT:
        g_CVParams.startVolt = -200.0f;
        g_CVParams.peakVolt = 200.0f;
        g_CVParams.rtiaIndex = 26; /* 512K */
        g_CVParams.stepNumber = 400;
        g_CVParams.duration = 20000; /* 20秒 */
        g_CVParams.sampleDelay = 10.0f;
        break;

    case CV_MODE_CUSTOM:
        /* 保持当前参数不变 */
        break;

    default:
        g_CVParams.errorCode = CV_ERR_PARAM_FORMAT;
        return AD5940ERR_PARA;
    }

    /* 重新计算扫速 */
    g_CVParams.scanRate = CV_CalculateScanRate(g_CVParams.startVolt,
                                               g_CVParams.peakVolt,
                                               g_CVParams.duration);

    /* 应用到RAMP配置 */
    return CV_ApplyParametersToRamp();
}

/**
 * @brief 获取电压范围
 */
AD5940Err CV_GetVoltageRange(float *pStartVolt, float *pPeakVolt)
{
    if (pStartVolt == NULL || pPeakVolt == NULL)
    {
        return AD5940ERR_NULLP;
    }

    *pStartVolt = g_CVParams.startVolt;
    *pPeakVolt = g_CVParams.peakVolt;

    return AD5940ERR_OK;
}

/**
 * @brief 获取电流档位
 */
AD5940Err CV_GetCurrentRange(uint32_t *pRtiaIndex)
{
    if (pRtiaIndex == NULL)
    {
        return AD5940ERR_NULLP;
    }

    *pRtiaIndex = g_CVParams.rtiaIndex;

    return AD5940ERR_OK;
}

/**
 * @brief 获取扫速
 */
AD5940Err CV_GetScanRate(float *pScanRate)
{
    if (pScanRate == NULL)
    {
        return AD5940ERR_NULLP;
    }

    *pScanRate = g_CVParams.scanRate;

    return AD5940ERR_OK;
}

/**
 * @brief 获取扫描参数
 */
AD5940Err CV_GetScanParams(uint32_t *pStepNum, uint32_t *pDuration)
{
    if (pStepNum == NULL || pDuration == NULL)
    {
        return AD5940ERR_NULLP;
    }

    *pStepNum = g_CVParams.stepNumber;
    *pDuration = g_CVParams.duration;

    return AD5940ERR_OK;
}

/**
 * @brief 获取所有参数
 */
AD5940Err CV_GetAllParams(CV_Params_Type *pParams)
{
    if (pParams == NULL)
    {
        return AD5940ERR_NULLP;
    }

    memcpy(pParams, &g_CVParams, sizeof(CV_Params_Type));

    return AD5940ERR_OK;
}

/**
 * @brief 开始测量
 */
AD5940Err CV_StartMeasurement(void)
{
    if (g_CVParams.state == CV_STATE_MEASURING)
    {
        return AD5940ERR_OK; /* 已经在测量中 */
    }

    if (g_CVParams.state != CV_STATE_CONFIGURED && g_CVParams.state != CV_STATE_PAUSED)
    {
        g_CVParams.errorCode = CV_ERR_NOT_INITIALIZED;
        return AD5940ERR_NOTINIT;
    }

    /* 启动RAMP测量 */
    AD5940Err error = AppRAMPCtrl(APPCTRL_START, 0);
    if (error == AD5940ERR_OK)
    {
        g_CVParams.state = CV_STATE_MEASURING;
        g_CVParams.errorCode = CV_ERR_NONE;
    }
    else
    {
        g_CVParams.errorCode = CV_ERR_SYSTEM_BUSY;
    }

    return error;
}

/**
 * @brief 停止测量
 */
AD5940Err CV_StopMeasurement(void)
{
    if (g_CVParams.state != CV_STATE_MEASURING && g_CVParams.state != CV_STATE_PAUSED)
    {
        return AD5940ERR_OK; /* 已经停止 */
    }

    /* 停止RAMP测量 */
    AD5940Err error = AppRAMPCtrl(APPCTRL_STOPNOW, 0);
    if (error == AD5940ERR_OK)
    {
        g_CVParams.state = CV_STATE_CONFIGURED;
        g_CVParams.errorCode = CV_ERR_NONE;
    }
    else
    {
        g_CVParams.errorCode = CV_ERR_SYSTEM_BUSY;
    }

    return error;
}

/**
 * @brief 暂停测量
 */
AD5940Err CV_PauseMeasurement(void)
{
    if (g_CVParams.state != CV_STATE_MEASURING)
    {
        return AD5940ERR_OK; /* 不在测量中 */
    }

    /* 暂停RAMP测量 */
    AD5940Err error = AppRAMPCtrl(APPCTRL_SHUTDOWN, 0);
    if (error == AD5940ERR_OK)
    {
        g_CVParams.state = CV_STATE_PAUSED;
        g_CVParams.errorCode = CV_ERR_NONE;
    }
    else
    {
        g_CVParams.errorCode = CV_ERR_SYSTEM_BUSY;
    }

    return error;
}

/**
 * @brief 恢复测量
 */
AD5940Err CV_ResumeMeasurement(void)
{
    if (g_CVParams.state != CV_STATE_PAUSED)
    {
        return CV_StartMeasurement(); /* 直接开始测量 */
    }

    /* 恢复RAMP测量 */
    AD5940Err error = AppRAMPCtrl(APPCTRL_START, 0);
    if (error == AD5940ERR_OK)
    {
        g_CVParams.state = CV_STATE_MEASURING;
        g_CVParams.errorCode = CV_ERR_NONE;
    }
    else
    {
        g_CVParams.errorCode = CV_ERR_SYSTEM_BUSY;
    }

    return error;
}

/**
 * @brief 获取测量状态
 */
CV_State_Type CV_GetMeasurementState(void)
{
    return g_CVParams.state;
}

/**
 * @brief 验证电压范围
 */
BoolFlag CV_ValidateVoltageRange(float startVolt, float peakVolt)
{
    if (startVolt < CV_VOLT_MIN || startVolt > CV_VOLT_MAX)
        return bFALSE;
    if (peakVolt < CV_VOLT_MIN || peakVolt > CV_VOLT_MAX)
        return bFALSE;
    if (fabs(peakVolt - startVolt) < CV_VOLT_MIN_DIFF)
        return bFALSE;

    return bTRUE;
}

/**
 * @brief 验证电流档位
 */
BoolFlag CV_ValidateCurrentRange(uint32_t rtiaIndex)
{
    return (rtiaIndex >= CV_RTIA_INDEX_MIN && rtiaIndex <= CV_RTIA_INDEX_MAX) ? bTRUE : bFALSE;
}

/**
 * @brief 验证扫速
 */
BoolFlag CV_ValidateScanRate(float scanRate)
{
    return (scanRate >= CV_SCAN_RATE_MIN && scanRate <= CV_SCAN_RATE_MAX) ? bTRUE : bFALSE;
}

/**
 * @brief 验证扫描参数
 */
BoolFlag CV_ValidateScanParams(uint32_t stepNum, uint32_t duration)
{
    if (stepNum < CV_STEP_NUM_MIN || stepNum > CV_STEP_NUM_MAX)
        return bFALSE;
    if (duration < CV_DURATION_MIN || duration > CV_DURATION_MAX)
        return bFALSE;

    /* 检查步进时间是否合理 */
    float stepTime = (float)duration / stepNum;
    if (stepTime < CV_SAMPLE_DELAY_MIN)
        return bFALSE;

    return bTRUE;
}

/**
 * @brief 根据RTIA索引获取电阻值
 */
uint32_t CV_GetRtiaValue(uint32_t rtiaIndex)
{
    if (rtiaIndex > CV_RTIA_INDEX_MAX)
        return 0;
    return g_RtiaValueTable[rtiaIndex];
}

/**
 * @brief 根据RTIA索引获取最大电流
 */
float CV_GetMaxCurrent(uint32_t rtiaIndex)
{
    if (rtiaIndex > CV_RTIA_INDEX_MAX)
        return 0.0f;
    return g_CurrentRangeTable[rtiaIndex].maxCurrent;
}

/**
 * @brief 计算扫速
 */
float CV_CalculateScanRate(float startVolt, float peakVolt, uint32_t duration)
{
    float voltageRange = fabs(peakVolt - startVolt);
    return (voltageRange * 1000.0f) / duration; /* mV/s */
}

/**
 * @brief 计算步进电压
 */
float CV_CalculateStepVoltage(float startVolt, float peakVolt, uint32_t stepNum)
{
    return fabs(peakVolt - startVolt) / stepNum;
}

/**
 * @brief 获取错误描述字符串
 */
const char *CV_GetErrorString(uint32_t errorCode)
{
    uint32_t index = 0;

    /* 找到第一个设置的错误位 */
    while (index < 16 && !(errorCode & (1 << index)))
    {
        index++;
    }

    if (index >= 16)
        index = 0; /* 默认返回"No Error" */

    return g_ErrorStrings[index];
}

/**
 * @brief 应用参数到RAMP配置
 */
static AD5940Err CV_ApplyParametersToRamp(void)
{
    AppRAMPCfg_Type *pRampCfg;

    /* 获取RAMP配置指针 */
    AppRAMPGetCfg(&pRampCfg);
    if (pRampCfg == NULL)
    {
        g_CVParams.errorCode = CV_ERR_NOT_INITIALIZED;
        return AD5940ERR_NULLP;
    }

    /* 更新RAMP配置 */
    pRampCfg->RampStartVolt = g_CVParams.startVolt;
    pRampCfg->RampPeakVolt = g_CVParams.peakVolt;
    pRampCfg->LPTIARtiaSel = g_CVParams.rtiaIndex;
    pRampCfg->StepNumber = g_CVParams.stepNumber;
    pRampCfg->RampDuration = g_CVParams.duration;
    pRampCfg->SampleDelay = g_CVParams.sampleDelay;
    pRampCfg->bParaChanged = bTRUE;

    g_CVParams.errorCode = CV_ERR_NONE;
    return AD5940ERR_OK;
}

/**
 * @brief 获取测量数据
 * @param pVoltage 电压输出指针
 * @param pCurrent 电流输出指针
 * @return 操作结果
 */
AD5940Err CV_GetMeasurementData(float *pVoltage, float *pCurrent)
{
    int32_t dataBuffer[256];  /* 数据缓冲区 */
    uint32_t dataCount = 256;
    
    if (pVoltage == NULL || pCurrent == NULL)
    {
        return AD5940ERR_PARA;
    }
    
    if (g_CVParams.state != CV_STATE_MEASURING)
    {
        return AD5940ERR_NOTINIT;
    }
    
    /* 获取RAMP应用的测量数据 */
    if (AppRAMPISR(dataBuffer, &dataCount) != AD5940ERR_OK)
    {
        return AD5940ERR_BUSY;
    }
    
    if (dataCount > 0)
    {
        /* 返回最新的测量数据 */
        /* 注意：这里需要根据实际的数据格式进行转换 */
        *pVoltage = (float)dataBuffer[dataCount - 1];  /* 最新数据点 */
        *pCurrent = (float)dataBuffer[dataCount - 1];  /* 同一数据点，需要根据实际情况调整 */
        return AD5940ERR_OK;
    }
    
    return AD5940ERR_BUFF;
}