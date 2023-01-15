#include "OS_CONFIG.h"
#include "GPIO.h"
#include "timer.h"
#include "usart.h"
#include "spi.h"
#include "w25qx.h"
// #include "charger.h"
#include "discharger.h"
#include "dwin.h"
#include "wifi.h"
#include "eeprom.h"
#include "Modbus.h"

#if (DEBUGGING == 1)
void HardDefault(uint8_t channel, uint8_t error);
#endif

#define SYS_SOFT_VERSION "1.5.5"
#if !defined(USING_RTOS)
// void Event_Init(TIMERS *ptimer);
// static void Event_Polling(void);
static void Event_Polling(TIMERS *ptimer);
static void Debug_Task(void);
static void Uartx_Parser(void);
static void Master_Task(void);
static void DisTimer_Task(void);
static void Report_Task(void);

/*禁止编译器优化该模块*/
#pragma OPTIMIZE(0)

TIMERS Timer_Group[] = {
#if (DEBUGGING == 1)
    {0, 100, true, false, Debug_Task}, /*调试接口（3s）*/
#else
    {0, 100, true, false, Uartx_Parser},   /*通用串口数据接收解析器(10ms)*/
    {0, 1500, true, false, Report_Task},   /*上报放电仪数据到迪文屏幕（1.0s）*/
    {0, 1000, true, false, Master_Task},   /*主动请求放电仪数据（0.5S）*/
    {0, 1000, true, false, DisTimer_Task}, /*放电数据统计（0.5S）*/

#endif
};
/*获得当前软件定时器个数*/
const uint8_t g_TimerNumbers = sizeof(Timer_Group) / sizeof(TIMERS);
#endif

/**
 * The ASSERT function.
 *
 * @param ex_string is the assertion condition string.
 *
 * @param func is the function name when assertion.
 *
 * @param line is the file line number when assertion.
 */
void assert_handler(const char *ex_string, const char *func, size_t line)
{
    volatile char dummy = 0;

    // if (rt_assert_hook == RT_NULL)
    {
#if (USING_DEBUG)
        Uartx_Printf(&Uart1, "(%s) assertion failed at file:%s, line:%d \n",
                     ex_string, func, line);
#endif
        while (dummy == 0)
        {
            IAP_CONTR = 0x60; // 复位单片机
        }
    }
    // else
    {
    }
}

