#include "IIC.h"
#include "stc8a.h"
#include "RDA5807M.h"
#include "intrins.h"
#include "Delay.h"


//RDA5807���üĴ����飬ÿ���Ĵ��������ֽ�
//���üĴ���
unsigned int REG02 = 0x0000,REG03 = 0x0000,REG04 = 0x0040,REG05 = 0x9088;
//״̬�Ĵ���
unsigned int REG0A = 0x0000,REG0B = 0x0000;
/********************���ò���**************************/
unsigned int CHAN = 0;
unsigned char BAND = 0;    //Banc Select
														//00 = 87-108 MHz(Default)
														//01 = 76-91 MHz
														//10 = 76-108MHz
														//11 = 65-76 MHz
unsigned char SPACE = 0;	//Channel Spacing
														//00 = 100kHz(Default)	01 = 200kHz;
														//10 = 50kHz;						11 = 25kHz;
unsigned char SEEKTH = 8;		//Seek SNR threshold value
unsigned char VOLUME = 8;		//DAC Gain Control Bit(Volume)
														//0000 = min;		1111 = max
float current_frequency;	//��ǰƵ��
unsigned char RSSI;				//��ǰƵ���ź�ǿ��ֵ
/**************************************************************************/

/**
	* @brief 	RDA5807Mд�Ĵ���
	*					RDA5807M�ĵļĴ���Ϊ16λ��д�Ĵ���ʱֻ������д��
	*					���ܹ�����дһ��16λ�Ĵ���������ʹ��IIC����д��8�ֽ�
	*					������дRDA5807M�е�4����Ҫ���üĴ���REG02 ~ REG05
	* @param  ��
	* @retval ��
	*/
void RDA5807MWriteRegs(void)
{
	unsigned char reg[8];
	reg[0] = REG02 >> 8;
	reg[1] = REG02;
	reg[2] = REG03 >> 8;
	reg[3] = REG03;
	reg[4] = REG04 >> 8;
	reg[5] = REG04;
	reg[6] = REG05 >> 8;
	reg[7] = REG05;
	P_SW2 |= 0x10;
	I2C_NoAddr_Write_MultiBytes(RDA5807M_IIC_ADDR, 8, reg);
	P_SW2 &= ~0x10;
}
/**
	* @brief 	RDA5807M���Ĵ���
	*					RDA5807M�ĵļĴ���Ϊ16λ�����Ĵ���ʱֻ����������
	*					���ܹ�������һ��16λ�Ĵ���������ʹ��IIC��������4�ֽ�
	*					����������RDA5807M�е�2����Ҫ״̬�Ĵ���REG0A,REG0B
	* @param  ��
	* @retval ��
	*/
void RDA5807MReadRges(void)
{
	unsigned char read_buf[4];
	P_SW2 |= 0x10;
	I2C_NoAddr_Read_MultiBytes(RDA5807M_IIC_ADDR, 4, read_buf);
	P_SW2 &= ~0x10;
	REG0A = read_buf[0];
	REG0A <<= 8;
	REG0A |= read_buf[1];
	REG0B = read_buf[2];
	REG0B <<= 8;
	REG0B |= read_buf[3];
}
/**
	* @brief 	RDA5807M��ʼ������
	*					��ʼ����RDA5807MĬ����˯��ģʽ
	* @param  ��
	* @retval 0��ʧ��
	*					1���ɹ�
	*/
unsigned char RDA5807MInit(void)
{
  // ���������λָ��
  REG02 |= SOFT_RST;
  RDA5807MWriteRegs();
  Delay1ms(10);
	//REG02 = 0xd001;	  //��ֹ�����������ֹ������˫������Bass boost enabled ��seek up,seek enable,ѭ������ģʽ��ʱ��ԴƵ�ʣ�32.768khz��Power Up Enable
	REG02 = 0x9000 | DMUTE;
	REG03 = (CHAN << 6) | (BAND << 2) | SPACE;
	REG04 = 0x0000;
	REG05 = 0x8080 | (SEEKTH << 8) | VOLUME;
  RDA5807MWriteRegs();
	return 1;
}
/**
	* @brief 	RDA5807M�趨����
	* @param  VOLUME��������0~15��
	* @retval ��
	*/
