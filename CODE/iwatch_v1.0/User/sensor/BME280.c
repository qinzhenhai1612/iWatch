#include "stc8a.h"
#include "bme280.h"
#include "IIC.h"
#include "Delay.h"

//sbit CSB = P0^0;	//Ƭѡ�ź�
//sbit SDO = P0^1;	//IIC��ַ������0��0xec�� 1��0xee
/*************************��������*****************************/
//����������ÿ���ϵ���BME280�ж�ȡһ�μ���
//Compensation Parameter for temperature
unsigned int	dig_T1;
int dig_T2, dig_T3;
//Compensation Parameter for pressure
unsigned int dig_P1;
int dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
//Compensation Parameter for humidity
unsigned char dig_H1, dig_H3;
int dig_H2, dig_H4, dig_H5;
char dig_H6;
long t_fine;
/*******************δ�����Ĵ�����ԭʼ����*********************/
unsigned long uncomp_pressure;
unsigned long uncomp_temperature;
unsigned int 	uncomp_humidity;
/**********************���ò���********************************/
//BME280����ģʽ�� 0�����ߣ� 1/2�����β���ģʽ�� 3����������ģʽ
unsigned char BME280_mode = 0;		//0:sleep mode, 1/2:forced mode, 3:normal mode
//������������ ����Խ����������ԽС����̬��ӦԽ����β�������ʱ��Խ����Ϊ0ʱ�ò����Ĳ�������
unsigned char osrs_h = 5, osrs_t = 5, osrs_p = 5;		//000: measurement skipped	001:oversampling x 1
																										//010: oversampling x 2			011:oversampling x 4
																										//100: oversampling x 8			101:oversampling x 16
//��������ģʽ�£����β��������ʱ��t_sb
unsigned char t_sb = 2;			//000:0.5ms		001:62.5ms		010:125ms
														//011:250ms		100:500ms			101:1000ms
														//110:10ms		111:20ms
//�˲�ϵ��
unsigned char filter = 4;		//000:filter off							001:filter coeffcient = 2
														//010:filter coeffcient = 4		011:filter coeffcient = 8
														//100:filter coeffcient = 16
unsigned char spi3w_en = 0;	//0:disable 3-wire SPI		1:enable 3-wire SPI
/***************************************************************************/

/**
  * @brief  ʹ��IIC������BME280�ļĴ�����дһ�ֽ�����
  * @param  addr: �Ĵ����ĵ�ַ
  * @param  dat: 	��д�������
  * @retval ��
  */
void BME280WriteByte(unsigned char addr, unsigned char dat)
{
	Single_WriteIIC(BME280_IIC_ADDR, addr, dat);
}
/**
  * @brief  ʹ��IIC���ߴ�BME280�ļĴ����ж�һ�ֽ�����
  * @param  addr: �Ĵ����ĵ�ַ
  * @retval ������һ�ֽ�����
  */
unsigned char BME280ReadByte(unsigned char addr)
{
	unsigned char temp;
	temp = Single_ReadIIC(BME280_IIC_ADDR,addr);
	return temp;
}
/**
  * @brief  �����BME280�������Ƿ�����
  * @param  ��
  * @retval 1���ɹ�����0��ʧ�ܣ�
  */
unsigned char BME280CheckConnection(void)
{
	if(BME280ReadByte(BME280_ID) == 0x60)
		return 1;
	else
		return 0;
}
/**
	* @brief �������õĲ���������ÿ�β�����������ʱ��
	* @param  ��
	* @retval ���뵥λ��ʱ��
	*/
unsigned int BME280_cal_meas_delay(void)
{
    unsigned char max_delay;
    unsigned char temp_osr;
    unsigned char pres_osr;
    unsigned char hum_osr;

    /*Array to map OSR config register value to actual OSR */
    unsigned char osr_sett_to_act_osr[] = { 0, 1, 2, 4, 8, 16 };

    /* Mapping osr settings to the actual osr values e.g. 0b101 -> osr X16  */
    if (osrs_t <= 5)
    {
        temp_osr = osr_sett_to_act_osr[osrs_t];
    }
    else
    {
        temp_osr = 16;
    }

    if (osrs_p <= 5)
    {
        pres_osr = osr_sett_to_act_osr[osrs_p];
    }
    else
    {
        pres_osr = 16;
    }

    if (osrs_h <= 5)
    {
        hum_osr = osr_sett_to_act_osr[osrs_h];
    }
    else
    {
        hum_osr = 16;
    }

    max_delay = (unsigned int)((1.25 + 2.3 * temp_osr) + (2.3 * pres_osr + 0.575) + (2.3 * hum_osr + 0.575));

    return max_delay;
}
/**
	* @brief 	��BME280�ж�ȡ��������
	*					ÿ�������ϵ�����һ�μ���
	* @param  ��
	* @retval ��
	*/
