/**
 * @file UART_Parser.c
 * @brief 串口命令解析器实现文件
 * @details 实现循环伏安法参数控制的串口命令解析和验证功能
 * @author ADI Application Team
 * @date 2024
 */

#include "UART_Parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 全局变量定义 */
UART_RxBuffer_Type g_UartRxBuffer = {0};

/* 命令表定义 */
static const UART_CmdEntry_Type g_CmdTable[] = {
    {"SVR",   UART_CMD_SVR,    2, 2, UART_HandleSetVoltageRange,  "Set Voltage Range: $SVR,start,peak*XX"},
    {"SCA",   UART_CMD_SCA,    1, 1, UART_HandleSetCurrentRange,  "Set Current Range: $SCA,rtia_index*XX"},
    {"SSR",   UART_CMD_SSR,    1, 1, UART_HandleSetScanRate,      "Set Scan Rate: $SSR,rate*XX"},
    {"SSP",   UART_CMD_SSP,    2, 2, UART_HandleSetScanParams,    "Set Scan Params: $SSP,steps,duration*XX"},
    {"SMD",   UART_CMD_SMD,    1, 1, UART_HandleSetMeasMode,      "Set Meas Mode: $SMD,mode*XX"},
    {"START", UART_CMD_START,  0, 0, UART_HandleStartMeas,        "Start Measurement: $START*XX"},
    {"STOP",  UART_CMD_STOP,   0, 0, UART_HandleStopMeas,         "Stop Measurement: $STOP*XX"},
    {"PAUSE", UART_CMD_PAUSE,  0, 0, UART_HandlePauseMeas,        "Pause Measurement: $PAUSE*XX"},
    {"RESUME",UART_CMD_RESUME, 0, 0, UART_HandleResumeMeas,       "Resume Measurement: $RESUME*XX"},
    {"QVR",   UART_CMD_QVR,    0, 0, UART_HandleQueryVoltageRange,"Query Voltage Range: $QVR*XX"},
    {"QCA",   UART_CMD_QCA,    0, 0, UART_HandleQueryCurrentRange,"Query Current Range: $QCA*XX"},
    {"QSR",   UART_CMD_QSR,    0, 0, UART_HandleQueryScanRate,    "Query Scan Rate: $QSR*XX"},
    {"QSP",   UART_CMD_QSP,    0, 0, UART_HandleQueryScanParams,  "Query Scan Params: $QSP*XX"},
    {"QST",   UART_CMD_QST,    0, 0, UART_HandleQueryStatus,      "Query Status: $QST*XX"},
    {"QALL",  UART_CMD_QALL,   0, 0, UART_HandleQueryAll,         "Query All Params: $QALL*XX"},
    {"RESET", UART_CMD_RESET,  0, 0, UART_HandleReset,            "Reset System: $RESET*XX"},
    {"HELP",  UART_CMD_HELP,   0, 0, UART_HandleHelp,             "Show Help: $HELP*XX"}
};

/* 错误描述字符串表 */
static const char* g_ParseErrorStrings[] = {
    "OK",
    "Format Error",
    "Checksum Error", 
    "Unknown Command",
    "Parameter Count Error",
    "Parameter Value Error",
    "Buffer Full"
};

/* 状态字符串表 */
static const char* g_StateStrings[] = {
    "IDLE",
    "CONFIGURED", 
    "MEASURING",
    "PAUSED",
    "ERROR"
};

/* 测量模式字符串表 */
static const char* g_ModeStrings[] = {
    "FAST_SCAN",
    "HIGH_PRECISION",
    "LOW_CURRENT", 
    "CUSTOM"
};

/**
 * @brief 初始化串口解析器
 */
AD5940Err UART_ParserInit(void)
{
    memset(&g_UartRxBuffer, 0, sizeof(UART_RxBuffer_Type));
    return AD5940ERR_OK;
}

/**
 * @brief 反初始化串口解析器
 */
AD5940Err UART_ParserDeInit(void)
{
    memset(&g_UartRxBuffer, 0, sizeof(UART_RxBuffer_Type));
    return AD5940ERR_OK;
}

/**
 * @brief 处理接收到的字符
 */
