/********************************** (C) COPYRIGHT *******************************
 * File Name          : Peripheral.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : Peripheral slave multi-connection application, initialize 
 *                      broadcast connection parameters, then broadcast, after connecting 
 *                      to the host, request to update connection parameters, 
 *                      and transmit data through custom services.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "GATTprofile.h"
#include "Peripheral.h"
#include "OTA.h"
#include "OTAprofile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
#define SBP_PERIODIC_EVT_PERIOD              1000

// What is the advertising interval when device is discoverable (units of 625us, 32=20ms)
#define DEFAULT_ADVERTISING_INTERVAL         32

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#define DEFAULT_DISCOVERABLE_MODE            GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 6=7.5ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    6

// Maximum connection interval (units of 1.25ms, 12=15ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    12

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST        TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL        6

// Company Identifier: WCH
#define WCH_COMPANY_ID                       0x07D7

#define INVALID_CONNHANDLE                   0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN                       15

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t Peripheral_TaskID = INVALID_TASK_ID; // Task ID for internal task/event processing

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[31] = {
    // complete name
    0x12, // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'T', 'A', 'O', 'T', 'A', '_', 'O', 'T', 'A', 'O', 'T', 'A', '_', 'O', 'T', 'A',
    // connection interval range
    0x05, // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL), // 100ms
    HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL), // 1s
    HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

    // Tx power level
    0x02, // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0 // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t advertData[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // service UUID, to notify central devices what services are included
    // in this peripheral
    0x03,                  // length of this data
    GAP_ADTYPE_16BIT_MORE, // some of the UUID's, but not all
    LO_UINT16(SIMPLEPROFILE_SERV_UUID),
    HI_UINT16(SIMPLEPROFILE_SERV_UUID)

};

// GAP GATT Attributes
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "OTAOTA_OTAOTA_OTA";

// OTA IAP VARIABLES
/* OTA communication frame */
OTA_IAP_CMD_t iap_rec_data;

/* OTA analysis results */
uint32_t OpParaDataLen = 0;
uint32_t OpAdd = 0;
uint16_t block_buf_len=0;
uint32_t prom_addr=0;

/* Flash data temporary storage */
__attribute__((aligned(8))) uint8_t block_buf[512];

/* IMAGE jump function address definition */
typedef int (*pImageTaskFn)(void);
pImageTaskFn user_image_tasks;

/* Flash erase */
uint32_t EraseAdd = 0;      //Removal address
uint32_t EraseBlockNum = 0; //Number of blocks that need to be erased
uint32_t EraseBlockCnt = 0; //Scratching block count

/* FLASH verification status */
uint8_t VerifyStatus = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void performPeriodicTask(void);

void OTA_IAPReadDataComplete(unsigned char index);
void OTA_IAPWriteData(unsigned char index, unsigned char *p_data, unsigned char w_len);
void Rec_OTA_IAP_DataDeal(void);
void OTA_IAP_SendCMDDealSta(uint8_t deal_status);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t Peripheral_PeripheralCBs = {
    peripheralStateNotificationCB, // Profile State Change Callbacks
    NULL,                          // When a valid RSSI is read from controller (not used by application)
    NULL};

// GAP Bond Manager Callbacks
static gapBondCBs_t Peripheral_BondMgrCBs = {
    NULL, // Passcode callback (not used by application)
    NULL  // Pairing / Bonding state Callback (not used by application)
};

// Simple GATT Profile Callbacks
static OTAProfileCBs_t Peripheral_OTA_IAPProfileCBs = {
    OTA_IAPReadDataComplete, // Charactersitic value change callback
    OTA_IAPWriteData};

// Callback when the connection parameteres are updated.
void PeripheralParamUpdate(uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout);

gapRolesParamUpdateCB_t PeripheralParamUpdate_t = NULL;

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Peripheral_Init
 *
 * @brief   Initialization function for the Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Peripheral_Init()
{
    Peripheral_TaskID = TMOS_ProcessEventRegister(Peripheral_ProcessEvent);

    // Setup the GAP Peripheral Role Profile
    {
        // For other hardware platforms, device starts advertising upon initialization
        uint8_t initial_advertising_enable = TRUE;

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
    }

    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);

    // Set advertising interval
    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    OTAProfile_AddService(GATT_ALL_SERVICES);

    //  Register callback with OTAGATTprofile
    OTAProfile_RegisterAppCBs(&Peripheral_OTA_IAPProfileCBs);

    // Setup a delayed profile startup
    tmos_set_event(Peripheral_TaskID, SBP_START_DEVICE_EVT);
}

