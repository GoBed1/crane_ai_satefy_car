#ifndef __SX126X_BOARD_H__
#define __SX126X_BOARD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "sx126x.h"
// typedef void ( *DioIrqHandler )( void );
//=============================================================================
// 基础硬件操作函数接口定义 (供驱动库和外部测试调用)
//=============================================================================

/**
 * @brief SPI 接口底层双向收发一个字节
 * @param data 要发送的字节
 * @return uint8_t 接收到的字节
 */
uint8_t SX126xSpiInOut( uint8_t data );

/**
 * @brief 控制 NSS (片选) 引脚电平
 * @param lev 1: 拉高(取消选中), 0: 拉低(选中芯片)
 */
void SX126xSetNss( uint8_t lev );

/**
 * @brief 硬件复位 SX1262 芯片
 */
void SX126xReset( void );

/**
 * @brief 阻塞等待 BUSY 引脚变低 (忙状态检查)
 */
void SX126xWaitOnBusy( void );

/**
 * @brief 驱动库内部需要的毫秒延时函数
 * @param ms 延时时间(毫秒)
 */
void SX126xDelayMs( uint32_t ms );

#ifdef __cplusplus
}
#endif

#endif // __SX126X_BOARD_H__