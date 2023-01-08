/*
 * Dwin.c
 *
 *  Created on: 2022年1月4日
 *      Author: play
 */

#include "dwin.h"
#include "usart.h"
#include "discharger.h"
#include "eeprom.h"

#if (!USING_DEBUG && DWIN_USING_DEBUG)
#error Global debugging mode is not turned on!
#endif

static uint8_t dtx_buf[DWIN_TX_BUF_SIZE];
static uint16_t user_name = 0x0000, user_code = 0x0000, error = 0x0000;

static void Dwin_SetDisMode(pDwinHandle pd, uint8_t Site);
// static void Dwin_SetCurrentFlag(pDwinHandle pd, uint8_t Site);
// static void Dwin_SetPowerFlag(pDwinHandle pd, uint8_t Site);
static void Dwin_SetSlaveId(pDwinHandle pd, uint8_t Site);
static void Dwin_SetDisTargetTimes(pDwinHandle pd, uint8_t Site);
static void Dwin_SetDisVcutoff(pDwinHandle pd, uint8_t Site);
static void Dwin_SetDisVreboot(pDwinHandle pd, uint8_t Site);
static void Dwin_SetDisILimit(pDwinHandle pd, uint8_t Site);
static void Dwin_SetDisPLimit(pDwinHandle pd, uint8_t Site);
static void Dwin_LoginSure(pDwinHandle pd, uint8_t Site);
static void Dwin_SaveSure(pDwinHandle pd, uint8_t Site);

/*迪文响应线程*/
static DwinMap Dwin_ObjMap[] = {
	{SLAVE_ID_ADDR, 32, 1, Dwin_SetSlaveId},
	{TARGET_DISTIMES_ADDR, 600, 120, Dwin_SetDisTargetTimes},
	{V_DISCUTOFF_ADDR, 600, 220, Dwin_SetDisVcutoff},
	{V_DISREBOOT_ADDR, 600, 220, Dwin_SetDisVreboot},
	{I_MAX_ADDR, 35, 1, Dwin_SetDisILimit},
	{P_MAX_ADDR, 950, 15, Dwin_SetDisPLimit},
	{DIS_MODE_ADDR, 1, 0, Dwin_SetDisMode},
	{USER_NAME_ADDR, 9999, 0, Dwin_LoginSure},
	{USER_CODE_ADDR, 9999, 0, Dwin_LoginSure},
	{LOGIN_SURE_ADDR, 0xFFFF, 0, Dwin_LoginSure},
	{LOGIN_CANCEL_ADDR, 0xFFFF, 0, Dwin_LoginSure},
	{PARAM_SAVE_ADDR, 0xFFFF, 0, Dwin_SaveSure},
};
struct ringbuffer dwin_rb = {
	NULL,
	0,
	0,
	0,
};

DwinHandle Dwin_Object = {
	DEFAULT_SYSTEM_ADDR,
	{dtx_buf, 0},
	{
#if (DWIN_USING_RB)
		&dwin_rb,
#else
		NULL,
		0,
#endif
		NULL,
		&discharger.Storage,
		Dwin_ObjMap,
		sizeof(Dwin_ObjMap) / sizeof(DwinMap),
	},
	&Uart4,
};

/*以下代码9级优化，速度优先*/
#pragma OPTIMIZE(9, speed)

#if (DWIN_USING_RB)
/**
 * @brief  迪文屏幕接收数据存入环形缓冲区
 * @param  dat 数据
 * @retval None
 */
// void Dwin_Put_Char_To_Rb(void) // using 3
// {
// 	uint8_t dat = S4BUF;
// 	// ringbuffer_put(Dwin_Object.Slave.rb, &dat, sizeof(dat));
// }
#endif

/**
 * @brief  对迪文屏幕发送数据帧
 * @param  pd 迪文屏幕对象
 * @retval None
 */
