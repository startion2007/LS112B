#include "YC_Define.h"
#include "YC_Peripheral.h"
#include "YC_Utilities.h"
#include "hwreg.h"
#include "system.h"
#include "CS_Scale_Proc.h"
#include "CS_CommTo1186.h"
#include "CS_ScaleDisplay.h"
#include "btreg.h"
#include "BT_IPC.h"

CS_ComState	xdata R_Weight_Com_Coo;	//协助通信处理的变量
u8_t uartRcvBuf[16];
u8_t uartTxBuf[16];
u8_t uartTimeoutCnt;		//timeunit is 10ms


void CS_Scale_1186ComSend(u8_t com_comand)
{
R_Weight_Com_Coo.sucess=false;
R_Weight_Com_Coo.now = com_comand;
}


u8_t CS_If1186ComSucess(void)
{
return R_Weight_Com_Coo.sucess;
}


void CS_Scale_1186Com(void)
{	
	
	if(uartTimeoutCnt < CS_CommTo1186_TimeOut)	 
		{
		
			switch(R_Scale_state)
			{
			case CS_Scale_state_init:
				CS_1186Com_Reset_Proc();			
				CS_1186Com_ReadMacAdress_Proc();
				CS_1186Com_ReadTime_Proc();
				CS_1186Com_SetOpenWeight_Proc();
				CS_1186Com_SetLcd_Proc();
				break;			
			case CS_Scale_state_standby:
				CS_1186Com_SetSleepMode_Proc();
				CS_1186Com_ReadAdZero_Proc();
				CS_1186Com_SetLcd_Proc();
				break;
			case CS_Scale_state_weighting:
			case CS_Scale_state_caling:
			case CS_Scale_state_locking:
				CS_1186Com_ReadTime_Proc();
				CS_1186Com_ReadAdc_Proc();
				CS_1186Com_SetLcd_Proc();

				/*
				if(B_Weight_AdOk == true)
					{
					R_Weight_Com_Coo.now = CS_CommTo1186_LcdDisplay;
					B_Weight_LcdOk =false;
					}
				if(B_Weight_LcdOk == true)
					{
					R_Weight_Com_Coo.now = CS_CommTo1186_ReadAd;	
					}
				*/
				break;
			default:
				break;			
			}		
		}
	else
		{
		YC_UARTClearBuffer();	// fresh rec buffer
		uartTimeoutCnt=0;
		if(R_Weight_Com_Coo.now !=CS_CommTo1186_Null)
		R_Weight_Com_Coo.now = R_Weight_Com_Coo.pre;	//re send
		}

}



void CS_1186Com_ReadMacAdress_Proc(void)
{
	u8_t i;
	u8_t xdata * ptr;

	i=0;
	ptr =&i;
	
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadOtp)
		{
		uartTxBuf[0]=6;		//read 6 byte MacAdress
		uartTxBuf[1]=0xF7;	//OTP  adress low 8bits
		uartTxBuf[2]=0x0F;	//OTP  adress high 8bits
		CS_CommTo1186_SendCmd(CS_CommTo1186_ReadOtp,uartTxBuf);
		R_Weight_Com_Coo.pre = CS_CommTo1186_ReadOtp;
		R_Weight_Com_Coo.now = CS_CommTo1186_ReadOtpStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadOtpStandby)
		{
		if(YC_UARTReciveDataExpected(uartRcvBuf, 11) == 11)
			{
			if(generateChecksum(uartRcvBuf,10) == uartRcvBuf[10] &&
			uartRcvBuf[2] == UART_EVENT_OK &&
			uartRcvBuf[3] == CS_CommTo1186_ReadOtp)
				{	
				//slave_state = SLAVE_ST_SET_PARAM;
				//R_Debug_temp= uartRcvBuf[5];	//测试用

				//数据放错地址，部分数据手动赋值
				uartRcvBuf[9]= uartRcvBuf[6];
				uartRcvBuf[8]=0x55;
				uartRcvBuf[7]=uartRcvBuf[5];
				uartRcvBuf[6]=0xbe;
				uartRcvBuf[5]=uartRcvBuf[4];
				uartRcvBuf[4]=0x08;

				/*
				xmemcpy(mem_le_lap,&uartRcvBuf[4],6);
				if((*mem_adv_lap_ptr) != 0) 
					{
					for(i=0;i<6;i++) 
						{
						ptr = (u8_t xdata *)(ESWAP(*mem_adv_lap_ptr)+i);
						*ptr= uartRcvBuf[9-i];
						}
					}
				*/
				R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				R_Weight_Com_Coo.sucess=true;
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}
		}
}


