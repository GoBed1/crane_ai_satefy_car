#ifndef SYS_DEF_H
#define SYS_DEF_H

#include <stdint.h>
#include "main.h"
#include "modbus.h"
#include "ModbusConfig.h"
/* ========================================================================= */
/* 日志定义                                                     */
/* ========================================================================= */
#define LOGD(...) printf("[DEBUG] " __VA_ARGS__)
#define LOGI(...) printf("[INFO]  " __VA_ARGS__)
#define LOGE(...) printf("[ERROR] " __VA_ARGS__)
/* ========================================================================= */
/* BMS 通信宏定义                                                         */
/* ========================================================================= */
#define SLAVE_BMS_ID 2                         // 外部 BMS 从机地址
#define REG_BATTERY_LEVEL 0x0000               // 获取：电池电量寄存器地址
#define REG_TOTAL_CURRENT 0x0001               // 获取：总电流
#define REG_TOTAL_VOLTAGE 0x0002               // 获取：总电压
#define REG_REMAIN_DISCHARGE 0x0007            // 获取：剩余放电时间
#define REG_REMAIN_CHARGE 0x0008               // 获取：剩余充电时间
#define INPUT_REG_BMS_BATTERY 17               // [上报] BMS当前电量
#define INPUT_REG_BMS_REMAIN_DISCHARGE_TIME 18 // [上报] BMS剩余放电时间
#define INPUT_REG_BMS_REMAIN_CHARGE_TIME 19    // [上报] BMS剩余充电时间
#define INPUT_REG_BMS_TOTAL_VOLTAGE 20         // [上报] BMS总电压
#define INPUT_REG_BMS_TOTAL_CURRENT 21         // [上报] BMS总电流
#define MODBUS_WAIT_TIMEOUT_MS 1000            // Modbus等待超时时间
// 充电/放电时间采样缓存数组大小
#define BMS_SAMPLE_BUFFER_SIZE 30              // 充电/放电时间，采样缓存数组大小
#define BMS_SAMPLE_VALID_COUNT    20      // 实际计算平均值的采样数

typedef enum
{
    READ_BATT_LEVEL = 0,
    READ_REMAIN_DISCHARGE,
    READ_REMAIN_CHARGE,
    READ_TOTAL_VOLTAGE,
    READ_TOTAL_CURRENT,
    READ_MSG_COUNT
} BmsReadMsgIdx_t;

typedef enum
{
    ERR_NONE = 0,          // 正常状态
    ERR_BMS_READ_FAIL = 3, // BMS 读取失败
} SystemErrorCode_t;

extern modbusHandler_t bms_mppt_master; // BMS Modbus 主机句柄
extern uint16_t modbus_master_buf[128]; // Modbus 主机共用缓冲区
extern modbus_t bms_read_telegrams[READ_MSG_COUNT];
extern uint16_t bms_read_results[READ_MSG_COUNT]; // 存储从 BMS 读取的结果
// 放电时间全局变量
extern uint16_t discharge_samples[BMS_SAMPLE_BUFFER_SIZE];
extern uint8_t discharge_idx;
extern uint8_t discharge_count;
// 充电时间全局变量
extern uint16_t charge_samples[BMS_SAMPLE_BUFFER_SIZE];
extern uint8_t charge_idx;
extern uint8_t charge_count;

#endif // SYS_DEF_H