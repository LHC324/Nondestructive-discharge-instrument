#include "discharger.h"
#include "eeprom.h"

Discharger_TypeDef discharger = {0};

/*转换效率*/
#define NCELL 0.9F
/*获取交流电压*/
#define __Get_VC_Voltage(uart, addr) \
    ((float)uart->Rx.pbuf[addr] * 25.6F + (float)uart->Rx.pbuf[addr + 1U] * 0.1F)
/*获取交流电流*/
#define __Get_AC_Current(p_vc, v_ac) (v_ac ? p_vc / v_ac : 0)
/*获取直流电压*/
#define __Get_DC_Voltage(uart, addr)                                                                \
    ((float)uart->Rx.pbuf[addr] * 32.0F + (float)((uart->Rx.pbuf[addr + 1U] & 0xF0) >> 4U) * 2.0F + \
     (float)(uart->Rx.pbuf[addr + 1U] & 0x0F) * 0.1F)
/*获取直流电流*/
#define __Get_DC_Current(p_vc, v_dc) (p_vc ? p_vc / (NCELL * v_dc) : 0)
/*获取放电功率*/
#define __Get_DisPower(uart, addr) \
    ((float)uart->Rx.pbuf[addr] * 25.6F + (float)uart->Rx.pbuf[addr + 1U] / 10.0F)
/*获取当前转换效率*/
#define __Get_NCELL(p_dc, p_ac) (p_ac ? (p_dc / p_ac) * 100.0F : 0)
/*组合2字节数据*/
#define __Get_Data(uart, addr) \
    ((uint16_t)(uart->Rx.pbuf[addr] << 8U | uart->Rx.pbuf[addr + 1U]))
/*获取系统温度*/
#define __Get_SystemTemparture(x) \
    (x > 320U ? (-35.02 * log(x) + 246.42F) : (-0.0916F * x + 72.756F))

/**
 * @brief	放电数据处理
 * @details
 * @param	None
 * @retval	None
 */
void Discharger_Handle(Discharger_TypeDef *pd, Uart_HandleTypeDef *puart)
{
#define DATA_START_ADDR 287U
    if (pd && puart)
    {
        /*解决部分变量未清零*/
        // memset(&pd->Current, 0x00, sizeof(pd->Current));
        pd->Current.V_Discharger = __Get_VC_Voltage(puart, 280U);
        pd->Current.P_Discharger = __Get_DisPower(puart, 282U);
        pd->Current.I_Discharger = __Get_AC_Current(pd->Current.P_Discharger, pd->Current.V_Discharger);
        pd->Current.V_Battery = __Get_DC_Voltage(puart, 284U);
        pd->Current.I_Battery = __Get_DC_Current(pd->Current.P_Discharger, pd->Current.V_Battery);
        pd->Current.E_Discharger = __Get_NCELL(pd->Current.P_Discharger, (pd->Current.V_Battery * pd->Current.I_Battery));
        pd->Current.S_Temperature = __Get_SystemTemparture(__Get_Data(puart, 292U));
        // if (pd->Current.V_Battery)
        // {
        //     pd->Current.I_Discharger = (pd->Current.P_Discharger / pd->Current.V_Battery); //       *sqrt(3.0F);
        // }
        // pd->Current.T_Discharger += 1;
        /*识别工作状态*/
        if (pd->Current.V_Discharger < 200.0F)
        {
            __SET_FLAG(pd->Current.M_State, Error), __RESET_FLAG(pd->Current.M_State, Work),
                __RESET_FLAG(pd->Current.M_State, Standy), __RESET_FLAG(pd->Current.M_State, End);
            pd->Current.Animation = false;
        }
        else
        {
            if (pd->Current.P_Discharger > 15.0F)
            {
                __RESET_FLAG(pd->Current.M_State, Error), __SET_FLAG(pd->Current.M_State, Work),
                    __RESET_FLAG(pd->Current.M_State, Standy), __RESET_FLAG(pd->Current.M_State, End);
                __SET_FLAG(pd->Current.M_State, Start);
                pd->Current.Animation = true;
            }
            else
            {
                pd->Current.Animation = false;
                __SET_FLAG(pd->Current.M_State, Standy);
                __RESET_FLAG(pd->Current.M_State, Error), __RESET_FLAG(pd->Current.M_State, Work);
                __GET_FLAG(pd->Current.M_State, Start) ? __SET_FLAG(pd->Current.M_State, End)
                                                       : __RESET_FLAG(pd->Current.M_State, End);
                /*放电对象被拔下，结束本轮放电*/
                if (pd->Current.V_Battery < 10.0F)
                {
                    __RESET_FLAG(pd->Current.M_State, Start);
                }
            }
        }
    }
    /*可以增加波形显示*/
}