void Dwin_Send(pDwinHandle pd)
{
#if defined(USING_CRC)
	uint16_t crc = 0;
	if (pd && pd->Uart)
	{
		/*The first three bytes do not participate in verification*/
		crc = Get_Crc16(&pd->Master.pTbuf[3U], pd->Master.TxCount - 3U, 0xFFFF);
		pd->Master.pTbuf[pd->Master.TxCount++] = crc;
		pd->Master.pTbuf[pd->Master.TxCount++] = (uint8_t)(crc >> 8U);
	}
#endif
	Uartx_SendStr(pd->Uart, pd->Master.pTbuf, pd->Master.TxCount, UART_BYTE_SENDOVERTIME);
}

/**
 * @brief  写数据变量到指定地址并显示
 * @param  pd 迪文屏幕对象句柄
 * @param  addr 开始地址
 * @param  dat 指向数据的指针
 * @param  len 数据长度
 * @retval None
 */
void Dwin_Write(pDwinHandle pd, uint16_t addr, uint8_t *dat, uint16_t len)
{
#if defined(USING_CRC)
	uint8_t temp_data[] = {0x5A, 0xA5, 0, WRITE_CMD, 0, 0};
#else
	uint8_t temp_data[] = {0x5A, 0xA5, 0, WRITE_CMD, 0, 0};
#endif
	if (pd)
	{
#if defined(USING_CRC)
		temp_data[2] = len + 5U;
#else
		temp_data[2] = len + 3U;
#endif
		temp_data[4] = addr >> 8U, temp_data[5] = addr;
		pd->Master.TxCount = sizeof(temp_data);
		memcpy(pd->Master.pTbuf, temp_data, sizeof(temp_data));
		memcpy(&pd->Master.pTbuf[pd->Master.TxCount], dat, len);
		pd->Master.TxCount += len;

		Dwin_Send(pd);
	}
}

/**
 * @brief  读出指定地址指定长度数据
 * @param  pd 迪文屏幕对象句柄
 * @param  addr 开始地址
 * @param  words 地址数目
 * @retval None
 */
void Dwin_Read(pDwinHandle pd, uint16_t addr, uint8_t words)
{
#if defined(USING_CRC)
	uint8_t temp_data[] = {0x5A, 0xA5, 6U, WRITE_CMD, 0, 0, 0};
#else
	uint8_t temp_data[] = {0x5A, 0xA5, 0x04, WRITE_CMD, 0, 0, 0};
#endif

	if (pd)
	{
		temp_data[4] = addr >> 8U, temp_data[5] = addr, temp_data[6] = words;
		pd->Master.TxCount = sizeof(temp_data);
		memcpy(pd->Master.pTbuf, temp_data, sizeof(temp_data));

		Dwin_Send(pd);
	}
}

/**
 * @brief  迪文屏幕指定页面切换
 * @param  pd 迪文屏幕对象句柄
 * @param  page 目标页面
 * @retval None
 */
void Dwin_PageChange(pDwinHandle pd, uint16_t page)
{
#if (USING_CRC)
	uint8_t buf[] = {
		0x5A, 0xA5, 0x07 + 2U, WRITE_CMD, 0x00, 0x84, 0x5A, 0x01, 0, 0};
#else
	uint8_t buf[] = {
		0x5A, 0xA5, 0x07, WRITE_CMD, 0x00, 0x84, 0x5A, 0x01, 0, 0};
#endif
	if (pd)
	{
		buf[8] = page >> 8U, buf[9] = page;
		pd->Master.TxCount = 0U;
		memcpy(pd->Master.pTbuf, buf, sizeof(buf));
		pd->Master.TxCount += sizeof(buf);

		Dwin_Send(pd);
	}
}

/*83指令返回数据以一个字为基础*/
#define DW_WORD 1U
#define DW_DWORD 2U
#if (1 == DWIN_USING_RB)
#define dw_rx_ptr(__ptr) ((__ptr)->Slave.pdat)
#else
#define dw_rx_ptr(__ptr) ((__ptr)->Slave.pRbuf)
#endif

