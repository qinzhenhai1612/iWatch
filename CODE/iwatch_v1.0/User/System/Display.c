#include "sys.h"
#include "oled.h"
#include "font.h"
#include "Display.h"
#include "math.h"
#include "stdio.h"
#include "string.h"


unsigned char xdata main_cache[1024] = {0};	//���Դ�
unsigned char xdata sub_cache1[1024] = {0};	//���Դ�1����������ʱ�ӽ���
unsigned char xdata sub_cache2[1024] = {0};	//���Դ�2

/**  
	* @brief 	����sys_config�ṹ������ݳ�ʼ����ʾ
	* @param  config��sys_config�ͽṹ���ָ��
	* @retval ��
	*/
void DisplayInit(struct sys_config *config)
{
	OLED_Init();																		//��ʼ��OLED
	ScreenSetBrightness(config->screen_brightness);	//�趨��Ļ����
	ScreenSetDirection(config->screen_direction);		//�趨��Ļ����
	ScreenSetInverse(config->screen_inverse);				//�趨��Ļ�Ƿ�ɫ
	ScreenOnOff(ON);																//��Ļ����ʾ
}
/**
	* @brief 	��BMPͼƬд��ָ���Դ��е�ָ��λ��
	* @param 	x�������꣬y�������꣬width��ͼƬ�Ŀ�ȣ�height��ͼƬ�ĸ߶ȣ�
	*					buf1��BMPͼ��ָ�룬buf2���Դ��ָ��
	*					k��ѡ�񸲸����Դ��ϻ����ص����Դ���
	*						COVER	����
	*						BLEND	����
	* @retval ��
	*/
void BMPToCache(unsigned char x, unsigned char y, unsigned char width, unsigned char height, unsigned char *buf1, unsigned char *buf2, unsigned char k)
{
	unsigned char data i, j;
	unsigned int data num1, num2, num3;
	num1 = height / 8;
	if(k == 0)				
	{
		for(i = 0; i < num1; i++)
		{
			num2 = (i + y) * 128 + x;
			num3 = i * width;
			for(j = 0; j < width; j++)
				buf2[num2 + j] = buf1[num3 + j];
		}
	}
	else
	{
		for(i = 0; i < num1; i++)
		{
			num2 = (i + y) * 128 + x;
			num3 = i * width;
			for(j = 0; j < width; j++)
				buf2[num2 + j] |= buf1[num3 + j];
		}
	}
	for(i = 0; i < num1; i++)
	{
		num2 = (i + y) * 128 + x;
		num3 = i * width;
		for(j = 0; j < width; j++)
			buf2[num2 + j] = buf1[num3 + j];
	}
}
/**
	* @brief 	��ָ���Դ�����
	* @param  buf���Դ��ָ��
	* @retval ��
	*/
void ClearCache(unsigned char *buf)
{
	unsigned int data i;
	for(i = 0; i < 1024; i++)
		buf[i] = 0x00;
}
/**
	* @brief 	��ָ���Դ��ָ����������
	* @param  x����ʼ�����꣬y����ʼ�����꣬width������Ŀ�ȣ�height������ĸ߶ȣ�
	*					buf1���Դ��ָ��
	* @retval ��
	*/
void ClearCacheArea(unsigned char x, unsigned char y, unsigned char width, unsigned char height, unsigned char *buf1)
{
	unsigned char data i, j;
	unsigned int data num1, num2;
	num1 = height / 8;
	for(i = 0; i < num1; i++)
	{
		num2 = (i + y) * 128 + x;
		for(j = 0; j < width; j++)
		{
			buf1[num2 + j] = 0x00;
		}
	}
}
/**
	* @brief 	�����Դ汣�浽���Դ�1��
	* @param  ��
	* @retval ��
	*/
void SaveScreen(void)
{
	unsigned int data n;
	for(n = 0; n < 1024; n++)
		sub_cache1[n] = main_cache[n];
}
/**
	* @brief 	��ָ���Դ�ˢ�µ�OLED��Ļ��
	* @param  cache_buf���Դ��ָ��
	* @retval ��
	*/
