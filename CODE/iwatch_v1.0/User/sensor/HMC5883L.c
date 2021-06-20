#include "HMC5883L.h"
#include "IIC.h"
#include "Delay.h"
#include "math.h"

/******************************���ò���*************************************/
unsigned char MA = 3;		//number of samples averaged (1 to 8) per measurement output.
												//00 = 1; 01 = 2; 10 = 4; 11 = 8(Default)
unsigned char DO = 6;		//Typical Data Output Rate (Hz) 
												//000 = 0.75;		001 = 1.5;		010 = 3;		011 = 7.5
												//100 = 15(Default)		101 = 30;		110 = 75;		111 = Not Use;
unsigned char MS = 0;		//Measurement Mode 
												//00 = Normal measurement configuration (Default). 
												//01 = a positive current is forced across the resistive load for all three axes.
												//10 = a negative current is forced across the resistive load for all three axes.
												//11 = reserved
unsigned char GN = 1;		//Gain Configuration (GN = Recommended Sensor Field Range(Guass))
												//000 = +/-0.8Ga;		001 = +/-1.3Ga;		010 = +/-1.9Ga;		
												//011 = +/-2.5Ga;		100 = +/-4.0Ga;		101 = +/-4.7Ga;		
												//110 = +/-5.6Ga;		111 = +/-8.1Ga;		
unsigned char MD = 2;		//Operating Mode 
												//00 = Continuous-Measurement Mode.
												//01 = Single-Measurement Mode (Default).
												//10 = Idle Mode.
/*****************************У׼����************************************/											
int mag_x_offset = 0, mag_y_offset = 0, mag_z_offset = 0;		//��λУ׼����
float mag_y_scale = 1, mag_z_scale = 1;											//����У׼����
/*************************************************************************/

/**
  * @brief  ʹ��IIC������HMC5883L�ļĴ�����дһ�ֽ�����
  * @param  addr: �Ĵ����ĵ�ַ
  * @param  dat: 	��д�������
  * @retval ��
  */
void HMC5883L_Write_Byte(unsigned char addr, unsigned char dat)
{
	Single_WriteIIC(HMC5883L_IIC_ADDR, addr, dat);
}
/**
  * @brief  ʹ��IIC���ߴ�HMC5883L�ļĴ����ж�һ�ֽ�����
  * @param  addr: �Ĵ����ĵ�ַ
  * @retval ������һ�ֽ�����
  */
unsigned char HMC5883L_Read_Byte(unsigned char addr)
{
	unsigned char temp;
	temp = Single_ReadIIC(HMC5883L_IIC_ADDR, addr);
	return temp;
}
/**
  * @brief  �����HMC5883L�������Ƿ�����
  * @param  ��
  * @retval 1���ɹ�����0��ʧ�ܣ�
  */
unsigned char CheckHMC5883LConnection(void)
{
	if(HMC5883L_Read_Byte(0x0a) != 0x48)
		return 0;
	else
		return 1;
}
/**
	* @brief 	��HMC5883L�ж�ȡ��������ԭʼ����
	* @param  buf��ԭʼ���������ָ�룬int*��
	* @retval ��
	*/
void Read_HMC5883L(int *buf)
{
	unsigned char tp[6];
	I2C_Read_MultiBytes(HMC5883L_IIC_ADDR, 0x03, 6, tp);
	buf[0] = tp[0] << 8 | tp[1]; //Combine MSB and LSB of X Data output register  �����Чλ
  buf[1] = tp[4] << 8 | tp[5]; //Combine MSB and LSB of Y Data output register
  buf[2] = tp[2] << 8 | tp[3]; //Combine MSB and LSB of Z Data output register 
	buf[0] -= mag_x_offset;
	buf[1] -= mag_y_offset;
	buf[2] -= mag_z_offset;
	buf[1] *= mag_y_scale;
	buf[2] *= mag_z_scale;
}
/**
	* @brief 	����HMC5883Lԭʼ���ݵ�У׼����
	* @param  x_offset, x_offset, x_offset����Ӧ���ƫ��
	*					y_scale��y���x��ı���У׼
	*					z_scale��z���x��ı���У׼
	* @retval ��
	*/
void HMC5883L_Set_Calibration_Value(int x_offset, int y_offset, int z_offset, float y_scale, float z_scale)
{
	mag_x_offset = x_offset;
	mag_y_offset = y_offset;
	mag_z_offset = z_offset;
	mag_y_scale = y_scale;
	mag_z_scale = z_scale;
}
/**
	* @brief 	����HMC5883L�Ĺ���ģʽ
	* @param  mode��0	��������ģʽ
	*								1	���β���ģʽ
	*								2	����ģʽ
	* @retval ��
	*/
void HMC5883L_Set_Mode(unsigned char mode)
{
	MD = mode;
	HMC5883L_Write_Byte(0x02, MD);
}
/**
	* @brief 	HMC5883L��ʼ������
	* @param  ��
	* @retval 0��ʧ��
	*					1���ɹ�
	*/
unsigned char HMC5883L_Init(void)
{
	unsigned char time_out = 200;
	while(CheckHMC5883LConnection() == 0)
	{
		Delay1ms(1);
		if(--time_out == 0)
			return 0;
	}
	HMC5883L_Write_Byte(0x00, (MA << 4) | (DO  << 2) | MS);
	HMC5883L_Write_Byte(0x01, (GN << 5));
	HMC5883L_Write_Byte(0x02, MD);
	//HMC5883L_Set_Calibration_Value(-83, 10, -74, 0.99041592, 1.16104081);
	return 1;
}
/**
	* @brief 	����x���y�����ǲ�����ƫ��
	* @param  mag_data�������������
	*					ax��ay��x���y������
	* @retval Yaw��������Ĵ�ƫ��
	*/
float HMC5883L_Get_AngleXY(int *mag_data, float ax, float ay)
{
	float Yaw, RadX, RadY, Xh, Yh;
	RadX = -ax / 57.2957795f;
	RadY = -ay / 57.2957795f;
	Xh = mag_data[0] * cos(RadX) + mag_data[1] * sin(RadY) * sin(RadX) - mag_data[2] * cos(RadY) * sin(RadX);
	Yh = mag_data[1] * cos(RadY) + mag_data[2] * sin(RadY);
	Yaw = atan2(Yh, Xh) * 180 / 3.14159 + 180;
	return Yaw;
}