/*获取迪文屏幕数据*/
#define Get_Data(__buf, __s, __size)                  \
	((__size) < 2U ? ((uint16_t)(__buf[__s] << 8U) |  \
					  (__buf[__s + 1U]))              \
				   : ((uint32_t)(__buf[__s] << 24U) | \
					  (__buf[__s + 1U] << 16U) |      \
					  (__buf[__s + 2U] << 8U) |       \
					  (__buf[__s + 3U])))

/**
 * @brief  迪文屏幕接收帧检查
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
dwin_result Dwin_Recv_Frame_Check(pDwinHandle pd, uint16_t *paddr)
{
#define DWIN_MIN_FRAME_LEN 5U // 3个前导码+2个crc16

	uint16_t crc16 = 0;

	/*检查接收数据的尺寸*/
#if (!DWIN_USING_RB)
	if (NULL == pd || dwin_rx_count(pd) < DWIN_MIN_FRAME_LEN)
	{
#if (DWIN_USING_DEBUG)
		DWIN_DEBUG(DWIN_DEBUG_UART, "@error:Data length error,cur_len: %bd.\r\n",
				   dwin_rx_count(pd));
#endif
		return err_data_len;
	}
#else
	static uint8_t dwin_buf[32];
	uint8_t len;

	if (NULL == pd || NULL == pd->Slave.rb || NULL == pd->Slave.rb->buf)
		return err_other;
#endif

	if (NULL == paddr)
		return err_other;

/*检查帧头是否符合要求*/
#if (!DWIN_USING_RB)
	if ((dwin_rx_buf[0] != 0x5A) || (dwin_rx_buf[1] != 0xA5))
#else
	len = ringbuffer_gets(pd->Slave.rb, dwin_buf, 2U);
	if (!ringbuffer_num(pd->Slave.rb)) // 无数据，直接退出
		return err_other;

	if ((2U != len) || (dwin_buf[0] != 0x5A) || (dwin_buf[1] != 0xA5))
#endif
	{
#if (DWIN_USING_DEBUG)
		DWIN_DEBUG(DWIN_DEBUG_UART, "@error:Protocol frame header error.\r\n");
#endif
#if (DWIN_SEE_RX_BUFF)
#if (!DWIN_USING_RB)
		DWIN_DEBUG(DWIN_DEBUG_UART, DWIN_DEBUG_UART, "dwin_rx_buf[%d]:", dwin_rx_count(pd));
		for (uint8_t i = 0; i < dwin_rx_count(pd); i++)
		{
			DWIN_DEBUG(DWIN_DEBUG_UART, DWIN_DEBUG_UART, "%bX ", dwin_rx_buf[i]);
		}
#else
		DWIN_DEBUG(DWIN_DEBUG_UART, "dwin_rx_buf[%bd]:", sizeof(dwin_buf));
		for (len = 0; len < sizeof(dwin_buf); len++)
		{
			DWIN_DEBUG(DWIN_DEBUG_UART, "%bX ", dwin_buf[len]);
		}
		DWIN_DEBUG(DWIN_DEBUG_UART, "\r\n\r\n");
#endif
#endif
		return err_frame_head;
	}
	/*不响应82H指令回复*/
	// #if (!DWIN_USING_RB)
	// 	if (WRITE_CMD == dwin_rx_buf[3U]) // 不回应写数据的指令
	// 		return err_other;
	// #else
	// 	if (WRITE_CMD == dwin_buf[0]) // 不回应写数据的指令
	// 		return err_other;
	// #endif
	/*不响应82H指令回复 && 获取83H指令的数据地址*/
#if (!DWIN_USING_RB)
	if (WRITE_CMD == dwin_rx_buf[3])
		return err_other;

	*paddr = Get_Data(dw_rx_ptr(pd), 4U, DW_WORD);
	pd->Slave.pdat = &dwin_rx_buf[6U]; // 返回用户数据长度首地址