void RDA5807MSetVOLUME(unsigned int VOLUME)
{
	if(VOLUME > 0x0f)				//�޷�
		VOLUME = 0x0f;
	REG05 &= ~0x000f;				//�����ԭ����ֵ
	REG05 |= VOLUME & 0x0f;	//д������ֵ
  RDA5807MWriteRegs();		//д��Ĵ���
}
/**
	* @brief 	RDA5807M�Զ���̨
	* @param  direction����������
	*						UPWARD		������̨
	*						DOWNWARD	������̨
	* @retval ��
	*/
float RDA5807MSEEK(unsigned char direction)
{
	static unsigned char step = 0;
	if(step == 0)
	{
		REG03 &= ~TUNE; 		//disable tune
		if(direction == UPWARD) //seek up
			REG02 |= SEEK_UP;
		else								//seek down
			REG02 &= ~SEEK_UP;
		REG02 |= SEEK;			//enable seek
		RDA5807MWriteRegs();
		
		step++;
	}
	else if(step == 1)
	{
		RDA5807MReadRges();
		if((REG0A & 0x4000))//�ȴ�STC��־��λ
		{
			REG02 &= ~SEEK;					//disable seek
			CHAN = REG0A & 0x03ff;
			current_frequency = (float)CHAN * 0.1 + 87;
			RSSI = (REG0B >> 9)&0x0003f;
			step = 0;
			return current_frequency;
		}
		else
			return 0;
  }
	return 0;
}
/**
	* @brief 	RDA5807M�趨Ƶ�ʣ���Ƶ��
	* @param  fq����̨��Ƶ�ʣ�87 ~ 108��
	* @retval ��
	*/
void RDA5807MSetFq(float fq)
{
	REG02 &= ~SEEK;			//disable seek
	REG03 |= TUNE; 			//enable tune
	CHAN = fq * 10 - 870;
	REG03 &= 0x003f;
	REG03 |= (CHAN << 6);
	RDA5807MWriteRegs();
	RDA5807MReadRges();
	while((REG0A & 0x4000) == 0)		//�ȴ�STC ��־��λ
  {
		Delay1ms(3);
    RDA5807MReadRges();	// ��ȡ״̬�Ĵ���
  }
	REG03 &= ~TUNE;				//disable tune
	current_frequency = fq;
	RSSI = (REG0B >> 9)&0x0003f;
}
/**
	* @brief 	��ȡ��ǰ������Ƶ����Ƶ��
	* @param  ��
	* @retval current_frequency����ǰ����Ƶ����Ƶ��
	*/
float RDA5807MGetCurrentFq(void)
{
	RDA5807MReadRges();
	CHAN = REG0A & 0x03ff;
	current_frequency = (float)CHAN * 0.1 + 87;
	return current_frequency;
}
/**
	* @brief 	��ȡ��ǰ������Ƶ�����ź�ǿ��ֵ
	* @param  ��
	* @retval RSSI����ǰ����Ƶ�����ź�ǿ��ֵ
	*/
unsigned char RDA5807MGetCurrentRSSI(void)
{
	RDA5807MReadRges();
	RSSI = (REG0B >> 9)&0x0003f;
	RSSI = (float)CHAN * 0.1 + 87;
	return RSSI;
}
/**
	* @brief 	ʹRDA5807M��Ƶ�������
	* @param  ��
	* @retval ��
	*/
void RDA5807MSetMute(void)
{
	REG02 &= ~DMUTE;
	RDA5807MWriteRegs();
}
/**
	* @brief 	RDA5807M�ϵ翪��
	* @param  ��
	* @retval ��
	*/
void RDA5807MPowerUp(void)
{
	REG02 |= POWER_EN;
	RDA5807MWriteRegs();
}
/**
	* @brief 	RDA5807M�ػ����磨˯��ģʽ��
	* @param  ��
	* @retval ��
	*/
void RDA5807MPowerDown(void)
{
	REG02 &= ~POWER_EN;
	RDA5807MWriteRegs();
}


