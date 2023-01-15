#ifndef __I2C_H
#define __I2C_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "config.h"

    /**
     * @name    GPIOSWPort_Type
     * @brief   外设复用GPIO切换枚举体。
     *          Peripheral IO define.
     * @param   SW_Port1  [uint8_t] 切换第一组IO。 Switch the first group io.
     * @param   SW_Port2  [uint8_t] 切换第二组IO。 Switch the second group io.
     * @param   SW_Port3  [uint8_t] 切换第三组IO。 Switch the third group io.
     * @param   SW_Port4  [uint8_t] 切换第四组IO。 Switch the fouth group io.
     ***/
    typedef enum
    {
        SW_Port1 = 0x00,
        SW_Port2 = 0x01,
        SW_Port3 = 0x02,
        SW_Port4 = 0x03
    } GPIOSWPort_Type;

    /**
     * @name    NVICPri_Type
     * @brief   中断管理优先级枚举体。
     *          Interrupt management priority enumeration body.
     * @param   NVIC_PR0  [uint8_t] 优先级0。 Priority 0.
     * @param   NVIC_PR1  [uint8_t] 优先级1。 Priority 1.
     * @param   NVIC_PR2  [uint8_t] 优先级2。 Priority 2.
     * @param   NVIC_PR3  [uint8_t] 优先级3。 Priority 3.
     ***/
    typedef enum
    {
        NVIC_PR0 = 0x00, // Lowest  priority |
        NVIC_PR1 = 0x01, // Lower   priority |
        NVIC_PR2 = 0x02, // Higher  priority |
        NVIC_PR3 = 0x03  // Highest priority V
    } NVICPri_Type;

    /**
     * @brief     SPI工作类型枚举体。
     * @details   SPI type control enum.
     **/
    typedef enum
    {
        SPI_Type_Master_Slave = 0x00, /*!< SPI互为主从机模式。SPI is mutually master-slave mode. */
        SPI_Type_Master = 0x90,       /*!< SPI互为主机模式。SPI is mutually master mode. */
        SPI_Type_Slave = 0x80,        /*!< SPI互为主机模式。SPI is mutually slave mode. */
    } SPIType_Type;

    /**
     * @brief     SPI时钟源选择枚举体。
     * @details   SPI clock source select enum.
     **/
    typedef enum
    {
        SPI_SCLK_DIV_4 = 0x00,  /*!< SPI系统时钟源4分频。SPI system clock source divided by 4. */
        SPI_SCLK_DIV_8 = 0x01,  /*!< SPI系统时钟源8分频。SPI system clock source divided by 8. */
        SPI_SCLK_DIV_16 = 0x02, /*!< SPI系统时钟源16分频。SPI system clock source divided by 16. */
        SPI_SCLK_DIV_32 = 0x03  /*!< SPI系统时钟源32分频。SPI system clock source divided by 32. */
    } SPIClkSrc_Type;

    /**
     * @brief     SPI传输类型枚举体。
     * @details   SPI Transmission sequence enum.
     **/
    typedef enum
    {
        SPI_Tran_MSB = 0x00, /*!< 数据的最高位存放在字节的第0位。
                                  The highest bit of the data is stored in the 0th bit of the byte. */
        SPI_Tran_LSB = 0x01  /*!< 数据的最低位存放在字节的第0位。
                                  The lowest bit of the data is stored in the 0th bit of the byte. */
    } SPITran_Type;

    /**
     * @brief     SPI模式枚举体。
     * @details   SPI mode control enum .
     **/
    typedef enum
    {
        SPI_Mode_0 = 0x00, /*!< 模式0。mode 0. */
        SPI_Mode_1 = 0x01, /*!< 模式1。mode 1. */
        SPI_Mode_2 = 0x02, /*!< 模式2。mode 2. */
        SPI_Mode_3 = 0x03  /*!< 模式3。mode 3. */
    } SPIMode_Type;

    /**
     * @brief   SPI初始化枚举体，需要你定义它，并用它的地址来传参给初始化函数。
     * @details SPI initializes the enumeration body, you need to define
     *          it and use its address to pass parameters to the initialization function.
     **/
    typedef struct
    {
        SPIType_Type Type;     /*!< SPI工作类型。SPI working type. */
        SPIClkSrc_Type ClkSrc; /*!< SPI时钟源。SPI clock source. */
        SPIMode_Type Mode;     /*!< SPI工作模式。SPI working mode. */
        SPITran_Type Tran;     /*!< SPI传输类型。SPI transmission type. */
        uint8_t Run;           /*!< SPI运行控制位。SPI operation control bit. */
    } SPIInit_Type;

    /**
     * @brief     SPI初始化函数。
     * @details   SPI initialization function.
     * @param[in] spix  SPI初始化结构体句柄，你需要定义它，并其地址传参。
     *            you need to definean associated initialization handle,
     *            And pass it by its address.
     * @return    FSC_SUCCESS 返回成功。Return to success.
     * @return    FSC_FAIL    返回失败。Return to fail.
     **/
    extern void spi_init(const SPIInit_Type *spix);

    /**
     * @brief     SPI发送数据（一个字节）函数。
     * @details   SPI send data function.
     * @param[in] dat   要发送的数据。 data of SPI.
     * @return    FSC_SUCCESS 返回成功。Return to success.
     * @return    FSC_FAIL    返回失败。Return to fail.
     **/
    extern void spi_send_data(uint8_t dat, uint16_t timeout);

    /**
     * @brief     SPI接收数据（一个字节）函数。
     * @details   SPI receive data function.
     * @param     None.
     * @return    [uint8_t] 接收的数据。 receive data.
     **/
    extern uint8_t spi_receive_data(uint16_t timeout);

    /**
     * @brief     SPI中断初始化函数。
     * @details   SPI init NVIC function.
     * @param[in] pri 中断优先级。interrupt priority.
     * @param[in] run 使能控制位。enable control.
     * @return    FSC_SUCCESS 返回成功。Return to success.
     * @return    FSC_FAIL    返回失败。Return to fail.
     **/
    extern void NVIC_spi_init(NVICPri_Type pri, uint8_t run);

    /**
     * @brief     SPI切换复用IO函数。
     * @details   SPI switch out port control function.
     * @param[in] port 复用IO枚举体。IO switch enumerator.
     * @return    FSC_SUCCESS 返回成功。Return to success.
     * @return    FSC_FAIL    返回失败。Return to fail.
     **/
    extern void GPIO_spi_sw_port(GPIOSWPort_Type port);

    /**
     * @brief     SPI中断开关控制宏函数。
     * @details   SPI interrupt switch control macro function.
     * @param[in] run  使能控制位。Enable control bit.
     **/
#define NVIC_SPI_CTRL(run)               \
    do                                   \
    {                                    \
        IE2 = (IE2 & 0xFD) | (run << 1); \
    } while (0)

/**
 * @brief      SPI选择中断优先级宏函数。
 * @details    SPI select interrupt pri macro function.
 * @param[in]  pri 中断优先级。 pri of interrupt.
 **/
#define NVIC_SPI_PRI(pri)                         \
    do                                            \
    {                                             \
        IP2H = (IP2H & 0xFD) | (pri & 0x02);      \
        IP2 = (IP2 & 0xFD) | ((pri & 0x01) << 1); \
    } while (0)

/**
 * @brief   SPI获取中断标志位宏函数。
 * @details SPI gets the interrupt flag macro function.
 **/
#define SPI_GET_FLAG() (SPSTAT & 0x80)

/**
 * @brief   SPI清除中断标志位宏函数。
 * @details SPI clears the interrupt flag macro function.
 **/
#define SPI_CLEAR_FLAG() \
    do                   \
    {                    \
        SPSTAT = 0xC0;   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H */