void main(void)
{
#if (USING_DEBUG)
    uint32_t jedec_id;
    uint8_t *id = &jedec_id, regs[] = {0, 0};
#endif
    uint16_t Crc = 0;
    Storage_TypeDef *ps = &discharger.Storage;
    // pModbusHandle pm = &Modbus_Object;
    Storage_TypeDef dis_storage = {0x01, 120, 220, 220, 1, 15, 0x02, 1001, 6666, 0xB59C};

    /*设置WiFi芯片复引脚：不复位会导致连接不上云平台*/
    // WIFI_RESET = 0;
    // Delay_Ms(200);
    // WIFI_RESET = 1;
    // WIFI_RELOAD = 1;
    /*初始化引脚*/
    Gpio_Init();
    /*定时器0初始化*/
    Timer0_Init();
#if !defined(USING_SIMULATE)
    /*串口1初始化*/
    Uart1_Init(BRT_1T(BAUD_115200)); // Wifi
#endif
    /*串口2初始化*/
    Uart2_Init(BRT_1T(BAUD_115200)); // 4G
    Uart3_Init(BRT_1T(BAUD_9600));   // RS485
    Uart4_Init(BRT_1T(BAUD_115200)); // Dwin
    /*WiFi初始化*/
    //	Wifi_Init();
    /*读出默认放电参数*/
    Iap_Reads(DEFAULT_SYSTEM_ADDR, (uint8_t *)ps, sizeof(Storage_TypeDef));
    Crc = Get_Crc16((uint8_t *)ps, sizeof(Storage_TypeDef) - sizeof(ps->Crc), 0xFFFF);
    if (ps->Crc != Crc)
    {
        // Uartx_Printf(&Uart2, "Initialization parameters:ps->Crc[%#X], Crc[%#X].\r\n");
        memcpy(ps, &dis_storage, sizeof(discharger.Storage));
        ps->Crc = Get_Crc16((uint8_t *)&dis_storage, sizeof(Storage_TypeDef) - sizeof(dis_storage.Crc), 0xFFFF);
        IapWrites(DEFAULT_SYSTEM_ADDR, (uint8_t *)ps, sizeof(Storage_TypeDef));
    }

    //     /*数据写回保持寄存器*/
    //     pm->Slave.Reg_Type = HoldRegister;
    //     pm->Slave.Operate = Write;
    //     /*读取对应寄存器*/
    //     if (!Modbus_Operatex(pm, 0x00, (uint8_t *)ps, sizeof(Storage_TypeDef)))
    //     {
    // #if defined(USING_DEBUG)
    //         Debug("Coil reading failed!\r\n");
    //         return;
    // #endif
    //     }

#if !defined(USING_RTOS)
    /*开总中断*/
    OPEN_GLOBAL_OUTAGE();
#else
    OSStart(); // 启动操作系统
#endif
    // /*上报后台参数*/
    // Dwin_Write(&Dwin_Objct, SLAVE_ID_ADDR, (uint8_t *)&(discharger.Storage),
    //            GET_PARAM_SITE(Storage_TypeDef, flag, uint8_t));

#if (USING_DEBUG)
    jedec_id = dev_flash_read_jedec_id();
    regs[0] = dev_flash_read_sr(FLASH_READ_SR1_CMD);
    regs[1] = dev_flash_read_sr(FLASH_READ_SR2_CMD);
    Uartx_Printf(&Uart2, "\r\nsystem vesrsion:%s\r\nJEDEC: %#bx  %#bx  %#bx\r\nregs(1~0): %#bx  %#bx\r\n",
                 SYS_SOFT_VERSION, id[1], id[2], id[3], regs[1], regs[0]);
#endif

    while (1)
    {
#if !defined(USING_RTOS)
        ET0 = 0;
        Event_Polling(Timer_Group);
        ET0 = 1;
#endif
        /*喂硬件看门狗*/
        IWDG_PIN ^= true;
    }
}

#if defined(USING_RTOS)
void TaskA(void)
{
    // for (;;)
    while (1)
    {
        OSWait(K_TMO, 100); // K_SIG
        // Uartx_Printf(&Uart1, "Hello world.\r\n");
        // Uartx_Printf(&Uart2, "Hello world.\r\n");
        // Uartx_Printf(&Uart3, "Hello world.\r\n");
        // Uartx_Printf(&Uart4, "Hello world.\r\n");
        IWDG_PIN ^= 1;
    }
}

#else
/**
 * @brief	任务组时间片调度
 * @details	按照指定时间片和执行标志调度任务
 * @param	None
 * @retval	None
 */
/*变量出现问题，加static*/
// #pragma disable
void Event_Polling(TIMERS *ptimer)
{
    TIMERS *p = ptimer;
    if (ptimer)
    {
        for (; p < ptimer + g_TimerNumbers; p++)
        {
            if (p->execute_flag == true)
            {
                p->ehandle();
                p->execute_flag = false;
            }
        }
    }
}

#if (DEBUGGING == 1)
/**
 * @brief	调试任务
 * @details	调试阶段输出调试信息
 * @param	None
 * @retval	None
 */
void Debug_Task(void)
{
    data uint8_t c = 0;

    for (c = 0; c < CHANNEL_MAX; c++)
    {
        Uartx_Printf(&Uart1, "channel %bd :STATU is 0x%x\r\n", c, (uint8_t)g_Sc8913_Registers[c][STATUS_ADDR]);
        Uartx_Printf(&Uart1, "channel %bd :VBUS is %f\r\n", c, READ_VBUS_VALUE(c, VBUS_FB_VALUE_ADDR));
        Uartx_Printf(&Uart1, "channel %bd :VBAT is %f\r\n", c, READ_VBAT_VALUE(c, VBAT_FB_VALUE_ADDR));
        Uartx_Printf(&Uart1, "channel %bd :IBUS is %f\r\n", c, READ_IBUS_VALUE(c, IBUS_VALUE_ADDR));
        Uartx_Printf(&Uart1, "channel %bd :IBAT is %f\r\n", c, READ_IBAT_VALUE(c, IBAT_VALUE_ADDR));
    }
}
#endif