AD5940Err UART_ProcessChar(char ch)
{
    /* 检查缓冲区是否已满 */
    if (g_UartRxBuffer.index >= UART_CMD_MAX_LENGTH - 1) {
        /* 缓冲区满，重置 */
        g_UartRxBuffer.index = 0;
        g_UartRxBuffer.cmdReady = bFALSE;
        return AD5940ERR_BUFF;
    }
    
    /* 检查命令起始字符 */
    if (g_UartRxBuffer.index == 0 && ch != UART_CMD_START_CHAR) {
        return AD5940ERR_OK; /* 忽略非起始字符 */
    }
    
    /* 存储字符 */
    g_UartRxBuffer.buffer[g_UartRxBuffer.index++] = ch;
    
    /* 检查命令结束 */
    if (ch == UART_CMD_END_CHAR2 && g_UartRxBuffer.index > 1 && 
        g_UartRxBuffer.buffer[g_UartRxBuffer.index - 2] == UART_CMD_END_CHAR1) {
        
        /* 命令接收完成 */
        g_UartRxBuffer.buffer[g_UartRxBuffer.index] = '\0';
        g_UartRxBuffer.cmdReady = bTRUE;
    }
    
    return AD5940ERR_OK;
}

/**
 * @brief 解析命令字符串
 */
