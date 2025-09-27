/**
 * @file CV_Controller.h
 * @brief 循环伏安法实时参数控制器头文件
 * @details 提供电压范围、电流档、扫速等关键参数的动态配置接口
 * @author ADI Application Team
 * @date 2024
 */

#ifndef _CV_CONTROLLER_H_
#define _CV_CONTROLLER_H_

#include "ad5940.h"
#include "Ramp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 错误代码定义 */
#define CV_ERR_NONE 0x00               /**< 无错误 */
#define CV_ERR_PARAM_RANGE 0x01        /**< 参数超出范围 */
#define CV_ERR_PARAM_FORMAT 0x02       /**< 参数格式错误 */
#define CV_ERR_SYSTEM_BUSY 0x04        /**< 系统忙碌 */
#define CV_ERR_NOT_INITIALIZED 0x08    /**< 未初始化 */
#define CV_ERR_MEASUREMENT_ACTIVE 0x10 /**< 测量进行中 */

/* 兼容性错误代码定义 */
#define CV_ERR_OK 0
#define CV_ERR_INVALID_PARAM -1
#define CV_ERR_BUSY -3
#define CV_ERR_TIMEOUT -4

/* AD5940错误代码扩展定义 */
#ifndef AD5940ERR_BUSY
#define AD5940ERR_BUSY -12      /**< Device is busy */
#endif
#ifndef AD5940ERR_NOTINIT
#define AD5940ERR_NOTINIT -13   /**< Device not initialized */
#endif
#ifndef AD5940ERR_FATAL
#define AD5940ERR_FATAL -14     /**< Fatal error */
#endif

/* 电压范围限制 (V) */
#define CV_VOLTAGE_MIN          -2.4f       /**< 最小电压 (V) */
#define CV_VOLTAGE_MAX          2.4f        /**< 最大电压 (V) */
#define CV_VOLTAGE_MIN_DIFF     0.01f       /**< 最小电压差 (V) */

/* 兼容性定义 */
#define CV_VOLT_MIN             CV_VOLTAGE_MIN
#define CV_VOLT_MAX             CV_VOLTAGE_MAX
#define CV_VOLT_MIN_DIFF        CV_VOLTAGE_MIN_DIFF

/* 循环伏安法参数限制 */
#define CV_SCAN_RATE_MIN        0.001f      /**< 最小扫速 (V/s) */
#define CV_SCAN_RATE_MAX        1000.0f     /**< 最大扫速 (V/s) */
#define CV_DURATION_MIN         1           /**< 最小持续时间 (ms) */
#define CV_DURATION_MAX         10000       /**< 最大持续时间 (ms) */
#define CV_SAMPLE_DELAY_MIN     0.001f      /**< 最小采样延迟 (s) */

/* 步数限制 */
#define CV_STEP_MIN             50          /**< 最小步数 */
#define CV_STEP_MAX             2000        /**< 最大步数 */

/* 兼容性定义 */
#define CV_STEP_NUM_MIN         CV_STEP_MIN
#define CV_STEP_NUM_MAX         CV_STEP_MAX

/* 采样延迟限制 (ms) */
#define CV_SAMPLE_DELAY_MAX 1000.0f

/* RTIA电阻索引范围 */
#define CV_RTIA_INDEX_MIN 1
#define CV_RTIA_INDEX_MAX 26

    /**
     * @brief 循环伏安法测量状态枚举
     */
    typedef enum
    {
        CV_STATE_IDLE = 0,   /**< 空闲状态 */
        CV_STATE_CONFIGURED, /**< 已配置状态 */
        CV_STATE_MEASURING,  /**< 测量中 */
        CV_STATE_PAUSED,     /**< 暂停状态 */
        CV_STATE_ERROR       /**< 错误状态 */
    } CV_State_Type;

    /**
     * @brief 预定义测量模式
     */
    typedef enum
    {
        CV_MODE_FAST_SCAN = 0,  /**< 快速扫描模式 */
        CV_MODE_HIGH_PRECISION, /**< 高精度模式 */
        CV_MODE_LOW_CURRENT,    /**< 低电流模式 */
        CV_MODE_CUSTOM          /**< 自定义模式 */
    } CV_MeasMode_Type;

    /**
     * @brief 电流档位信息结构体
     */
    typedef struct
    {
        uint32_t rtiaIndex;      /**< RTIA索引 */
        uint32_t rtiaValue;      /**< RTIA电阻值 (Ohm) */
        float maxCurrent;        /**< 最大电流范围 (mA) */
        const char *description; /**< 描述字符串 */
    } CV_CurrentRange_Type;

    /**
     * @brief 循环伏安法参数结构体
     */
    typedef struct
    {
        float startVolt;     /**< 起始电压 (mV) */
        float peakVolt;      /**< 峰值电压 (mV) */
        uint32_t rtiaIndex;  /**< RTIA索引 */
        float scanRate;      /**< 扫速 (mV/s) */
        uint32_t stepNumber; /**< 步数 */
        uint32_t duration;   /**< 持续时间 (ms) */
        float sampleDelay;   /**< 采样延迟 (ms) */
        CV_State_Type state; /**< 当前状态 */
        uint32_t errorCode;  /**< 错误代码 */
    } CV_Params_Type;

    /* 数据质量枚举 */
typedef enum {
    CV_DATA_QUALITY_GOOD = 0,
    CV_DATA_QUALITY_NOISY,
    CV_DATA_QUALITY_SATURATED
} CV_DataQuality_Type;

/* CV数据点结构体 */
typedef struct {
    float voltage;      /* 电压值 (V) */
    float current;      /* 电流值 (A) */
    uint32_t timestamp; /* 时间戳 (ms) */
    CV_DataQuality_Type quality; /* 数据质量 */
} CV_DataPoint_Type;