void ScreenRefreshAll(unsigned char *cache_buf)
{
	OLED_DrawBMP(cache_buf);
}
/**
	* @brief  �����Դ�ָ��λ�á�ָ����Ⱥ�ָ�����ȵ�����ˢ�µ�OLED��Ļ�ϣ��ֲ�ˢ�£�
	* @param  x����ʼ�����꣨0~127����y����ʼ�����꣨0~7��
	*					width��ˢ������Ŀ�ȣ�0~127����height��ˢ������ĸ߶ȣ�0~7��
	* @retval ��
	*/
void ScreenRefreshArea(unsigned char x, unsigned char y, unsigned char width, unsigned char height)
{
	unsigned char data i,j;
	unsigned int data n;	
	for(i = y; i < height; i++)  
	{
		OLED_Set_Pos(x, i);
		n = i * 128;
		for(j = 0; j < width; j++)
			OLED_WR_Byte(main_cache[n + j], OLED_DATA); 
	}
}
/**
	* @brief  ʵ�ֵ�ǰ��Ļ���ݻ����뿪������ʾ�����Դ�����ݽ��ӽ���������ʾ�Ķ�̬Ч��
	* @param  cache�����Դ��ָ�루�Դ�Ĵ�С��1024bytes��
	*					direction��������Ļ�ķ���
	*							UP		����
	*							DOWN	����
	*							LEFT	����
	*							RIGHT	����
	* @retval ��
	*/
void ScreenPushAnimation(unsigned char *cache, unsigned char direction)
{
	unsigned char data n, i;	//ѭ������
	unsigned int data j;			//ѭ������
	unsigned int data num1, num2;
	if(direction == UP)
	{
		for(n = 0; n < 32; n++)
		{
			for(i = 0;i < 7;i++)
			{
				num1 = i*128;
				num2 = num1 + 128;
				for(j = num1; j < num2; j++)
				{
					main_cache[j] = (main_cache[j + 128] << 6) | (main_cache[j] >> 2);
				}
			}
			num1 = (n / 4) * 128;
			num2 = 6 - (n % 4) * 2;
			for(i = 0;i < 128; i++)
			{
				main_cache[896 + i] = (main_cache[896 + i] >> 2) | ((cache[num1 + i] << num2) & 0xc0);
			}
			OLED_DrawBMP(main_cache);		//�����Դ�д��OLED��ʾ
		}
	}
	else if(direction == DOWN)
	{
		for(n = 0; n < 32; n++)
		{
			for(i = 0; i < 7; i++)
			{
				num1 = (7 - i)*128;
				num2 = num1 + 128;
				for(j = num1; j < num2; j++)
					main_cache[j] = (main_cache[j] << 2) | (main_cache[j - 128] >> 6 );
			}
			num1 =(7 - (n/4))*128;
			num2 = 6 - (n%4)*2;
			for(i = 0;i < 128; i++)
				main_cache[i] = (main_cache[i] << 2) | ((cache[num1 + i] >> num2)&0x03);
			OLED_DrawBMP(main_cache);	//�����Դ�д��OLED��ʾ
		}
	}
	else if(direction == LEFT)
	{
		for(n = 0; n < 64; n++)
		{
			for(i = 0; i < 8; i++)
			{
				num1 = i * 128;
				num2 = num1 + 126;
				for(j = num1; j < num2; j++)
					main_cache[j] = main_cache[j + 2];
				main_cache[num2] = cache[num1 + n * 2];
				main_cache[num2 + 1] = cache[num1 + 1+ n * 2];
			}
			OLED_DrawBMP(main_cache);		//�����Դ�д��OLED��ʾ
		}
	}
	else if(direction == RIGHT)
	{
		for(n = 0; n < 64; n++)
		{
			for(i = 0; i < 8; i++)
			{
				num1 = i * 128;
				num2 = num1 + 127;
				for(j = num2; j > num1; j--)
					main_cache[j] = main_cache[j - 2];
				main_cache[num1 + 1] = cache[num1 + 127 - n * 2];
				main_cache[num1] = cache[num1 + 126 - n * 2];
			}
			OLED_DrawBMP(main_cache);		//�����Դ�д��OLED��ʾ
		}
	}
}/**
	* @brief  ʵ�ֵ�ǰ��Ļ�������»����뿪������ʾ�����Դ�����ݽ��ӽ���������ʾ�Ķ�̬Ч��
	* @param  cache�����Դ��ָ��
	*					num�����»���������
	* @retval ��
	*/
