
/********************************** (C) COPYRIGHT *******************************
* File Name          : HID_TP.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/05/09
* Description        : CH554ģ��HID Touch�豸�ϴ�����
                       ������Դ������I2C��������оƬGT911
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "CH554.H"
#include "DEBUG.H"
#include "FT5206.H"
#include "USB_DESC.H"


UINT8X  Ep0Buffer[8<(THIS_ENDP0_SIZE+2)?8:(THIS_ENDP0_SIZE+2)] _at_ 0x0000;        //�˵�0 OUT&IN��������������ż��ַ
UINT8X  Ep2Buffer[128<(2*MAX_PACKET_SIZE+4)?128:(2*MAX_PACKET_SIZE+4)] _at_ 0x0044;//�˵�2 IN&OUT������,������ż��ַ
UINT8   SetupReq,Ready,Count,FLAG,UsbConfig;
UINT16 SetupLen;
PUINT8  pDescr;                                                                    //USB���ñ�־
USB_SETUP_REQ   SetupReqBuf;                                                       //�ݴ�Setup��
UINT8 USB_Enum_OK;
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)  

#pragma  NOAREGS


UINT8X EP_UP_BUF[84] = {0};
UINT8X UserEp2Buf[64];                                            //�û����ݶ���

/*******************************************************************************
* Function Name  : USBDeviceInit()
* Description    : USB�豸ģʽ����,�豸ģʽ�������շ��˵����ã��жϿ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceInit()
{   
	UINT8 i;
	  IE_USB = 0;
	  USB_CTRL = 0x00;                                                           // ���趨USB�豸ģʽ
#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED;                                                //ѡ�����1.5Mģʽ
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //ѡ��ȫ��12Mģʽ��Ĭ�Ϸ�ʽ
#endif
    UEP2_DMA = Ep2Buffer;                                                      //�˵�2���ݴ����ַ
    UEP2_3_MOD |= bUEP2_TX_EN;                                                 //�˵�2����ʹ��
    UEP2_3_MOD |= bUEP2_RX_EN;                                                 //�˵�2����ʹ��
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;                                              //�˵�2�շ���64�ֽڻ�����
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //�˵�2�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK
    UEP0_DMA = Ep0Buffer;                                                      //�˵�0���ݴ����ַ
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                                //�˵�0��64�ֽ��շ�������
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //OUT���񷵻�ACK��IN���񷵻�NAK
		
	USB_DEV_AD = 0x00;
	UDEV_CTRL = bUD_PD_DIS;                                                    // ��ֹDP/DM��������
	USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                      // ����USB�豸��DMA�����ж��ڼ��жϱ�־δ���ǰ�Զ�����NAK
	UDEV_CTRL |= bUD_PORT_EN;                                                  // ����USB�˿�
	USB_INT_FG = 0xFF;                                                         // ���жϱ�־
	USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
	IE_USB = 1;
	
	for(i=0; i<64; i++)                                                   //׼����ʾ����
    {
        UserEp2Buf[0] = 0;
    }
	FLAG = 0;
    Ready = 0;
}

/*******************************************************************************
* Function Name  : Enp2BlukIn()
* Description    : USB�豸ģʽ�˵�2�������ϴ�
* Input          : UINT8X buf, UINT8 n 
* Output         : None
* Return         : None
*******************************************************************************/
static void Enp2BlukIn( UINT8 * buf, UINT8 n )
{
    memcpy( Ep2Buffer+MAX_PACKET_SIZE, buf, n);        //�����ϴ�����
    UEP2_T_LEN = n;                                              //�ϴ���������
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                  //������ʱ�ϴ����ݲ�Ӧ��ACK
    while(UEP2_CTRL&UEP_T_RES_ACK);                                            //�ȴ��������
}

