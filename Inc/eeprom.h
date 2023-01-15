#ifndef __EEPROM_H_
#define __EEPROM_H_

#include "config.h"

#define WT_30M 0x80
#define WT_24M 0x81
#define WT_20M 0x82
#define WT_12M 0x83
#define WT_6M 0x84
#define WT_3M 0x85
#define WT_2M 0x86
#define WT_1M 0x87

// 使用的时候，将对应宏前面的注释符号去掉即可。
// #define IAP_OFFSET 0x2000 //STC8A8K08S4A12
// #define IAP_OFFSET 0x4000 //STC8A8K16S4A12
// #define IAP_OFFSET 0x6000 //STC8A8K24S4A12
// #define IAP_OFFSET 0x8000 //STC8A8K32S4A12
// #define IAP_OFFSET 0xA000 //STC8A8K40S4A12
// #define IAP_OFFSET 0xC000 //STC8A8K48S4A12
// #define IAP_OFFSET 0xE000 //STC8A8K56S4A12
// #define IAP_OFFSET 0xF000 //STC8A8K60S4A12
// #define IAP_OFFSET 0xF000 //STC8A8K60S4A12
/*自己计算给出*/
// #define IAP_OFFSET 0xF300 //STC8A8K64S4A12-3k
// #define IAP_OFFSET 0xF800 //STC8A8K64S4A12-2k
// #define IAP_OFFSET 0xFC00 // STC8A8K64S4A12-1k
// #define IAP_OFFSET 0xFE00 //STC8A8K64S4A12-0.5k
// #define IAP_OFFSET 0x0000 // STC8A8K64S4A12-64k

#define IAP_OFFSET (64U * 1024U - EEPROM_SIZE()) // STC8A8K64S4A12-64k

/**
 * @brief	配置STC8单片机EEPROM的等待时间
 * @details
 * @param	None
 * @retval	None
 */
static void IapConfigWaitTime();

/**
 * @brief	关闭iap功能
 * @details
 * @param	None
 * @retval	None
 */
extern void IapIdle();

/**
 * @brief	从iap的e2prom中读出一个字节数据
 * @details
 * @param	addr: 指定地址
 * @retval	None
 */
extern char IapRead(unsigned short addr);

/**
 * @brief	在iap的e2prom的指定地址写入一个字节数据
 * @details
 * @param	addr: 指定地址, dat: 数据
 * @retval	None
 */
extern void IapProgram(unsigned short addr, unsigned char dat);

/**
 * @brief	擦除指定地址的内容
 * @details	擦除每次按照512B进行，仅提供首地址即可
 * @param	addr:起始地址
 * @retval	None
 */
extern void IapErase(unsigned short addr);

/**
 * @brief	在指定地址写入多个字节数据
 * @details	写数据一般请按照一个扇区一个扇区的来写，不然在擦除数据的时候会造成数据丢失；一个扇区的大小是 512字节
 * @param	addr:开始地址, *Str: 指向字节数据指针, Len:长度
 * @retval	None
 */
extern void IapWrites(uint16_t addr, const uint8_t *str, uint16_t len);

/**s
 * @brief	从指定地址读取多个字节数据
 * @details
 * @param	addr:开始地址, *Str: 指向目标缓冲区指针, Len:长度
 * @retval	None
 */
extern void Iap_Reads(uint16_t addr, uint8_t *str, uint16_t len);

#endif