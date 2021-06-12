#include "sys.h"
#include "IIC.h"
#include "Delay.h"
#include "PowerManage.h"
#include "Buzzer.h"
#include "EEPROM.h"



	
//��EEPROM��û�����ݻ������ݲ����������������Ĭ������
const struct sys_config default_config = {
	//Ĭ����ʾ����
	10,						//�Զ�Ϣ��ʱ��
	5,						//�Զ�����ʱ��
	50,						//��Ļ���ȣ�0~255��
	NO_INVERSED,	//��Ļ�Ƿ�ɫ
	NORMAL,				//��Ļ����ʾ����
	OFF,					//�رհ�����
	//Ĭ�����Ӳ���
	8,							//Сʱ
	30,							//����
	1,							//1��
	1,							//��һ
	ALARM_DISABLE,	//���ӹر�
	//Ĭ������������
	1,			//����������
	97.1,		//������Ƶ��
	//Ĭ��У������
	0,			//x�����ƫ��
	0,			//y�����ƫ��
	0,			//������У��ϵ��
	0,			//������У��ϵ��
	0,			//������У��ϵ��
	1,			//������У��ϵ��
	1,			//������У��ϵ��
	{{0}, {0}, {0}, {0}, {0}, {0}, {0}},	//�Ʋ�������ʷ���ݣ���Ź�ȥ����Ĳ���
	0				//������������Ҫ�޸�
};

extern struct sys_config config;	

              
void Timer0Init(void)
{
	TMOD |= 0x04;   //�ⲿ����ģʽ
	TL0 = 0x00;
	TH0 = 0x00;
	TR0 = 0;       //�رն�ʱ��
	ET0 = 1;       //ʹ�ܶ�ʱ���ж�
}
void Timer3Init(void)    //��ʱ1ms   24MHZ
{
	T4T3M |= 0x02;     //T4R T4_C/T T4x12 T4CLKO ��ʱ��ʱ��1Tģʽ��CPUʱ�Ӳ���Ƶfosc/1
	T3L = 0x40;        //���ö�ʱ����ֵ
	T3H = 0xA2;        //���ö�ʱ����ֵ
	T4T3M |= 0x08;     //��ʱ��3��ʼ��ʱ
	IE2 |= ET2;        //��ʱ��3������ж�
}
void InitConfig(void)
{
	IT0 = 1;    //ʹ��INT0�½����жϣ���ӦPCF8653���ж��������
	EX0 = 1;    //ʹ��INT0�ж�
	IT1 = 1;    //ʹ��INT1�½����жϣ���ӦLSM6DSM���ж����INT1
	EX1 = 1;    //ʹ��INT1�ж�
	INTCLKO |= EX2; //ʹ��INT2�ж�    ��ӦLSM6DDSM���ж��������2
  INTCLKO |= EX3;  //ʹ��INT3�ж�    ��Ӧ����2�жϣ�
  //INTCLKO |= EX4;  //ʹ��INT4�ж�	
}

//�����ڶ���ú���ʱʹ����ifdef ��else
void UartInit(void)       //115200bps@24.000MHz
{
	SCON = 0x50;  // SM0 SM1 SM2 REN TB RB8 TI RI 8λ���ݣ��ɱ䲨����
	AUXR |= 0x40;  //   AUXR�����Ĵ�����T1x12Ϊ1����CPUʱ�Ӳ���Ƶ����Ϊ0����Ϊ12��Ƶ
	AUXR &= 0xFE;  //  ��bit0�����⣬����λ����ԭ�����ò���
	TMOD &= 0x0F;  //   ��4λ GATE C/T M1 M0    ����λ���㣬����λ���ֲ���(T1��������-��gate,T1����Ϊ��ʱ������ϵͳ�ڲ�ʱ�ӽ��м�����������Ϊ16λ�Զ���װģʽ��T0���ֲ���)
	TL1 = 0xCC;    //   0xcc  204
	TH1 = 0xFF;    //   0xff  255    ÿ����װ�������Ƕ���
	ET1 = 0;       //   ��ʱ��/������T1������ж�����λ����ֹT1�ж�
	TR1 = 1;       //   �򿪶�ʱ��1
	TI = 1;        //    ģʽ0�У������ڷ������ݵ�8λ����ʱ����Ӳ���Զ���TI��1,�����������жϣ���Ӧ�жϺ�TI�������������㣬����ģʽ������ֹͣλ��ʼ����ʱ��Ӳ���Զ���1����CPU�������жϣ���Ӧ�����������㣬ģʽ0��ͬ����λ���з�ʽ
}

void Uart2Init(void)    //9600bps  24MHZ
{
	S2CON = 0x50;  //8λ���ݣ��ɱ䲨����
	AUXR |= 0x04;  //  ��ʱ��2ʼ��δFosc,��1T��ѡ��ʱ��1��Ϊ�����ʷ�����
	T2L = 0x8F;   //�趨��ʱ����ʼֵ
	T2H = 0xFD;   //�趨��ʱ����ֵ
	AUXR |= 0x10;  //������ʱ��2
	IE |= ES2;     //ʹ�ܴ����ж�
}


