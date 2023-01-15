#include "GPIO.h"
#include "timer.h"
#include "usart.h"
#include "eeprom.h"
#include "w25qx.h"
#include "ymodem.h"
#include "utils_ringbuffer.h"

#if (!USING_DEBUG && YMODEM_USING_DEBUG)
#error Global debugging mode is not turned on!
#endif

#define YMODEM_FLASH_SIZE USER_FLASH_ADDR

extern struct ringbuffer rm_rb;

/*************	本地变量声明	**************/
static uint8_t ym_buf[PACKET_1K_SIZE + PACKET_OVERHEAD];
static ymodem ymo = {0};
// static uint16_t ym_count = 0;
// uint8_t ota_flag;

/*************	本地函数声明	**************/
// extern uint8_t *uint32_to_string(uint32_t num);
static ym_err_t Ymodem_Download(ymodem_t ym);
static ym_err_t ymodem_putchar(ym_err_t ym_code);
static ym_err_t ymodem_handshake(ymodem_t ym);
static ym_err_t ymodem_do_trans(ymodem_t ym);
static ym_err_t ymodem_do_fin(ymodem_t ym);
static ym_err_t ymodem_wait_start(ymodem_t ym);

static void report_isp_information(ymodem_t ym)
{
    if (NULL == ym)
        return;

    Uartx_Printf(OTA_WORK_UART, "================================\r\nFile name: %s\r\n", ym->file_name);
    Uartx_Printf(OTA_WORK_UART, "File length: %dBytes\r\n", ym->save.file_size);
    Uartx_Printf(OTA_WORK_UART, "Bootloader Version:   2022-12-29 by LHC\r\n");
    Uartx_Printf(OTA_WORK_UART, "================================\r\n\r\n");
}

// static void reset_ota_flag(void)
// {
//     ota_flag = ~OTA_FLAG_VALUE;
//     if (read_ota_addr != (~OTA_FLAG_VALUE))
//         IapWrites(OTA_FLAG_ADDR, &ota_flag, sizeof(ota_flag)); // 擦除ota标志
// }

// static void set_ota_flag(void)
// {
//     ota_flag = OTA_FLAG_VALUE;
//     if (read_ota_addr != OTA_FLAG_VALUE)
//         IapWrites(OTA_FLAG_ADDR, &ota_flag, sizeof(ota_flag)); // 写入ota标志
// }

#define _ymodem_set_timer(__t)   \
    do                           \
    {                            \
        _timer_ota.flag = false; \
        _timer_ota.count = __t;  \
    } while (false)

char Ota_Menue(void *const rb)
{
    ym_err_t res;
    ymodem_t xdata ym = &ymo;

    ym->buf = ym_buf;
    ym->rb = (struct ringbuffer *)rb;

    if ((NULL == ym->buf) || (NULL == ym->rb))
        return -ym_err_other;

    switch (ym->cur_state)
    {
    case ym_wait:
        res = ymodem_wait_start(ym);
        break;
    case ym_handshake:
        res = ymodem_handshake(ym);
        break;
    case ym_trans_data:
        res = ymodem_do_trans(ym);
        break;
    case ym_trans_end:
        res = ymodem_do_fin(ym);
        break;
    // case ym_trans_fail:
    //     memset(ym->comm, 0x00, sizeof(ym->comm));
    //     ym->cur_state = ym_wait;
    //     ym->next_state = ym_wait;
    //     break;
    case ym_transition:
    {
        ym->cur_state = ym->next_state;
        memset(&ym->comm.count, 0x00, sizeof(ym->comm) - sizeof(ym->comm.flag));
        // ym->comm.count = 0;
        // ym->comm.len = 0;
    }
    break;
    default:
        // return -res;
        // ringbuffer_clean(rb);
        ym->comm.count = 0;
        break;
    }

    switch (res)
    {
    case ym_ok:
        Uartx_Printf(OTA_WORK_UART, "\r\nProgramming Completed Successfully !\r\n");
        report_isp_information(ym);
        break;
    case ym_user_cancel:
        Uartx_Printf(OTA_WORK_UART, "\r\n MCU abort !\r\n");
        break;
    case ym_pc_cancel:
        Uartx_Printf(OTA_WORK_UART, "\r\n User abort !\r\n");
        break;
    case ym_file_size_large:
        Uartx_Printf(OTA_WORK_UART, "\r\n File size is too large !\r\n");
        break;
    case ym_program_err:
        Uartx_Printf(OTA_WORK_UART, "\r\n Programming Error !\r\n");
        break;
    case ym_next:
        break;
    default:
        Uartx_Printf(OTA_WORK_UART, "\r\n other error: %bd.\r\n", res);
        break;
    }

    if (ym_next != res) // 产生错误时，清空系统信息
    {
        ringbuffer_clean(((struct ringbuffer *)rb));
        memset(&ym->comm, 0x00, sizeof(ym->comm));
        ym->cur_state = ym_wait;
        ym->next_state = ym_wait;
        if (ym_ok != res)
            ymodem_putchar(YM_CAN);
    }

    return ((res >= ym_ok) && (res < ym_user_cancel) ? res : -res);
}

