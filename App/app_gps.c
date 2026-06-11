#include "app_gps.h"
#include "nmea.h"
#include "modbus_tcp_server_interface.h"
extern RTC_HandleTypeDef hrtc;

// 全局标志位
uint8_t s_gps_synced = 0;               // 授时同步标志（上位机授时或GPS锁星后置1）
volatile uint16_t is_soft_standby = 0;  // 软休眠状态标志（爆闪灯断电）

// 内部函数声明
static void sync_time_from_host(uint16_t host_time_hhmm);
static uint8_t rtc_is_wakeup_from_standby(void);
static void enter_standby(void);
static void set_alarm_b(uint8_t local_h, uint8_t local_m);
static void gps_sync_rtc_once(void);
static void print_internal_rtc_time(void);


// =========================================================================
// ==================== 上位机授时 与 软休眠核心逻辑 ====================
// =========================================================================

// 初始化RTC电源与时间管理
void rtc_power_init(void)
{
    // 解锁备份域访问权限
    HAL_PWR_EnableBkUpAccess();

    LOGI("[PWR] System Boot\r\n");

    if (rtc_is_wakeup_from_standby())
    {
        LOGI("[PWR] from hard standby\r\n");
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    }
    else
    {
        LOGI("[PWR] cold start\r\n");
        // 检查备份寄存器中是否有魔数 0x5AA5
        if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == RTC_BKP_MAGIC_NUMBER)
        {
            LOGI("[PWR] RTC time is valid (Coin Cell active)!\r\n");
            s_gps_synced = 1; // 纽扣电池有电，认为时间已同步
        }
        else
        {
            LOGI("[PWR] RTC time invalid! Waiting for Host or GPS to sync...\r\n");
            s_gps_synced = 0;
        }
    }

    uint16_t current_off_time = 0;
    mb_get_holding_reg_by_address(HOLDING_REG_POWER_OFF_TIME, &current_off_time);

    // 如果寄存器是 0，说明是刚开机或第一次出厂，填入默认的休眠时间
    if (current_off_time == 0)
    {
        mb_set_holding_reg_by_address(HOLDING_REG_POWER_OFF_TIME, POWER_OFF_DEFAULT);
        mb_set_holding_reg_by_address(HOLDING_REG_POWER_ON_TIME, POWER_ON_DEFAULT);
        LOGI("[PWR] Init default schedule: OFF=%02d:%02d, ON=%02d:%02d\r\n",
             POWER_OFF_DEFAULT >> 8, POWER_OFF_DEFAULT & 0xFF,
             POWER_ON_DEFAULT >> 8, POWER_ON_DEFAULT & 0xFF);
    }
}

// 将上位机发来的时间写入硬件 RTC
static void sync_time_from_host(uint16_t host_time_hhmm)
{
    uint8_t target_hour = (host_time_hhmm >> 8) & 0xFF;
    uint8_t target_minute = host_time_hhmm & 0xFF;

    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = target_hour;
    sTime.Minutes = target_minute;
    sTime.Seconds = 0; // 上位机授时，秒数直接清零
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    // 写入魔数，确保断电后纽扣电池能继续维持时间有效性
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_BKP_MAGIC_NUMBER);

    s_gps_synced = 1; // 标记时间已被上位机校准
    LOGI("[RTC] Time synced from Host: %02d:%02d\r\n", target_hour, target_minute);
}

