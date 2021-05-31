#include "stc8a.h"
#include "EEPROM.h"
#include "intrins.h"

//===============================================
// 函数：void DisableEEPROM(void)
// 描述：禁止访问ISP/IAP
// 参数：non
// 返回：non
// 版本：V1.0 2012-10-22
//===============================================
void DisableEEPROM(void)
{
	IAP_CONTR = 0;        //禁止ISP/IAP操作
	IAP_CMD   = 0; 				//去除ISP/IAP命令
	IAP_TRIG  = 0;				//防止ISP/IAP命令误触发
	IAP_ADDRH = 0xff;     //清0地址高字节
	IAP_ADDRL = 0xff; 		//清0地址低字节，指向非EEPROM区，防止误操作
}
//===============================================
// 函数：void EEPROM_read_n(u16 EE_address,u8 *DataAddress,u16 number)
// 描述：从指定EEPROM首地址读出n个字节放在指定的缓冲区
// 参数：EE_adress
//       DataAddress
//  		 number
// 返回：non
// 版本：V1.0 2012-10-22
//===============================================
void EEPROM_read_n(unsigned int EE_address,unsigned char *DataAddress,unsigned int number )
{
	F0 = EA;
	EA = 0 ;    //禁止中断
	IAP_CONTR = ENABLE_IAP;
	IAP_CMD = CMD_READ;
	do
	{
		IAP_ADDRH = EE_address /256;    //送地址高字节（地址需要改变时才需要重新送地址）
		IAP_ADDRL = EE_address %256; 		//送地址低字节
		IAP_TRIG  = 0x5a; IAP_TRIG = 0xa5;
		_nop_();
		*DataAddress = IAP_DATA; 				//读出数据送出
		EE_address++;
		DataAddress++;
	}while (--number);
	
	DisableEEPROM();
	EA = F0;                					//重新允许中断
}
/*******************扇区擦除函数****************/
//===============================================
// 函数：void EEPROM_SectorErase(u16 EE_Address)
// 描述：把指定地址的EEPROM扇区擦除
// 参数：EE_address :要擦除的扇区EEPROM的地址
// 返回：non
// 版本：V1.0 2012-10-22
//===============================================
void EEPROM_SectorErase(unsigned int EE_address)
{
	F0 = EA;
	IAP_ADDRH = EE_address / 256;
	IAP_ADDRL = EE_address % 256;
	IAP_CONTR = ENABLE_IAP;
	IAP_CMD = CMD_ERASE;
	IAP_TRIG = 0x5a; IAP_TRIG = 0xa5;
	_nop_();
	DisableEEPROM();
	EA = P0;                 //重新允许中断
}
//===============================================
// 函数：void EEPROM_write_n(u16 EE_address,u8 *DataAddress,u16 number)
// 描述：把缓冲区的n个字节写入指定首地址的EEPROM
// 参数：EE_address :写入EEPROM的首地址
//       DataAdress :写入源数据的缓冲区的首地址
//       number 		:写入的字节长度
// 返回：non
// 版本：V1.0 2012-10-22
//===============================================
void EEPROM_write_n(unsigned int EE_address,unsigned char *DataAddress,unsigned int number)
{
	F0 = EA;
	EA = 0 ;       	//禁止中断
	IAP_CONTR = ENABLE_IAP;
	IAP_CMD   = CMD_PROGRAM;
	do
	{
	IAP_ADDRH = EE_address / 256;
	IAP_ADDRL = EE_address % 256;	
	IAP_DATA = *DataAddress;      //送数据到IAP_DATA,只有数据改变时才需要送
	IAP_TRIG = 0x5a; IAP_TRIG = 0xa5;
	_nop_();
	EE_address++;
	DataAddress++;
	}while(--number);
	DisableEEPROM();
	EA = F0 ;        //重新允许中断
}






















