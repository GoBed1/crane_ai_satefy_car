#include "sx126x-board.h"
#include "main.h"  // 必须引入 main.h 以获取 HAL 库环境及引脚宏定义
#include "spi.h"   // 引入 CubeMX 生成的 spi.h，里面声明了 hspi4
#include "gpio.h"  // 引入 CubeMX 生成的 gpio.h，里面声明了 HAL_GPIO_WritePin 和 HAL_GPIO_ReadPin
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include <stdio.h>
static DioIrqHandler *dio1IrqCallback = NULL;
// 声明外部定义的 SPI4 句柄 (确保你的 main.c 或 spi.c 中有这个变量)
extern SPI_HandleTypeDef hspi4;

//=============================================================================
// 1. SPI 收发一个字节 (替代老代码里的 HALSpi1InOut)
//=============================================================================
uint8_t SX126xSpiInOut( uint8_t data ) 
{
    uint8_t rxData = 0;
    
    // 使用 HAL 库的双向收发函数
    // 参数：SPI句柄, 发送缓冲区指针, 接收缓冲区指针, 数据长度(1字节), 超时时间(10ms)
    HAL_SPI_TransmitReceive( &hspi4, &data, &rxData, 1, 10 );
    
    return rxData;
}

//=============================================================================
// 2. 控制 NSS 片选引脚 (PE15)
//=============================================================================
void SX126xSetNss( uint8_t lev ) 
{
    if( lev == 1 ) 
    {
        // 输出高电平：取消选中 LoRa 芯片
        HAL_GPIO_WritePin( LORA1_NSS_GPIO_Port, LORA1_NSS_Pin, GPIO_PIN_SET );   
    } 
    
    else 
    {
        // 输出低电平：选中 LoRa 芯片，准备 SPI 传输
        HAL_GPIO_WritePin( LORA1_NSS_GPIO_Port, LORA1_NSS_Pin, GPIO_PIN_RESET ); 
    }
}

//=============================================================================
// 3. 硬件复位芯片 (RESET: PD4)
//=============================================================================
void SX126xReset( void ) 
{
    HAL_Delay( 20 );
    // 拉低复位引脚 (LoRa芯片低电平硬复位)
    HAL_GPIO_WritePin( LORA1_RESET_GPIO_Port, LORA1_RESET_Pin, GPIO_PIN_RESET ); 
    HAL_Delay( 40 );
    // 拉高释放复位
    HAL_GPIO_WritePin( LORA1_RESET_GPIO_Port, LORA1_RESET_Pin, GPIO_PIN_SET );   
    HAL_Delay( 20 );
}

//=============================================================================
// 4. 等待 BUSY 引脚变低 (BUSY: PB7)
//=============================================================================
void SX126xWaitOnBusy( void ) 
{
    uint32_t timeout = 0;
    
    // 轮询 PB7 的电平状态，高电平代表芯片忙碌
    while( HAL_GPIO_ReadPin( LORA1_BUSY_GPIO_Port, LORA1_BUSY_Pin ) == GPIO_PIN_SET ) 
    {
        // 考虑到你后续在 FreeRTOS 任务中调用，这里最好用 osDelay(1)
        // 但如果是在进入内核调度前测试，可以用底层的 HAL_Delay(1)
        // osDelay( 1 ); 
        timeout++;
        
        // 防呆设计：如果等了超过 1000ms 芯片还是忙，说明硬件或引脚配置有问题，强制跳出，防止 RTOS 死锁
        if( timeout > 5000000 ) 
        {
            // 如果你的串口开了，可以加个打印提示
             printf("[LoRa ERROR] BUSY Timeout! Check Hardware!\r\n");
            break; 
        }
    }
}

//=============================================================================
// 5. 毫秒延时函数桥接
//=============================================================================
void SX126xDelayMs( uint32_t ms ) 
{
    osDelay( ms ); // 直接映射到系统的毫秒延时
}
//=============================================================================
// 6. 补充 Semtech 驱动库需要的底层“桩函数”(Stubs)
//=============================================================================

// IO 与中断初始化 (我们在 CubeMX 和 HAL_GPIO_EXTI_Callback 中已实现，故留空)
void SX126xIoInit( void ) {}
void SX126xIoIrqInit( DioIrqHandler dioIrq ) {
    dio1IrqCallback = dioIrq; // 保存官方库传进来的中断回调指针
}
void SX126x_DIO1_Interrupt_Handle(void) {
    if (dio1IrqCallback != NULL) {
        dio1IrqCallback(NULL); // 调用它，官方库就会把 IrqFired 设为 true!
    }
}
void SX126xIoDeInit( void ) {}
void SX126xIoDbgInit( void ) {}

// 定时器操作 (由于 FreeRTOS 机制取代了死等轮询，这里的硬件定时器留空即可)
void SX126xTimerInit(void) {}
void SX126xSetTxTimerValue(uint32_t nMs) {}
void SX126xTxTimerStart(void) {}
void SX126xTxTimerStop(void) {}
void SX126xSetRxTimerValue(uint32_t nMs) {}
void SX126xRxTimerStart(void) {}
void SX126xRxTimerStop(void) {}

// 如果之前没有加延时函数，把这个也带上：
void delay_ms(uint32_t ms) {
    HAL_Delay(ms); // 如果在 RTOS 里想出让 CPU，可以换成 osDelay(ms);
}