#if !defined USING_SIMULATE
void display_hex_data(uint8_t *dat, uint32_t len)
{
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH 16
    uint16_t i, j;

    if (NULL == dat || !len)
        return;

    for (i = 0; i < len; i += HEXDUMP_WIDTH)
    {
        Uartx_Printf(OTA_INFO_OUT_UART, "\r\n[%04x]: ", i);
        for (j = 0; j < HEXDUMP_WIDTH; ++j)
        {
            if (i + j < len)
            {
                Uartx_Printf(OTA_INFO_OUT_UART, "%02bX ", dat[i + j]);
            }
            else
            {
                Uartx_Printf(OTA_INFO_OUT_UART, "   ");
            }
        }
        Uartx_Printf(OTA_INFO_OUT_UART, "\t\t");
        for (j = 0; (i + j < len) && j < HEXDUMP_WIDTH; ++j)
            Uartx_Printf(OTA_INFO_OUT_UART, "%c",
                         __is_print(dat[i + j]) ? dat[i + j] : '.');
    }
    if (len)
        Uartx_Printf(OTA_INFO_OUT_UART, "\r\n\r\n");
}
#endif

static uint16_t ym_crc16(unsigned char *q, int len)
{
    uint16_t crc;
    char i;

    crc = 0;
    while (--len >= 0)
    {
        crc = crc ^ (int)*q++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }

    return (crc);
}

// void ym_memcpy(void *s1, const void *s2, size_t n)
// {
//     uint8_t *dest = (uint8_t *)s1;
//     const uint8_t *source = (const uint8_t *)s2;

//     if (NULL == dest || NULL == source)
//         return;

//     while (n--)
//     {
//         *dest++ = *source++;
//     }
// }

// #pragma OT(0)
// void delay_1s(void) //@24MHz
// {
//     unsigned char i, j, k;

//     _nop_();
//     _nop_();
//     i = 122;
//     j = 193;
//     k = 128;
//     do
//     {
//         do
//         {
//             while (--k)
//                 ;
//         } while (--j);
//     } while (--i);
// }
// #pragma OT(9)

static ym_err_t ymodem_putchar(ym_err_t ym_code)
{
    char c;
    uint8_t i;
    static uint8_t errors = 0;

    switch (ym_code)
    {
    case YM_PUT_C:
        c = CRC16;
        break;
    case YM_ACK:
        c = ACK;
        errors = 0;
        break;
    case YM_NACK:
        c = NAK;
        if (errors++ > MAX_ERRORS)
            goto __exit;
        break;
    case YM_CAN:
        c = CANCEL;
        for (i = 0; i < RYM_END_SESSION_SEND_CAN_NUM; ++i)
            Uartx_SendStr(OTA_WORK_UART, (uint8_t *)&c, sizeof(c), UART_BYTE_SENDOVERTIME);
        // delay_1s();

        return ym_ok;
    __exit:
    default:
        return ym_err_other;
    }
    Uartx_SendStr(OTA_WORK_UART, (uint8_t *)&c, sizeof(c), UART_BYTE_SENDOVERTIME);
    // if (YM_NACK == ym_code)
    //     delay_1s();

    return ym_next;
}

