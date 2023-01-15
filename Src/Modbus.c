/*
 * ModbusSlave.c
 *
 *  Created on: 2022年04月08日
 *      Author: LHC
 */

#include "Modbus.h"
#include "usart.h"
// #include "tool.h"
#include "discharger.h"
#include "Dwin.h"
#include "eeprom.h"
#if (USIING_OTA)
#include "ymodem.h"
#endif

/*使用屏幕接收处理时*/
#define TYPEDEF_STRUCT uint8_t
/*定义Modbus对象*/
// pModbusHandle Modbus_Object;
extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);
ModbusPools Spool;

/*静态函数声明*/
// static void Modbus_TI_Recive(pModbusHandle pd, DMA_HandleTypeDef *hdma);
static void Modbus_Poll(pModbusHandle pd);
static void Modbus_Send(pModbusHandle pd, enum Using_Crc crc);
#if defined(USING_MASTER)
static void Modbus_46H(pModbusHandle pd, uint16_t regaddr, uint8_t *pdat, uint8_t datalen);
#endif
// static uint8_t Modbus_Operatex(pModbusHandle pd, uint16_t addr, uint8_t *pdat, uint8_t len);
// static uint8_t Modbus_Operatex(pModbusHandle pd, uint16_t addr, uint8_t *pdat, uint8_t len);
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
static void Modus_ReadXCoil(pModbusHandle pd);
static void Modus_WriteCoil(pModbusHandle pd);
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
static void Modus_ReadXRegister(pModbusHandle pd);
static void Modus_WriteHoldRegister(pModbusHandle pd);
#endif
static void Modus_ReportSeverId(pModbusHandle pd);
static void Modbus_CallBack(pModbusHandle pd);

static uint8_t mtx_buf[128];
#if (USIING_OTA)
struct ringbuffer modbus_rb = {
    NULL,
    0,
    0,
    0,
};
// static uint8_t modbus_rx_buf[1032];
#endif
/*定义Modbus对象*/
ModbusHandle Modbus_Object = {
    0x02,
#if (USIING_OTA)
    false,
#endif
    Modbus_Poll,
    Modbus_Send,
    Modbus_46H,
    // Modbus_Operatex,
    Modus_ReadXCoil,
    Modus_WriteCoil,
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
    Modus_ReadXRegister,
    Modus_WriteHoldRegister,
    Modbus_CallBack,
#endif
    // Modus_ReportSeverId,
    {mtx_buf, sizeof(mtx_buf), 0},
#if (!MODBUS_USING_RB)
    {NULL, 0, &modbus_rb, &discharger.Storage, InputCoil, &Spool, Read},
#else
    {&modbus_rb, NULL, &discharger.Storage, InputCoil, &Spool, Read},
#endif
    &Uart2,
};

// void *md_memcpy(void *s1, const void *s2, size_t n)
// {
//     uint8_t *dest = (uint8_t *)s1;
//     const uint8_t *source = (const uint8_t *)s2;

//     if (NULL == dest || NULL == source || !n)
//         return NULL;

//     while (n--)
//     {
//         *dest++ = *source++;
//     }

//     return s1;
// }

void Modbus_Handle(void)
{
    Modbus_Object.Mod_Poll(&Modbus_Object);
}

#define MOD_WORD 1U
#define MOD_DWORD 2U
#if (!MODBUS_USING_RB)
#define md_rx_ptr(__ptr) ((__ptr)->Slave.pRbuf)
/*获取主机号*/
#define Get_ModId(__obj) ((__obj)->Slave.pRbuf[0U])
/*获取Modbus功能号*/
#define Get_ModFunCode(__obj) ((__obj)->Slave.pRbuf[1U])
/*获取Modbus协议数据*/
#define Get_Data(__ptr, __s, __size)                                                                           \
    ((__size) < 2U ? ((uint16_t)((__ptr)->Slave.pRbuf[__s] << 8U) | ((__ptr)->Slave.pRbuf[__s + 1U]))          \
                   : ((uint32_t)((__ptr)->Slave.pRbuf[__s] << 24U) | ((__ptr)->Slave.pRbuf[__s + 1U] << 16U) | \
                      ((__ptr)->Slave.pRbuf[__s + 2U] << 8U) | ((__ptr)->Slave.pRbuf[__s + 3U])))
