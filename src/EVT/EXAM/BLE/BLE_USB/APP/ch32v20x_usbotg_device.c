/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v30x_usbotg_device.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/06/06
 * Description        : This file provides all the USBOTG firmware functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "ch32v20x_usbotg_device.h"
#include "CONFIG.h"
#include "peripheral.h"
#include "RingMem.h"

/* Global define */
/* OTH */
#define pMySetupReqPakHD    ((PUSB_SETUP_REQ)EP0_DatabufHD)
#define RepDescSize         62

#if DEF_USBD_SPEED == DEF_USBD_SPEED_LOW
  #define RepDataLoadLen    8
#else
  #define RepDataLoadLen    64
#endif

/******************************************************************************/
/* Endpoint Buffer */
__attribute__((aligned(4))) UINT8 EP0_DatabufHD[64];      //ep0(64)
__attribute__((aligned(4))) UINT8 EP1_DatabufHD[64 + 64]; //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4))) UINT8 EP2_DatabufHD[64 + 64]; //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4))) UINT8 EP3_DatabufHD[64 + 64]; //ep3_out(64)+ep3_in(64)
__attribute__((aligned(4))) UINT8 EP4_DatabufHD[64 + 64]; //ep4_out(64)+ep4_in(64)
__attribute__((aligned(4))) UINT8 EP5_DatabufHD[64 + 64]; //ep5_out(64)+ep5_in(64)
__attribute__((aligned(4))) UINT8 EP6_DatabufHD[64 + 64]; //ep6_out(64)+ep6_in(64)
__attribute__((aligned(4))) UINT8 EP7_DatabufHD[64 + 64]; //ep7_out(64)+ep7_in(64)

PUINT8 pEP0_RAM_Addr; //ep0(64)
PUINT8 pEP1_RAM_Addr; //ep1_out(64)+ep1_in(64)
PUINT8 pEP2_RAM_Addr; //ep2_out(64)+ep2_in(64)
PUINT8 pEP3_RAM_Addr; //ep3_out(64)+ep3_in(64)
PUINT8 pEP4_RAM_Addr; //ep4_out(64)+ep4_in(64)
PUINT8 pEP5_RAM_Addr; //ep5_out(64)+ep5_in(64)
PUINT8 pEP6_RAM_Addr; //ep6_out(64)+ep6_in(64)
PUINT8 pEP7_RAM_Addr; //ep7_out(64)+ep7_in(64)

const UINT8    *pDescr;
volatile UINT8  USBHD_Dev_SetupReqCode = 0xFF;   /* USB2.0 full-speed device setup package command */
volatile UINT16 USBHD_Dev_SetupReqLen = 0x00;    /* USB2.0 full-speed device setup package length */
volatile UINT8  USBHD_Dev_SetupReqValueH = 0x00; /* USB2.0 full-speed device setup package value high byte */
volatile UINT8  USBHD_Dev_Config = 0x00;         /* USB2.0 full-speed device configuration value */
volatile UINT8  USBHD_Dev_Address = 0x00;        /* USB2.0 full-speed device address value */
volatile UINT8  USBHD_Dev_SleepStatus = 0x00;    /* USB2.0 full-speed device sleep state */
volatile UINT8  USBHD_Dev_EnumStatus = 0x00;     /* USB2.0 full-speed device enumeration status */
volatile UINT8  USBHD_Dev_Endp0_Tog = 0x01;      /* USB2.0 Full-speed device endpoint 0 sync flag*/
volatile UINT8  USBHD_Dev_Speed = 0x01;          /* USB2.0 Full-speed device speed */

volatile UINT16 USBHD_Endp1_Up_Flag = 0x00;   /* USB2.0 full-speed device endpoint 1 data upload status: 0: free; 1: upload; */
volatile UINT8  USBHD_Endp1_Down_Flag = 0x00; /* USB2.0 full-speed device endpoint 1 Pass successful logo */
volatile UINT8  USBHD_Endp1_Down_Len = 0x00;  /* USB2.0 full-speed device endpoint 1 download length */
volatile BOOL   USBHD_Endp1_T_Tog = 0;        /* USB2.0 full speed device endpoint 1 sends tog bit flip */
volatile BOOL   USBHD_Endp1_R_Tog = 0;