uint32_t UART_ParseCommand(const char *pCmdStr, UART_Command_Type *pCmd)
{
    if (pCmdStr == NULL || pCmd == NULL) {
        return UART_PARSE_ERR_FORMAT;
    }
    
    /* 清空命令结构体 */
    memset(pCmd, 0, sizeof(UART_Command_Type));
    
    /* 检查命令格式 */
    if (pCmdStr[0] != UART_CMD_START_CHAR) {
        return UART_PARSE_ERR_FORMAT;
    }
    
    /* 复制命令字符串用于解析 */
    char cmdBuffer[UART_CMD_MAX_LENGTH];
    strncpy(cmdBuffer, pCmdStr + 1, sizeof(cmdBuffer) - 1); /* 跳过起始字符 */
    cmdBuffer[sizeof(cmdBuffer) - 1] = '\0';
    
    /* 移除结束字符 */
    char *endPtr = strstr(cmdBuffer, "\r\n");
    if (endPtr) {
        *endPtr = '\0';
    }
    
    /* 检查是否有校验和 */
    char *checksumPtr = strchr(cmdBuffer, UART_CMD_CHECKSUM_CHAR);
    if (checksumPtr) {
        *checksumPtr = '\0';
        checksumPtr++;
        
        /* 解析校验和 */
        char *endptr;
        unsigned long checksum = strtoul(checksumPtr, &endptr, 16);
        if (*endptr == '\0' && checksum <= 0xFF) {
            pCmd->checksum = (uint8_t)checksum;
            pCmd->hasChecksum = bTRUE;
        } else {
            return UART_PARSE_ERR_CHECKSUM;
        }
    }
    
    /* 解析命令名称和参数 */
    char *token = strtok(cmdBuffer, ",");
    if (token == NULL) {
        return UART_PARSE_ERR_FORMAT;
    }
    
    /* 复制命令名称 */
    strncpy(pCmd->name, token, sizeof(pCmd->name) - 1);
    pCmd->name[sizeof(pCmd->name) - 1] = '\0';
    
    /* 解析参数 */
    pCmd->paramCount = 0;
    while ((token = strtok(NULL, ",")) != NULL && pCmd->paramCount < UART_CMD_MAX_PARAMS) {
        strncpy(pCmd->params[pCmd->paramCount], token, sizeof(pCmd->params[0]) - 1);
        pCmd->params[pCmd->paramCount][sizeof(pCmd->params[0]) - 1] = '\0';
        pCmd->paramCount++;
    }
    
    /* 验证校验和 */
    if (pCmd->hasChecksum && !UART_VerifyChecksum(pCmd, pCmdStr)) {
        return UART_PARSE_ERR_CHECKSUM;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 执行命令
 */
uint32_t UART_ExecuteCommand(const UART_Command_Type *pCmd, char *pResponse)
{
    if (pCmd == NULL || pResponse == NULL) {
        return UART_PARSE_ERR_FORMAT;
    }
    
    /* 查找命令处理函数 */
    const UART_CmdEntry_Type *pEntry = NULL;
    for (uint32_t i = 0; i < sizeof(g_CmdTable) / sizeof(g_CmdTable[0]); i++) {
        if (strcmp(pCmd->name, g_CmdTable[i].cmdName) == 0) {
            pEntry = &g_CmdTable[i];
            break;
        }
    }
    
    if (pEntry == NULL) {
        return UART_PARSE_ERR_CMD_UNKNOWN;
    }
    
    /* 检查参数个数 */
    if (pCmd->paramCount < pEntry->minParams || pCmd->paramCount > pEntry->maxParams) {
        return UART_PARSE_ERR_PARAM_COUNT;
    }
    
    /* 调用处理函数 */
    return pEntry->handler(pCmd, pResponse);
}

/**
 * @brief 处理就绪的命令
 */
BoolFlag UART_ProcessReadyCommand(char *pResponse)
{
    if (!g_UartRxBuffer.cmdReady || pResponse == NULL) {
        return bFALSE;
    }
    
    UART_Command_Type cmd;
    uint32_t parseResult = UART_ParseCommand(g_UartRxBuffer.buffer, &cmd);
    
    if (parseResult == UART_PARSE_OK) {
        /* 执行命令 */
        uint32_t execResult = UART_ExecuteCommand(&cmd, pResponse);
        if (execResult != UART_PARSE_OK) {
            /* 生成错误响应 */
            UART_GenerateNakResponse(cmd.name, execResult, pResponse);
        }
    } else {
        /* 解析错误 */
        UART_GenerateNakResponse("PARSE", parseResult, pResponse);
    }
    
    /* 重置接收缓冲区 */
    g_UartRxBuffer.index = 0;
    g_UartRxBuffer.cmdReady = bFALSE;
    
    return bTRUE;
}

/**
 * @brief 计算XOR校验和
 */
uint8_t UART_CalculateChecksum(const char *pStr, uint32_t length)
{
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < length; i++) {
        checksum ^= (uint8_t)pStr[i];
    }
    return checksum;
}

/**
 * @brief 验证命令校验和
 */
BoolFlag UART_VerifyChecksum(const UART_Command_Type *pCmd, const char *pCmdStr)
{
    if (!pCmd->hasChecksum) {
        return bTRUE; /* 没有校验和，认为正确 */
    }
    
    /* 找到校验和分隔符位置 */
    const char *checksumPtr = strchr(pCmdStr, UART_CMD_CHECKSUM_CHAR);
    if (checksumPtr == NULL) {
        return bFALSE;
    }
    
    /* 计算校验和 */
    uint32_t length = checksumPtr - pCmdStr;
    uint8_t calculated = UART_CalculateChecksum(pCmdStr, length);
    
    return (calculated == pCmd->checksum) ? bTRUE : bFALSE;
}

/**
 * @brief 生成ACK响应
 */
uint32_t UART_GenerateAckResponse(const char *pCmd, const char *pData, char *pResponse)
{
    uint32_t length;
    if (pData && strlen(pData) > 0) {
        length = sprintf(pResponse, "$ACK,%s,%s", pCmd, pData);
    } else {
        length = sprintf(pResponse, "$ACK,%s", pCmd);
    }
    
    /* 添加校验和 */
    uint8_t checksum = UART_CalculateChecksum(pResponse, length);
    length += sprintf(pResponse + length, "*%02X\r\n", checksum);
    
    return length;
}

/**
 * @brief 生成NAK响应
 */
uint32_t UART_GenerateNakResponse(const char *pCmd, uint32_t errorCode, char *pResponse)
{
    uint32_t length = sprintf(pResponse, "$NAK,%s,E%02X", pCmd, errorCode);
    
    /* 添加校验和 */
    uint8_t checksum = UART_CalculateChecksum(pResponse, length);
    length += sprintf(pResponse + length, "*%02X\r\n", checksum);
    
    return length;
}

/**
 * @brief 生成数据响应
 */
uint32_t UART_GenerateDataResponse(float voltage, float current, uint32_t timestamp, char *pResponse)
{
    uint32_t length = sprintf(pResponse, "$DATA,%.3f,%.6f,%u", voltage, current, timestamp);
    
    /* 添加校验和 */
    uint8_t checksum = UART_CalculateChecksum(pResponse, length);
    length += sprintf(pResponse + length, "*%02X\r\n", checksum);
    
    return length;
}

/**
 * @brief 生成信息响应
 */
uint32_t UART_GenerateInfoResponse(const char *pInfo, char *pResponse)
{
    uint32_t length = sprintf(pResponse, "$INFO,%s", pInfo);
    
    /* 添加校验和 */
    uint8_t checksum = UART_CalculateChecksum(pResponse, length);
    length += sprintf(pResponse + length, "*%02X\r\n", checksum);
    
    return length;
}

/* 命令处理函数实现 */

/**
 * @brief 处理设置电压范围命令
 */
uint32_t UART_HandleSetVoltageRange(const UART_Command_Type *pCmd, char *pResponse)
{
    float startVolt, peakVolt;
    
    /* 解析参数 */
    if (!UART_StrToFloat(pCmd->params[0], &startVolt) || 
        !UART_StrToFloat(pCmd->params[1], &peakVolt)) {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    /* 设置电压范围 */
    AD5940Err result = CV_SetVoltageRange(startVolt, peakVolt);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%.1f,%.1f", startVolt, peakVolt);
        UART_GenerateAckResponse("SVR", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理设置电流档命令
 */
uint32_t UART_HandleSetCurrentRange(const UART_Command_Type *pCmd, char *pResponse)
{
    uint32_t rtiaIndex;
    
    /* 解析参数 */
    if (!UART_StrToUint32(pCmd->params[0], &rtiaIndex)) {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    /* 设置电流档 */
    AD5940Err result = CV_SetCurrentRange(rtiaIndex);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%u", rtiaIndex);
        UART_GenerateAckResponse("SCA", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理设置扫速命令
 */
uint32_t UART_HandleSetScanRate(const UART_Command_Type *pCmd, char *pResponse)
{
    float scanRate;
    
    /* 解析参数 */
    if (!UART_StrToFloat(pCmd->params[0], &scanRate)) {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    /* 设置扫速 */
    AD5940Err result = CV_SetScanRate(scanRate);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%.3f", scanRate);
        UART_GenerateAckResponse("SSR", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理设置扫描参数命令
 */
uint32_t UART_HandleSetScanParams(const UART_Command_Type *pCmd, char *pResponse)
{
    uint32_t stepNum, duration;
    
    /* 解析参数 */
    if (!UART_StrToUint32(pCmd->params[0], &stepNum) || 
        !UART_StrToUint32(pCmd->params[1], &duration)) {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    /* 设置扫描参数 */
    AD5940Err result = CV_SetScanParams(stepNum, duration);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%u,%u", stepNum, duration);
        UART_GenerateAckResponse("SSP", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理设置测量模式命令
 */
uint32_t UART_HandleSetMeasMode(const UART_Command_Type *pCmd, char *pResponse)
{
    uint32_t mode;
    
    /* 解析参数 */
    if (!UART_StrToUint32(pCmd->params[0], &mode)) {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    /* 设置测量模式 */
    AD5940Err result = CV_SetMeasurementMode((CV_MeasMode_Type)mode);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%s", g_ModeStrings[mode]);
        UART_GenerateAckResponse("SMD", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理开始测量命令
 */
uint32_t UART_HandleStartMeas(const UART_Command_Type *pCmd, char *pResponse)
{
    AD5940Err result = CV_StartMeasurement();
    if (result == AD5940ERR_OK) {
        UART_GenerateAckResponse("START", "OK", pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理停止测量命令
 */
uint32_t UART_HandleStopMeas(const UART_Command_Type *pCmd, char *pResponse)
{
    AD5940Err result = CV_StopMeasurement();
    if (result == AD5940ERR_OK) {
        UART_GenerateAckResponse("STOP", "OK", pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理暂停测量命令
 */
uint32_t UART_HandlePauseMeas(const UART_Command_Type *pCmd, char *pResponse)
{
    AD5940Err result = CV_PauseMeasurement();
    if (result == AD5940ERR_OK) {
        UART_GenerateAckResponse("PAUSE", "OK", pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理恢复测量命令
 */
uint32_t UART_HandleResumeMeas(const UART_Command_Type *pCmd, char *pResponse)
{
    AD5940Err result = CV_ResumeMeasurement();
    if (result == AD5940ERR_OK) {
        UART_GenerateAckResponse("RESUME", "OK", pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询电压范围命令
 */
uint32_t UART_HandleQueryVoltageRange(const UART_Command_Type *pCmd, char *pResponse)
{
    float startVolt, peakVolt;
    
    AD5940Err result = CV_GetVoltageRange(&startVolt, &peakVolt);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%.1f,%.1f", startVolt, peakVolt);
        UART_GenerateAckResponse("QVR", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询电流档命令
 */
uint32_t UART_HandleQueryCurrentRange(const UART_Command_Type *pCmd, char *pResponse)
{
    uint32_t rtiaIndex;
    
    AD5940Err result = CV_GetCurrentRange(&rtiaIndex);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%u", rtiaIndex);
        UART_GenerateAckResponse("QCA", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询扫速命令
 */
uint32_t UART_HandleQueryScanRate(const UART_Command_Type *pCmd, char *pResponse)
{
    float scanRate;
    
    AD5940Err result = CV_GetScanRate(&scanRate);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%.3f", scanRate);
        UART_GenerateAckResponse("QSR", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询扫描参数命令
 */
uint32_t UART_HandleQueryScanParams(const UART_Command_Type *pCmd, char *pResponse)
{
    uint32_t stepNum, duration;
    
    AD5940Err result = CV_GetScanParams(&stepNum, &duration);
    if (result == AD5940ERR_OK) {
        char dataStr[64];
        sprintf(dataStr, "%u,%u", stepNum, duration);
        UART_GenerateAckResponse("QSP", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询状态命令
 */
uint32_t UART_HandleQueryStatus(const UART_Command_Type *pCmd, char *pResponse)
{
    CV_State_Type state = CV_GetMeasurementState();
    UART_GenerateAckResponse("QST", g_StateStrings[state], pResponse);
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理查询所有参数命令
 */
uint32_t UART_HandleQueryAll(const UART_Command_Type *pCmd, char *pResponse)
{
    CV_Params_Type params;
    
    AD5940Err result = CV_GetAllParams(&params);
    if (result == AD5940ERR_OK) {
        char dataStr[128];
        sprintf(dataStr, "VR:%.1f,%.1f;CA:%u;SR:%.3f;SP:%u,%u;ST:%s", 
                params.startVolt, params.peakVolt, params.rtiaIndex,
                params.scanRate, params.stepNumber, params.duration,
                g_StateStrings[params.state]);
        UART_GenerateAckResponse("QALL", dataStr, pResponse);
    } else {
        return UART_PARSE_ERR_PARAM_VALUE;
    }
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理重置命令
 */
uint32_t UART_HandleReset(const UART_Command_Type *pCmd, char *pResponse)
{
    CV_DeInit();
    CV_Init();
    UART_GenerateAckResponse("RESET", "OK", pResponse);
    
    return UART_PARSE_OK;
}

/**
 * @brief 处理帮助命令
 */
uint32_t UART_HandleHelp(const UART_Command_Type *pCmd, char *pResponse)
{
    /* 生成帮助信息 */
    strcpy(pResponse, "$INFO,Available Commands:\r\n");
    for (uint32_t i = 0; i < sizeof(g_CmdTable) / sizeof(g_CmdTable[0]); i++) {
        strcat(pResponse, "$INFO,");
        strcat(pResponse, g_CmdTable[i].description);
        strcat(pResponse, "\r\n");
    }
    
    return UART_PARSE_OK;
}

/* 工具函数实现 */

/**
 * @brief 字符串转浮点数
 */
BoolFlag UART_StrToFloat(const char *pStr, float *pValue)
{
    if (pStr == NULL || pValue == NULL) return bFALSE;
    
    char *endptr;
    *pValue = strtof(pStr, &endptr);
    
    return (*endptr == '\0') ? bTRUE : bFALSE;
}

/**
 * @brief 字符串转整数
 */
BoolFlag UART_StrToUint32(const char *pStr, uint32_t *pValue)
{
    if (pStr == NULL || pValue == NULL) return bFALSE;
    
    char *endptr;
    unsigned long temp = strtoul(pStr, &endptr, 10);
    
    if (*endptr != '\0' || temp > UINT32_MAX) return bFALSE;
    
    *pValue = (uint32_t)temp;
    return bTRUE;
}

/**
 * @brief 浮点数转字符串
 */
uint32_t UART_FloatToStr(float value, uint32_t precision, char *pStr)
{
    if (pStr == NULL) return 0;
    
    return sprintf(pStr, "%.*f", precision, value);
}

/**
 * @brief 整数转字符串
 */
uint32_t UART_Uint32ToStr(uint32_t value, char *pStr)
{
    if (pStr == NULL) return 0;
    
    return sprintf(pStr, "%u", value);
}

/**
 * @brief 获取错误代码描述
 */
const char* UART_GetErrorDescription(uint32_t errorCode)
{
    if (errorCode < sizeof(g_ParseErrorStrings) / sizeof(g_ParseErrorStrings[0])) {
        return g_ParseErrorStrings[errorCode];
    }
    return "Unknown Error";
}