void CS_1186Com_ReadAdc_Proc(void)
{
	u32_t	data_rec;
	
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadAd)
		{
		CS_CommTo1186_SendCmd(CS_CommTo1186_ReadAd,0);
		R_Weight_Com_Coo.pre = CS_CommTo1186_ReadAd;
		R_Weight_Com_Coo.now = CS_CommTo1186_ReadAdStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadAdStandby)
		{						
		if(YC_UARTReciveDataExpected(uartRcvBuf, 9)==9)
			{						
			if(uartRcvBuf[3] == CS_CommTo1186_ReadAd)   
				{
				
				
				R_1186sys_state = uartRcvBuf[7];

				if(R_1186sys_state&0x08)			//AD数据更新标志
					{
					B_Weight_AdOk = true;
					data_rec = uartRcvBuf[4];
					data_rec = (data_rec<<8) + uartRcvBuf[5];
					data_rec = (data_rec<<8) + uartRcvBuf[6];
					R_AD_Original =data_rec >> 6;	
					}
				
				if(R_Scale_state==CS_Scale_state_init)
					R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				else
					R_Weight_Com_Coo.now = CS_CommTo1186_LcdDisplay;
				R_Weight_Com_Coo.sucess=true;
				
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}
		}
}



void CS_1186Com_SetLcd_Proc(void)
{

	if(R_Weight_Com_Coo.now == CS_CommTo1186_LcdDisplay)
		{
		CS_CommTo1186_SendCmd(CS_CommTo1186_LcdDisplay,CS_Lcd_Send_Data);
		R_Weight_Com_Coo.pre = CS_CommTo1186_LcdDisplay;
		R_Weight_Com_Coo.now = CS_CommTo1186_LcdDisplayStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_LcdDisplayStandby)
		{
		if(YC_UARTReciveDataExpected(uartRcvBuf, 5)==5)
			{
			if(uartRcvBuf[3] == CS_CommTo1186_LcdDisplay)    
				{
				if(R_Scale_state==CS_Scale_state_init)
					R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				else
					R_Weight_Com_Coo.now = CS_CommTo1186_ReadAd;
				R_Weight_Com_Coo.sucess=true;		
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}	
		}
}



void CS_1186Com_ReadTime_Proc(void)
{
	//static u8_t xdata temp=0;
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadTime)
		{
		CS_CommTo1186_SendCmd(CS_CommTo1186_ReadTime,0);
		R_Weight_Com_Coo.pre = CS_CommTo1186_ReadTime;
		R_Weight_Com_Coo.now = CS_CommTo1186_ReadTimeStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadTimeStandby)
		{
		if(YC_UARTReciveDataExpected(uartRcvBuf, 9)==9)
			{
			if(uartRcvBuf[3] == CS_CommTo1186_ReadTime)    
				{			
				//读到时间的操作
				R_1186_RTC[3] = uartRcvBuf[4];
				R_1186_RTC[2] = uartRcvBuf[5];
				R_1186_RTC[1] = uartRcvBuf[6];
				R_1186_RTC[0] = uartRcvBuf[7];

			
				/*
				R_Debug_temp=R_1186RTC.R_32;
				CS_ScaleDisplay_Debug();
				while(temp==2);
				temp=2;
				*/
				/*
				R_1186RTC.R_BYTE[3]=  uartRcvBuf[4];
				R_1186RTC.R_BYTE[2] = uartRcvBuf[5];
				R_1186RTC.R_BYTE[1] = uartRcvBuf[6];
				R_1186RTC.R_BYTE[0] = uartRcvBuf[7];
				/*
				R_1186_RTC = uartRcvBuf[4];
				R_1186_RTC = R_1186_RTC<<8;
				R_1186_RTC |= uartRcvBuf[5];
				R_1186_RTC = R_1186_RTC<<8;
				R_1186_RTC |= uartRcvBuf[6];
				R_1186_RTC = R_1186_RTC<<8;
				R_1186_RTC |= uartRcvBuf[7];
				*/
				R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				R_Weight_Com_Coo.sucess=true;
				B_Weight_AdOk = true;
						
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}	
		}
}



void CS_1186Com_SetSleepMode_Proc(void)
{
	u16_t	R_AD_Zero;
	
	R_AD_Zero = CS_Scale_ZeroProc(GetRunningZero,0);
		
	if(R_Weight_Com_Coo.now == CS_CommTo1186_SetSleepMode)
		{
		
		uartTxBuf[0]= 0x91;
		uartTxBuf[1]= R_AD_Zero<<6;
		uartTxBuf[2]= R_AD_Zero>>2;
		uartTxBuf[3]= R_AD_Zero>>10;
		uartTxBuf[3]|= 0x80;
		CS_CommTo1186_SendCmd(CS_CommTo1186_SetSleepMode,uartTxBuf);
		
		YC_UARTWaitSendData();
		
		R_Weight_Com_Coo.pre = CS_CommTo1186_SetSleepMode;
		R_Weight_Com_Coo.now = CS_CommTo1186_SetSleepModeStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_SetSleepModeStandby)
		{
		if(YC_UARTReciveDataExpected(uartRcvBuf, 5)==5)
			{
			if(uartRcvBuf[3] == CS_CommTo1186_SetSleepMode)    
				{			
				R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				R_Weight_Com_Coo.sucess=true;	
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}	
		}
}




void CS_1186Com_SetOpenWeight_Proc(void)
{
	u16_t	R_Open_Weight;
	R_Open_Weight = CS_Scale_CaliProc(CaliProcGetOpenWeight);	//获取开机重量值
	if(R_Weight_Com_Coo.now == CS_CommTo1186_SetOpenWeight)
		{
		uartTxBuf[0]= 0x20;
		uartTxBuf[1]= R_Open_Weight<<6;
		uartTxBuf[2]= R_Open_Weight>>2;
		uartTxBuf[3]= R_Open_Weight<<6;
		uartTxBuf[4]= R_Open_Weight>>2;	
		CS_CommTo1186_SendCmd(CS_CommTo1186_SetOpenWeight,uartTxBuf);
		R_Weight_Com_Coo.pre = CS_CommTo1186_SetOpenWeight;
		R_Weight_Com_Coo.now = CS_CommTo1186_SetOpenWeightStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_SetOpenWeightStandby)
		{
		if(YC_UARTReciveDataExpected(uartRcvBuf, 5)==5)
			{
			if(uartRcvBuf[3] == CS_CommTo1186_SetOpenWeight)    
				{			
				//读到时间的操作
				//R_Debug_temp=uartRcvBuf[5];
				
				R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				R_Weight_Com_Coo.sucess=true;
				
						
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}	
		}
}



void CS_1186Com_ReadAdZero_Proc(void)
{
	u32_t	data_rec;
	u16_t	R_AD_Zero;
	
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadAdZero)
		{
		CS_CommTo1186_SendCmd(CS_CommTo1186_ReadAdZero,0);
		R_Weight_Com_Coo.pre = CS_CommTo1186_ReadAdZero;
		R_Weight_Com_Coo.now = CS_CommTo1186_ReadAdZeroStandby;
		uartTimeoutCnt=0;
		YC_UARTClearBuffer();
		}
	if(R_Weight_Com_Coo.now == CS_CommTo1186_ReadAdZeroStandby)
		{						
		if(YC_UARTReciveDataExpected(uartRcvBuf, 8)==8)
			{						
			if(uartRcvBuf[3] == CS_CommTo1186_ReadAdZero)   
				{
				data_rec = uartRcvBuf[4];
				data_rec = (data_rec<<8) + uartRcvBuf[5];
				data_rec = (data_rec<<8) + uartRcvBuf[6];
				R_AD_Zero =data_rec >> 6;	
				CS_Scale_ZeroProc(SetRunningZero,R_AD_Zero);
				
				R_Weight_Com_Coo.now = CS_CommTo1186_Null;
				R_Weight_Com_Coo.sucess=true;
				
				}
			else
				uartTimeoutCnt =CS_CommTo1186_TimeOut;   //手动超时重发
			}
		}
}



void CS_1186Com_Reset_Proc(void)
{

	if(R_Weight_Com_Coo.now == CS_CommTo1186_Reset)
		{
		CS_CommTo1186_SendCmd(CS_CommTo1186_Reset,0);
		YC_UARTClearBuffer();
		R_Weight_Com_Coo.now=CS_CommTo1186_Null;
		
		}
													
}



u8_t CS_CommTo1186_Xor(u8_t * buf,u8_t len)
{
	u8_t	i;
	for(i=0;i<len;i++)
		{
		*(buf+9) = *(buf+9) ^	*(buf+i);
		}
	return *(buf+9);
}

void CS_CommTo1186_SendCmd(u8_t cmd_code ,u8_t * databuf)
{
	u8_t buf[10] = {0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00,0x00,0x00};
       u8_t len;

	 buf[0] = 0xc5;							//包头数据
	switch(cmd_code)
		{
		case CS_CommTo1186_Reset:			//0
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			len  =  3;
			break;
		case CS_CommTo1186_ReadAd:		 	// 1 
			buf[1] = 0x01;
			buf[2] = 0x80;
			buf[3] = 0x44;
			len  =  4;
			break;			
		case CS_CommTo1186_ReadVersion:		 // 2 
			buf[1] = 0x01;
			buf[2] = 0x81;
			buf[3] = 0x45;
			len  =  4;
			break;
		case CS_CommTo1186_ReadTime:		 // 3 
			buf[1] = 0x01;
			buf[2] = 0x82;
			buf[3] = 0x46;
			len  =  4;
			break;
		case CS_CommTo1186_ReadRam:		 // 4 
			buf[1] = 0x03;
			buf[2] = 0x83;
			buf[3] = databuf[1];
			buf[4] = databuf[0];	 
			buf[5] =  CS_CommTo1186_Xor(buf,5);
			len  =  6;
			break;
		case CS_CommTo1186_ReadOtp:		 // 5 
			buf[1] = 0x04;
			buf[2] = 0x84;
			buf[3] = databuf[2];
			buf[4] = databuf[1];	
			buf[5] = databuf[0]; 
			buf[6] = CS_CommTo1186_Xor(buf,6);
			len  =  7;
			break;
		case CS_CommTo1186_ReadAdZero:		 // 6 
			buf[1] = 0x01;
			buf[2] = 0x85;
			buf[3] = 0x41;
			len  =  4;
			break;
		case CS_CommTo1186_SetSleepMode:	 // 7 
			buf[1] = 0x05;
			buf[2] = 0xA0;
			buf[3] = databuf[3];
			buf[4] = databuf[2];	
			buf[5] = databuf[1];
			buf[6] = databuf[0]; 
			buf[7] = CS_CommTo1186_Xor(buf,7);
			len  =  8;
			break;
		case CS_CommTo1186_SetOpenWeight:	 // 8 
			buf[1] = 0x06;
			buf[2] = 0xA1;
			buf[3] = databuf[4];
			buf[4] = databuf[3];	
			buf[5] = databuf[2];
			buf[6] = databuf[1];
			buf[7] = databuf[0]; 
			buf[8] = CS_CommTo1186_Xor(buf,8);
			len  =  9;
			break;
		case CS_CommTo1186_SetTime:		 // 9 
			buf[1] = 0x05;
			buf[2] = 0xA2;
			buf[3] = databuf[3];
			buf[4] = databuf[2];	
			buf[5] = databuf[1];
			buf[6] = databuf[0]; 
			buf[7] = CS_CommTo1186_Xor(buf,7);
			len  =  8;	
			break;
		case CS_CommTo1186_SetRam:		 	// 10 
			buf[1] = 0x04;
			buf[2] = 0xA3;
			buf[3] = databuf[2];
			buf[4] = databuf[1];	
			buf[5] = databuf[0]; 
			buf[6] = CS_CommTo1186_Xor(buf,6);
			len  =  7;			
			break;
		case CS_CommTo1186_LcdDisplay:		// 11 
			buf[1] = 0x07;
			buf[2] = 0xe0;
			buf[3] = databuf[5];
			buf[4] = databuf[4];
			buf[5] = databuf[3];
			buf[6] = databuf[2];
			buf[7] = databuf[1];
			buf[8] = databuf[0];		 
			buf[9] = CS_CommTo1186_Xor(buf,9);
			len  =  10;
			break;
		default:		
			buf[1] = 0x00;
			len   = 2;
			break;
		}
	YC_UARTSendData(buf, len);
}





























