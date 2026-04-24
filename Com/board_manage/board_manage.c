#include "board_manage.h"

#include <stdio.h>

#include <main.h>
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

#include <usart.h>
#include "stdbool.h"
#include "uart_manage.h"

extern RTC_HandleTypeDef hrtc;

extern volatile bool rtc_updated_from_ntp;

uint32_t board_get_time_ms(void)
{
    /* If RTC was synchronized from NTP, prefer RTC as authoritative time source. */
    if (rtc_updated_from_ntp)
    {
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};

        /* Try to read RTC time/date. If successful, convert to unix ms. */
        if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
        {
            /* Convert RTC date/time to unix seconds (UTC) */
            uint16_t year = 2000 + (uint16_t)sDate.Year; /* RTC stores year offset from 2000 */
            uint8_t month = sDate.Month;
            uint8_t day = sDate.Date;
            uint8_t hour = sTime.Hours;
            uint8_t minute = sTime.Minutes;
            uint8_t second = sTime.Seconds;

            /* Compute days since 1970-01-01 */
            uint32_t days = 0;
            for (uint16_t y = 1970; y < year; ++y)
            {
                bool is_leap = ( (y%4==0 && y%100!=0) || (y%400==0) );
                days += is_leap ? 366U : 365U;
            }

            uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
            bool is_leap_year = ( (year%4==0 && year%100!=0) || (year%400==0) );
            if (is_leap_year) days_in_month[1] = 29;

            for (uint8_t m = 1; m < month; ++m)
            {
                days += days_in_month[m-1];
            }

            days += (uint32_t)(day - 1);

            uint32_t seconds = days * 86400UL + (uint32_t)hour * 3600UL + (uint32_t)minute * 60UL + (uint32_t)second;
            uint32_t ms = seconds * 1000UL;

            return ms;
        }
        /* If RTC read fails, fall through to tick-based fallback */
    }

#if defined(FREERTOS) || defined(USE_FREERTOS) || defined(configUSE_PREEMPTION)
    // 判断是否在中断中
    if (__get_IPSR() != 0) {
        // 在中断
        return (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    } else {
        // 任务上下文
        return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
#else
    // 非FreeRTOS环境,HAL_GetTick()本身就是线程安全的，可以在中断服务函数（ISR）中直接调用
    return HAL_GetTick();
#endif
}

uint32_t board_get_time_s(void)
{
    // 如果RTC已从NTP同步，则直接从RTC读取并安全地计算Unix秒（避免32位毫秒溢出）
    if (rtc_updated_from_ntp)
    {
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};

        if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
        {
            uint16_t year = 2000 + (uint16_t)sDate.Year;
            uint8_t month = sDate.Month;
            uint8_t day = sDate.Date;
            uint8_t hour = sTime.Hours;
            uint8_t minute = sTime.Minutes;
            uint8_t second = sTime.Seconds;

            // 计算自1970-01-01的天数（使用64位中间值以避免溢出）
            uint64_t days = 0;
            for (uint16_t y = 1970; y < year; ++y)
            {
                bool is_leap = ( (y%4==0 && y%100!=0) || (y%400==0) );
                days += is_leap ? 366ULL : 365ULL;
            }

            uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
            bool is_leap_year = ( (year%4==0 && year%100!=0) || (year%400==0) );
            if (is_leap_year) days_in_month[1] = 29;

            for (uint8_t m = 1; m < month; ++m)
            {
                days += (uint64_t)days_in_month[m-1];
            }

            days += (uint64_t)(day - 1);

            uint64_t seconds = days * 86400ULL + (uint64_t)hour * 3600ULL + (uint64_t)minute * 60ULL + (uint64_t)second;

            return (uint32_t)seconds;
        }
        // 如果RTC读取失败，继续走回退逻辑
    }

    // 回退到基于tick的方式
    uint32_t ms = board_get_time_ms();
    return ms / 1000U;
}

void board_config(void)
{
  specify_redirect_uart(&huart1);
  printf("\r\nspecify redirect printf to huart1\r\n");
}

void board_create_user_tasks(void)
{
  user_debug_task_init();
  // init_read_sensor_task();
  encoder_hub_task_init();
}

/* Check current task's stack high-water mark and warn when below 1/4 of total */
void board_check_task_heap_stack(const char *name)
{   
    /* xPortGetFreeHeapSize and xPortGetMinimumEverFreeHeapSize are provided by the FreeRTOS heap implementation */
    size_t free_bytes = xPortGetFreeHeapSize();
    size_t min_ever = xPortGetMinimumEverFreeHeapSize();

    /* configTOTAL_HEAP_SIZE is defined in FreeRTOSConfig.h */
    size_t total_bytes = (size_t)configTOTAL_HEAP_SIZE;

    /* Warn if free heap drops below 25% of total, or minimum ever free is below 10% */
    if (free_bytes < (total_bytes / 4U) || min_ever < (total_bytes / 10U))
    {
        LOG_ERR("SYS", "HL(%lu,%lu,%lu)\r\n",
               (unsigned long)free_bytes,
               (unsigned long)min_ever,
               (unsigned long)total_bytes);
    }

    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    if (high_water < configMINIMAL_STACK_SIZE * 2U)
    {
        LOG_ERR("SYS","SO(%s,%u)\r\n",
               name ? name : "unknown",
               high_water);
    }
}

