#include "Buzzer.h"
#include "stc8a.h"


sbit buzzer = P2^6;     //��V1.1�����ͬ�������޸�

#define BUZZER_FQ 4000     //��������г��Ƶ��Ϊ4khz
#define Focs  24000000     //ϵͳƵ��Ϊ24mhz
#define Fdiv  1						 //PWM��������Ƶϵ��
#define PWM_CNT (Focs / (BUZZER_FQ *Fdiv))
#define PWM_duty   0.5

bit bee_en = 0;
unsigned char bee_mode = 0;
unsigned int bee_time = 0;


void BuzzerInit(void)
{
	P_SW2 |= 0x80;        //P_SW2Ϊ����˿ڼĴ���2���������⹦�ܼĴ���ǰ����Ҫ�Ƚ�P_SW2(BAH)�Ĵ��������λ��1
	//����PWM��Ƶ��
	PWMCKS = 0x00;        //PWM������ʱ��Ϊϵͳʱ�ӵ�1��Ƶ
	PWMC = PWM_CNT;        //����PWM������
	//ʹ��PWM����
	PWM6CR = 0x80;
	//����PWMռ�ձȣ�
	PWM6T1 = 0x0000;
	PWM6T2 = (unsigned int )(PWM_CNT * PWM_duty);
	//PWMCR = 0x80;      //PWM��������ʼ����
	P_SW2 &= ~0x80;

  T4T3M |= 0x20 ;        //��ʱ��ʱ��1Tģʽ
	T4L = 0x40;          //���ö�ʱ��ֵ
	T4H = 0xA2;         	//���ö�ʱ��ֵ
	IE2 |= ET4; 
	bee_en = 0;
}
void BuzzerOnOff(unsigned char b)
{
	P_SW2 |= 0x80;
	if(b == 1)
	{
		PWMCR |= 0x80;   //��������ʼ����
	}
	else
	{
		PWMCR &= ~0x80;   //������ֹͣ����
		buzzer = 0;
	}
	P_SW2 &= ~0x80;
}
void TM4_Isr()   interrupt 20 using 1
{
	AUXINTIF &= ~T4IF;   //���жϱ�־
	if(bee_mode == 0)
	{
		if(bee_time == 200)
		{
			bee_time = 0;
			BuzzerOnOff(0);
			T4T3M &= ~0x80;
		}
	}
	else if(bee_mode == 1) 
	{
		if(bee_time < 600)
		{
			if(bee_time % 100 ==0)
			{
				BuzzerOnOff(((bee_time / 100) % 2 ) ^ 0x01);
				P20 = ((bee_time / 100 ) % 2);
			}
		}
		if(bee_time == 1000)
		{
			bee_time = 0;
		}
	}
}
void Bee(void)
{
	bee_mode = 0;
	bee_time = 0;
	if(bee_en)
	{
		BuzzerOnOff(1);
		T4T3M |= 0x80;
	}
}
void Beebeebee(void)
{
	bee_mode = 1;
	bee_time = 0;
	//BuzzerOnOff(1);
	T4T3M |= 0x80;
}
void EnableBuzzer(unsigned char k)
{
	if(k)
		bee_en = 1;
	else 
		bee_en = 0;
}





