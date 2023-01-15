#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <STC8.H>
#include <stdio.h>
#include <math.h>
#include <intrins.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <absacc.h> //可直接操作内存地址
#include <stdarg.h>
#include <stddef.h>

// #define USING_RTOS
/**********************布尔变量定义**********************/
#define true 1
#define false 0
/**********************布尔变量定义**********************/

/***********************************API配置接口***********************************/
/*使用仿真模式*/
// #define USING_SIMULATE
/*使用外部晶振*/
#if defined(USING_SIMULATE)
#define EXTERNAL_CRYSTAL 0
#else
#define EXTERNAL_CRYSTAL 1
#endif
/*使用自动下载功能*/
#define UAING_AUTO_DOWNLOAD 1
/*EEPROM使用MOVC指令*/
#define EEPROM_USING_MOVC 1
/*flash用做EEPROM尺寸*/
#define EEPROM_SIZE() (64U * 1024U)
/*调试是否启用串口*/
#define USE_PRINTF_DEBUG 0
#if defined(USING_SIMULATE)
/*调试选项*/
#define USING_DEBUG 0
#else
#define USING_DEBUG 1
#endif
#define DEBUGGING 0
/*迪文屏幕使用CRC校验*/
#define USING_CRC 1
/*迪文屏幕使用环形缓冲区接收数据*/
#define DWIN_USING_RB 1
/*modbus协议站使用环形缓冲区接收数据*/
#define MODBUS_USING_RB 0
/*使用ota升级*/
#define USIING_OTA 1
/*OTA升级标志*/
#define OTA_FLAG_VALUE 0x5AA5
#define USING_RGB_LED 1
/*定义WIFI模块相关引脚*/
#define WIFI_RELOADPIN P34
#define WIFI_REDY_PIN P35
#define WIFI_LINK_PIN P36
#define WIFI_RESET_PIN P37

#define LTE_RELOADPIN P06
#define LTE_NET_PIN P07
#define LTE_LINK_PIN P12
#define LTE_RESET_PIN P13

#define IWDG_PIN P55

#define COUNTMAX 65536U

/*消除编译器未使用变量警告*/
#define UNUSED_VARIABLE(x) ((void)(x))
#define UNUSED_PARAMETER(x) UNUSED_VARIABLE(x)

//(1/FOSC)*count =times(us)->count = time*FOSC/1000(ms)
#if defined(USING_SIMULATE)
#define FOSC 24000000UL // 11059200UL
#else
#define FOSC 25000000UL
#endif
/*1ms(时钟频率越高，所能产生的时间越小)*/
#define TIMES 1U
/*定时器模式选择*/
#define TIMER_MODE 12U
/*定时器分频系数，默认为一分频*/
#define TIME_DIV 1U
#define T12_MODE (TIMES * FOSC / 1000 / 12 / TIME_DIV)
#define T1_MODE (TIMES * FOSC / 1000 / TIME_DIV)
#define TIMERS_OVERFLOW (COUNTMAX * 1000 * TIMER_MODE * TIME_DIV) / FOSC
/*串口接收超时时间:ms*/
#define UARTX_OVERTIMES 10

#define OPEN_GLOBAL_OUTAGE() (EA = 1 << 0)
#define CLOSE_GLOBAL_OUTAGE() (EA = 0 << 0)
#define __LOCK_UARTX(x) (x > 1U ? false : (~(ES##x)))
#define __UNLOCK_UARTX(x) (x > 1U ? true : (ES##x))
#define __SET_FLAG(__OBJECT, __BIT) (((__OBJECT) |= 1U << (__BIT)))
#define __RESET_FLAG(__OBJECT, __BIT) (((__OBJECT) &= ~(1U << (__BIT))))
#define __GET_FLAG(__OBJECT, __BIT) (((__OBJECT) & (1U << (__BIT))))
/*判断延时数是否超出硬件允许范围*/
#if (TIMES > TIMERS_OVERFLOW)
#error The timer cannot generate the current duration!
#endif
/***********************************API配置接口***********************************/
#if defined(USING_RGB_LED)
#define LED_R P10
#define LED_G P11
#define LED_B P07
#endif
/***********************************常用的数据类型***********************************/
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short int uint16_t;
typedef int int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef volatile __IO;
typedef bit bool;

/***********************************常用的数据类型***********************************/

/***********************************系统上电参数***********************************/
#if (!EEPROM_USING_MOVC)
#define DEFAULT_SYSTEM_ADDR 0x0000
#else
#define DEFAULT_SYSTEM_ADDR 0xFC00
#endif
#define DEFAULT_SYSTEM_PARAMETER "\xFF\xFF\xFF\xFF\x02\x01\x00\x84\x50"
#define OTA_FLAG_ADDR 0x10000U //(DEFAULT_SYSTEM_ADDR + 0x200U)

/***********************************系统上电参数***********************************/