#else
	ringbuffer_gets(pd->Slave.rb, &len, 1U);
	if (0 == len)
		return err_data_len;

	len = ringbuffer_gets(pd->Slave.rb, dwin_buf, len);
	if (0 == len)
		return err_data_len;

	if (WRITE_CMD == dwin_buf[0]) // 不回应写数据的指令
		return err_other;

	*paddr = Get_Data(dwin_buf, 1U, DW_WORD);
	pd->Slave.pdat = &dwin_buf[3];				   // 返回用户数据长度首地址
#endif

	/*检查crc校验码*/
#if (!DWIN_USING_RB)
	crc16 = Get_Crc16(&dwin_rx_buf[3U], dwin_rx_count(pd) - 5U, 0xFFFF);
	crc16 = (crc16 >> 8U) | (crc16 << 8U);
	if (crc16 == Get_Data(dw_rx_ptr(pd), dwin_rx_count(pd) - 2U, DW_DWORD))
#else
	crc16 = Get_Crc16(dwin_buf, len - 2U, 0xFFFF); // 去掉2字节crc
	crc16 = (crc16 >> 8U) | (crc16 << 8U);
	if (crc16 != Get_Data(dwin_buf, len - 2U, DW_WORD))
#endif
	{
#if (DWIN_USING_DEBUG)
		DWIN_DEBUG(DWIN_DEBUG_UART, "@error:crc check code error.\r\n");
#if (!DWIN_USING_RB)
		DWIN_DEBUG(DWIN_DEBUG_UART, "dwin_rxcount = %d,crc16 = 0x%bX.\r\n", dwin_rx_count(pd),
				   crc16);
#else
		DWIN_DEBUG(DWIN_DEBUG_UART, "crc16 = 0x%bX.\r\n", crc16);
#endif
#endif
		return err_check_code;
	}

	return dwin_ok;
#undef DWIN_MIN_FRAME_LEN
}

/**
 * @brief  迪文屏幕接收数据解析
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
void Dwin_Poll(pDwinHandle pd)
{
	uint16_t addr = 0;
	uint8_t i = 0;

	if (Dwin_Recv_Frame_Check(pd, &addr) != dwin_ok)
		return;

#if (USING_DEBUG && DWIN_USING_DEBUG)
	DWIN_DEBUG(DWIN_DEBUG_UART, "addr = 0x%x\r\n", addr);
#endif
	for (i = 0; i < pd->Slave.Events_Size; ++i)
	{
		if (pd->Slave.pMap[i].addr == addr)
		{
			if (pd->Slave.pMap[i].event)
				pd->Slave.pMap[i].event(pd, i);
			break;
		}
	}

#if (0 == DWIN_USING_RB)
	memset(pd->Slave.pRbuf, 0x00, pd->Slave.RxCount);
	pd->Slave.RxCount = 0U;
#endif
}

/**
 * @brief  迪文屏幕数据保存到EEPROM/Modbus协议栈
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
void Dwin_Save(pDwinHandle pd)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (pd && ps)
	{
		/*计算crc校验码*/
		ps->Crc = Get_Crc16((uint8_t *)ps, sizeof(Storage_TypeDef) - sizeof(ps->Crc), 0xFFFF);
		/*参数保存到Flash*/
		IapWrites(pd->Save_Addr, (const uint8_t *)ps, sizeof(Storage_TypeDef));
	}
}

/**
 * @brief  迪文屏幕切换到极限电流/极限功率放电模式
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisMode(pDwinHandle pd, uint8_t Site)
{
#define CURRENT_PAGE 6U
#define POWER_PAGE 7U
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	Site = Site;

	if (pd && ps)
	{
#if (!DWIN_USING_RB)
		uint8_t dat = (uint8_t)Get_Data(dw_rx_ptr(pd), 7U, dw_rx_ptr(pd)[6U]);
#else
		uint8_t dat = (uint8_t)Get_Data(dw_rx_ptr(pd), 1U, dw_rx_ptr(pd)[0]);
#endif
		uint8_t page = 0;
		if (dat == RSURE_CODE)
		{
			__SET_FLAG(ps->flag, I_Limit_Enable);
			__RESET_FLAG(ps->flag, P_Limit_Enable);
			page = CURRENT_PAGE;
		}
		else
		{
			__SET_FLAG(ps->flag, P_Limit_Enable);
			__RESET_FLAG(ps->flag, I_Limit_Enable);
			page = POWER_PAGE;
		}
		// Dwin_Save(pd);
		Dwin_PageChange(pd, page);
	}
}

/**
 * @brief  迪文屏幕设置放电仪放电参数
 * @param  pd 迪文屏幕对象句柄
 * @param  site 记录当前Map中位置
 * @retval None
 */