static ym_err_t ymodem_wait_start(ymodem_t ym)
{
    if ((ym->comm.count < _ym_wait_counts) && _timer_ota.flag)
    {
        Uartx_Printf(OTA_WORK_UART, "\r\n Press 'd' to start......\r\n");

        if ((1U == ringbuffer_gets((struct ringbuffer *)ym->rb, ym->buf, 1U)) &&
            (ym->buf[0] == 'd'))
        {
            Uartx_Printf(OTA_WORK_UART, "\r\n\r\n Waiting for the file to be sent ... (press 'a' to abort)\r\n");

            ym->cur_state = ym_transition;
            ym->next_state = ym_handshake;
            // ym->comm.count = 0;
        }

        if (ringbuffer_num((struct ringbuffer *)ym->rb)) // 清除堆积的错误数据
            ringbuffer_flush((struct ringbuffer *)ym->rb);

        // if (fingbuffer_get_num(((struct ringbuffer *)ym->rb)))
        //     ringbuffer_clean(((struct ringbuffer *)ym->rb));
        ym->comm.count++;
        _ymodem_set_timer(_ym_delay_1s);
    }

    if (_ym_wait_counts == ym->comm.count)
        return ym_rec_timeout;

    return ym_next;
}

/**
 * @brief	ymodem对接flash擦除函数
 * @details 此处适配的擦除策略适用于w25Q64
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_erase_falsh(ymodem_t ym)
{
    /*快速擦除策略：不足一块的按一块补齐*/
    // uint8_t need_block = (ym->save.file_size + (DEV_FLASH_BLOCK_SIZE - 1U)) >> 16U;
    uint8_t regs[] = {0, 0};

    // #if (YMODEM_USING_DEBUG == 1)
    //     Uartx_Printf(OTA_INFO_OUT_UART, "need erase balock: %bd.\r\n", need_block);
    // #endif

    dev_flash_auto_adapt_erase(FLASH_START_ADDR, ym->save.file_size);
    // dev_flash_erase_chip();

#if (YMODEM_USING_DEBUG)
    regs[0] = dev_flash_read_sr(FLASH_READ_SR1_CMD);
    regs[1] = dev_flash_read_sr(FLASH_READ_SR2_CMD);
    Uartx_Printf(OTA_INFO_OUT_UART, "regs(1~0): %#bx  %#bx\r\n", regs[1], regs[0]);
    dev_flash_read_bytes(ym->buf, 0x6f00, 1024);
    display_hex_data(ym->buf, 1024);
#endif

    return ym_next;

    //    uint16_t i;
    // for (i = FLASH_START_ADDR; i < YMODEM_FLASH_SIZE; i += SECTOR_SIZE) // 擦除N页
    //     IapErase(i);
}

/**
 * @brief	ymodem开始接收数据前处理工作
 * @version V1.0.0,2022/01/18
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_begin(ymodem_t ym, uint8_t *buf, uint16_t size)
{
    char *file_name, *file_size;

    size = size;

    /* calculate and store file size */
    file_name = (char *)&buf[0];
    file_size = (char *)&buf[strlen(file_name) + 1];

    // memmove(ym->file_name, file_name, sizeof(ym->file_name));
    memcpy(ym->file_name, file_name, strlen(file_name)); // ym_memcpy
    ym->save.file_size = atoi(file_size);                // Str_To_Int(file_size)文件长度由字符串转成十六进制数据

#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "[file name]: %s, [file size]: %d bytes.\r\n",
                 ym->file_name, ym->save.file_size);
