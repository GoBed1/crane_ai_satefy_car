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
/* ========================================================================= */
/* MPPT 通信宏定义                                                         */
/* ========================================================================= */
#define SLAVE_MPPT_ID 1 // MPPT 从机默认ID
// MPPT 寄存器地址定义 (读功能码04)
#define REG_MPPT_PV_VOLTAGE    0x3100  // 阵列电压
#define REG_MPPT_PV_CURRENT    0x3101  // 阵列电流
#define REG_MPPT_LOAD_VOLTAGE  0x310C  // 负载电压
#define REG_MPPT_LOAD_CURRENT  0x310D  // 负载电流
//mppt存放在输入寄存器中的地址定义（上报）
#define INPUT_REG_MPPT_PV_VOLTAGE    22 // [上报] MPPT阵列电压
#define INPUT_REG_MPPT_PV_CURRENT    23 // [上报] MPPT阵列电流
#define INPUT_REG_MPPT_LOAD_VOLTAGE  24   // [上报] MPPT负载电压
#define INPUT_REG_MPPT_LOAD_CURRENT  25   // [上报] MPPT负载电流


/* ====================相关结构体定义===================================================== */
// MPPT 读取消息索引
typedef enum
{
    READ_MPPT_PV_VOLTAGE = 0,
    READ_MPPT_PV_CURRENT,
    READ_MPPT_LOAD_VOLTAGE,
    READ_MPPT_LOAD_CURRENT,
    READ_MPPT_MSG_COUNT
} MpptReadMsgIdx_t;


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

// 外部引用的数据和报文数组
extern uint16_t mppt_read_results[READ_MPPT_MSG_COUNT];
extern modbus_t mppt_read_telegrams[READ_MPPT_MSG_COUNT];


#endif // SYS_DEF_H