/*******************************************************************************
* Function Name  : DeviceInterrupt
* Description    : CH559USB�жϴ�������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void    DeviceInterrupt( void ) interrupt INT_NO_USB using 1                    //USB�жϷ������,ʹ�üĴ�����1
{
    UINT8 i;
    UINT16 len;
    if(UIF_TRANSFER)                                                            //USB������ɱ�־
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# �˵������ϴ�
             UEP2_T_LEN = 0;                                                    //Ԥʹ�÷��ͳ���һ��Ҫ���
//            UEP1_CTRL ^= bUEP_T_TOG;                                          //����������Զ���ת����Ҫ�ֶ���ת
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Ĭ��Ӧ��NAK
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 2# �˵������´�
//             if ( U_TOG_OK )                                                     // ��ͬ�������ݰ�������
//             {
//                 len = USB_RX_LEN;                                               //�������ݳ��ȣ����ݴ�Ep2Buffer�׵�ַ��ʼ���
//                 for ( i = 0; i < len; i ++ )
//                 {
//                     Ep2Buffer[MAX_PACKET_SIZE+i] = Ep2Buffer[i] ^ 0xFF;         // OUT����ȡ����IN�ɼ������֤
//                 }
//                 UEP2_T_LEN = len;
//                 UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;       // �����ϴ�
//             }
            break;
        case UIS_TOKEN_SETUP | 0:                                               //SETUP����
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthH;
                SetupLen <<= 8;							
                SetupLen |= UsbSetupBuf->wLengthL;
                len = 0;                                                         // Ĭ��Ϊ�ɹ������ϴ�0����
                SetupReq = UsbSetupBuf->bRequest;							
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )/*HID������*/
                {
					switch( SetupReq )                                             
					{
						case 0x01:                                                   //GetReport				
							Ep0Buffer[0] = 0x02;
							Ep0Buffer[1] = 0x0A;										
							len = 2;
							break;
						case 0x02:                                                   //GetIdle
							break;	
						case 0x03:                                                   //GetProtocol
							break;				
						case 0x09:                                                   //SetReport										
							break;
						case 0x0A:                                                   //SetIdle
							break;	
						case 0x0B:                                                   //SetProtocol
							break;
						default:
							len = 0xFF;  				                                   /*���֧��*/					
							break;
				  }		
                }
                else                                                             //��׼����
                {
                    switch(SetupReq)                                             //������
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                  //�豸������
                            pDescr = DevDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                  //����������
                            pDescr = CfgDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(CfgDesc);
                            break;
                        case 0x22:                                               //����������
                            pDescr = HIDRepDesc;                                 //����׼���ϴ�
                            len = sizeof(HIDRepDesc);
                            Ready = 1;                                           //����и���ӿڣ��ñ�׼λӦ�������һ���ӿ�������ɺ���Ч
                            break;
						case 3:                                          // �ַ���������
							switch( UsbSetupBuf->wValueL ) {
								case 1:
									pDescr = (PUINT8)( &MyManuInfo[0] );
									len = sizeof( MyManuInfo );
									break;
								case 2:
									pDescr = (PUINT8)( &MyProdInfo[0] );
									len = sizeof( MyProdInfo );
									break;
								case 0:
									pDescr = (PUINT8)( &MyLangDescr[0] );
									len = sizeof( MyLangDescr );
									break;
								default:
									len = 0xFF;                               // ��֧�ֵ��ַ���������
									break;
							}
							break;												
                        default:
                            len = 0xff;                                          //��֧�ֵ�������߳���
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //�����ܳ���
                        }
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//���δ��䳤��
                        memcpy(Ep0Buffer,pDescr,len);                            //�����ϴ�����
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                         //�ݴ�USB�豸��ַ
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
						USB_Enum_OK = 1;
						EX0 = 1;
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                      //Clear Feature
                        if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// �˵�
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;                                       // ��֧�ֵĶ˵�
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                           // ���Ƕ˵㲻֧��
                        }
                        break;
                    case USB_SET_FEATURE:                                         /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )        /* �����豸 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* ���û���ʹ�ܱ�־ */
                                }
                                else
                                {
                                    len = 0xFF;                                    /* ����ʧ�� */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                        /* ����ʧ�� */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )    /* ���ö˵� */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* ���ö˵�2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF;                                     /* ����ʧ�� */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                         /* ����ʧ�� */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                             /* ����ʧ�� */
                        } 
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                  //����ʧ��
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                          //�����ȴ���
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= THIS_ENDP0_SIZE)                                         //�ϴ����ݻ���״̬�׶η���0���Ȱ�
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1������Ӧ��ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1,����Ӧ��ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;     //���δ��䳤��
                memcpy( Ep0Buffer, pDescr, len );                                   //�����ϴ�����
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                            //ͬ����־λ��ת
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ���
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            len = USB_RX_LEN;
            if(SetupReq == 0x09)
            {
							if ( U_TOG_OK )                                                     // ��ͬ�������ݰ�������
							{
									len = USB_RX_LEN;                                                 //�������ݳ��ȣ����ݴ�Ep1Buffer�׵�ַ��ʼ���
									for ( i = 0; i < len; i ++ )
									{
											UserEp2Buf[i] = Ep0Buffer[i] ^ 0xFF;		// OUT����ȡ����IN�ɼ������֤									
									}							
                  UEP0_T_LEN = 0;
                  UEP0_CTRL = (UEP0_CTRL ^ bUEP_R_TOG) | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1������Ӧ��ACK										
							}
            }
            UEP0_T_LEN = 0;  															//��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;									//Ĭ�����ݰ���DATA0,����Ӧ��ACK
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           	//д0����ж�
    }
    if(UIF_BUS_RST)                                                                 	//�豸ģʽUSB���߸�λ�ж�
    {
		EX0 = 0;
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                             	//���жϱ�־
    }
    if (UIF_SUSPEND)                                                                 	//USB���߹���/�������
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             	//����
        {
#if DE_PRINTF
            printf( "zz" );                                                          	//˯��״̬
#endif
            while ( XBUS_AUX & bUART0_TX )
            {
                ;    //�ȴ��������
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                   	//USB����RXD0���ź�ʱ�ɱ�����
            PCON |= PD;                                                               	//˯��
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    else
	{                                                                             		//������ж�,�����ܷ��������
        USB_INT_FG = 0xFF;                                                         	    //���жϱ�־
//      printf("UnknownInt  N");
    }
}
/*******************************************************************************
* Function Name  : Absolute_Up_Pack
* Description    : �ϴ�����������
* Input          : POINTER * pTP, UINT8 point_num
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 Absolute_Up_Pack( POINTER * pTP, UINT8 Point_Num )
{
	UINT8 i;
	UINT8 dat[84] = {0};
	static UINT16 Contact_Cnt = 0x0000;	
	UINT8 num = 0;
	dat[0] = 0x01;
	for( i = 0; i < POINTER_NUM; i ++ ) 
	{
		dat[1 + i * 8] = ( pTP + i ) -> Tip_Switch;
		dat[2 + i * 8] = ( pTP + i ) -> Contact_Identifier;
		dat[3 + i * 8] = ( ( pTP + i ) -> X_pos ) & 0x00ff;
		dat[4 + i * 8] = ( ( pTP + i ) -> X_pos >> 8 ) & 0x00ff ;
		dat[5 + i * 8] = ( pTP + i ) -> Y_pos & 0x00ff;
		dat[6 + i * 8] = ( ( pTP + i ) -> Y_pos >> 8 ) & 0x00ff ;
		dat[7 + i * 8] = ( pTP + i ) -> Resolution_Multi & 0x00ff;
		dat[8 + i * 8] = ( ( pTP + i ) -> Resolution_Multi >> 8 ) & 0x00ff ;
		if ( ( pTP + i ) -> Tip_Switch == 1)
		{
			num++;
		}
}

#if 0
	for( i = 0; i < 84; i++ )
	{
		printf( "dat[%02d]=%04x\t",(UINT16)i, (UINT16)dat[i] );
	}
	printf("\n");
#endif	
	
	Contact_Cnt++;
	dat[81] = Contact_Cnt & 0X00FF;
	dat[82] = (Contact_Cnt >> 8) & 0x00ff;
	dat[83] = ( num == 0) ? 1: num;
	EX0 = 0;
	Enp2BlukIn( dat, 64 );
	mDelaymS(2);
	Enp2BlukIn( &dat[64], 20 );
	EX0 = 1;
	
	return 1;
}


/* END OF FILE */