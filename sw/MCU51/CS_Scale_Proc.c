#include "YC_Define.h"
#include "YC_Peripheral.h"
#include "system.h"
#include "CS_Scale_Proc.h"
#include "CS_CommTo1186.h"
#include "CS_ScaleDisplay.h"
#include "btreg.h"
#include "BT_IPC.h"
#include "lxProtocol.h"

#include "retation_variable.h"
#include "YC_Utilities.h"
#include "hwreg.h"



bool	B_Debug_Test;				//����ģʽ
u16_t xdata R_Debug_temp;		//������

u8_t   xdata R_Selet_Unit;			//��λѡ��״̬
u8_t	  xdata R_Scale_state;		//�ӵ�״̬
u8_t	  xdata R_Display_Err;		//��ʾ������Ϣ
	

//ȫ�ֱ���--------------------
u16_t xdata  R_AD_Original;		//ԭʼADֵ
u8_t	  xdata  R_1186sys_state;		//1186ϵͳ״̬
u8_t   xdata  B_Weight_AdOk;		//AD׼���ñ�־




u8_t xdata  R_1186_RTC[4];			//1186��RTCʱ��


u16_t xdata R_Weight_Mem;		//��������
u16_t xdata R_Weight_Lock;		//��������
CS_Weight_T  xdata R_UnitWeight;	//����ֵ



//////////////////////////////////////////////ϵͳ����

#define	CS_Scale_FiterX					0x05		//�����˲���Χ5��AD����
#define	CS_Scale_MaxWeight				15300		//����������153.00kg
#define	CS_Scale_MinLockWeight			250			//��С��������2.50kg
#define	CS_Scale_RetLockWeightInc		1000		//���ؽ�������10kg
#define	CS_Scale_RetLockWeightDec		300			//���ؽ�������3kg
#define	CS_Scale_SameWeightRange		50			//�ж���ͬһ����������ֵ50��AD����
#define	CS_Scale_TimeOut_Zero			15			//��λ��ʱ�ػ�ʱ��15s
#define	CS_Scale_TimeOut_Lock			15			//������ʱ�ػ�ʱ��15s
#define	CS_Scale_TimeOut_Unstable		15			//���ȶ��ػ�ʱ��15s
#define	CS_Scale_TimeOut_OverLoad		3			//���عػ�ʱ��3s
#define	CS_Scale_TimeOut_LowBat		3			//�͵�ػ�ʱ��3s
#define	SameZeroRange					10			//��Ϊ��ͬһ��������Χ
#define	ScaleWeightMin					10			//����������߽�ֵ
#define	ScaleWeightMax					15			//���������ұ߽�ֵ
#define	ScaleStartMemoryWeight			600			//�ӿ�ʼ����������Χ
#define	CS_CaliStateData_Up				6000			//�궨������Чֵ����(��ӦAD����)
#define	CS_CaliStateData_Down			1000			//�궨������Чֵ����(��ӦAD����)
#define   CS_PowerOnAD_Steadytimes		10			//�ȶ������ȶ�����
#define   CS_SmallWeight_Steadytimes		6			//�ж��ȶ��Ĵ���
#define   CS_BigWeight_Steadytimes		6			//�ж��ȶ��Ĵ���
#define   CS_Cali_Steadytimes				15			//�궨ʱ�ж��ȶ��Ĵ���



//////////////////////////////////////////////�ڲ���������

void CS_Scale_WeightProc(void);


#define	GetDeltaAD					0
#define	GetADDirection				1
u16_t CS_Scale_GetDeltaAD(u8_t option);


#define	ScaleAdcFilterRun			0
#define	ScaleAdcFilterReset			1
#define	ScaleAdcFilterGetFilTimes	2
u16_t CS_Scale_AdcFilter(u8_t option,u16_t R_AD_BUF);


void CS_Scale_GetWeight(u16_t R_AD_BUF);


#define	JudgeSteadyRun				0
#define	ResetSteady					1
#define	GetIfSmallSteady			2
#define	GetIfHeavySteady			3
u8_t CS_Scale_JudgeSteady(u8_t option);



#define	LockProcRun					0
#define	SetDowningLock				1
#define	GetIfLock					2
u8_t CS_Scale_LockProc(u8_t option);


#define	SteadyProcReset				0
#define	SteadyProcRun				1
void CS_Scale_SteadyProc(u8_t option);


void CS_Scale_ReturnLock(void);


void CS_Scale_StandbyProc(u8_t B_Reset);


void CS_Scale_PowerOnProc(bool B_Reset);

/*
R_Debug_temp=1234;
CS_ScaleDisplay_Debug();
while(1);

*/

//////////////////////////////////////////������

void CS_Scale_Proc(void)
{
	if(R_Scale_state == CS_Scale_state_init)
		{	
		CS_Scale_PowerOnProc(0);
		}
	if(R_Scale_state == CS_Scale_state_standby)
		{
		CS_Scale_StandbyProc(0);		
		}
		
	if(B_Weight_AdOk == true)
		{		
		B_Weight_AdOk = false;	
		
		if(CS_Scale_ZeroProc(GetPowerOnZeroState,0)== false)	//��û�ϵ�ȡ�����ȡ���
			{
			CS_Scale_ZeroProc(FindPowerOnZero,0);
			}
		else
			{
				
			if(R_Scale_state == CS_Scale_state_weighting)
				{
				CS_Scale_WeightProc();
				CS_ScaleDisplay();
				}
			if(R_Scale_state == CS_Scale_state_locking)
				{					
				CS_ScaleDisplay_SteadyFlash();
				CS_Scale_SteadyProc(SteadyProcRun);
				}
			if(R_Scale_state == CS_Scale_state_caling)
				{
				CS_Scale_CaliProc(CaliProcRun);
				}	
			
			CS_Scale_TimeOutProc(0);
			}
		}
}	



//////////////////////////////////////////��������



void CS_Scale_PowerOn(void)
{
	
	if(R_Scale_state == CS_Scale_state_init)
		{
		B_Weight_AdOk = false;
		R_Weight_Com_Coo.pre =CS_CommTo1186_Null;	
		R_Weight_Com_Coo.now = CS_CommTo1186_Null;	
		//R_Scale_state =	CS_Scale_state_weighting;
		R_Selet_Unit = CS_ScaleDisplay_L_kg;

		R_Debug_temp = 0;
		B_Debug_Test =false;
		
		CS_Scale_CaliProc(CaliProcSetCali);		
		CS_Scale_ZeroProc(ZeroProcReset,0);	
		
		CS_Scale_TimeOutProc(CS_Reset_AllTime);
		CS_Scale_PowerOnProc(1);
		}
}