void PeripheralParamUpdate(uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{
    PRINT("update %d %d %d \n", connInterval, connSlaveLatency, connTimeout);

    //		GAPRole_SendUpdateParam( DEFAULT_DESIRED_MIN_CONN_INTERVAL, DEFAULT_DESIRED_MAX_CONN_INTERVAL,
    //                                 DEFAULT_DESIRED_SLAVE_LATENCY, DEFAULT_DESIRED_CONN_TIMEOUT, GAPROLE_NO_ACTION );
}

/*********************************************************************
 * @fn      Peripheral_ProcessEvent
 *
 * @brief   Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Peripheral_ProcessEvent(uint8_t task_id, uint16_t events)
{
    //  VOID task_id; // TMOS required parameter that isn't used in this function

    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(Peripheral_TaskID)) != NULL)
        {
            Peripheral_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & SBP_START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(Peripheral_TaskID, &Peripheral_BondMgrCBs, &Peripheral_PeripheralCBs);
        // Set timer for first periodic event
        tmos_start_task(Peripheral_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD);
        return (events ^ SBP_START_DEVICE_EVT);
    }

    if(events & SBP_PERIODIC_EVT)
    {
        // Restart timer
        if(SBP_PERIODIC_EVT_PERIOD)
        {
            tmos_start_task(Peripheral_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD);
        }
        // Perform periodic application task
        performPeriodicTask();
        return (events ^ SBP_PERIODIC_EVT);
    }

    //OTA_FLASH_ERASE_EVT
    if(events & OTA_FLASH_ERASE_EVT)
    {
        uint8_t status;

        PRINT("ERASE:%08x num:%d\r\n", (int)(EraseAdd + EraseBlockCnt * FLASH_BLOCK_SIZE), (int)EraseBlockCnt);
        FLASH_Unlock();
        status = FLASH_ErasePage(EraseAdd + EraseBlockCnt * FLASH_BLOCK_SIZE);
        FLASH_Lock();

        /* Erase failed */
        if(status != FLASH_COMPLETE)
        {
            OTA_IAP_SendCMDDealSta(status);
            return (events ^ OTA_FLASH_ERASE_EVT);
        }

        EraseBlockCnt++;

        /* End of erasing */
        if(EraseBlockCnt >= EraseBlockNum)
        {
            PRINT("ERASE Complete\r\n");
            OTA_IAP_SendCMDDealSta(SUCCESS);
            return (events ^ OTA_FLASH_ERASE_EVT);
        }

        return (events);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Peripheral_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:
            break;
    }
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            PRINT("Initialized..\n");
            break;

        case GAPROLE_ADVERTISING:
            PRINT("Advertising..\n");
            break;

        case GAPROLE_CONNECTED:
        {
            gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
            uint16_t              conn_interval = 0;

            conn_interval = event->connInterval;
            PRINT("Connected.. \n");

            if(conn_interval > DEFAULT_DESIRED_MAX_CONN_INTERVAL)
            {
                PRINT("Send Update\r\n");
                GAPRole_PeripheralConnParamUpdateReq(event->connectionHandle,
                                                     DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                                     DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                                     DEFAULT_DESIRED_SLAVE_LATENCY,
                                                     DEFAULT_DESIRED_CONN_TIMEOUT,
                                                     Peripheral_TaskID);
            }
            break;
        }
        case GAPROLE_CONNECTED_ADV:
            PRINT("Connected Advertising..\n");
            break;
        case GAPROLE_WAITING:
        {
            uint8_t initial_advertising_enable = TRUE;

            // Set the GAP Role Parameters
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
            PRINT("Disconnected..\n");
        }
        break;

        case GAPROLE_ERROR:
            PRINT("Error..\n");
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      performPeriodicTask
 *
 * @brief   Perform a periodic application task. This function gets
 *          called every five seconds as a result of the SBP_PERIODIC_EVT
 *          TMOS event. In this example, the value of the third
 *          characteristic in the SimpleGATTProfile service is retrieved
 *          from the profile, and then copied into the value of the
 *          the fourth characteristic.
 *
 * @param   none
 *
 * @return  none
 */
static void performPeriodicTask(void)
{
}

/*********************************************************************
 * @fn      OTA_IAP_SendData
 *
 * @brief   OTA IAP sends data, limits within 20 bytes when used
 *
 * @param   p_send_data - Poems of sending data
 * @param   send_len    - Send data length
 *
 * @return  none
 */
void OTA_IAP_SendData(uint8_t *p_send_data, uint8_t send_len)
{
    OTAProfile_SendData(OTAPROFILE_CHAR, p_send_data, send_len);
}

