#include "stc8a.h"
#include "PowerManage.h"
#include "sys.h"
#include "intrins.h"



//���������ط��ĺ���
int *BGV = (int  idata *)0xef;     

void ADCInit(void)
{
	ADCCFG = 0x2f;   //  speed[3:0] ����ADCʱ��Ϊϵͳʱ��/2/16/speed
	ADC_CONTR = 0x8f;   //ʹ�� ADCģ�飬ѡ���16ͨ��
}
/*
   * @brief   �ɼ�ģ��ͨ����ADCת��ֵ
   * @param   ch: ͨ����
   * @retval res ��ת�������12λ��Ч
*/
int ADCRead(unsigned char adc_ch)
{
	int res;
	ADC_CONTR = 0x80 | adc_ch;
	ADC_CONTR = 0x40;                //����ADת��
	_nop_();
	_nop_();
	while(!(ADC_CONTR & 0x20));      //��ѯADC��ɱ�־   ADC_FLAG ADCת����������жϱ�־���������
	ADC_CONTR &= ~0x20;              //����ɱ�־
	res = (ADC_RES <<8 ) | ADC_RES;	//��ȡADC���
	return res;
}
/**
  * @brief  3.3V��Դʹ��
  * @param  k: 0 �ر�3.3V��Դ  
               1 ��3.3V��Դ
  * @retval  ��
*/
void Enable3V3Output(unsigned char k)
{
	EN_3V3 = k;
}


/**
  * @brief  ���ʹ��
  * @param  k��
              0 ��ֹ���
              1 ������
  * @retval ��
*/
void BatteryChargeEnable(unsigned char k)
{
	if(k)
	{
		P1M1 |= 0x80;          //P17����Ϊ����
	}
	else
	{
		P1M1 &= ~0x80;    //P17����Ϊͨ��˫��
		PROG = 0;         //����PROG����
	}  
}
/**
  * @brief  ����MCU��ǰ��Դ��ѹ
  * @param  ��
  * @retval ��ѹֵ����λV
*/
float GetBatteryVoltage(void)
{
	long res =0;
	int i = 0;
	int vcc = 0;
	ADCRead(BAT_CH);
	ADCRead(BAT_CH);    //����ǰ���ν��
	for(i = 0; i < 64; i ++)
	{
		res += ADCRead(BAT_CH);
	}
	res >>= 6;    //res /= 64;
	vcc = (int)(4095L * *BGV /res);
	return vcc /1000.0;
}
/**
  * @brief  ������ǰ���ʣ���������������Ѿ��������˲�
  * @param  ��
* @retval  percentage :ʣ������ٷֱ�
*/
#define V_SHUTDOWN   3.4   //�ŵ��ֹ��ѹ
#define V_FULLCHARGE  4.13  //�����ѹ
#define WINDOW_WIDTH  20  //�����˲����

float GetBatteryLife(void)
{
	static unsigned char first_time_flag = 0;
	static float queue[WINDOW_WIDTH];    //�����˲�����
	float queue_average = 0;             //���о�ֵ
	float percentage;
	unsigned char i = 0;
	percentage = (GetBatteryVoltage()-V_SHUTDOWN) / (V_FULLCHARGE - V_SHUTDOWN);
	if(percentage > 1)
	{
		percentage = 1;
	}
	else if(percentage < 0)
	{
		percentage = 0;
	}
	if(first_time_flag == 0)
	{
		first_time_flag = 1;
		for(i = 0; i < WINDOW_WIDTH; i ++)
		queue[i] = percentage;
	}
	for(i = WINDOW_WIDTH - 1; i > 0; i--)
	{
		queue[1] = queue[i-1];
		queue_average += queue[i];
	}
	queue_average += percentage;;
	percentage = queue_average / WINDOW_WIDTH;
	queue[0] = percentage;
	return percentage;
}
/**
  * @brief  ���øú���ʹMCU�������ģʽ  ��Ҫ���׸Ķ���������������Ӱ�칦��
  * @param  ��
  * @retval  ��
*/
void SystemPowerDown(void)
{
	unsigned char temp = P1;
	P0 = 0x00;
	P1 = 0x00;
	P1M0 = 0x30;
	P1M1 = 0xb0;
	P2M0 = 0x01;
	P2M1 = 0x00;
	P3 = 0x84;
	P4 = 0x99;
	P5 = 0x00;
	ET0 = 0;
	IE2 &= ~ES2;   //�رմ���2�ж�
	IE2 &= ~ET3;    //��ʱ��3������ж�
	PCON = 0x02;     //MCU�������ģʽ
	_nop_();
	_nop_();
}
/**
	* @brief 	���øú���ʹϵͳ������ȵ���ģʽ
	*					��Ҫ���׸Ķ���������������Ӱ�칦��
	* @param  ��
	* @retval ��
	*/
void SystemDeepPowerDown(void)
{
	unsigned char temp = P1;
	P0 = 0x00;
	P1 = 0x00;
	P1M0 = 0x30;
	P1M1 = 0xb0;
	P2 = 0x01;
	P2M0 = 0x00;
	P2M1 = 0x00;
	P3 = 0x84;
	P4 = 0x00;
	P5 = 0x00;
	ET0 = 0;
	IE2 &= ~ES2; 		//�رմ���2�ж�
	IE2 &= ~ET3;		//��ʱ��3������ж�
	PCON = 0x02;		//MCU�������ģʽ
	_nop_();
	_nop_();
	P1 = temp;
}
/**
  * @brief  ���øú�����ϵͳ�ӵ���ģʽ������ȵ���ģʽ�лָ���������Ҫ���׸Ķ��������������Ӱ�칦��
  * @param  ��
  * @retval  ��
*/
void SystemPowerOn(void)
{
	PinInit();
	ET0 = 1;     //
	IE2 |= ES2;  //ʹ�ܴ���2�ж�
	IE2 |= ET3;  //ʹ�ܶ�ʱ��3����ж�
}


















