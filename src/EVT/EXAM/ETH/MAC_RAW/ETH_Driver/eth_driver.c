/********************************** (C) COPYRIGHT *******************************
* File Name          : eth_driver.c
* Author             : WCH
* Version            : V1.3.0
* Date               : 2022/05/26
* Description        : eth program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "string.h"
#include "eth_driver.h"

__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB];       /* MAC receive descriptor, 4-byte aligned*/
__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB];       /* MAC send descriptor, 4-byte aligned */

__attribute__((__aligned__(4))) uint8_t  MACRxBuf[ETH_RXBUFNB*ETH_RX_BUF_SZE];      /* MAC receive buffer, 4-byte aligned */
__attribute__((__aligned__(4))) uint8_t  MACTxBuf[ETH_TXBUFNB*ETH_TX_BUF_SZE];      /* MAC send buffer, 4-byte aligned */

ETH_DMADESCTypeDef *pDMARxSet;
ETH_DMADESCTypeDef *pDMATxSet;
ETH_DMADESCTypeDef *DMATxDescToSet;
ETH_DMADESCTypeDef *DMARxDescToGet;

volatile uint8_t LinkSta = 0; //0: No valid link established   1:Valid link establishe
volatile uint8_t phyLinkReset;
volatile uint32_t phyLinkTime;
volatile uint32_t LocalTime;
uint8_t phyPN = 0x01;
uint8_t phyStatus = 0;
uint8_t phySucCnt = 0;
uint8_t phyLinkCnt = 0;
uint8_t phyRetryCnt = 0;
uint8_t CRCErrPktCnt = 0;
uint8_t phyLinkStatus = 0;
uint8_t phyPNChangeCnt = 0;
uint8_t PhyPolarityDetect = 0;
/*********************************************************************
 * @fn      WCHNET_GetMacAddr
 *
 * @brief   Get MAC address
 *
 * @return  none.
 */
void WCHNET_GetMacAddr( uint8_t *p )
{
    uint8_t i;
    uint8_t *macaddr=(uint8_t *)(ROM_CFG_USERADR_ID+5);

    for(i=0;i<6;i++)
    {
        *p = *macaddr;
        p++;
        macaddr--;
    }
}

/*********************************************************************
 * @fn      WCHNET_TimeIsr
 *
 * @brief
 *
 * @return  none.
 */
void WCHNET_TimeIsr( uint16_t timperiod )
{
    LocalTime += timperiod;
}

/*********************************************************************
 * @fn      WritePHYReg
 *
 * @brief   MCU write PHY register.
 *
 * @param   reg_add - PHY address,
 *          reg_val - value you want to write.
 *
 * @return  none
 */
void WritePHYReg(uint8_t reg_add,uint16_t reg_val)
{
    R32_ETH_MIWR = (reg_add & RB_ETH_MIREGADR_MIRDL) | (1<<8) | (reg_val<<16);
}

/*********************************************************************
 * @fn      ReadPHYReg
 *
 * @brief   MCU read PHY register.
 *
 * @param   reg_add - PHY address.
 *
 * @return  value you want to get.
 */
uint16_t ReadPHYReg(uint8_t reg_add)
{
    R8_ETH_MIREGADR = reg_add;                          // write address
    return R16_ETH_MIRD;                                // get data
}