//////////////////////////////////////////������������

void CS_Scale_WeightProc(void)
{
	u16_t	R_AD_BUF;
	
	R_AD_BUF = CS_Scale_GetDeltaAD(GetDeltaAD);
	R_AD_BUF = CS_Scale_AdcFilter(ScaleAdcFilterRun,R_AD_BUF);
	CS_Scale_GetWeight(R_AD_BUF);
	CS_Scale_ZeroProc(RunningZero,0);
	CS_Scale_LockProc(LockProcRun);
	CS_Scale_JudgeOverLoad(0);
	CS_Scale_JudgeLowBat(0);
	CS_Scale_JudgeSteady(JudgeSteadyRun);
	
}


//��ǰAD�����AD�Ĳ�ֵ
u16_t CS_Scale_GetDeltaAD(u8_t option)
{
	static       u8_t xdata B_Weight_Pos;				//����AD�仯��־
	u16_t	ad_temp;
	u16_t      R_AD_Zero;
	R_AD_Zero = CS_Scale_ZeroProc(GetRunningZero,0);
	if(R_AD_Original>R_AD_Zero)
		{
		ad_temp = R_AD_Original - R_AD_Zero;
		B_Weight_Pos = true;

		}
	else
		{
		ad_temp = R_AD_Zero - R_AD_Original;
		B_Weight_Pos = false;		
		}

	
	if(option == GetADDirection)
		{
		return	B_Weight_Pos;
		}	
	return ad_temp;
}



//����ƽ���˲�
u16_t CS_Scale_AdcFilter(u8_t option,u16_t R_AD_BUF)
{
	u32_t	ad_temp;
	static	u16_t xdata R_AD_FilTimes;
	static	u16_t xdata R_AD_BUF_OLD;
	static	u16_t xdata R_AD_F1;
	static	u16_t xdata R_AD_F2;
	static	u16_t xdata R_AD_F3;
	static	u16_t xdata R_AD_F4;
	static	u16_t xdata R_AD_F5;
	static	u16_t xdata R_AD_F6;
	static	u16_t xdata R_AD_F7;
	static	u16_t xdata R_AD_F8;	
		
	if(option==ScaleAdcFilterGetFilTimes)
		{
		return  R_AD_FilTimes;
		}
	
	
	if(R_AD_BUF >R_AD_BUF_OLD)
		ad_temp = R_AD_BUF - R_AD_BUF_OLD;
	else
		ad_temp = R_AD_BUF_OLD - R_AD_BUF;

	R_AD_BUF_OLD	=	R_AD_BUF;
	
	if((ad_temp > CS_Scale_FiterX) ||(option==ScaleAdcFilterReset))	
		{						
		R_AD_FilTimes = 0;
		R_AD_F1 	    = 0;
		R_AD_F2 	    = 0;
		R_AD_F3 	    = 0;
		R_AD_F4 	    = 0;
		R_AD_F5 	    = 0;
		R_AD_F6 	    = 0;
		R_AD_F7 	    = 0;
		R_AD_F8 	    = 0;
		}
	else
		{
		if(R_AD_FilTimes < 200 )
		R_AD_FilTimes ++;	

			if(R_AD_FilTimes<=1)
				R_AD_F1 = R_AD_BUF;	
			if(R_AD_FilTimes<=2)
				R_AD_F2 = R_AD_BUF;
			if(R_AD_FilTimes<=3)
				R_AD_F3 = R_AD_BUF;
			if(R_AD_FilTimes<=4)
				R_AD_F4 = R_AD_BUF;
			if(R_AD_FilTimes<=5)
				R_AD_F5 = R_AD_BUF;
			if(R_AD_FilTimes<=6)
				R_AD_F6 = R_AD_BUF;
			if(R_AD_FilTimes<=7)
				R_AD_F7 = R_AD_BUF;
			if(R_AD_FilTimes<=8)
				R_AD_F8 = R_AD_BUF;
			if(R_AD_FilTimes>8)
				{
				R_AD_F1 = R_AD_F2;
				R_AD_F2 = R_AD_F3;
				R_AD_F3 = R_AD_F4;
				R_AD_F4 = R_AD_F5;
				R_AD_F5 = R_AD_F6;
				R_AD_F6 = R_AD_F7;
				R_AD_F7 = R_AD_F8;
				R_AD_F8 = R_AD_BUF;
				}

		ad_temp	= R_AD_F1;
		ad_temp	+= R_AD_F2;
		ad_temp	+= R_AD_F3;
		ad_temp	+= R_AD_F4;
		ad_temp	+= R_AD_F5;
		ad_temp	+= R_AD_F6;
		ad_temp	+= R_AD_F7;
		ad_temp	+= R_AD_F8;

		R_AD_BUF = ad_temp / 8 ;
		R_AD_BUF_OLD	=	R_AD_BUF;
		}
	return	R_AD_BUF;
}




void CS_Scale_GetWeight(u16_t R_AD_BUF)
{
	u32_t	ad_temp;
	u16_t	ad_cal;
	u16_t	CaliDot1,CaliDot2,CaliDot3;
	
	CaliDot1 =CS_Scale_CaliProc(CaliProcGetCaliDot1);
	CaliDot2 =CS_Scale_CaliProc(CaliProcGetCaliDot2);
	CaliDot3 =CS_Scale_CaliProc(CaliProcGetCaliDot3);
	
	if(R_AD_BUF <= CaliDot1 )
		{
		ad_temp = (u32_t)R_AD_BUF  * 5000;	
		R_UnitWeight.origin = ad_temp / CaliDot1;
		return;
		}
	ad_cal = R_AD_BUF - CaliDot1;

	if(ad_cal <=CaliDot2)
		{
		ad_temp = (u32_t)ad_cal  * 5000;	
		R_UnitWeight.origin = ad_temp / CaliDot2 +5000 ;
		return;
		}
	
	ad_cal  = R_AD_BUF - CaliDot1 - CaliDot2 ;	
	ad_temp = (u32_t)ad_cal  * 5000;	
	R_UnitWeight.origin = ad_temp /CaliDot3 +10000 ;

	
}


