/**
 * @file UART_Parser.h
 * @brief 串口命令解析器头文件
 * @details 提供循环伏安法参数控制的串口命令解析和验证功能
 * @author ADI Application Team
 * @date 2024
 */

#ifndef _UART_PARSER_H_
#define _UART_PARSER_H_

#include "ad5940.h"
#include "CV_Controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 命令解析相关常量 */
#define UART_CMD_START_CHAR         '$'     /**< 命令起始字符 */
#define UART_CMD_END_CHAR1          '\r'    /**< 命令结束字符1 */
#define UART_CMD_END_CHAR2          '\n'    /**< 命令结束字符2 */
#define UART_CMD_SEPARATOR          ','     /**< 参数分隔符 */
#define UART_CMD_CHECKSUM_CHAR      '*'     /**< 校验和分隔符 */

#define UART_CMD_MAX_LENGTH         128     /**< 最大命令长度 */
#define UART_CMD_MAX_PARAMS         8       /**< 最大参数个数 */
#define UART_CMD_MAX_NAME_LEN       8       /**< 最大命令名长度 */
#define UART_RESPONSE_MAX_LENGTH    256     /**< 最大响应长度 */

/* 解析错误代码 */
#define UART_PARSE_OK               0x00    /**< 解析成功 */
#define UART_PARSE_ERR_FORMAT       0x01    /**< 格式错误 */
#define UART_PARSE_ERR_CHECKSUM     0x02    /**< 校验和错误 */
#define UART_PARSE_ERR_CMD_UNKNOWN  0x03    /**< 未知命令 */
#define UART_PARSE_ERR_PARAM_COUNT  0x04    /**< 参数个数错误 */
#define UART_PARSE_ERR_PARAM_VALUE  0x05    /**< 参数值错误 */
#define UART_PARSE_ERR_BUFFER_FULL  0x06    /**< 缓冲区满 */

/**
 * @brief 命令类型枚举
 */
typedef enum {
    UART_CMD_UNKNOWN = 0,       /**< 未知命令 */
    UART_CMD_SVR,               /**< 设置电压范围 */
    UART_CMD_SCA,               /**< 设置电流档 */
    UART_CMD_SSR,               /**< 设置扫速 */
    UART_CMD_SSP,               /**< 设置扫描参数 */
    UART_CMD_SMD,               /**< 设置测量模式 */
    UART_CMD_START,             /**< 开始测量 */
    UART_CMD_STOP,              /**< 停止测量 */
    UART_CMD_PAUSE,             /**< 暂停测量 */
    UART_CMD_RESUME,            /**< 恢复测量 */
    UART_CMD_QVR,               /**< 查询电压范围 */
    UART_CMD_QCA,               /**< 查询电流档 */
    UART_CMD_QSR,               /**< 查询扫速 */
    UART_CMD_QSP,               /**< 查询扫描参数 */
    UART_CMD_QST,               /**< 查询状态 */
    UART_CMD_QALL,              /**< 查询所有参数 */
    UART_CMD_RESET,             /**< 重置系统 */
    UART_CMD_HELP,              /**< 帮助信息 */
    UART_CMD_COUNT              /**< 命令总数 */
} UART_CmdType_Type;

/**
 * @brief 响应类型枚举
 */
typedef enum {
    UART_RESP_ACK = 0,          /**< 确认响应 */
    UART_RESP_NAK,              /**< 否定响应 */
    UART_RESP_DATA,             /**< 数据响应 */
    UART_RESP_INFO              /**< 信息响应 */
} UART_RespType_Type;

/**
 * @brief 命令参数结构体
 */
typedef struct {
    char name[UART_CMD_MAX_NAME_LEN];       /**< 命令名称 */
    uint32_t paramCount;                    /**< 参数个数 */
    char params[UART_CMD_MAX_PARAMS][16];   /**< 参数字符串数组 */
    uint8_t checksum;                       /**< 校验和 */
    BoolFlag hasChecksum;                   /**< 是否包含校验和 */
} UART_Command_Type;

/**
 * @brief 命令处理函数指针类型
 */
typedef uint32_t (*UART_CmdHandler_Type)(const UART_Command_Type *pCmd, char *pResponse);

/**
 * @brief 命令表项结构体
 */
typedef struct {
    const char *cmdName;                    /**< 命令名称 */
    UART_CmdType_Type cmdType;              /**< 命令类型 */
    uint32_t minParams;                     /**< 最小参数个数 */
    uint32_t maxParams;                     /**< 最大参数个数 */
    UART_CmdHandler_Type handler;           /**< 处理函数 */
    const char *description;                /**< 命令描述 */
} UART_CmdEntry_Type;

/**
 * @brief 接收缓冲区结构体
 */
typedef struct {
    char buffer[UART_CMD_MAX_LENGTH];       /**< 接收缓冲区 */
    uint32_t index;                         /**< 当前索引 */
    BoolFlag cmdReady;                      /**< 命令就绪标志 */
} UART_RxBuffer_Type;

