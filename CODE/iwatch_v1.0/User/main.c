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

/***********************状态机相关变量*********************/
enum function func_num = WATCH;    //功能号
char func_index;               //功能索引
bit ON_OPEN = 0;
bit ON_CLOSE = 0;
bit ON_RETURN = 0;
/********************************************************/
/************************时基信号************************/
bit tick_1ms = 0;             //1000Hz
bit tick_8ms = 0;             //125hz
bit tick_20ms = 0;     		    //50hz

/*************************工作状态相关变量*******************************/
bit action = 0;             //动态标志位
bit active_flag =1;   		//活动标志位
bit sleep_flag = 0; 				//睡眠标志位
bit powerdown_flag = 0;     //掉电标志位
bit deep_powerdown_flag = 0;		//深度掉电标志位
unsigned int inactive_time =0;  //自动息屏的时间
unsigned int sleep_time = 0;		//自动待机时间
unsigned int autowake_cnt = 0;  //自动唤醒的次数
bit screen_on_flag = 1;			//屏幕状态标志位



void main()
{	
	SysInit();

}