void ScreeRollDown(unsigned char *buf2, unsigned char num)
{
	unsigned char data n, i;
	unsigned int data j, num1, num2;
	for(n = 0; n < num; n++)
	{
		for(i = 0; i < 7; i++)
		{
			num1 = (7 - i)*128;
			num2 = num1 + 128;
			for(j = num1; j < num2; j++)
				main_cache[j] = (main_cache[j] << 1) | (main_cache[j - 128] >> 7);
		}
		num1 =(7 - (n / 8)) * 128;
		num2 = 7 - (n % 8);
		for(i = 0;i < 128; i++)
			main_cache[i] = (main_cache[i] << 1) | ((buf2[num1 + i] >> num2)&0x01);
		OLED_DrawBMP(main_cache);
	}
}
/**
	* @brief 	�����͹ر���Ļ��ʾ
	* @param  k��ON		����
	*						 OFF	�ر�
	* @retval ��
	*/
void ScreenOnOff(unsigned char k)
{
	if(k)
		OLED_Display_On();
	else
		OLED_Display_Off();
}

/**
	* @brief 	������Ļ������
	* @param  screen_brightness����Ļ������ֵ��0~255��
	* @retval ��
	*/
void ScreenSetBrightness(unsigned char screen_brightness)
{
	if(screen_brightness > 255)	//�޷�
		screen_brightness = 255;
	OLED_Set_Brightness(screen_brightness);
}
/**
	* @brief 	������Ļ����
	* @param  direction��
	*						NORMAL			��������
	*						UPSIDEDOWN	�������ҵߵ�
	* @retval ��
	*/
void ScreenSetDirection(unsigned char k)
{
	if(k)
	{
		OLED_Horizental_Reverse(1);
		OLED_Vertical_Reverse(1);
	}
	else
	{
		OLED_Horizental_Reverse(0);
		OLED_Vertical_Reverse(0);
	}
}

/**
	* @brief 	������Ļ�Ƿ�ɫ
	* @param  inverse��
	*						NO_INVERSED	����ɫ���ڵװ���
	*						INVERSED		��ɫ���׵׺���
	* @retval ��
	*/
void ScreenSetInverse(unsigned char k)
{
	OLED_Inverse(k);
}
/**
	* @brief 	�����Դ�����ʾʱ�䣬��ʵ��ʱ��䶯ʱ�Ķ�̬Ч��
	*					����δ���������ϴε��øú����Ĳ�����ͬʱ����ͬ����λ���ֻᶯ̬�л�
	*					������ͬ��λ���ֲ��κδ���
	* @param  hour��Сʱ��min�����ӣ�sec������
	* @retval ��
	*/
void DisplayTime(unsigned char hour, unsigned char min, unsigned char sec)
{
	char n; 
	unsigned char i, j, num3;
	unsigned int k, num1, num2;
	unsigned int temp;
	unsigned char buf[8];
	static unsigned char last_buf[8] = {11,11,11,11,11,11,11,11};
	buf[0] = hour / 10;
	buf[1] = hour % 10;
	buf[2] = 10;
	buf[3] = min / 10;
	buf[4] = min % 10;
	buf[5] = 10;
	buf[6] = sec / 10;
	buf[7] = sec % 10;
	for(n = 7; n  >= 0; n--)
	{
		if(buf[n] != last_buf[n])
		{
			//last_buf[n] = buf[n];
			for(i = 0; i < 48; i++)
			{
				for(j = 0; j < 5; j ++)
				{
					num1 = (j + 1) * 128 + n * 16;
					num2 = num1 + 16;
					for(k = num1; k < num2; k++)
					{
						temp = (main_cache[k + 128] << 8) | main_cache[k];
						main_cache[k] = (temp >> 1);
					}
				}
				num1 = (j + 1) * 128 + n * 16;
				num2 = (i / 8) * 16;
				num3 = 7 - i % 8;
				for(k = 0; k < 16; k++)	
				{
					main_cache[k + num1] =  (main_cache[k + num1] >> 1) | ((F16X48[buf[n]][num2 + k] << num3) & 0x80);
				}
				OLED_DrawBMP(main_cache);
			}
		}
		last_buf[n] = buf[n];
	}
}
/**
	* @brief 	��ָ���Դ��ָ��λ������ʾ�ַ��������ܹ��Զ�����
	*					���ﵽ��Ļ�ײ�ʱ���������Ϲ����Լ�����ʾ
	* @param  x���ַ�����ʼλ�õĺ����꣨0~127��
	*					y���ַ�����ʼλ�õ������꣨0~7��
	*					str������ʾ���ַ�����ָ��
	*					cache_buf���Դ��ָ��
	*					front_size��ѡ�����壬	FONT8X16		�������
	*																	FONT6X8			С������
	*					inverse��		�Ƿ�ɫ��	NO_INVERSED	����ɫ
	*														 			INVERSED		��ɫ
	*					do_refresh����ˢ����Ļ��0						��ˢ��
	*																	1						ˢ��
	* @retval ��ǰ�ַ�����ռ������
	*/
