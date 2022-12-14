/*********************************************************************************************************
**				                               Small RTOS 51
**                                   The Real-Time Kernel For Keil c51
**
**                                  (c) Copyright 2002-2003, chenmingji
**                                           All Rights Reserved
**
**                                                  V1.12.0
**
**
**--------------文件信息--------------------------------------------------------------------------------
**文   件   名: OS_CPU_C.C
**创   建   人: 陈明计
**最后修改日期:  2002年12月2日
**描        述: Small RTOS 51与CPU(既8051系列)相关的C语言代码
**
**--------------历史版本信息----------------------------------------------------------------------------
** 创建人: 陈明计
** 版  本: V0.50
** 日　期: 2002年2月22日
** 描　述: 原始版本
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V0.52
** 日　期: 2002年5月9日
** 描　述: 更正for keil c51 移植的堆栈在某种情况下初始值错误
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.00
** 日　期: 2002年6月20日
** 描　述: 增加配置
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.10
** 日　期: 2002年9月1日
** 描　述: 根据新版本要求更改变量定义
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.10.4
** 日　期: 2002年10月5日
** 描　述: 统一了一下格式和宏定义风格
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.10.5
** 日　期: 2002年10月23日
** 描　述: 修改OSTickISR()使得在允许的情况下，每次进入中断都调用UserTickTimer()
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.11.0
** 日　期: 2002年12月2日
** 描　述: 根据新版本要求增加函数OSIdle和修改函数OSStart；增加注释
**
**------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 版  本: V1.12.0
** 日　期: 2002年12月30日
** 描　述: 根据新版本要求修改初始化代码
**
**--------------当前版本修订------------------------------------------------------------------------------
** 修改人:
** 日　期:
** 描　述:
**
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/

#define IN_OS_CPU_C
#include "OS_CONFIG.h"

extern idata uint8 STACK[1];                           /* 堆栈起始位置,在OS_CPU_A定义 */
uint8 idata *data OSTsakStackBotton[OS_MAX_TASKS + 2]; /* 任务堆栈底部位置            */

#if EN_SP2 > 0
uint8 idata Sp2[Sp2Space]; /* 高级中断堆栈           */
#endif

#if OS_MAX_TASKS < 9
uint8 data OSFastSwap = 0xff; /* 任务是否可以快速切换 */
#else
uint16 data OSFastSwap = 0xffff;
#endif

/*********************************************************************************************************
** 函数名称: C_OSCtxSw
** 功能描述: 用C写的堆栈处理函数,已经用汇编改写,此函数在移植并非必须
** 输　入: 无
** 输　出 : 无
** 全局变量: OSTaskID,OSTsakStackBotton,SP
** 调用模块: LoadCtx
**
** 作　者: 陈明计
** 日　期: 2002年2月22日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**-------------------------------------------------------------------------------------------------------
********************************************************************************************************/
/*        void C_OSCtxSw(void)

{
    uint8 idata *cp1,idata *cp2;
    uint8 i,temp;

    cp1 = (uint8 idata *)SP +1;
    temp = (uint8 )OSTsakStackBotton[OSNextTaskID+1];
    cp2 = OSTsakStackBotton[OSTaskID+1];
    if( OSNextTaskID > OSTaskID)
    {
        while(cp2 != (uint8 idata *)temp)
        {
            *cp1++ = *cp2++;
        }
        OSNextTaskID++;
        OSTaskID++;
        temp = OSTsakStackBotton[OSTaskID] - (uint8 idata *)SP-1;
        SP = (uint8 )cp1 - 1;
        for(i = OSTaskID;i < OSNextTaskID; i++)
        {
            OSTsakStackBotton[i] -= temp;
        }
        OSNextTaskID--;
        OSTaskID = OSNextTaskID;
        LoadCtx();
    }

    if( OSNextTaskID < OSTaskID)
    {
        cp2--;
        cp1--;
        while(cp2 != (uint8 idata *)temp)
        {
            *cp2-- = *cp1--;
        }
        OSTaskID++;
        temp = OSTsakStackBotton[OSTaskID] - (uint8 idata *)SP-1;
        SP = (uint8 )OSTsakStackBotton[OSNextTaskID+1];
        for(i = OSNextTaskID+1;i < OSTaskID; i++)
        {
            OSTsakStackBotton[i] += temp;
        }
        OSTaskID = OSNextTaskID;
        SP--;
    }
    LoadCtx();
    LoadCtx();
}
*/