#else
#define md_rx_ptr(__ptr) ((__ptr)->Slave.pdat)
#define Get_ModId(__ptr) (__ptr[0U])
#define Get_Data(__buf, __s, __size)                                             \
    ((__size) < 2U ? ((uint16_t)(__buf[__s] << 8U) | (__buf[__s + 1U]))          \
                   : ((uint32_t)(__buf[__s] << 24U) | (__buf[__s + 1U] << 16U) | \
                      (__buf[__s + 2U] << 8U) | (__buf[__s + 3U])))
#endif

/**
 * @brief  Modbus从机响应回调
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
static void Modbus_CallBack(pModbusHandle pd)
{
    Storage_TypeDef *ps = (Storage_TypeDef *)pd->Slave.pHandle;
    pDwinHandle pdw = &Dwin_Object;
    uint16_t addr = 0, dat = 0, *pdat;
    uint8_t save_flag = false;

    addr = Get_Data(pd, 2U, MOD_WORD);
    dat = Get_Data(pd, 4U, MOD_WORD);

    if (ps)
    {
        pdat = &ps->Slave_Id;
        switch (Get_ModFunCode(pd))
        {
        case WriteCoil:
        { /*初始化时需要同时更新该寄存器*/
            if (!addr)
            {
                dat                                        ? __SET_FLAG(ps->flag, P_Limit_Enable),
                    __RESET_FLAG(ps->flag, I_Limit_Enable) : (__SET_FLAG(ps->flag, I_Limit_Enable), __RESET_FLAG(ps->flag, P_Limit_Enable));
                save_flag = true;
            }
        }
        break;
        case WriteHoldReg:
        {

            if ((dat >= pdw->Slave.pMap[addr].lower) && (dat <= pdw->Slave.pMap[addr].upper) &&
                addr < pdw->Slave.Events_Size)
            {
                pdat[addr] = dat;
                save_flag = true;
            }
            else
            {
                /*保存原值不变:数据写回保持寄存器*/
                pd->Slave.Reg_Type = HoldRegister;
                pd->Slave.Operate = Write;
                /*读取对应寄存器*/
                if (!Modbus_Operatex(pd, addr, (uint8_t *)&pdat[addr], sizeof(uint16_t)))
                {
#if defined(USING_DEBUG)
                    //                    Debug("Coil reading failed!\r\n");

#endif
                    return;
                }
            }
        }
        break;
        default:
            break;
        }
        if (save_flag)
        {
            __SET_FLAG(ps->flag, Save_Flag);
            Dwin_Save(pdw);
        }
    }
}

#if (USIING_OTA)
/**
 * @brief  modbus协议栈进行ota升级
 * @param  pd modbus协议站句柄
 * @retval None
 */
static void lhc_ota_update(pModbusHandle pd)
{
    char ret;
    uint8_t ota_value = OTA_FLAG_VALUE;
    // pd = pd;

    // IapWrites(OTA_FLAG_ADDR, &ota_value, sizeof(ota_value)); // 写入ota标志
    // IAP_CONTR = 0x60;                                        // 复位单片机

    // ret < 0:失败 / =0:成功 / > 0：等待
    ret = Ota_Menue(pd->Slave.rb);
    /*升级失败:退出ota模式，进入modbus模式*/
    if (ret <= 0)
        pd->Ota_Flag = false;

    if (!ret) // 应用程序后台升级成功
    {
        // IapWrites(OTA_FLAG_ADDR, &ota_value, sizeof(ota_value)); // 写入ota标志
        IAP_CONTR = 0x60; // 复位单片机
    }
}

/**
 * @brief	Determine how the wifi module works
 * @details
 * @param	pd:modbus master/slave handle
 * @retval	true：MODBUS;fasle:shell
 */
static bool lhc_check_is_ota(pModbusHandle pd)
{
    if (NULL == pd || NULL == pd->Slave.pRbuf)
        return false;

#define ENTER_OTA_MODE_CODE 0x0D
    return (((pd->Slave.RxCount == 1U) &&
             (pd->Slave.pRbuf[0] == ENTER_OTA_MODE_CODE)));
#undef ENTER_OTA_MODE_CODE
}
#endif

