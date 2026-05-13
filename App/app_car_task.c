#include "app_car_task.h"
#include "app_bms.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "ethernetif.h"
//呼吸道任务线程
osThreadId_t heart_led_handle;
const osThreadAttr_t heart_led_attributes = {
    .name = "HeartLedTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
//modbus读取线程
osThreadId_t bms_read_handle;
const osThreadAttr_t bms_read_attributes = {
    .name = "BmsReadTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

void init_modtcp_server(void);
void init_modtcp_server(void)
{
    MX_LWIP_Init();
    extern struct netif gnetif;
    while (!netif_is_link_up(&gnetif) || !netif_is_up(&gnetif))
    {
        osDelay(100);
    }
    modbus_tcp_init_server();
}
// 心跳LED闪烁任务线程
void heart_beat_thread(void *argument)
{
    // 初始化modbus tcp server模块
    init_modtcp_server();
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

        process_bms_logic();

        osDelay(100);
    }
}

// 初始化应用层任务模块
void init_app_car_task(void)
{
    specify_redirect_uart(&huart5);
    printf("\r\n[INFO] [BOARD] specify redirect printf to huart5\r\n");
    
    // 初始化modbus主机模块 (BMS、mppt等)
    init_modbus_master();
    //呼吸道任务线程
    heart_led_handle = osThreadNew(heart_beat_thread, NULL, &heart_led_attributes);
    //modbus读取线程
    bms_read_handle = osThreadNew(bms_read_thread, NULL, &bms_read_attributes);
}