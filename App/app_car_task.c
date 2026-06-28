#include "app_car_task.h"
#include "app_bms.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "printf_redirect.h"
#include "app_mppt.h"
#include "app_gps.h"
#include "app_laser.h"
#include "app_sys_monitor.h"
#include "mongoose_callbacks.h"
#include "mongoose_config.h"
#include "mongoose.h"
#include "iwdg.h"
// 呼吸道任务线程
osThreadId_t heart_led_handle;
const osThreadAttr_t heart_led_attributes = {
    .name = "HeartLedTask",
    .stack_size = 1024 * 2,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
// modbus读取线程
osThreadId_t bms_read_handle;
const osThreadAttr_t bms_read_attributes = {
    .name = "BmsReadTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
// gps待机线程
osThreadId_t gps_standby_handle;
const osThreadAttr_t gps_standby_attributes = {
    .name = "GPSStandby",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal1,
};
// laser激光测距线程
osThreadId_t laser_handle;
const osThreadAttr_t laser_attributes = {
    .name = "LaserTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
// 状态监控线程
osThreadId_t sys_monitor_handle;
const osThreadAttr_t sys_monitor_attributes = {
    .name = "SysMonitorTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
// 电源控制任务线程
static osThreadId_t power_ctrl_handle;
static const osThreadAttr_t power_ctrl_attributes = {
    .name = "PowerCtrlTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityAboveNormal,
};
osThreadId_t web_server_handle;
const osThreadAttr_t web_server_attributes = {
    .name = "WebServerTask",
    .stack_size = 1024 * 8, 
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
// GPS/待机线程
void gps_standby_thread(void *argument)
{
    // gps / 待机初始化
    gps_rtc_app_init();
    for (;;)
    {
        process_gps_logic();

        osDelay(1000);
    }
}
// 激光测距线程
void laser_thread(void *argument)
{
    for (;;)
    {
        process_laser_logic();
        osDelay(1000);
    }
}
// 状态监控线程
void sys_monitor_thread(void *argument)
{
    for (;;)
    {
        process_sys_monitor_logic();
        net_ping_monitor();
        osDelay(1000);
    }
}
// 电源控制任务线程
void power_control_thread(void *argument)
{
    HAL_GPIO_WritePin(BRIDGE_EN_GPIO_Port, BRIDGE_EN_Pin, GPIO_PIN_SET);
    for (;;)
    {
        power_control_logic();

        osDelay(1000);
    }
}
void web_server_thread(void *argument)
{
    extern struct netif gnetif;
    {
      uint32_t start_tick = osKernelGetTickCount();
      while (!netif_is_link_up(&gnetif) || !netif_is_up(&gnetif))
      {
        if ((osKernelGetTickCount() - start_tick) > 60000U)
        {
          /* 网络在 60s 内未就绪，执行系统重启 */
        //   system_reset();
        HAL_NVIC_SystemReset();
        }
        osDelay(100);
      }
    }
    mongoose_init();
    
    for (;;)
    {
        //看门狗喂狗
        HAL_IWDG_Refresh(&hiwdg1);
        mongoose_poll();
        
        osDelay(10); 
    }
}

// 初始化应用层任务模块
void init_app_car_task(void)
{
    specify_redirect_uart(&huart5);
    printf("\r\n[INFO] [BOARD] specify redirect printf to huart5\r\n");
    init_uart_manage(); // 初始化串口管理模块（gps、laser01/02）
    mb_init_reg();
#if (CURRENT_CRANE_TYPE == CRANE_TYPE_FLAT_TOP)
    init_modbus_master(); // 初始化modbus主机模块 (BMS、mppt等)
    // modbus读取线程
    bms_read_handle = osThreadNew(bms_read_thread, NULL, &bms_read_attributes);
    // 休眠待机线程
    gps_standby_handle = osThreadNew(gps_standby_thread, NULL, &gps_standby_attributes);
#endif

    // 呼吸道任务线程
    heart_led_handle = osThreadNew(heart_beat_thread, NULL, &heart_led_attributes);
    
    // 激光测距线程
    laser_handle = osThreadNew(laser_thread, NULL, &laser_attributes);
    // 状态监控线程
    sys_monitor_handle = osThreadNew(sys_monitor_thread, NULL, &sys_monitor_attributes);
    // 电源控制任务线程
    power_ctrl_handle = osThreadNew(power_control_thread, NULL, &power_ctrl_attributes);
    // Web ota线程
    web_server_handle = osThreadNew(web_server_thread, NULL, &web_server_attributes);
}