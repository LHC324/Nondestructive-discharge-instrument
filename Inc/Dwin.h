/*
 * Dwin.h
 *
 *  Created on: Nov 19, 2020
 *      Author: play
 */

#ifndef INC_DWIN_H_
#define INC_DWIN_H_

#include "config.h"

#define RX_BUF_SIZE 128
#define TX_BUF_SIZE 128

#define WRITE_CMD 0x82 //写
#define READ_CMD 0x83  //读

#define PAGE_CHANGE_CMD 0x84 //页面切换
#define TOUCH_CMD 0xD4		 //触摸动作

#define V_BATTERY_ADDR 0x1000	 //电池电压地址
#define V_DISCHARGER_ADDR 0x1002 //放电电压地址
#define I_DISCHARGER_ADDR 0x1004 //放电电流地址
#define ANIMATION_ADDR 0x1018	 //放电动画

#define SLAVE_ID_ADDR 0x1011		//从站地址
#define TARGET_DISTIMES_ADDR 0x1012 //目标放电时长地址
#define V_DISCUTOFF_ADDR 0x1013		//放电截止电压地址
#define V_DISREBOOT_ADDR 0x1014		//二次放电起放电压
#define I_MAX_ADDR 0x1015			//极限放电电流
#define P_MAX_ADDR 0x1016			//极限放电功率
#define MODE1_ADDR 0x1017			//按电流
#define MODE2_ADDR 0x1018			//按功率

#define USER_NAME_ADDR 0x1020	 //用户名
#define USER_CODE_ADDR 0x1021	 //用户密码
#define LOGIN_SURE_ADDR 0x1022	 //登录确认地址
#define LOGIN_CANCEL_ADDR 0x1023 //登录取消地
#define INPUT_ERROR_ADDR 0x1024	 //输入错误地址
#define PARAM_SAVE_ADDR 0x1030	 //参数保存确认地址

#define RSURE_CODE 0x00F1	//恢复出厂设置确认键值
#define RCANCEL_CODE 0x00F0 //注销键值

typedef struct
{
	uint8_t RxBuf[RX_BUF_SIZE];
	uint16_t RxCount;

	uint8_t TxBuf[TX_BUF_SIZE];
	uint16_t TxCount;
} Dwin_T;

typedef struct Dwin_HandleTypeDef *pDwinHandle;
typedef struct Dwin_HandleTypeDef DwinHandle;
typedef void (*pDfunc)(pDwinHandle, uint8_t);
typedef struct
{
	uint32_t addr;
	uint16_t upper;
	uint16_t lower;
	pDfunc event;
} DwinMap;

struct Dwin_HandleTypeDef
{
	uint16_t Save_Addr;
	struct
	{
		uint8_t *pTbuf;
		// uint16_t TxSize;
		uint16_t TxCount;
	} Master;
	struct
	{
		uint8_t *pRbuf;
		// uint16_t RxSize;
		uint16_t RxCount;
		/*预留外部数据结构接口*/
		void *pHandle;
		DwinMap *pMap;
		uint16_t Events_Size;
	} Slave;
	void *Uart;
};

// extern Dwin_T g_Dwin;
// extern void Dwin_SendWithCRC(uint8_t *_pBuf, uint16_t _ucLen);
// extern void Dwin_Send(uint8_t *_pBuf, uint16_t _ucLen);
// extern void Dwin_Write(uint16_t start_addr, uint8_t *dat, uint16_t length);
// extern void Dwin_Read(uint16_t start_addr, uint16_t words);

extern DwinHandle Dwin_Object;
extern void Dwin_Save(pDwinHandle pd);
extern void Dwin_Send(pDwinHandle pd);
extern void Dwin_Write(pDwinHandle pd, uint16_t addr, uint8_t *dat, uint16_t len);
extern void Dwin_Read(pDwinHandle pd, uint16_t addr, uint8_t words);
extern void Dwin_PageChange(pDwinHandle pd, uint16_t page);
extern void Dwin_Poll(pDwinHandle pd);
extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);

#endif /* INC_DWIN_H_ */