void CS_Scale_ChangeUnit()
{
	switch(R_Selet_Unit)
		{
		case CS_ScaleDisplay_L_kg:
			R_Selet_Unit=CS_ScaleDisplay_L_lb;
			break;
		case CS_ScaleDisplay_L_lb:
			R_Selet_Unit=CS_ScaleDisplay_L_jin;
			break;
		case CS_ScaleDisplay_L_jin:
			R_Selet_Unit=CS_ScaleDisplay_L_gongjin;
			break;
		case CS_ScaleDisplay_L_gongjin:
			R_Selet_Unit=CS_ScaleDisplay_L_kg;
			break;
		default :
			R_Selet_Unit=CS_ScaleDisplay_L_kg;
			break;
		}
}



u16_t  CS_Scale_ZeroProc(u8_t option,u16_t setdata)
{
	static	u8_t	   xdata R_Weight_GetZeroTimeOut = 0;	
	static	u16_t  xdata R_AD_PowerOnZero;	//�ϵ����ADֵ
	static	u16_t  xdata R_AD_Zero;			//���ADֵ
	static	u8_t    xdata B_Weight_Zero;		//��������־
	static	u8_t    xdata B_Weight_ZeroOk;	//�ϵ��ȡ������־
	static	u8_t	   xdata B_LockDownCheck;	//�³Ӻ��һ���ȶ�׷��
	static	u8_t	   xdata B_WeightZeroErr;		//�������־
	
	u16_t	R_AdFilTimes;
	u16_t	R_AD_BUF;
	u16_t	weight_temp;
	u16_t	R_Open_Weight;
	u8_t		B_Weight_Pos;

	
	
	if(option==ZeroProcReset)
		{
		B_Weight_ZeroOk = 0;
		return 0;		
		}
	if(option==SetPowerOnZero)
		{
		R_AD_PowerOnZero = setdata;
		return 0;		
		}
	if(option==SetRunningZero)
		{
		R_AD_Zero = setdata;
		return 0;		
		}
	if(option==ClrLockDownCheckFlag)
		{
		B_LockDownCheck=false;
		return 0;
		}
	
	if(option==FindPowerOnZero)
	{
		
	
		if(R_Weight_GetZeroTimeOut <60)
		{
		R_Weight_GetZeroTimeOut ++;
		
		R_AD_BUF = R_AD_Original;
		R_AD_BUF=CS_Scale_AdcFilter(ScaleAdcFilterRun,R_AD_BUF);
		R_AdFilTimes = CS_Scale_AdcFilter(ScaleAdcFilterGetFilTimes,R_AD_BUF);
		
			if(R_AdFilTimes >=CS_PowerOnAD_Steadytimes)
				{
				R_AD_Zero	= R_AD_BUF;
				R_AD_PowerOnZero = R_AD_BUF;
				B_Weight_ZeroOk = true;
				}
		}
		else
		{
		R_AD_Zero	= 4000;
		R_AD_PowerOnZero = 4000;
		B_Weight_ZeroOk = true;
		}

	}

	if(option==RunningZero)
	{
	B_Weight_Zero = false;		
	//3kg���²�׷�����������
	B_Weight_Pos = CS_Scale_GetDeltaAD(GetADDirection);		//��ȡAD����
	R_Open_Weight = CS_Scale_CaliProc(CaliProcGetOpenWeight);	//��ȡ��������ֵ
	
	if(R_UnitWeight.origin <= 300 ||B_Weight_Pos ==false)	
		B_Weight_Zero = true;

		if(R_UnitWeight.origin <= 200 || B_Weight_Pos==false)
			{		
				if(CS_Scale_JudgeSteady(GetIfSmallSteady)==true)
					{

					CS_Scale_JudgeSteady(ResetSteady);
					
					//������״̬����׷��
					if(CS_Scale_LockProc(GetIfLock)==false)	
						{
						if(R_AD_Original >= R_AD_PowerOnZero)
							{
							weight_temp = R_AD_Original - R_AD_PowerOnZero;
							if(weight_temp < R_Open_Weight)
								R_AD_Zero = R_AD_Original;
							}
						else
							R_AD_Zero = R_AD_Original;
						}
					//����״̬�³ӵ�һ��׷������������
					else			
						{
						if(B_LockDownCheck==false)
							{
							if(R_AD_Original>R_AD_Zero)
								weight_temp = R_AD_Original - R_AD_Zero;
							else
								weight_temp = R_AD_Zero - R_AD_Original;

							//������ǰ��������Ƚϲ�����һ�����������Ĵ���
							//if(weight_temp<ScaleWeightMin&&weight_temp>ScaleWeightMax)
								//{
								R_AD_Zero = R_AD_Original;	//�������
								if(weight_temp>SameZeroRange)
									{
									//������ǰ����һ����㱨����������
									B_WeightZeroErr=true;
									R_Display_Err=CS_ScaleDisplay_L_err;
									CS_Scale_LockProc(LockProcUnLock);
									}
								B_LockDownCheck=true;
								//}

							}						
						}
					
					}
			}
	return	0;		
	}

	if(option==GetPowerOnZero)
		return	R_AD_PowerOnZero;
	if(option==GetPowerOnZeroState)
		return	B_Weight_ZeroOk;
	if(option==GetRunningZero)
		return	R_AD_Zero;
	if(option==GetZeroState)
		return	B_Weight_Zero;
	if(option==GetIfZeroErr)
		return   B_WeightZeroErr;
	
	return 0;	
}



#define	LockProcZeroLock			1
#define	LockProcZeroUnLock			0xfe
#define	LockProcDownLock			2
#define	LockProcDownUnLock		0xfd
#define	LockProcLock			LockProcZeroLock+LockProcDownLock
#define	LockProcUnLock				3

