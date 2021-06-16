#include "Sys.h"
#include "bmp.h"
#include "intrins.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "Delay.h"
#include "PowerManage.h"
#include "Buzzer.h"
#include "Display.h"
#include "PW02.h"
#include "PCF8563.h"
#include "BME280.h"                  
#include "HMC5883L.h"                 
#include "RDA5807M.h"
#include "LSM6DSM.h"
#include "Ellipsoid fitting.h"




/***********************״̬����ر���*********************/
enum function func_num = WATCH;    //���ܺ�
char func_index;               //��������
bit ON_OPEN = 0;
bit ON_CLOSE = 0;
bit ON_RETURN = 0;
/********************************************************/
/************************ʱ���ź�************************/
bit tick_1ms = 0;             //1000Hz
bit tick_8ms = 0;             //125hz
bit tick_20ms = 0;     		    //50hz

/*************************����״̬��ر���*******************************/
bit action = 0;             //��̬��־λ
bit active_flag =1;   		//���־λ
bit sleep_flag = 0; 				//˯�߱�־λ
bit powerdown_flag = 0;     //�����־λ
bit deep_powerdown_flag = 0;		//��ȵ����־λ
unsigned int inactive_time =0;  //�Զ�Ϣ����ʱ��
unsigned int sleep_time = 0;		//�Զ�����ʱ��
unsigned int autowake_cnt = 0;  //�Զ����ѵĴ���
bit screen_on_flag = 1;			//��Ļ״̬��־λ
bit PCF8563_int_flag = 0;		//pcf8563�жϲ�����־λ
bit alarm_flag = 0;					//�����־λ
bit radio_on_flag = 0;			//������������־λ
bit timer_on_flag = 0; 			//��ʱ��������־λ
/*************************������ر���*********************/
unsigned char Trg = 0;      //��������ֵ
unsigned char Cont = 0;			//��������ֵ
/*************************��ʾ����*******************************/
extern unsigned char xdata main_cache[];
extern unsigned char xdata sub_cache1[];
extern unsigned char xdata sub_cache2[];          

struct sys_config config;   //������Ϣ�ṹ��
struct pcf8563_time RTC;		//ʱ����Ϣ�ṹ��
struct bme280_data BME280;	//bme280���ݽṹ��
struct lsm6dsm_data LSM6DSM;//LSM6DSM ���ݽṹ��
struct cal_data magnet_cal_data;  //�����ƽ������ݽṹ��
float battery_life;					//��ص���
unsigned int  timer_cnt = 0;//��ʱ��0�������


void KeyScan(void)
{
	unsigned char dat = 0x00;
	if(K1 == 0)
		dat |= KEY1;
	if(K2 == 0)
		dat |= KEY2;
	if(K3 ==0)
		dat |= KEY3;
	Trg = dat&(dat^Cont);
	Cont = dat;
}

void TM3_Ist()  interrupt 19 using 1 		//	mcu��ʱ���ж�
{
	static unsigned int t_count = 0;
	static unsigned int t_key_press =0; 
	AUXINTIF &= ~T3IF; 										//����жϱ�־λ
	if((K1 == 0) && (K2 == 0) && (K3 == 0))
	{
		while ((K1 == 0) ||(K2 == 0) ||(K3 == 0) ); //����������̧��
			MCUSoftReset();                    		  //��λMCU
	}
		if(t_count % 50 == 0)
	{
		KeyScan();
		if(Trg)
		{
			Bee();
			action = 1;
			if(sleep_flag || powerdown_flag || alarm_flag)
			{
				Trg = 0;
				if(alarm_flag)
				{
					alarm_flag = 0;
					LED(OFF);
				}
			}
		}
		if(Cont)
		{
			if(++t_key_press > 10)	//�ж��Ƿ񳤰�
				Trg = Cont;						//�ظ�����
		}
		else
			t_key_press = 0;
	}
	tick_1ms = 1;
	if(t_count % 8 == 0)
		tick_8ms = 1;
	if(t_count % 20 == 0)
		tick_20ms = 1;
	if(++t_count == 1000)
	{
		t_count = 0;
		if(active_flag)
		{
			if(++inactive_time == config.t_inactive_max)
			{
				active_flag = 0;
				inactive_time = 0;
				sleep_flag = 1;
			}
		}
		if(sleep_flag)
		{
			if(++sleep_time == config.t_sleep_max)
			{
				sleep_flag = 0;
				sleep_time = 0;
				powerdown_flag = 1;
				ON_CLOSE = 1;
			}
		}
	}
}

//void INT0_Isr() interrupt 0						//PCF8563�ж�
//{
//	PCF8563_int_flag = 1;
//}
void TM0_Isr() interrupt 1 using 1		//������0����ж�
{
	timer_cnt++;
}
void INT1_Isr() interrupt 2 using 1		//˫���ж�
{
	action = 1;
	alarm_flag = 0;
	Bee();
	if(sleep_flag || powerdown_flag || alarm_flag)
		Trg = 0;
	else
		Trg = DOUBLE_TAP;
	if(active_flag && func_num == WATCH)
	{
		action = 0;
		active_flag = 0;
		inactive_time = 0;
		ScreenOnOff(OFF);
		sleep_flag = 1;
	}
}
void INT2_Isr() interrupt 10 using 1	//̧�����ж�
{
	action = 1;
	if(sleep_flag || powerdown_flag)
	{Trg = 0;}
  else 
  {Trg = AWT;}
}
void INT3_Isr() interrupt 11					//����2�ж�
{
	action = 1;
}
void SensorInit(void)
{
	unsigned char y = 0;
	unsigned int time_out = 700;
	unsigned char error_count = 0;
	if(BME280Init() == 0)
	{
		y += ShowString(0, y, "BME280 ERROR", main_cache, FONT8X16, NO_INVERSED, 1);
		error_count++;
	}
	if(HMC5883L_Init() == 0)
	{
		y += ShowString(0, y, "HMC5883L ERROR", main_cache, FONT8X16, NO_INVERSED, 1);
		error_count++;
	}
	if(LSM6DSMInit() == 0)
	{
		y += ShowString(0, y, "LSM6DSM ERROR", main_cache, FONT8X16, NO_INVERSED, 1);
		error_count++;
	}
	if(RDA5807MInit() == 0)
	{
		y += ShowString(0, y, "RDA5807M ERROR", main_cache, FONT8X16, NO_INVERSED, 1);
		error_count++;
	}
	if(error_count != 0)
	{
		while(Trg == 0)
		{
			Delay1ms(1);
			if(--time_out == 0)
				break;
		}
	}
}


