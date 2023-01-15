/********************************** (C) COPYRIGHT *******************************
 * File Name          : dev_flash.c
 * Author             : XZH
 * Version            : V1.00
 * Date               : 2020/07/20
 * Description        : w25qxx驱动
 *
 *
 *******************************************************************************/

/*******************************************************************************/
/* Include */
#include "w25qx.h"
#include "spi.h"
#include "usart.h"
/*******************************************************************************/

#if (!USING_DEBUG && W25Qx_USING_DEBUG)
#error Global debugging mode is not turned on!
#endif

#define SPI_TX_TIMEOUT 200 // 400
#define SPI_RX_TIMEOUT 200 // 1000

/*******************************************************************************/
/* Variables */
/*******************************************************************************/

/*******************************************************************************/
/* Globle Variate */
/******************************************************************************/

/******************************************************************************/
/* Static Function  */

/*******************************************************************************
 * Function Name  : dev_flash_delay
 * Description    : flash驱动使用的delay
 *                  平台适配
 *
 * Input          : delay_time 延迟时间 注意最大只能8位这里设置了
 *
 *
 * Output         : None
 * Return         : None
 *******************************************************************************/
static void dev_flash_delay(uint8_t delay_time)
{
    unsigned char i, j;

    i = 4U * delay_time; //@24.000MHz
    j = 225U;
    do
    {
        while (--j)
            ;
    } while (--i);
}

/*******************************************************************************
 * Function Name  : dev_flash_read_write_byte
 * Description    : 全双工读写函数 平台适配
 *                  平台适配
 *
 * Input          : tx_data 8位数据 这里注意适配一下retry超时时间，不能太短，否则会直接跳出导致数据错误
 *
 *
 * Output         : None
 * Return         : None
 *******************************************************************************/
static uint8_t dev_flash_read_write_byte(bit rx_en, uint8_t tx_data)
{
    spi_send_data(tx_data, SPI_TX_TIMEOUT);

    if (rx_en)
        return spi_receive_data(SPI_RX_TIMEOUT);

    return 0xFF;
}

/*******************************************************************************
 * Function Name  : dev_flash_send_bytes
 * Description    : 发送多个数据
 *
 *
 * Input          : pdata 数据指针
 *                  send_length  16位数据长度
 *
 * Output         : None
 * Return         : None
 *******************************************************************************/
static void dev_flash_send_bytes(uint8_t *pdat, uint16_t len)
{
    while (len--)
    {
        dev_flash_read_write_byte(false, *pdat++);
    }
}

/*******************************************************************************
 * Function Name  : dev_flash_recv_bytes
 * Description    : spi读取多次数据 这里发送没用数据0xff用于给时钟
 *
 *
 * Input          : recv_length  16位数据长度
 *
 *
 * Output         : pdata 读取到buf的数据指针
 * Return         : None
 *******************************************************************************/
static void dev_flash_recv_bytes(uint8_t *pdat, uint16_t len)
{
    while (len--)
    {
        *pdat++ = dev_flash_read_write_byte(true, 0xff);
    }
}

/*******************************************************************************
 * Function Name  : dev_flash_write_enable
 * Description    : 写数据之前必须发写使能
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : None
 *******************************************************************************/
static void dev_flash_write_enable(void)
{
    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, FLASH_WRITE_ENABLE_CMD);

    FLASH_CS_HIGH;
}

/*******************************************************************************
 * Function Name  : dev_flash_write_disable
 * Description    : 写失能
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : None
 *******************************************************************************/
static void dev_flash_write_disable(void)
{
    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, FLASH_WRITE_ENABLE_CMD);

    FLASH_CS_HIGH;
}

/*******************************************************************************
 * Function Name  : dev_flash_read_sr
 * Description    : 读状态寄存器
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : 8位状态寄存器
 *******************************************************************************/
uint8_t dev_flash_read_sr(uint8_t regs)
{
    uint8_t temp;

    if ((regs != FLASH_READ_SR1_CMD) && (regs != FLASH_READ_SR2_CMD))
        return 0xFF;

    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, regs);
    temp = dev_flash_read_write_byte(true, 0xff);

    FLASH_CS_HIGH;

    return temp;
}

