#include "app_car_task.h"
#include "app_bms.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "app_mppt.h"

#include "sx126x-board.h" 

void Test_LoRa_SPI(void) 
{
    printf("\r\n--- LoRa SPI HAL Test Start ---\r\n");
    
    // 1. 极其重要：使能射频前端放大器 (RF_EN = PD7 拉高)
    HAL_GPIO_WritePin(LORA1_RF_EN_GPIO_Port, LORA1_RF_EN_Pin, GPIO_PIN_SET);
    
    // 2. 硬件硬复位芯片
    SX126xReset();
    
    // 3. 读取 SyncWord 寄存器 (地址 0x0740)
    SX126xWaitOnBusy();
    SX126xSetNss(0);      // 开启片选
    SX126xSpiInOut(0x1D); // 发送“读寄存器”指令 (READ_REGISTER)
    SX126xSpiInOut(0x07); // 发送地址高字节
    SX126xSpiInOut(0x40); // 发送地址低字节
    SX126xSpiInOut(0x00); // 发送一个哑字节(NOP)，给芯片留出反应时间
    uint8_t test_val = SX126xSpiInOut(0x00); // 这时从 MISO 线上读回真正的寄存器值
    SX126xSetNss(1);      // 关闭片选
    
    // 4. 打印结果
    printf("[LoRa] Read Reg 0x0740: 0x%02X\r\n", test_val);
    
    if (test_val == 0x14 || test_val == 0x34) {
        printf("[LoRa] SUCCESS! SPI and HAL configuration are perfectly working!\r\n");
    } else {
        printf("[LoRa] ERROR! Received 0x%02X. Check SPI4 settings or wiring!\r\n", test_val);
    }
    printf("--- LoRa SPI HAL Test End ---\r\n\r\n");
}

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
    // 测试LoRa的SPI接口是否正常
    Test_LoRa_SPI();
// HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_RESET);
    // 初始化modbus主机模块 (BMS、mppt等)
    init_modbus_master();
    // 呼吸道任务线程
    heart_led_handle = osThreadNew(heart_beat_thread, NULL, &heart_led_attributes);
    // modbus读取线程
    bms_read_handle = osThreadNew(bms_read_thread, NULL, &bms_read_attributes);
}