#define CLEAR_UARTX_BUFFER(Uart)                        \
    do                                                  \
    {                                                   \
        memset(Uart->Rx.pbuf, 0x00, Uart->Rx.rx_count); \
        Uart->Rx.rx_count = 0;                          \
    } while (false)

// void Delay20ms() //@27.000MHz
// {
//     unsigned char i, j, k;

//     _nop_();
//     _nop_();
//     i = 3;
//     j = 14;
//     k = 67;
//     do
//     {
//         do
//         {
//             while (--k)
//                 ;
//         } while (--j);
//     } while (--i);
// }

/**
 * @brief	串口接收通用数据帧解析器
 * @details
 * @param	None
 * @retval	None
 */
void Uartx_Parser(void)
{
    Uart_HandleTypeDef *puart = Uart_Group;

    for (; puart && (puart < Uart_Group + UART_GROUP_SIZE()); puart++)
    {
        /*迪文屏幕使用ringbuf*/
#if (DWIN_USING_RB)
        if (UART4 == puart->Instance)
        {
            // IE2 &= ~ES4;
            Dwin_Object.Slave.rb->size = (puart->Rx.rx_size - 1U);
            Dwin_Object.Slave.rb->buf = puart->Rx.pbuf;
            // ringbuffer_put(Dwin_Object.Slave.rb, puart->Rx.pbuf, puart->Rx.rx_count);
            Dwin_Poll(&Dwin_Object);
            // IE2 |= ES4;
        }
#endif
#if (USIING_OTA)
        if (UART2 == puart->Instance)
        {
            // IE2 &= ~ES2;
            Modbus_Object.Slave.rb->buf = puart->Rx.pbuf;
            Modbus_Object.Slave.rb->size = puart->Rx.rx_size - 1U;
            Modbus_Object.Mod_Poll(&Modbus_Object);
            // IE2 |= ES2;
        }
#endif
        if (__GET_FLAG(puart->Rx.flag, Finish_Flag))
        {
            __RESET_FLAG(puart->Rx.flag, Finish_Flag);
            switch (puart->Instance)
            {
#if !defined(USING_SIMULATE)
            case UART1:
            {
#define ES1 NULL
                ES = 0;
                ES = 1;
                // Uartx_Printf(&Uart1, "uart1.____________________________\r\n");
            }
            break;
#endif
            case UART2:
            {
                if (Modbus_Object.Ota_Flag)
                    return;
                IE2 &= ~ES2;
                Modbus_Object.Slave.pRbuf = puart->Rx.pbuf;
                Modbus_Object.Slave.RxCount = puart->Rx.rx_count;
                Modbus_Object.Mod_Poll(&Modbus_Object);
                IE2 |= ES2;
                // Uartx_Printf(&Uart2, "uart2.____________________________\r\n");
            }
            break;
            case UART3:
            {
                IE2 &= ~ES3;
                Discharger_Handle(&discharger, puart);
                IE2 |= ES3;
                // Uartx_Printf(&Uart3, "uart3.____________________________\r\n");
            }
            break;
#if (!DWIN_USING_RB)
            case UART4:
            {
                IE2 &= ~ES4;
                // memcpy(Dwin_Objct.Slave.pRbuf, puart->Rx.pbuf, puart->Rx.rx_count);
                Dwin_Object.Slave.pRbuf = puart->Rx.pbuf;
                Dwin_Object.Slave.RxCount = puart->Rx.rx_count;
                Dwin_Poll(&Dwin_Object);
                IE2 |= ES4;
                // Delay20ms();
                // Uartx_Printf(&Uart4, "uart4.____________________________\r\n");
            }
            break;
#endif
            default:
                break;
            }
            // Uartx_SendStr(puart, puart->Rx.pbuf, puart->Rx.rx_count, UART_BYTE_SENDOVERTIME);
            CLEAR_UARTX_BUFFER(puart);
        }
    }
}

/**
 * @brief	主动请求放电仪数据
 * @details
 * @param	None
 * @retval	None
 */