/* 全局变量声明 */
extern UART_RxBuffer_Type g_UartRxBuffer;

/* 函数声明 */

/**
 * @brief 初始化串口解析器
 * @return AD5940Err 错误代码
 */
AD5940Err UART_ParserInit(void);

/**
 * @brief 反初始化串口解析器
 * @return AD5940Err 错误代码
 */
AD5940Err UART_ParserDeInit(void);

/**
 * @brief 处理接收到的字符
 * @param ch 接收到的字符
 * @return AD5940Err 错误代码
 */
AD5940Err UART_ProcessChar(char ch);

/**
 * @brief 解析命令字符串
 * @param pCmdStr 命令字符串
 * @param pCmd 解析后的命令结构体
 * @return uint32_t 解析错误代码
 */
uint32_t UART_ParseCommand(const char *pCmdStr, UART_Command_Type *pCmd);

/**
 * @brief 执行命令
 * @param pCmd 命令结构体
 * @param pResponse 响应字符串缓冲区
 * @return uint32_t 执行错误代码
 */
uint32_t UART_ExecuteCommand(const UART_Command_Type *pCmd, char *pResponse);

/**
 * @brief 处理就绪的命令
 * @param pResponse 响应字符串缓冲区
 * @return BoolFlag 是否有命令被处理
 */
BoolFlag UART_ProcessReadyCommand(char *pResponse);

/* 校验和相关函数 */

/**
 * @brief 计算字符串的XOR校验和
 * @param pStr 字符串
 * @param length 字符串长度
 * @return uint8_t 校验和
 */
uint8_t UART_CalculateChecksum(const char *pStr, uint32_t length);

/**
 * @brief 验证命令校验和
 * @param pCmd 命令结构体
 * @param pCmdStr 原始命令字符串
 * @return BoolFlag 校验和是否正确
 */
BoolFlag UART_VerifyChecksum(const UART_Command_Type *pCmd, const char *pCmdStr);

/* 响应生成函数 */

/**
 * @brief 生成ACK响应
 * @param pCmd 命令名称
 * @param pData 数据字符串
 * @param pResponse 响应缓冲区
 * @return uint32_t 响应长度
 */
uint32_t UART_GenerateAckResponse(const char *pCmd, const char *pData, char *pResponse);

/**
 * @brief 生成NAK响应
 * @param pCmd 命令名称
 * @param errorCode 错误代码
 * @param pResponse 响应缓冲区
 * @return uint32_t 响应长度
 */
uint32_t UART_GenerateNakResponse(const char *pCmd, uint32_t errorCode, char *pResponse);

/**
 * @brief 生成数据响应
 * @param voltage 电压值
 * @param current 电流值
 * @param timestamp 时间戳
 * @param pResponse 响应缓冲区
 * @return uint32_t 响应长度
 */
uint32_t UART_GenerateDataResponse(float voltage, float current, uint32_t timestamp, char *pResponse);

/**
 * @brief 生成信息响应
 * @param pInfo 信息字符串
 * @param pResponse 响应缓冲区
 * @return uint32_t 响应长度
 */
uint32_t UART_GenerateInfoResponse(const char *pInfo, char *pResponse);

/* 命令处理函数 */
uint32_t UART_HandleSetVoltageRange(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleSetCurrentRange(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleSetScanRate(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleSetScanParams(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleSetMeasMode(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleStartMeas(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleStopMeas(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandlePauseMeas(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleResumeMeas(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryVoltageRange(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryCurrentRange(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryScanRate(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryScanParams(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryStatus(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleQueryAll(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleReset(const UART_Command_Type *pCmd, char *pResponse);
uint32_t UART_HandleHelp(const UART_Command_Type *pCmd, char *pResponse);

/* 工具函数 */

/**
 * @brief 字符串转浮点数
 * @param pStr 字符串
 * @param pValue 浮点数指针
 * @return BoolFlag 转换是否成功
 */
BoolFlag UART_StrToFloat(const char *pStr, float *pValue);

/**
 * @brief 字符串转整数
 * @param pStr 字符串
 * @param pValue 整数指针
 * @return BoolFlag 转换是否成功
 */
BoolFlag UART_StrToUint32(const char *pStr, uint32_t *pValue);

/**
 * @brief 浮点数转字符串
 * @param value 浮点数
 * @param precision 精度
 * @param pStr 字符串缓冲区
 * @return uint32_t 字符串长度
 */
uint32_t UART_FloatToStr(float value, uint32_t precision, char *pStr);

/**
 * @brief 整数转字符串
 * @param value 整数
 * @param pStr 字符串缓冲区
 * @return uint32_t 字符串长度
 */
uint32_t UART_Uint32ToStr(uint32_t value, char *pStr);

/**
 * @brief 获取错误代码描述
 * @param errorCode 错误代码
 * @return const char* 错误描述字符串
 */
const char* UART_GetErrorDescription(uint32_t errorCode);

#ifdef __cplusplus
}
#endif

#endif /* _UART_PARSER_H_ */