/*******************************************************************************
 * Function Name  : dev_flash_write_sr
 * Description    : 写状态寄存器
 *
 *
 * Input          : sr 8位状态寄存器
 *
 *
 * Output         : None
 * Return         :
 *******************************************************************************/
static void dev_flash_write_sr(uint8_t sr)
{
    dev_flash_write_enable();

    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, FLASH_WRITE_SR_CMD);
    dev_flash_read_write_byte(false, sr);

    FLASH_CS_HIGH;

    // dev_flash_write_disable();
}

/*******************************************************************************
 * Function Name  : dev_flash_wait_nobusy
 * Description    : Flash操作是否处于忙状态，判断之前的工作是否完成
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : 8位状态寄存器
 *******************************************************************************/
static void dev_flash_wait_nobusy(void)
{
    while ((dev_flash_read_sr(FLASH_READ_SR1_CMD) & 0x01))
        dev_flash_delay(2); // 必须做一下延迟
}

/******************************************************************************/

/******************************************************************************/
/* Extern Function */

/*******************************************************************************
 * Function Name  : dev_flash_read_device_id
 * Description    : 读取 deviceid
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : 16位deviec_id 0xxx13 代表25q80这种8M的 0xxx14代表16M 0xxx15代表 32M 0xxx16代表 64M的 前面1字节看生产厂商
 *******************************************************************************/
uint16_t dev_flash_read_device_id(void)
{
    uint16_t dat = 0;
    uint8_t cmd[] = {FLASH_READ_DEVICE_ID, 0x00, 0x00, 0x00};

    FLASH_CS_LOW;

    /* Send "RDID " instruction */
    dev_flash_send_bytes(cmd, sizeof(cmd) / sizeof(cmd[0]));

    /* Read a byte from the FLASH */
    dat |= (uint16_t)dev_flash_read_write_byte(true, 0xff) << 8;
    /* Read a byte from the FLASH */
    dat |= dev_flash_read_write_byte(true, 0xff);

    FLASH_CS_HIGH;

    return (dat);
}

/*******************************************************************************
 * Function Name  : dev_flash_read_jedec_id
 * Description    : 读取 jedec id
 *
 *
 * Input          : None
 *
 *
 * Output         : None
 * Return         : 32位jedec id 前2字节为0x00
 *******************************************************************************/
uint32_t dev_flash_read_jedec_id(void)
{
    uint32_t dat = 0;

    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, FLASH_READ_JEDEC_ID);

    dat = (uint32_t)dev_flash_read_write_byte(true, 0xff) << 16;

    dat |= (uint32_t)dev_flash_read_write_byte(true, 0xff) << 8;

    dat |= dev_flash_read_write_byte(true, 0xff);

    FLASH_CS_HIGH;

    return (dat);
}

/*******************************************************************************
 * Function Name  : dev_flash_read_bytes
 * Description    : 读取数据
 *
 *
 * Input          : addr 读取地址
 *                  read_length 读取长度
 *
 * Output         : pdata 读取到的buf指针
 * Return         : None
 *******************************************************************************/
void dev_flash_read_bytes(uint8_t *pdat, uint32_t addr, uint16_t read_length)
{
    uint8_t cmd[] = {FLASH_READ_DATA, 0, 0, 0};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr >> 0);

    FLASH_CS_LOW;

    dev_flash_send_bytes(cmd, sizeof(cmd) / sizeof(cmd[0]));

    dev_flash_recv_bytes(pdat, read_length);

    FLASH_CS_HIGH;
}

