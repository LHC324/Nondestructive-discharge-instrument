/********************************** (C) COPYRIGHT *******************************
 * File Name          : dev_flash.h
 * Author             : XZH
 * Version            : V1.00
 * Date               : 2020/07/09
 * Description        : iap
 *
 *
 *******************************************************************************/

#ifndef __DEV_SPI_H
#define __DEV_SPI_H

/*******************************************************************************/
/* Include */
#include "config.h"
/*******************************************************************************/
#if defined(USING_SIMULATE)
#define W25Qx_USING_DEBUG 0
#else
#define W25Qx_USING_DEBUG 0
#define W25Qx_UART &Uart1
#endif
/******************************************************************************/
/* Typedef enum */
#define FLASH_WRITE_ENABLE_CMD 0x06
#define FLASH_WRITE_DISABLE_CMD 0x04
#define FLASH_READ_SR1_CMD 0x05
#define FLASH_READ_SR2_CMD 0x35
#define FLASH_WRITE_SR_CMD 0x01
#define FLASH_READ_DATA 0x03
#define FLASH_FASTREAD_DATA 0x0b
#define FLASH_WRITE_PAGE 0x02
// #define FLASH_ERASE_PAGE 0x81
#define FLASH_ERASE_SECTOR 0x20
#define FLASH_ERASE_64KB_BLOCK_CMD 0xd8
#define FLASH_ERASE_32KB_BLOCK_CMD 0x52
#define FLASH_ERASE_CHIP 0xc7
#define FLASH_POWER_DOWN 0xb9
#define FLASH_RELEASE_POWER_DOWN 0xab
#define FLASH_READ_DEVICE_ID 0x90
#define FLASH_READ_JEDEC_ID 0x9f

#define DEV_FLASH_FLASH_SIZE (8UL * 1024UL * 1024UL) //  8MB
#define DEV_FLASH_64KB_BLOCK_SIZE 65536U             // 64-Kbyte
#define DEV_FLASH_32KB_BLOCK_SIZE 32768U             // 32-Kbyte
#define DEV_FLASH_SECTOR_SIZE 4096U                  // 4-Kbyte
#define DEV_FLASH_PAGE_SIZE 256U                     // 256 bytes

#define FLASH_CS_LOW (P22 = 0)
#define FLASH_CS_HIGH (P22 = 1)

/******************************************************************************/

/******************************************************************************/
/* Extern Variate */
/******************************************************************************/

/******************************************************************************/
/* Extern Function */
extern uint16_t dev_flash_read_device_id(void);
extern uint32_t dev_flash_read_jedec_id(void);
extern uint8_t dev_flash_read_sr(uint8_t regs);
extern void dev_flash_read_bytes(uint8_t *pdat, uint32_t addr, uint16_t read_length);
extern void dev_flash_write_page(uint8_t *pdat, uint32_t write_addr, uint16_t write_length);
extern void dev_flash_write_bytes_nocheck(uint8_t *pdat, uint32_t write_addr, uint16_t write_length);
// extern void dev_flash_write(uint8_t *pdat, uint32_t addr, uint16_t len);
// extern void dev_flash_erase_page(uint32_t start_addr);
extern void dev_flash_erase_sector(uint32_t start_addr);
extern void dev_flash_erase_block(uint32_t start_addr, uint8_t cmd);
extern void dev_flash_erase_chip(void);
extern void dev_flash_auto_adapt_erase(uint32_t start_addr, uint32_t len);
/******************************************************************************/

#endif