void PinInit()
{
	//P0����ΪOLED��Ļ�Ĳ��нӿ�
	P0 = 0x00;
	P0M0 = 0x00;
	P0M1 = 0x00;
	//ÿ��IO�ڶ�����ʹ�������Ĵ����������ã�����P0����Ҫ�õ�P0MO,P0M1,P0�����е�IO����Ϊ׼˫��ڣ�20mA��������
	//P10,P11 �Ǵ���2
	//P12,P13Ϊ��ʾ��D/C,RES���ţ�(V1.0:P12=CS,P14=D/C)
	//P14,P15Ϊ IIC���ţ��ⲿ�������������裬������Ϊ��©          
	//P16Ϊ����ʹ�����ţ��ߵ�ƽʱ����͹���
	//P17Ϊ���ʹ�����ţ�����ʱʹ�ܳ�磨V1.0 ����Ҫ������֧�֣�
	//P13 = 1; //��ʾ����RES ����   ��V1.0�汾ΪD/C��
	P13 = 1;    
	P16 = 1; 
	P1M0 = 0x30;  //00110000b
	P1M1 = 0xb0;  //10110000b
	//P20ΪLED�������ţ�ǿ����ģʽ����V1.0 ΪP35��
	//P21,P22Ϊ�ⲿ��չ���ţ� 
	//P23Ϊ����UP
	//P24,P25ΪI2C���ţ��ⲿ���������裬������Ϊͨ��˫��
	//P26Ϊ�������������ţ�ǿ����ģʽ
	//P27Ϊ����,Ĭ�ϸߵ�ƽ
	P2 = 0xb9;  //10111001b
	P2M0 = 0x41;//01000001b
	P2M1 = 0x00;    
	//P37Ϊ�����������ⲿ�ж�3��Ĭ�ϸߵ�ƽ��
	//P36Ϊ�ⲿ�ж�2
	//P34ΪT0������������    ����Ϊ1������
	//P33Ϊ�ⲿ�ж�1         
	//P32Ϊ�ⲿ�ж�0         ����Ϊ1������
	//P30,P31Ϊ����1����
	P3 = 0x94;//1001 01       00b
	P3M0 = 0x00;
	P3M1 = 0x00;    
	//P44,P43Ϊ��ʾ��E/RD,R/W����
	//P40Ϊ3.3Vʹ���������
	P4 = 0x19; //00011000b
	P4M0 = 0x00;
	P4M1 = 0x00;
	//P55Ϊ������Ĭ�ϸߵ�ƽ   DOWN
	P5 = 0x20;    //0010 0000b
	P5M0 = 0x00;
	P5M1 = 0x00;
}
void EnableWatchDog(void)
{ 
	WDT_CONTR = 0x27;            //�������4.194sû��ι����MCU�Զ���λ
}
void FeedWatchDog(void)
{
	WDT_CONTR |= 0x10;           //ι����������Ź�������
}

//void EEPROMWriteConfiguration(struct sys_config *config)
//{
//	unsigned char i= 0 ;
//	unsigned int temp = 0;
//	for(i; i < CONFIG_SIZE -2 ; i++)      //�Խṹ����ÿһ�ֽ����
//		temp += ((unsigned char *)config)[i];
//	config->check_sum = temp;
//	EEPROM_SectorErase(EE_ADDRESS1);     //����ͽ�����ڽṹ�����һ��������
//	EEPROM_write_n(EE_ADDRESS1,(unsigned char *)config,CONFIG_SIZE);
//	
//}



void EEPROMWriteConfiguration(struct sys_config *config)
{
	unsigned char i = 0;
	unsigned int temp = 0;
	for(i; i < CONFIG_SIZE - 2; i++)				//�Խṹ����ÿһ�ֽ����
		temp += ((unsigned char *)config)[i];
	config->check_sum = temp;
	EEPROM_SectorErase(EE_ADDRESS1);				//����ͽ������ڽṹ�����һ��������
	EEPROM_write_n(EE_ADDRESS1, (unsigned char *)config, CONFIG_SIZE);
}
	
unsigned char EEPROMReadConfiguration(struct sys_config *config)
{
	unsigned char i = 0;
	unsigned int temp = 0;
	EEPROM_read_n(EE_ADDRESS1,(unsigned char *)config,CONFIG_SIZE);
	for(i; i < CONFIG_SIZE - 2; i++)    //�Խṹ����ÿһ�ֽ����
		temp += ((unsigned char *)config)[i];
	if(temp == config->check_sum)        //���������Ƿ���ȷ�������ṹ��������ֽ�Ϊ��������ֽ�
			return 1;
	else
	{
		for(i = 0;i < CONFIG_SIZE; i++)
		((unsigned char *)config)[i] = ((unsigned char *)(&default_config))[i];
		return 0;
	}
}

void LED(unsigned char k)
{
	LED1 = k^0x01;     //����ΪʲôҪ��򣬶�����ȡ��
}

void SysInit(void)
{
	PinInit();   //���ų�ʼ��
	UartInit();   //����1��ʼ��,ʹ�ö�ʱ��1
	Uart2Init();  //����2��ʼ����ʹ�ö�ʱ��3
	Timer0Init();  //��ʱ��0��ʼ�����ⲿ����ģʽ
	Timer3Init();  //��ʱ��3��ʼ��������1ms����ж�
	InitConfig(); //�жϳ�ʼ��
	ADCInit();    //ADC��ʼ�� ���ɼ���Դ��ѹ
	IIC_Init();   //Ӳ��IIC��ʼ��
	EnableWatchDog();   //ʹ�ܿ��Ź���������4s���Զ���λ
	EEPROMReadConfiguration(&config);//�����籣������
	BuzzerInit();  //��������ʼ����ʹ��Ӳ��PWM7
	EnableBuzzer(config.key_sound);
	LED(OFF);
	BatteryChargeEnable(1);  //������س��
	Delay1ms(5);  
	Enable3V3Output(1);  //����3.3V﮵�������
	EA = 1;         //��ȫ���ж�
}




