#endif

    if (!ym->save.file_size || ym->save.file_size >= YMODEM_FLASH_SIZE) // 长度过长错误
        return YM_CAN;                                                  // 错误返回N个 CA, 长度过长

    Iap_Reads(STC_BOOT_JMP_ADDR, ym->jmp_code, sizeof(ym->jmp_code));
#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "boot loader addr: 0x%bX 0x%bX 0x%bX .\r\n",
                 ym->jmp_code[0], ym->jmp_code[1], ym->jmp_code[2]);

#endif

    if (ymodem_erase_falsh(ym) != ym_next)
        return ym_program_err;

    ym->next_flash_addr = FLASH_START_ADDR; //+ 3U 记录数据帧开始写入的首地址(3byte ISP code + 3Byte NULL)

    return ym_next; // 擦除完成, 返回应答
}

// #pragma OT(0)
/**
 * @brief	ymodem发送握手信号
 * @version V1.0.0,2022/01/18
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_handshake(ymodem_t ym)
{
    ym_err_t res;
    uint16_t rqe_crc16, rel_crc16;
    struct ringbuffer *rb = (struct ringbuffer *)ym->rb;

    if (!__GET_FLAG(ym->comm.flag, ym_step1) && _timer_ota.flag &&
        ym->comm.count < _ym_handshake_counts)
    { // xshell特殊握手信号
        ymodem_putchar(YM_PUT_C);

        ym->comm.len = ringbuffer_gets((struct ringbuffer *)ym->rb, ym->buf, 1);

        if (1U == ym->comm.len)
        {
            if (ABORT1 == ym->buf[0] || ABORT2 == ym->buf[0])
            {
                return ym_pc_cancel;
            }
            else if (SOH == ym->buf[0] || STX == ym->buf[0])
            {
                __SET_FLAG(ym->comm.flag, ym_step1);
                memset(&ym->comm.count, 0x00, sizeof(ym->comm) - sizeof(ym->comm.flag));

                if (SOH == ym->buf[0])
                    ym->data_size = RE_PACKET_128B_SIZE;
                else
                    ym->data_size = RE_PACKET_1K_SIZE;
            }
        }
        _ymodem_set_timer(_ym_delay_100ms);
        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step1))
    {
        if (_ym_handshake_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step2) && _timer_ota.flag &&
        ym->comm.count < _ym_handshake_counts)
    {
        ym->comm.len += ringbuffer_gets((struct ringbuffer *)ym->rb, (ym->buf + ym->comm.len + 1U),
                                        ym->data_size - 1U - ym->comm.len);
#if (YMODEM_USING_DEBUG == 1)
        Uartx_Printf(OTA_INFO_OUT_UART, "len: %d\r\n", ym->comm.len);
#endif
        if (ym->comm.len == ym->data_size - 1U)
        {
            __SET_FLAG(ym->comm.flag, ym_step2);
        }

        // if (ym->comm.len && (ABORT1 == ym->buf[ym->comm.len - 1U]) ||
        //     (ABORT2 == ym->buf[ym->comm.len - 1U]))
        // {
        //     return ym_pc_cancel;
        // }

        _ymodem_set_timer(_ym_delay_100ms);
        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step2))
    {
        if (_ym_handshake_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

#if !defined USING_SIMULATE
    display_hex_data(ym->buf, ym->comm.len);
#endif

    /* sanity check */
    if (ym->buf[1] != 0 || ym->buf[2] != 0xFF)
        return ym_err_other;

    rqe_crc16 = ym_crc16(ym->buf + 3U, ym->data_size - 5U);
    rel_crc16 = ((uint16_t)ym->buf[ym->data_size - 2U] << 8U) | ym->buf[ym->data_size - 1U];