#if (!DWIN_USING_RB)
#define __dwin_get_data_at_macro(__pd, __type) (dat = (__type)Get_Data(dw_rx_ptr(__pd), 7U, dw_rx_ptr(__pd)[6U]))
#else
#define __dwin_get_data_at_macro(__pd, __type) (dat = (__type)Get_Data(dw_rx_ptr(__pd), 1U, dw_rx_ptr(__pd)[0]))
#endif

#define __Dwin_SetValue(__pd, __site, __type, __value)                                                          \
	do                                                                                                          \
	{                                                                                                           \
		if (__pd)                                                                                               \
		{                                                                                                       \
			__type dat = 0;                                                                                     \
			__dwin_get_data_at_macro(__pd, __type);                                                             \
			if ((dat >= (__type)pd->Slave.pMap[__site].lower) && (dat <= (__type)pd->Slave.pMap[__site].upper)) \
			{                                                                                                   \
				if (__site < __pd->Slave.Events_Size)                                                           \
				{                                                                                               \
					(__value) = dat;                                                                            \
				}                                                                                               \
			}                                                                                                   \
			else                                                                                                \
				(__value) = (__type)pd->Slave.pMap[__site].lower;                                               \
			Dwin_Write(pd, pd->Slave.pMap[__site].addr, (uint8_t *)&(__value), sizeof((__value)));              \
		}                                                                                                       \
	} while (false)