// 循环每秒检测 (上位机时间同步差分检测 + 定时软硬休眠触发)
void rtc_power_schedule_check(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    uint8_t beijing_h = sTime.Hours; 
    uint8_t beijing_m = sTime.Minutes;
    uint16_t now_hhmm = (uint16_t)((beijing_h << 8) | beijing_m);

    // ------------------ 上位机授时检测逻辑 ------------------
    uint16_t reg_time = 0;
    mb_get_holding_reg_by_address(HOLDING_REG_RTC_TIME, &reg_time);
    
    int16_t now_minutes = beijing_h * 60 + beijing_m;
    int16_t reg_minutes = ((reg_time >> 8) & 0xFF) * 60 + (reg_time & 0xFF);
    
    int16_t diff = abs(reg_minutes - now_minutes);
    if (diff > 720) 
    {
        diff = 1440 - diff; // 处理 23:59 和 00:00 跨日跳变的差值
    }

    // 如果 Modbus 寄存器时间与单片机真实时间相差 >= 2分钟，说明上位机强制写了时间！
    if (diff >= 2)
    {
        sync_time_from_host(reg_time);
        now_hhmm = reg_time; // 立即使用新时间进行后续休眠判断
    }
    
    // 把当前准确的 RTC 时间刷回 Modbus 寄存器，供外部监控
    mb_set_holding_reg_by_address(HOLDING_REG_RTC_TIME, now_hhmm);

    // 安全保证：如果没有魔数（从没授过时），立刻退出，绝不瞎休眠
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != RTC_BKP_MAGIC_NUMBER)
    {
        return; 
    }

    // ------------------ 休眠触发逻辑 ------------------
    uint16_t off_hhmm = 0;
    uint16_t on_hhmm = 0;
    mb_get_holding_reg_by_address(HOLDING_REG_POWER_OFF_TIME, &off_hhmm);
    mb_get_holding_reg_by_address(HOLDING_REG_POWER_ON_TIME, &on_hhmm);

    static uint16_t last_trigger_hhmm = 0xFFFF;

    // 仅在时间跳变到指定分钟的那一刻触发一次
    if (now_hhmm != last_trigger_hhmm)
    {
        // 刚好到了关机时间 (如 19:00)
        if (now_hhmm == off_hhmm)
        {
            LOGI("[PWR] Auto Trigger: Enter Soft Sleep\r\n");
            // 触发软休眠：通知电源线程拉低供电引脚
            mb_set_coil_reg_by_address(COIL_REG_IS_ENTRY_SLEEP_CMD, 1);
            last_trigger_hhmm = now_hhmm; 
            
            // 兼容触发硬休眠 (如果上位机开启了硬休眠开关)
            uint8_t is_standby_flag = 0;
            mb_get_coil_reg_by_address(COIL_REG_CMD_IS_ENTRY_STANDBY, &is_standby_flag);
            if (is_standby_flag == 1) 
            {
                uint8_t on_h_local = (on_hhmm >> 8) & 0xFF;
                set_alarm_b(on_h_local, (uint8_t)(on_hhmm & 0xFF));
                enter_standby(); 
            }
        }
        // 刚好到了开机时间 (如 07:00)
        else if (now_hhmm == on_hhmm)
        {
            LOGI("[PWR] Auto Trigger: Exit Soft Sleep\r\n"); 
            mb_set_coil_reg_by_address(COIL_REG_IS_ENTRY_SLEEP_CMD, 0);
            last_trigger_hhmm = now_hhmm; 
        }
    }
}

// 统一初始化接口
void gps_rtc_app_init(void)
{
    // config_gps_app(); // [保留] 如果接了物理GPS模块，请取消注释
    rtc_power_init();
}

// 业务总线 (供 RTOS 任务轮询调用)
void process_gps_logic(void)
{
    static TickType_t last_1000ms = 0;

    // update_gps_app();

    // 2. 纯粹的时间轮询与休眠检测 (每 5秒 执行一次)
    if (xTaskGetTickCount() - last_1000ms >= pdMS_TO_TICKS(5000))
    {
        last_1000ms = xTaskGetTickCount();
        rtc_power_schedule_check();
    }
}

// =========================================================================
// ====================GPS 物理解析 与 硬件深度休眠 ====================
// =========================================================================

void config_gps_app(void)
{
    (void)uart_manage_enable_dma_recv_by_name("gps");
    osDelay(1000);

#if (GPS_TYPE_STD == WT_RTK_UM982)
    osDelay(10);
    const char cfgmsg_gga[] = "GPGGA COM1 100\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gga, sizeof(cfgmsg_gga) - 1U);
    osDelay(10);
    const char cfgmsg_rmc[] = "GPRMC COM1 1\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_rmc, sizeof(cfgmsg_rmc) - 1U);
    osDelay(10);
    const char cfgmsg_save[] = "SAVECONFIG\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_save, sizeof(cfgmsg_save) - 1U);
    osDelay(10);
#elif (GPS_TYPE_STD == WT_GPS_UM626N)
    // 只启用RMC消息，关闭其他所有消息
    const char cfgmsg_gga[] = "$CFGMSG,0,0,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gga, sizeof(cfgmsg_gga) - 1U);
    osDelay(10);
    const char cfgmsg_gll[] = "$CFGMSG,0,1,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gll, sizeof(cfgmsg_gll) - 1U);
    osDelay(10);
    const char cfgmsg_gsa[] = "$CFGMSG,0,2,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gsa, sizeof(cfgmsg_gsa) - 1U);
    osDelay(10);
    const char cfgmsg_gsv[] = "$CFGMSG,0,3,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gsv, sizeof(cfgmsg_gsv) - 1U);
    osDelay(10);
    const char cfgmsg_rmc[] = "$CFGMSG,0,4,1\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_rmc, sizeof(cfgmsg_rmc) - 1U);
    osDelay(10);
    const char cfgmsg_vtg[] = "$CFGMSG,0,5,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_vtg, sizeof(cfgmsg_vtg) - 1U);
    osDelay(10);
    const char cfgmsg_zda[] = "$CFGMSG,0,6,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_zda, sizeof(cfgmsg_zda) - 1U);
    osDelay(10);
    const char cfgmsg_gst[] = "$CFGMSG,0,7,0\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_gst, sizeof(cfgmsg_gst) - 1U);
    osDelay(10);
#elif (GPS_TYPE_STD == WT_GPS_6N)
    const char cfgmsg_freq[] = "$PCAS03,1,0,0,0,0,0,0,0*03\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_freq, sizeof(cfgmsg_freq) - 1U);
    osDelay(10);
    const char cfgmsg_save[] = "$PCAS00*01\r\n";
    uart_manage_dma_send_by_name("gps", (uint8_t *)cfgmsg_save, sizeof(cfgmsg_save) - 1U);
    osDelay(10);
#endif
    HAL_GPIO_WritePin(GPS_EN_GPIO_Port, GPS_EN_Pin, GPIO_PIN_SET); // 高电平gps工作
    osDelay(3000);
}

