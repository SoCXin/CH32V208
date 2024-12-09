/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/01/18
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "debug.h"
#include "eth_driver.h"
#include "IAP_Task.h"
/*
 *@Note
ETH IAP example, demonstrating the transmission of data via TCP, perform IAP.
This example uses the software as the 1_Tool_Doc folder under "VerifyBinTool_WCHNET",
The user manual is "WCHNET IAP Upgrade Solution Tutorial".

For details on the selection of engineering chips,
please refer to the "CH32V20x Evaluation Board Manual" under the CH32V20xEVT\EVT\PUB folder.
*/

u8 MACAddr[6];                                                      //MAC address
u8 IPAddr[4]   = {192,168,1,10};                                    //IP address
u8 GWIPAddr[4] = {192,168,1,1};                                     //Gateway IP address
u8 IPMask[4]   = {255,255,255,0};                                   //subnet mask
u8 DESIP[4]    = {192,168,1,100};                                   //destination IP address
u16 desport = 1000;                                                 //destination port
u16 srcport = 1000;                                                 //source port

u8 SocketId;
u8 SocketRecvBuf[WCHNET_MAX_SOCKET_NUM][RECE_BUF_LEN];              //socket receive buffer
u8 connFlag = 0;                                                    /* 0 disconnect 1 connect */

/*********************************************************************
 * @fn      mStopIfError
 *
 * @brief   check if error.
 *
 * @param   iError - error constants.
 *
 * @return  none
 */
void mStopIfError(u8 iError)
{
    if (iError == WCHNET_ERR_SUCCESS)
        return;
    printf("Error: %02X\r\n", (u16)iError);
}

/*********************************************************************
 * @fn      TIM2_Init
 *
 * @brief   Initializes TIM2.
 *
 * @return  none
 */
void TIM2_Init( void )
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure={0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = SystemCoreClock / 1000000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = WCHNETTIMERPERIOD * 1000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update ,ENABLE);

    TIM_Cmd(TIM2, ENABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update );
    NVIC_EnableIRQ(TIM2_IRQn);
}

/*********************************************************************
 * @fn      WCHNET_CreateTcpSocket
 *
 * @brief   Create TCP Socket
 *
 * @return  none
 */
void WCHNET_CreateTcpSocket(void)
{
   u8 i;
   SOCK_INF TmpSocketInf;

   memset((void *)&TmpSocketInf,0,sizeof(SOCK_INF));
   memcpy((void *)TmpSocketInf.IPAddr,DESIP,4);
   TmpSocketInf.DesPort  = desport;
   TmpSocketInf.SourPort = srcport;
   TmpSocketInf.ProtoType = PROTO_TYPE_TCP;
   TmpSocketInf.RecvBufLen = RECE_BUF_LEN;
   i = WCHNET_SocketCreat(&SocketId,&TmpSocketInf);
   printf("WCHNET_SocketCreat %d\r\n",SocketId);
   mStopIfError(i);
   i = WCHNET_SocketConnect(SocketId);
   mStopIfError(i);
}

/*********************************************************************
 * @fn      WCHNET_HandleSockInt
 *
 * @brief   Socket Interrupt Handle
 *
 * @param   socketid - socket id.
 *          intstat - interrupt status
 *
 * @return  none
 */
void WCHNET_HandleSockInt(u8 socketid,u8 intstat)
{
    if(intstat & SINT_STAT_RECV)                                                //receive data
    {
        if (fileDataLen < BIN_INF_LEN) {
            IAPFileParaCheck(socketid);
        }
        else{
            receUpdatedFile(socketid);
        }
    }
    if(intstat & SINT_STAT_CONNECT)                                             //connect successfully
    {
        WCHNET_ModifyRecvBuf(socketid, (u32)SocketRecvBuf[socketid], RECE_BUF_LEN);
        connFlag = 1;
        IAP_EEPROM_ERASE_44k(BACKUP_IMAGE_START_ADD);
        printf("TCP Connect Success\r\n");
    }
    if(intstat & SINT_STAT_DISCONNECT)                                          //disconnect
    {
        connFlag = 0;
        IAPParaInit();
        printf("TCP Disconnect\r\n");
    }
    if(intstat & SINT_STAT_TIM_OUT)                                             //timeout disconnect
    {
        printf("TCP Timeout\r\n");
        connFlag = 0;
        IAPParaInit();
        WCHNET_CreateTcpSocket();
    }
}