/*******************************************************************************
* Function Name  : dev_flash_write_page
* Description    : 写一页 最多256字节 注意这里别越扇区了！！！不会自己检查
*
*
* Input          : pdata 数据
*                  addr  写入地址
*                  write_length 写入长度

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_write_page(uint8_t *pdat, uint32_t write_addr, uint16_t write_length)
{
    uint8_t cmd[] = {FLASH_WRITE_PAGE, 0, 0, 0};

    cmd[1] = (uint8_t)(write_addr >> 16);
    cmd[2] = (uint8_t)(write_addr >> 8);
    cmd[3] = (uint8_t)(write_addr >> 0);

    dev_flash_write_enable();

    FLASH_CS_LOW;

    dev_flash_send_bytes(cmd, sizeof(cmd) / sizeof(cmd[0]));

    dev_flash_send_bytes(pdat, write_length);

    FLASH_CS_HIGH;

    dev_flash_wait_nobusy();

    // dev_flash_write_disable();
}

extern void display_hex_data(uint8_t *dat, uint32_t len);
/*******************************************************************************
* Function Name  : dev_flash_write_bytes_nocheck
* Description    : 写数据 自动查询当前地址 自动越扇区写入 注意这里只能写擦除过的
*
*
* Input          : pdata 数据
*                  write_addr  写入地址
*                  write_length 写入长度

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_write_bytes_nocheck(uint8_t *pdat,
                                   uint32_t write_addr,
                                   uint16_t write_length)
{
    uint16_t PageByte = DEV_FLASH_PAGE_SIZE - (write_addr & (DEV_FLASH_PAGE_SIZE - 1U)); // 单页剩余的字节数
    // uint16_t PageByte = DEV_FLASH_PAGE_SIZE - write_addr % DEV_FLASH_PAGE_SIZE; // 单页剩余的字节数

    if (write_length < PageByte)
    {
        PageByte = write_length;
    }
#if (W25Qx_USING_DEBUG)
    Uartx_Printf(W25Qx_UART, "\r\npage byte\taddr\t\tlen\r\n");
#endif
    while (1)
    {
#if (W25Qx_USING_DEBUG)
        uint8_t buf[DEV_FLASH_PAGE_SIZE];
        Uartx_Printf(W25Qx_UART, "%d\t%#X\t\t%#X\r\n", PageByte, (uint16_t)write_addr, write_length);
        display_hex_data(pdat, PageByte);
#endif
        dev_flash_write_page(pdat, write_addr, PageByte);
#if (W25Qx_USING_DEBUG)
        dev_flash_read_bytes(buf, write_addr, PageByte);
        display_hex_data(pdat, PageByte);
#endif
        if (write_length == PageByte)
            break;
        else
        {
            pdat += PageByte;
            write_addr += PageByte;
            write_length -= PageByte;
            if (write_length > DEV_FLASH_PAGE_SIZE)
            {
                PageByte = DEV_FLASH_PAGE_SIZE;
            }
            else
            {
                PageByte = write_length;
            }
        }
    }
}

/*******************************************************************************
* Function Name  : dev_flash_write
* Description    : 写数据 自动查询当前地址 自动越扇区写入 注意这里只能写擦除过的
*
*
* Input          : pdata 数据
*                  write_addr  写入地址
*                  write_length 写入长度

* Output         : None
* Return         : None
*******************************************************************************/
// static bit dev_flash_check_erase(uint8_t *pbuf,
//                                  uint32_t addr,
//                                  uint16_t len)
// {
//     uint8_t *p;

//     if (NULL == pbuf)
//         return true;

//     dev_flash_read_bytes(pbuf, addr, len);

//     for (p = pbuf; p < pbuf + len; ++p)
//     {
//         if (0xFF != *p)
//             return true;
//     }

//     return false;
// }

/*******************************************************************************
* Function Name  : dev_flash_write
* Description    : 写数据 自动查询当前地址 自动越扇区写入 注意这里只能写擦除过的
*
*
* Input          : pdata 数据
*                  write_addr  写入地址
*                  write_length 写入长度

* Output         : None
* Return         : None
*******************************************************************************/
// void dev_flash_write(uint8_t *pdat,
//                      uint32_t addr,
//                      uint16_t len)
// {
//     uint8_t buf[DEV_FLASH_PAGE_SIZE];
//     uint16_t cur_pages = addr >> 8U;                                                 // addr / DEV_FLASH_PAGE_SIZE;                                  // 整数倍页地址
//     uint8_t remaind_bytes = DEV_FLASH_PAGE_SIZE - addr & (DEV_FLASH_PAGE_SIZE - 1U); // 单页剩余的字节数
//     uint8_t front_bytes;
//     bit aligned_flag = false;

