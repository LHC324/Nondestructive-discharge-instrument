/*
 * Dwin.h
 *
 *  Created on: Jan 3, 2023
 *      Author: LHC
 */

#ifndef INC_DWIN_H_
#define INC_DWIN_H_

#include "config.h"

#if defined(USING_SIMULATE)
#define DWIN_USING_DEBUG 0
#else
#define DWIN_USING_DEBUG 1
#endif

#define DWIN_SEE_RX_BUFF 1
#if defined(USING_SIMULATE)
#define DWIN_DEBUG_UART NULL
#define DWIN_DEBUG
#else
#define DWIN_DEBUG_UART &Uart1
#define DWIN_DEBUG Uartx_Printf
#endif

#if (1 == DWIN_USING_RB)
// #include "ringbuffer.h"
#include "utils_ringbuffer.h"
#endif

#define DWIN_RX_BUF_SIZE 128
#define DWIN_TX_BUF_SIZE 128

#define WRITE_CMD 0x82 // 写
#define READ_CMD 0x83  // 读

#define PAGE_CHANGE_CMD 0x84 // 页面切换
#define TOUCH_CMD 0xD4		 // 触摸动作

#define V_BATTERY_ADDR 0x1000	 // 电池电压地址
#define V_DISCHARGER_ADDR 0x1002 // 放电电压地址
#define I_DISCHARGER_ADDR 0x1004 // 放电电流地址
#define ANIMATION_ADDR 0x1018	 // 放电动画

#define SLAVE_ID_ADDR 0x1012		// 从站地址
#define TARGET_DISTIMES_ADDR 0x1013 // 目标放电时长地址
#define V_DISCUTOFF_ADDR 0x1014		// 放电截止电压地址
#define V_DISREBOOT_ADDR 0x1015		// 二次放电起放电压
#define I_MAX_ADDR 0x1016			// 极限放电电流
#define P_MAX_ADDR 0x1017			// 极限放电功率
#define DIS_MODE_ADDR 0x1018		// 放电模式
// #define MODE1_ADDR 0x1018			// 按电流
// #define MODE2_ADDR 0x1019			// 按功率

#define USER_NAME_ADDR 0x1020	 // 用户名
#define USER_CODE_ADDR 0x1021	 // 用户密码
#define LOGIN_SURE_ADDR 0x1022	 // 登录确认地址
#define LOGIN_CANCEL_ADDR 0x1023 // 登录取消地
#define INPUT_ERROR_ADDR 0x1024	 // 输入错误地址
#define PARAM_SAVE_ADDR 0x1030	 // 参数保存确认地址

#define RSURE_CODE 0x00F1	// 恢复出厂设置确认键值
#define RCANCEL_CODE 0x00F0 // 注销键值

typedef enum
{
	err_frame_head = 0,	 /*数据帧头错误*/
	err_check_code,		 /*校验码错误*/
	err_event,			 /*事件错误*/
	err_max_upper_limit, /*超出数据上限*/
	err_min_lower_limit, /*低于数据下限*/
	err_data_len,		 /*数据长度错误*/
	err_data_type,		 /*数据类型错误*/
	err_other,			 /*其他错误*/
	dwin_ok,
} dwin_result;

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
#if (1 == DWIN_USING_RB)
		struct ringbuffer *rb;
#else
		uint8_t *pRbuf;
		uint16_t RxCount;
#endif
		uint8_t *pdat;
		/*预留外部数据结构接口*/
		void *pHandle;
		DwinMap *pMap;
		uint16_t Events_Size;
	} Slave;
	void *Uart;
};

/*带上(pd)解决多级指针解引用问题：(*pd)、(**pd)*/
// #define dwin_tx_size(pd) ((pd)->Uart.tx.size)
// #define dwin_rx_size(pd) ((pd)->Uart.rx.size)
#define dwin_tx_count(pd) ((pd)->Master.TxCount)
#define dwin_rx_count(pd) ((pd)->Slave.RxCount)
#define dwin_tx_buf ((pd)->Uart.TxCount.pTbuf)
#define dwin_rx_buf ((pd)->Slave.pRbuf)

extern DwinHandle Dwin_Object;
extern void Dwin_Save(pDwinHandle pd);
extern void Dwin_Send(pDwinHandle pd);
extern void Dwin_Write(pDwinHandle pd, uint16_t addr, uint8_t *dat, uint16_t len);
extern void Dwin_Read(pDwinHandle pd, uint16_t addr, uint8_t words);
extern void Dwin_PageChange(pDwinHandle pd, uint16_t page);
extern void Dwin_Poll(pDwinHandle pd);
extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);
// extern void Dwin_Put_Char_To_Rb(void);

#endif /* INC_DWIN_H_ */
