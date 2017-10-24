/********************************** (C) COPYRIGHT *******************************
* File Name          : MAIN.C
* Author             : RZ
* Version            : V1.00
* Date               : 2017-5-16
* Description        : ��CH554��ģ��Multi Touch�豸ʵ�ֶ�ָ���ع���
*******************************************************************************/
#include "CH554.H"
#include "GT911.H"
#include "DEBUG.H"
#include "DEVICE.H"
#include <stdio.h>


/*******************************************************************************
* Function Name  : main
* Description    :
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void main( void )
{

    CfgFsys( );                                                           	/* CH559ʱ��ѡ������ */
    mDelaymS(5);                                                          	/* �޸���Ƶ�ȴ��ڲ������ȶ�,�ؼ� */
    mInitSTDIO( );                                                        	/* ����0��ʼ�� */
#if DE_PRINTF
    printf("CH554_HID_TP_V100 DEBUG\n DEBUG_DATA: "__DATE__""__TIME__" \n");
#endif	
	
    USBDeviceInit();                                                     	/* USB�豸ģʽ��ʼ�� */
	GT911_Init();
    UEP1_T_LEN = 0;                                                      	/* Ԥʹ�÷��ͳ���һ��Ҫ��� */
    UEP2_T_LEN = 0;                                                      	/* Ԥʹ�÷��ͳ���һ��Ҫ��� */
		

	IT0 = 1;
//	EX0 = 1;
	EA = 1;                                                              	/* ������Ƭ���ж� */
	CH554WDTModeSelect(1);

    while(1)
    {
		CH554WDTFeed(0);

		GT911_Touch_Check();
    }
}