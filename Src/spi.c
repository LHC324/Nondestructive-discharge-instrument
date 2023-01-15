#include "spi.h"

// bit spi_busy = false;

// void SPI_ISRQ_Handler(void)
// {
//     if (SPSTAT & SPIF)
//     {
//         SPSTAT = 0xC0; // Clear interrupt flag
//         P22 = 1;
//         spi_busy = false;
//     }
// }

/**
 * @brief     SPI初始化函数。
 * @details   SPI initialization function.
 * @param[in] spix  SPI初始化结构体句柄，你需要定义它，并其地址传参。
 *            you need to definean associated initialization handle,
 *            And pass it by its address.
 * @return    FSC_SUCCESS 返回成功。Return to success.
 * @return    FSC_FAIL    返回失败。Return to fail.
 **/
void spi_init(const SPIInit_Type *spix)
{
    if (NULL == spix)
        return;

    SPDAT = 0;
    SPSTAT = SPIF | WCOL;
    SPCTL = (SPCTL & 0x6F) | (spix->Type);
    SPCTL = (SPCTL & 0xFC) | (spix->ClkSrc);
    SPCTL = (SPCTL & 0xF3) | (spix->Mode << 2);
    SPCTL = (SPCTL & 0xDF) | (spix->Tran << 5);
    SPCTL = (SPCTL & 0xBF) | (spix->Run << 6);
}

/**
 * @brief     SPI发送数据（一个字节）函数。
 * @details   SPI send data function.
 * @param[in] dat   要发送的数据。 data of SPI.
 * @return    FSC_SUCCESS 返回成功。Return to success.
 * @return    FSC_FAIL    返回失败。Return to fail.
 **/
void spi_send_data(uint8_t dat, uint16_t timeout)
{
    // spi_busy = true;
    SPDAT = dat; // Data register assignment
    while (timeout--)
    {
        if (SPSTAT & 0x80)
        {
            // Query completion flag
            SPSTAT = 0xC0; // Clear interrupt flag
            break;
        }
    }

    // while (spi_busy && timeout--)
    //     ;
}

/**
 * @brief     SPI接收数据（一个字节）函数。
 * @details   SPI receive data function.
 * @param     None.
 * @return    [uint8_t] 接收的数据。 receive data.
 **/
uint8_t spi_receive_data(uint16_t timeout)
{
    while (timeout--)
    {
        if (SPSTAT & 0x80)
        {
            SPSTAT = 0xC0; // Clear interrupt flag
            break;
        }

    }; // Query completion flag

    // spi_busy = true;
    // while (spi_busy && timeout--)
    //     ;

    return SPDAT; // Data register assignment
}

/**
 * @brief     SPI中断初始化函数。
 * @details   SPI init NVIC function.
 * @param[in] pri 中断优先级。interrupt priority.
 * @param[in] run 使能控制位。enable control.
 * @return    FSC_SUCCESS 返回成功。Return to success.
 * @return    FSC_FAIL    返回失败。Return to fail.
 **/
void NVIC_spi_init(NVICPri_Type pri, uint8_t run)
{
    NVIC_SPI_PRI(pri);
    IE2 = (IE2 & 0xFD) | (run << 1);
}

/**
 * @brief     SPI切换复用IO函数。
 * @details   SPI switch out port control function.
 * @param[in] port 复用IO枚举体。IO switch enumerator.
 * @return    FSC_SUCCESS 返回成功。Return to success.
 * @return    FSC_FAIL    返回失败。Return to fail.
 **/
void GPIO_spi_sw_port(GPIOSWPort_Type port)
{
    P_SW1 = (P_SW1 & 0xF3) | (port << 2);
}