void Master_Task(void)
{
    Discharger_TypeDef *ps = &discharger;
    uint8_t data_buf[50] = {
        0x16, 0x03, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x17, 0x0A, 0x1A, 0x0F, 0x1E, 0x19, 0x22, 0x23, 0x26,
        0x34, 0x2A, 0x46, 0x2E, 0x5A, 0x32, 0x7D, 0x34, 0xBE, 0x35,
        0x3C, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x0F, 0x01, 0x00,
        0xDC, 0xDC, 0xDC, 0xFF, 0xFF, 0xFF, 0x16, 0x08, 0x02, 0xFA};
    /*收到参数保存命令*/
    if (__GET_FLAG(ps->Storage.flag, Save_Flag))
    {
        __RESET_FLAG(ps->Storage.flag, Save_Flag);
        data_buf[0] = 0x15;
    }
    else
        data_buf[0] = 0x16;

    Set_DischargerParam(ps, data_buf);
    Uartx_SendStr(&Uart3, data_buf, sizeof(data_buf), UART_BYTE_SENDOVERTIME);
}

/**
 * @brief	放电数据统计
 * @details
 * @param	None
 * @retval	None
 */
void DisTimer_Task(void)
{
    static uint32_t DisCharging_Times = 0;
    static float DisCharging_Quantity = 0;
    Discharger_TypeDef *pd = &discharger;

    if (__GET_FLAG(pd->Current.M_State, Work))
    {
        DisCharging_Times += 1U;
        /*统计充电电量*/
        DisCharging_Quantity += pd->Current.I_Discharger;
    }
    if (__GET_FLAG(pd->Current.M_State, Standy))
    {
        DisCharging_Times = 0U;
        /*统计充电电量*/
        DisCharging_Quantity = 0;
    }

    pd->Current.T_Discharger = __Get_ChargingTimes(DisCharging_Times);
    // pd->Current.Q_Discharger = __Get_ChargingQuantity(DisCharging_Quantity);
}

#define __Mod_OprateReg(pm, pd, reg_type, opr_type, src_dat)                     \
    {                                                                            \
        pm->Slave.Reg_Type = reg_type;                                           \
        pm->Slave.Operate = Write;                                               \
        Modbus_Operatex(pm, 0x00, (uint8_t *)&pd->Current, sizeof(pd->Current)); \
    }
/**
 * @brief	迪文屏幕数据上报任务
 * @details	轮询模式
 * @param	None
 * @retval	None
 */
void Report_Task(void)
{
    Discharger_TypeDef *pd = &discharger;
    pModbusHandle pm = &Modbus_Object;
    uint8_t state = 0;

    /*数据写回输入寄存器*/
    pm->Slave.Reg_Type = InputRegister;
    pm->Slave.Operate = Write;
    if (!Modbus_Operatex(pm, 0x00, (uint8_t *)&pd->Current, sizeof(pd->Current)))
    {
#if defined(USING_DEBUG)
//        Debug("Coil reading failed!\r\n");
#endif
        return;
    }
    /*数据写回保持寄存器*/
    pm->Slave.Reg_Type = HoldRegister;
    pm->Slave.Operate = Write;
    if (!Modbus_Operatex(pm, 0x00, (uint8_t *)&pd->Storage, sizeof(pd->Storage)))
    {
#if defined(USING_DEBUG)
//        Debug("Coil reading failed!\r\n");
#endif
        return;
    }
    /*工作模式写回线圈*/
    pm->Slave.Reg_Type = Coil;
    pm->Slave.Operate = Write;
    state = __GET_FLAG(pd->Storage.flag, P_Limit_Enable) ? 0xFF : 0x00;

    if (!Modbus_Operatex(pm, 0x00, (uint8_t *)&state, sizeof(state)))
        return;
    /*上报前台数据*/
    Dwin_Write(&Dwin_Object, V_BATTERY_ADDR, (uint8_t *)&(pd->Current), sizeof(Current_TypeDef));
    /*上报放电状态、放电动画和仪器状态*/
}

#if (DEBUGGING == 1)
/**
 * @brief	硬件错误检测
 * @details	轮询模式
 * @param	None
 * @retval	None
 */
void HardDefault(uint8_t channel, uint8_t error)
{
    Uartx_Printf(&Uart1, "\r\nChannel %bd product a error:%bd\r\n", channel, error);
}
#endif

#endif

