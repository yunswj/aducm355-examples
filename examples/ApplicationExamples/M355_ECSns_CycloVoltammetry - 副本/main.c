/******************************************************************************
Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#include <stdio.h>
#include "ad5940.h"
#include "ADuCM355.h"
#include "ClkLib.h"
#include "UrtLib.h"
#include "DioLib.h"
#include "CV_Controller.h"
#include "UART_Parser.h"

void UartInit(void);
void ClockInit(void);
void ProcessUartCommands(void);

/* 全局变量 */
static char g_ResponseBuffer[512];
static char g_RxChar;
static BoolFlag g_CharReceived = bFALSE;

int main(void)
{
   void AD5940_Main(void);

   /* 系统初始化 */
   ClockInit();
   UartInit();
   AD5940_MCUResourceInit(0); /* Initialize all peripherals etc. used for AD5940/AFE. */

   /* 初始化串口控制系统 */
   CV_Init();
   UART_ParserInit();

   /* 设置默认参数 */
   CV_SetVoltageRange(-1.0f, 1.0f);          /* 电压范围: -1V 到 +1V */
   CV_SetCurrentRange(10);                   /* 电流档: RTIA_INT_10K */
   CV_SetScanRate(0.1f);                     /* 扫速: 0.1 V/s */
   CV_SetScanParams(200, 1000);              /* 步数: 200, 持续时间: 1000ms */
   CV_SetMeasurementMode(CV_MODE_FAST_SCAN); /* 快速扫描模式 */

   printf("AD5940 Cyclic Voltammetry with UART Control Ready!\r\n");

   /* 通过统一的INFO帧提示命令帮助，避免使用带占位校验和的纯文本 */
   UART_GenerateInfoResponse("Type $HELP for available commands", g_ResponseBuffer);
   printf("%s", g_ResponseBuffer);

   /* 主循环 */
   while (1)
   {
      /* 处理串口命令 */
      ProcessUartCommands();

      /* 原始AD5940主函数处理 */
      AD5940_Main();

      /* 短暂延时 */
      AD5940_Delay10us(100); /* 1ms延时 */
   }
}

void ClockInit(void)
{
   DigClkSel(DIGCLK_SOURCE_HFOSC);
   ClkDivCfg(1, 1); // HCLK = PCLK = 26MHz
}

// Initialize UART for 115200-8-N-1 (modified for better compatibility)
void UartInit(void)
{
   DioCfgPin(pADI_GPIO0, PIN10 | PIN11, 1); // Setup P0.10, P0.11 as UART pin
   UrtCfg(pADI_UART0, B115200,              // Changed to 115200 for better compatibility
          (BITM_UART_COMLCR_WLS | 3), 0);   // Configure UART for 115200 baud rate
   UrtFifoCfg(pADI_UART0, RX_FIFO_1BYTE,    // Configure the UART FIFOs for 8 bytes deep
              BITM_UART_COMFCR_FIFOEN);
   UrtFifoClr(pADI_UART0, BITM_UART_COMFCR_RFCLR // Clear the Rx/TX FIFOs
                              | BITM_UART_COMFCR_TFCLR);

   /* 启用UART接收中断 */
   UrtIntCfg(pADI_UART0, BITM_UART_COMIEN_ERBFI);
   NVIC_EnableIRQ(UART_EVT_IRQn);
}

/**
 * @brief 处理串口命令
 */
void ProcessUartCommands(void)
{
   /* 检查是否有字符接收 */
   if (g_CharReceived)
   {
      g_CharReceived = bFALSE;

      /* 处理接收到的字符 */
      UART_ProcessChar(g_RxChar);
   }

   /* 检查是否有完整命令需要处理 */
   if (UART_ProcessReadyCommand(g_ResponseBuffer))
   {
      /* 发送响应 */
      printf("%s", g_ResponseBuffer);
   }
}

/**
 * @brief UART中断处理函数
 */
void UART_Int_Handler(void)
{
   uint32_t iir = UrtIntSta(pADI_UART0);

   if ((iir & 0x1) == 0x1) /* 检查中断标志位 */
   {
      return; /* 无中断 */
   }

   /* 接收数据中断 */
   if ((iir & 0xE) == 0x4) /* 接收缓冲区有数据 */
   {
      g_RxChar = UrtRx(pADI_UART0);
      g_CharReceived = bTRUE;
   }
}
