/********************************** (C) COPYRIGHT *******************************
* File Name          : Udisk_Func_longname.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/11/22
* Description        : USB full-speed port host operation functions.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*******************************************************************************/
/* Header File */
#include "Udisk_Operation.h"

/*******************************************************************************/
/* Variable Definition */
uint8_t    LongNameBuf[ LONG_NAME_BUF_LEN ];
/*
���ļ���ʾ��(UNICODE����Ĵ�С�� ������UNICODE_ENDIAN������ͬ)
������LongName���������:
�������ļ������������������� 1.����(unicode ���)���ַ���ĩβ������0��ʾ����;2.ANSI������ļ���.TXT
*/
uint8_t LongName[ ] =
#if UNICODE_ENDIAN == 1
{
    0x5E, 0xFA, 0x7A, 0xCB, 0x95, 0x7F, 0x65, 0x87, 0x4E, 0xF6, 0x54, 0x0D, 0xFF, 0x0C, 0x8F, 0x93,
    0x51, 0x65, 0x4E, 0x24, 0x4E, 0x2A, 0x53, 0xC2, 0x65, 0x70, 0xFF, 0x1A, 0x00, 0x20, 0x00, 0x31,
    0x00, 0x2E, 0x91, 0xC7, 0x75, 0x28, 0x00, 0x28, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x69, 0x00, 0x63,
    0x00, 0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x20, 0x59, 0x27, 0x7A, 0xEF, 0x00, 0x29, 0xFF, 0x0C,
    0x5B, 0x57, 0x7B, 0x26, 0x4E, 0x32, 0x67, 0x2B, 0x5C, 0x3E, 0x75, 0x28, 0x4E, 0x24, 0x4E, 0x2A,
    0x00, 0x30, 0x88, 0x68, 0x79, 0x3A, 0x7E, 0xD3, 0x67, 0x5F, 0x00, 0x3B, 0x00, 0x32, 0x00, 0x2E,
    0x00, 0x41, 0x00, 0x4E, 0x00, 0x53, 0x00, 0x49, 0x7F, 0x16, 0x78, 0x01, 0x77, 0xED, 0x65, 0x87,
    0x4E, 0xF6, 0x54, 0x0D, 0x00, 0x2E, 0x00, 0x54, 0x00, 0x58, 0x00, 0x54
};
#else
{
    0xFA, 0x5E, 0xCB, 0x7A, 0x7F, 0x95, 0x87, 0x65, 0xF6, 0x4E, 0x0D, 0x54, 0x0C, 0xFF, 0x93, 0x8F,
    0x65, 0x51, 0x24, 0x4E, 0x2A, 0x4E, 0xC2, 0x53, 0x70, 0x65, 0x1A, 0xFF, 0x20, 0x00, 0x31, 0x00,
    0x2E, 0x00, 0xC7, 0x91, 0x28, 0x75, 0x28, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x69, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x20, 0x00, 0x27, 0x59, 0xEF, 0x7A, 0x29, 0x00, 0x0C, 0xFF,
    0x57, 0x5B, 0x26, 0x7B, 0x32, 0x4E, 0x2B, 0x67, 0x3E, 0x5C, 0x28, 0x75, 0x24, 0x4E, 0x2A, 0x4E,
    0x30, 0x00, 0x68, 0x88, 0x3A, 0x79, 0xD3, 0x7E, 0x5F, 0x67, 0x3B, 0x00, 0x32, 0x00, 0x2E, 0x00,
    0x41, 0x00, 0x4E, 0x00, 0x53, 0x00, 0x49, 0x00, 0x16, 0x7F, 0x01, 0x78, 0xED, 0x77, 0x87, 0x65,
    0xF6, 0x4E, 0x0D, 0x54, 0x2E, 0x00, 0x54, 0x00, 0x58, 0x00, 0x54, 0x00
};
#endif

/*********************************************************************
 * @fn      UDisk_USBH_Longname
 *
 * @brief   Demo Function For UDisk long-name Operation(EXAM 13)
 *
 * @return  none
 */