/*********************************************************************
 * @fn      WCHNET_LinkProcess
 *
 * @brief   link process.
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_LinkProcess( void )
{
    uint16_t phy_anlpar, phy_bmcr, phy_bmsr;

    phy_anlpar = ReadPHYReg(PHY_ANLPAR);
    phy_bmsr = ReadPHYReg(PHY_BMSR);

    if(phy_anlpar&PHY_ANLPAR_SELECTOR_FIELD)
    {
        if( !(phyLinkStatus&PHY_LINK_WAIT_SUC) )
        {
            if( (phyPN&0x0C) == PHY_PN_SWITCH_P )
            {
                phySucCnt = 0;
                phyLinkCnt = 0;
                phyLinkStatus = PHY_LINK_WAIT_SUC;
            }
            else
            {
                if( !(phyLinkStatus&PHY_LINK_SUC_N) )
                {
                    phyRetryCnt = 0;
                    phyLinkStatus |= PHY_LINK_SUC_N;
                    phyPN &= ~PHY_PN_SWITCH_N;
                    phy_bmcr = ReadPHYReg(PHY_BMCR);
                    phy_bmcr |= 1<<9;
                    WritePHYReg(PHY_BMCR, phy_bmcr);
                    WritePHYReg(PHY_MDIX, phyPN);
                }
                else
                {
                    phySucCnt = 0;
                    phyLinkCnt = 0;
                    phyLinkStatus = PHY_LINK_WAIT_SUC;
                }
            }
        }
        else{
            if((phySucCnt++ == 5) && ((phy_bmsr&PHY_AutoNego_Complete) == 0))
            {
                phySucCnt = 0;
                phyRetryCnt = 0;
                phyPNChangeCnt = 0;
                phyLinkStatus = PHY_LINK_INIT;
                phy_bmcr = ReadPHYReg(PHY_BMCR);
                phy_bmcr |= 1<<9;
                WritePHYReg(PHY_BMCR, phy_bmcr);
                if((phyPN&0x0C) == PHY_PN_SWITCH_P)
                {
                    phyPN |= PHY_PN_SWITCH_N;
                }
                else {
                    phyPN &= ~PHY_PN_SWITCH_N;
                }
                WritePHYReg(PHY_MDIX, phyPN);
            }
        }
    }
    else
    {
        if(phy_bmsr & PHY_AutoNego_Complete)
        {
            phySucCnt = 0;
            phyLinkCnt = 0;
            phyLinkStatus = PHY_LINK_WAIT_SUC;
        }
        else {
            if( phyLinkStatus == PHY_LINK_WAIT_SUC )
            {
                if(phyLinkCnt++ == 10)
                {
                    phyLinkCnt = 0;
                    phyRetryCnt = 0;
                    phyPNChangeCnt = 0;
                    phyLinkStatus = PHY_LINK_INIT;
                }
            }
            else if(phyLinkStatus == PHY_LINK_INIT)
            {
                if(phyPNChangeCnt++ == 10)
                {
                    phyPNChangeCnt = 0;
                    phyPN = ReadPHYReg(PHY_MDIX);
                    phyPN &= ~0x0c;
                    phyPN ^= 0x03;
                    WritePHYReg(PHY_MDIX, phyPN);
                }
                else{
                    if((phyPN&0x0C) == PHY_PN_SWITCH_P)
                    {
                        phyPN |= PHY_PN_SWITCH_N;
                    }
                    else {
                        phyPN &= ~PHY_PN_SWITCH_N;
                    }
                    WritePHYReg(PHY_MDIX, phyPN);
                }
            }
            else if(phyLinkStatus == PHY_LINK_SUC_N)
            {
                if((phyPN&0x0C) == PHY_PN_SWITCH_P)
                {
                    phyPN |= PHY_PN_SWITCH_N;
                    phy_bmcr = ReadPHYReg(PHY_BMCR);
                    phy_bmcr |= 1<<9;
                    WritePHYReg(PHY_BMCR, phy_bmcr);
                    Delay_Us(10);
                    WritePHYReg(PHY_MDIX, phyPN);
                }
                else{
                    if(phyRetryCnt++ == 15)
                    {
                        phyRetryCnt = 0;
                        phyPNChangeCnt = 0;
                        phyLinkStatus = PHY_LINK_INIT;
                    }
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_PhyPNProcess
 *
 * @brief   Phy PN Polarity related processing
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_PhyPNProcess(void)
{
    uint32_t PhyVal;

    phyLinkTime = LocalTime;
    if(CRCErrPktCnt >= 3)
    {
        PhyVal = ReadPHYReg(PHY_MDIX);
        if((PhyVal >> 2) & 0x01)
            PhyVal &= ~(3 << 2);                //change PHY PN Polarity to normal
        else
            PhyVal |= 1 << 2;                   //change PHY PN Polarity to reverse
        WritePHYReg(PHY_MDIX, PhyVal);
        CRCErrPktCnt = 0;
    }
}

/*********************************************************************
 * @fn      WCHNET_HandlePhyNegotiation
 *
 * @brief   Handle PHY Negotiation.
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_HandlePhyNegotiation(void)
{
    if(phyLinkReset)              /* After the PHY link is disconnected, wait 500ms before turning on the PHY clock*/
    {
        if( LocalTime - phyLinkTime >= 500 )
        {
            phyLinkReset = 0;
            EXTEN->EXTEN_CTR |= EXTEN_ETH_10M_EN;
            WritePHYReg(PHY_BMCR, PHY_Reset);
            PHY_NEGOTIATION_PARAM_INIT();
        }
    }
    else
    {
        if( !phyStatus )                        /* Handling PHY Negotiation Exceptions */
        {
            if( LocalTime - phyLinkTime >= PHY_LINK_TASK_PERIOD )  /* 50ms cycle timing call */
            {
                phyLinkTime = LocalTime;
                WCHNET_LinkProcess( );
            }
        }
        else{
            if(PhyPolarityDetect)
            {
                if( LocalTime - phyLinkTime >= 2 * PHY_LINK_TASK_PERIOD )
                {
                    WCHNET_PhyPNProcess();
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      RecDataPolling
 *
 * @brief   process received data.
 *
 * @param   none.
 *
 * @return  none.
 */
void RecDataPolling(void)
{
    uint32_t length, buffer;

    while(!(pDMARxSet->Status & ETH_DMARxDesc_OWN))
    {
        if( !(pDMARxSet->Status & ETH_DMARxDesc_ES) &&\
        (pDMARxSet->Status & ETH_DMARxDesc_LS) &&\
        (pDMARxSet->Status & ETH_DMARxDesc_FS) )
        {
            /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
            length = ((pDMARxSet->Status & ETH_DMARxDesc_FL) >> 16) - 4;
            /* Get the addrees of the actual buffer */
            buffer = pDMARxSet->Buffer1Addr;

            /* Do something*/
            printf("rec data:%d bytes\r\n",length);
            printf("data:%x\r\n",*((uint8_t *)buffer));
        }
        pDMARxSet->Status = ETH_DMARxDesc_OWN;
        pDMARxSet = (ETH_DMADESCTypeDef *)pDMARxSet->Buffer2NextDescAddr;
    }
}

/*********************************************************************
 * @fn      WCHNET_MainTask
 *
 * @brief   library main task function
 *
 * @return  none.
 */
void WCHNET_MainTask(void)
{
    WCHNET_HandlePhyNegotiation( );
    RecDataPolling();
}

/*********************************************************************
 * @fn      ETH_DMATxDescChainInit
 *
 * @brief   Initializes the DMA Tx descriptors in chain mode.
 *
 * @param   DMATxDescTab - Pointer on the first Tx desc list
 *          TxBuff - Pointer on the first TxBuffer list
 *          TxBuffCount - Number of the used Tx desc in the list
 *
 * @return  none
 */
void ETH_DMATxDescChainInit(ETH_DMADESCTypeDef *DMATxDescTab, uint8_t *TxBuff, uint32_t TxBuffCount)
{
    ETH_DMADESCTypeDef *DMATxDesc;

    DMATxDescToSet = DMATxDescTab;
    DMATxDesc = DMATxDescTab;
    DMATxDesc->Status = 0;
    DMATxDesc->Buffer1Addr = (uint32_t)TxBuff;
    DMATxDesc->Buffer2NextDescAddr = (uint32_t)DMATxDescTab;
}

/*********************************************************************
 * @fn      ETH_DMARxDescChainInit
 *
 * @brief   Initializes the DMA Rx descriptors in chain mode.
 *
 * @param   DMARxDescTab - Pointer on the first Rx desc list.
 *          RxBuff - Pointer on the first RxBuffer list.
 *          RxBuffCount - Number of the used Rx desc in the list.
 *
 * @return  none
 */
void ETH_DMARxDescChainInit(ETH_DMADESCTypeDef *DMARxDescTab, uint8_t *RxBuff, uint32_t RxBuffCount)
{
    uint8_t i = 0;
    ETH_DMADESCTypeDef *DMARxDesc;

    DMARxDescToGet = DMARxDescTab;
    for(i = 0; i < RxBuffCount; i++)
    {
        DMARxDesc = DMARxDescTab + i;
        DMARxDesc->Status = ETH_DMARxDesc_OWN;
        DMARxDesc->Buffer1Addr = (uint32_t)(&RxBuff[i * ETH_MAX_PACKET_SIZE]);

        if(i < (RxBuffCount - 1))
        {
            DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab + i + 1);
        }
        else
        {
            DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab);
        }
    }
}

/*********************************************************************
 * @fn      ETH_Start
 *
 * @brief   Enables ENET MAC and DMA reception/transmission.
 *
 * @return  none
 */
void ETH_Start(void)
{
    R16_ETH_ERXST = DMARxDescToGet->Buffer1Addr;
    R8_ETH_ECON1 |= RB_ETH_ECON1_RXEN;                                //receive enable
}

/*********************************************************************
 * @fn      ETH_SetClock
 *
 * @brief   Set ETH Clock(60MHz).
 *
 * @return  none
 */
void ETH_SetClock(void)
{
    /* ETH initialization */
    RCC_ETHDIVConfig(RCC_ETHCLK_Div2);  // 120M/2 = 60MHz
}

/*********************************************************************
 * @fn      ETH_Configuration
 *
 * @brief   Ethernet configure.
 *
 * @return  none
 */
void ETH_Configuration( uint8_t *macAddr )
{
    ETH_SetClock( );
    R8_ETH_EIE = 0;
    R8_ETH_EIE |= RB_ETH_EIE_INTIE |
                  RB_ETH_EIE_RXIE|
                  RB_ETH_EIE_LINKIE|
                  RB_ETH_EIE_TXIE  |
                  RB_ETH_EIE_TXERIE|
                  RB_ETH_EIE_RXERIE;                                    //Turn on all interrupts

    R8_ETH_EIE |= RB_ETH_EIE_R_EN50;                                    //Turn on 50 ohm pull-up

    R8_ETH_EIR = 0xff;                                                  //clear interrupt flag
    R8_ETH_ESTAT |= RB_ETH_ESTAT_INT | RB_ETH_ESTAT_BUFER;              //clear state

    R8_ETH_ECON1 |= (RB_ETH_ECON1_TXRST|RB_ETH_ECON1_RXRST);            //Transceiver module reset
    R8_ETH_ECON1 &= ~(RB_ETH_ECON1_TXRST|RB_ETH_ECON1_RXRST);

    //Filter mode, received packet type
    R8_ETH_ERXFCON = 0;
    R8_ETH_MAADRL1 = macAddr[5];                                        // MAC assignment
    R8_ETH_MAADRL2 = macAddr[4];
    R8_ETH_MAADRL3 = macAddr[3];
    R8_ETH_MAADRL4 = macAddr[2];
    R8_ETH_MAADRL5 = macAddr[1];
    R8_ETH_MAADRL6 = macAddr[0];

    //Filter mode, limit packet type
    R8_ETH_MACON1 |= RB_ETH_MACON1_MARXEN;                              //MAC receive enable
    R8_ETH_MACON2 &= ~RB_ETH_MACON2_PADCFG;
    R8_ETH_MACON2 |= PADCFG_AUTO_3;                                     //All short packets are automatically padded to 60 bytes
    R8_ETH_MACON2 |= RB_ETH_MACON2_TXCRCEN;                             //Hardware padded CRC
    R8_ETH_MACON2 &= ~RB_ETH_MACON2_HFRMEN;                             //Jumbo frames are not received
    R8_ETH_MACON2 |= RB_ETH_MACON2_FULDPX;
    R16_ETH_MAMXFL = ETH_MAX_PACKET_SIZE;
    R8_ETH_ECON2 &= ~(0x07 << 1);
    R8_ETH_ECON2 |= 5 << 1;

    EXTEN->EXTEN_CTR |= EXTEN_ETH_10M_EN;
}

/*********************************************************************
 * @fn      MACRAW_Tx
 *
 * @brief   Ethernet sends data frames in chain mode.
 *
 * @param   buff   send buffer pointer
 *          len    Send data length
 *
 * @return  Send status.
 */
uint32_t MACRAW_Tx(uint8_t *buff, uint16_t len)
{
     /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
    if( DMATxDescToSet->Status & ETH_DMATxDesc_OWN )
    {
        /* Return ERROR: OWN bit set */
        return ETH_ERROR;
    }
    DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;
    R16_ETH_ETXLN = len;
    R16_ETH_ETXST = (uint32_t)buff;
    R8_ETH_ECON1 |= RB_ETH_ECON1_TXRTS;                               //start sending
    /* Update the ETHERNET DMA global Tx descriptor with next Tx descriptor */
    /* Chained Mode */
    /* Selects the next DMA Tx descriptor list for next buffer to send */
    DMATxDescToSet = (ETH_DMADESCTypeDef*) (DMATxDescToSet->Buffer2NextDescAddr);
    /* Return SUCCESS */
    return ETH_SUCCESS;
}

/*********************************************************************
 * @fn      ETH_LinkUpCfg
 *
 * @brief   When the PHY is connected, configure the relevant functions.
 *
 * @param   regval  BMSR register value
 *
 * @return  none.
 */
void ETH_LinkUpCfg(uint16_t regval)
{
    LinkSta = 1;
    /* Receive CRC error packets */
    R8_ETH_ERXFCON |= RB_ETH_ERXFCON_CRCEN;
    CRCErrPktCnt = 0;
    PhyPolarityDetect = 1;
    phyLinkTime = LocalTime;
    phyStatus = PHY_Linked_Status;
    ETH_Start( );
}

/*********************************************************************
 * @fn      ETH_LinkDownCfg
 *
 * @brief   When the PHY is disconnected, configure the relevant functions.
 *
 * @param   regval  BMSR register value
 *
 * @return  none.
 */
void ETH_LinkDownCfg(uint16_t regval)
{
    LinkSta = 0;
    EXTEN->EXTEN_CTR &= ~EXTEN_ETH_10M_EN;
    phyLinkReset = 1;
    phyLinkTime = LocalTime;
}

/*********************************************************************
 * @fn      ETH_PHYLink
 *
 * @brief
 *
 * @return  none
 */
void ETH_PHYLink( void )
{
    u16 phy_bsr, phy_anlpar;

    phy_bsr = ReadPHYReg(PHY_BMSR);
    phy_anlpar = ReadPHYReg(PHY_ANLPAR);

    if(phy_bsr & PHY_Linked_Status)     //Valid link established
    {
        if(phy_bsr & PHY_AutoNego_Complete)     //Auto-negotiation completed -- LinkUp
        {
            ETH_LinkUpCfg(phy_bsr);
        }
        else {
            if(phy_anlpar == 0)     //The auto-negotiation signal of the peer device is not obtained
            {
                WritePHYReg(PHY_BMCR, PHY_Reset);
                PHY_NEGOTIATION_PARAM_INIT();
            }
            else {
                ETH_LinkDownCfg(phy_bsr);
            }
        }
    }
    else {                              //LinkDown
        ETH_LinkDownCfg(phy_bsr);
    }
}

/*********************************************************************
 * @fn      WCHNET_ETHIsr
 *
 * @brief
 *
 * @return  none
 */
void WCHNET_ETHIsr( void )
{
    uint8_t eth_irq_flag, estat_regval;

    eth_irq_flag = R8_ETH_EIR;
    if(eth_irq_flag&RB_ETH_EIR_RXIF)                                //Receive complete
    {
        R8_ETH_EIR = RB_ETH_EIR_RXIF;
        /* Check if the descriptor is owned by the ETHERNET DMA */
        if( DMARxDescToGet->Status & ETH_DMARxDesc_OWN )
        {
            estat_regval = R8_ETH_ESTAT;
            if(estat_regval & \
                    (RB_ETH_ESTAT_BUFER | RB_ETH_ESTAT_RXCRCER | RB_ETH_ESTAT_RXNIBBLE | RB_ETH_ESTAT_RXMORE))
            {
                return;
            }
            if( ((ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr))->Status& ETH_DMARxDesc_OWN )
            {
                DMARxDescToGet->Status &= ~ETH_DMARxDesc_OWN;
                DMARxDescToGet->Status &= ~ETH_DMARxDesc_ES;
                DMARxDescToGet->Status |= (ETH_DMARxDesc_FS|ETH_DMARxDesc_LS);
                DMARxDescToGet->Status &= ~ETH_DMARxDesc_FL;
                DMARxDescToGet->Status |= ((R16_ETH_ERXLN+4)<<ETH_DMARxDesc_FrameLengthShift);
                /* Update the ETHERNET DMA global Rx descriptor with next Rx descriptor */
                /* Selects the next DMA Rx descriptor list for next buffer to read */
                DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);
                R16_ETH_ERXST = DMARxDescToGet->Buffer1Addr;
            }
        }
        if(PhyPolarityDetect)
		{
			PhyPolarityDetect = 0;
			/* Discard CRC error packet */
			R8_ETH_ERXFCON &= ~RB_ETH_ERXFCON_CRCEN;
		}
    }
    if(eth_irq_flag&RB_ETH_EIR_TXIF)                                //send completed
    {
        DMATxDescToSet->Status &= ~ETH_DMATxDesc_OWN;
        R8_ETH_EIR = RB_ETH_EIR_TXIF;
    }
    if(eth_irq_flag&RB_ETH_EIR_LINKIF)                              //Link change
    {
        ETH_PHYLink();
        R8_ETH_EIR = RB_ETH_EIR_LINKIF;
    }
    if(eth_irq_flag&RB_ETH_EIR_TXERIF)                              //send error
    {
        DMATxDescToSet->Status &= ~ETH_DMATxDesc_OWN;
        R8_ETH_EIR = RB_ETH_EIR_TXERIF;
    }
    if(eth_irq_flag&RB_ETH_EIR_RXERIF)                              //receive error
    {
        if(PhyPolarityDetect) CRCErrPktCnt++;
        R8_ETH_EIR = RB_ETH_EIR_RXERIF;
    }
}

/*********************************************************************
 * @fn      ETH_Init
 *
 * @brief   Ethernet initialization.
 *
 * @return  none
 */
void ETH_Init( uint8_t *macAddr )
{
    Delay_Ms(100);
    ETH_Configuration( macAddr );
    ETH_DMATxDescChainInit(DMATxDscrTab, MACTxBuf, ETH_TXBUFNB);
    ETH_DMARxDescChainInit(DMARxDscrTab, MACRxBuf, ETH_RXBUFNB);
    pDMARxSet = DMARxDscrTab;
    pDMATxSet = DMATxDscrTab;
    NVIC_EnableIRQ(ETH_IRQn);
}


/******************************** endfile @ eth_driver ******************************/