/*********************************************************************************************************
** 函数名称: OSIdle
** 功能描述: 优先级最低的任务
** 输　入: 无
** 输　出 : 无
** 全局变量: 无
** 调用模块: 无
**
** 作　者: 陈明计
** 日　期: 2002年12月2日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**-------------------------------------------------------------------------------------------------------
********************************************************************************************************/

void OSIdle(void)
{
    while (1)
    {
        PCON = PCON | 0x01; /* CPU进入休眠状态 */
    }
}

/*********************************************************************************************************
** 函数名称: OSStart
** 功能描述: Small RTOS 51初始化函数,调用此函数后多任务开始运行,首先执ID为0的任务
** 输　入: 无
** 输　出 : 无
** 全局变量: OSTsakStackBotton,SP
** 调用模块: 无
**
** 作　者: 陈明计
** 日　期: 2002年2月22日
**-------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 日　期: 2002年12月2日
**-------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 日　期: 2002年12月30日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**-------------------------------------------------------------------------------------------------------
********************************************************************************************************/
void OSStart(void)

{
    uint8 idata *cp;
    uint8 i;

    cp = STACK;

    OSTsakStackBotton[0] = STACK;
    OSTsakStackBotton[OS_MAX_TASKS + 1] = (uint8 idata *)(IDATA_RAM_SIZE % 256);

    /* 初始化优先级最高的任务堆栈，使返回地址为任务开始地址 */
    *cp++ = ((uint16)(TaskFuction[0])) % 256;
    SP = (uint8)cp;
    *cp = ((uint16)(TaskFuction[0])) / 256;

    /* 初始化优先级最低的任务堆栈 */
    cp = (uint8 idata *)(IDATA_RAM_SIZE - 1);
    *cp-- = 0;
    *cp-- = ((uint16)(OSIdle)) / 256;
    OSTsakStackBotton[OS_MAX_TASKS] = cp;
    *cp-- = ((uint16)(OSIdle)) % 256;

    /* 初始化其它优先级的任务堆栈 */
    for (i = OS_MAX_TASKS - 1; i > 0; i--)
    {
        *cp-- = 0;
        *cp-- = ((uint16)(TaskFuction[i])) / 256;
        OSTsakStackBotton[i] = cp;
        *cp-- = ((uint16)(TaskFuction[i])) % 256;
    }
    /* 允许中断 */
    Os_Enter_Sum = 1;
    OS_EXIT_CRITICAL();
    /* 函数返回优先级最高的任务 */
}

/*********************************************************************************************************
** 函数名称: OSTickISR
** 功能描述: 系统时钟中断服务函数
** 输　入: 无
** 输　出 : 无
** 全局变量: 无
** 调用模块: OS_IBT_ENTER,(UserTickTimer),OSTimeTick,OSIntExit
**
** 作　者: 陈明计
** 日　期: 2002年2月22日
**-------------------------------------------------------------------------------------------------------
** 修改人: 陈明计
** 日　期: 2002年10月23日
**-------------------------------------------------------------------------------------------------------
** 修改人:
** 日　期:
**-------------------------------------------------------------------------------------------------------
********************************************************************************************************/
#if EN_OS_INT_ENTER > 0
#pragma disable /* 除非最高优先级中断，否则，必须加上这一句                 */
#endif
#if defined(USING_RTOS)
void OSTickISR(void) interrupt OS_TIME_ISR

{
#if TICK_TIMER_SHARING > 1
    static uint8 TickSum = 0;
#endif

#if EN_USER_TICK_TIMER > 0
    UserTickTimer(); /* 用户函数                                                 */
#endif

#if TICK_TIMER_SHARING > 1
    TickSum = (TickSum + 1) % TICK_TIMER_SHARING;
    if (TickSum != 0)
    {
        return;
    }
#endif

#if EN_OS_INT_ENTER > 0
    OS_INT_ENTER(); /* 中断开始处理                                    */
#endif

#if EN_TIMER_SHARING > 0
    OSTimeTick(); /* 调用系统时钟处理函数                            */
#else
    OSIntSendSignal(TIME_ISR_TASK_ID); /* 唤醒ID为TIME_ISR_TASK_ID的任务                 */
#endif

    OSIntExit(); /* 中断结束处理                                             */
}
#endif
