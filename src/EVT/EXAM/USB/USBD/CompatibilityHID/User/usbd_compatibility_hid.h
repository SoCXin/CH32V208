/********************************** (C) COPYRIGHT *******************************
 * File Name  :usbd_custom_hid.h
 * Author     :OWNER
 * Version    : v0.01
 * Date       : 2022��7��8��
 * Description:
*******************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#ifndef USER_USBD_CUSTOM_HID_H_
#define USER_USBD_CUSTOM_HID_H_

#include "debug.h"
#include "string.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_prop.h"


#define DEF_UART2_BUF_SIZE            2048
#define DEF_UART2_TOUT_TIME           30             // NOTE: the timeout time should be set according to the actual baud rate.

#define SET_REPORT_DEAL_OVER          0x00
#define SET_REPORT_WAIT_DEAL          0x01


extern uint8_t  HID_Report_Buffer[64];               // HID Report Buffer

extern void UART2_Tx_Service( void );
extern void UART2_Rx_Service( void );
extern void UART2_Init( void );
extern void UART2_DMA_Init( void );
extern void TIM2_Init( void );

#endif /* USER_USBD_CUSTOM_HID_H_ */
