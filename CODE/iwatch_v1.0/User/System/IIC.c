#include "stc8a.h"
#include "IIC.h"
#include "Delay.h"

unsigned int data timeout;
 
 

void IIC_Init()
{
	P_SW2 |= 0x80;     //外设端口控制寄存器
	I2CCFG = 0xd0;    //使能I2C主机模式,只有在主机模式时，配置时钟数才有效， 对应的时钟数设置为2^5+1=65
	I2CMSST = 0x00;    //I2C主机状态寄存器，配置为处于空闲状态、开启中断请求，响应请求后用软件清零；
	P_SW2 &= 0X7f;      //关闭外设端口寄存器，I2C选用P3.2 P3.2,比较位	P4.1,串口4 :P5.2,P5.3, ,串口3：P5.0 P5.1，串口2：P4.0,P4.2
}	

void Wait()
{
	timeout = 300;
	while (!(I2CMSST & 0x40))
	{
		if(--timeout == 0)
			break;
	}
	I2CMSST &= ~0x40;   //i2cMSST其他位保持不变，中断请求位软件清零		
}
void I2C_Start()
{
	I2CMSCR = 0x01;    //发送START指令
	Wait();
}
void I2C_Stop()
{
	I2CMSCR = 0x06;   //发送STOP指令
	Wait();
}
void I2C_SendByte(unsigned char dat)
{
	I2CTXD = dat;        //写数据到数据缓冲区
	I2CMSCR = 0x02;       //发送SEND命令
	Wait();
}
unsigned char I2C_RecvByte()
{
	I2CMSCR = 0x04;
	Wait();
	return I2CRXD;
}
void I2C_RecvACK()
{
	I2CMSCR = 0x03;       //发送读ACK命令
	Wait();
}
void I2C_SendACK()
{
	I2CMSST = 0x00;          //设置ACK信号
	I2CMSCR = 0x05;          //发送ACK命令
	Wait();
}
void I2C_SendNAK()
{
	I2CMSST = 0x01;          //设置NACK信号
	I2CMSCR = 0x05;
	Wait();
}
//********************************************
//向I2C设备写入一个字节数据；
//********************************************
void Single_WriteIIC(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data)
{
	P_SW2 |= 0x80;                 //访问外部寄存器需要将P_SW2的最高位置1
	I2C_Start();                   //起始信号 
	I2C_SendByte(SlaveAddress);   //发送从设备地址+写信号
	I2C_RecvACK();      
	I2C_SendByte(REG_Address);     //内部寄存器地址
	I2C_RecvACK();
	I2C_SendByte(REG_data);        //内部寄存器数据
	I2C_RecvACK();
	I2C_Stop();                   //发送停止信号
	P_SW2 &= 0x7f;                 //访问结束后最高位清零
}
unsigned char Single_ReadIIC(unsigned char SlaveAddress,unsigned char REG_Address)
{
	unsigned char REG_data;
	P_SW2 |= 0x80;
	I2C_Start();                          //起始信号
	I2C_SendByte(SlaveAddress);           //发送从设备地址+写信号
	I2C_RecvACK();
	I2C_SendByte(REG_Address);            //存储单元地址，从0开始
	I2C_RecvACK();
	I2C_Start();                          //起始信号
	I2C_SendByte(SlaveAddress);           //发送从设备地址+读信号
	I2C_RecvACK();
	REG_data = I2C_RecvByte();            //读出寄存器数据保存
	I2C_SendNAK();
	I2C_Stop();
	P_SW2 &= 0x7f; 
	return REG_data;
}
void I2C_NoAddr_Write_MultiBytes(unsigned char DeviceAddr,unsigned char BytesNum,unsigned char *buf)
{
	unsigned char i = 0;
	P_SW2 |= 0x80;
	I2C_Start();
	I2C_SendByte(DeviceAddr);
	I2C_RecvACK();
	for(i = 0; i < BytesNum; i++)
	{
		I2C_SendByte(buf[i]);
		I2C_RecvACK();
	}
	I2C_Stop();
	P_SW2 &= 0x7f;
}
void I2C_NoAddr_Read_MultiBytes(unsigned char DeviceAddr,unsigned char BytesNum,unsigned char *buf)
{
	unsigned char i;
	P_SW2 |= 0x80;
	I2C_Start();
	I2C_SendByte(DeviceAddr+1);        //发送设备地址+读信号
	I2C_RecvACK();
	for(i = 0; i < (BytesNum - 1); i++ )
	{
		buf[i] = I2C_RecvByte();
		I2C_SendNAK();
		I2C_Stop();
	}
	buf[i] = I2C_RecvByte();
	I2C_SendNAK();
	I2C_Stop();
	P_SW2 &=  0x7f;                    //关闭对外设存储取悦的寻址
}
void I2C_Read_MultiBytes(unsigned char DeviceAddr,unsigned char address,unsigned char BytesNum,unsigned char *buf)
{
	unsigned char i;
	P_SW2 |= 0x80;                     //开启对外部存储器的寻址
	I2C_Start();
	I2C_SendByte(DeviceAddr);          //发送设备地址+写信号
	I2C_RecvACK();
	I2C_SendByte(address);             // 内部寄存器地址
	I2C_RecvACK();
	I2C_Start();
	I2C_SendByte(DeviceAddr+1);
	for(i = 0; i < (BytesNum-1); i++)
	{
		buf[i] = I2C_RecvByte();
		I2C_SendACK();
	}
	buf[i] = I2C_RecvByte();
	I2C_SendNAK();
	I2C_Stop();
	P_SW2 &= 0x7f;          
}
               