/**
 * @brief  Modbus接收数据解析
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
static void Modbus_Poll(pModbusHandle pd)
{
    uint16_t crc16 = 0;
#if (!MODBUS_USING_RB)
    uint32_t len = 0;

    if (NULL == pd || NULL == pd->Slave.pRbuf)
        return;

#if (USIING_OTA)
    /*检查是否进入OTA升级*/

    if (!pd->Ota_Flag)
    {
        if (lhc_check_is_ota(pd))
        {
            pd->Ota_Flag = true;
        }
    }
    else
    {
        lhc_ota_update(pd);
        return;
    }
#endif
    if (!pd->Slave.RxCount)
        return;
#else
    if (NULL == pd || NULL == pd->Slave.rb ||
        NULL == pd->Slave.rb->buf || !ringbuffer_num(pd->Slave.rb)) // 无数据，直接退出
        return;
#endif

#if (!MODBUS_USING_RB)
    /*首次调度时RXcount值被清零，导致计算crc时地址越界*/
    if (pd->Slave.RxCount > 2U)
        crc16 = Get_Crc16(pd->Slave.pRbuf, pd->Slave.RxCount - 2U, 0xffff);
#if defined(USING_DEBUG)
        // Debug("rxcount = %d,crc16 = 0x%X.\r\n", pd->Slave.RxCount, (uint16_t)((crc16 >> 8U) | (crc16 << 8U)));
#endif
    crc16 = (crc16 >> 8U) | (crc16 << 8U);
    /*检查是否是目标从站*/
    if ((Get_ModId(pd) == pd->Slave_Id) &&
        (Get_Data(pd, pd->Slave.RxCount - 2U, MOD_WORD) == crc16))
#else

    len = ringbuffer_gets(pd->Slave.rb, modbus_rx_buf, 1U);

    if ((1U != len) && (Get_ModId(modbus_rx_buf) != pd->Slave_Id))
        return;

#endif
    {
#if defined(USING_DEBUG)
        // Debug("Data received!\r\n");
        // for (uint8_t i = 0; i < pd->Slave.RxCount; i++)
        // {
        //     Debug("prbuf[%d] = 0x%X\r\n", i, pd->Slave.pRbuf[i]);
        // }
#endif
        switch (Get_ModFunCode(pd))
        {
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
        case ReadCoil:
        case ReadInputCoil:
        {
            pd->Mod_ReadXCoil(pd);
        }
        break;
        case WriteCoil:
        case WriteCoils:
        {
            pd->Mod_WriteCoil(pd);
            pd->Mod_CallBack(pd);
        }
        break;
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
        case ReadHoldReg:
        case ReadInputReg:
        {
            pd->Mod_ReadXRegister(pd);
        }
        break;
        case WriteHoldReg:
        case WriteHoldRegs:
        {
            pd->Mod_WriteHoldRegister(pd);
            pd->Mod_CallBack(pd);
        }
        break;
#endif
        case ReportSeverId:
        {
            //                pd->Mod_ReportSeverId(pd);
            /**/
            if (pd->Slave.pHandle)
            {
                *(TYPEDEF_STRUCT *)pd->Slave.pHandle = true;
            }
        }
        break;
        default:
            break;
        }
    }
#if (!MODBUS_USING_RB)
    memset(pd->Slave.pRbuf, 0x00, pd->Slave.RxCount);
    pd->Slave.RxCount = 0U;
#endif
}

/*获取寄存器类型*/
#define Get_RegType(__obj, __type) \
    ((__type) < InputRegister ? (__obj)->Slave.pPools->Coils : (__obj)->Slave.pPools->InputRegister)

/*获取寄存器地址*/
#if defined(USING_COIL) && defined(USING_INPUT_COIL) && defined(USING_INPUT_REGISTER) && defined(USING_HOLD_REGISTER)
#define Get_RegAddr(__obj, __type, __addr)                                        \
    ((__type) == Coil                                                             \
         ? (uint8_t *)&(__obj)->Slave.pPools->Coils[__addr]                       \
         : ((__type) == InputCoil                                                 \
                ? (uint8_t *)&(__obj)->Slave.pPools->InputCoils[__addr]           \
                : ((__type) == InputRegister                                      \
                       ? (uint8_t *)&(__obj)->Slave.pPools->InputRegister[__addr] \
                       : (uint8_t *)&(__obj)->Slave.pPools->HoldRegister[__addr])))
