#include "app_car_task.h"
#include "app_bms.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "app_mppt.h"

#include "sx126x-board.h" 
#include "radio.h"
#include <string.h>
extern void SX126x_DIO1_Interrupt_Handle(void);
// 1. 定义事件回调函数
static void OnTxDone(void) {
    printf("[LoRa Event] DIO1 Triggered: Tx Done!\r\n");
    Radio.Standby(); // 发送完进入待机
}

static void OnTxTimeout(void) {
    printf("[LoRa Event] Tx Timeout!\r\n");
}
// static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {}
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    // 把收到的字节数组变成字符串打印出来
    char rx_str[256];
    memset(rx_str, 0, sizeof(rx_str));
    memcpy(rx_str, payload, size);
    
    printf("\r\n=======================================\r\n");
    printf("[LoRa Event] Rx Done!\r\n");
    printf("[LoRa Event] Payload: %s\r\n", rx_str);
    printf("[LoRa Event] RSSI: %d dBm, SNR: %d dB\r\n", rssi, snr);
    printf("=======================================\r\n");
}
static void OnRxTimeout(void) {}
static void OnRxError(void) {}

// 将回调函数打包
static RadioEvents_t LoRaEvents = {
    .TxDone = OnTxDone,
    .RxDone = OnRxDone,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = OnRxTimeout,
    .RxError = OnRxError
};

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

// 声明将在下一步创建的 LoRa 任务句柄
extern osThreadId_t lora_task_handle;

// HAL库的外部中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 假设你的 DIO1 接在 PD3
    if (GPIO_Pin == GPIO_PIN_3) 
    {
        printf("[Debug] DIO1 EXTI IRQ Triggered!!\r\n");
        SX126x_DIO1_Interrupt_Handle();
        // 给 LoRa 任务发送一个信号 (0x01)，唤醒它去处理中断
        if (lora_task_handle != NULL) {
            osThreadFlagsSet(lora_task_handle, 0x01);
        }
    }
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

// 2. 任务句柄与属性
osThreadId_t lora_task_handle;
const osThreadAttr_t lora_task_attributes = {
    .name = "LoRaTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

// 3. LoRa 主线程
void lora_thread(void *argument)
{
    printf("\r\n[LoRa Task] Initializing Radio State Machine (RX Mode)...\r\n");

    Radio.Init(&LoRaEvents);
    Radio.SetChannel(868000000); // 必须和发送端频率一模一样！

    // 配置为接收模式 (参数必须和 TX 端完全对应: BW=125kHz, SF=7, CR=4/5)
    Radio.SetRxConfig(MODEM_LORA, 0, 7, 1, 0, 8, 0, false, 0, true, false, 0, false, true);

    printf("[LoRa Task] Radio Init Complete. Entering Continuous RX mode...\r\n");
    
    // 启动连续接收模式，0 代表不超时
    Radio.Rx(0); 

    for (;;)
    {
        // 依然是死等 DIO1 中断
        uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
        
        if (flags == 0x01) {
            Radio.IrqProcess(); 
        }
    }
}

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
        // process_bms_logic();
        // // 处理 MPPT 逻辑
        // process_mppt_logic();
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
    // LoRa主线程
    lora_task_handle = osThreadNew(lora_thread, NULL, &lora_task_attributes);
}