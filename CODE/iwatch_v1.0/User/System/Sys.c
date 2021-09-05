#include "sys.h"
#include "IIC.h"
#include "Delay.h"
#include "PowerManage.h"
#include "Buzzer.h"
#include "EEPROM.h"



	
//若EEPROM中没有数据或者数据不完整，则加载以下默认设置
const struct sys_config default_config = {
	//默认显示参数
	10,						//自动息屏时间
	5,						//自动待机时间
	10,						//屏幕亮度（0~255）
	NO_INVERSED,	//屏幕是否反色
	NORMAL,				//屏幕的显示方向
	OFF,					//关闭按键音
	//默认闹钟参数
	8,							//小时
	30,							//分钟
	1,							//1号
	1,							//周一
	ALARM_DISABLE,	//闹钟关闭
	//默认收音机参数
	1,			//收音机音量
	97.1,		//收音机频道
	//默认校正参数
	0,			//x轴倾角偏移
	0,			//y轴倾角偏移
	0,			//磁力计校正系数
	0,			//磁力计校正系数
	0,			//磁力计校正系数
	1,			//磁力计校正系数
	1,			//磁力计校正系数
	{{0}, {0}, {0}, {0}, {0}, {0}, {0}},	//计步器的历史数据，存放过去七天的步数
	0				//检验用求和项，不要修改
};

extern struct sys_config config;	


void PinInit()
{
	//P0口作为OLED屏幕的并行接口  8080方式有读使能(RE)和写使能(WE)，6800用总使能E,读写选择W/R
	//每个IO口都必须使用两个寄存器进行配置，配置P0口需要用到P0MO,P0M1,P0口所有的IO配置为准双向口，20mA驱动能力
	P0 = 0x00;
	P0M0 = 0x00;
	P0M1 = 0x00;
	//P10,P11 是串口2
	//P12,P13为显示屏D/C,RST引脚，(V1.0:P12=RST,P13=D/C)
	//P14,P15为 IIC引脚，外部接入了上拉电阻，故设置为开漏          
	//P16为蓝牙使能引脚，高电平时进入低功耗
	//P17为充电使能引脚，高阻时使能充电（V1.0 :0R tie on GND，去掉了）
	P12 = 1; //显示屏的RST 拉高   
	P16 = 1;   
	P1M0 = 0x30;  //00110000b
	P1M1 = 0xb0;  //10110000b
	//P20为LED驱动引脚，强推挽模式；（V1.0该引脚么有用，LED驱动引脚 为P35）
	//P21,P22为外部拓展引脚； 
	//P23为按键UP,默认高电平;
	//P24,P25为I2C引脚，外部无上拉电阻，故这是为通用双向；
	//P26为蜂鸣器驱动引脚，强推挽模式,默认低电平
	//P27为按键,默认高电平
	P2 = 0xb8;  //10111000b
	P2M0 = 0x40;//01000000b
	P2M1 = 0x00;//00000000b   
	//P37为按键按键，外部中断3，默认高电平；
	//P36为外部中断2
	//P34为T0脉冲输入引脚    设置为1有疑问，因为T_CLKOUT 引脚为开漏输出，无法对外输出高电平
	//P33为外部中断1         
	//P32为外部中断0         设置为1有疑问,设置1是因为T_INT是开漏输出，且输出低电平有效
	//P30,P31为串口1引脚
	P3 = 0x34;//0011 0100b
	P3M0 = 0x00;
	P3M1 = 0x00;    
	//P44,P43为显示屏E/RD,R/W引脚
	//P40为3.3V使能输出引脚(V1.0:没有使用)
	P4 = 0x19; //00011000b
	P4M0 = 0x00;
	P4M1 = 0x00;
	//P55为按键，默认高电平   DOWN
	P5 = 0x20;    //0010 0000b
	P5M0 = 0x00;
	P5M1 = 0x00;
}