/*********************************************************************
 * @fn      WCHNET_HandleGlobalInt
 *
 * @brief   Global Interrupt Handle
 *
 * @return  none
 */
void WCHNET_HandleGlobalInt(void)
{
    u8 intstat;
    u16 i;
    u8 socketint;

    intstat = WCHNET_GetGlobalInt();                                             //get global interrupt flag
    if(intstat & GINT_STAT_UNREACH)                                              //Unreachable interrupt
    {
       printf("GINT_STAT_UNREACH\r\n");
    }
   if(intstat & GINT_STAT_IP_CONFLI)                                             //IP conflict
   {
       printf("GINT_STAT_IP_CONFLI\r\n");
   }
   if(intstat & GINT_STAT_PHY_CHANGE)                                            //PHY status change
   {
       i = WCHNET_GetPHYStatus();
       if(i&PHY_Linked_Status)
       printf("PHY Link Success\r\n");
   }
   if(intstat & GINT_STAT_SOCKET)                                                //socket related interrupt
   {
       for(i = 0; i < WCHNET_MAX_SOCKET_NUM; i++)
       {
           socketint = WCHNET_GetSocketInt(i);
           if(socketint)WCHNET_HandleSockInt(i,socketint);
       }
   }
}

/*********************************************************************
 * @fn      GPIOInit
 *
 * @brief   GPIO configuration
 *
 * @return  none
 */
void GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program
 *
 * @return  none
 */
int main(void)
{
    u8 i;
    u32 updateFlag;

    Delay_Init();
    USART_Printf_Init(115200);                                                      //USART initialize
    GPIOInit();
    /*Detect whether the button(PA0) is pressed, if pressed,
     *make a TCP connection to upgrade, otherwise jump
     *to the application.*/
    if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0){
        Delay_Ms(50);
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0){
            printf("IAP test\r\n");
            printf("SystemClk:%d\r\n",SystemCoreClock);
            printf("net version:%x\n",WCHNET_GetVer());
            if( WCHNET_LIB_VER != WCHNET_GetVer() ){
                printf("version error.\n");
            }
            WCHNET_GetMacAddr(MACAddr);                                             //get the chip MAC address
            printf("mac addr:");
            for(i = 0; i < 6; i++) 
                printf("%x ",MACAddr[i]);
            printf("\n");
            TIM2_Init();
            i = ETH_LibInit(IPAddr,GWIPAddr,IPMask,MACAddr);                        //Ethernet library initialize
            mStopIfError(i);
            if(i == WCHNET_ERR_SUCCESS) printf("WCHNET_LibInit Success\r\n");
            WCHNET_CreateTcpSocket();                                               //Create TCP Socket

            while(1)
            {
                /*Ethernet library main task function,
                 * which needs to be called cyclically*/
                WCHNET_MainTask();
                /*Query the Ethernet global interrupt,
                 * if there is an interrupt, call the global interrupt handler*/
                if(WCHNET_QueryGlobalInt())
                {
                    WCHNET_HandleGlobalInt();
                }
                if(connFlag){
                    saveUpdatedFile();
                }
            }
        }
    }
    IAP_EEPROM_READ(UPDATA_FLAG_STORAGE_ADD,(u8 *)&updateFlag,4);
    if(updateFlag == IMAGE_FLAG_UPDATE){
        printf("start IAP program!\r\n");
        if(IAPCopyFlashDeal() == READY){
            printf("update success!\r\n");
            printf("Run APP!\r\n");
            NVIC_EnableIRQ(Software_IRQn);
            NVIC_SetPendingIRQ(Software_IRQn);
        }
        else{
            printf("update error!\r\n");
            while(1);
        }
    }
    else{
        printf("Run APP!\r\n");
        NVIC_EnableIRQ(Software_IRQn);
        NVIC_SetPendingIRQ(Software_IRQn);
    }
}
