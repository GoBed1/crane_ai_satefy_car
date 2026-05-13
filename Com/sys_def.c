#include "sys_def.h"


//bms读取相关
uint16_t bms_read_results[READ_MSG_COUNT] = {0};
modbus_t bms_read_telegrams[READ_MSG_COUNT] = {
    [READ_BATT_LEVEL] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_BATTERY_LEVEL, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_BATT_LEVEL]},
    [READ_REMAIN_DISCHARGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_REMAIN_DISCHARGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_REMAIN_DISCHARGE]},
    [READ_REMAIN_CHARGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_REMAIN_CHARGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_REMAIN_CHARGE]},
    [READ_TOTAL_VOLTAGE] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_TOTAL_VOLTAGE, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_TOTAL_VOLTAGE]},
    [READ_TOTAL_CURRENT] = {.u8id = SLAVE_BMS_ID, .u8fct = MB_FC_READ_REGISTERS, .u16RegAdd = REG_TOTAL_CURRENT, .u16CoilsNo = 1, .u16reg = &bms_read_results[READ_TOTAL_CURRENT]}};

modbusHandler_t bms_mppt_master;       // BMS Modbus 主机句柄
uint16_t modbus_master_buf[128] = {0}; // Modbus 主机共用缓冲区


