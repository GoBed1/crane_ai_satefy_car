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
// 将此宏设为 1，当前板子编译为【发送端】；设为 0，编译为【接收端】
#define LORA_IS_TX_NODE 0
extern void SX126x_DIO1_Interrupt_Handle(void);
// 1. 定义事件回调函数
static void OnTxDone(void)
{
    printf("[LoRa Event] DIO1 Triggered: Tx Done!\r\n");
    Radio.Standby(); // 发送完进入待机
}

static void OnTxTimeout(void)
{
    printf("[LoRa Event] Tx Timeout!\r\n");
}
// static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {}
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
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
    .RxError = OnRxError};

void Test_LoRa_SPI(void)
{
    printf("\r\n--- LoRa SPI HAL Test Start ---\r\n");

    // 1. 极其重要：使能射频前端放大器 (RF_EN = PD7 拉高)
    HAL_GPIO_WritePin(LORA1_RF_EN_GPIO_Port, LORA1_RF_EN_Pin, GPIO_PIN_SET);

    // 2. 硬件硬复位芯片
    SX126xReset();

    // 3. 读取 SyncWord 寄存器 (地址 0x0740)
    SX126xWaitOnBusy();
    SX126xSetNss(0);                         // 开启片选
    SX126xSpiInOut(0x1D);                    // 发送“读寄存器”指令 (READ_REGISTER)
    SX126xSpiInOut(0x07);                    // 发送地址高字节
    SX126xSpiInOut(0x40);                    // 发送地址低字节
    SX126xSpiInOut(0x00);                    // 发送一个哑字节(NOP)，给芯片留出反应时间
    uint8_t test_val = SX126xSpiInOut(0x00); // 这时从 MISO 线上读回真正的寄存器值
    SX126xSetNss(1);                         // 关闭片选

    // 4. 打印结果
    printf("[LoRa] Read Reg 0x0740: 0x%02X\r\n", test_val);

    if (test_val == 0x14 || test_val == 0x34)
    {
        printf("[LoRa] SUCCESS! SPI and HAL configuration are perfectly working!\r\n");
    }
    else
    {
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
    if (GPIO_Pin == LORA1_DIO1_Pin)
    {
        printf("[Debug] DIO1 EXTI IRQ Triggered!!\r\n");
        SX126x_DIO1_Interrupt_Handle();
        // 给 LoRa 任务发送一个信号 (0x01)，唤醒它去处理中断
        if (lora_task_handle != NULL)
        {
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

// 3.LoRa 主线程 (通过宏定义区分 TX 和 RX)
/*
设置Lora参数时需要保持tx和rx完全一致的参数

*/
void lora_thread(void *argument)
{
    printf("\r\n[LoRa Task] Initializing Radio State Machine...\r\n");

    Radio.Init(&LoRaEvents);
    Radio.SetChannel(868000000); // 868 MHz

#if LORA_IS_TX_NODE
    // ------均衡模式，具体参数设置查看最下面Tx详解---------------------------------------------------
    Radio.SetTxConfig(MODEM_LORA, 14, 0, 0, 7, 1, 8, false, true, 0, 0, false, 3000);
    printf("[LoRa Task] TX Mode Configured. SF=7, Power=14dBm.\r\n");
    uint32_t packet_counter = 0;
    char send_buf[64];

    for (;;)
    {
        // 格式化带有自增序号的字符串，方便测试丢包率
        snprintf(send_buf, sizeof(send_buf), "STM32H743_Test_Packet_%04lu", packet_counter++);

        printf("\r\n[LoRa TX] Sending: %s\r\n", send_buf);
        Radio.Send((uint8_t *)send_buf, strlen(send_buf));

        // 阻塞等待发送完成中断
        uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
        if (flags == 0x01)
        {
            Radio.IrqProcess();
        }

        // 重要：SF12发包非常耗时，且需要遵守占空比限制，这里间隔 5 秒再发下一包
        osDelay(3000);
    }

#else
    // ---------------------------------------------------------
    // 【接收端配置】- 均衡距离模式 (参数必须与TX完全一致)
    // ---------------------------------------------------------
    //具体参数设置查看最下面Rx详解
    Radio.SetRxConfig(MODEM_LORA, 0, 7, 1, 0, 8, 0, false, 0, true, false, 0, false, true);

    printf("[LoRa Task] RX Continuous Mode Configured. Waiting for packets...\r\n");

    // 启动连续接收模式
    Radio.Rx(0);

    for (;;)
    {
        // 阻塞等待接收完成中断
        uint32_t flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
        if (flags == 0x01)
        {
            Radio.IrqProcess();
        }
    }
#endif
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

/* ==================================================================================
 * 【Semtech LoRa 驱动参数详解 —— Radio.SetRxConfig】
 * ==================================================================================
 * 代码定义：
 * Radio.SetRxConfig( MODEM_LORA, 0, 7, 1, 0, 8, 0, false, 0, true, false, 0, false, true );
 * * 参数顺序列形解析如下：
 * ----------------------------------------------------------------------------------
 * 1. modem          = MODEM_LORA  -> 调制解调器模式：选择 LoRa 调制（可选 MODEM_FSK）
 * * 2. bandwidth      = 0           -> 信号带宽 (BW)：0 代表 125 kHz（最通用平衡点）
 * [ 1: 250 kHz | 2: 500 kHz ] 越窄传得越远
 * * 3. datarate       = 7           -> 扩频因子 (SF)：设置为 SF7（高速率、低延迟、省电）
 * [ 可调范围: 5 ~ 12 ] 越大抗干扰越强、传得越远、发包越慢
 * * 4. coderate       = 1           -> 纠错编码率 (CR)：1 代表 4/5 纠错冗余度
 * [ 1: 4/5 | 2: 4/6 | 3: 4/7 | 4: 4/8 ] 越大抗噪越强
 * * 5. bandwidthAfc   = 0           -> FSK 自动频率控制带宽：LoRa 模式下不使用，固定填 0
 * * 6. preambleLen    = 8           -> 前导码长度 (Preamble)：单位为 LoRa Symbol。这里设为 8
 * 硬件底层会自动+4。大 SF 建议拉长（如 12）利于远距离唤醒
 * * 7. symbTimeout    = 0           -> Symbol 超时时间：仅在单次接收(RxSingle)模式下有效。
 * 由于开启了连续接收(rxContinuous=true)，此值在底层会被忽略
 * * 8. fixLen         = false       -> 固定包长模式：false 代表显式报头模式（Explicit Header）
 * 数据包会自动携带长度信息，支持变长收发
 * * 9. payloadLen     = 0           -> 固定包长大小：由于上面设置了变长(false)，这里填 0 即可
 * * 10. crcOn         = true        -> CRC 校验开关：true 代表开启硬件 CRC 循环冗余校验。
 * 若空中收到错码包，底层直接报 RxError 并丢弃，确保数据 100% 正确
 * * 11. freqHopOn     = false       -> 内包跳频 (FHSS) 开关：false 代表关闭跳频通信
 * * 12. hopPeriod     = 0           -> 跳频周期：由于跳频关闭，这里固定填 0
 * * 13. iqInverted    = false       -> IQ 信号翻转：false 代表不翻转。
 * [ 节点点对点互发：必须同为 false | 网关组网：网关发节点收设为 true ]
 * * 14. rxContinuous  = true        -> 连续接收模式 (Continuous RX)：true 代表芯片一直开启射频监听，
 * 收到包并处理完中断后，自动保持在 RX 状态继续监听，不会退出
 * ================================================================================== */

 /* ==================================================================================
 * 【Semtech LoRa 驱动参数详解 —— Radio.SetTxConfig】
 * ==================================================================================
 * 代码定义：
 * Radio.SetTxConfig( MODEM_LORA, 14, 0, 0, 7, 1, 8, false, true, 0, 0, false, 3000 );
 * * 参数顺序列形解析如下：
 * ----------------------------------------------------------------------------------
 * 1. modem          = MODEM_LORA  -> 调制解调器模式：选择 LoRa 发送模式（可选 MODEM_FSK）
 * * 2. power          = 14          -> 发送输出功率 (Power)：单位为 dBm。
 * [ Ra-01-SH 标准版 ]：最高可填 22 (即 22dBm 拉满)。这里填 14 属于温和均衡模式。
 * [ 注意 ]：如果是大功率带功放的 Ra-01-SH-P 版，此处绝对不能超过 3，否则会烧毁外置功放！
 * * 3. fdev           = 0           -> 频率偏移 (Frequency Deviation)：仅在 GFSK 模式下有用，LoRa 模式下固定填 0
 * * 4. bandwidth      = 0           -> 信号带宽 (BW)：0 代表 125 kHz。（收发两端必须完全一致！）
 * [ 1: 250 kHz | 2: 500 kHz ] 带宽越窄，传输距离越远，但发包会变慢
 * * 5. datarate       = 7           -> 扩频因子 (SF)：设置为 SF7。（收发两端必须完全一致！）
 * [ 可调范围: 5 ~ 12 ] SF 越小速率越快、省电；SF 越大抗干扰越强、传得越远、但包空中时间呈指数级暴涨
 * * 6. coderate       = 1           -> 纠错编码率 (CR)：1 代表 4/5 冗余度。（收发两端必须完全一致！）
 * [ 1: 4/5 | 2: 4/6 | 3: 4/7 | 4: 4/8 ] 增大到 4 (4/8) 可在极限边缘环境下极大地提升数据解调准确性
 * * 7. preambleLen    = 8           -> 前导码长度 (Preamble)：单位为 LoRa Symbol。这里设为 8
 * 硬件在底层会自动在此基础上 +4 符号。大 SF (SF10-SF12) 远距离测试时，建议将此项拉长到 12 或 16
 * * 8. fixLen         = false       -> 固定包长模式：false 代表显式报头模式（Explicit Header）。
 * 允许发送动态变长的数据包（长度信息会自动打包进空中数据包的 Header 里）
 * * 9. crcOn         = true        -> CRC 校验开关：true 代表开启发送包数据硬件 CRC。
 * 开启后，空中数据包尾部会带上校验码，接收端会自动校验，保证接收数据 100% 准确
 * * 10. freqHopOn     = false       -> 内包跳频 (FHSS) 开关：false 代表关闭发送跳频通信
 * * 11. hopPeriod     = 0           -> 跳频周期：由于跳频关闭，这里固定填 0
 * * 12. iqInverted    = false       -> IQ 信号翻转：false 代表不翻转。（收发两端必须完全一致！）
 * [ 节点对传：必须同为 false | 网关下行组网：网关发节点收才会设为 true ]
 * * 13. timeout       = 3000        -> 发送硬件超时时间：单位为毫秒 (ms)。
 * 开启发送后，如果因为硬件异常等原因过了 3000ms 还没发完，会强制退出并触发 OnTxTimeout 回调
 * [ 注意 ]：如果后续调大扩频因子到 SF12，由于空中时间极长，此超时时间务必增加到 5000 毫秒以上！
 * ================================================================================== */


/* ==================================================================================
 * 【起重机/吊钩多设备防串台与调优修改指南】
 * 1. 防止串台：
 * （1）推荐：修改发送和接收端的 Radio.SetChannel(频率); 建议两组设备频点错开 1MHz （A：868000000 | B：869000000）
 * （2）更改同步字 (SyncWord)手动向 SX1262 的 0x0740 寄存器写入一个自定义的网络 ID 。现：0x1424
 * （3）软件应用层隔离： A:payload[0] == 'A'  |  B:payload[0] == 'B'
 * 注：策略 2 和 3 虽然能防止错乱，但两台设备如果碰巧在同一毫秒按下发送键，空中的电磁波依然会“撞车”导致双双丢包。故推荐（1）
 * 2. 提升准确性与穿透力：将第 4 参数(CR)改为 4 (4/8)；
 * 3. 增大日常测距距离：将第 3 参数(SF)从 7 改为 9 或 10（注意：SF 增大后发包时间会变长，需增大延时）
 * 【起重机/吊钩多设备测试 】：
 * 4. 收发握手：接收端的 Radio.SetRxConfig(...) 里的【带宽(0)、SF(7)、CR(1)、前导码(8)、CRC(true)、IQ(false)】
 *    这 6 大物理层核心参数，必须与发送端的这行配置做到 100% 完全镜像对齐，否则绝对无法成功接收！
 * 5. RadioSetTxConfig中，内部强行加了这一句 if(power > 3){ power = 3; } 进行限流保护，
 *    上层无论设置多大功率（比如填 14 甚至 22），芯片在底层都会被死死限制在 3dBm 发射，从而极大地影响你的测试距离。（在RadioSetTxConfig注释即可）
 * ================================================================================== */


/* ==================================================================================
 * 结合 Ra-01-SH（标准版，最大22dBm） 的硬件特性，定制了 4 套 最经典的 LoRa 参数模板
 * （1）均衡模式：速度快，功耗低，不占信道。这是官方默认。
 * 【TX RX 配置】: Power=14, BW=125k(0), SF=7, CR=4/5(1), Preamble=8
 * Radio.SetTxConfig(MODEM_LORA, 14, 0, 0, 7, 1, 8, false, true, 0, 0, false, 3000);
 * Radio.SetRxConfig(MODEM_LORA, 0, 7, 1, 0, 8, 0, false, 0, true, false, 0, false, true);
 * （2）极限远距模式：穿透好，但发包极慢，一秒钟只能发一包，容易堵塞信道
 * 【TX RX 配置】: Power=22(拉满), BW=125k(0), SF=12(极远), CR=4/8(4,满纠错), Preamble=12(加长)
 * Radio.SetTxConfig(MODEM_LORA, 22, 0, 0, 12, 4, 12, false, true, 0, 0, false, 5000);
 * Radio.SetRxConfig(MODEM_LORA, 0, 12, 4, 0, 12, 0, false, 0, true, false, 0, false, true);
 * （3）高速超低延时模式：把带宽拉宽到 500kHz。适合用来做“急停按钮”或高频姿态数据连续上报，几乎感受不到延迟，但距离最短
 * 【TX RX 配置】: Power=14, BW=500k(2,超宽带), SF=7, CR=4/5(1), Preamble=8
 * Radio.SetTxConfig(MODEM_LORA, 14, 0, 2, 7, 1, 8, false, true, 0, 0, false, 1000);
 * Radio.SetRxConfig(MODEM_LORA, 2, 7, 1, 0, 8, 0, false, 0, true, false, 0, false, true);
 * (4)强抗干扰穿墙模式：针对工地复杂环境（钢筋水泥多、电磁干扰大）的最佳折中方案。兼顾了长距离和响应速度。
 * 【TX RX 配置】: Power=22(拉满), BW=125k(0), SF=9(中等偏远), CR=4/8(4,满纠错), Preamble=8
 * Radio.SetTxConfig(MODEM_LORA, 22, 0, 0, 9, 4, 8, false, true, 0, 0, false, 3000);
 * Radio.SetRxConfig(MODEM_LORA, 0, 9, 4, 0, 8, 0, false, 0, true, false, 0, false, true);
 * 
 * 注意：底层 radio.c 里那个 if(power > 3) { power = 3; } 的代码注释掉，否则这 4 套模板的发射功率都会被锁死在极小的 3dBm。
 * ================================================================================== */