//     if (addr >= DEV_FLASH_FLASH_SIZE)
//         return;

//     if (len < remaind_bytes)
//         remaind_bytes = len;

//     for (;;)
//     {
//         if (dev_flash_check_erase(buf, addr, remaind_bytes))
//         {
//             front_bytes = addr & (DEV_FLASH_PAGE_SIZE - 1U);
//             if (front_bytes) // 非整页对齐数据写入
//             {
//                 aligned_flag = true;
//                 dev_flash_read_bytes(buf, addr - front_bytes, front_bytes);
//             }

//             dev_flash_erase_page(cur_pages);

//             if (aligned_flag)
//             {
//                 aligned_flag = false;
//                 dev_flash_write_page(buf, addr - front_bytes, remaind_bytes);
//             }
//         }

//         dev_flash_write_page(pdat, addr, remaind_bytes);
//         if (len == remaind_bytes)
//             break;
//         else
//         {
//             pdat += remaind_bytes;
//             addr += remaind_bytes;
//             len -= remaind_bytes;
//             remaind_bytes = len > DEV_FLASH_PAGE_SIZE ? DEV_FLASH_PAGE_SIZE : len;
//             cur_pages = addr >> 8U;
//         }
//     }
// }

/*******************************************************************************
* Function Name  : dev_flash_erase_page
* Description    : 擦除一页 256字节
*
*
* Input          : page_num 页码 注意填入的是 地址/256
*
*

* Output         : None
* Return         : None
*******************************************************************************/
// void dev_flash_erase_page(uint32_t start_addr)
// {
//     // uint32_t pages = page_num << 8U; //*256
//     uint8_t buf[] = {FLASH_ERASE_PAGE, 0, 0, 0};

//     buf[1] = (uint8_t)(start_addr >> 16);
//     buf[2] = (uint8_t)(start_addr >> 8);
//     buf[3] = (uint8_t)(start_addr >> 0);

//     dev_flash_write_enable();

//     FLASH_CS_LOW;

//     dev_flash_send_bytes(buf, sizeof(buf) / sizeof(buf[0]));

//     FLASH_CS_HIGH;

//     dev_flash_wait_nobusy();

//     // dev_flash_write_disable();
// }