void main()
{	
	//LED(ON);
	SysInit();						//ϵͳ��ʼ��
	DisplayInit(&config);	//��ʾ��ʼ��     
//	PCF8563Init();				//ʵʱʱ�ӳ�ʼ��        
//	SensorInit();					//��������ʼ��
	ClearCache(main_cache);	//������Դ�
	while(1)
	
	{
		FeedWatchDog();			//ι���Ź�
		
	
  /********
		if(func_num == WATCH)						//��ʾ����
		{
			unsigned char x, n, m;
			unsigned char str[16];
			if(ON_OPEN)			//����ù���ʱִ�е�����
			{
				ON_OPEN = 0;
				if(func_index == FIRST_FUNC)
					ScreenPushAnimation(sub_cache1, DOWN);
				else if(func_index == LAST_FUNC)
					ScreenPushAnimation(sub_cache1, UP);
				func_index = WATCH;
			}
			if(ON_RETURN)		//���ظù���ʱִ�е�����
			{
				ON_RETURN = 0;
			}
			
			if(tick_20ms)		//�ڹ�����ʱ��ÿ20msִ��һ�ι�������
			{
				tick_20ms = 0;
				//��ʾʱ��
				PCF8563ReadTime(&RTC);
				//printf("%d:%d:%d\n", (int)RTC.hour, (int)RTC.minute, (int)RTC.second);
				DisplayTime(RTC.hour, RTC.minute, RTC.second);
				//��ʾСͼ��
				ClearCacheArea(0, 0, 35, 8, main_cache);
				x = 0;
				BMPToCache(x, 0, 8, 8, BLUETOOTH_SMALL_ICON, main_cache, 0);
				x += 9;
				if(timer_on_flag)
				{
					BMPToCache(x, 0, 8, 8, TIMER_SMALL_ICON, main_cache, 0);
					x += 9;
				}
				if(config.alarm_mode != ALARM_DISABLE)
				{
					BMPToCache(x, 0, 8, 8, CLOCK_SMALL_ICON, main_cache, 0);
					x += 9;
				}
				if(radio_on_flag)
					BMPToCache(x, 0, 8, 8, RADIO_SMALL_ICON, main_cache, 0);
				//��ʾ���ڣ�����
				ClearCacheArea(0, 7, 128, 8, main_cache);
				sprintf(str, "%d/%d/%d ", (int)RTC.year, (int)RTC.month, (int)RTC.day);
				strcat(str, WEEKDAY_IN_STR[RTC.weekday - 1]);
				ShowString(0, 7, str, main_cache, FONT6X8, NO_INVERSED, 0);
				//��ʾ����
				battery_life = GetBatteryLife();
				for(n = 0; n < 24; n++)
					str[n] = BATTERY_LIFE_ICON[n];
				m = 18 * battery_life;
				for(n = 2; n < 2 + m; n++)
					str[n] |= 0x3c;
				BMPToCache(104, 0, 24, 8, str, main_cache, 0);
				sprintf(str, "%3d%%", (int)(battery_life * 100));
				ShowString(78, 0, str, main_cache, FONT6X8, NO_INVERSED, 1);
			}
			
			if(Trg != 0)		//�ڸù����¶԰����¼��Ĵ���
			{
				switch(Trg)
				{
					case KEY1:
					{
						func_num = SUB_MENU;
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						func_index = FIRST_FUNC;
						func_num = MENU;
						ON_CLOSE = 1;
						break;
					}	
					case KEY3:
					{
						func_index = LAST_FUNC;
						func_num = MENU;
						ON_CLOSE = 1;
						break;
					}
					case KEY13:
					{
						active_flag = 0;
						sleep_flag = 0;
						powerdown_flag = 0;
						inactive_time = 0;
						sleep_time = 0;
						deep_powerdown_flag = 1;
						ON_CLOSE = 1;
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)		//�رոù���ʱִ�е�����
			{
				ON_CLOSE = 0;
				SaveScreen();
				ON_OPEN = 1;
			}
		}
		else if(func_num == MENU)				//���˵�		ok
     		{
			if(ON_OPEN)
			{
				ClearCache(sub_cache2);
				BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, COVER);
				if(func_index == FIRST_FUNC)
					ScreenPushAnimation(sub_cache2, UP);
				else
					ScreenPushAnimation(sub_cache2, DOWN);
				ON_OPEN = 0;
			}
			if(ON_RETURN)
			{
				ON_RETURN = 0;
				ClearCache(sub_cache2);
				BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, COVER);
				ScreenPushAnimation(sub_cache2, RIGHT);
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						goto case_AWT;
						break;
					}
					case KEY2:
					{
						if(++func_index > FUNC_NUM_MAX - 1)
						{
							func_index = LAST_FUNC;
							func_num = WATCH;
							ON_CLOSE = 1;
						}
						else
						{
							ClearCache(sub_cache2);
							BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, 0);
							ScreenPushAnimation(sub_cache2, UP);
						}
						break;
					}
					case KEY3:
					{
						if(--func_index < 0)
						{
							func_index = FIRST_FUNC;
							func_num = WATCH;
							ON_CLOSE = 1;
						}
						else
						{
							ClearCache(sub_cache2);
							BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, 0);
							ScreenPushAnimation(sub_cache2, DOWN);
						}
						break;
					}
					case DOUBLE_TAP:
					{
						func_num = func_index;
						ON_CLOSE= 1;
						break;
					}
					case AWT:
					{
						case_AWT:
						if(func_index <= (FUNC_NUM_MAX - func_index))
						{
							while(func_index > 0)
							{
								func_index --;
								BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, 0);
								ScreenPushAnimation(sub_cache2, DOWN);
							}
						}
						else
						{
							while(func_index < FUNC_NUM_MAX - 1)
							{
								func_index ++;
								BMPToCache(32, 0, 64, 64, FUNC_ICON[func_index], sub_cache2, 0);
								ScreenPushAnimation(sub_cache2, UP);
							}
						}
						func_num = WATCH;
						ON_CLOSE = 1;
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				ON_OPEN = 1;
			}
		}
		else if(func_num == SUB_MENU)		//�β˵�		ok
		{
			
			char x, y;
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				BMPToCache(0, 0, 128, 64, SUB_MENU_BMP, sub_cache2, COVER);
				ScreenPushAnimation(sub_cache2, LEFT);
				//LSM6DSMSetODR(ACC_ODR_208_HZ, GYR_ODR_208_HZ);
				LSM6DSMConfigAcc(ACC_ODR_208_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(ACC_ODR_208_HZ, GYR_SCALE_500_DPS);
			}
			if(tick_8ms)				//��̬����Ƶ�ʣ�125hz
			{
				tick_8ms = 0;
				LSM6DSMReadGYRAndACC(&LSM6DSM);
				IMUupdate(&LSM6DSM);
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				LSM6DSM.AngleX -= config.cal_anglex;	//��Ǽ�ȥƫ��
				LSM6DSM.AngleY -= config.cal_angley;	//��Ǽ�ȥƫ��
				LSM6DSM.AngleY -= 30;
				LSM6DSM.AngleX *= 3;			//����һ�º���������
				LSM6DSM.AngleY *= 2;			//����һ������������
				if(LSM6DSM.AngleX > 63)		//�Լ����������޷�
					LSM6DSM.AngleX = 63;
				else if(LSM6DSM.AngleX < -64)
					LSM6DSM.AngleX = -64;
				if(LSM6DSM.AngleY > 31)
					LSM6DSM.AngleY = 31;
				else if(LSM6DSM.AngleY < -31)
					LSM6DSM.AngleY = -31;
				x = 64 + LSM6DSM.AngleX;
				y = 32 + LSM6DSM.AngleY;
				if(x > 96)
					x = 96;
				if(y > 32)
					y = 32;
				ClearCache(main_cache);				//���������
				BMPToCache(0, 0, 128, 64, SUB_MENU_BMP, main_cache, COVER);	//���˵����������Դ���
				DrawSelectionFrame(x, y);			//����xy���껭ѡ������Դ���
				ScreenRefreshAll(main_cache);	//�����Դ�ˢ�µ�OLED��Ļ��
			}
			if(Cont == 0)
			{
				func_index = (enum function)((x + 16) / 32 + ((y + 16) / 32) * 4);
				func_num = func_index;
				ON_CLOSE = 1;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				//LSM6DSMSetODR(ACC_ODR_416_HZ, GYR_POWER_DOWN);
				LSM6DSMConfigAcc(ACC_ODR_416_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(GYR_POWER_DOWN, GYR_SCALE_500_DPS);
				ON_OPEN = 1;
			}
		}
		else if(func_num == STOPWATCH)	//��ʱ��		ok
		{	
			unsigned char t = 0, t_x = 0, t_y = 2;
			static int ms = 0, sec = 0, min = 0;
			static unsigned char n = 0, x = 0, y = 2;
			static unsigned char str1[10][11];
			unsigned char str2[10];
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				ms = (timer_cnt * 65536 + ((TH0 << 8) | TL0)) / 10.24;
				sec = ms / 100 % 60;
				min = sec / 60;
				sprintf(str2, "%02d:%02d:%02d", min, sec % 60, ms % 100);
				ShowString(32, 0, str2, sub_cache2, FONT8X16, NO_INVERSED, 0);
				if(n != 0)
				{
					for(t = 0; t < n; t++)
					{
						if(t == 6)
						{
							t_x = 64;
							t_y = 2;
						}
						ShowString(t_x, t_y++, str1[t], sub_cache2, FONT6X8, NO_INVERSED, 0);
					}
				}
				ScreenPushAnimation(sub_cache2, LEFT);
				ClearCache(sub_cache2);
			}
			if(tick_1ms)
			{
				tick_1ms = 0;
				ms = (timer_cnt * 65536 + ((TH0 << 8) | TL0)) / 10.24;
				sec = ms / 100;
				min = sec / 60;
				sprintf(str2, "%02d:%02d:%02d", min, sec % 60, ms % 100);
				ShowString(32, 0, str2, main_cache, FONT8X16, NO_INVERSED, 1);
				sleep_time = 0;
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						if(TR0)
						{
							TR0 = 0;
							PCF8563EnableClockOuput(CLKOUT_1024_HZ, DISABLE_CLKOUT);
							timer_on_flag = 0;
						}
						else
						{
							PCF8563EnableClockOuput(CLKOUT_1024_HZ, ENABLE_CLKOUT);
							TR0 = 1;
							timer_on_flag = 1;
						}
						break;
					}
					case KEY3:
					{
						timer_cnt = 0;
						TH0 = 0;
						TL0 = 0;
						ms = 0;
						sec = 0;
						min = 0;
						n = 0;
						x = 0;
						y = 2;
						ClearCache(main_cache);
						break;
					}
					case DOUBLE_TAP:
					{
						if(n < 9 && timer_on_flag == 1)
						{
							if(n == 6)
							{
								x = 64;
								y = 2;
							}
							sprintf(str1[n], "%d.%02d:%02d:%02d\0", (int)(n + 1), min, sec % 60, ms % 100);
							ShowString(x + 0, y++, str1[n++], main_cache, FONT6X8, NO_INVERSED, 1);
						}
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
		else if(func_num == PEDOMETER)	//�Ʋ���		ok
		{
			unsigned char n;
			unsigned int step;
			unsigned char buf[8];
			if(ON_OPEN)
			{
				ON_OPEN = 0;
//				PCF8563ReadTime(&RTC);
				ClearCache(sub_cache2);
				ShowString(0, 0, "Today:    History:", sub_cache2, FONT6X8, NO_INVERSED, 0);
				step = LSM6DSMGetCurrentStep();
				sprintf(buf, "%d ", step);
				ShowString(0, 1, buf, sub_cache2, FONT8X16, NO_INVERSED, 0);		
				ScreenPushAnimation(sub_cache2, LEFT);
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				step = LSM6DSMGetCurrentStep();
				sprintf(buf, "%d ", step);
				ShowString(0, 1, buf, main_cache, FONT8X16, NO_INVERSED, 0);
				for(n = 0; n < 7; n++)
				{
					ShowString(64, n + 1, config.history_step[n], main_cache, FONT6X8, NO_INVERSED, 1);
				}
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						//LSM6DSMResetStepCounter();
						break;
					}	
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
		else if(func_num == THPMETER)		//��ʪ�ȼ�	ok
		{		
			float altitude;
			unsigned char str1[10] = {0};
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				BMPToCache(0, 0, 48, 64, T_H_P_A, sub_cache2, 0);
				sprintf(str1, "%.1f", BME280.temperature);
				ShowString(48, 0, str1, sub_cache2, FONT8X16, NO_INVERSED, 0);
				BMPToCache(84, 0, 16, 16, CELSIUS_ICON, sub_cache2, 0);
				sprintf(str1, "%.1f%%", BME280.humidity);
				ShowString(48, 2, str1, sub_cache2, FONT8X16, NO_INVERSED, 0);
				sprintf(str1, "%ld Pa", (long)BME280.pressure);
				ShowString(48, 4, str1, sub_cache2, FONT8X16, NO_INVERSED, 0);
				ScreenPushAnimation(sub_cache2, LEFT);
				BME280ContinuousMeasurement(MS_125);		//��������ģʽ���������125ms
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				BME280GetSensorData(&BME280);					//��BME280�������ݲ���ʾ
				altitude = 44330.77 * (1 - pow((BME280.pressure / 101500), 0.190263));
				ClearCache(main_cache);
				BMPToCache(0, 0, 48, 64, T_H_P_A, main_cache, 0);
				sprintf(str1, "%.1f", BME280.temperature);
				ShowString(48, 0, str1, main_cache, FONT8X16, NO_INVERSED, 0);
				BMPToCache(84, 0, 16, 16, CELSIUS_ICON, main_cache, 0);
				sprintf(str1, "%.1f %%", BME280.humidity);
				ShowString(48, 2, str1, main_cache, FONT8X16, NO_INVERSED, 0);
				sprintf(str1, "%ld Pa", (long)BME280.pressure);
				ShowString(48, 4, str1, main_cache, FONT8X16, NO_INVERSED, 0);
				sprintf(str1, "%d m", (int)altitude);
				ShowString(48, 6, str1, main_cache, FONT8X16, NO_INVERSED, 1);
			}
			if(Trg == KEY1)
			{
				Trg = 0;
				ON_CLOSE = 1;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				BME280SetMode(SLEEP_MODE);
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
		else if(func_num == RADIO)			//������		ok
		{
			static unsigned char index = 0;
			static bit searching_flag = 0;
			static unsigned char search_direction;
			float temp;
			unsigned char str2[16];
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				ShowString(80, 0, "volume", sub_cache2, FONT8X16, NO_INVERSED, 0);
				sprintf(str2,"%.1fMHz",config.radio_channel);
				ShowString(40, 3, str2, sub_cache2, FONT8X16, NO_INVERSED, 0);
				ShowString(0, 6, "tune", sub_cache2, FONT8X16, NO_INVERSED, 0);
				ShowString(80, 6, "search", sub_cache2, FONT8X16, NO_INVERSED, 0);
				ShowString(0, 0, "close", main_cache, FONT8X16, NO_INVERSED, 0);
				ScreenPushAnimation(sub_cache2, LEFT);
				if(radio_on_flag != 1)
				{
					RDA5807MPowerUp();
					RDA5807MSetFq(config.radio_channel);
					if(config.radio_volume > 0x0f)
						config.radio_volume = 0;
					RDA5807MSetVOLUME(config.radio_volume);
					radio_on_flag = 1;
				}
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				ClearCache(main_cache);
				ShowString(80, 0, "volume", main_cache, FONT8X16, index == 0?INVERSED:NO_INVERSED, 0);
				sprintf(str2,"%.1fMHz", config.radio_channel);
				ShowString(40, 3, str2, main_cache, FONT8X16, NO_INVERSED, 0);
				if(searching_flag)
				{
					temp = RDA5807MSEEK(search_direction);
					if(temp == 0)
						ShowString(28, 5, "searching...", main_cache, FONT6X8, NO_INVERSED, 0);
					else
					{
						config.radio_channel = temp;
						searching_flag = 0;
					}
				}
				ShowString(80, 6, "search", main_cache, FONT8X16, index == 1?INVERSED:NO_INVERSED, 0);
				ShowString(0, 6, "tune", main_cache, FONT8X16, index == 2?INVERSED:NO_INVERSED, 0);
				ShowString(0, 0, "close", main_cache, FONT8X16, index == 3?INVERSED:NO_INVERSED, 1);
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						if(index == 0)
						{
							if(++config.radio_volume > 0x0f)
								config.radio_volume = 0x0f;
							RDA5807MSetVOLUME(config.radio_volume);
						}
						else if(index == 1)
						{
							search_direction = UPWARD;
							searching_flag = 1;
						}
						else if(index == 2)
						{
							config.radio_channel += 0.1;
							if(config.radio_channel > 108)
								config.radio_channel = 87;
							RDA5807MSetFq(config.radio_channel);
						}
						break;
					}	
					case KEY3:
					{
						if(index == 0)
						{
							if(--config.radio_volume == 255)
								config.radio_volume = 0;
							RDA5807MSetVOLUME(config.radio_volume);
						}
						else if(index == 1)
						{
							search_direction = DOWNWARD;
							searching_flag = 1;
						}
						else if(index == 2)
						{
							config.radio_channel -= 0.1;
							if(config.radio_channel < 87)
								config.radio_channel = 108;
							RDA5807MSetFq(config.radio_channel);
						}
						break;
					}
					case DOUBLE_TAP:
					{
						if(++index == 4)
							index = 0;
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				if(powerdown_flag == 0 && index == 3)
				{
					RDA5807MPowerDown();
					radio_on_flag = 0;
					index = 0;	
					EEPROMWriteConfiguration(&config);
				}
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
		else if(func_num == COMPASS)		//������		ok
		{
			//��ʾ�ǶȺ�ָ��
			static char mode = 0;
			int magnet_data[3] = {0};
			unsigned char str3[16];
			if(mode == 0)			
			{
				unsigned char n;
				int angle_from_north;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					sprintf(str3, "%d  ", angle_from_north);
					ShowString(24, 4, str3, sub_cache2, FONT8X16, NO_INVERSED, 0);
					BMPToCache(64, 0, 64, 64, COMPASS_ICON, sub_cache2, COVER);
					ScreenPushAnimation(sub_cache2, LEFT);
					//LSM6DSMSetODR(ACC_ODR_208_HZ, GYR_ODR_208_HZ);
					LSM6DSMConfigAcc(ACC_ODR_208_HZ, ACC_SCALE_4_G);
					LSM6DSMConfigGyr(ACC_ODR_208_HZ, GYR_SCALE_500_DPS);
					HMC5883L_Set_Mode(0);  									//������������ģʽ
					HMC5883L_Set_Calibration_Value(config.cal_magnet_x0, config.cal_magnet_y0, config.cal_magnet_z0, 
																				config.cal_magnet_ab, config.cal_magnet_ac);	//����У׼����
				}
				if(ON_RETURN)
				{
					ON_RETURN = 0;
					ClearCache(sub_cache2);
					BMPToCache(16, 2, 32, 16, NESW[n], sub_cache2, COVER);
					sprintf(str3, "%d  ", angle_from_north);
					ShowString(24, 4, str3, sub_cache2, FONT8X16, NO_INVERSED, COVER);
					BMPToCache(64, 0, 64, 64, COMPASS_ICON, sub_cache2, COVER);
					ScreenPushAnimation(sub_cache2, RIGHT);
				}
				if(tick_8ms)
				{
					tick_8ms = 0;	
					LSM6DSMReadGYRAndACC(&LSM6DSM);
					IMUupdate(&LSM6DSM);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					Read_HMC5883L(magnet_data);
					angle_from_north = (int)HMC5883L_Get_AngleXY(magnet_data, (int)LSM6DSM.AngleX, (int)LSM6DSM.AngleY);
					angle_from_north += 90;
					if(angle_from_north >= 360)
						angle_from_north -= 360;
					n = (angle_from_north + 22.5) / 45;
					if(n == 8)
						n = 0;
					BMPToCache(16, 2, 32, 16, NESW[n], main_cache, 0);
					sprintf(str3, "%d  ", angle_from_north);
					ShowString(24, 4, str3, main_cache, FONT8X16, NO_INVERSED, 0);
					BMPToCache(64, 0, 64, 64, COMPASS_ICON, main_cache, 0);
					angle_from_north -= 90;
					if(angle_from_north < 0)
						angle_from_north += 360;
					DrawArm(95, 31, 18, angle_from_north);
					ScreenRefreshAll(main_cache);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case KEY2:
						{
							mode = 1;
							ON_OPEN = 1;
							break;
						}	
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					HMC5883L_Set_Mode(2);  //����˯��ģʽ
					LSM6DSMConfigAcc(ACC_ODR_416_HZ, ACC_SCALE_4_G);
					LSM6DSMConfigGyr(GYR_POWER_DOWN, GYR_SCALE_500_DPS);
					func_num = MENU;
					ON_RETURN = 1;
				}
			}
			else if(mode == 1)	//����������У׼
			{
				static int data_cnt = 0;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					sprintf(str3, "x:%d", magnet_data[0]);
					ShowString(0, 0, str3, sub_cache2, FONT8X16, NO_INVERSED, 0);
					sprintf(str3, "y:%d", magnet_data[1]);
					ShowString(0, 2, str3, sub_cache2, FONT8X16, NO_INVERSED, 0);
					sprintf(str3, "z:%d", magnet_data[2]);
					ShowString(0, 4, str3, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
					ResetMatrix();
					HMC5883L_Set_Calibration_Value(0, 0, 0, 1, 1);	//���У׼����
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					Read_HMC5883L(magnet_data);
					CalcData_Input(magnet_data[0], magnet_data[1], magnet_data[2]);
					sprintf(str3, "x:%d", magnet_data[0]);
					ShowString(0, 0, str3, main_cache, FONT8X16, NO_INVERSED, 0);
					sprintf(str3, "y:%d", magnet_data[1]);
					ShowString(0, 2, str3, main_cache, FONT8X16, NO_INVERSED, 0);
					sprintf(str3, "z:%d", magnet_data[2]);
					ShowString(0, 4, str3, main_cache, FONT8X16, NO_INVERSED, 0);
					sprintf(str3, "data_cnt:%d", data_cnt);
					ShowString(0, 6, str3, main_cache, FONT8X16, NO_INVERSED, 1);
					if(++data_cnt == 5000)
					{
						ON_CLOSE = 1;
						Bee();
					}
				}
				if(Trg == KEY1)
				{
					Trg = 0;
					ON_CLOSE = 1;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					Ellipsoid_fitting_Process(&magnet_cal_data);		//����У׼�㷨
					HMC5883L_Set_Calibration_Value(magnet_cal_data.X0, magnet_cal_data.Y0, magnet_cal_data.Z0, 
																				magnet_cal_data.A / magnet_cal_data.B, 
																				magnet_cal_data.A / magnet_cal_data.C);//����У׼����
					config.cal_magnet_x0 = magnet_cal_data.X0;
					config.cal_magnet_y0 = magnet_cal_data.Y0;
					config.cal_magnet_z0 = magnet_cal_data.Z0;
					config.cal_magnet_ab = magnet_cal_data.A / magnet_cal_data.B;
					config.cal_magnet_ac = magnet_cal_data.A / magnet_cal_data.C;
					EEPROMWriteConfiguration(&config);
					data_cnt = 0;
					mode = 0;
					ON_RETURN = 1;
				}
			}
		}
		
		else if(func_num == BLUETOOTH)	//����		ok
		{
			unsigned char bluetooth_data[128] = "\0";
			static bit bluetooth_init = 0;
			static unsigned char y = 0;
			static unsigned char connected = 0;
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				ScreenPushAnimation(sub_cache2, LEFT);
				if(bluetooth_init == 0)
				{
					while(PW02Init() == 0);				//������ʼ��
					bluetooth_init = 1;
				}
				PW02SetMode(1);
				y = 0;
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				if(connected == 0)
				{
					if(PW02CheckConnection())
					{
						connected = 1;
						y = 6;
					}
				}
				if((connected == 1) && PW02GetRxData(bluetooth_data))
					y += ShowString(0, y, bluetooth_data, main_cache, FONT8X16, NO_INVERSED, 1);
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						ClearCache(main_cache);
						y = 0;
						y += ShowString(0, y, "Screen cleaned", main_cache, FONT8X16, NO_INVERSED, 1);
						break;
					}	
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				if(connected == 0)
					while(PW02ExitATMode() == 0);
				else
					connected = 0;
				PW02SetMode(0);
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
		else if(func_num == SPIRIT_LEVEL)	//ˮƽ��	ok
		{
			int x, y;
			unsigned char str[16];
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				sprintf(str, "x:%.1f  ", LSM6DSM.AngleX);
				ShowString(0, 0, str, sub_cache2, FONT6X8, NO_INVERSED, 0);
				sprintf(str, "y:%.1f  ", LSM6DSM.AngleY);
				ShowString(0, 1, str, sub_cache2, FONT6X8, NO_INVERSED, 0);
				sprintf(str, "z:%.1f  ", LSM6DSM.AngleZ);
				ShowString(0, 2, str, sub_cache2, FONT6X8, NO_INVERSED, 0);
				BMPToCache(56, 3, 16, 16, CIRCLE16X16, sub_cache2, 0);
				ScreenPushAnimation(sub_cache2, LEFT);
				//LSM6DSMSetODR(ACC_ODR_208_HZ, GYR_ODR_208_HZ);
				LSM6DSMConfigAcc(ACC_ODR_208_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(GYR_ODR_208_HZ, GYR_SCALE_500_DPS);
			}
			if(tick_8ms)
			{
				tick_8ms = 0;
				LSM6DSMReadGYRAndACC(&LSM6DSM);
				IMUupdate(&LSM6DSM);
			}
			if(tick_20ms)
			{
				tick_20ms = 0;
				ClearCache(main_cache);
				LSM6DSM.AngleX -= config.cal_anglex;
				LSM6DSM.AngleY -= config.cal_angley;
				sprintf(str, "x:%.1f  ", LSM6DSM.AngleX);
				ShowString(0, 0, str, main_cache, FONT6X8, NO_INVERSED, 0);
				sprintf(str, "y:%.1f  ", LSM6DSM.AngleY);
				ShowString(0, 1, str, main_cache, FONT6X8, NO_INVERSED, 0);
				sprintf(str, "z:%.1f  ", LSM6DSM.AngleZ);
				ShowString(0, 2, str, main_cache, FONT6X8, NO_INVERSED, 0);
				BMPToCache(56, 3, 16, 16, CIRCLE16X16, main_cache, 0);
				x = 64 + LSM6DSM.AngleX;
				y = 32 + LSM6DSM.AngleY;
				if(x > 126)
					x = 126;
				else if(x < 1)
					x = 1;
				if(y > 62)
					y = 62;
				else if(y < 1)
					y = 1;
				if(x ==63 && y == 32)
					ScreenSetInverse(config.screen_inverse == NO_INVERSED?INVERSED:NO_INVERSED);
				else
					ScreenSetInverse(config.screen_inverse);
				DrawDot(x, y);
				DrawDot(x - 1, y);
				DrawDot(x + 1, y);
				DrawDot(x, y + 1);
				DrawDot(x, y - 1);
				ScreenRefreshAll(main_cache);
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						config.cal_anglex = LSM6DSM.AngleX;
						config.cal_angley = LSM6DSM.AngleY;
						break;
					}	
					case KEY3:
					{
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				ScreenSetInverse(config.screen_inverse);
				//LSM6DSMSetODR(ACC_ODR_416_HZ, GYR_POWER_DOWN);
				LSM6DSMConfigAcc(ACC_ODR_416_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(GYR_POWER_DOWN, GYR_SCALE_500_DPS);
				EEPROMWriteConfiguration(&config);
				func_num = MENU;
				ON_RETURN = 1;
			}
		}
	
		else if(func_num == FLASH_LIGHT)	//�ֵ�Ͳ	ok
		{
			static char flash_mode = 0;
			static char flash_t_cnt = 0;
			static bit k = 0;
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				ShowString(28, 3, "Torch ON", sub_cache2, FONT8X16, NO_INVERSED, 0);
				ScreenPushAnimation(sub_cache2, LEFT);
				ScreenSetInverse(INVERSED);
				ScreenSetBrightness(255);
				LED(ON);
			}
			if(tick_20ms)
 			{
				tick_20ms = 0;
				inactive_time = 0;
				if(flash_mode)
				{
					if(flash_t_cnt++ == 5)
					{
						flash_t_cnt = 0;
						k = ~k;
						LED(k);
					}
				}
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						if(flash_mode == 0)
							flash_mode = 1;
						else
						{
							flash_mode = 0;
							LED(ON);
						}
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				ScreenSetBrightness(config.screen_brightness);
				ScreenSetInverse(config.screen_inverse);
				LED(OFF);
				flash_mode = 0;
				flash_t_cnt = 0;
				func_num = MENU;
				ON_RETURN = 1;	
			}
		}
		else if(func_num == SETTING)			//����		ok
		{
			static char index = 0;		//�˵�������(0~MENU_MAX_ROW-1)
			static char pointer = 0;	//��ǰ��ѡ������Ļ�ϵ�λ��(0~3)
			static char mode = -1;
			unsigned char str[21];
			unsigned char *cache;
			if(mode == -1)	//���ò˵�����
			{
				if(ON_OPEN | ON_RETURN)
				{
					cache = &sub_cache2;
				}
				else
					cache = &main_cache;
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(cache);
					ShowString(0, pointer * 2, ">", main_cache, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 0, SETTING_MENU[index - pointer], cache, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 2, SETTING_MENU[index - pointer + 1], cache, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 4, SETTING_MENU[index - pointer + 2], cache, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 6, SETTING_MENU[index - pointer + 3], cache, FONT8X16, NO_INVERSED, 1);
					if(ON_OPEN)
					{
						ScreenPushAnimation(sub_cache2, LEFT);
						ON_OPEN = 0;
					}
					else if(ON_RETURN)
					{
						ScreenPushAnimation(sub_cache2, RIGHT);
						ON_RETURN = 0;
					}	
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case KEY2:
						{
							index--;
							if(--pointer < 0)
							{
								pointer = 0;
								if(index != -1)
								{
									ClearCache(sub_cache2);
									ShowString(16, 6, SETTING_MENU[index], sub_cache2, FONT8X16, NO_INVERSED, 0);
									ScreeRollDown(sub_cache2, 16);
								}
								else
									index = 0;
							}
							break;
						}	
						case KEY3:
						{
							index++;
							if(++pointer == 4)
							{
								pointer = 3;
								if(index != MENU_MAX_ROW)
								{
									ClearCache(sub_cache2);
									ShowString(16, 0, SETTING_MENU[index], sub_cache2, FONT8X16, NO_INVERSED, 0);
									ScreeRollUp(sub_cache2, 16);
								}
								else
									index = MENU_MAX_ROW - 1;
							}
							break;
						}
						case DOUBLE_TAP:
						{
							mode = index;
							ON_OPEN = 1;
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					func_num = MENU;
					ON_RETURN = 1;
				}
			}
			else if(mode == 0)	//����ʱ��
			{
				static char pointer = 0;
				static char set_index = 0;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					PCF8563ReadTime(&RTC);
					ClearCache(sub_cache2);
					ShowString(0, 0, ">", sub_cache2, FONT8X16, NO_INVERSED, 0);
					sprintf(str, "%2d:%2d:%2d", (int)RTC.hour, (int)RTC.minute, (int)RTC.second);
					ShowString(16, 0, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					sprintf(str, "%4d/%2d/%2d", (int)RTC.year, (int)RTC.month, (int)RTC.day);
					ShowString(16, 2, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 4, WEEKDAY_IN_STR[RTC.weekday - 1], sub_cache2, FONT6X8, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					if(set_index == 0)
						PCF8563ReadTime(&RTC);
					ShowString(0, pointer * 2, ">", main_cache, FONT8X16, NO_INVERSED, 0);
					sprintf(str, "%02d:%02d:%02d", (int)RTC.hour, (int)RTC.minute, (int)RTC.second);
					ShowString(16, 0, str, main_cache, FONT8X16, NO_INVERSED, 0);
					if(pointer == 0 && set_index != 0)
					{
						if(set_index == 1)
						{
							sprintf(str, "%02d", (int)RTC.hour);
							ShowString(16, 0, str, main_cache, FONT8X16, INVERSED, 0);
						}
						else if(set_index == 2)
						{
							sprintf(str, "%02d", (int)RTC.minute);
							ShowString(40, 0, str, main_cache, FONT8X16, INVERSED, 0);
						}
						else if(set_index == 3)
						{
							sprintf(str, "%02d", (int)RTC.second);
							ShowString(64, 0, str, main_cache, FONT8X16, INVERSED, 0);
						}
					}
					sprintf(str, "%4d/%2d/%2d", (int)RTC.year, (int)RTC.month, (int)RTC.day);
					ShowString(16, 2, str, main_cache, FONT8X16, NO_INVERSED, 0);
					if(pointer == 1 && set_index != 0)
					{
						if(set_index == 1)
						{
							sprintf(str, "%4d", (int)RTC.year);
							ShowString(16, 2, str, main_cache, FONT8X16, INVERSED, 0);
						}
						else if(set_index == 2)
						{
							sprintf(str, "%2d", (int)RTC.month);
							ShowString(56, 2, str, main_cache, FONT8X16, INVERSED, 0);
						}
						else if(set_index == 3)
						{
							sprintf(str, "%2d", (int)RTC.day);
							ShowString(80, 2, str, main_cache, FONT8X16, INVERSED, 0);
						}
					}
					if(pointer == 2 && set_index != 0)
						ShowString(16, 4, WEEKDAY_IN_STR[RTC.weekday - 1], main_cache, FONT8X16, INVERSED, 1);
					else
						ShowString(16, 4, WEEKDAY_IN_STR[RTC.weekday - 1], main_cache, FONT8X16, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							if(set_index != 0)
							{
								set_index = 0;
								PCF8563WriteTime(RTC.hour, RTC.minute, RTC.second);
								PCF8563WriteDate(RTC.year, RTC.month, RTC.day, RTC.weekday);
							}
							else
								ON_CLOSE = 1;
							break;
						}
						case KEY2:
						{
							if(set_index == 0)
							{
								if(--pointer < 0)
									pointer = 2;
							}
							else
							{
								if(pointer == 0)
								{
									if(set_index == 1)
									{
										if(++RTC.hour == 24)
											RTC.hour = 0;
									}
									else if(set_index == 2)
									{
										if(++RTC.minute == 60)
											RTC.minute = 0;
									}
									else if(set_index == 3)
									{
										if(++RTC.second == 60)
											RTC.second = 0;
									}
								}
								else if(pointer == 1)
								{
									if(set_index == 1)
									{
										if(++RTC.year == 2100)
											RTC.year = 2000;
									}
									else if(set_index == 2)
									{
										if(++RTC.month == 13)
											RTC.month = 1;
									}
									else if(set_index == 3)
									{
										if(++RTC.day == 32)
											RTC.day = 1;
									}
								}
								else if(pointer == 2)
								{
									if(set_index == 1)
										if(++RTC.weekday == 8)
											RTC.weekday = 1;
								}
							}
							break;
						}
						case KEY3:
						{
							if(set_index == 0)
							{
								if(++pointer == 3)
									pointer = 0;
							}
							else
							{
								if(pointer == 0)
								{
									if(set_index == 1)
									{
										if(--RTC.hour == 255)
											RTC.hour = 23;
									}
									else if(set_index == 2)
									{
										if(--RTC.minute == 255)
											RTC.minute = 59;
									}
									else if(set_index == 3)
									{
										if(--RTC.second == 255)
											RTC.second = 59;
									}
								}
								else if(pointer == 1)
								{
									if(set_index == 1)
									{
										if(--RTC.year == 1999)
											RTC.year = 2099;
									}
									else if(set_index == 2)
									{
										if(--RTC.month == 255)
											RTC.month = 12;
									}
									else if(set_index == 3)
									{
										if(--RTC.day == 255)
											RTC.day = 31;
									}
								}
								else if(pointer == 2)
								{
									if(set_index == 1)
										if(--RTC.weekday == 0)
											RTC.weekday = 7;
								}
							}
							break;
						}
						case DOUBLE_TAP:
						{
							if(pointer < 2)
							{
								if(++set_index == 4)
									set_index = 1;
							}
							else if(pointer == 2)
							{
								if(++set_index == 2)
									set_index = 1;
							}
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					set_index = 0;
					pointer = 0;
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 1)	//��������
			{
				static unsigned char pointer = 0;
				static unsigned char set_index = 0;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					ShowString(16, 0, "Alarm:", sub_cache2, FONT6X8, NO_INVERSED, 0);
					ShowString(0, 1, ">", sub_cache2, FONT8X16, NO_INVERSED, 0);
					sprintf(str, "%02d:%02d", (int)config.alarm_hour, (int)config.alarm_min);
					ShowString(16, 1, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 3, "Mode:", sub_cache2, FONT6X8, NO_INVERSED, 0);
					ShowString(16, 4, ALARM_MODE_MENU[config.alarm_mode], sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(0, 1 + pointer * 3, ">", main_cache, FONT8X16, NO_INVERSED, 0);
					ShowString(16, 0, "Alarm:", main_cache, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%02d:%02d", (int)config.alarm_hour, (int)config.alarm_min);
					ShowString(16, 1, str, main_cache, FONT8X16, NO_INVERSED, 0);
					if(pointer == 0 && set_index != 0)
					{
						if(set_index == 1)
						{
							sprintf(str, "%02d", (int)config.alarm_hour);
							ShowString(16, 1, str, main_cache, FONT8X16, INVERSED, 0);
						}
						else if(set_index == 2)
						{
							sprintf(str, "%02d", (int)config.alarm_min);
							ShowString(40, 1, str, main_cache, FONT8X16, INVERSED, 0);
						}
					}
					ShowString(16, 3, "Mode:", main_cache, FONT6X8, NO_INVERSED, 0);
					if(pointer == 1 && set_index != 0)
					{
						
						if(config.alarm_mode == ALARM_SPECIFIC_DAY)
						{
							sprintf(str, "%02d", (int)config.alarm_day);
							ShowString(16, 6, str, main_cache, FONT8X16, set_index==1?NO_INVERSED:INVERSED, 0);
						}
						ShowString(16, 4, ALARM_MODE_MENU[config.alarm_mode], main_cache, FONT8X16, INVERSED, 1);	
					}
					else
					{
						ShowString(16, 4, ALARM_MODE_MENU[config.alarm_mode], main_cache, FONT8X16, NO_INVERSED, 1);						
					}
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							if(set_index != 0)
								set_index = 0;
							else
								ON_CLOSE = 1;
							break;
						}
						case KEY2:
						{
							if(set_index == 0)
							{
								if(++pointer == 2)
									pointer = 0;
							}
							else
							{
								if(pointer == 0)
								{
									if(set_index == 1)
									{
										if(++config.alarm_hour == 24)
											config.alarm_hour = 0;
									}
									else if(set_index == 2)
									{
										if(++config.alarm_min == 60)
										config.alarm_min = 0;
									}
								}
								else if(pointer == 1)
								{
									if(set_index == 1)
									{
										if(++config.alarm_mode == ALARM_MODE_NUM)
											config.alarm_mode = 0;
									}
									else if(set_index == 2)
									{
										if(++config.alarm_day == 32)
										{
											config.alarm_day = 1;
										}
									}
								}
							}
							break;
						}	
						case KEY3:
						{
							if(set_index == 0)
							{
								if(--pointer == 255)
									pointer = 1;
							}
							else 
							{
								if(pointer == 0)
								{
									if(set_index == 1)
									{
										if(--config.alarm_hour == 255)
											config.alarm_hour = 23;
									}
									else if(set_index == 2)
									{
										if(--config.alarm_min == 255)
											config.alarm_min = 59;
									}
								}
								else if(pointer == 1)
								{
									if(set_index == 1)
									{
										if(--config.alarm_mode == 255)
											config.alarm_mode = ALARM_MODE_NUM - 1;
									}
									else if(set_index == 2)
									{
										if(--config.alarm_day == 0)
										{
											config.alarm_day = 31;
										}
									}
								}
							}
							break;
						}
						case DOUBLE_TAP:
						{
							if(set_index == 0)
								set_index ++;
							else
							{
								if(pointer == 0)
								{
									if(++set_index == 3)
										set_index = 1;
								}
								else if(pointer == 1)
								{
									if(config.alarm_mode == ALARM_SPECIFIC_DAY)
									{
										if(++set_index == 3)
										set_index = 1;
									}
								}
							}
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					set_index = 0;
					pointer = 0;
					if(config.alarm_mode != ALARM_DISABLE)
					{
						if(config.alarm_mode == ALARM_WORKDAY)
						{
							PCF8563ReadTime(&RTC);
							if(RTC.weekday > 5)
								config.alarm_weekday = 1;
							else
							{
								if((config.alarm_hour > RTC.hour) && (config.alarm_min > RTC.minute))
									config.alarm_weekday = RTC.weekday;
								else
								{
									if(RTC.weekday != 5)
										config.alarm_weekday = RTC.weekday;
									else
										config.alarm_weekday = RTC.weekday + 1;
								}
							}
						}
						PCF8563SetAlarm(config.alarm_hour, config.alarm_min, config.alarm_weekday, config.alarm_day, config.alarm_mode);
					}
					else
						PCF8563DisableAlarm();
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 2)	//��������
			{
				static char temp;
				//unsigned char *cache;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					temp = config.screen_brightness / 50;
					ClearCache(sub_cache2);
					ShowString(0, 0, "Set the contrast ratio of the screen.", sub_cache2, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", (int)temp);
					ShowString(56, 3, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;	
					ClearCache(main_cache);
					ShowString(0, 0, "Set the contrast ratio of the screen.", main_cache, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", (int)temp);
					ShowString(56, 3, str, main_cache, FONT8X16, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case 0x01:
						{
							ON_CLOSE = 1;
							break;
						}
						case 0x02:
						{
							if(++temp == 6)
								temp = 5;
							config.screen_brightness = temp * 50 + 1;
							ScreenSetBrightness(config.screen_brightness);
							break;
						}	
						case 0x04:
						{
							if(--temp < 0)
								temp = 0;
							config.screen_brightness = temp * 50 + 1;
							ScreenSetBrightness(config.screen_brightness);
							break;
						}
						case 0x08:
							break;
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 3)	//�����Զ�Ϣ��ʱ��
			{
				static int temp;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					temp = config.t_inactive_max;
					ClearCache(sub_cache2);
					ShowString(0, 0, "The time which screen stay-ON for without any operation,when set to 0,the screen will never turn off.", sub_cache2, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", temp);
					ShowString(56, 5, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ShowString(72, 6, "s", sub_cache2, FONT6X8, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(0, 0, "The time which screen stay-ON for	without any operation,when set to 0,the screen will never turn off.", main_cache, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", temp);
					ShowString(56, 5, str, main_cache, FONT8X16, NO_INVERSED, 0);
					ShowString(72, 6, "s", main_cache, FONT6X8, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case KEY2:
						{
							if(++temp == 100)
								temp = 99;
							break;
						}	
						case KEY3:
						{
							if(--temp < 0)
								temp = 0;
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					config.t_inactive_max = temp;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 4)	//�����Զ��ػ�ʱ��
			{
				static int temp;
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					temp = config.t_sleep_max;
					ClearCache(sub_cache2);
					ShowString(0, 0, "The time which mcu stay active for without any operation before power-down,when set to 0,the mcu will never power down.", sub_cache2, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", temp);
					ShowString(56, 6, str, sub_cache2, FONT8X16, NO_INVERSED, 0);
					ShowString(72, 7, "s", sub_cache2, FONT6X8, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(0, 0, "The time which mcu stay active for before without any operation power-down,when set to 0,the mcu will never power down.", main_cache, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%d", temp);
					ShowString(56, 6, str, main_cache, FONT8X16, NO_INVERSED, 0);
					ShowString(72, 7, "s", main_cache, FONT6X8, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case 0x01:
						{
							ON_CLOSE = 1;
							break;
						}
						case 0x02:
						{
							if(++temp == 100)
								temp = 99;
							break;
						}	
						case 0x04:
						{
							if(--temp < 0)
								temp = 0;
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					config.t_sleep_max = temp;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 5)	//������Ļ��ɫ
			{
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					ShowString(56, 3, config.screen_inverse == INVERSED?"ON":"OFF", sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(56, 3, config.screen_inverse == INVERSED?"ON":"OFF", main_cache, FONT8X16, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case DOUBLE_TAP:
						{
							if(config.screen_inverse == INVERSED)
								config.screen_inverse = NO_INVERSED;
							else
								config.screen_inverse = INVERSED;
							ScreenSetInverse(config.screen_inverse);
							break;
						}	
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 6)	//������Ļ����
			{
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					ShowString(40, 3, config.screen_direction == NORMAL?"NORMAL":"UP-SIDE-DOWN", sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(40, 3, config.screen_direction == NORMAL?"NORMAL":"UP-SIDE-DOWN", main_cache, FONT8X16, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case DOUBLE_TAP:
						{
							if(config.screen_direction == UPSIDEDOWN)
								config.screen_direction = NORMAL;
							else
								config.screen_direction = UPSIDEDOWN;
							ScreenSetDirection(config.screen_direction);
							break;
						}	
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 7)	//���ð�����
			{
				if(ON_OPEN)
				{
					ON_OPEN = 0;
					ClearCache(sub_cache2);
					ShowString(24, 3, config.key_sound == ON?"Buzzer ON":"Buzzer OFF", sub_cache2, FONT8X16, NO_INVERSED, 0);
					ScreenPushAnimation(sub_cache2, LEFT);
				}
				if(tick_20ms)
				{
					tick_20ms = 0;
					ClearCache(main_cache);
					ShowString(24, 3, config.key_sound == ON?"Buzzer ON":"Buzzer OFF", main_cache, FONT8X16, NO_INVERSED, 1);
				}
				if(Trg != 0)
				{
					switch(Trg)
					{
						case KEY1:
						{
							ON_CLOSE = 1;
							break;
						}
						case DOUBLE_TAP:
						{
							if(config.key_sound == ON)
								config.key_sound = OFF;
							else
								config.key_sound = ON;
							EnableBuzzer(config.key_sound);
							break;
						}
					}
					Trg = 0;
				}
				if(ON_CLOSE)
				{
					ON_CLOSE = 0;
					EEPROMWriteConfiguration(&config);
					mode = -1;
					ON_RETURN = 1;
				}
			}
			else if(mode == 8)	//��Ƭ����λ
			{
				MCUSoftReset();
			}
			else if(mode == 9)	//ϵͳ����
			{
				ON_OPEN = 0;
				active_flag = 0;
				action = 0;
				sleep_flag = 0;
				powerdown_flag = 0;
				inactive_time = 0;
				sleep_time = 0;
				deep_powerdown_flag = 1;
				mode = -1;
				func_num = WATCH;
			}
		}
		else if(func_num == SNAKES)				//̰����  ok
		{
			#define PANEL_WIDTH		92
			#define PANEL_HEIGHT	64
			#define	MAX_SNAKE_SPEED	50
			unsigned char i = 0;
			unsigned char str[6];
			static unsigned char snake_direction = 0;//means sneak direction
			static unsigned char snake_speed = 10;
			static unsigned char past_x[256];
			static unsigned char past_y[256];
			static unsigned char snake_head_x = 64, snake_head_y = 32, snake_len = 20;
			static unsigned char test_now = 0, delicious_x= 80,delicious_y = 40;
			static unsigned char best_score = 0;
			static bit game_loop = 0;
			static bit game_over = 0;
			static bit game_pause = 0;
			if(ON_OPEN)
			{
				ON_OPEN = 0;
				ClearCache(sub_cache2);
				ScreenPushAnimation(sub_cache2, LEFT);//�趨�ý������ƽ�����Ļ
				snake_direction = 0;
				snake_head_x = 64;
				snake_head_y = 32;
				snake_len = 20;
				game_over = 0;
				game_pause = 0;
				for(i = 0; i < snake_len; i++)
				{
					past_x[i] = 64;
					past_y[i] = 32;
				}
				LSM6DSMConfigAcc(ACC_ODR_208_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(GYR_ODR_208_HZ, GYR_SCALE_500_DPS);
			}
			if(tick_8ms)
			{
				tick_8ms = 0;
				LSM6DSMReadGYRAndACC(&LSM6DSM);
				IMUupdate(&LSM6DSM);
			}
			if(tick_1ms)
			{
				static unsigned char t_cnt = 0;
				tick_1ms = 0;
				if(++t_cnt == (MAX_SNAKE_SPEED - snake_speed))
				{
					t_cnt = 0;
					game_loop = 1;
				}
			}
			if(game_loop)
 			{
				game_loop = 0;
				inactive_time = 0;//to not close screen
				if(game_pause != 1)
				{
					ClearCache(main_cache);
					for(i = 0; i < PANEL_WIDTH; i++)	//��һ����
					{
						DrawDot(i, 0);
						DrawDot(i, PANEL_HEIGHT - 1);
					}
					for(i = 0; i < PANEL_HEIGHT; i++)
					{
						DrawDot(0, i);
						DrawDot(PANEL_WIDTH - 1, i);
					}
					ShowString(PANEL_WIDTH, 1, "Score:", main_cache, FONT6X8, NO_INVERSED, 0);
					sprintf(str, "%3d", (int)snake_len);
					ShowString(PANEL_WIDTH, 2, str, main_cache, FONT6X8, NO_INVERSED, 0);
					ShowString(PANEL_WIDTH, 5, "Best:", main_cache, FONT6X8, NO_INVERSED, 0);
					if(snake_len > best_score)
						best_score = snake_len;
					sprintf(str, "%3d", (int)best_score);
					ShowString(PANEL_WIDTH, 6, str, main_cache, FONT6X8, NO_INVERSED, 0);
					if(snake_direction == 0)
					{
						if(LSM6DSM.AngleY > 10)
							snake_direction = 1;
						else if(LSM6DSM.AngleY < -10)
							snake_direction = 3;
					}
					else if(snake_direction == 1)
					{
						if(LSM6DSM.AngleX > 10)
							snake_direction = 0;
						else if(LSM6DSM.AngleX < -10)
							snake_direction = 2;
					}
					else if(snake_direction == 2)
					{
						if(LSM6DSM.AngleY > 10)
							snake_direction = 1;
						else if(LSM6DSM.AngleY < -10)
							snake_direction = 3;
					}
					else if(snake_direction == 3)
					{
						if(LSM6DSM.AngleX > 10)
							snake_direction = 0;
						else if(LSM6DSM.AngleX < -10)
							snake_direction = 2;
					}
					//����ǰ������������ת��Ϊ�����Լ���λ�õĳ���
					//sneak_direction�Ƿ���test_x��ͷ����x��ֵ��test_y��ͷ����y��ֵ
					switch(snake_direction)
					{
						case 0:		//��
							snake_head_x++;
							break;
						case 1:		//��
							snake_head_y++;
							break;
						case 2:		//��
							snake_head_x--;
							break;
						case 3:		//��
							snake_head_y--;
							break;
					}
					if(snake_head_x >= PANEL_WIDTH)
						snake_head_x = 1;
					else if(snake_head_x <= 0)
						snake_head_x = PANEL_WIDTH - 1;
					if(snake_head_y >= PANEL_HEIGHT)
						snake_head_y = 1;
					else if(snake_head_y <= 0)
						snake_head_y = PANEL_HEIGHT - 1;
					//����Ƿ������Լ�������
					for(i = 0; i < snake_len; i++)
					{
						if((snake_head_x == past_x[i]) && (snake_head_y == past_y[i]))
						{
							game_over = 1;
							game_pause = 1;
							break;
						}
					}
					//����ǿ�����̰�����пɱ䳤��β�͵Ĳ��Գ���test_num����β�͵����������50��
					//����ԭ���ǰ�ÿ�ι�ȥ�����궼��¼���������档��ʾ��ʱ��Ͱѹ�ȥ�ĵ�Ҳ����ʾ����
					test_now++;
					test_now = test_now % snake_len;
					past_x[test_now] = snake_head_x;
					past_y[test_now] = snake_head_y;
					for(i = 0; i < snake_len; i++)
					{
						DrawDot(past_x[i], past_y[i]);
					}
					//��γ���ű��Ե���С���������delicious_x,delicious_yΪʳ�������
					//�Ե�ʳ���Ժ���һ���µ�ʳ�����test_numҲ����β�͵��������1
					if((snake_head_x <= delicious_x+1) && (snake_head_y <= delicious_y+1) && (snake_head_x >= delicious_x-1) && (snake_head_y>=delicious_y-1))
					{
						snake_len++;
						past_x[snake_len - 1] = past_x[snake_len - 2];//���С�׵������
						past_y[snake_len - 1] = past_y[snake_len - 2];
						Bee();
						delicious_x = rand() % 92;
						if(delicious_x > PANEL_WIDTH - 2)
							delicious_x = PANEL_WIDTH - 2;
						else if(delicious_x < 2)
							delicious_x  = 2;
						delicious_y = rand() % 64;
						if(delicious_y > PANEL_HEIGHT - 2)
							delicious_y = PANEL_HEIGHT - 2;
						else if(delicious_y < 2)
							delicious_y  = 2;
					}
					DrawDot(delicious_x,delicious_y);
					DrawDot(delicious_x+1,delicious_y);
					DrawDot(delicious_x-1,delicious_y);
					DrawDot(delicious_x,delicious_y+1);
					DrawDot(delicious_x+1,delicious_y+1);
					DrawDot(delicious_x-1,delicious_y+1);
					DrawDot(delicious_x,delicious_y-1);
					DrawDot(delicious_x+1,delicious_y-1);
					DrawDot(delicious_x-1,delicious_y-1);
					if(snake_len < 30)
						snake_speed = 10;
					else if(snake_len < 40)
						snake_speed = 20;
					else if(snake_len < 60)
						snake_speed = 30;
					else if(snake_len < 90)
						snake_speed = 35;
					else if(snake_len < 130)
						snake_speed = 40;
					else
						snake_speed = 45;
				}
				if(game_over)
				{
					ShowString(30, 2, "GAME", main_cache, FONT8X16, NO_INVERSED, 0);
					ShowString(30, 4, "OVER", main_cache, FONT8X16, NO_INVERSED, 0);
				}
				ScreenRefreshAll(main_cache);
			}
			if(Trg != 0)
			{
				switch(Trg)
				{
					case KEY1:
					{
						ON_CLOSE = 1;
						break;
					}
					case KEY2:
					{
						if(game_over != 1)
						{
							if(game_pause)
								game_pause = 0;
							else
								game_pause = 1;
						}
						break;
					}
					case KEY3:
					{
						ON_OPEN = 1;
						break;
					}
				}
				Trg = 0;
			}
			if(ON_CLOSE)
			{
				ON_CLOSE = 0;
				LSM6DSMConfigAcc(ACC_ODR_416_HZ, ACC_SCALE_4_G);
				LSM6DSMConfigGyr(GYR_POWER_DOWN, GYR_SCALE_500_DPS);
				func_num = MENU;
				ON_RETURN = 1;	
			}
		}
		if(action)					//ϵͳ�ڲ�ͬ״̬�¶Զ����Ĵ���
		{
			action = 0;
			if(active_flag)
				inactive_time = 0;
			else if(sleep_flag)
			{
				sleep_flag = 0;
				sleep_time = 0;
				active_flag = 1;
				ScreenOnOff(ON);
				screen_on_flag = 1;
			}
			else if(powerdown_flag)
			{
				powerdown_flag = 0;
				active_flag = 1;
				SystemPowerOn();
				ScreenOnOff(ON);
				screen_on_flag = 1;
				ON_OPEN = 1;
			}
			else if(deep_powerdown_flag)
			{
				deep_powerdown_flag = 0;
				active_flag = 1;
				SystemPowerOn();
				DisplayInit(&config);
				screen_on_flag = 1;
				SensorInit();
				PW02SetMode(0);
				PCF8563EnableTimer(TIMERCLK_1_60_HZ, 1);	//�Զ�����Ƶ��1minһ��
				ON_OPEN = 1;
			}
			autowake_cnt = 0;
		}
		if(sleep_flag)
		{
			if(screen_on_flag)
			{
				ScreenOnOff(OFF);
				screen_on_flag = 0;
			}
		}
		if(powerdown_flag)		
		{
			if(ON_CLOSE == 0)
			{
				SystemPowerDown();
			}
		}
		if(deep_powerdown_flag)
		{
			if(ON_CLOSE == 0)
			{
				SystemDeepPowerDown();
			}
			Enable3V3Output(1);
			Delay1ms(10);
		}
		if(alarm_flag)
		{
			sleep_time = 0;			//���屻�ص�ǰ�������
		}
		if(timer_on_flag)
		{
			sleep_time = 0;			//��ʱ���ڹ���ʱ�������
		}
		if(PCF8563_int_flag)	//PCF8563�������ж��ź�
		{
			unsigned char pcf8563_int_src;
			PCF8563_int_flag = 0;
			pcf8563_int_src = PCF8563ReadIntSrc();	//��PCF8563״̬�Ĵ���
			PCF8563ReadTime(&RTC);
			if(pcf8563_int_src & ALARM_INT)				//����������ж�
			{
				PCF8563ClearAlarmFlag();	//��������жϱ�־λ
				action = 1;				//��������
				alarm_flag = 1;		//��λ���ӱ�־λ
				Beebeebee();			//����
				Delay1ms(5);
				if(config.alarm_mode == ALARM_WORKDAY)
				{
					if(RTC.weekday + 1 > 5)
						config.alarm_weekday = 1;
					else
						config.alarm_weekday = RTC.weekday + 1;
					PCF8563SetAlarm(config.alarm_hour, config.alarm_min, config.alarm_weekday, config.alarm_day, config.alarm_mode);
				}
				else if(config.alarm_mode != ALARM_EVERYDAY)	//�������ģʽ����ÿ�춼��
				{
					config.alarm_mode = ALARM_DISABLE;	//�ر�����
					PCF8563DisableAlarm();							//�ر�����
				}
				EEPROMWriteConfiguration(&config);
			}
			if(pcf8563_int_src & TIMER_INT)				//����Ƕ�ʱ���ж�
			{
				PCF8563ClearTimerFlag();	//�����ʱ���жϱ�־λ
				//PCF8563�Թ̶���ʱ��������MCU
				//����֮��ִ����������
				battery_life = GetBatteryLife();
				if(active_flag || sleep_flag || powerdown_flag)
				{
					if((RTC.hour == 22) && (RTC.minute == 4))		//22:04,��¼����Ĳ�������
					{
						char i, j;
						for(i = 6; i > 0; i--)
						{
							for(j = 0; j < 10; j++)
							{
								config.history_step[i][j] = config.history_step[i - 1][j];
							}
						}
						sprintf(config.history_step[0], "%d/%d %d", (int)RTC.month, (int)RTC.day, (int)LSM6DSMGetCurrentStep());
						EEPROMWriteConfiguration(&config);
						LSM6DSMResetStepCounter();
					}
				}
				if(powerdown_flag)
				{
					if(++autowake_cnt >= 1440)		//1440���� = 1��
					{
						autowake_cnt = 0;
						powerdown_flag = 0;
						PCF8563EnableTimer(TIMERCLK_1_60_HZ, 10);	//�Զ�����Ƶ�ʸ�Ϊ10minһ�Σ�����1minһ��
						deep_powerdown_flag = 1;
					}
				}
			}    
		}
			
    }	
  ********************/           
	}

}
	