/*********************************************************************
 * @fn      OTA_IAP_SendCMDDealSta
 *
 * @brief   OTA IAP execution status returns
 *
 * @param   deal_status - Return state
 *
 * @return  none
 */
void OTA_IAP_SendCMDDealSta(uint8_t deal_status)
{
    uint8_t send_buf[2];

    send_buf[0] = deal_status;
    send_buf[1] = 0;
    OTA_IAP_SendData(send_buf, 2);
}

/*********************************************************************
 * @fn      OTA_IAP_CMDErrDeal
 *
 * @brief   OTA IAP abnormal command code processing
 *
 * @return  none
 */
void OTA_IAP_CMDErrDeal(void)
{
    OTA_IAP_SendCMDDealSta(0xfe);
}

/*********************************************************************
 * @fn      SwitchImageFlag
 *
 * @brief   Switch the ImageFlag in DataFlash
 *
 * @param   new_flag    - Switching ImageFlag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    uint16_t i;
    uint32_t ver_flag;

    /* Read the first block */
    FLASH_read(OTA_DATAFLASH_ADD, &block_buf[0], 4);

    FLASH_Unlock_Fast();
    /* Erase the first block */
    FLASH_ErasePage_Fast( OTA_DATAFLASH_ADD );

    /* Update Image information */
    block_buf[0] = new_flag;
    block_buf[1] = 0x5A;
    block_buf[2] = 0x5A;
    block_buf[3] = 0x5A;

    /* Programming DataFlash */
    FLASH_ProgramPage_Fast( OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0]);
    FLASH_Lock_Fast();
}

/*********************************************************************
 * @fn      DisableAllIRQ
 *
 * @brief   Turn off all the interrupts
 *
 * @return  none
 */
void DisableAllIRQ(void)
{
    __disable_irq();
}

/*********************************************************************
 * @fn      Rec_OTA_IAP_DataDeal
 *
 * @brief   Receive OTA packet processing
 *
 * @return  none
 */