volatile UINT16 USBHD_Endp2_Up_Flag = 0x00;    /* USB2.0 full-speed device endpoint 2 Data upload status: 0: free; 1: upload; */
volatile UINT16 USBHD_Endp2_Up_LoadPtr = 0x00; /* USB2.0 full-speed device endpoint 2 Data upload loading offset */
volatile UINT8  USBHD_Endp2_Down_Flag = 0x00;  /* USB2.0 full-speed device endpoint 2 download success flag */
volatile BOOL   USBHD_Endp2_T_Tog = 0;         /* USB2.0 full speed device endpoint 2 sends tog bit flip */
volatile BOOL   USBHD_Endp2_R_Tog = 0;

volatile UINT32V Endp2_send_seq = 0x00;
volatile UINT8   DevConfig;
volatile UINT8   SetupReqCode;
volatile UINT16  SetupReqLen;

/******************************************************************************/
/* Device Descriptor */
const UINT8 MyDevDescrHD[] = {

    0x12, 0x01, 0x10, 0x01, 0xFF, 0x00, 0x00, DEF_USBD_UEP0_SIZE,
    0x86, 0x1A, 0x23, 0x75, 0x63, 0x02, 0x00, 0x02,
    0x00, 0x01};

/* Configration Descriptor */
const UINT8 MyCfgDescrHD[] =
    {
        0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0x80, 0xf0, //Configure descriptor, interface descriptor, endpoint descriptor
        0x09, 0x04, 0x00, 0x00, 0x03, 0xff, 0x01, 0x02, 0x00,
        0x07, 0x05, 0x82, 0x02, 0x20, 0x00, 0x00, //Bulk upload endpoint
        0x07, 0x05, 0x02, 0x02, 0x20, 0x00, 0x00, //Bulk download endpoint
        0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x01};

/* Language Descriptor */
const UINT8 MyLangDescrHD[] =
    {
        0x04, 0x03, 0x09, 0x04};

/* Manufactor Descriptor */
const UINT8 MyManuInfoHD[] =
    {
        0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0};

/* Product Information */
const UINT8 MyProdInfoHD[] =
    {
        0x0C, 0x03, 'C', 0, 'H', 0, '1', 0, '0', 0, 'x', 0};

/* Product descriptor */
const uint8_t StrDesc[28] =
    {
        0x1C, 0x03, 0x55, 0x00, 0x53, 0x00, 0x42, 0x00,
        0x32, 0x00, 0x2E, 0x00, 0x30, 0x00, 0x2D, 0x00,
        0x53, 0x00, 0x65, 0x00, 0x72, 0x00, 0x69, 0x00,
        0x61, 0x00, 0x6C, 0x00};

const uint8_t Return1[2] = {0x31, 0x00};
const uint8_t Return2[2] = {0xC3, 0x00};
const uint8_t Return3[2] = {0x9F, 0xEE};

void USBHD_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      USBOTG_FS_DeviceInit
 *
 * @brief   Initializes USB device.
 *
 * @return  none
 */