#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "req_crc\trel_crc\r\n%#x\t%#x\r\n", rqe_crc16, rel_crc16);
#endif
    if (rqe_crc16 != rel_crc16)
        return ym_recv_err;

    res = ymodem_on_begin(ym, ym->buf + 3U, ym->data_size - 5U);
    ym->packets = 0;

    if (res != ym_next)
    {
        return ym_err_other;
    }

    ym->comm.flag = 0;
    ym->cur_state = ym_transition;
    ym->next_state = ym_trans_data;

    ymodem_putchar(YM_ACK);
    ymodem_putchar(YM_PUT_C); // 请求pc发送数据帧

#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "Handshake succeeded,start recv data.\r\n");
#endif

    return res;
}
#pragma OT(9)

/**
 * @brief	ymodem对接flash编程函数
 * @details 此处适配的编程策略适用于w25Q64
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_program_falsh(ymodem_t ym, uint8_t *pdat, uint16_t len)
{
    if (ym->next_flash_addr >= YMODEM_FLASH_SIZE)
        return ym_program_err;

    dev_flash_write_bytes_nocheck(pdat, ym->next_flash_addr, len);
    ym->next_flash_addr += len;

    return ym_next;

    // uint16_t base_addr;
    // for (base_addr = ym->next_flash_addr;
    //      pdat && ym->next_flash_addr < base_addr + size;
    //      ++ym->next_flash_addr, ++pdat) // 去掉3个字节帧头
    // {
    //     IapProgram(ym->next_flash_addr, *pdat);

    //     if (ym->next_flash_addr >= YMODEM_FLASH_SIZE)
    //         return ym_program_err;
    // }
    // return ym_next;
}

// #pragma OT(0)
/**
 * @brief	用户处理数据帧
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_data(ymodem_t ym, uint8_t *pdat, uint16_t size)
{
    // uint8_t t_jmp_code[3];

    // if (0 == ym->packets) // 第0帧数据特殊处理
    // {
    //     memcpy(t_jmp_code, pdat, sizeof(t_jmp_code));           // 取出应用程序跳转地址 ym_memcpy
    //     memcpy(pdat, ym->jmp_code, sizeof(ym->jmp_code));       // 写回bootloader跳转地址
    //     memcpy(ym->jmp_code, t_jmp_code, sizeof(ym->jmp_code)); // 记录app跳转地址
    // }

    if (ymodem_program_falsh(ym, pdat, size) != ym_next)
        return ym_program_err;

#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "start\tend\tsize\tpackets\r\n%#x\t%#x\t%#x\t%#x\r\n",
                 (ym->next_flash_addr - size), ym->next_flash_addr, size, ym->packets);
#endif
    return ym_next;
}
// #pragma OT(9)

static ym_err_t ymodem_trans_data(ymodem_t ym)
{
    ym_err_t ym_res;
    uint16_t tsz = 2U + ym->data_size + 2U;
    uint16_t rqe_crc16, rel_crc16;

    if (!__GET_FLAG(ym->comm.flag, ym_step2) && ym->comm.count < _ym_recv_counts)
    {
        ym->comm.len += ringbuffer_gets((struct ringbuffer *)ym->rb, ym->buf + 1U + ym->comm.len,
                                        tsz - ym->comm.len);
        if (ym->comm.len == tsz)
        {
            __RESET_FLAG(ym->comm.flag, ym_step1);
            __SET_FLAG(ym->comm.flag, ym_step2);
        }
        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step2))
    {
        if (_ym_recv_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

    if ((ym->buf[1] + ym->buf[2]) != 0xFF)
    {
        ym_res = ym_recv_err;
        goto __exit;
    }

    rqe_crc16 = ym_crc16(ym->buf + 3U, ym->data_size);
    rel_crc16 = ((uint16_t)ym->buf[ym->comm.len - 1U] << 8U) | ym->buf[ym->comm.len];
#if (YMODEM_USING_DEBUG == 1)
    Uartx_Printf(OTA_INFO_OUT_UART, "req_crc\trel_crc\r\n%#x\t%#x\r\n", rqe_crc16, rel_crc16);
#endif
    if (rqe_crc16 != rel_crc16)
    {
        ym_res = ym_recv_err;
        goto __exit;
    }

    ym_res = ymodem_on_data(ym, ym->buf + 3U, ym->data_size);
    if (ym_next == ym_res)
    {
        memset(&ym->comm, 0x00, sizeof(ym->comm));
        ymodem_putchar(YM_ACK); // 保存完成, 返回应答
        // _ymodem_set_timer(_ym_delay_100ms); // 解决网络延时可能导致粘包问题
        ym->packets++;
    }

__exit:
    return ym_res;
}

/**
 * @brief	ymodem连续接收数据帧
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_do_trans(ymodem_t ym)
{
    ym_err_t ym_res;
    struct ringbuffer *rb = (struct ringbuffer *)ym->rb;

    if (!__GET_FLAG(ym->comm.flag, ym_step1) && //&& _timer_ota.flag
        ym->comm.count < _ym_recv_counts)
    {
        ym->comm.len = ringbuffer_gets((struct ringbuffer *)ym->rb, ym->buf, 1U);

        if (1U == ym->comm.len)
        {
            switch (ym->buf[0])
            {
            case SOH:
                ym->data_size = PACKET_SIZE;
                __SET_FLAG(ym->comm.flag, ym_step1);
                __RESET_FLAG(ym->comm.flag, ym_step2);
                break;
            case STX:
                ym->data_size = PACKET_1K_SIZE;
                __SET_FLAG(ym->comm.flag, ym_step1);
                __RESET_FLAG(ym->comm.flag, ym_step2);
                break;
            case EOT:
                ym->cur_state = ym_transition;
                ym->next_state = ym_trans_end;
                ymodem_putchar(YM_NACK);
#if (YMODEM_USING_DEBUG == 1)
                Uartx_Printf(OTA_INFO_OUT_UART, "Data frame reception completed.\r\n");
#endif
                return ym_next;
                break;
            default:
            __repeat:
                memset(&ym->comm, 0x00, sizeof(ym->comm));
                if (fingbuffer_get_num(rb))
                    ringbuffer_clean(rb);

                if (ymodem_putchar(YM_NACK) != ym_next)
                    return ym_recv_err;

                return ym_next;
                break;
            }
            ym->comm.count = 0;
            ym->comm.len = 0;
        }

        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step1))
    {
        if (_ym_recv_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

    ym_res = ymodem_trans_data(ym);

    if (ym_res != ym_next)
    {
        goto __repeat;
    }

    return ym_res;
}

/**
 * @brief	ymodem用户收尾工作
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_on_end(ymodem_t ym)
{

#if (YMODEM_USING_DEBUG == 1)
#define READ_FLASH_SIZE 1024
#define YM_READ_FLASH_START_ADDR(_x) (ym->save.file_size - (_x)*READ_FLASH_SIZE) >> 10U
    uint8_t i, counts = (ym->save.file_size + (READ_FLASH_SIZE - 1U)) >> 10U;

    //     Uartx_Printf(OTA_INFO_OUT_UART, "counts: %bd\r\n", counts);

    for (i = YM_READ_FLASH_START_ADDR(2); i < counts; ++i) //(ym->save.file_size - 5 * READ_FLASH_SIZE) >> 10U
    {
        Uartx_Printf(OTA_INFO_OUT_UART, "\r\ncur count[%#bx]:", i);
        // memset(ym_buf, 0x00, sizeof(ym_buf));
        // Iap_Reads(addr, ym_buf, READ_FLASH_SIZE);
        dev_flash_read_bytes(ym->buf, FLASH_START_ADDR + i << 10U, READ_FLASH_SIZE);
        display_hex_data(ym->buf, READ_FLASH_SIZE);
    }
    // dev_flash_read_bytes(ym->buf, 0x0000, READ_FLASH_SIZE);
    // display_hex_data(ym->buf, READ_FLASH_SIZE);
#else
    ym = ym;
#endif

    // IapWrites(YMODEM_FLASH_SIZE, ym->jmp_code, sizeof(ym->jmp_code)); // 写入用户程序跳转地址

    // IapWrites(OTA_FLAG_ADDR, (uint8_t *)&ym->save, sizeof(ym->save)); // 写入文件信息
    dev_flash_auto_adapt_erase(OTA_FLAG_ADDR, sizeof(ym->save));
    // dev_flash_read_bytes(ym->buf, OTA_FLAG_ADDR, 0x100);
    // display_hex_data(ym->buf, 0x100);
    ym->save.ota_value = OTA_FLAG_VALUE;
    dev_flash_write_page((uint8_t *)&ym->save, OTA_FLAG_ADDR, sizeof(ym->save));
    // dev_flash_write_bytes_nocheck((uint8_t *)&ym->save, OTA_FLAG_ADDR, sizeof(ym->save));

    // dev_flash_read_bytes(ym->buf, OTA_FLAG_ADDR, 0x100);
    // display_hex_data(ym->buf, 0x100);
#if (YMODEM_USING_DEBUG == 1)
    dev_flash_read_bytes(ym->buf, OTA_FLAG_ADDR, sizeof(ym->save));
    Uartx_Printf(OTA_INFO_OUT_UART, "ota_flag\tflie_size(Bytes)\r\n%#X\t\t%#X\r\n",
                 ((uint16_t)ym->buf[0] << 8U | ym->buf[1]), ((uint16_t)ym->buf[2] << 8U | ym->buf[3]));
#endif

#if (YMODEM_USING_DEBUG == 1)
    // Iap_Reads(YMODEM_FLASH_SIZE, ym->jmp_code, sizeof(ym->jmp_code));
    // Uartx_Printf(OTA_INFO_OUT_UART, "app loader addr: 0x%bX 0x%bX 0x%bX .\r\n",
    //              ym->jmp_code[0], ym->jmp_code[1], ym->jmp_code[2]);
#endif

    return ym_next; // 正确
}

/**
 * @brief	ymodem收尾工作
 * @details
 * @param	None
 * @retval	None
 */
