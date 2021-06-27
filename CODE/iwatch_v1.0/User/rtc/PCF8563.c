#include "IIC.h"
#include "PCF8563.h"

/**
  * @brief   ʹ��IIC������PCF8563�ļĴ�����дһ�ֽ�����
	* @param   addr:�Ĵ����ĵ�ַ	
	* @param   dat: ��д��ĵ�ַ
	* @retval  none
  */
	void PCF8563WriteByte(unsigned char addr,unsigned char dat)
	{
		Single_WriteIIC(PCF8563_IIC_ADDR,addr,dat);
	}
	/**
  * @brief  ʹ��IIC���ߴ�PCF8563�ļĴ����ж�һ�ֽ�����
  * @param  addr: �Ĵ����ĵ�ַ
  * @retval ������һ�ֽ�����
  */
	unsigned char PCF8563ReadByte(unsigned char addr)
	{
		unsigned char temp;
		temp = Single_ReadIIC(PCF8563_IIC_ADDR,addr);
		return temp;
	}
	/**
	  * @brief  ��λ�������PCF8563�Ĵ����е�ָ��λ
		* @param  addr: �Ĵ�����ַ
							bit_mask: ָ��λ
							SC: 0 �����1 ��λ
		* @retval ��
		*/
	void  PCF8563SetRegisterBits(unsigned char addr,unsigned char bit_mask,unsigned char SC)
	{
		unsigned char temp;
		temp = PCF8563ReadByte(addr);
		if(SC == 1)
			temp |= bit_mask;
		else if(SC == 0)
			temp &= ~bit_mask;
		PCF8563WriteByte(addr,temp);
	}
	/**
  * @brief  ��ȡPCF8563�е�ʱ�䲢����ʱ��ṹ����
  * @param  t��pcf8563_time�ͽṹ���ָ��
  * @retval None
  */
	void PCF8563ReadTime(struct pcf8563_time *t)
{
	unsigned char buf[7];
	I2C_Read_MultiBytes(PCF8563_IIC_ADDR, VL_SECONDS, 7, buf);
	buf[0] &= 0x7f;
	t->second = (buf[0] >> 4) * 10 + (buf[0] & 0x0f);
	buf[1] &= 0x7f;
	t->minute = (buf[1] >> 4) * 10 + (buf[1] & 0x0f);
	buf[2] &= 0x3f;
	t->hour 	= (buf[2] >> 4) * 10 + (buf[2] & 0x0f);
	buf[3] &= 0x3f;
	t->day 		= (buf[3] >> 4) * 10 + (buf[3] & 0x0f);
	buf[4] &= 0x07;
	if(buf[4] == 0)
		buf[4] = 7;
	t->weekday = buf[4];
	if(buf[5] & 0x80 == 0x80)
		t->century += 1;
	buf[5] &= 0x1f;
	t->month = (buf[5] >> 4) * 10 + (buf[5] & 0x0f);
	t->year = (unsigned int)buf[6] + 2000;
}
/**
  * @brief  ����ʱ�䣬����ʮ����ʱ�����
	*					����PCF8563WriteTime(23, 59, 50);	�趨ʱ��Ϊ12��59��50
	* @param  hour,min,sec��Сʱ�����ӣ�����
  * @retval None
  */
void PCF8563WriteTime(unsigned char hour, unsigned char min, unsigned char sec)
{
	PCF8563WriteByte(VL_SECONDS, ((sec / 10) << 4) | (sec % 10));
	PCF8563WriteByte(MINUTES, ((min / 10) << 4) | (min % 10));
	PCF8563WriteByte(HOURS, ((hour / 10) << 4) | (hour % 10));
}
/**
  * @brief  �������ڣ�����ʮ�������ڲ���
	*					����PCF8563WriteDate(2020, 3, 3, 1);
	*					�趨����Ϊ2020��3��3����һ
	* @param  year,month,day,weekday���꣬�£��գ�����(��һ�����߶�Ӧ1~7)
  * @retval None
  */
void PCF8563WriteDate(unsigned int year, unsigned char month, unsigned char day, unsigned char weekday)
{
	year -= 2000;
	PCF8563WriteByte(YEARS, (unsigned char)year);
	PCF8563WriteByte(CENTURY_MOUTHS, ((month / 10) << 4) | (month % 10));
	PCF8563WriteByte(DAYS, ((day / 10) << 4) | (day % 10));
	if(weekday == 7)
		weekday = 0;
	PCF8563WriteByte(WEEKDAYS, weekday);
}
/**
  * @brief  ��������
	* @param  hour,min��Сʱ������
	*					day,weekday���գ�����(��һ�����߶�Ӧ1~7)
	*					mode�����ӵ�ģʽ
	*						ALARM_DISABLE				�ر�����
	*						ALARM_ONCE					������Ч
	*						ALARM_EVERYDAY			ÿ�춼��
	*						ALARM_WORKDAY				��������
	*						ALARM_SPECIFIC_DAY	ָ��ĳһ����
  * @retval None
  */