/*******************************************************************************
* Function Name  : dev_flash_erase_sector
* Description    : 擦除一扇区 4K字节
*
*
* Input          : sector_num 扇区码 注意填入的是 地址/4096
*
*

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_erase_sector(uint32_t start_addr)
{
    uint8_t buf[] = {FLASH_ERASE_SECTOR, 0, 0, 0};

    // sector_num <<= 12U; // 4096;
    buf[1] = (uint8_t)(start_addr >> 16);
    buf[2] = (uint8_t)(start_addr >> 8);
    buf[3] = (uint8_t)(start_addr >> 0);

    dev_flash_write_enable();
    dev_flash_wait_nobusy();

    FLASH_CS_LOW;

    dev_flash_send_bytes(buf, sizeof(buf) / sizeof(buf[0]));

    FLASH_CS_HIGH;

    dev_flash_wait_nobusy();

    // dev_flash_write_disable();
}

/*******************************************************************************
* Function Name  : dev_flash_erase_sector
* Description    : 擦除一块  64K字节
*
*
* Input          : sector_num 扇区码 注意填入的是 地址/65536
*
*

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_erase_block(uint32_t start_addr, uint8_t cmd)
{
    uint8_t buf[] = {0, 0, 0, 0};

    if (start_addr > DEV_FLASH_FLASH_SIZE)
        return;
    buf[0] = cmd;
    buf[1] = (uint8_t)(start_addr & 0xFF0000 >> 16);
    buf[2] = (uint8_t)(start_addr & 0xFF00 >> 8);
    buf[3] = (uint8_t)(start_addr & 0xFF >> 0);

    dev_flash_write_enable();
    dev_flash_wait_nobusy();
#if (W25Qx_USING_DEBUG)
    Uartx_Printf(W25Qx_UART, "sf1: %#bx.\r\n", dev_flash_read_sr(FLASH_READ_SR1_CMD));
#endif

    FLASH_CS_LOW;

    dev_flash_send_bytes(buf, sizeof(buf) / sizeof(buf[0]));

    FLASH_CS_HIGH;

    dev_flash_wait_nobusy();

    // dev_flash_write_disable();
}

/*******************************************************************************
* Function Name  : dev_flash_erase_chip
* Description    : 整片擦除
*
*
* Input          : None
*
*

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_erase_chip(void)
{

    dev_flash_write_enable();

    FLASH_CS_LOW;

    dev_flash_read_write_byte(false, FLASH_ERASE_CHIP);

    FLASH_CS_HIGH;

    dev_flash_wait_nobusy();

    // dev_flash_write_disable();
}

// flash自适应擦除算法：https://www.amobbs.com/thread-5648832-1-1.html
/*******************************************************************************
* Function Name  : dev_flash_auto_adapt_erase
* Description    : 自适应擦除算法(w25Qx系列不存在页擦除指令)
*
*
* Input          : None
*
*

* Output         : None
* Return         : None
*******************************************************************************/
void dev_flash_auto_adapt_erase(uint32_t start_addr, uint32_t len)
{
#define RE_32KB(_s) (_s & (DEV_FLASH_64KB_BLOCK_SIZE - 1U))
#define RE_4KB(_s) (RE_32KB(_s) & (DEV_FLASH_32KB_BLOCK_SIZE - 1U))
#define RE_256B(_s) (RE_4KB(_s) & (DEV_FLASH_SECTOR_SIZE - 1U))

    uint8_t need_64kb_block = len >> 16U;
    uint8_t need_32kb_block = RE_32KB(len) >> 15U;
    uint8_t need_sector = (RE_4KB(len) + (DEV_FLASH_SECTOR_SIZE - 1U)) >> 12U; // 不足4KB的做补足处理
    uint8_t need_page = ((RE_256B(len) + (DEV_FLASH_PAGE_SIZE - 1U)) >> 8U);   // 不足256B的做补足处理

#if (W25Qx_USING_DEBUG)
    Uartx_Printf(W25Qx_UART, "start\tsize\r\n%#X\t%#X\r\n", (uint16_t)start_addr, (uint16_t)len);
#endif

    if ((start_addr > DEV_FLASH_FLASH_SIZE) || ((start_addr + len) > DEV_FLASH_FLASH_SIZE))
        return;

    // if (RE_32KB(start_addr) && RE_4KB(start_addr) && RE_256B(start_addr)) // 存在越扇区擦除
    // {
    // }

    // #if (W25Qx_USING_DEBUG)
    Uartx_Printf(W25Qx_UART, "64KB\t32KB\t4KB\t256B\r\n%bd\t%bd\t%bd\t%bd\r\n",
                 need_64kb_block, need_32kb_block, need_sector, need_page);
    // #endif

    for (; need_64kb_block--; start_addr += DEV_FLASH_64KB_BLOCK_SIZE)
        dev_flash_erase_block(start_addr, FLASH_ERASE_64KB_BLOCK_CMD);
    Uartx_Printf(W25Qx_UART, "next addr: %#X\r\n", (uint16_t)start_addr);

    for (; need_32kb_block--; start_addr += DEV_FLASH_32KB_BLOCK_SIZE)
        dev_flash_erase_block(start_addr, FLASH_ERASE_32KB_BLOCK_CMD);
    Uartx_Printf(W25Qx_UART, "next addr: %#X\r\n", (uint16_t)start_addr);

    for (; need_sector--; start_addr += DEV_FLASH_SECTOR_SIZE)
        dev_flash_erase_sector(start_addr);
    Uartx_Printf(W25Qx_UART, "next addr: %#X\r\n", (uint16_t)start_addr);

    // for (; need_page--; start_addr += DEV_FLASH_PAGE_SIZE)
    //     dev_flash_erase_page(start_addr);
    // Uartx_Printf(W25Qx_UART, "next addr: %#X\r\n", (uint16_t)start_addr);
}

/******************************************************************************/