void USBDeviceInit(void)
{
    USBOTG_FS->BASE_CTRL = 0x00;

    USBOTG_FS->UEP4_1_MOD = USBHD_UEP4_RX_EN | USBHD_UEP4_TX_EN | USBHD_UEP1_RX_EN | USBHD_UEP1_TX_EN;
    USBOTG_FS->UEP2_3_MOD = USBHD_UEP2_RX_EN | USBHD_UEP2_TX_EN | USBHD_UEP3_RX_EN | USBHD_UEP3_TX_EN;
    USBOTG_FS->UEP5_6_MOD = USBHD_UEP5_RX_EN | USBHD_UEP5_TX_EN | USBHD_UEP6_RX_EN | USBHD_UEP6_TX_EN;
    USBOTG_FS->UEP7_MOD = USBHD_UEP7_RX_EN | USBHD_UEP7_TX_EN;

    USBOTG_FS->UEP0_DMA = (UINT32)pEP0_RAM_Addr;
    USBOTG_FS->UEP1_DMA = (UINT32)pEP1_RAM_Addr;
    USBOTG_FS->UEP2_DMA = (UINT32)pEP2_RAM_Addr;
    USBOTG_FS->UEP3_DMA = (UINT32)pEP3_RAM_Addr;
    USBOTG_FS->UEP4_DMA = (UINT32)pEP4_RAM_Addr;
    USBOTG_FS->UEP5_DMA = (UINT32)pEP5_RAM_Addr;
    USBOTG_FS->UEP6_DMA = (UINT32)pEP6_RAM_Addr;
    USBOTG_FS->UEP7_DMA = (UINT32)pEP7_RAM_Addr;

    USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP1_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP2_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP3_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP4_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP5_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP6_RX_CTRL = USBHD_UEP_R_RES_ACK;
    USBOTG_FS->UEP7_RX_CTRL = USBHD_UEP_R_RES_ACK;

    USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_RES_NAK;
    USBOTG_FS->UEP1_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP2_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP3_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP4_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP5_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP6_TX_LEN = RepDataLoadLen;
    USBOTG_FS->UEP7_TX_LEN = RepDataLoadLen;

    USBOTG_FS->UEP1_TX_CTRL = USBHD_UEP_T_RES_ACK;
    USBOTG_FS->UEP2_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;
    USBOTG_FS->UEP3_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;
    USBOTG_FS->UEP4_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;
    USBOTG_FS->UEP5_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;
    USBOTG_FS->UEP6_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;
    USBOTG_FS->UEP7_TX_CTRL = USBHD_UEP_T_RES_ACK | USBHD_UEP_AUTO_TOG;

    USBOTG_FS->INT_FG = 0xFF;
    USBOTG_FS->INT_EN = USBHD_UIE_SUSPEND | USBHD_UIE_BUS_RST | USBHD_UIE_TRANSFER;
    USBOTG_FS->DEV_ADDR = 0x00;

    USBOTG_FS->BASE_CTRL = USBHD_UC_DEV_PU_EN | USBHD_UC_INT_BUSY | USBHD_UC_DMA_EN;
    USBOTG_FS->UDEV_CTRL = USBHD_UD_PD_DIS | USBHD_UD_PORT_EN;
}

/*********************************************************************
 * @fn      USBOTG_RCC_Init
 *
 * @brief   Initializes the usbotg clock configuration.
 *
 * @return  none
 */
void USBOTG_RCC_Init(void)
{
    RCC->CFGR2 &= ~RCC_USBFS_CLK_SRC; //usbotg clock selection: 0 = systick, 1 = usb20_phy
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div2);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_OTG_FS, ENABLE);
}

/*********************************************************************
 * @fn      USBOTG_Init
 *
 * @brief   Initializes the USBOTG full speed device.
 *
 * @return  none
 */
void USBOTG_Init(void)
{
    /* Endpoint buffer initialization */
    pEP0_RAM_Addr = EP0_DatabufHD;
    pEP1_RAM_Addr = EP1_DatabufHD;
    pEP2_RAM_Addr = EP2_DatabufHD;
    pEP3_RAM_Addr = EP3_DatabufHD;
    pEP4_RAM_Addr = EP4_DatabufHD;
    pEP5_RAM_Addr = EP5_DatabufHD;
    pEP6_RAM_Addr = EP6_DatabufHD;
    pEP7_RAM_Addr = EP7_DatabufHD;
    /* Enable USB clock */
    USBOTG_RCC_Init();
    Delay_Us(100);
    /* USB device initialization */
    USBDeviceInit();
    /* Enable USB interrupt */
    NVIC_EnableIRQ(USBHD_IRQn);
}

/*********************************************************************
 * @fn      USBHD_IRQHandler
 *
 * @brief   This function handles OTG_FS exception.
 *
 * @return  none
 */