/* 全局变量声明 */
    extern CV_Params_Type g_CVParams;
    extern const CV_CurrentRange_Type g_CurrentRangeTable[];

    /* 函数声明 */

    /**
     * @brief 初始化循环伏安法控制器
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_Init(void);

    /**
     * @brief 反初始化循环伏安法控制器
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_DeInit(void);

    /* 参数设置接口 */

    /**
     * @brief 设置电压范围
     * @param startVolt 起始电压 (mV)
     * @param peakVolt 峰值电压 (mV)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetVoltageRange(float startVolt, float peakVolt);

    /**
     * @brief 设置电流档位
     * @param rtiaIndex RTIA索引 (1-26)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetCurrentRange(uint32_t rtiaIndex);

    /**
     * @brief 设置扫速
     * @param scanRate 扫速 (mV/s)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetScanRate(float scanRate);

    /**
     * @brief 设置扫描参数（步数和持续时间）
     * @param stepNum 步数
     * @param duration 持续时间 (ms)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetScanParams(uint32_t stepNum, uint32_t duration);

    /**
     * @brief 设置采样延迟
     * @param sampleDelay 采样延迟 (ms)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetSampleDelay(float sampleDelay);

    /**
     * @brief 应用预定义测量模式
     * @param mode 测量模式
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_SetMeasurementMode(CV_MeasMode_Type mode);

    /* 参数查询接口 */

    /**
     * @brief 获取电压范围
     * @param pStartVolt 起始电压指针 (mV)
     * @param pPeakVolt 峰值电压指针 (mV)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_GetVoltageRange(float *pStartVolt, float *pPeakVolt);

    /**
     * @brief 获取电流档位
     * @param pRtiaIndex RTIA索引指针
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_GetCurrentRange(uint32_t *pRtiaIndex);

    /**
     * @brief 获取扫速
     * @param pScanRate 扫速指针 (mV/s)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_GetScanRate(float *pScanRate);

    /**
     * @brief 获取扫描参数
     * @param pStepNum 步数指针
     * @param pDuration 持续时间指针 (ms)
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_GetScanParams(uint32_t *pStepNum, uint32_t *pDuration);

    /**
     * @brief 获取所有参数
     * @param pParams 参数结构体指针
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_GetAllParams(CV_Params_Type *pParams);

    /* 测量控制接口 */

    /**
     * @brief 开始测量
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_StartMeasurement(void);

    /**
     * @brief 停止测量
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_StopMeasurement(void);

    /**
     * @brief 暂停测量
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_PauseMeasurement(void);

    /**
     * @brief 恢复测量
     * @return AD5940Err 错误代码
     */
    AD5940Err CV_ResumeMeasurement(void);

    /**
     * @brief 获取测量状态
     * @return CV_State_Type 当前状态
     */
    CV_State_Type CV_GetMeasurementState(void);

    /**
     * @brief 获取测量数据
     * @param pVoltage 电压输出指针
     * @param pCurrent 电流输出指针
     * @return 操作结果
     */
    AD5940Err CV_GetMeasurementData(float *pVoltage, float *pCurrent);

    /* 参数验证接口 */

    /**
     * @brief 验证电压范围
     * @param startVolt 起始电压 (mV)
     * @param peakVolt 峰值电压 (mV)
     * @return bTRUE: 有效, bFALSE: 无效
     */
    BoolFlag CV_ValidateVoltageRange(float startVolt, float peakVolt);

    /**
     * @brief 验证电流档位
     * @param rtiaIndex RTIA索引
     * @return bTRUE: 有效, bFALSE: 无效
     */
    BoolFlag CV_ValidateCurrentRange(uint32_t rtiaIndex);

    /**
     * @brief 验证扫速
     * @param scanRate 扫速 (mV/s)
     * @return bTRUE: 有效, bFALSE: 无效
     */
    BoolFlag CV_ValidateScanRate(float scanRate);

    /**
     * @brief 验证扫描参数
     * @param stepNum 步数
     * @param duration 持续时间 (ms)
     * @return bTRUE: 有效, bFALSE: 无效
     */
    BoolFlag CV_ValidateScanParams(uint32_t stepNum, uint32_t duration);

    /* 工具函数 */

    /**
     * @brief 根据RTIA索引获取电阻值
     * @param rtiaIndex RTIA索引
     * @return uint32_t 电阻值 (Ohm)，0表示无效索引
     */
    uint32_t CV_GetRtiaValue(uint32_t rtiaIndex);

    /**
     * @brief 根据RTIA索引获取最大电流
     * @param rtiaIndex RTIA索引
     * @return float 最大电流 (mA)，0表示无效索引
     */
    float CV_GetMaxCurrent(uint32_t rtiaIndex);

    /**
     * @brief 计算扫速
     * @param startVolt 起始电压 (mV)
     * @param peakVolt 峰值电压 (mV)
     * @param duration 持续时间 (ms)
     * @return float 扫速 (mV/s)
     */
    float CV_CalculateScanRate(float startVolt, float peakVolt, uint32_t duration);

    /**
     * @brief 计算步进电压
     * @param startVolt 起始电压 (mV)
     * @param peakVolt 峰值电压 (mV)
     * @param stepNum 步数
     * @return float 步进电压 (mV)
     */
    float CV_CalculateStepVoltage(float startVolt, float peakVolt, uint32_t stepNum);

    /**
     * @brief 获取错误描述字符串
     * @param errorCode 错误代码
     * @return const char* 错误描述
     */
    const char *CV_GetErrorString(uint32_t errorCode);

#ifdef __cplusplus
}
#endif

#endif /* _CV_CONTROLLER_H_ */