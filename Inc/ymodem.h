
#ifndef _YMODEM_H_
#define _YMODEM_H_
#include "config.h"

#if defined(USING_SIMULATE)
#define YMODEM_USING_DEBUG 0
#else
#define YMODEM_USING_DEBUG 0
#endif
/*ISP开始地址, 高地址必须是偶数, 注意要预留ISP空间,本例程留10K*/
#define BOOTLOADER_ADDRESS 0xC800U
/*用户FLASH长度, 保留个字节存放用户地址程序的跳转地址*/
#define USER_FLASH_ADDR (BOOTLOADER_ADDRESS - 3U)
/*内部falsh开始地址*/
#define FLASH_START_ADDR 0x0000 // 0x1800
#define STC_BOOT_JMP_ADDR 0x0000
#define PACKET_SEQNO_INDEX 1
#define PACKET_SEQNO_COMP_INDEX 2

#define PACKET_HEADER 3
#define PACKET_TRAILER 2
#define PACKET_OVERHEAD (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE 128
#define PACKET_1K_SIZE 1024
#define RE_PACKET_128B_SIZE (PACKET_SIZE + PACKET_OVERHEAD)
#define RE_PACKET_1K_SIZE (PACKET_1K_SIZE + PACKET_OVERHEAD)

#define FILE_NAME_LENGTH 32
#define FILE_SIZE_LENGTH 8

#define SOH 0x01    /* start of 128-byte data packet */
#define STX 0x02    /* start of 1024-byte data packet */
#define EOT 0x04    /* end of transmission */
#define ACK 0x06    /* acknowledge */
#define NAK 0x15    /* negative acknowledge */
#define CANCEL 0x18 /* two of these in succession aborts transfer */
#define CRC16 0x43  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1 'A' /* 'A' == 0x41, abort by user */
#define ABORT2 'a' /* 'a' == 0x61, abort by user */

/* how many CAN be sent when user active end the session. */
#ifndef RYM_END_SESSION_SEND_CAN_NUM
#define RYM_END_SESSION_SEND_CAN_NUM 0x07
#endif

#define _ym_handshake_counts 600U /*握手超时时间*/
#define _ym_recv_counts 600U * 2U /*接收超时时间*/
// #define NAK_TIMEOUT 10000
#define _ym_wait_counts 60 // 300*5ms
#define TIMEOUTS 5000      // 5ms5300 1000
#define MAX_ERRORS 10
#define _ym_delay_1s (1000U / TIMES)
#define _ym_delay_500ms (500U / TIMES)
#define _ym_delay_100ms (100U / TIMES)
#define _ym_delay_10ms (10U / TIMES)

#define OTA_INFO_OUT_UART &Uart1 // 0
#define OTA_WORK_UART &Uart2

#pragma OT(0)
typedef enum
{
    ym_none = -1,
    ym_ok,
    ym_next,
    ym_user_cancel,
    ym_pc_cancel,
    ym_file_size_large,
    ym_file_name_err,
    ym_program_err,
    ym_recv_err,
    ym_rec_timeout,
    ym_handshake_err,
    ym_err_other,
    /*应答指令*/
    YM_PUT_C,
    YM_ACK,
    YM_NACK,
    YM_CAN,
    ym_err_max,
} ym_err_t;

typedef enum
{
    ym_no_state = -1,
    ym_wait,
    ym_transition, // 中间状态
    ym_handshake,
    ym_trans_data,
    ym_trans_end,
    ym_trans_fail,
    ym_sate_max,
} ym_state;

typedef enum
{
    ym_step1 = 0,
    ym_step2,
    ym_step3,
    ym_step4,
    ym_clear_count,
    ym_clear_len,
    ym_clear_size,
} ym_flag;

typedef struct ymodem_s *ymodem_t;
typedef struct ymodem_s ymodem;
struct ymodem_s
{
    ym_state cur_state, next_state;
    uint8_t file_name[FILE_NAME_LENGTH];
    struct
    {
        uint16_t ota_value;
        uint16_t file_size;
    } save;
    volatile uint16_t next_flash_addr;
    uint16_t packets;
    uint16_t count, recv_len, data_size;
    uint8_t next_flag[4];
    // volatile uint32_t check_sum;
    volatile uint8_t jmp_code[3];
    struct
    {
        uint16_t flag;
        uint16_t count;
        uint16_t len;
        uint16_t size;
    } comm;

    uint8_t *buf;
    void *rb;
};

extern char Ota_Menue(void *const rb);

#pragma OT(9)
#endif

/*******************(C)COPYRIGHT 2013 STC *****END OF FILE****/
