#ifndef  _POWERMANAGE_H
#define  _POWERMANAGE_H

#define BAT_CH 15
 
sbit  PROG = P1^7;     //TP4054�ĳ��ʹ������
sbit  EN_3V3 = P4^0;    //LDO TPS78233��ʹ������


void ADCInit(void);
int ADCRead(unsigned char adc_ch);
void Enable3V3Output(unsigned char k);
void BatteryChargeEnable(unsigned char c);
float GetBatteryVoltage(void);
float GetBatteryLife(void);
void SystemPowerDown(void);
void SyetemDeepPowerDown(void);
void SystemPowerOn(void);

#endif   



