#include "app_car_task.h"
#include "app_bms.h"       
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

osThreadId_t bms_read_handle;
const osThreadAttr_t bms_read_attributes = {
    .name = "BmsReadTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/**
 * @brief BMS读取任务线程
 */
void bms_read_thread(void *argument)
{
    for (;;)
    {
        
        process_bms_logic();

        osDelay(5000); 
    }
}

// 初始化应用层任务模块
void init_app_car_task(void)
{
    specify_redirect_uart(&huart5);
    printf("\r\n[INFO] [BOARD] specify redirect printf to huart5\r\n");
    // 1. 初始化modbus主机模块 (BMS、mppt等)
    init_modbus_master();
    
    bms_read_handle = osThreadNew(bms_read_thread, NULL, &bms_read_attributes);
}