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



void main()
{	
	SysInit();

}