unsigned char ShowString(unsigned char x, unsigned char y, unsigned char *str, unsigned char *cache_buf, unsigned char front_size, unsigned char inverse, unsigned char do_refresh)
{
	unsigned char data temp = 0x00;
	unsigned char data str_len = 0;
	unsigned char data n,i,j;
	unsigned char data line;
	unsigned int data num1, num2;
	if(inverse == INVERSED)
		temp = 0xff;
	while(str[str_len] != '\0')
	{
		str_len ++;
	}
	if(front_size == FONT8X16)
	{
		for(n = 0; n < str_len; n++)
		{
			line = y + (n / 16) * 2;
			if(line > 6)
			{
				for(i = 0; i < 2; i++)
				{
					num1 = i * 128 + x + (n % 16) * 8;
					num2 = i * 8;
					for(j = 0; j < 8; j++)
						sub_cache2[num1 + j] = temp ^ F8X16[str[n] - ' '][num2 + j];
				}
				if(n % 16 == 15 || n == str_len - 1)
				{
					ScreeRollUp(sub_cache2, 16);
					ClearCache(sub_cache2);
				}
			}
			else
			{
				for(i = 0; i < 2; i++)
				{
					num1 = (line + i) * 128 + x + (n % 16) * 8;
					num2 = i * 8;
					for(j = 0; j < 8; j++)
						cache_buf[num1 + j] = temp ^ F8X16[str[n] - ' '][num2 + j];
				}
			}
		}
	}
	else
	{
		for(n = 0; n <str_len; n++)
		{
			line = y + (n / 21);
			if(line > 7)
			{
				num1 = x + (n % 21) * 6;
				for(j = 0; j < 6; j++)
					sub_cache2[num1 + j] = temp ^ F6X8[str[n] - ' '][j];
				if(n % 21 == 20 || n == str_len - 1)
				{
					ScreeRollUp(sub_cache2, 8);
					ClearCache(sub_cache2);
				}
			}
			else
			{
				num1 = line * 128 + x + (n % 21) * 6;
				for(j = 0; j < 6; j++)
					cache_buf[num1 + j] = temp ^ F6X8[str[n] - ' '][j];
			}
		}
	}
	if(do_refresh)
		ScreenRefreshArea(0, 0, 128, 8);
	if(front_size == FONT8X16)
		return (str_len / 16 + 1) * 2;
	else
		return (str_len / 21 + 1);
}
void DrawDot(unsigned char x, unsigned char y)
{
	main_cache[(y / 8) * 128 + x] |= (0x01 << (y % 8));
}
void DrawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
	float k;
	float b;
	if(x1 != x2)
	{
		k = ((float)y2 - (float)y1) / ((float)x2 - (float)x1);
		b = y1 - k * x1;
		if(k < 1 && k > -1)
		{
			if(x1 <= x2)
			{
				for(x1; x1 <= x2; x1++)
					DrawDot(x1, k * x1 + b);
			}
			else
			{
				for(x2; x2 <= x1; x2++)
					DrawDot(x2, k * x2 + b);
			}
		}
		else
		{
			if(y1 <= y2)
			{
				for(y1; y1 <= y2; y1++)
					DrawDot((y1 - b) / k, y1);
			}
			else
			{
				for(y2; y2 <= y1; y2++)
					DrawDot((y2 - b) / k, y2);
			}
		}
	}
	else
	{
		if(y1 <= y2)
		{
			for(y1; y1 <= y2; y1++)
				DrawDot(x1, y1);
		}
		else
		{
			for(y2; y2 <= y1; y2++)
				DrawDot(x2, y2);
		}
	}
}
/*
void DrawCircle(unsigned char x, unsigned char y, unsigned char radius)
{
	float x0, y0, k, rad;
	for(rad = 0; rad <= 6.28; rad += 0.02)
	{
		k = tan(rad);
		if((rad >= 0) && (rad < 1.57))
			x0 = (float)radius / sqrt(k * k + 1);
		else if(rad < 3.14)
			x0 = - (float)radius / sqrt(k * k + 1);
		else if(rad < 4.71)
			x0 = - (float)radius / sqrt(k * k + 1);
		else
			x0 = (float)radius / sqrt(k * k + 1);
		y0 = x0 * k;
		DrawDot((x + x0), (y + y0));
	}
}
*/
void DrawArm(unsigned char x, unsigned char y, unsigned char radius, int angle)
{
	float k, rad, x0, y0;
	rad = angle * 0.0174;
	k = tan(rad);
	if((rad >= 0) && (rad < 1.57))
		x0 = (float)radius / sqrt(k * k + 1);
	else if(rad < 3.14)
		x0 = - (float)radius / sqrt(k * k + 1);
	else if(rad < 4.71)
		x0 = - (float)radius / sqrt(k * k + 1);
	else
		x0 = (float)radius / sqrt(k * k + 1);
	y0 = x0 * k;
	DrawLine(x, y, x+x0, y+y0);
}
void DrawSelectionFrame(unsigned char x, unsigned char y)
{
	DrawDot(x, y);
	DrawDot(x + 1, y);
	DrawDot(x + 2, y);
	DrawDot(x + 3, y);
	DrawDot(x + 4, y);
	DrawDot(x + 1, y + 1);
	DrawDot(x + 2, y + 1);
	DrawDot(x + 3, y + 1);
	DrawDot(x + 4, y + 1);
	DrawDot(x, y + 1);
	DrawDot(x, y + 2);
	DrawDot(x, y + 3);
	DrawDot(x, y + 4);
	DrawDot(x + 1, y + 1);
	DrawDot(x + 1, y + 2);
	DrawDot(x + 1, y + 3);
	DrawDot(x + 1, y + 4);
					
	x += 31;
	DrawDot(x, y);
	DrawDot(x - 1, y);
	DrawDot(x - 2, y);
	DrawDot(x - 3, y);
	DrawDot(x - 4, y);
	DrawDot(x - 1, y + 1);
	DrawDot(x - 2, y + 1);
	DrawDot(x - 3, y + 1);
	DrawDot(x - 4, y + 1);
	DrawDot(x, y + 1);
	DrawDot(x, y + 2);
	DrawDot(x, y + 3);
	DrawDot(x, y + 4);
	DrawDot(x - 1, y + 1);
	DrawDot(x - 1, y + 2);
	DrawDot(x - 1, y + 3);
	DrawDot(x - 1, y + 4);
						
	y += 31;
	DrawDot(x, y);
	DrawDot(x - 1, y);
	DrawDot(x - 2, y);
	DrawDot(x - 3, y);
	DrawDot(x - 4, y);
	DrawDot(x - 1, y - 1);
	DrawDot(x - 2, y - 1);
	DrawDot(x - 3, y - 1);
	DrawDot(x - 4, y - 1);
	DrawDot(x, y - 1);
	DrawDot(x, y - 2);
	DrawDot(x, y - 3);
	DrawDot(x, y - 4);
	DrawDot(x - 1, y - 1);
	DrawDot(x - 1, y - 2);
	DrawDot(x - 1, y - 3);
	DrawDot(x - 1, y - 4);
						
	x -= 31;
	DrawDot(x, y);
	DrawDot(x + 1, y);
	DrawDot(x + 2, y);
	DrawDot(x + 3, y);
	DrawDot(x + 4, y);
	DrawDot(x + 1, y - 1);
	DrawDot(x + 2, y - 1);
	DrawDot(x + 3, y - 1);
	DrawDot(x + 4, y - 1);
	DrawDot(x, y - 1);
	DrawDot(x, y - 2);
	DrawDot(x, y - 3);
	DrawDot(x, y - 4);
	DrawDot(x + 1, y - 1);
	DrawDot(x + 1, y - 2);
	DrawDot(x + 1, y - 3);
	DrawDot(x + 1, y - 4);
}