#elif defined(USING_COIL) || defined(USING_INPUT_COIL)
#define Get_RegAddr(__obj, __type, __addr) \
    ((__type) == Coil ? (uint8_t *)&(__obj)->Slave.pPools->Coils[__addr] : (uint8_t *)&(__obj)->Slave.pPools->InputCoils[__addr])
#elif defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
#define Get_RegAddr(__obj, __type, __addr) \
    ((__type) == InputRegister ? (uint8_t *)&(__obj)->Slave.pPools->InputRegister[__addr] : (uint8_t *)&(__obj)->Slave.pPools->HoldRegister[__addr])
#else
#define Get_RegAddr(__obj, __type, __addr) (__obj, __type, __addr)
#endif

/**
 * @brief  Modbus协议读取/写入寄存器
 * @param  pd 需要初始化对象指针
 * @param  regaddr 寄存器地址[寄存器起始地址从1开始]
 * @param  pdat 数据指针
 * @param  len  读取数据长度
 * @retval None
 */
uint8_t Modbus_Operatex(pModbusHandle pd, uint16_t addr, uint8_t *pdat, uint8_t len)
{
    // uint16_t offset = pd->Slave.Reg_Type > Coil ? (pd->Slave.Reg_Type + 10000U) : 1U;
    uint8_t max = pd->Slave.Reg_Type < InputRegister ? REG_POOL_SIZE : REG_POOL_SIZE * 2U;
    // uint8_t reg_addr = addr - 1U, *pDest, *pSou;
    uint8_t *pDest, *pSou;
    // typeof(Get_RegType(pd, pd->Slave.Reg_Type)) *paddr;
    uint8_t ret = false;

    if (NULL == pd || NULL == pdat || !len)
        return ret;
#if defined(USING_DEBUG)
        // if (addr < 1U)
        // {
        //     Debug("Error: Register address must be > = 1.\r\n");
        // }
#endif
    // if (reg_addr < max)
    if ((addr < max) && (len < max))
    {
#if defined(USING_COIL) || defined(USING_INPUT_COIL) || defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
        if (pd->Slave.Operate == Read)
        {
            pDest = pdat, pSou = Get_RegAddr(pd, pd->Slave.Reg_Type, addr);
            // pd->Slave.pPools->Coils[0] = 0x000;
            // pDest = (uint8_t *)Get_RegType(pd, pd->Slave.Reg_Type);
        }
        else
        {
            pDest = Get_RegAddr(pd, pd->Slave.Reg_Type, addr), pSou = pdat;
        }
#endif
#if defined(USING_DEBUG)
        // Debug("pdest[%p] = 0x%X, psou[%p]= 0x%X, len= %d.\r\n", pDest, *pDest, pSou, *pSou, len);
#endif
        if (pDest && pSou && memmove(pDest, pSou, len)) // md_memcpy
            ret = true;
    }
    return ret;
}

/**
 * @brief  Modbus协议主站有人云拓展46指令
 * @param  pd 需要初始化对象指针
 * @param  regaddr 寄存器地址
 * @param  pdata 数据指针
 * @param  datalen 数据长度
 * @retval None
 */
#if defined(USING_MASTER)
static void Modbus_46H(pModbusHandle pd, uint16_t regaddr, uint8_t *pdat, uint8_t datalen)
{
#define MASTER_FUNCTION_CODE 0x46
    uint8_t buf[] = {0, MASTER_FUNCTION_CODE, 0, 0, 0, 0, 0};
    buf[0] = pd->Slave_Id, buf[2] = regaddr >> 8U, buf[3] = regaddr,
    buf[4] = (datalen / 2U) >> 8U, buf[5] = (datalen / 2U), buf[6] = datalen;

    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    pd->Master.TxCount = 0U;
    memcpy(pd->Master.pTbuf, buf, sizeof(buf));
    pd->Master.TxCount += sizeof(buf);
    memcpy(&pd->Master.pTbuf[pd->Master.TxCount], pdat, datalen);
    pd->Master.TxCount += datalen;

    pd->Mod_Transmit(pd, UsedCrc);
}
#endif