void UDisk_USBH_Longname( void )
{
    uint8_t  ret, i, len;
    uint16_t j;

    ret = UDisk_USBH_DiskReady( );
    if( ( ret == DISK_READY )&&( UDisk_Opeation_Flag == 1 ) )
    {
        UDisk_Opeation_Flag = 0;
        /*==================== ������ʾ��������ȡ���ļ��� ============================*/
        // ���Ƴ��ļ���(UNICODE ���)��LongNameBuf��
        len = LongName_Len;
        memcpy( LongNameBuf, LongName, len );
        // ĩβ������0��ʾ����
        LongNameBuf[len] = 0x00;
        LongNameBuf[len + 1] = 0x00;
        // �ó��ļ�����ANSI������ļ���(8+3��ʽ)
        strcpy( mCmdParam.Create.mPathName, "\\���ļ���.TXT" );
        i = CHRV3CreateLongName( );
        if( i == ERR_SUCCESS )
        {
            DUG_PRINTF( "Created Long Name OK!\r\n" );
        }
        else
        {
            /* Error code defined in "udisk_operaion.h" */
            DUG_PRINTF( "Error Code: %02X\r\n", (uint16_t)i );
        }

        DUG_PRINTF( "Get long Name#\r\n" );
        strcpy( mCmdParam.Open.mPathName, "\\���ļ���.TXT" );
        // ������Ҫ�����ļ���������·��
        i = CHRV3GetLongName( );
        if( i == ERR_SUCCESS )
        {
            // ���ļ����ռ����,��UNICODE���뷽ʽ(��UNICODE_ENDIAN����)
            // �����LongNameBuf������,���ļ������������0����.
            // ������ʾ����������������
            DUG_PRINTF( "LongNameBuf: " );
            for( j=0; j!=LONG_NAME_BUF_LEN; j++ )
            {
                DUG_PRINTF( "%02X ", (uint16_t)LongNameBuf[j] );
            }
            DUG_PRINTF( "\r\n" );
        }
        else
        {
            /* Error code defined in "udisk_operaion.h" */
            DUG_PRINTF( "Error Code: %02X\r\n", (uint16_t)i );
        }
    }
}

/*********************************************************************
 * @fn      CheckNameSum
 *
 * @brief   ��鳤�ļ����Ķ��ļ��������
 *
 * @return  ������У���
 */
uint8_t CheckNameSum( uint8_t *p )
{
uint8_t FcbNameLen;
uint8_t Sum;

    Sum = 0;
    for (FcbNameLen=0; FcbNameLen!=11; FcbNameLen++)
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *p++;
    return Sum;
}

/*********************************************************************
 * @fn      AnalyzeLongName
 *
 * @brief   �����ļ��� �����м�����26����
 *
 * @return  �����ж��ٸ�26�ĳ���
 */
uint8_t AnalyzeLongName( void )
{
uint8_t   i, j;
uint16_t  index;

    i = FALSE;
    for( index=0; index!=LONG_NAME_BUF_LEN; index = index + 2 )
    {
        if( ( LongNameBuf[index] == 0 ) && ( LongNameBuf[index+1] == 0 ) )
        {
            i = TRUE;
            break;
        }
    }
    if( ( i == FALSE ) || ( index == 0) )
        return 0;                   // ����0��ʾ����ĳ��ļ���

    i = index % 26;
    if( i != 0 )
    {
        index += 2;
        if( index % 26 != 0 )       // ��0�պý���
        {
            for( j=i+2; j!=26; j++ )// ��ʣ��������Ϊ0XFF
                LongNameBuf[index++] = 0xff;
        }
    }
    return  (index / 26);
}

/*********************************************************************
 * @fn      CHRV3CreateLongName
 *
 * @brief   �������ļ���,��Ҫ������ļ���������·��
 *
 * @return  ����״̬
 */