void BME280ReadCompensationParameter()
{
	unsigned char buf1[26], buf2[7];
	I2C_Read_MultiBytes(BME280_IIC_ADDR, CALIB00, 26, buf1);
	I2C_Read_MultiBytes(BME280_IIC_ADDR, CALIB26, 7, buf2);
	
	dig_T1 = (buf1[1] << 8) | buf1[0];
	dig_T2 = (buf1[3] << 8) | buf1[2];
	dig_T3 = (buf1[5] << 8) | buf1[4];
	dig_P1 = (buf1[7] << 8) | buf1[6];
	dig_P2 = (buf1[9] << 8) | buf1[8];
	dig_P3 = (buf1[11] << 8) | buf1[10];
	dig_P4 = (buf1[13] << 8) | buf1[12];
	dig_P5 = (buf1[15] << 8) | buf1[14];
	dig_P6 = (buf1[17] << 8) | buf1[16];
	dig_P7 = (buf1[19] << 8) | buf1[18];
	dig_P8 = (buf1[21] << 8) | buf1[20];
	dig_P9 = (buf1[23] << 8) | buf1[22];
	dig_H1 = buf1[25];
	dig_H2 = (buf2[1] << 8) | buf2[0];
	dig_H3 = buf2[2];
	dig_H4 = (buf2[3] << 4) | buf2[4];
	dig_H5 = (buf2[5] << 4) | (buf2[4] >> 4);
	dig_H6 = buf2[6];
}
/**
	* @brief 	��BME280�ж�ȡ��������ԭʼ����
	* @param  ��
	* @retval ��
	*/
void BME280ReadSensorRawData(void)
{
	unsigned char buf[8];
	I2C_Read_MultiBytes(BME280_IIC_ADDR, PRESS_MSB, 8, buf);
	uncomp_pressure 	 = ((unsigned long)buf[0] << 12) | ((unsigned long)buf[1] << 4) | (buf[2] >> 4);
	uncomp_temperature = ((unsigned long)buf[3] << 12) | ((unsigned long)buf[4] << 4) | (buf[5] >> 4);
	uncomp_humidity 	 = ((unsigned int)buf[6] << 8) | buf[7];
}
/**
	* @brief This internal API is used to compensate the raw temperature data and
	* return the compensated temperature data in float data type.
	*/
float compensate_temperature(void)
{
	float var1;
  float var2;
  float temperature;
  float temperature_min = -40;
  float temperature_max = 85;

	var1 = ((float)uncomp_temperature) / 16384.0 - ((float)dig_T1) / 1024.0;
  var1 = var1 * ((float)dig_T2);
  var2 = (((float)uncomp_temperature) / 131072.0 - ((float)dig_T1) / 8192.0);
  var2 = (var2 * var2) * ((float)dig_T3);
  t_fine = (long)(var1 + var2);
  temperature = (var1 + var2) / 5120.0;
  if (temperature < temperature_min)
  {
		temperature = temperature_min;
  }
  else if (temperature > temperature_max)
  {
    temperature = temperature_max;
  }
	return temperature;
}
/**
	* @brief This internal API is used to compensate the raw pressure data and
	* return the compensated pressure data in float data type.
	*/