void Timer0Init(void)
{
	TMOD |= 0x04;   //外部计数模式
	TL0 = 0x00;
	TH0 = 0x00;
	TR0 = 0;       //关闭定时器
	ET0 = 1;       //使能定时器中断
}
void Timer3Init(void)    //定时1ms   24MHZ
{
	T4T3M |= 0x02;     //T4R T4_C/T T4x12 T4CLKO 定时器时钟1T模式，CPU时钟不分频fosc/1
	T3L = 0x40;        //设置定时器初值
	T3H = 0xA2;        //设置定时器初值
	T4T3M |= 0x08;     //定时器3开始计时
	IE2 |= ET3;        //定时器3开溢出中断
}

#ifdef badapple
extern unsigned char xdata sub_cache2[];
unsigned int rx_buf_num = 0;
bit frame_received_flag = 0;
bit serial_busy = 0;
void UartInit(void)     //576000bps  24MHz
{
	SCON = 0x50;   //   SM0 SM1 SM2 REN TB RB8 TI RI 8位数据，可变波特率
	AUXR |= 0x40;  //   AUXR辅助寄存器，T1x12为1，则CPU时钟不分频，若为0，则为12分频
	AUXR &= 0xFE;  //   除bit0清零外，其他位保持原来设置不变
	TMOD &= 0x0F;  //   高4位 GATE C/T M1 M0    高四位清零，低四位保持不变(T1设置清零-打开gate,T1设置为定时器，对系统内部时钟进行计数，且设置为16位自动重装模式，T0保持不变)
	TL1 = 0xF6;    //   0xcc  246
	TH1 = 0xFF;    //   0xff  255    每次重装的数字是多少
	ET1 = 0;       //   定时器/计数器T1的溢出中断允许位，禁止T1中断
	TR1 = 1;       //   打开定时器1
	ES = 1;        //   开串口中断
	TI = 1;        //   模式0中，当串口发送数据第8位结束时，由硬件自动将TI置1,向主机请求中断，响应中断后TI必须用软件清零，其他模式，则在停止位开始发送时由硬件自动置1，向CPU发请求中断，响应后由软件清零，模式0是同步移位串行方式
}      
void UART1_Isr()  interrupt 4  using 1
{
	if(RI)
	{
		RI = 0;      //清中断标志位
		if(SBUF == MCU_RESET_CMD)
			MCUSoftReset();  //软复位MCU
	}
	if(RI)
	{
		RI = 0;
		sub_cache[rx_buf_num++] = SBUF;
		if(rx_buf_num == 1024)
		{
			rx_buf_num = 0;
			frame_received_flag =1;
		}
	}
	if(TI)
	{
		TI = 0;
		serial_busy = 0;
	}
}
void UART1SendString(char *str)
{
	while(*str)
	{
		while(serial_busy);
		serial_busy = 1;
		SBUF = *str++;
	}
}
void StartFrameReceive()
{
	UART1SendString("ok");
	frame_received_flag = 0;
	rx_buf_num = 0;
}
unsigned char CheckFrameRecevied()
{
	if(frame_received_flag)
	{
		frame_received_flag = 0;
		return 1;
	}
	else 
		return 0;
}
#else

void UartInit(void)       //115200bps@24.000MHz      串口1用定时器1
{
	SCON = 0x50;  // SM0 SM1 SM2 REN TB RB8 TI RI 8位数据，可变波特率
	AUXR |= 0x40;  //   AUXR辅助寄存器，T1x12为1，则CPU时钟不分频，若为0，则为12分频
	AUXR &= 0xFE;  //  除bit0清零外，其他位保持原来设置不变
	TMOD &= 0x0F;  //   高4位 GATE C/T M1 M0    高四位清零，低四位保持不变(T1设置清零-打开gate,T1设置为定时器，对系统内部时钟进行计数，且设置为16位自动重装模式，T0保持不变)
	TL1 = 0xCC;    //   0xcc  204
	TH1 = 0xFF;    //   0xff  255    每次重装的数字是多少
	ET1 = 0;       //   定时器/计数器T1的溢出中断允许位，禁止T1中断
	TR1 = 1;       //   打开定时器1
	TI = 1;        //    模式0中，当串口发送数据第8位结束时，由硬件自动将TI置1,向主机请求中断，响应中断后TI必须用软件清零，其他模式，则在停止位开始发送时由硬件自动置1，向CPU发请求中断，响应后由软件清零，模式0是同步移位串行方式
}

