#ifndef _SYS_H
#define _SYS_H


#include "stc8a.h"
#include "Display.h"
#include "PCF8563.h"

/*********************�궨��***********************/
#define LED1 P35
#define K1   P27   // m
#define K2   P23   // up  
#define K3   P55   // down
#define KEY1         0x01
#define KEY2         0x02
#define KEY3   	  	 0x04
#define KEY12     	 0x03
#define KEY13     	 0x05
#define KEY23    	   0x06
#define DOUBLE_TAP   0x08
#define AWT	         0x09
#define ON   1
#define OFF  0
 
#define EE_ADDRESS1  0x0180         //��ַΪʲô�����
#define	ALARM_DISABLE				0x00

//ö�ٹ��ܺ�
enum function{
	WATCH = -3,
	MENU,
	SUB_MENU,
	STOPWATCH,
	PEDOMETER,
	THPMETER,
	RADIO,
	COMPASS,
	BLUETOOTH,
	SPIRIT_LEVEL,
	FLASH_LIGHT,
	SNAKES,
	SETTING
};
#define FIRST_FUNC   STOPWATCH   //��һ������
#define LAST_FUNC    SETTING     //�ڶ�������
#define FUNC_NUM_MAX 10          //��������
//������Ϣ�Ľṹ��
struct sys_config{
	unsigned char t_inactive_max;     //�Զ�Ϣ��ʱ��
	unsigned char t_sleep_max;        //�Զ�����ʱ��
	unsigned char screen_brightness;  //��Ļ����
	unsigned char screen_inverse;     //��Ļ�Ƿ�ɫ
	unsigned char screen_direction;   //��Ļ����ʾ����
	unsigned char key_sound;    			//�����Ƿ�������
	unsigned char alarm_hour;
	unsigned char alarm_min;
	unsigned char alarm_day;
	unsigned char alarm_weekday;
	unsigned char alarm_mode;         //��������ģʽ
	unsigned char radio_volume; 			//������������
	float radio_channel;              //������Ƶ��
	float cal_anglex;                 //x�����ƫ��
	float cal_angley;						      //y�����ƫ��
	int cal_magnet_x0;          	  //�����ƽ���ϵ��
	int cal_magnet_y0;          	    //�����ƽ���ϵ��
	int cal_magnet_z0;         	    //�����ƽ���ϵ��
	float cal_magnet_ab;             //�����ƽ���ϵ��
	float cal_magnet_ac;             //�����ƽ���ϵ��
	unsigned char history_step[7][11];//�Ʋ�������ʷ���ݣ���Ź�ȥ7��Ĳ���
	unsigned int check_sum;           //����������Ϣ����ͽ����������֤�����Ƿ�����
};
#define CONFIG_SIZE sizeof(struct sys_config)

void PinInit(void);
void MCUSoftReset(void);
void FeedWatchDog(void);
void UART1SendString(char *str);
void StartFrameReceive();
unsigned char CheckFrameReceived();
void LED(unsigned char k);
unsigned char EEPROMReadConfiguration(struct sys_config *config);
void EEPROMWriteConfiguration(struct sys_config *config);
void SysInit(void);

#endif



















