#include "app_car_task.h"
#include "app_bms.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "app_mppt.h"
extern void init_uart_manage(void);
// 呼吸道任务线程
osThreadId_t heart_led_handle;
const osThreadAttr_t heart_led_attributes = {
    .name = "HeartLedTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
// modbus读取线程
osThreadId_t bms_read_handle;
const osThreadAttr_t bms_read_attributes = {
    .name = "BmsReadTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

// 心跳LED闪烁任务线程
void heart_beat_thread(void *argument)
{
    for (;;)
    {
        // 心跳LED闪烁
        HAL_GPIO_TogglePin(HEART_LED_GPIO_Port, HEART_LED_Pin);
        osDelay(1000);
    }
}

/**
 * @brief BMS读取任务线程
 */
void bms_read_thread(void *argument)
{
    for (;;)
    {
        // 处理 BMS 逻辑
        process_bms_logic();
        // 处理 MPPT 逻辑
        process_mppt_logic();
        osDelay(100);
    }
}

// 初始化应用层任务模块
void init_app_car_task(void)
{
    specify_redirect_uart(&huart5);
    printf("\r\n[INFO] [BOARD] specify redirect printf to huart5\r\n");
    // 4g模块复位操作
    HAL_GPIO_WritePin(RESET_4G_GPIO_Port, RESET_4G_Pin, GPIO_PIN_RESET);   // RESET_4G 引脚拉低
    HAL_GPIO_WritePin(RELOAD_4G_GPIO_Port, RELOAD_4G_Pin, GPIO_PIN_RESET); // RELOAD 引脚拉低
    init_uart_manage();                                                    // 初始化 UART 管理模块，设置好串口和回调函数
    uart_manage_enable_dma_recv_by_name("shell");
    uart_manage_enable_dma_recv_by_name("4g");

    // 初始化modbus主机模块 (BMS、mppt等)
    init_modbus_master();
    // 呼吸道任务线程
    heart_led_handle = osThreadNew(heart_beat_thread, NULL, &heart_led_attributes);
    // modbus读取线程
    bms_read_handle = osThreadNew(bms_read_thread, NULL, &bms_read_attributes);
}