/**
 * @brief  迪文屏幕设置放电仪从站id
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetSlaveId(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint16_t, ps->Slave_Id);
}

/**
 * @brief  迪文屏幕设置放电时长
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisTargetTimes(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint16_t, ps->Target_Timers);
}

/**
 * @brief  迪文屏幕设置放电截止电压
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisVcutoff(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint16_t, ps->V_CuttOff);
}

/**
 * @brief  迪文屏幕设置二次放电起放电压
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisVreboot(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint16_t, ps->V_Reboot);
}

/**
 * @brief  迪文屏幕设置极限放电电流
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisILimit(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint8_t, ps->I_Limit);
}

/**
 * @brief  迪文屏幕设置极限放电功率
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SetDisPLimit(pDwinHandle pd, uint8_t Site)
{
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	if (ps)
		__Dwin_SetValue(pd, Site, uint16_t, ps->P_Limit);
}

#define __Clear_UserInfo(__pd)                                                        \
	do                                                                                \
	{                                                                                 \
		uint32_t temp_value = 0;                                                      \
		user_name = user_code = 0x0000;                                               \
		Dwin_Write(__pd, USER_NAME_ADDR, (uint8_t *)&temp_value, sizeof(temp_value)); \
	} while (false)

/**
 * @brief  迪文屏幕用户登录确认
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_LoginSure(pDwinHandle pd, uint8_t Site)
{
#define USER_NAMES 1001
#define USER_PASSWORD 6666
#define SETTNG_PAGE 0x06

	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
#if (!DWIN_USING_RB)
	uint16_t dat = Get_Data(dw_rx_ptr(pd), 7U, dw_rx_ptr(pd)[6U]);
#else
	uint16_t dat = Get_Data(dw_rx_ptr(pd), 1U, dw_rx_ptr(pd)[0]);
#endif

	uint16_t addr = pd->Slave.pMap[Site].addr;
	uint16_t page = 0;
	uint16_t default_name = ps->User_Name, defalut_code = ps->User_Code;
	// uint32_t temp_value = 0;

	if ((dat >= pd->Slave.pMap[Site].lower) && (dat <= pd->Slave.pMap[Site].upper))
	{
		addr == USER_NAME_ADDR ? user_name = dat : (addr == USER_CODE_ADDR ? user_code = dat : 0U);
		if ((addr == LOGIN_SURE_ADDR) && (dat == RSURE_CODE))
		{ /*密码用户名正确*/
			if ((user_name == default_name) && (user_code == defalut_code))
			{ /*清除错误信息*/
				error = 0x0000;
				page = __GET_FLAG(ps->flag, I_Limit_Enable) ? CURRENT_PAGE : POWER_PAGE;
				Dwin_PageChange(pd, page);

				/*上报后台参数*/
				Dwin_Write(pd, SLAVE_ID_ADDR, (uint8_t *)ps,
						   GET_PARAM_SITE(Storage_TypeDef, flag, uint8_t));

#if (USING_DEBUG)
				Uartx_Printf(&Uart1, "success: The password is correct!\r\n");
#endif
			}
			else
			{
				/*用户名、密码错误*/
				if ((user_name != default_name) && (user_code != defalut_code))
				{
					error = 0x0003;
#if (USING_DEBUG)
					Uartx_Printf(&Uart1, "error: Wrong user name and password!\r\n");
#endif
				}
				/*用户名错误*/
				else if (user_name != default_name)
				{
					error = 0x0001;
#if (USING_DEBUG)
					Uartx_Printf(&Uart1, "error: User name error!\r\n");
#endif
				}
				/*密码错误*/
				else
				{
					error = 0x0002;

#if (USING_DEBUG)
					Uartx_Printf(&Uart1, "error: User password error!\r\n");
#endif
				}
			}
		}
		if ((addr == LOGIN_CANCEL_ADDR) && (dat == RCANCEL_CODE))
		{
			error = 0x0000;
			// user_name = user_code = 0x0000;
			// Dwin_Write(pd, USER_NAME_ADDR, (uint8_t *)&temp_value, sizeof(temp_value));
			__Clear_UserInfo(pd);
#if (USING_DEBUG)
			Uartx_Printf(&Uart1, "success: Clear Error Icon!\r\n");
#endif
		}
		Dwin_Write(pd, INPUT_ERROR_ADDR, (uint8_t *)&error, sizeof(error));
	}
}

/**
 * @brief  迪文屏幕参数保存确认
 * @param  pd 迪文屏幕对象句柄
 * @param  Site 记录当前Map中位置
 * @retval None
 */
static void Dwin_SaveSure(pDwinHandle pd, uint8_t Site)
{
#define MAIN_PAGE 2U
	Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
	Site = Site;

	if (pd && ps)
	{
#if (!DWIN_USING_RB)
		uint8_t dat = (uint8_t)Get_Data(dw_rx_ptr(pd), 7U, dw_rx_ptr(pd)[6U]);
#else
		uint8_t dat = Get_Data(dw_rx_ptr(pd), 1U, dw_rx_ptr(pd)[0]);
#endif

		if (dat == RSURE_CODE)
		{
			__SET_FLAG(ps->flag, Save_Flag);
			/*清除用户登录信息*/
			// user_name = user_code = 0;
			__Clear_UserInfo(pd);
			Dwin_Save(pd);
			Dwin_PageChange(pd, MAIN_PAGE);
		}
	}
}

/**
 * @brief  取得16bitCRC校验码
 * @param  ptr   当前数据串指针
 * @param  length  数据长度
 * @param  init_dat 校验所用的初始数据
 * @retval 16bit校验码
 */
uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat)
{
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t crc16 = init_dat;

	for (i = 0; i < length; i++)
	{
		crc16 ^= *ptr++;

		for (j = 0; j < 8; j++)
		{
			if (crc16 & 0x0001)
			{
				crc16 = (crc16 >> 1) ^ 0xa001;
			}
			else
			{
				crc16 = crc16 >> 1;
			}
		}
	}
	return (crc16);
}