// 读取PWR标志位，1=来自待机唤醒，0=正常上电
static uint8_t rtc_is_wakeup_from_standby(void)
{
    // 读PWR标志位，1=来自待机唤醒，0=正常上电
    return (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) ? 1 : 0;
}

// 提取并解析 GPS DMA 缓冲区数据
void update_gps_app(void)
{
    uart_inferface_t *m_obj = uart_manage_get_obj_by_name("gps");
    if (m_obj == NULL) return;

    lwrb_sz_t available = lwrb_get_full(&m_obj->process_ring_buffer);
    if (available == 0) return;

    uint8_t to_read_buffer[128];
    lwrb_sz_t to_read = (available > sizeof(to_read_buffer)) ? sizeof(to_read_buffer) : available;
    lwrb_sz_t read_size = lwrb_read(&m_obj->process_ring_buffer, to_read_buffer, to_read);

    if (read_size > 0)
    {
        int parse_result = nmea_parse(to_read_buffer, (uint16_t)read_size);
        if (parse_result == NMEA_OK || parse_result == NMEA_ERR_NO_FIX)
        {
            if (g_nmea_gnss.time_h > 0 || g_nmea_gnss.time_m > 0 || g_nmea_gnss.time_s > 0)
            {
                gps_sync_rtc_once(); // 解析成功，触发校准
            }
        }
    }
}

// GPS 定位成功后，将 UTC 转换为北京时间写入 RTC (仅同步一次)
static void gps_sync_rtc_once(void)
{
    static uint8_t rtc_synced = 0;
    if (rtc_synced) return;

    RTC_TimeTypeDef sTime = {0};
    // 注意：GPS 发来的是零时区(UTC)时间，必须 +8 小时转为北京时间写入 RTC
    sTime.Hours = (g_nmea_gnss.time_h + 8) % 24; 
    sTime.Minutes = g_nmea_gnss.time_m;
    sTime.Seconds = g_nmea_gnss.time_s;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_BKP_MAGIC_NUMBER);

    print_internal_rtc_time();
    osDelay(100);

    rtc_synced = 1;
    s_gps_synced = 1; 
}

// 打印内部调试时间
static void print_internal_rtc_time(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    LOGI("[RTC] Internal Beijing Time: %02u:%02u:%02u\r\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
}

// 设置硬件闹钟 (为深度休眠做唤醒准备)
static void set_alarm_b(uint8_t local_h, uint8_t local_m)
{
    HAL_PWR_EnableBkUpAccess();
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_B); 

    RTC_AlarmTypeDef sAlarm = {0};
    sAlarm.AlarmTime.Hours = local_h;
    sAlarm.AlarmTime.Minutes = local_m;
    sAlarm.AlarmTime.Seconds = 0;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_SECONDS;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = 1;
    sAlarm.Alarm = RTC_ALARM_B;

    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
    {
        LOGE("[RTC] Alarm B SET FAILED!\r\n");
        return;
    }
    __HAL_RTC_ALARM_EXTI_ENABLE_IT();
    __HAL_RTC_ALARM_EXTI_ENABLE_RISING_EDGE();

    LOGI("[RTC] Standby Alarm B Set: %02d:%02d (Beijing)\r\n", local_h, local_m);
}

// 进入硬件深度休眠 (STM32 STANDBY)
static void enter_standby(void)
{
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != RTC_BKP_MAGIC_NUMBER)
    {
        LOGE("[PWR-ERR] Backup domain invalid! Magic number lost.\r\n");
        return;           
    }

    LOGI("[PWR] enter STM32 hard standby mode...\r\n");
    osDelay(200);

    // 切断 GPS 外设电源
    HAL_GPIO_WritePin(GPS_EN_GPIO_Port, GPS_EN_Pin, GPIO_PIN_RESET);
    osDelay(100);

    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRBF);
    __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WKUP1 | PWR_FLAG_WKUP2 | PWR_FLAG_WKUP3 | 
                         PWR_FLAG_WKUP4 | PWR_FLAG_WKUP5 | PWR_FLAG_WKUP6 | PWR_FLAG_SB);
    
    // 真正断开芯片内核供电，仅保留 RTC 区域
    HAL_PWR_EnterSTANDBYMode();
}