/***********************************结构体的妙用 ***********************************/
/*获得结构体(TYPE)的变量成员(MEMBER)在此结构体中的偏移量*/
// #define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
/*根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针*/
#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) ); })

#define GET_PARAM_SITE(TYPE, MEMBER, SIZE) (offsetof(TYPE, MEMBER) / sizeof(SIZE))
/***********************************结构体的妙用 ***********************************/

/***********************************中断函数声明区***********************************/
/*-----------------------------------------------------------------------
|                         ISR FUNCTION(Public STC)                      |
-----------------------------------------------------------------------*/

/*--------------------------------------------------------
| @Description: EXTI ISR define                          |
--------------------------------------------------------*/

#define EXTI0_ISRQ_Handler(void) EXTI0_ISR(void) interrupt 0
#define EXTI1_ISRQ_Handler(void) EXTI1_ISR(void) interrupt 2
#define EXTI2_ISRQ_Handler(void) EXTI2_ISR(void) interrupt 10
#define EXTI3_ISRQ_Handler(void) EXTI3_ISR(void) interrupt 11
#define EXTI4_ISRQ_Handler(void) EXTI4_ISR(void) interrupt 16

/*--------------------------------------------------------
| @Description: TIMER ISR define                         |
--------------------------------------------------------*/

#define TIMER0_ISRQ_Handler(void) TIMER0_ISR(void) interrupt 1
#define TIMER1_ISRQ_Handler(void) TIMER1_ISR(void) interrupt 3
#define TIMER2_ISRQ_Handler(void) TIMER2_ISR(void) interrupt 12
#define TIMER3_ISRQ_Handler(void) TIMER3_ISR(void) interrupt 19
#define TIMER4_ISRQ_Handler(void) TIMER4_ISR(void) interrupt 20

/*--------------------------------------------------------
| @Description: UART ISR define                          |
--------------------------------------------------------*/

#define UART1_ISRQ_Handler(void) UART1_ISR(void) interrupt 4 using 0
#define UART2_ISRQ_Handler(void) UART2_ISR(void) interrupt 8 using 1
#define UART3_ISRQ_Handler(void) UART3_ISR(void) interrupt 17 using 2
#define UART4_ISRQ_Handler(void) UART4_ISR(void) interrupt 18 using 3

/*--------------------------------------------------------
| @Description: SPI ISR define                           |
--------------------------------------------------------*/

#define SPI_ISRQ_Handler(void) SPI_ISR(void) interrupt 9

/*--------------------------------------------------------
| @Description: COMP ISR define                          |
--------------------------------------------------------*/

#define COMP_ISRQ_Handler(void) COMP_ISR(void) interrupt 21

/*--------------------------------------------------------
| @Description: I2C ISR define                           |
--------------------------------------------------------*/

#define I2C_ISRQ_Handler(void) I2C_ISR(void) interrupt 24

/*--------------------------------------------------------
| @Description: LVD ISR define                           |
--------------------------------------------------------*/

#define LVD_ISRQ_Handler(void) LVD_ISR(void) interrupt 6

/*-----------------------------------------------------------------------
|                          ISR FUNCTION(STC8x)                          |
-----------------------------------------------------------------------*/

/*--------------------------------------------------------
| @Description: ADC ISR define                           |
--------------------------------------------------------*/

// #if (PER_LIB_MCU_MUODEL == STC8Ax || PER_LIB_MCU_MUODEL == STC8Gx || PER_LIB_MCU_MUODEL == STC8Hx)
#define ADC_ISRQ_Handler(void) ADC_ISR(void) interrupt 5
// #endif

/*--------------------------------------------------------
| @Description: PCA ISR define                           |
--------------------------------------------------------*/

// #if (PER_LIB_MCU_MUODEL == STC8Ax || PER_LIB_MCU_MUODEL == STC8Gx)
#define PCA_ISRQ_Handler(void) PCA_ISR(void) interrupt 7
// #endif

/*--------------------------------------------------------
| @Description: PWM ISR define                           |
--------------------------------------------------------*/

// #if (PER_LIB_MCU_MUODEL == STC8Ax)
#define PWM_ISRQ_Handler(void) PWM_ISR(void) interrupt 22
#define PWM_ABD_ISRQ_Handler(void) PWM_ABD_ISR(void) interrupt 23
// #endif

#define ALIGN_SIZE 4
#define ALIGN_DOWN(size, align) ((size) & ~((align)-1))
extern void assert_handler(const char *ex_string, const char *func, size_t line);
#define ASSERT(EX)                                       \
        if (!(EX))                                       \
        {                                                \
                assert_handler(#EX, __FILE__, __LINE__); \
        }
/***********************************中断函数声明区 ***********************************/

extern const uint8_t g_TimerNumbers;
/***********************************函数声明***********************************/
void Gpio_Init(void);
/***********************************函数声明***********************************/
#endif