/************************************外设初始化************************************/
/**
 * @brief	GPIO初始化
 * @details	初始化对应的外设引脚
 * @param	None
 * @retval	None
 */
void Gpio_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    //    #ifdef EXTERNAL_CRYSTAL //只要有宏名，就成立
#if EXTERNAL_CRYSTAL
    P_SW2 = 0x80;
    XOSCCR = 0xC0;
    /*启动外部晶振11.0592MHz*/
    while (!(XOSCCR & 1))
        ;
    /*时钟不分频*/
    CLKDIV = 0x01;
    /*选择外部晶振*/
    CKSEL = 0x01;
    P_SW2 = 0x00;
#endif
    //  P_SW1 = 0xC0; //串口1切换到P4.3、4.4(P0.2、0.3)
    //	P_SW2 |= 0x01; //串口2切换到P4.0、4.2(P1.0、1.1)（新板子引脚问题）

    /*手册提示，串口1、2、3、4的发送引脚必须设置为强挽输出*/
    /**/

#if !defined(USING_SIMULATE)
#define USING_WIFI______________________________________
    {
#define WIFI_RELORAD GPIO_Pin_4
#define WIFI_READY GPIO_Pin_5
#define WIFI_LINK GPIO_Pin_6
#define WIFI_RESET GPIO_Pin_7
#define WIFI_GPIO_PORT GPIO_P3

        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = WIFI_RELORAD | WIFI_RESET;
        GPIO_Inilize(WIFI_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Mode = GPIO_HighZ;
        GPIO_InitStruct.Pin = WIFI_READY | WIFI_LINK;
        GPIO_Inilize(WIFI_GPIO_PORT, &GPIO_InitStruct);
    }
#endif

#define USING_W25QX______________________________________
    {
#define W25QX_NSS GPIO_Pin_2
#define W25QX_MOSI GPIO_Pin_3
#define W25QX_MISO GPIO_Pin_4
#define W25QX_CLK GPIO_Pin_5
#define W25QX_PORT GPIO_P2

        SPIInit_Type spi = {
            SPI_Type_Master,
            SPI_SCLK_DIV_16,
            SPI_Mode_0,
            SPI_Tran_MSB,
            true,
        };

        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = W25QX_NSS | W25QX_CLK | W25QX_MOSI;
        GPIO_Inilize(W25QX_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Mode = GPIO_HighZ;
        GPIO_InitStruct.Pin = W25QX_MISO;
        GPIO_Inilize(W25QX_PORT, &GPIO_InitStruct);

        GPIO_spi_sw_port(SW_Port2);
        // NVIC_spi_init(NVIC_PR3, true);
        spi_init(&spi);
    }

#define USING_LTE______________________________________
    {
#define LTE_RELORAD GPIO_Pin_6
#define LTE_NET GPIO_Pin_7
#define LTE_LINK GPIO_Pin_2
#define LTE_RESET GPIO_Pin_3
#define LTE_GPIO_PORT0 GPIO_P0
#define LTE_GPIO_PORT1 GPIO_P1

        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = LTE_RELORAD;
        GPIO_Inilize(LTE_GPIO_PORT0, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = LTE_RESET;
        GPIO_Inilize(LTE_GPIO_PORT1, &GPIO_InitStruct);

#if (USING_RGB_LED)
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
#else
        GPIO_InitStruct.Mode = GPIO_HighZ;

#endif
        GPIO_InitStruct.Pin = LTE_NET;
        GPIO_Inilize(LTE_GPIO_PORT0, &GPIO_InitStruct);
        GPIO_InitStruct.Mode = GPIO_HighZ;
        GPIO_InitStruct.Pin = LTE_LINK;
        GPIO_Inilize(LTE_GPIO_PORT1, &GPIO_InitStruct);
    }

#define USING_IWDG______________________________________
    {
#define IWDG GPIO_Pin_5
#define IWDG_PORT GPIO_P5

        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = IWDG;
        GPIO_Inilize(IWDG_PORT, &GPIO_InitStruct);
    }
#if (USING_RGB_LED)
    GPIO_InitStruct.Mode = GPIO_OUT_PP;
    GPIO_InitStruct.Pin = GPIO_Pin_7;
    GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);
#endif
}
/************************************外设初始化************************************/
