/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2024/02/21
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 * The code initializes the power voltage detector (PVD) and prints the voltage level.
 */
#include "debug.h"

/* Global define */

/* Global Variable */
volatile uint8_t Voltage_5V_Flag=0;

void PVD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void PVD_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line16) != RESET)
    {
        Voltage_5V_Flag = !PWR_GetFlagStatus(PWR_FLAG_PVDO);
        printf("Run at EXTI\r\n");
        EXTI_ClearITPendingBit(EXTI_Line16);
    }
}

/*********************************************************************
 * @fn       PVD_Init
 *
 * @brief    The function initializes the power voltage detector (PVD) by configuring the necessary registers and
 *          enabling the PVD interrupt.
 *
 * @return   none
 */

void PVD_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    EXTI_InitTypeDef EXIT_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    EXIT_InitStructure.EXTI_Line = EXTI_Line16;
    EXIT_InitStructure.EXTI_LineCmd = ENABLE;
    EXIT_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXIT_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_Init(&EXIT_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = PVD_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    PWR_PVDLevelConfig(PWR_PVDLevel_MODE7);

    PWR_PVDCmd(ENABLE);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

    printf("Voltage judger TEST\r\n");

    PVD_Init();
    Voltage_5V_Flag = !PWR_GetFlagStatus(PWR_FLAG_PVDO);
    while (1)
    {
        Delay_Ms(250);
        Voltage_5V_Flag?printf("Voltage is higher than 3.3v\r\n"):printf("Voltage is lower than 3.3v\r\n",Voltage_5V_Flag?5:3);
    }
}