uint8_t CHRV3CreateLongName( void )
{
// ���� �����ļ�·�� ����һ�����ļ� �õ�FDTƫ�ƺ����������� ɾ���ļ�
// ���ƫ������ ����ʧ�� ��FAT12/16���ڸ�Ŀ¼�� �����Ϻ��ٴδ����ļ�
uint8_t   i;
uint8_t   len;                                // ���·���ĳ���
uint16_t  index;                              // ���ļ�ƫ������
uint16_t  indexBak;                           // ���ļ�ƫ����������
uint32_t  Secoffset;                          // ����ƫ��

uint8_t   Fbit;                               // ��һ�ν���д����
uint8_t   Mult;                               // ���ļ�������26�ı���
uint8_t   MultBak;                            // ���ļ�������26�ı�������

uint16_t  Backoffset;                         // �����ļ�ƫ�Ʊ���
uint16_t  BackoffsetBak;                      // ����ƫ�Ʊ��ݵı���
uint32_t  BackFdtSector;                      // ����ƫ����һ������
uint8_t   sum;                                // ���泤�ļ�����У���

uint8_t   BackPathBuf[MAX_PATH_LEN];    // �����ļ�·��

    Mult = AnalyzeLongName( );              // ���泤�ļ�����26�ı���
    if( Mult == 0 )
        return ERR_LONG_NAME;
    MultBak = Mult;

    i = CHRV3FileOpen();                    // ���ļ��������򷵻ش���
    if( i == ERR_SUCCESS )
        return ERR_NAME_EXIST;

    i = CHRV3FileCreate( );
    if( i == ERR_SUCCESS )
    {
        Backoffset = CHRV3vFdtOffset;
        BackoffsetBak = Backoffset;
        BackFdtSector = CHRV3vFdtLba;
        sum = CheckNameSum( &DISK_BASE_BUF[Backoffset ] );
        for( i=0; i!=MAX_PATH_LEN; i++ )    // ���ļ�·�����б���
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        CHRV3FileErase( );                  // ɾ�����ļ�

        Secoffset   = 0;                    // ��0��ʼƫ��
        index       = Mult*26;              // �õ����ļ����ĳ���
        indexBak    = index;
        Fbit        = FALSE;                // Ĭ��û�н���
        // ���ϼ� ���������������
        P_RETRY:
        for(len=0; len!=MAX_PATH_LEN; len++)
        {
            if(mCmdParam.Open.mPathName[len] == 0)
                break;                      // �õ��ַ�������
        }

        for(i=len-1; i!=0xff; i--)          // �õ��ϼ�Ŀ¼λ��
        {
            if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
                break;
        }
        mCmdParam.Open.mPathName[i] = 0x00;

        if( i==0 )                          // ��һ��Ŀ¼ע��:���ڸ�Ŀ¼��ʼ���������
        {
            mCmdParam.Open.mPathName[0] = '/';
            mCmdParam.Open.mPathName[1] = 0;
        }

        i = CHRV3FileOpen();                // ���ϼ�Ŀ¼
        if( i == ERR_OPEN_DIR )
        {
            while( 1 )                      // ѭ����д ֱ�����
            {
                mCmdParam.Locate.mSectorOffset = Secoffset;
                i = CHRV3FileLocate( );
                if( i == ERR_SUCCESS )
                {
                    if( Fbit )             // �ڶ��ν����д����
                    {
                        if( mCmdParam.Locate.mSectorOffset != 0x0FFFFFFFF )
                        {
                            BackFdtSector = mCmdParam.Locate.mSectorOffset;
                            Backoffset = 0;
                        }
                        else
                        {
                            for( i=0; i!=MAX_PATH_LEN; i++ )// ��ԭ�ļ�·��
                                mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                            i = CHRV3FileCreate( );         // ���пռ���չ
                            if( i != ERR_SUCCESS )
                                return i;
                            CHRV3FileErase( );
                            goto P_RETRY;                   // ���´��ϼ�Ŀ¼
                        }
                    }

                    if( BackFdtSector == mCmdParam.Locate.mSectorOffset )
                    {
                        mCmdParam.Read.mSectorCount = 1;   // ��һ�����������̻�����
                        mCmdParam.Read.mDataBuffer = &DISK_BASE_BUF[0];
                        i = CHRV3FileRead( );
                        CHRV3DirtyBuffer( );                // ������̻�����
                        if( i!= ERR_SUCCESS )
                            return i;

                        i = ( CHRV3vSectorSize - Backoffset ) / 32;
                        if( Mult > i )
                            Mult = Mult - i;                // ʣ��ı���
                        else
                        {
                            i = Mult;
                            Mult = 0;
                        }

                        for( len=i; len!=0; len-- )
                        {
                            indexBak -= 26;
                            index = indexBak;
                            for( i=0; i!=5; i++)            // ���ļ�����1-5���ַ�
                            {                               // �ڴ�����UNICODE��С�˷�ʽ���
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + i*2 + 2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + i*2 + 2 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            for( i =0; i!=6; i++)           // ���ļ�����6-11���ַ�
                            {
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + 14 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 14 + i*2 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + 14 + i*2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 14 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            for( i=0; i!=2; i++)            // ���ļ�����12-13���ַ�
                            {
                                #if UNICODE_ENDIAN == 1
                                DISK_BASE_BUF[Backoffset + 28 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 28 + i*2 ] =
                                    LongNameBuf[index++];
                                #else
                                DISK_BASE_BUF[Backoffset + 28 + i*2 ] =
                                    LongNameBuf[index++];
                                DISK_BASE_BUF[Backoffset + 28 + i*2 + 1 ] =
                                    LongNameBuf[index++];
                                #endif
                            }

                            DISK_BASE_BUF[Backoffset + 0x0b] = 0x0f;
                            DISK_BASE_BUF[Backoffset + 0x0c] = 0x00;
                            DISK_BASE_BUF[Backoffset + 0x0d] = sum;
                            DISK_BASE_BUF[Backoffset + 0x1a] = 0x00;
                            DISK_BASE_BUF[Backoffset + 0x1b] = 0x00;
                            DISK_BASE_BUF[Backoffset] = MultBak--;
                            Backoffset += 32;
                        }

                        if( !Fbit )
                        {
                            Fbit = TRUE;
                            DISK_BASE_BUF[ BackoffsetBak ] |= 0x40;
                        }
                        CHRV3vLbaCurrent = BackFdtSector;
                        i = CHRV3WriteSector( 1, DISK_BASE_BUF );
                        if( i!= ERR_SUCCESS )
                            return i;

                        if( Mult==0 )
                        {   // ��ԭ�ļ�·��
					        CHRV3FileClose( );
                            for( i=0; i!=MAX_PATH_LEN; i++ )
                                mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                            i = CHRV3FileCreate( );
                            return i;
                        }
                    }
                }
                else
                    return i;
                Secoffset++;
            }
        }
    }
    return i;
}

/*********************************************************************
 * @fn      GetUpSectorData
 *
 * @brief   �ɵ�ǰ�����õ���һ�����������ݣ����ڴ��̻�����
 *
 * @return  ����״̬
 */
uint8_t GetUpSectorData( uint32_t *NowSector )
{
uint8_t  i;
uint8_t  len;             // ���·���ĳ���
uint32_t index;           // Ŀ¼����ƫ��������

    index = 0;
    for(len=0; len!=MAX_PATH_LEN; len++)
    {
        if(mCmdParam.Open.mPathName[len] == 0)          // �õ��ַ�������
            break;
    }

    for(i=len-1; i!=0xff; i--)                          // �õ��ϼ�Ŀ¼λ��
    {
        if((mCmdParam.Open.mPathName[i] == '\\') || (mCmdParam.Open.mPathName[i] == '/'))
            break;
    }
    mCmdParam.Open.mPathName[i] = 0x00;

    if( i==0 )  // ��һ��Ŀ¼ע��:���ڸ�Ŀ¼��ʼ���������
    {
        mCmdParam.Open.mPathName[0] = '/';
        mCmdParam.Open.mPathName[1] = 0;
        i = CHRV3FileOpen();
        if ( i == ERR_OPEN_DIR )
            goto P_NEXT0;
    }
    else
    {
        i = CHRV3FileOpen();
        if ( i == ERR_OPEN_DIR )
        {
            while( 1 )
            {
                P_NEXT0:
                mCmdParam.Locate.mSectorOffset = index;
                i = CHRV3FileLocate( );
                if( i == ERR_SUCCESS )
                {
                    if( *NowSector == mCmdParam.Locate.mSectorOffset )
                    {
                        if( index==0 )                          // ���ڸ�Ŀ¼�����Ŀ�ʼ
                            return ERR_NO_NAME;
                        mCmdParam.Locate.mSectorOffset = --index;
                        i = CHRV3FileLocate( );                 // ����һ������������
                        if( i == ERR_SUCCESS )
                        {                                       // ���±��浱ǰ����������
                            *NowSector = mCmdParam.Locate.mSectorOffset;
                            mCmdParam.Read.mSectorCount = 1;   // ��һ�����������̻�����
                            mCmdParam.Read.mDataBuffer = &DISK_BASE_BUF[0];
                            i = CHRV3FileRead( );
                            CHRV3DirtyBuffer( );                // ������̻�����
                            return i;
                        }
                        else
                            return i;
                    }
                }
                else
                    return i;
                index++;
            }
        }
    }
    return i;
}

/*********************************************************************
 * @fn      CHRV3GetLongName
 *
 * @brief   ���������ļ���·��(�������ļ����ļ���)�õ���Ӧ�ĳ��ļ���
 *
 * @return  ����״̬
 */
uint8_t CHRV3GetLongName( void )
{
// ��Ҫ����������С
// ��һ�������ļ��Ƿ��ҵ��ļ�,�����ļ��Ƿ����,���õ�FDT�ڴ�������ƫ�ƺ���������
// �ڶ����������������Ϣ���Ƿ��г��ļ������ڣ��Ƿ���Ŀ¼�ĵ�һ�������Ŀ�ʼ
// ��������ʵ�����ƫ��һ������?��ȡ���ļ���(����512�ֽڵ�U��)
uint8_t   i;
uint16_t  index;          // �ڳ��ļ����������ڵ�����
uint32_t  BackFdtSector;  // ����ƫ����һ������
uint8_t   sum;            // ���泤�ļ�����У���
//uint16_t  Backoffset;     // �����ļ�ƫ�Ʊ���
uint16_t  offset;         // �������ļ�ƫ��32����
uint8_t   FirstBit;       // ���ļ�����Խ����������־λ
uint8_t   BackPathBuf[MAX_PATH_LEN]; // �����ļ�·��

    i = CHRV3FileOpen( );
    if( ( i == ERR_SUCCESS ) || ( i == ERR_OPEN_DIR ) )
    {
        for( i=0; i!=MAX_PATH_LEN; i++ )
            BackPathBuf[i] = mCmdParam.Open.mPathName[i];
        // ������ɶ�·���ı���

        sum = CheckNameSum( &DISK_BASE_BUF[CHRV3vFdtOffset] );
        index = 0;
        FirstBit = FALSE;
//        Backoffset = CHRV3vFdtOffset;
        BackFdtSector = CHRV3vFdtLba;
        if( CHRV3vFdtOffset == 0 )
        {
            // ���ж��Ƿ���һ��������ʼ �Ƿ��ڸ�Ŀ¼��ʼ ���������ƫ��
            if( FirstBit == FALSE )
                FirstBit = TRUE;
            i = GetUpSectorData( &BackFdtSector );
            if( i == ERR_SUCCESS )
            {
                CHRV3vFdtOffset = CHRV3vSectorSize;
                goto P_NEXT1;
            }
        }
        else
        {
            // ��ȡƫ�ƺ�����ݣ�ֱ��������������������ƫ��
            P_NEXT1:
            offset = CHRV3vFdtOffset;
            while( 1 )
            {
                if( offset != 0 )
                {
                    offset = offset - 32;
                    if( ( DISK_BASE_BUF[offset + 11] == ATTR_LONG_NAME )
                        && (  DISK_BASE_BUF[offset + 13] == sum ) )
                    {
                        if( (index + 26) > LONG_NAME_BUF_LEN )
                            return ERR_BUF_OVER;

                        for( i=0; i!=5; i++)            // ���ļ�����1-5���ַ�
                        {                               // �ڴ�����UNICODE��С�˷�ʽ���
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 2];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 1];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + i*2 + 2];
                            #endif
                        }

                        for( i =0; i!=6; i++)           // ���ļ�����6-11���ַ�
                        {
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 14 + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + + 14 + i*2 ];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + + 14 + i*2 ];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 14 + i*2 + 1];
                            #endif

                        }

                        for( i=0; i!=2; i++)            // ���ļ�����12-13���ַ�
                        {
                            #if UNICODE_ENDIAN == 1
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 + 1];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 ];
                            #else
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 ];
                            LongNameBuf[index++] =
                                DISK_BASE_BUF[offset + 28 + i*2 + 1];
                            #endif
                        }

                        if( DISK_BASE_BUF[offset] & 0X40 )
                        {
                            if( ! (((LongNameBuf[index -1] ==0x00)
                                && (LongNameBuf[index -2] ==0x00))
                                || ((LongNameBuf[index -1] ==0xFF)
                                && (LongNameBuf[index -2 ] ==0xFF))))
                            {                           // ����պ�Ϊ26�ֽڳ��������ļ���
                                if(index + 52 >LONG_NAME_BUF_LEN )
                                    return ERR_BUF_OVER;
                                LongNameBuf[ index ] = 0x00;
                                LongNameBuf[ index + 1] = 0x00;
                            }
                            return ERR_SUCCESS;         // �ɹ���ɳ��ļ����ռ����
                        }
                    }
                    else
                        return ERR_NO_NAME;             // ����ĳ��ļ���,���򷵻�
                }
                else
                {
                    if( FirstBit == FALSE )
                        FirstBit = TRUE;
                    else                                // ����ڶ��ν���
                    {
                        for( i=0; i!=MAX_PATH_LEN; i++ )// ��ԭ·��
                            mCmdParam.Open.mPathName[i] = BackPathBuf[i];
                    }
                    i = GetUpSectorData( &BackFdtSector );
                    if( i == ERR_SUCCESS )
                    {
                        CHRV3vFdtOffset = CHRV3vSectorSize;
                        goto P_NEXT1;
                    }
                    else
                        return i;
                    // ���ƫ������
                }
            }
        }
    }
    return i;                // ���ش���
}