u8_t CS_Scale_LockProc(u8_t option)
{
	static u8_t xdata B_Weight_Downing_Lock;		//�³ӹ�����������������ס
	
	if(option==SetDowningLock)
		{
		B_Weight_Downing_Lock =LockProcLock;
		return 0;
		}

	if(option==GetIfLock)
		{
		return B_Weight_Downing_Lock;
		}
	if(option==LockProcUnLock)
		{
		B_Weight_Downing_Lock =false;
		return 0;
		}
	
	if(CS_Scale_ZeroProc(GetZeroState,0)==true)		//������λ
		{
		B_Weight_Downing_Lock &=LockProcDownUnLock ;
		
		if(B_Weight_Downing_Lock&LockProcZeroLock)
		R_UnitWeight.origin = R_Weight_Lock;
		}
	else
		{
		if(B_Weight_Downing_Lock&LockProcDownLock)
			R_UnitWeight.origin = R_Weight_Lock;
		else
			B_Weight_Downing_Lock &= LockProcZeroUnLock;
		}

	return 0;
}




//�ò��ֹ����ƽ���1186�����������󿪻�����
/*
void CS_Scale_AvoidFalseBoot(void)
{
	static u8_t xdata R_Weight_ZeroTimes;
	static u8_t xdata B_Weight_PowerOn;		//������־
	static u8_t xdata B_Weight_AutoOpen;		//�Զ��ϳӱ�־
	
	if(R_UnitWeight.origin == 0)
		{
		if(R_Weight_ZeroTimes<255)
			R_Weight_ZeroTimes++;
		}	
	else
		{
		R_Weight_ZeroTimes = 0;
		}

	
	if(R_Weight_ZeroTimes == 1)
		{
		if(B_Weight_AutoOpen)
		B_Weight_PowerOn = false;
		else
		B_Weight_PowerOn = true;	
		}
	else
		{
		B_Weight_PowerOn = true;
		B_Weight_AutoOpen = false;
		}
}	
*/


#define	CS_Scale_LowBatBit	0x02
#define	GetIfLowBat			1

u8_t CS_Scale_JudgeLowBat(u8_t option)
{
	static	u8_t xdata B_Weight_LowBat;			//�͵��־
	
	if(R_1186sys_state&CS_Scale_LowBatBit==false)
		{
		B_Weight_LowBat = true;
		R_Display_Err=CS_ScaleDisplay_L_err;
		}
	else
		B_Weight_LowBat = false;

	if(option==GetIfLowBat)
		return B_Weight_LowBat;
	
		return 0;	
}


u8_t CS_Scale_JudgeOverLoad(u8_t option)
{

	static	u8_t	  xdata R_Weight_OverLoadTimes;
	static	u8_t  xdata  B_Weight_OverLoad;			//���ر�־
	if(option==ResetOverLoad)
		{
		R_Weight_OverLoadTimes = 0;
		B_Weight_OverLoad = false;
		return 0;
		}
	
	if(R_UnitWeight.origin > CS_Scale_MaxWeight)
		R_Weight_OverLoadTimes++;
	if(R_Weight_OverLoadTimes >=2)
		{
		R_Weight_OverLoadTimes = 2;
		B_Weight_OverLoad = true;
		R_Display_Err=CS_ScaleDisplay_L_err;
		}
	else
		B_Weight_OverLoad = false;

	if(option==GetIfOverLoad)
		return B_Weight_OverLoad;
	
		return 0;
}




u8_t CS_Scale_JudgeSteady(u8_t option)
{

	u16_t	weight_temp;
	static	u16_t xdata R_Weight_kg_old;
	static	u8_t	  xdata R_Weight_SameTimes;
	static	u8_t xdata B_Weight_SmallSteady;		//С�����ȶ���־
	static	u8_t xdata B_Weight_HeavySteady;	//�������ȶ���־
	
	if(option==ResetSteady)
		{
		R_Weight_SameTimes = 0;
		B_Weight_SmallSteady = false;
		B_Weight_HeavySteady = false;
		return 0;
		}	

	if(option==GetIfSmallSteady)
		return B_Weight_SmallSteady;

	if(option==GetIfHeavySteady)
		return B_Weight_HeavySteady;
	
	if(CS_Scale_LockProc(GetIfLock)==false)
		{
	
		if(R_UnitWeight.origin > R_Weight_kg_old)
			weight_temp = R_UnitWeight.origin -R_Weight_kg_old;
		else
			weight_temp = R_Weight_kg_old -R_UnitWeight.origin;
		
		if(weight_temp <  CS_Scale_SameWeightRange)
			{
			if(R_Weight_SameTimes<255)
				R_Weight_SameTimes++;
			}
		else
			R_Weight_SameTimes = 0;
	
		
		R_Weight_kg_old = R_UnitWeight.origin;

		B_Weight_SmallSteady = false;
		B_Weight_HeavySteady = false;
		
		if(R_UnitWeight.origin < CS_Scale_MinLockWeight)
			{
			if(R_Weight_SameTimes>CS_SmallWeight_Steadytimes)
				B_Weight_SmallSteady = true;		
			}
		else
			{			
			if(R_Weight_SameTimes>CS_BigWeight_Steadytimes)
				{
				B_Weight_HeavySteady = true;		
				R_Scale_state = CS_Scale_state_locking;
				CS_SoftTimer(ResetSoftTimer);		//������ʱ����ʼ������������ʾ��ʱ
				CS_Scale_SteadyProc(SteadyProcReset);
				CS_Scale_TimeOutProc(CS_Reset_AllTime);
				}
			}
		}
	return 0;
}




///////////////////////////////////��������

#define	CS_MemoryRange	25