void PCF8563SetAlarm(unsigned char hour, unsigned char min, unsigned char weekday, unsigned char day, unsigned char mode)
{
	unsigned char temp1 = 0;
	unsigned char temp2 = 0;
	unsigned char temp3 = 0;
	unsigned char temp4 = 0;
	temp1 = 0x80 | ((hour / 10) << 4) | (hour % 10);
	temp2 = 0x80 | ((min / 10) << 4) | (min % 10);
	temp3 = 0x80 | weekday;
	temp4 = 0x80 | ((day / 10) << 4) | (day % 10);
	if(mode == ALARM_EVERYDAY || mode == ALARM_ONCE)
	{
		temp1 &= ~0x80;
		temp2 &= ~0x80;
	}
	else if(mode == ALARM_WORKDAY)
	{
		temp1 &= ~0x80;
		temp2 &= ~0x80;
		temp3 &= ~0x80;
	}
	else if(mode == ALARM_SPECIFIC_DAY)
	{
		temp1 &= ~0x80;
		temp2 &= ~0x80;
		temp4 &= ~0x80;
	}
	PCF8563WriteByte(HOUR_ALARM, temp1);
	PCF8563WriteByte(MINUTE_ALARM, temp2);
	PCF8563WriteByte(WEEKDAY_ALARM, temp3);
	PCF8563WriteByte(DAY_ALARM, temp4);
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x02, 1);	//ʹ��ʱ���ж����
}
/**
  * @brief  ���PCF8563�ڲ����ӱ�־λ
	*					�����жϲ����󣬱��������־λ����һ�β�����������
	* @param  ��
  * @retval ��
  */
void PCF8563ClearAlarmFlag(void)
{
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x08, 0);
}
/**
  * @brief  �ر�����
	* @param  ��
  * @retval ��
  */
void PCF8563DisableAlarm(void)
{
	PCF8563SetRegisterBits(HOUR_ALARM, 0x80, 1);
	PCF8563SetRegisterBits(MINUTE_ALARM, 0x80, 1);
	PCF8563SetRegisterBits(WEEKDAY_ALARM, 0x80, 1);
	PCF8563SetRegisterBits(DAY_ALARM, 0x80, 1);
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x08, 0);
}
/**
  * @brief  ʹ��PCF8563�ڲ���ʱ���������ж�
	* @param  clock_fq��PCF8563�ڲ���ʱ����ʱ��Դ
	*						TIMERCLK_4096_HZ	4.096khz
	*						TIMERCLK_64_HZ		64hz
	*						TIMERCLK_1_HZ			1hz
	*						TIMERCLK_1_60_HZ	1/60hz
	*					value������ʱ��ֵ
  * @retval ��
  */
void PCF8563EnableTimer(unsigned char clock_fq, unsigned char value)
{
	PCF8563SetRegisterBits(TIMER_CONTROL, 0x80, 0);	//�رն�ʱ��
	PCF8563WriteByte(TIMER_CONTROL, clock_fq);			//���ö�ʱ����ʱ��Դ
	PCF8563WriteByte(TIMER, value);									//���ö�ʱ���ĳ�ֵ
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x01, 1);	//���ж����
	PCF8563SetRegisterBits(TIMER_CONTROL, 0x80, 1);			//�򿪶�ʱ��
}
/**
  * @brief  ���PCF8563�ڲ���ʱ����־λ
	*					����ʱ��ʱ���жϲ����󣬱��������־λ����һ�β�����������
	* @param  ��
  * @retval ��
  */
void PCF8563ClearTimerFlag(void)
{
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x04, 0);
}
/**
  * @brief  �ر�PCF8563�ڲ���ʱ��
	* @param  ��
  * @retval ��
  */
void PCF8563DisableTimer(void)
{
	PCF8563SetRegisterBits(CONTROL_STATUS_2, 0x01, 0);	//�ر��ж����
	PCF8563SetRegisterBits(TIMER_CONTROL, 0x80, 0);			//�رն�ʱ��
}
/**
  * @brief  ��PCF8563�ж�Դ
	* @param  ��
  * @retval temp��PCF8563�ڲ�״̬�Ĵ�����ֵ
  */
unsigned char PCF8563ReadIntSrc(void)
{
	unsigned char temp;
	temp = PCF8563ReadByte(CONTROL_STATUS_2);
	temp &= 0x0c;
	return temp;
}
/**
	* @brief 	ʹ��PCF8563��ʱ�����
	* @param  clock_fq�������ʱ��Ƶ��
	*							CLKOUT_32768_HZ		32.768khz
	*							CLKOUT_1024_HZ		1.924khz
	*							CLKOUT_32_HZ			32hz
	*							CLKOUT_1_HZ				1hz
	*					en��ENABLE_CLKOUT			ʹ��ʱ�����
	*							DISABLE_CLKOUT		��ֹʱ�����
	* @retval ��
	*/
void PCF8563EnableClockOuput(unsigned char clock_fq, unsigned char en)
{
	PCF8563WriteByte(CLKOUT_CONTROL, (en << 7) | clock_fq);
}
void PCF8563Init(void)
{
//	PCF8563ClearAlarmFlag();
//	PCF8563ClearTimerFlag();
//	PCF8563EnableTimer(TIMERCLK_1_60_HZ, 1);
	//PCF8563DisableTimer();
	PCF8563EnableClockOuput(CLKOUT_1024_HZ, DISABLE_CLKOUT);
}
	
	
	
	
	
	
	
	
	
	
	
	
	






