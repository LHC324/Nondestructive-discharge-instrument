#ifndef __DISCHARGER_H_
#define __DISCHARGER_H_
#include "config.h"
#include "usart.h"

#define MS_CONVERT_MINUTE 60U // 把应硬件定时器10ms换算成分钟
#define S_CONVERT_HOUR 3600U  // 1S累计的电量换算成1h累计的电量
/*
 * 获得当前充电电量：单位Ah
 */
#define __Get_ChargingQuantity(__Q) ((float)((__Q) / S_CONVERT_HOUR))
/*
 * 获得当前充电时间: 单位min
 */
#define __Get_ChargingTimes(__T) ((uint32_t)((__T) / MS_CONVERT_MINUTE))

typedef enum
{
    Standy = 0x01,
    Work,
    Error,
    Start,
    End,
} Discharger_State;

// typedef enum
// {
//     Open,
//     Close,
// } Discharger_Animation;

typedef enum
{
    Internal_Limit, // 内部限制
    I_Limit_Enable, // 使能电流极限
    P_Limit_Enable, // 使能功率极限
    Save_Flag,      // 参数保存标志
} Discharger_Flag_Group;

typedef struct
{
    float V_Battery;
    float V_Discharger;
    float I_Discharger;
    float S_Temperature;
    uint32_t T_Discharger;
    // float Q_Discharger;
    float I_Battery;
    float P_Discharger;
    float E_Discharger; /*转换效率*/
    // uint16_t W_State;   /*工作状态*/
    uint16_t M_State;       /*机器状态*/
    uint16_t Animation : 1; /*动画*/
} Current_TypeDef;

typedef struct
{ /*迪文屏幕变量显示必须占用16bit*/
    uint16_t Slave_Id;
    uint16_t Target_Timers;
    uint16_t V_CuttOff;
    uint16_t V_Reboot;
    uint16_t I_Limit;
    uint16_t P_Limit;
    // uint8_t Internal_Limit : 1; /*内部限制*/
    uint8_t flag;
    uint16_t User_Name;
    uint16_t User_Code;
    uint16_t Crc;
} Storage_TypeDef;

typedef struct
{
    // uint8_t flag;
    // Discharger_State Status;
    Current_TypeDef Current;
    Storage_TypeDef Storage;
} Discharger_TypeDef;

extern Discharger_TypeDef discharger;
extern void Discharger_Handle(Discharger_TypeDef *pd, Uart_HandleTypeDef *puart);
extern void Set_DischargerParam(Discharger_TypeDef *pd, uint8_t *pbuf);

#endif