void CS_Scale_SteadyProc(u8_t option)
{
	u16_t	R_AD_BUF;	
	u16_t	R_Weight_Temp;
	static	u8_t xdata B_Debug_Send;	
	static 	u8_t xdata ReadUTC;
	UserMeasureData SendData;

	if(option==SteadyProcReset)
		{
		B_Debug_Send=false;
		return;
		}
	
	if(CS_SoftTimer(GetIfTimeOut) == true)	//��������ʱ�䵽
		{
		if(CS_Scale_JudgeSteady(GetIfHeavySteady) == true)
			{
			
			CS_Scale_JudgeSteady(ResetSteady);

			if(R_UnitWeight.origin>R_Weight_Mem)
				R_Weight_Temp = R_UnitWeight.origin -R_Weight_Mem;
			else
				R_Weight_Temp = R_Weight_Mem - R_UnitWeight.origin;

			//�ڼ��䷶Χ�ڼ���
			if(R_UnitWeight.origin > ScaleStartMemoryWeight)
				{
				if(R_Weight_Temp<CS_MemoryRange)
					R_UnitWeight.origin = R_Weight_Mem;
				}
			
			R_Weight_Lock = R_UnitWeight.origin;
			R_Weight_Mem = R_UnitWeight.origin;
			CS_Scale_ZeroProc(ClrLockDownCheckFlag,0);
			ReadUTC = false;	//��ʼ����ADC
			}
		else
			{
			
			/*
			if(wechatStatus == WECHAT_ST_OK&&B_Debug_Send==false)
				{
				sendMeasureData();
				B_Debug_Send=true;
				}
			*/		
			if(B_Debug_Send==false)
				{
				
				if(ReadUTC==false)
					{
					ReadUTC=true;
					CS_Scale_1186ComSend(CS_CommTo1186_ReadTime);	
					}
				
				if(CS_If1186ComSucess()==true)
					{
					
					
					ReadUTC=false;
					
					SendData.UTCTime[0]=R_1186_RTC[0];
					SendData.UTCTime[1]=R_1186_RTC[1];	
					SendData.UTCTime[2]=R_1186_RTC[2];	
					SendData.UTCTime[3]=R_1186_RTC[3];	
					SendData.weight=ESWAP(R_UnitWeight.origin);
					QueuePush(&SendData);
					B_Debug_Send=true;

					CS_Scale_1186ComSend(CS_CommTo1186_ReadAd);
			
					//CS_SoftTimer(ResetSoftTimer);	
					
					}
				
				}
			else
				{
				R_AD_BUF = CS_Scale_GetDeltaAD(GetDeltaAD);
				R_AD_BUF = CS_Scale_AdcFilter(ScaleAdcFilterRun,R_AD_BUF);
				CS_Scale_GetWeight(R_AD_BUF);				
				CS_Scale_ReturnLock();
				}
			}
		}

			/*
			if(CS_SoftTimer(GetSoftTimerTime)<200)
				{
			R_Debug_temp=SendData.UTCTime;
			CS_ScaleDisplay_Debug();			
			return;
				}
			*/	
			/* B_Debug_Test =true;
			if(B_Debug_Test)
				{
				R_Debug_temp=888;
				CS_ScaleDisplay_Debug();
				while(1);
				}
			*/
}



void CS_Scale_ReturnLock(void)
{
	u16_t	weight_temp;
	
	if( R_UnitWeight.origin > R_Weight_Lock)
		{
		weight_temp = R_UnitWeight.origin -R_Weight_Lock;
		if(weight_temp >= CS_Scale_RetLockWeightInc)
			{
			R_Scale_state = CS_Scale_state_weighting;
			CS_Scale_TimeOutProc(CS_Reset_AllTime);
			}
		}
	else
		{
		weight_temp = R_Weight_Lock -R_UnitWeight.origin;
		if(weight_temp >= CS_Scale_RetLockWeightDec)
			{
			
			R_Scale_state = CS_Scale_state_weighting;
			CS_Scale_LockProc(SetDowningLock);
			CS_Scale_TimeOutProc(CS_Reset_AllTime);
			
			}
		}

}

#define	StartClearMemWeightTimer	1
#define	MemWeightTimerInc			2
#define	MemWeightTimer			30

void CS_MemoryWeightClear_Timer(u8_t option)
{
	static u8_t xdata timecount; 				//��ʱ����
	static u8_t xdata B_Timeout;

	if(option==StartClearMemWeightTimer)
		{
		timecount=0;
		B_Timeout = false;
		}

	if(option==MemWeightTimerInc)
		{
		if(timecount<255 ) 
		timecount++;
		}
	
	if(B_Timeout == false)
		{
		if(timecount>SoftTimerTimeOut)
			{
			B_Timeout = true;
			R_Weight_Mem = 0;	//ʱ�䵽���������
			}
		}
}


///////////////////////////////////�궨����
#define	CS_CaliState_GetZero			0
#define	CS_CaliState_ShowCal1			1
#define	CS_CaliState_GetCal1			2
#define	CS_CaliState_ShowCal2			3
#define	CS_CaliState_GetCal2			4
#define	CS_CaliState_ShowCal3			5
#define	CS_CaliState_GetCal3			6
#define	CS_CaliState_ShowCal4			7
#define	CS_CaliState_SaveData			8
#define	CS_CaliState_ShowPass			9
#define	CS_CaliState_ShowErr			10

