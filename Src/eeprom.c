#include "eeprom.h"
// #include "systemtimer.h"

/*********************************************************
 * 函数名：IapConfigWaitTime()
 * 功能：  配置STC8单片机EEPROM的等待时间
 * 参数：
 * 作者:
 * note：
 *
 **********************************************************/
static void IapConfigWaitTime()
{
	uint32_t XTAL = FOSC / 1000000;

	if (XTAL >= 30) //>=30M
		IAP_CONTR = 0x80;
	else if (XTAL >= 24)
		IAP_CONTR = 0x81;
	else if (XTAL >= 20)
		IAP_CONTR = 0x82;
	else if (XTAL >= 12)
		IAP_CONTR = 0x83;
	else if (XTAL >= 6)
		IAP_CONTR = 0x84;
	else if (XTAL >= 3)
		IAP_CONTR = 0x85;
	else if (XTAL >= 2)
		IAP_CONTR = 0x86;
	else
		IAP_CONTR = 0x87;
}

/*********************************************************
 * 函数名：void IapIdle()
 * 功能：
 * 参数：
 * 作者:
 * note：
 *
 **********************************************************/
void IapIdle()
{
	IAP_CONTR = 0x00; //关闭IAP功能
	IAP_CMD = 0x00;	  //清除命令寄存器
	IAP_TRIG = 0x00;  //清除触发寄存器
	IAP_ADDRH = 0x80; //将地址设置到非IAP区域
	IAP_ADDRL = 0x00;
}

/*********************************************************
 * 函数名：char IapRead(unsigned short addr)
 * 功能：  读函数  读出一个字节
 * 参数：
 * 作者：
 * note：
 *
 **********************************************************/
char IapRead(char addr)
{
	char dat = '\0';

	IapConfigWaitTime();
	IAP_CMD = 0x01; // EEPROM读命令
	IAP_ADDRL = addr & 0xFF;
	IAP_ADDRH = addr >> 8;
	IAP_TRIG = 0x5A; //触发指令
	IAP_TRIG = 0xA5;
	dat = IAP_DATA;

	if (IAP_CONTR & 0x10)
	{
		IAP_CONTR &= 0xEF;
		return 0x5AA5;
	}
	else
		return dat;
}

/*********************************************************
 * 函数名：void IapProgram(unsigned short addr, char dat)
 * 功能：  指定地址写数据
 * 参数：
 * 作者：
 * note：
 *
 **********************************************************/
void IapProgram(uint16_t addr, uint8_t dat)
{
	//    IAP_CONTR = WT_12M;//WT_24M;                //使能IAP
	IapConfigWaitTime();
	IAP_CMD = 0x02;					//设置IAP写命令
	IAP_ADDRL = (uint8_t)addr;		//设置IAP低地址
	IAP_ADDRH = (uint8_t)addr >> 8; //设置IAP高地址
	IAP_DATA = dat;					//写IAP数据
	IAP_TRIG = 0x5a;				//写触发命令(0x5a)
	IAP_TRIG = 0xa5;				//写触发命令(0xa5)
	//_nop_();
	//	Delay_ms(5);//延时5毫秒
	IapIdle(); //关闭IAP功能
}

/*********************************************************
 * 函数名：void IapErase(int addr)
 * 功能：  擦除指定地址的内容
 * 参数：
 * 作者：
 * note：擦除每次按照512B进行，仅提供首地址即可
 *
 **********************************************************/
void IapErase(uint16_t addr)
{
	//    IAP_CONTR = WT_12M;//WT_24M;                         //使能IAP
	IapConfigWaitTime();
	IAP_CMD = 0x03;					//设置IAP擦除命令
	IAP_ADDRL = (uint8_t)addr;		//设置IAP低地址
	IAP_ADDRH = (uint8_t)addr >> 8; //设置IAP高地址
	IAP_TRIG = 0x5a;				//写触发命令(0x5a)
	IAP_TRIG = 0xa5;				//写触发命令(0xa5)
	//_nop_();                                    //
	IapIdle(); //关闭IAP功能
			   // Delay_ms(4); //延时5毫秒
}

/*********************************************************
 * 函数名：void EEPROM_writeString(unsigned short Address,unsigned char *Pdata,unsigned short length);
 * 功能：  指定地址 写入数据
 * 参数：
 * 作者：
 * note：写数据一般请按照一个扇区一个扇区的来写，不然在擦除数据的时候会造成数据丢失
 * 一个扇区的大小是 512字节
 *
 *
 *
 ***********************************************************************************/
void IapWrites(uint16_t addr, const uint8_t *str, uint16_t len)
{
	if (str)
	{
		IapErase(addr);
		while (len--)
		{
			IapProgram(addr, *str);
			if (IapRead(addr) != *str)
				break;
			addr++, str++;
		}
	}
}

void Iap_Reads(uint16_t addr, uint8_t *str, uint16_t len)
{
	if (str)
	{
		while (len--)
		{
			*str++ = IapRead(addr++);
		}
	}
}