/**
 * @brief  Modbus协议读取线圈和输入线圈状态(0x01\0x02)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
static void Modus_ReadXCoil(pModbusHandle pd)
{
#define Byte_To_Bits 8U
    uint8_t len = Get_Data(pd, 4U, MOD_WORD);
    // uint8_t bytes = len / Byte_To_Bits + !!(len % Byte_To_Bits);
    uint8_t bytes = (len >> 3U) + !!(len & (Byte_To_Bits - 1U));
    uint8_t buf[REG_POOL_SIZE * 2U], *prbits = &buf;
    uint8_t i = 0, j = 0;
    // uint8_t bits = 0x00;
    if (len < sizeof(buf))
    {
        memset(prbits, 0x00, len);
        /*必须清除pbuf，原因是：519行漏洞*/
        memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
        pd->Master.TxCount = 0U;
        memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
        pd->Master.TxCount += 2U;
        pd->Master.pTbuf[pd->Master.TxCount++] = bytes;
        /*通过功能码寻址寄存器*/
        pd->Slave.Reg_Type = Get_ModFunCode(pd) == ReadCoil ? Coil : InputCoil;
        pd->Slave.Operate = Read;
        // pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prbits, len);
        Modbus_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prbits, len);
#if defined(USING_DEBUG)
        // for (uint8_t i = 0; i < len; i++)
        //     Debug("prbits[%d] = 0x%X, len= %d.\r\n", i, prbits[i], len);
#endif
        for (; i < bytes; i++)
        {
            /*2022/11/2：此处j每一轮都要初始化，否则只能读到前8bit*/
            for (j = 0; j < Byte_To_Bits && (i * Byte_To_Bits + j) < len; j++)
            {
                uint8_t _bit = (prbits[i * Byte_To_Bits + j] & 0x01);
                if (_bit)
                    pd->Master.pTbuf[pd->Master.TxCount] |= (_bit << j);
                else
                    pd->Master.pTbuf[pd->Master.TxCount] &= ~(_bit << j);
#if defined(USING_DEBUG)
                    // Debug("pTbuf[%d] = 0x%X, j= %d.\r\n", i, pd->Master.pTbuf[pd->Master.TxCount], j);
#endif
            }
            pd->Master.TxCount++;
        }
#if defined(USING_DEBUG)
        // Debug("pd->Master.TxCount = %d.\r\n", pd->Master.TxCount);
#endif
        pd->Mod_Transmit(pd, UsedCrc);
    }
}

/**
 * @brief  Modbus协议写线圈/线圈组(0x05\0x0F)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modus_WriteCoil(pModbusHandle pd)
{
    uint8_t *pdat = NULL, len = 0x00, i = 0;
    enum Using_Crc crc;

    /*通过功能码寻址寄存器*/
    pd->Slave.Reg_Type = Coil;
    pd->Slave.Operate = Write;
    /*写单个线圈*/
    if (Get_ModFunCode(pd) == WriteCoil)
    {
        uint8_t wbit = !!(Get_Data(pd, 4U, MOD_WORD) == 0xFF00);
        len = 1U;
        pdat = &wbit;
        pd->Master.TxCount = pd->Slave.RxCount;
        crc = NotUsedCrc;
    }
    /*写多个线圈*/
    else
    {
        len = Get_Data(pd, 4U, MOD_WORD);
        /*利用发送缓冲区空间暂存数据*/
        pdat = pd->Master.pTbuf;

        for (; i < len; i++)
        {
            pdat[i] = (pd->Slave.pRbuf[7U + i / Byte_To_Bits] >> (i % Byte_To_Bits)) & 0x01;
        }
        pd->Master.TxCount = 6U;
        crc = UsedCrc;
    }
#if defined(USING_DEBUG)
    // Debug("pdata = 0x%X, len= %d.\r\n", *pdat, len);
#endif
    if (pdat)
        // pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdat, len);
        Modbus_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdat, len);
    // memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    /*请求数据原路返回*/
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, pd->Master.TxCount);
    pd->Mod_Transmit(pd, crc);
}
#endif