u16_t CS_Scale_CaliProc(u8_t option)
{
	static	u16_t xdata R_AD_CaliDot1;		//��һ���궨��ADֵ
	static	u16_t xdata R_AD_CaliDot2;		//�ڶ����궨��ADֵ
	static	u16_t xdata R_AD_CaliDot3;		//�������궨��ADֵ
	static	u8_t	  xdata R_CaliState;			//�궨״̬
	static	u16_t xdata R_OpenWeight_Ad;	//��������ADֵ
	
	u16_t	R_AdFilTimes;	
	u16_t	weight_temp;
	u16_t	R_AD_BUF;
	u16_t	R_AD_PowerOnZero;

	if(option==CaliProcReset)
		{
		R_CaliState=CS_CaliState_GetZero;
		return	0;
		}
	if(option==CaliProcSetCali)
		{
/*
		*(mem_sensor_buf )= 12;
		*(mem_sensor_buf+1) = 12;
		*(mem_sensor_buf+2 )= 12;
		*(mem_sensor_buf+3) = 12;
		*(mem_sensor_buf+4 )= 12;
		*(mem_sensor_buf+5) = 12;
		SpiFlashOperator(CS_CMD_WRITE_FLASH);
		//YC_Delay10ms(10);
		
		*(mem_sensor_buf )= 0;
		*(mem_sensor_buf+1) = 0;
		*(mem_sensor_buf+2 )= 0;
		*(mem_sensor_buf+3) = 0;
		*(mem_sensor_buf+4 )= 0;
		*(mem_sensor_buf+5) = 0;
		
		
		SpiFlashOperator( CS_CMD_READ_FLASH);
		
		
		
		R_Debug_temp=(u16_t) *(mem_sensor_buf+2 ) ;
		CS_ScaleDisplay_Debug();
		while(1);
		*/
		//YC_Delay1S(1);
		
		R_AD_BUF = *(mem_sensor_buf );
		R_AD_CaliDot1 =R_AD_BUF<<8 |  (*(mem_sensor_buf+1));
		R_AD_BUF = *(mem_sensor_buf+2 );
		R_AD_CaliDot2 =R_AD_BUF<<8 |  (*(mem_sensor_buf+3));
		R_AD_BUF = *(mem_sensor_buf+4 );
		R_AD_CaliDot3 =R_AD_BUF<<8 |  (*(mem_sensor_buf+5));	

		

		if(R_AD_CaliDot1<CS_CaliStateData_Down || R_AD_CaliDot1>CS_CaliStateData_Up)
			if(R_AD_CaliDot2<CS_CaliStateData_Down||R_AD_CaliDot2>CS_CaliStateData_Up)
				if(R_AD_CaliDot3<CS_CaliStateData_Down||R_AD_CaliDot3>CS_CaliStateData_Up)
					{
					R_AD_CaliDot1	= 6000;
					R_AD_CaliDot2	= 5000;
					R_AD_CaliDot3	= 4000;	
					}	
		
		
		R_OpenWeight_Ad	= R_AD_CaliDot1/10;
		return	0;		
		}
	if(option==CaliProcSetOpenWeight)
		{
		R_OpenWeight_Ad	= R_AD_CaliDot1/10;
		return	0;
		}
	
	if(option==CaliProcGetOpenWeight)
		return R_OpenWeight_Ad;
	if(option==CaliProcGetCaliDot1)
		return R_AD_CaliDot1;
	if(option==CaliProcGetCaliDot2)
		return R_AD_CaliDot2;
	if(option==CaliProcGetCaliDot3)
		return R_AD_CaliDot3;
	
	R_AD_BUF = R_AD_Original;
	R_AD_BUF=CS_Scale_AdcFilter(ScaleAdcFilterRun,R_AD_BUF);
	R_AdFilTimes = CS_Scale_AdcFilter(ScaleAdcFilterGetFilTimes,R_AD_BUF);
	
	

	//�����ȶ��Ŵ���
		
	if(R_CaliState==CS_CaliState_GetZero) 
		{	

			R_AD_CaliDot1 = R_AD_BUF;	//�ݴ����������ʾ
			if(R_AdFilTimes>=CS_Cali_Steadytimes)
				{
			CS_Scale_ZeroProc(SetPowerOnZero,R_AD_BUF);	
			CS_Scale_AdcFilter(ScaleAdcFilterReset,R_AD_BUF);
			CS_SoftTimer(ResetSoftTimer);
			R_CaliState = CS_CaliState_ShowCal1;
				}
		CS_ScaleDisplay_FreshBuf(DisplayData,R_AD_BUF);
		}
	
	if(R_CaliState == CS_CaliState_ShowCal1)
		{
		if(CS_SoftTimer(GetSoftTimerTime)>200) 	
			R_CaliState = CS_CaliState_GetCal1;
		CS_ScaleDisplay_FreshBuf(DisplayCal1,0);
		}

	if(R_CaliState==CS_CaliState_GetCal1)
		{	
			R_AD_PowerOnZero =CS_Scale_ZeroProc(GetPowerOnZero,0);	
			R_AD_CaliDot1 = R_AD_BUF - R_AD_PowerOnZero;
				
			if((R_AD_CaliDot1 < R_OpenWeight_Ad)||(R_AD_BUF < R_AD_PowerOnZero))
				{
				if(R_AdFilTimes>=CS_Cali_Steadytimes)
					{
					CS_Scale_ZeroProc(SetPowerOnZero,R_AD_BUF);	
					CS_Scale_AdcFilter(ScaleAdcFilterReset,R_AD_BUF);
					}
				R_AD_CaliDot1 = 0;
				}
			else
				{
				if((R_AD_CaliDot1> 750)&&(R_AdFilTimes>=60))
					{				
					CS_SoftTimer(ResetSoftTimer);
					CS_Scale_AdcFilter(ScaleAdcFilterReset,R_AD_BUF);
					R_CaliState = CS_CaliState_ShowCal2;
					}
				}
			CS_ScaleDisplay_FreshBuf(DisplayData,R_AD_CaliDot1);
			
		}

	if(R_CaliState == CS_CaliState_ShowCal2)
		{
		if(CS_SoftTimer(GetSoftTimerTime)>200) 	
			R_CaliState = CS_CaliState_GetCal2;
		CS_ScaleDisplay_FreshBuf(DisplayCal2,0);
		}
	
	if(R_CaliState==CS_CaliState_GetCal2)
		{		
			R_AD_PowerOnZero =CS_Scale_ZeroProc(GetPowerOnZero,0);		
			weight_temp = R_AD_BUF - R_AD_PowerOnZero;
					
			if((weight_temp < R_AD_CaliDot1)||(R_AD_BUF < R_AD_PowerOnZero))
				{
				R_AD_CaliDot2 = 0;
				}
			else
				{
				R_AD_CaliDot2 = weight_temp -R_AD_CaliDot1;
			
				if((R_AD_CaliDot2> 750)&& (R_AdFilTimes>=CS_Cali_Steadytimes))
					{
					CS_Scale_AdcFilter(ScaleAdcFilterReset,R_AD_BUF);
					CS_SoftTimer(ResetSoftTimer);
					R_CaliState = CS_CaliState_ShowCal3;
					}
				}
			CS_ScaleDisplay_FreshBuf(DisplayData,R_AD_CaliDot2);
		}

	if(R_CaliState == CS_CaliState_ShowCal3)
		{
		if(CS_SoftTimer(GetSoftTimerTime)>200) 	
			R_CaliState = CS_CaliState_GetCal3;
		CS_ScaleDisplay_FreshBuf(DisplayCal3,0);
		}
	
	if(R_CaliState==CS_CaliState_GetCal3)
		{		
			R_AD_PowerOnZero =CS_Scale_ZeroProc(GetPowerOnZero,0);		
			weight_temp = R_AD_BUF - R_AD_PowerOnZero;
					
			if((weight_temp < R_AD_CaliDot1)||(R_AD_BUF < R_AD_PowerOnZero))
				{
				R_AD_CaliDot3 = 0;
				}
			else
				{
				weight_temp = weight_temp -R_AD_CaliDot1;

				if(weight_temp < R_AD_CaliDot2)
					{
					R_AD_CaliDot3 = 0;
					
					}
				else
					{
				R_AD_CaliDot3 = weight_temp -R_AD_CaliDot2;
			
				if((R_AD_CaliDot3> 750)&& (R_AdFilTimes>=CS_Cali_Steadytimes))
					{
					CS_SoftTimer(ResetSoftTimer);
					R_CaliState = CS_CaliState_ShowCal4;
					}
					}
				}
			CS_ScaleDisplay_FreshBuf(DisplayData,R_AD_CaliDot3);
		}
	
	if(R_CaliState == CS_CaliState_ShowCal4)
		{
		if(CS_SoftTimer(GetSoftTimerTime)>200) 
			R_CaliState = CS_CaliState_SaveData;
		CS_ScaleDisplay_FreshBuf(DisplayCal4,0);
		}
	
	if(R_CaliState==CS_CaliState_SaveData)
		{
		
		R_AD_PowerOnZero =CS_Scale_ZeroProc(GetPowerOnZero,0);
		CS_Scale_ZeroProc(SetRunningZero,R_AD_PowerOnZero);
		R_Scale_state = CS_Scale_state_weighting;
		R_OpenWeight_Ad	=  R_AD_CaliDot1/10;

		/*
		R_Debug_temp=R_AD_CaliDot2;
		CS_ScaleDisplay_Debug();
		while(1);
		*/
		
		*(mem_sensor_buf )= HI_UINT16(R_AD_CaliDot1);
		*(mem_sensor_buf+1) = LO_UINT16(R_AD_CaliDot1);
		*(mem_sensor_buf+2 )= HI_UINT16(R_AD_CaliDot2);
		*(mem_sensor_buf+3) = LO_UINT16(R_AD_CaliDot2);
		*(mem_sensor_buf+4 )= HI_UINT16(R_AD_CaliDot3);
		*(mem_sensor_buf+5) = LO_UINT16(R_AD_CaliDot3);
		SpiFlashOperator(CS_CMD_WRITE_FLASH);

		

		}
	return 0;
	
}



