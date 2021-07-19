#include "stc8a.h"
#include "IIC.h"
#include "Delay.h"

unsigned int data timeout;
 
 

void IIC_Init()
{
	P_SW2 |= 0x80;     //����˿ڿ��ƼĴ���
	I2CCFG = 0xd0;    //ʹ��I2C����ģʽ,ֻ��������ģʽʱ������ʱ��������Ч�� ��Ӧ��ʱ��������Ϊ2^5+1=65
	I2CMSST = 0x00;    //I2C����״̬�Ĵ���������Ϊ���ڿ���״̬�������ж�������Ӧ�������������㣻
	P_SW2 &= 0X7f;      //�ر�����˿ڼĴ�����I2Cѡ��P3.2 P3.2,�Ƚ�λ	P4.1,����4 :P5.2,P5.3, ,����3��P5.0 P5.1������2��P4.0,P4.2
}	

void Wait()
{
	timeout = 300;
	while (!(I2CMSST & 0x40))
	{
		if(--timeout == 0)
			break;
	}
	I2CMSST &= ~0x40;   //i2cMSST����λ���ֲ��䣬�ж�����λ�������		
}
void I2C_Start()
{
	I2CMSCR = 0x01;    //����STARTָ��
	Wait();
}
void I2C_Stop()
{
	I2CMSCR = 0x06;   //����STOPָ��
	Wait();
}
void I2C_SendByte(unsigned char dat)
{
	I2CTXD = dat;        //д���ݵ����ݻ�����
	I2CMSCR = 0x02;       //����SEND����
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
	I2CMSCR = 0x03;       //���Ͷ�ACK����
	Wait();
}
void I2C_SendACK()
{
	I2CMSST = 0x00;          //����ACK�ź�
	I2CMSCR = 0x05;          //����ACK����
	Wait();
}
void I2C_SendNAK()
{
	I2CMSST = 0x01;          //����NACK�ź�
	I2CMSCR = 0x05;
	Wait();
}
//********************************************
//��I2C�豸д��һ���ֽ����ݣ�
//********************************************
void Single_WriteIIC(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data)
{
	P_SW2 |= 0x80;                 //�����ⲿ�Ĵ�����Ҫ��P_SW2�����λ��1
	I2C_Start();                   //��ʼ�ź� 
	I2C_SendByte(SlaveAddress);   //���ʹ��豸��ַ+д�ź�
	I2C_RecvACK();      
	I2C_SendByte(REG_Address);     //�ڲ��Ĵ�����ַ
	I2C_RecvACK();
	I2C_SendByte(REG_data);        //�ڲ��Ĵ�������
	I2C_RecvACK();
	I2C_Stop();                   //����ֹͣ�ź�
	P_SW2 &= 0x7f;                 //���ʽ��������λ����
}
unsigned char Single_ReadIIC(unsigned char SlaveAddress,unsigned char REG_Address)
{
	unsigned char REG_data;
	P_SW2 |= 0x80;
	I2C_Start();                          //��ʼ�ź�
	I2C_SendByte(SlaveAddress);           //���ʹ��豸��ַ+д�ź�
	I2C_RecvACK();
	I2C_SendByte(REG_Address);            //�洢��Ԫ��ַ����0��ʼ
	I2C_RecvACK();
	I2C_Start();                          //��ʼ�ź�
	I2C_SendByte(SlaveAddress);           //���ʹ��豸��ַ+���ź�
	I2C_RecvACK();
	REG_data = I2C_RecvByte();            //�����Ĵ������ݱ���
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
	I2C_SendByte(DeviceAddr+1);        //�����豸��ַ+���ź�
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
	P_SW2 &=  0x7f;                    //�رն�����洢ȡ�õ�Ѱַ
}
void I2C_Read_MultiBytes(unsigned char DeviceAddr,unsigned char address,unsigned char BytesNum,unsigned char *buf)
{
	unsigned char i;
	P_SW2 |= 0x80;                     //�������ⲿ�洢����Ѱַ
	I2C_Start();
	I2C_SendByte(DeviceAddr);          //�����豸��ַ+д�ź�
	I2C_RecvACK();
	I2C_SendByte(address);             // �ڲ��Ĵ�����ַ
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
               