float compensate_pressure(void)
{
    float var1;
    float var2;
    float var3;
    float pressure;
    float pressure_min = 30000.0;
    float pressure_max = 110000.0;

    var1 = ((float)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((float)dig_P6) / 32768.0;
    var2 = var2 + var1 * ((float)dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((float)dig_P4) * 65536.0);
    var3 = ((float)dig_P3) * var1 * var1 / 524288.0;
    var1 = (var3 + ((float)dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((float)dig_P1);

    /* avoid exception caused by division by zero */
    if (var1 > (0.0))
    {
        pressure = 1048576.0 - (float) uncomp_pressure;
        pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = ((float)dig_P9) * pressure * pressure / 2147483648.0;
        var2 = pressure * ((float)dig_P8) / 32768.0;
        pressure = pressure + (var1 + var2 + ((float)dig_P7)) / 16.0;
        if (pressure < pressure_min)
        {
            pressure = pressure_min;
        }
        else if (pressure > pressure_max)
        {
            pressure = pressure_max;
        }
    }
    else /* Invalid case */
    {
        pressure = pressure_min;
    }

    return pressure;
}
/**
	* @brief This internal API is used to compensate the raw humidity data and
	* return the compensated humidity data in float data type.
	*/
float compensate_humidity(void)
{
	float humidity;
  float humidity_min = 0.0;
  float humidity_max = 100.0;
  float var1;
  float var2;
  float var3;
  float var4;
  float var5;
  float var6;

  var1 = ((float)t_fine) - 76800.0;
  var2 = (((float)dig_H4) * 64.0 + (((float)dig_H5) / 16384.0) * var1);
  var3 = uncomp_humidity - var2;
  var4 = ((float)dig_H2) / 65536.0;
  var5 = (1.0 + (((float)dig_H3) / 67108864.0) * var1);
  var6 = 1.0 + (((float)dig_H6) / 67108864.0) * var1 * var5;
  var6 = var3 * var4 * (var5 * var6);
  humidity = var6 * (1.0 - ((float)dig_H1) * var6 / 524288.0);

  if (humidity > humidity_max)
  {
		humidity = humidity_max;
  }
  else if (humidity < humidity_min)
  {
		humidity = humidity_min;
  }
  return humidity;
}
/**
	* @brief 	��BME280�ж�ȡ�������������������
	* @param  ��
	* @retval ��
	*/
void BME280GetSensorData(struct bme280_data *p)
{
	BME280ReadSensorRawData();
	p->temperature = compensate_temperature();
	p->pressure = compensate_pressure();
	p->humidity = compensate_humidity();
}
/**
	* @brief 	BME280��λ
	* @param  ��
	* @retval ��
	*/
void BME280SoftReset(void)
{
	BME280WriteByte(RESET, 0xB6);
	Delay1ms(5);
}
/**
	* @brief 	BME280��ʼ������
	* @param  ��
	* @retval 0����ʼ��ʧ��
	*				 	1����ʼ���ɹ�
	*/
unsigned char BME280Init()
{
	unsigned char time_out = 200;
	//Ӳ����·���Ѿ���оƬ��������������������
	//����������������
	//CSB = 1;
	//SDO = 0;
	Delay1ms(5);
	while(BME280CheckConnection() == 0)
	{
		Delay1ms(1);
		if(--time_out == 0)
			return 0;
	}
	BME280SoftReset();
	BME280ReadCompensationParameter();
	BME280WriteByte(CONFIG, ((t_sb << 5) | (filter << 2) | spi3w_en));
	BME280WriteByte(CTRL_HUM, osrs_h);
	BME280WriteByte(CTRL_MEAS, ((osrs_t << 5) | (osrs_p << 2) | BME280_mode));
	return 1;
}
/**
	* @brief �趨BME280�Ĺ���ģʽ
	* @param  mode:����ģʽѡ��
	*				SLEEP_MODE	˯��ģʽ���͹��ģ�
	*				FORCED_MODE	���β���ģʽ
	*				NORMAL_MODE	�Զ���������ģʽ
	* @retval ��
	*/
void BME280SetMode(unsigned char mode)
{
	BME280_mode = mode;
	BME280WriteByte(CTRL_MEAS, (osrs_t << 5) | (osrs_p << 2) | BME280_mode);
}
/**
	* @brief BME280�趨Ϊ���β���ģʽ
	* @param  p:bme280_data�ṹ���ָ��
	* @retval ��
	*/
void BME280SingleMeasurement(struct bme280_data *p)
{
	BME280SetMode(FORCED_MODE);
	Delay1ms(BME280_cal_meas_delay());
	BME280GetSensorData(p);
}
/**
	* @brief BME280�趨Ϊ�Զ���������ģʽ���ú���ִ��һ��֮��
	*				����BME280GetSensorData��ȡ��������������
	* @param  t�����β���֮������ʱ��
	* 				MS_0_5	0.5ms		MS_10		10ms
	*					MS_20		20ms		MS_62_5	62.5ms
	*					MS_125	125ms		MS_250	250ms
	*					MS_500	500ms		MS_1000	1000ms
	* @retval ��
	*/
void BME280ContinuousMeasurement(unsigned char t)
{
	if(t > 7)
		t = 7;
	t_sb = t;
	BME280WriteByte(CONFIG, ((t_sb << 5) | (filter << 2) | spi3w_en));
	BME280SetMode(NORMAL_MODE);
}