///////////////////////////////////��������

#define	CS_Standby_S1_ToSleep				0
#define	CS_Standby_S2_WakeUp				1
#define	CS_Standby_S3_xxxx				2



void CS_Scale_StandbyProc(u8_t B_Reset)
{
	static u8_t xdata comm_doing;
	static u8_t xdata StandbyState;

	if(B_Reset==1)
		{
		StandbyState=CS_Standby_S1_ToSleep;
		comm_doing=false;
		return;
		}

	if(B_Reset==2)
		{
		StandbyState=CS_Standby_S2_WakeUp;
		comm_doing=false;
		return;
		}
	
	switch(StandbyState)
	{
	case CS_Standby_S1_ToSleep:
		if(comm_doing==false)
			{
			comm_doing=true;

			CS_Scale_1186ComSend(CS_CommTo1186_SetSleepMode);
			//sys_state = SYS_ST_IDLE;			//ϵͳ����˯��
			}
		if(CS_If1186ComSucess()==true)
			{
			comm_doing=false;
			StandbyState =  CS_Standby_S2_WakeUp;

			btCmdBuf.len = 0x01;
			btCmdBuf.DATA[0] = CS_CMD_SLEEP_REQ;
			BT_CmdFifoIn(&btCmdBuf);

			//btCmdBuf.len = 2;
			//btCmdBuf.DATA[0] = 0x20;
			//btCmdBuf.DATA[1] = 0;
			//BT_CmdFifoIn(&btCmdBuf);
			
			 
			sys_state = SYS_ST_IDLE;  //SYS_ST_SLEEP;			//ϵͳ����˯��
					
			}	
		break;
	case CS_Standby_S2_WakeUp:
		if(comm_doing==false)
			{
			comm_doing=true;
			CS_Scale_1186ComSend(CS_CommTo1186_ReadAdZero);
			}
		if(CS_If1186ComSucess()==true)
			{
			comm_doing=false;
			StandbyState =  CS_Standby_S1_ToSleep;	
			sys_state = SYS_ST_RUN;

			/*
			while(1)
			{
			R_Debug_temp = R_AD_Zero;
			CS_ScaleDisplay_Debug();
			}
			*/
			//����쳣״̬
			CS_Scale_JudgeOverLoad(ResetOverLoad);
			CS_Scale_TimeOutProc(CS_Reset_AllTime);
			R_Display_Err = 0;
			
			R_Scale_state = CS_Scale_state_weighting;

			CS_Scale_1186ComSend(CS_CommTo1186_ReadAd);
			
			}
		break;
	default:
		StandbyState = CS_Standby_S1_ToSleep;
		break;
	}
	
	//R_Debug_temp = 456;

}


///////////////////////////////////��������

#define	CS_PowerOn_State1_Reset				0
#define	CS_PowerOn_State2_ReadOtp				1
#define	CS_PowerOn_State3_ReadVersion			2
#define	CS_PowerOn_State4_ReadTime			3
#define	CS_PowerOn_State5_SetOpenWeight		4
#define	CS_PowerOn_State6_ShowOpenPic		5
#define	CS_PowerOn_State7_ShowOpenPicWait	6


