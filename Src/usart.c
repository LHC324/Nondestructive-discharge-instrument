#include "usart.h"
#include "GPIO.h"

#if (1 == DWIN_USING_RB)
#include "dwin.h"
#endif

/*********************************************************
* 函数名：
* 功能：
* 参数：
* 作者：LHC
* note：
        同时使用多个串口的时候会出现数据传输错误的情况 建议在使用该板子与其他
        通讯模块建立通讯的时候使用1对1的建立连接的模式

        解决了多串口同时数据传输错误问题 [2021/5/31]

        在切换串口的引脚输入时，建议将RX端初始化的时候给个0值 TX引脚手动给个1值
        （基于STC单片机的特性）

**********************************************************/
// Uart_HandleTypeDef Uart1; //串口1句柄
// Uart_HandleTypeDef Uart2; //串口2句柄
// Uart_HandleTypeDef Uart3; //串口3句柄
// Uart_HandleTypeDef Uart4; //串口4句柄

Uart_HandleTypeDef Uart_Group[4] = {0, 0, 0, 0};
static uint8_t Uart1_Buffer[128], Uart2_Buffer[128], Uart3_Buffer[364], Uart4_Buffer[128];

#define S1BUF SBUF
#define Uartx_CallBack(id)                                             \
    do                                                                 \
    {                                                                  \
        if ((Uart##id).Rx.pbuf &&                                      \
            !__GET_FLAG((Uart##id).Rx.flag, Finish_Flag))              \
        {                                                              \
            __SET_FLAG((Uart##id).Rx.flag, Start_Flag);                \
            (Uart##id).Rx.pbuf[(Uart##id).Rx.rx_count++] = S##id##BUF; \
            (Uart##id).Rx.rx_count %= (Uart##id).Rx.rx_size;           \
            (Uart##id).Rx.over_time = UARTX_OVERTIMES;                 \
        }                                                              \
    } while (false)
#if (!DWIN_USING_RB)
#else
#define _ringbuffer_put(_id, _rb)                             \
    do                                                        \
    {                                                         \
        _rb->buf[_rb->write_index & _rb->size] = S##_id##BUF; \
        if ((_rb->write_index - _rb->read_index) > _rb->size) \
        {                                                     \
            _rb->read_index = _rb->write_index - _rb->size;   \
        }                                                     \
        _rb->write_index++;                                   \
    } while (false)
#endif

#if !defined(USING_SIMULATE)
/*********************************************************
 * 函数名：void Uart_1Init(void)
 * 功能：  串口1的初始化
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器1作为波特率发生器,LAN口用p
 **********************************************************/
void Uart1_Init(uint16_t baud) // 串口1选择定时器1作为波特率发生器
{
    Uart1.Instance = UART1;
    Uart1.Register_SCON = 0x50; // 模式1，8位数据，可变波特率
    Uart1.Uart_Mode = 0x00;     // 定时器模式0，16bit自动重载
    Uart1.Uart_Count = baud;
    Uart1.RunUart_Enable = true;
    Uart1.Interrupt_Enable = true;
    Uart1.Gpio_Switch = false;   // 默认功能引脚切换
    Uart1.Register_AUXR = 0x40;  // 定时器1，1T模式
    Uart1.Register_AUXR &= 0xFE; // 波特率发生器选用定时器1，最好按照要求来

    Uart1.Uart_NVIC.Register_IP = 0xEF; // PS=0,PSH=0,串口1中断优先级为第0级，最低级
    Uart1.Uart_NVIC.Register_IPH = 0xEF;

    Uart1.Rx.flag = false;
    Uart1.Rx.pbuf = Uart1_Buffer;
    Uart1.Rx.rx_size = sizeof(Uart1_Buffer);
    Uart1.Rx.rx_count = 0;
    // Uart1.CallBack = Uartx_CallBack;

    Uart_Base_MspInit(&Uart1);
}

#if (UAING_AUTO_DOWNLOAD)
/**
 * @brief    软件复位自动下载功能，需要在串口中断里调用，
 *           需要在STC-ISP助手里设置下载口令：10个0x7F。
 * @details  Software reset automatic download function,
 *			 need to be called in serial interrupt,
 *			 The download password needs to be
 *			 set in the STC-ISP assistant: 10 0x7F.
 * @param    None.
 * @return   None.
 **/
void Auto_RST_download(void)
{
    static uint8_t semCont = 0;
    if (SBUF == 0x7F || SBUF == 0x80)
    {
        if (++semCont >= 10)
        {
            semCont = 0;
            IAP_CONTR = 0x60;
        }
    }
    else
    {
        semCont = 0;
    }
}
#endif

/*********************************************************
 * 函数名：void Uart1_ISR() interrupt 4 using 0
 * 功能：  串口1的定时中断服务函数
 * 参数：
 * 作者：  LHC
 * note：https://blog.csdn.net/jasper_lin/article/details/41170533
 *		使用的是定时器2作为波特率发生器,485口用
 **********************************************************/
void UART1_ISRQ_Handler() // 串口1的定时中断服务函数
{
    if (TI) // 发送中断标志
    {
        TI = 0;
        Uart1.Uartx_busy = false; // 发送完成，清除占用
    }

    if (RI) // 接收中断标志
    {
        RI = 0;
        // Uart1.Rx.rdata = SBUF;
        // Uart1.CallBack(&Uart1);
#if (UAING_AUTO_DOWNLOAD)
        Auto_RST_download();
#else
        Uartx_CallBack(1);
#endif
    }
}
#endif

/*********************************************************
 * 函数名：void Uart_2Init(void)
 * 功能：  串口2的初始化
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器2作为波特率发生器,485口用
 **********************************************************/
void Uart2_Init(uint16_t baud) // 串口2选择定时器2作为波特率发生器
{
    Uart2.Instance = UART2;
    Uart2.Register_SCON = 0x10; // 模式1，8位数据，可变波特率，开启串口2接收
    Uart2.Uart_Mode = 0x00;     // 定时器模式0，16bit自动重载
    Uart2.Uart_Count = baud;
    Uart2.RunUart_Enable = true;
    Uart2.Interrupt_Enable = 0x01;
    Uart2.Register_AUXR = 0x14;         // 开启定时器2，1T模式
    Uart2.Uart_NVIC.Register_IP = 0x01; // PS2=1,PS2H=0,串口2中断优先级为第1级
    Uart2.Uart_NVIC.Register_IPH = 0xFE;

    Uart2.Rx.flag = false;
    Uart2.Rx.pbuf = Uart2_Buffer;
    Uart2.Rx.rx_size = sizeof(Uart2_Buffer);
    Uart2.Rx.rx_count = 0;
    // Uart2.CallBack = Uartx_CallBack;

    Uart_Base_MspInit(&Uart2);
}

/*********************************************************
 * 函数名：void Uart2_ISR() interrupt 8 using 1
 * 功能：  串口2中断函数
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器2作为波特率发生器,4G口用
 **********************************************************/
void UART2_ISRQ_Handler()
{
    if (S2CON & S2TI) // 发送中断
    {
        S2CON &= ~S2TI;
        Uart2.Uartx_busy = false; // 发送完成，清除占用
    }

    if (S2CON & S2RI) // 接收中断
    {
        S2CON &= ~S2RI;
        Uartx_CallBack(2);
    }
}

///*********************************************************
//* 函数名：void Uart_3Init(void)
//* 功能：  串口3的初始化
//* 参数：
//* 作者：  LHC
//* note：
//*		使用的是定时器3作为波特率发生器,恩外部485转发
//**********************************************************/
void Uart3_Init(uint16_t baud) // 串口3选择定时器3作为波特率发生器
{
    Uart3.Instance = UART3;
    Uart3.Register_SCON = 0x50; // 模式0，8位数据，可变波特率；定时器3，1T模式
                                //  Uart3.Register_SCON = 0xD0; //模式1，9位数据，可变波特率；定时器3，1T模式
    Uart3.Uart_Mode = 0x0A;     // 打开定时器3，1T模式
    Uart3.Uart_Count = baud;
    Uart3.Interrupt_Enable = 0x08;

    Uart3.Rx.flag = false;
    Uart3.Rx.pbuf = Uart3_Buffer;
    Uart3.Rx.rx_size = sizeof(Uart3_Buffer);
    Uart3.Rx.rx_count = 0;
    // Uart3.CallBack = Uartx_CallBack;

    Uart_Base_MspInit(&Uart3);
}

/*********************************************************
 * 函数名：void Uart3_ISR() interrupt 17 using 2
 * 功能：  串口3中断函数
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器3作为波特率发生器,RS485模块
 **********************************************************/
void UART3_ISRQ_Handler()
{
    /*发送中断完成*/
    if (S3CON & S3TI)
    {
        S3CON &= ~S3TI;
        Uart3.Uartx_busy = false; // 发送完成，清除占用
    }
    /*接收中断*/
    if (S3CON & S3RI)
    {
        S3CON &= ~S3RI;
        Uartx_CallBack(3);
    }
}

///*********************************************************
//* 函数名：void Uart_4Init(void)
//* 功能：  串口4的初始化
//* 参数：
//* 作者：  LHC
//* note：
//*		使用的是定时器4作为波特率发生器,PLC口用
//**********************************************************/
void Uart4_Init(uint16_t baud) // 串口4选择定时器4作为波特率发生器
{
    Uart4.Instance = UART4;
    Uart4.Register_SCON = 0x50; // 模式0，8位数据，可变波特率
                                //  Uart4.Register_SCON = 0xD0; //模式1，9位数据，可变波特率
    Uart4.Uart_Mode = 0xA0;     // 定时器模式0，16bit自动重载;开启定时器4，1T模式
    Uart4.Uart_Count = baud;
    Uart4.Interrupt_Enable = 0x10;

    Uart4.Rx.flag = false;
    Uart4.Rx.pbuf = Uart4_Buffer;
    Uart4.Rx.rx_size = sizeof(Uart4_Buffer);
    Uart4.Rx.rx_count = 0;
    // Uart4.CallBack = Uartx_CallBack;

    Uart_Base_MspInit(&Uart4);
}

/*********************************************************
 * 函数名：void Uart4_Isr() interrupt 18 using 3
 * 功能：  串口4中断函数
 * 参数：
 * 作者：  LHC
 * note：
 *		使用的是定时器4作为波特率发生器,PLC口用
 **********************************************************/
void UART4_ISRQ_Handler()
{
#if (DWIN_USING_RB)
    struct ringbuffer *const rb = Dwin_Object.Slave.rb;
#endif
    if (S4CON & S4TI)
    {
        S4CON &= ~S4TI;
        /*发送完成，清除占用*/
        Uart4.Uartx_busy = false;
    }
    /*接收中断*/
    if (S4CON & S4RI)
    {
        S4CON &= ~S4RI;
#if (!DWIN_USING_RB)
        Uartx_CallBack(4);
#else
        // if (Uart4.pbuf && Uart4.rx_count < rx_size)
        //     Uart4.pbuf[Uart4.rx_count++] = S4BUF;
        if (NULL == rb || NULL == rb->buf)
            return;
        _ringbuffer_put(4, rb);

        // rb->buf[rb->write_index & rb->size] = S4BUF;
        // /*
        //  * buffer full strategy: new data will overwrite the oldest data in
        //  * the buffer
        //  */
        // if ((rb->write_index - rb->read_index) > rb->size)
        // {
        //     rb->read_index = rb->write_index - rb->size;
        // }

        // rb->write_index++;
#endif
    }
}

/**********************************公用函数************************/

/*********************************************************
 * 函数名：Uart_Base_MspInit(Uart_HandleTypeDef *uart_baseHandle)
 * 功能：  所有串口底层初始化函数
 * 参数：  Uart_HandleTypeDef *uart_baseHandle串口句柄
 * 作者：  LHC
 * note：
 *		注意正确给出串口句柄
 **********************************************************/
void Uart_Base_MspInit(Uart_HandleTypeDef *const uart_baseHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    switch (uart_baseHandle->Instance)
    {
    case UART1:
    {
        SCON = uart_baseHandle->Register_SCON;
        TMOD |= uart_baseHandle->Uart_Mode;
        TL1 = uart_baseHandle->Uart_Count;
        TH1 = uart_baseHandle->Uart_Count >> 8;
        TR1 = uart_baseHandle->RunUart_Enable;
        AUXR |= uart_baseHandle->Register_AUXR;
        IP &= uart_baseHandle->Uart_NVIC.Register_IP;
        IPH &= uart_baseHandle->Uart_NVIC.Register_IPH;
#if USEING_PRINTF // 如果使用printf
        TI = 1;   // 放到printf重定向
#else
        ES = uart_baseHandle->Interrupt_Enable; // 串口1中断允许位
#endif
        /*设置P3.0为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_0;
        GPIO_Inilize(GPIO_P3, &GPIO_InitStruct);

        /*设置P3.1为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_1;
        GPIO_Inilize(GPIO_P3, &GPIO_InitStruct);
    }
    break;
    case UART2:
    {
        S2CON = uart_baseHandle->Register_SCON;
        TL2 = uart_baseHandle->Uart_Count;
        TH2 = uart_baseHandle->Uart_Count >> 8;
        AUXR |= uart_baseHandle->Register_AUXR;
        IE2 = (uart_baseHandle->Interrupt_Enable & 0x01); // 串口2中断允许位
        IP2 &= uart_baseHandle->Uart_NVIC.Register_IP;
        IP2H &= uart_baseHandle->Uart_NVIC.Register_IPH;
        /*设置P1.0为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_0;
        GPIO_Inilize(GPIO_P1, &GPIO_InitStruct);

        /*设置P1.1为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_1;
        GPIO_Inilize(GPIO_P1, &GPIO_InitStruct);
    }
    break;
    case UART3:
    {
        S3CON = uart_baseHandle->Register_SCON;
        T4T3M = uart_baseHandle->Uart_Mode;
        T3L = uart_baseHandle->Uart_Count;
        T3H = uart_baseHandle->Uart_Count >> 8;
        IE2 |= (uart_baseHandle->Interrupt_Enable & 0x08); // 串口3中断允许位

        /*设置P0.0为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_0;
        GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

        // GPIO_InitStruct.Mode = GPIO_OUT_OD;
        // GPIO_InitStruct.Pin = GPIO_Pin_0;
        // GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

        /*设置P0.1为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_1;
        GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);
    }
    break;
    case UART4:
    {
        S4CON = uart_baseHandle->Register_SCON;
        T4T3M |= uart_baseHandle->Uart_Mode; // 此处串口3和串口4共用
        T4L = uart_baseHandle->Uart_Count;
        T4H = uart_baseHandle->Uart_Count >> 8;
        IE2 |= (uart_baseHandle->Interrupt_Enable & 0x10); // 串口4中断允许位

        /*设置P0.2为准双向口*/
        GPIO_InitStruct.Mode = GPIO_PullUp;
        GPIO_InitStruct.Pin = GPIO_Pin_2;
        GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);

        /*设置P0.3为推挽输出*/
        GPIO_InitStruct.Mode = GPIO_OUT_PP;
        GPIO_InitStruct.Pin = GPIO_Pin_3;
        GPIO_Inilize(GPIO_P0, &GPIO_InitStruct);
    }
    break;
    default:
        break;
    }
}

/*********************************************************
 * 函数名：Uartx_CallBack(Uart_HandleTypeDef *const Uart)
 * 功能：  串口中断通用回调函数
 * 参数：  Uart_HandleTypeDef * const Uart
 * 作者：  LHC
 * note：
 *
 **********************************************************/
// void Uartx_CallBack(Uart_HandleTypeDef *const Uart)
// {
//     if (Uart && Uart->Rx.pbuf &&
//         !__GET_FLAG(Uart->Rx.flag, Finish_Flag))
//     {
//         __SET_FLAG(Uart->Rx.flag, Start_Flag);
//         //        Uart->Rx.pbuf[Uart->Rx.rx_count] = Uart->Rx.rdata;
//         Uart4.Rx.pbuf[Uart->Rx.rx_count] = Uart->Rx.rdata;
//         Uart->Rx.rx_count++;
//         Uart->Rx.rx_count %= Uart->Rx.rx_size;
//         Uart->Rx.over_time = UARTX_OVERTIMES;
//     }
// #if defined(USING_RGB_LED)
//     LED_B ^= true;
// #endif
// }

/*********************************************************
 * 函数名：static void Busy_Await(Uart_HandleTypeDef * const Uart, uint16_t overtime)
 * 功能：  字节发送超时等待机制
 * 参数：  Uart_HandleTypeDef * const Uart;uint16_t overtime
 * 作者：  LHC
 * note：
 *
 **********************************************************/
void Busy_Await(Uart_HandleTypeDef *const Uart, uint16_t overtime)
{

    while (Uart->Uartx_busy) // 等待发送完成：Uart->Uartx_busy清零
    {
        if (!(overtime--))
            break;
    }

    Uart->Uartx_busy = true; // 发送数据，把相应串口置忙
}

/*********************************************************
 * 函数名：Uartx_SendStr(Uart_HandleTypeDef *const Uart,uint8_t *p,uint8_t length)
 * 功能：  所有串口字符串发送函数
 * 参数：  Uart_HandleTypeDef *const Uart,uint8_t *p;uint8_t length
 * 作者：  LHC
 * note：
 *
 **********************************************************/
void Uartx_SendStr(Uart_HandleTypeDef *const Uart, uint8_t *p,
                   uint8_t length, uint16_t time_out)
{
    if (!Uart && !p)
        return;
    while (length--)
    {
        Busy_Await(&(*Uart), time_out); // 等待当前字节发送完成
        switch (Uart->Instance)
        {
#if !defined(USING_SIMULATE)
        case UART1:
            SBUF = *p++;
            break;
#endif
        case UART2:
            S2BUF = *p++;
            break;
        case UART3:
            S3BUF = *p++;
            break;
        case UART4:
            S4BUF = *p++;
            break;
        default:
            break;
        }
    }
}

#if (USING_DEBUG)
/*********************************************************
 * 函数名：char putchar(char str)
 * 功能：  putchar重定向,被printf调用
 * 参数：  char str，发送的字符串
 * 作者：  LHC
 * note：
 *		  使用printf函数将会占用1K 左右FLASH
 **********************************************************/
void Uartx_Printf(Uart_HandleTypeDef *const uart, const char *format, ...)
{
    uint16_t length = 0;
    char uart_buf[256] = {0};
    va_list ap;

    va_start(ap, format);
    /*使用可变参数的字符串打印,类似sprintf*/
    length = vsprintf(uart_buf, format, ap);
    va_end(ap);

    Uartx_SendStr(uart, (uint8_t *)&uart_buf[0], length, UART_BYTE_SENDOVERTIME);
}
#endif
/**********************************公用函数************************/
