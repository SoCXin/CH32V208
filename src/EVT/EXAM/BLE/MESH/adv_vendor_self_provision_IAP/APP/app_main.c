/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2019/11/05
 * Description        : �жϱ�־�Լ����˴��뵽APP������
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* Header file contains */
#include "debug.h"
#include "app.h"
void dbg_printf(const char* format, ...);

/* ��¼��ǰ��Image */
unsigned char CurrImageFlag = 0xff;

/* flash��������ʱ�洢 */
__attribute__((aligned(8))) uint8_t block_buf[256];

#define jumpApp    ((void (*)(void))((int *)(IMAGE_A_START_ADD-0x08000000)))

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * @fn      FLASH_read
 *
 * @brief   �� flash
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
 * @fn      SwitchImageFlag
 *
 * @brief   �л�dataflash���ImageFlag
 *
 * @param   new_flag    - �л���ImageFlag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    uint16_t i;
    uint32_t ver_flag;

    /* ��ȡ��һ�� */
    FLASH_read(OTA_DATAFLASH_ADD, &block_buf[0], 4);

    FLASH_Unlock_Fast();
    /* ������һ�� */
    FLASH_ErasePage_Fast( OTA_DATAFLASH_ADD );

    /* ����Image��Ϣ */
    block_buf[0] = new_flag;
    block_buf[1] = 0x5A;
    block_buf[2] = 0x5A;
    block_buf[3] = 0x5A;

    /* ���DataFlash */
    FLASH_ProgramPage_Fast( OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0]);
    FLASH_Lock_Fast();
}

/*********************************************************************
 * @fn      jump_APP
 *
 * @brief   �л�APP����
 *
 * @return  none
 */
void jump_APP(void)
{
    if(CurrImageFlag == IMAGE_IAP_FLAG)
    {
        __attribute__((aligned(8))) uint8_t flash_Data[4096];
        uint8_t i;
        uint8_t page_count;

        FLASH_Unlock();
        FLASH_Unlock_Fast();
        for(i = 0; i < IMAGE_A_SIZE / 4096; i++)
        {
            FLASH_ErasePage(IMAGE_A_START_ADD + (i * 4096));
            FLASH_read(IMAGE_B_START_ADD + (i * 4096), flash_Data, 4096);
            {
                for(page_count = 0; page_count < 4096 / 256; page_count++)
                {
                    FLASH_ProgramPage_Fast(IMAGE_A_START_ADD + (i * 4096)+(page_count*256), (uint32_t *)&flash_Data[page_count*256]);
                }
            }
        }
        SwitchImageFlag(IMAGE_A_FLAG);
        // ���ٱ��ݴ���
        for(i = 0; i < IMAGE_A_SIZE / 4096; i++)
        {
            FLASH_ErasePage(IMAGE_B_START_ADD + (i * 4096));
        }
        FLASH_Lock();
        FLASH_Lock_Fast();
    }
    jumpApp();
}

/*********************************************************************
 * @fn      ReadImageFlag
 *
 * @brief   ��ȡ��ǰ�ĳ����Image��־��DataFlash���Ϊ�գ���Ĭ����ImageA
 *
 * @return  none
 */
void ReadImageFlag(void)
{
    OTADataFlashInfo_t p_image_flash;

    FLASH_read(OTA_DATAFLASH_ADD, (uint8_t *)&p_image_flash, 4);
    CurrImageFlag = p_image_flash.ImageFlag;

    /* �����һ��ִ�У�����û�и��¹����Ժ���º��ڲ���DataFlash */
    if((p_image_flash.flag[0]!=0x5A)||(p_image_flash.flag[1]!=0x5A)||(p_image_flash.flag[2]!=0x5A))
    {
        CurrImageFlag = IMAGE_A_FLAG;
    }
    dbg_printf("Image Flag %02x\n", CurrImageFlag);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main function
 *
 * @return  none
 */
int main(void)
{
    Delay_Init();
#ifdef DEBUG
    USART_Printf_Init( 115200 );
#endif
    ReadImageFlag();
    jump_APP();
}

/******************************** endfile @ main ******************************/