#endif

void Uart2Init(void)    //9600bps  24MHZ        串口2用定时器    
{
	S2CON = 0x50;  //8位数据，可变波特率
	AUXR |= 0x04;  //  定时器2始终未Fosc,即1T，选择定时器1作为波特率发生器
	T2L = 0x8F;   //设定定时器初始值
	T2H = 0xFD;   //设定定时器初值
	AUXR |= 0x10;  //启动定时器2
	IE |= ES2;     //使能串口中断
}

void InitConfig(void)    //中断初始化
{
	IT0 = 1;    //使能INT0下降沿中断，对应PCF8653的中断输出引脚
	EX0 = 1;    //使能INT0中断
	IT1 = 1;    //使能INT1下降沿中断，对应LSM6DSM的中断输出INT1
	EX1 = 1;    //使能INT1中断
	INTCLKO |= EX2; //使能INT2中断    对应LSM6DDSM的中断输出引脚2
  //INTCLKO |= EX3;  //使能INT3中断    对应按键2中断；  此处与V1.2不同
  //INTCLKO |= EX4;  //使能INT4中断	
}
void MCUSoftReset(void)   //单片机复位
{
	IAP_CONTR = 0x60;				//单片机复位指令
}

void EnableWatchDog(void)
{ 
	WDT_CONTR = 0x27;            //如果超过4.194s没有喂狗，MCU自动复位
}
void FeedWatchDog(void)
{
	WDT_CONTR |= 0x10;           //喂狗，清除看门狗计数器
}
void EEPROMWriteConfiguration(struct sys_config *config)
{
	unsigned char i = 0;
	unsigned int temp = 0;
	for(i; i < CONFIG_SIZE - 2; i++)				//对结构体中每一字节求和
		temp += ((unsigned char *)config)[i];
	config->check_sum = temp;
	EEPROM_SectorErase(EE_ADDRESS1);				//将求和结果存放在结构体最后一个数字中
	EEPROM_write_n(EE_ADDRESS1, (unsigned char *)config, CONFIG_SIZE);
}
	
unsigned char EEPROMReadConfiguration(struct sys_config *config)
{
	unsigned char i = 0;
	unsigned int temp = 0;
	EEPROM_read_n(EE_ADDRESS1,(unsigned char *)config,CONFIG_SIZE);
	for(i; i < CONFIG_SIZE - 2; i++)    //对结构体中每一字节求和
		temp += ((unsigned char *)config)[i];
	if(temp == config->check_sum)        //检验数据是否正确完整，结构体最后两字节为检验求和字节
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
	LED1 = k^0x01;     //异或的目的是与原来的状态相反
}
   
void SysInit(void)            
{      
	PinInit();   //引脚初始化
	UartInit();   //串口1初始化,使用定时器1
	Uart2Init();  //串口2初始化，使用定时器3
	Timer0Init();  //定时器0初始化，外部计数模式
	Timer3Init();  //定时器3初始化，产生1ms溢出中断
	InitConfig(); //中断初始化
	ADCInit();    //ADC初始化 ，采集电源电压
	IIC_Init();   //硬件IIC初始化
	EnableWatchDog();   //使能看门狗，程序卡死4s后自动复位
	EEPROMReadConfiguration(&config);//读掉电保存数据
	BuzzerInit();  //蜂鸣器初始化，使用硬件PWM7
	EnableBuzzer(config.key_sound);
	LED(OFF);
	BatteryChargeEnable(1);  //允许电池充电
	Delay1ms(5);  
	Enable3V3Output(1);  //允许3.3V锂电池输出；
	EA = 1;         //开全局中断
}










   

