static ym_err_t ymodem_do_fin(ymodem_t ym)
{
    if (!__GET_FLAG(ym->comm.flag, ym_step1) && ym->comm.count < _ym_recv_counts)
    {
        ym->comm.len = ringbuffer_gets((struct ringbuffer *)ym->rb, ym->buf, 1U);
        if ((1U == ym->comm.len) && (EOT == ym->buf[0]))
        {
            __SET_FLAG(ym->comm.flag, ym_step1);
        }

        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step1))
    {
        if (_ym_recv_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step2))
    {
        ymodem_putchar(YM_ACK);
        ymodem_putchar(YM_PUT_C);
        __SET_FLAG(ym->comm.flag, ym_step2);
        memset(&ym->comm.count, 0x00, sizeof(ym->comm) - sizeof(ym->comm.flag));
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step3) && ym->comm.count < _ym_recv_counts)
    {
        ym->comm.len += ringbuffer_gets((struct ringbuffer *)ym->rb, (ym->buf + ym->comm.len),
                                        RE_PACKET_128B_SIZE - ym->comm.len);
        if (RE_PACKET_128B_SIZE == ym->comm.len)
        {
            __SET_FLAG(ym->comm.flag, ym_step3);
        }

        ym->comm.count++;
    }

    if (!__GET_FLAG(ym->comm.flag, ym_step3))
    {
        if (_ym_recv_counts == ym->comm.count)
            return ym_rec_timeout;

        return ym_next;
    }

    ymodem_putchar(YM_ACK); // 最后一次应答

    memset(&ym->comm, 0x00, sizeof(ym->comm));
    ym->cur_state = ym->next_state = ym_wait;

    if (ymodem_on_end(ym) != ym_next)
        return ym_err_other;

    return ym_ok;
}