/**
 * @brief  Modbus协议读输入寄存器/保持寄存器(0x03\0x04)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
static void Modus_ReadXRegister(pModbusHandle pd)
{
    uint8_t len = Get_Data(pd, 4U, MOD_WORD) * sizeof(uint16_t);
    uint8_t buf[REG_POOL_SIZE * 2U], *prdata = &buf;

    if (!prdata < sizeof(buf))
    {
        memset(prdata, 0x00, len);
        memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
        pd->Master.TxCount = 0U;
        memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
        pd->Master.TxCount += 2U;
        pd->Master.pTbuf[pd->Master.TxCount] = len;
        pd->Master.TxCount += sizeof(len);
        /*通过功能码寻址寄存器*/
        pd->Slave.Reg_Type = Get_ModFunCode(pd) == ReadHoldReg ? HoldRegister : InputRegister;
        pd->Slave.Operate = Read;
        // pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prdata, len);
        Modbus_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prdata, len);
        memcpy(&pd->Master.pTbuf[pd->Master.TxCount], prdata, len);
        pd->Master.TxCount += len;

        pd->Mod_Transmit(pd, UsedCrc);
    }
}

/**
 * @brief  Modbus协议写保持寄存器/多个保持寄存器(0x06/0x10)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modus_WriteHoldRegister(pModbusHandle pd)
{
    uint8_t *pdat = NULL, len = 0x00;
    enum Using_Crc crc;

    pd->Slave.Reg_Type = HoldRegister;
    pd->Slave.Operate = Write;
    /*写单个保持寄存器*/
    if (Get_ModFunCode(pd) == WriteHoldReg)
    {
        len = sizeof(uint16_t);
        /*改变数据指针*/
        pdat = &pd->Slave.pRbuf[4U];
        pd->Master.TxCount = pd->Slave.RxCount;
        crc = NotUsedCrc;
    }
    else
    {
        len = pd->Slave.pRbuf[6U];
        /*改变数据指针*/
        pdat = &pd->Slave.pRbuf[7U];
        pd->Master.TxCount = 6U;
        crc = UsedCrc;
    }
    if (pdat)
        // pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdat, len);
        Modbus_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdat, len);
    /*请求数据原路返回*/
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, pd->Master.TxCount);
    pd->Mod_Transmit(pd, crc);
}
#endif

/**
 * @brief  Modbus协议上报一些特定信息
 * @param  pd 需要初始化对象指针
 * @retval None
 */
// static void Modus_ReportSeverId(pModbusHandle pd)
// {
//     // extern TIM_HandleTypeDef htim1;
//     // HAL_TIM_Base_Stop_IT(&htim1);
//     // memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
//     pd->Master.TxCount = 0U;
//     memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
//     pd->Master.TxCount += 2U;
//     pd->Master.pTbuf[pd->Master.TxCount++] = sizeof(uint8_t);
//     /*读取卡槽与板卡编码*/
//     pd->Master.pTbuf[pd->Master.TxCount++] = Get_CardId();
//     pd->Mod_Transmit(pd, UsedCrc);
//     // HAL_TIM_Base_Start_IT(&htim1);
// }

/**
 * @brief  Modbus协议发送
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modbus_Send(pModbusHandle pd, enum Using_Crc crc)
{
    if (crc == UsedCrc)
    {
        uint16_t crc16 = Get_Crc16(pd->Master.pTbuf, pd->Master.TxCount, 0xffff);

        crc16 = (crc16 >> 8U) | (crc16 << 8U);
        memcpy(&pd->Master.pTbuf[pd->Master.TxCount], (uint8_t *)&crc16, sizeof(crc16));
        pd->Master.TxCount += sizeof(crc16);
    }
    if (pd->huart)
        Uartx_SendStr(pd->huart, pd->Master.pTbuf, pd->Master.TxCount, UART_BYTE_SENDOVERTIME);

    // HAL_UART_Transmit_DMA(pd->huart, pd->Master.pTbuf, pd->Master.TxCount);
    // while (__HAL_UART_GET_FLAG(pd->huart, UART_FLAG_TC) == RESET)
    // {
    // }
}