#define __Set_BufData(buf, site, value) \
    (buf[site] = value)
/**
 * @brief	根据放电参数设置数据帧
 * @details
 * @param	None
 * @retval	None
 */
void Set_DischargerParam(Discharger_TypeDef *pd, uint8_t *pbuf)
{
    if (pd && pbuf)
    {
        /*设置从站ID*/
        if (!(pd->Storage.Slave_Id > 0U) || !(pd->Storage.Slave_Id < 33U))
        {
            pd->Storage.Slave_Id = 1U;
        }
        __Set_BufData(pbuf, 38, pd->Storage.Slave_Id);
        /*设置内部限制*/
        __GET_FLAG(pd->Storage.flag, Internal_Limit) ? __Set_BufData(pbuf, 4, 2U),
            __Set_BufData(pbuf, 5, 0U)               : (__Set_BufData(pbuf, 4, 0U), __Set_BufData(pbuf, 5, 1U));
        /*设置电流极限*/
        __GET_FLAG(pd->Storage.flag, I_Limit_Enable) ? __Set_BufData(pbuf, 31, 1U) : __Set_BufData(pbuf, 31, 0U);
        /*设置放电电流*/
        if (!(pd->Storage.I_Limit > 0U) || !(pd->Storage.I_Limit < 36U))
        {
            pd->Storage.I_Limit = 1U;
        }
        __Set_BufData(pbuf, 33, pd->Storage.I_Limit);
        /*检查功率和电流模式二选一*/
        if (__GET_FLAG(pd->Storage.flag, I_Limit_Enable) && __GET_FLAG(pd->Storage.flag, P_Limit_Enable))
        {
            __RESET_FLAG(pd->Storage.flag, P_Limit_Enable);
        }
        /*设置功率极限*/
        __GET_FLAG(pd->Storage.flag, P_Limit_Enable) ? __Set_BufData(pbuf, 34, 1U) : __Set_BufData(pbuf, 34, 0U);
        /*设置放电功率*/
        if (!(pd->Storage.P_Limit > 14U) || !(pd->Storage.P_Limit < 951U))
        {
            pd->Storage.P_Limit = 15U;
        }
        __Set_BufData(pbuf, 36, pd->Storage.P_Limit >> 8U), __Set_BufData(pbuf, 37, pd->Storage.P_Limit);
        /*设置放电截止电压*/
        if (!(pd->Storage.V_CuttOff > 219U) || !(pd->Storage.V_CuttOff < 601U))
        {
            pd->Storage.V_CuttOff = 220U;
        }
        __Set_BufData(pbuf, 32, pd->Storage.V_CuttOff >> 8U), __Set_BufData(pbuf, 35, pd->Storage.V_CuttOff >> 8U);
        __Set_BufData(pbuf, 40, pd->Storage.V_CuttOff), __Set_BufData(pbuf, 41, pd->Storage.V_CuttOff);
        /*设置二次起放电压*/
        if (!(pd->Storage.V_Reboot > 219U) || !(pd->Storage.V_Reboot < 601U))
        {
            pd->Storage.V_Reboot = 220U;
        }
        __Set_BufData(pbuf, 39, pd->Storage.V_Reboot >> 8U), __Set_BufData(pbuf, 42, pd->Storage.V_Reboot);
    }
}
