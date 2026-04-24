#ifndef MODBUS_TCP_SERVER_REG_H
#define MODBUS_TCP_SERVER_REG_H

#include "modbus_tcp_server_interface.h"
#include "stdbool.h"

#define APP_HOLDING_SIZE 50
#define APP_INPUT_SIZE   50
#define APP_COIL_SIZE    20

extern uint16_t holding_regs_database[APP_HOLDING_SIZE];
extern uint8_t  coil_regs_database[APP_COIL_SIZE];

//寄存器类型宏
#define REG_TYPE_HOLDING 0x01
#define REG_TYPE_INPUT   0x02
#define REG_TYPE_COIL    0x04
#define REG_TYPE_ALL     0xFF

// API 初始化入口
void mb_init_reg(void);
void mb_default_reg(uint8_t type);

#endif /* MODBUS_TCP_SERVER_REG_H */