void CS_Scale_PowerOnProc(bool B_Reset)
{
	static u8_t xdata comm_doing;
	static u8_t xdata PowerOnState;
	
	
	if(B_Reset)
		{
		PowerOnState=CS_PowerOn_State1_Reset;
		comm_doing=false;
		return;
		}
	
	//-----------S1
	if(PowerOnState==CS_PowerOn_State1_Reset)
		{
		if(comm_doing==false)
			{
			comm_doing=true;
			CS_Scale_1186ComSend(CS_CommTo1186_Reset);
			}
		else
			{
			comm_doing=false;
			PowerOnState	= CS_PowerOn_State2_ReadOtp;
			}		
		}
	//-----------S2
	if(PowerOnState==CS_PowerOn_State2_ReadOtp)
		{
		if(comm_doing==false)
			{
			comm_doing=true;
			CS_Scale_1186ComSend(CS_CommTo1186_ReadOtp);
			
			}
		if(CS_If1186ComSucess()==true)
			{
			comm_doing=false;
			PowerOnState =  CS_PowerOn_State3_ReadVersion;
			}
		}
	//-----------S3
	if(PowerOnState==CS_PowerOn_State3_ReadVersion)
		{
		PowerOnState =  CS_PowerOn_State4_ReadTime;		
		}
	//-----------S4
	if(PowerOnState==CS_PowerOn_State4_ReadTime)
		{
		if(comm_doing==false)
			{
			comm_doing=true;
			CS_Scale_1186ComSend(CS_CommTo1186_ReadTime);
			}
		if(CS_If1186ComSucess()==true)
			{
			comm_doing=false;
			PowerOnState =  CS_PowerOn_State5_SetOpenWeight;	
			}
		}
	//-----------S5
	if(PowerOnState==CS_PowerOn_State5_SetOpenWeight)
		{
		if(comm_doing==false)
			{
			comm_doing=true;
			CS_Scale_1186ComSend(CS_CommTo1186_SetOpenWeight);
			}
		if(CS_If1186ComSucess()==true)
			{
			comm_doing=false;
			PowerOnState =  CS_PowerOn_State6_ShowOpenPic;
			}
		}
	//-----------S6
	if(PowerOnState==CS_PowerOn_State6_ShowOpenPic)
		{
		if(comm_doing==false)
			{
			comm_doing=true;	
			CS_ScaleDisplay_FreshBuf(DisplayData,8888);
			CS_Scale_1186ComSend(CS_CommTo1186_LcdDisplay);
			}
		if(CS_If1186ComSucess()==true)
			{	
			comm_doing=false;
			PowerOnState =	CS_PowerOn_State7_ShowOpenPicWait;
			CS_SoftTimer(ResetSoftTimer);
			}		
		}
	//-----------S7
	if(PowerOnState==CS_PowerOn_State7_ShowOpenPicWait)
		{
		if(CS_SoftTimer(GetSoftTimerTime)>200)
			{
			R_Scale_state = CS_Scale_state_weighting;
			CS_Scale_1186ComSend(CS_CommTo1186_ReadAd);
			}
		}

}




u8_t CS_SoftTimer(u8_t option)
{
	static u8_t xdata timecount; 				//��ʱ����
	static u8_t xdata B_Timeout;

	if(option==ResetSoftTimer)
		{
		timecount=0;
		B_Timeout = false;
		}

	if(option==GetSoftTimerTime)
		return	timecount;

	if(option==SoftTimerInc)
		{
		if(timecount<255 ) 
		timecount++;
		}
	
	
	if(timecount>SoftTimerTimeOut)
		B_Timeout = true;

	if(option==GetIfTimeOut)
		return	B_Timeout;
	
	return 0;

}





///////////////////////////////////��ʱ����


typedef struct _CS_TimeOut_T{
	u8_t zero; 	
	u8_t lock;
	u8_t unstable;
	u8_t overload;
	u8_t lowbat;
}CS_TimeOut_T;



void CS_Scale_TimeOutProc(u8_t option)
{
	static	CS_TimeOut_T xdata R_TimeOut;
	static	u8_t xdata B_TimeBase_1s;		//1sʱ��

	if(option==CS_SetTimeBase1S)
		{
		B_TimeBase_1s = true;
		return;
		}
	//��ʱ��������λ
	if(option)
	{
	if(option==CS_Reset_ZeroTime)
		R_TimeOut.zero=0;
	if(option==CS_Reset_LockTime)
		R_TimeOut.lock=0;
	if(option==CS_Reset_UnstableTime)
		R_TimeOut.unstable=0;
	if(option==CS_Reset_OverLoadTime)
		R_TimeOut.overload=0;
	if(option==CS_Reset_LowBatTime)
		R_TimeOut.lowbat=0;
	if(option==CS_Reset_AllTime)
		{
		R_TimeOut.zero=0;
		R_TimeOut.lock=0;
		R_TimeOut.unstable=0;
		R_TimeOut.overload=0;
		R_TimeOut.lowbat=0;
		}
	return;
	}
		
	if(B_TimeBase_1s == true)
	{
		B_TimeBase_1s = false;

		//�͵糬ʱ����
		if(CS_Scale_JudgeLowBat(GetIfLowBat) == true)
		{
			R_TimeOut.lowbat++;

			if(R_TimeOut.lowbat>=CS_Scale_TimeOut_LowBat)
			{
				R_Scale_state = CS_Scale_state_standby;
				CS_Scale_StandbyProc(1);
				R_TimeOut.lowbat=0;
			}
		return;			
		}

		//���س�ʱ����
		if(CS_Scale_JudgeOverLoad(GetIfOverLoad)== true)
		{
			R_TimeOut.overload++;

			if(R_TimeOut.overload>=CS_Scale_TimeOut_OverLoad)
			{
				R_Scale_state = CS_Scale_state_standby;
				CS_Scale_StandbyProc(1);
				R_TimeOut.overload=0;
			}
		return;	

		}
		
		
		//����ģʽ�µĳ�ʱ����
		if(R_Scale_state == CS_Scale_state_weighting)
		{
		
		if(CS_Scale_ZeroProc(GetZeroState,0) == true)
			{
			R_TimeOut.unstable=0;
			R_TimeOut.zero++;
			}
		else
			{
			R_TimeOut.zero=0;
			R_TimeOut.unstable++;
			}

		if(R_TimeOut.zero >=CS_Scale_TimeOut_Zero||R_TimeOut.unstable>=CS_Scale_TimeOut_Unstable)
			{
				//R_Debug_temp=R_TimeOut.unstable;
				//CS_ScaleDisplay_Debug();
				//while(1);
				
				R_Scale_state = CS_Scale_state_standby;
				CS_Scale_StandbyProc(1);
				R_TimeOut.zero=0;
				R_TimeOut.unstable=0;
			}
		return;

		}
		

		//����ģʽ�µĳ�ʱ����
		if(R_Scale_state == CS_Scale_state_locking)
		{
			R_TimeOut.unstable=0;
			R_TimeOut.lock++;
			
			if(R_TimeOut.lock>=CS_Scale_TimeOut_Lock)
			{
				
				R_Scale_state = CS_Scale_state_standby;
				CS_Scale_StandbyProc(1);
				R_TimeOut.lock=0;
			}
		return;
		}		
		

	}

}










