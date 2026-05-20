#include "sys_def.h"

// bms读取相关
uint16_t bms_read_results[READ_MSG_COUNT] = {0};
modbus_t bms_read_telegrams[READ_MSG_COUNT] = {
    [READ_BATT_LEVEL] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_BATTERY_LEVEL, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_BATT_LEVEL]},
    [READ_REMAIN_DISCHARGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_REMAIN_DISCHARGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_REMAIN_DISCHARGE]},
    [READ_REMAIN_CHARGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_REMAIN_CHARGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_REMAIN_CHARGE]},
    [READ_TOTAL_VOLTAGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_TOTAL_VOLTAGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_TOTAL_VOLTAGE]},
    [READ_TOTAL_CURRENT] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_TOTAL_CURRENT, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_TOTAL_CURRENT]}};

modbusHandler_t bms_mppt_master;       // BMS Modbus 主机句柄
uint16_t modbus_master_buf[128] = {0}; // Modbus 主机共用缓冲区

// 放电时间全局变量
uint16_t discharge_samples[BMS_SAMPLE_BUFFER_SIZE];
uint8_t discharge_idx = 0;
uint8_t discharge_count = 0;
// 充电时间全局变量
uint16_t charge_samples[BMS_SAMPLE_BUFFER_SIZE];
uint8_t charge_idx = 0;
uint8_t charge_count = 0;

// MPPT 读取结果存储
uint16_t mppt_read_results[READ_MPPT_MSG_COUNT] = {0};

// MPPT 读取报文配置 (使用功能码 04 读输入寄存器)
modbus_t mppt_read_telegrams[READ_MPPT_MSG_COUNT] = {
    [READ_MPPT_PV_VOLTAGE] = {.u8id = SLAVE_MPPT_ID, .u8fct = MB_FC_READ_INPUT_REGISTER, .u16RegAdd = REG_MPPT_PV_VOLTAGE, .u16CoilsNo = 1, .u16reg = &mppt_read_results[READ_MPPT_PV_VOLTAGE]},
    [READ_MPPT_PV_CURRENT] = {.u8id = SLAVE_MPPT_ID, .u8fct = MB_FC_READ_INPUT_REGISTER, .u16RegAdd = REG_MPPT_PV_CURRENT, .u16CoilsNo = 1, .u16reg = &mppt_read_results[READ_MPPT_PV_CURRENT]},
    [READ_MPPT_LOAD_VOLTAGE] = {.u8id = SLAVE_MPPT_ID, .u8fct = MB_FC_READ_INPUT_REGISTER, .u16RegAdd = REG_MPPT_LOAD_VOLTAGE, .u16CoilsNo = 1, .u16reg = &mppt_read_results[READ_MPPT_LOAD_VOLTAGE]},
    [READ_MPPT_LOAD_CURRENT] = {.u8id = SLAVE_MPPT_ID, .u8fct = MB_FC_READ_INPUT_REGISTER, .u16RegAdd = REG_MPPT_LOAD_CURRENT, .u16CoilsNo = 1, .u16reg = &mppt_read_results[READ_MPPT_LOAD_CURRENT]}};

// gps相关参数
uint16_t off_hhmm = 0;
uint16_t on_hhmm = 0;
uint8_t standby_flag = 0; 

// SFA1000A 单次测距指令 (发送给模块的命令)
const uint8_t laser_single_cmd[8] = {0x55, 0xAA, 0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0x84};