void USBHD_IRQHandler(void)
{
    UINT8 len, chtype;
    UINT8 intflag, errflag = 0;

    intflag = USBOTG_FS->INT_FG;

    if(intflag & USBHD_UIF_TRANSFER)
    {
        switch(USBOTG_FS->INT_ST & USBHD_UIS_TOKEN_MASK)
        {
            /* Setup package processing */
            case USBHD_UIS_TOKEN_SETUP:

                USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_TOG | USBHD_UEP_T_RES_NAK;
                USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_TOG | USBHD_UEP_R_RES_ACK;
                SetupReqLen = pSetupReqPakHD->wLength;
                SetupReqCode = pSetupReqPakHD->bRequest;
                chtype = pSetupReqPakHD->bRequestType;
                len = 0;
                errflag = 0;
                /* Determine whether the current standard request or other requests */
                if((pSetupReqPakHD->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
                {
                    /* Other requests, such as class requests, manufacturer requests, etc. */
                    if(pSetupReqPakHD->bRequestType == 0xC0)
                    {
                        if(SetupReqCode == 0x5F)
                        {
                            pDescr = Return1;
                            len = sizeof(Return1);
                        }
                        else if(SetupReqCode == 0x95)
                        {
                            if((pSetupReqPakHD->wValue) == 0x18)
                            {
                                pDescr = Return2;
                                len = sizeof(Return2);
                            }
                            else if((pSetupReqPakHD->wValue) == 0x06)
                            {
                                pDescr = Return3;
                                len = sizeof(Return3);
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    else
                    {
                        len = 0;
                    }
                }
                else
                {
                    /* Processing standard USB request package */
                    switch(SetupReqCode)
                    {
                        case USB_GET_DESCRIPTOR:
                        {
                            switch(((pSetupReqPakHD->wValue) >> 8))
                            {
                                case USB_DESCR_TYP_DEVICE:
                                    /* Get the device descriptor */
                                    pDescr = MyDevDescrHD;
                                    len = MyDevDescrHD[0];
                                    break;

                                case USB_DESCR_TYP_CONFIG:
                                    /* Get the configuration descriptor */
                                    pDescr = MyCfgDescrHD;
                                    len = MyCfgDescrHD[2];
                                    break;

                                case USB_DESCR_TYP_STRING:
                                    /* Get the string descriptor */
                                    switch((pSetupReqPakHD->wValue) & 0xff)
                                    {
                                        case 0:
                                            /* Language string descriptor */
                                            pDescr = MyLangDescrHD;
                                            len = MyLangDescrHD[0];
                                            break;

                                        case 1:
                                            /* USB manufacturer string descriptor */
                                            pDescr = MyManuInfoHD;
                                            len = MyManuInfoHD[0];
                                            break;

                                        case 2:
                                            /* USB product string descriptor */
                                            pDescr = StrDesc;
                                            len = StrDesc[0];
                                            break;

                                        default:
                                            errflag = 0xFF;
                                            break;
                                    }
                                    break;

                                case USB_DESCR_TYP_REPORT:
                                    break;

                                case USB_DESCR_TYP_QUALIF:
                                    break;

                                case USB_DESCR_TYP_BOS:
                                    /* BOS descriptor */
                                    /* USB2.0 device does not support the BOS descriptor */
                                    errflag = 0xFF;
                                    break;

                                default:
                                    errflag = 0xff;
                                    break;
                            }

                            if(SetupReqLen > len)
                                SetupReqLen = len;
                            len = (SetupReqLen >= DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : SetupReqLen;
                            memcpy(pEP0_DataBuf, pDescr, len);
                            pDescr += len;
                        }
                        break;

                        case USB_SET_ADDRESS:
                            /* Setting address */
                            SetupReqLen = (pSetupReqPakHD->wValue) & 0xff;
                            break;

                        case USB_GET_CONFIGURATION:
                            /* Get the configuration value */
                            pEP0_DataBuf[0] = DevConfig;
                            if(SetupReqLen > 1)
                                SetupReqLen = 1;
                            break;

                        case USB_SET_CONFIGURATION:
                            /* Set the configuration value */
                            DevConfig = (pSetupReqPakHD->wValue) & 0xff;
                            break;

                        case USB_CLEAR_FEATURE:
                            /* Cleaer feature */
                            if((pSetupReqPakHD->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                            {
                                /* Clear endpoint */
                                switch((pSetupReqPakHD->wIndex) & 0xff)
                                {
                                    case 0x82:
                                        USBOTG_FS->UEP2_TX_CTRL = (USBOTG_FS->UEP2_TX_CTRL & ~(USBHD_UEP_T_TOG | USBHD_UEP_T_RES_MASK)) | USBHD_UEP_T_RES_NAK;
                                        break;

                                    case 0x02:
                                        USBOTG_FS->UEP2_RX_CTRL = (USBOTG_FS->UEP2_RX_CTRL & ~(USBHD_UEP_R_TOG | USBHD_UEP_R_RES_MASK)) | USBHD_UEP_R_RES_ACK;
                                        break;

                                    case 0x81:
                                        USBOTG_FS->UEP1_TX_CTRL = (USBOTG_FS->UEP1_TX_CTRL & ~(USBHD_UEP_T_TOG | USBHD_UEP_T_RES_MASK)) | USBHD_UEP_T_RES_NAK;
                                        break;

                                    case 0x01:
                                        USBOTG_FS->UEP1_RX_CTRL = (USBOTG_FS->UEP1_RX_CTRL & ~(USBHD_UEP_R_TOG | USBHD_UEP_R_RES_MASK)) | USBHD_UEP_R_RES_ACK;
                                        break;

                                    default:
                                        errflag = 0xFF;
                                        break;
                                }
                            }
                            else
                                errflag = 0xFF;
                            break;

                        case USB_SET_FEATURE:
                            /* Set feature */
                            if((pMySetupReqPakHD->bRequestType & 0x1F) == 0x00)
                            {
                                /* Setting device */
                                if(pMySetupReqPakHD->wValue == 0x01)
                                {
                                    if(MyCfgDescrHD[7] & 0x20)
                                    {
                                        /* Set wake-up enable flag */
                                        USBHD_Dev_SleepStatus = 0x01;
                                    }
                                    else
                                    {
                                        errflag = 0xFF;
                                    }
                                }
                                else
                                {
                                    errflag = 0xFF;
                                }
                            }
                            else if((pMySetupReqPakHD->bRequestType & 0x1F) == 0x02)
                            {
                                /* Set the endpoint */
                                if(pMySetupReqPakHD->wValue == 0x00)
                                {
                                    /* Set the specified endpoint stall */
                                    switch((pMySetupReqPakHD->wIndex) & 0xff)
                                    {
                                        case 0x82:
                                            /* Set endpoint 2 IN STALL */
                                            USBOTG_FS->UEP2_TX_CTRL = (USBOTG_FS->UEP2_TX_CTRL &= ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_STALL;
                                            //USBHS->UEP2_CTRL  = ( USBHS->UEP2_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                            break;

                                        case 0x02:
                                            /* Set endpoint 2 OUT Stall */
                                            USBOTG_FS->UEP2_RX_CTRL = (USBOTG_FS->UEP2_RX_CTRL &= ~USBHD_UEP_R_RES_MASK) | USBHD_UEP_R_RES_STALL;
                                            //USBHS->UEP2_CTRL  = ( USBHS->UEP2_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                            break;

                                        case 0x81:
                                            /* Set endpoint 1 IN STALL */
                                            USBOTG_FS->UEP1_TX_CTRL = (USBOTG_FS->UEP1_TX_CTRL &= ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_STALL;
                                            //USBHS->UEP1_CTRL  = ( USBHS->UEP1_CTRL & ~USBHS_EP_T_RES_MASK ) | USBHS_EP_T_RES_STALL;
                                            break;

                                        case 0x01:
                                            /* Set endpoint 1 OUT STALL */
                                            USBOTG_FS->UEP1_RX_CTRL = (USBOTG_FS->UEP1_RX_CTRL &= ~USBHD_UEP_R_RES_MASK) | USBHD_UEP_R_RES_STALL;
                                            //USBHS->UEP1_CTRL  = ( USBHS->UEP1_CTRL & ~USBHS_EP_R_RES_MASK ) | USBHS_EP_R_RES_STALL;
                                            break;

                                        default:
                                            errflag = 0xFF;
                                            break;
                                    }
                                }
                                else
                                {
                                    errflag = 0xFF;
                                }
                            }
                            else
                            {
                                errflag = 0xFF;
                            }
                            break;

                        case USB_GET_INTERFACE:
                            /* Obtain interface */
                            pEP0_DataBuf[0] = 0x00;
                            if(SetupReqLen > 1)
                                SetupReqLen = 1;
                            break;

                        case USB_SET_INTERFACE:
                            /* Set interface */
                            EP0_DatabufHD[0] = 0x00;
                            if(USBHD_Dev_SetupReqLen > 1)
                            {
                                USBHD_Dev_SetupReqLen = 1;
                            }
                            break;

                        case USB_GET_STATUS:
                            /* Reply according to the actual state of the current endpoint */
                            EP0_DatabufHD[0] = 0x00;
                            EP0_DatabufHD[1] = 0x00;
                            if(pMySetupReqPakHD->wIndex == 0x81)
                            {
                                if((USBOTG_FS->UEP1_TX_CTRL & USBHD_UEP_T_RES_MASK) == USBHD_UEP_T_RES_STALL)
                                {
                                    EP0_DatabufHD[0] = 0x01;
                                }
                            }
                            else if(pMySetupReqPakHD->wIndex == 0x01)
                            {
                                if((USBOTG_FS->UEP1_RX_CTRL & USBHD_UEP_R_RES_MASK) == USBHD_UEP_R_RES_STALL)
                                {
                                    EP0_DatabufHD[0] = 0x01;
                                }
                            }
                            else if(pMySetupReqPakHD->wIndex == 0x82)
                            {
                                if((USBOTG_FS->UEP2_TX_CTRL & USBHD_UEP_T_RES_MASK) == USBHD_UEP_T_RES_STALL)
                                {
                                    EP0_DatabufHD[0] = 0x01;
                                }
                            }
                            else if(pMySetupReqPakHD->wIndex == 0x02)
                            {
                                if((USBOTG_FS->UEP2_RX_CTRL & USBHD_UEP_R_RES_MASK) == USBHD_UEP_R_RES_STALL)
                                {
                                    EP0_DatabufHD[0] = 0x01;
                                }
                            }
                            if(USBHD_Dev_SetupReqLen > 2)
                            {
                                USBHD_Dev_SetupReqLen = 2;
                            }
                            break;

                        default:
                            errflag = 0xff;
                            break;
                    }
                }
                if(errflag == 0xff)
                {
                    USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_TOG | USBHD_UEP_T_RES_STALL;
                    USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_TOG | USBHD_UEP_R_RES_STALL;
                }
                else
                {
                    if(chtype & 0x80)
                    {
                        len = (SetupReqLen > DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : SetupReqLen;
                        SetupReqLen -= len;
                    }
                    else
                        len = 0;

                    USBOTG_FS->UEP0_TX_LEN = len;
                    USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_TOG | USBHD_UEP_T_RES_ACK;
                    USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_TOG | USBHD_UEP_R_RES_ACK;
                }
                break;

            case USBHD_UIS_TOKEN_IN:
                switch(USBOTG_FS->INT_ST & (USBHD_UIS_TOKEN_MASK | USBHD_UIS_ENDP_MASK))
                {
                    case USBHD_UIS_TOKEN_IN:
                        switch(SetupReqCode)
                        {
                            case USB_GET_DESCRIPTOR:
                                len = SetupReqLen >= DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : SetupReqLen;
                                memcpy(pEP0_DataBuf, pDescr, len);
                                SetupReqLen -= len;
                                pDescr += len;
                                USBOTG_FS->UEP0_TX_LEN = len;
                                USBOTG_FS->UEP0_TX_CTRL ^= USBHD_UEP_T_TOG;
                                break;

                            case USB_SET_ADDRESS:
                                USBOTG_FS->DEV_ADDR = (USBOTG_FS->DEV_ADDR & USBHD_UDA_GP_BIT) | SetupReqLen;
                                USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_RES_NAK;
                                USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_RES_ACK;
                                break;

                            default:
                                USBOTG_FS->UEP0_TX_LEN = 0;
                                USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_RES_NAK;
                                USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_RES_ACK;
                                break;
                        }
                        break;
                        /* Back to NCK by default */
                    case USBHD_UIS_TOKEN_IN | 1:
                        USBOTG_FS->UEP1_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP1_TX_CTRL = (USBOTG_FS->UEP1_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 2:

                        USBOTG_FS->UEP2_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP2_TX_CTRL = (USBOTG_FS->UEP2_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 3:
                        USBOTG_FS->UEP3_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP3_TX_CTRL = (USBOTG_FS->UEP3_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 4:
                        USBOTG_FS->UEP4_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP4_TX_CTRL = (USBOTG_FS->UEP4_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 5:
                        USBOTG_FS->UEP5_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP5_TX_CTRL = (USBOTG_FS->UEP5_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 6:
                        USBOTG_FS->UEP6_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP6_TX_CTRL = (USBOTG_FS->UEP6_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    case USBHD_UIS_TOKEN_IN | 7:
                        USBOTG_FS->UEP7_TX_CTRL ^= USBHD_UEP_T_TOG;
                        USBOTG_FS->UEP7_TX_CTRL = (USBOTG_FS->UEP7_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_NAK;
                        break;

                    default:
                        break;
                }
                break;

            case USBHD_UIS_TOKEN_OUT:
                switch(USBOTG_FS->INT_ST & (USBHD_UIS_TOKEN_MASK | USBHD_UIS_ENDP_MASK))
                {
                    case USBHD_UIS_TOKEN_OUT:
                        len = USBOTG_FS->RX_LEN;
                        break;

                    case USBHD_UIS_TOKEN_OUT | 1:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP1_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;

                            DevEP1_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 2:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP2_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP2_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 3:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP3_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP3_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 4:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP4_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP4_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 5:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP5_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP5_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 6:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP6_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP6_OUT_Deal(len);
                        }
                        break;

                    case USBHD_UIS_TOKEN_OUT | 7:
                        if(USBOTG_FS->INT_ST & USBHD_UIS_TOG_OK)
                        {
                            USBOTG_FS->UEP7_RX_CTRL ^= USBHD_UEP_R_TOG;
                            len = USBOTG_FS->RX_LEN;
                            DevEP7_OUT_Deal(len);
                        }
                        break;
                }
                break;

            case USBHD_UIS_TOKEN_SOF:

                break;

            default:
                break;
        }

        USBOTG_FS->INT_FG = USBHD_UIF_TRANSFER;
    }
    else if(intflag & USBHD_UIF_BUS_RST)
    {
        USBOTG_FS->DEV_ADDR = 0;

        USBOTG_FS->UEP0_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP1_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP2_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP3_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP4_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP5_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP6_RX_CTRL = USBHD_UEP_R_RES_ACK;
        USBOTG_FS->UEP7_RX_CTRL = USBHD_UEP_R_RES_ACK;

        USBOTG_FS->UEP0_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP1_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP2_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP3_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP4_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP5_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP6_TX_CTRL = USBHD_UEP_T_RES_NAK;
        USBOTG_FS->UEP7_TX_CTRL = USBHD_UEP_T_RES_NAK;

        USBOTG_FS->INT_FG |= USBHD_UIF_BUS_RST;
    }
    else if(intflag & USBHD_UIF_SUSPEND)
    {
        if(USBOTG_FS->MIS_ST & USBHD_UMS_SUSPEND)
        {
            ;
        }
        else
        {
            ;
        }
        USBOTG_FS->INT_FG = USBHD_UIF_SUSPEND;
    }
    else
    {
        USBOTG_FS->INT_FG = intflag;
    }
}

/*********************************************************************
 * @fn      USBSendData
 *
 * @brief   Send data to the host
 *
 * @return  none
 */
uint8_t USBSendData(void)
{
    if(USBOTG_FS->UEP2_TX_CTRL & USBHD_UEP_T_RES_MASK == USBHD_UEP_T_RES_ACK)
    {
        return FAILURE;
    }
    if(RingMemBLE.CurrentLen > 32)
    {
        RingMemRead(&RingMemBLE, pEP2_IN_DataBuf, 32);
        DevEP2_IN_Deal(32);
    }
    else
    {
        uint8_t len = RingMemBLE.CurrentLen;
        RingMemRead(&RingMemBLE, pEP2_IN_DataBuf, len);
        DevEP2_IN_Deal(len);
    }
    return SUCCESS;
}

/*********************************************************************
 * @fn      DevEP1_IN_Deal
 *
 * @brief   Device endpoint1 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP1_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP1_TX_LEN = l;
    USBOTG_FS->UEP1_TX_CTRL = (USBOTG_FS->UEP1_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP2_IN_Deal
 *
 * @brief   Device endpoint2 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP2_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP2_TX_LEN = l;
    USBOTG_FS->UEP2_TX_CTRL = (USBOTG_FS->UEP2_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP3_IN_Deal
 *
 * @brief   Device endpoint3 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP3_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP3_TX_LEN = l;
    USBOTG_FS->UEP3_TX_CTRL = (USBOTG_FS->UEP3_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP4_IN_Deal
 *
 * @brief   Device endpoint4 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP4_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP4_TX_LEN = l;
    USBOTG_FS->UEP4_TX_CTRL = (USBOTG_FS->UEP4_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP5_IN_Deal
 *
 * @brief   Device endpoint5 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP5_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP5_TX_LEN = l;
    USBOTG_FS->UEP5_TX_CTRL = (USBOTG_FS->UEP5_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP6_IN_Deal
 *
 * @brief   Device endpoint6 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP6_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP6_TX_LEN = l;
    USBOTG_FS->UEP6_TX_CTRL = (USBOTG_FS->UEP6_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP7_IN_Deal
 *
 * @brief   Device endpoint7 IN.
 *
 * @param   l - IN length(<64B)
 *
 * @return  none
 */
void DevEP7_IN_Deal(UINT8 l)
{
    USBOTG_FS->UEP7_TX_LEN = l;
    USBOTG_FS->UEP7_TX_CTRL = (USBOTG_FS->UEP7_TX_CTRL & ~USBHD_UEP_T_RES_MASK) | USBHD_UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   Deal device Endpoint 1 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP1_OUT_Deal(UINT8 l)
{
}

/*********************************************************************
 * @fn      DevEP2_OUT_Deal
 *
 * @brief   Deal device Endpoint 2 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP2_OUT_Deal(UINT8 l)
{
    if(RingMemWrite(&RingMemUSB, pEP2_OUT_DataBuf, l) != SUCCESS)
    {
        PRINT("RingMemBLE ERR \n");
    }
    tmos_start_task(Peripheral_TaskID, SBP_PROCESS_USBDATA_EVT, 32);
}

/*********************************************************************
 * @fn      DevEP3_OUT_Deal
 *
 * @brief   Deal device Endpoint 3 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP3_OUT_Deal(UINT8 l)
{
}

/*********************************************************************
 * @fn      DevEP4_OUT_Deal
 *
 * @brief   Deal device Endpoint 4 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP4_OUT_Deal(UINT8 l)
{
}

/*********************************************************************
 * @fn      DevEP5_OUT_Deal
 *
 * @brief   Deal device Endpoint 5 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP5_OUT_Deal(UINT8 l)
{
}

/*********************************************************************
 * @fn      DevEP6_OUT_Deal
 *
 * @brief   Deal device Endpoint 6 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP6_OUT_Deal(UINT8 l)
{
}

/*********************************************************************
 * @fn      DevEP7_OUT_Deal
 *
 * @brief   Deal device Endpoint 7 OUT.
 *
 * @param   l - Data length.
 *
 * @return  none
 */
void DevEP7_OUT_Deal(UINT8 l)
{
}