void Rec_OTA_IAP_DataDeal(void)
{
    switch(iap_rec_data.other.buf[0])
    {
        /* Programming */
        case CMD_IAP_PROM:
        {
            uint32_t i;
            uint8_t  status;

            OpParaDataLen = iap_rec_data.program.len;
            OpAdd = (uint32_t)(iap_rec_data.program.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.program.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            PRINT("IAP_PROM: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);

            /* Current is ImageA, programming directly */
            tmos_memcpy(&block_buf[block_buf_len], iap_rec_data.program.buf, OpParaDataLen);
            block_buf_len+=OpParaDataLen;
            if( block_buf_len>=FLASH_PAGE_SIZE )
            {
                FLASH_Unlock_Fast();
                FLASH_ProgramPage_Fast(prom_addr, (uint32_t*)block_buf);
                FLASH_Lock_Fast();
                tmos_memcpy(block_buf, &block_buf[FLASH_PAGE_SIZE], block_buf_len-FLASH_PAGE_SIZE);
                block_buf_len-=FLASH_PAGE_SIZE;
                prom_addr+=FLASH_PAGE_SIZE;
            }
            OTA_IAP_SendCMDDealSta(status);
            break;
        }
        /* Erase -- Bluetooth erase is controlled by the host */
        case CMD_IAP_ERASE:
        {
            OpAdd = (uint32_t)(iap_rec_data.erase.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.erase.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            OpAdd += 0x08000000;

            EraseBlockNum = (uint32_t)(iap_rec_data.erase.block_num[0]);
            EraseBlockNum |= ((uint32_t)(iap_rec_data.erase.block_num[1]) << 8);
            EraseAdd = OpAdd;
            EraseBlockCnt = 0;

            /* The inspection is placed in the era of clearing 0 */
            VerifyStatus = 0;

            prom_addr = IMAGE_A_START_ADD;
            PRINT("IAP_ERASE start:%08x num:%d\r\n", (int)OpAdd, (int)EraseBlockNum);

            if(EraseAdd < IMAGE_A_START_ADD || (EraseAdd + (EraseBlockNum - 1) * FLASH_BLOCK_SIZE) > (IMAGE_A_START_ADD+IMAGE_A_SIZE))
            {
                OTA_IAP_SendCMDDealSta(0xFF);
            }
            else
            {
                /* Modify DataFlash, switch to ImageB */
                SwitchImageFlag(IMAGE_IAP_FLAG);

                /* Start erasing */
                tmos_set_event(Peripheral_TaskID, OTA_FLASH_ERASE_EVT);
            }
            break;
        }
        /* Verify */
        case CMD_IAP_VERIFY:
        {
            uint32_t i;
            uint8_t  status = 0;
            uint8_t  verifyData[iap_rec_data.verify.len];

            if( block_buf_len )
            {
                FLASH_Unlock_Fast();
                FLASH_ProgramPage_Fast(prom_addr, (uint32_t*)block_buf);
                FLASH_Lock_Fast();
                block_buf_len=0;
                prom_addr=0;
            }

            OpParaDataLen = iap_rec_data.verify.len;

            OpAdd = (uint32_t)(iap_rec_data.verify.addr[0]);
            OpAdd |= ((uint32_t)(iap_rec_data.verify.addr[1]) << 8);
            OpAdd = OpAdd * 16;

            OpAdd += 0x08000000;
            PRINT("IAP_VERIFY: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);
            FLASH_read(OpAdd, verifyData, OpParaDataLen);
            /* It is currently ImageA, read the ImageB check directly */
            status = tmos_memcmp(verifyData, iap_rec_data.verify.buf, OpParaDataLen);
            if(status == FALSE)
            {
                PRINT("IAP_VERIFY err \r\n");
                VerifyStatus = 0xFF;
            }
            OTA_IAP_SendCMDDealSta(VerifyStatus);
            break;
        }
        /* End of rogramming */
        case CMD_IAP_END:
        {
            PRINT("IAP_END \r\n");

            /* Close all the current use interrupt, or it is convenient to directly close */
            DisableAllIRQ();

            /* Modify data flash, switch to ImageA */
            SwitchImageFlag(IMAGE_A_FLAG);

            /* Waiting for printing, jump to ImageB*/
            Delay_Ms(10);
            jumpApp();

            /* Will not execute here */
            NVIC_SystemReset();

            break;
        }
        case CMD_IAP_INFO:
        {
            uint8_t send_buf[20];

            PRINT("IAP_INFO \r\n");

            /* IMAGE FLAG */
            send_buf[0] = IMAGE_IAP_FLAG;

            /* IMAGE_IAP_START_ADD */
            send_buf[1] = (uint8_t)(IMAGE_IAP_START_ADD & 0xff);
            send_buf[2] = (uint8_t)((IMAGE_IAP_START_ADD >> 8) & 0xff);
            send_buf[3] = (uint8_t)((IMAGE_IAP_START_ADD >> 16) & 0xff);
            send_buf[4] = (uint8_t)((IMAGE_IAP_START_ADD >> 24) & 0xff);

            /* BLOCK SIZE */
            send_buf[5] = (uint8_t)(FLASH_BLOCK_SIZE & 0xff);
            send_buf[6] = (uint8_t)((FLASH_BLOCK_SIZE >> 8) & 0xff);

            send_buf[7] = CHIP_ID&0xFF;
            send_buf[8] = (CHIP_ID>>8)&0xFF;
            /* Add more if necessary */

            /* send message */
            OTA_IAP_SendData(send_buf, 20);

            break;
        }

        default:
        {
            OTA_IAP_CMDErrDeal();
            break;
        }
    }
}

/*********************************************************************
 * @fn      OTA_IAPReadDataComplete
 *
 * @brief   OTA data reading complete processing
 *
 * @param   index   - OTA channel serial number
 *
 * @return  none
 */
void OTA_IAPReadDataComplete(unsigned char index)
{
    PRINT("OTA Send Comp \r\n");
}

/*********************************************************************
 * @fn      OTA_IAPWriteData
 *
 * @brief   OTA channel data receiving complete processing
 *
 * @param   index   - OTA channel serial number
 * @param   p_data  - Written data
 * @param   w_len   - Length
 *
 * @return  none
 */
void OTA_IAPWriteData(unsigned char index, unsigned char *p_data, unsigned char w_len)
{
    unsigned char  rec_len;
    unsigned char *rec_data;

    rec_len = w_len;
    rec_data = p_data;
    tmos_memcpy((unsigned char *)&iap_rec_data, rec_data, rec_len);
    Rec_OTA_IAP_DataDeal();
}

/*********************************************************************
 * @fn      FLASH_read
 *
 * @brief   Read flash
 *
 * @return  none
 */
void FLASH_read(uint32_t addr, uint8_t *pData, uint32_t len)
{
    uint32_t i;
    for(i=0;i<len;i++)
    {
        *pData++ = *(uint8_t*)addr++;
    }
}

/*********************************************************************
*********************************************************************/
