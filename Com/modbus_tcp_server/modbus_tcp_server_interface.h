#ifndef MODBUS_TCP_SERVER_API_H
#define MODBUS_TCP_SERVER_API_H

#include <stdint.h>
#include <stddef.h>
#include "stdbool.h"




/* Modbus 错误码 */
typedef enum {
    MB_OK                        = 0x00,
    MB_ERR_NOT_WRITEABLE         = 0x01,
    MB_ERR_OUT_OF_RANGE          = 0x02,
    MB_ERR_TYPE                  = 0x04,
    MB_ERR_ACQUIRE_MUTEX_TIMEOUT = 0x08,
    MB_ERR_NULL_POINTER          = 0x10,
    MB_ERR_NOT_INITIALIZED       = 0x20
} mb_err_t;

/* Flash 保存钩子函数指针类型 */
typedef void (*mb_flash_save_cb_t)(void);

/* API 初始化配置结构体 */
typedef struct {
    uint16_t *holding_regs;
    uint16_t holding_len;

    uint16_t *input_regs;
    uint16_t input_len;

    uint8_t *coil_regs;
    uint16_t coil_len;

    mb_flash_save_cb_t flash_save_cb; 
} mb_api_config_t;

extern uint8_t mb_tcp_init_flag;
extern mb_api_config_t mb_config;

extern bool enable_flash_save_cb;


/* ================== API 接口 ================== */
mb_err_t mb_api_init(const mb_api_config_t *config);

// 兼容 Protocol 层的容量宏检查
uint16_t mb_get_holding_size(void);
uint16_t mb_get_input_size(void);
uint16_t mb_get_coil_size(void);

#define REG_DATABASE_HOLDING_SIZE mb_get_holding_size()
#define REG_DATABASE_INPUT_SIZE   mb_get_input_size()
#define REG_DATABASE_COILS_SIZE   mb_get_coil_size()

mb_err_t mb_get_holding_reg_by_address(uint16_t address, uint16_t *value);
mb_err_t mb_set_holding_reg_by_address(uint16_t address, uint16_t value);

mb_err_t mb_get_input_reg_by_address(uint16_t address, uint16_t *value);
mb_err_t mb_set_input_reg_by_address(uint16_t address, uint16_t value);

mb_err_t mb_get_coil_reg_by_address(uint16_t address, uint8_t *value);
mb_err_t mb_set_coil_reg_by_address(uint16_t address, uint8_t value);

#endif /* MODBUS_TCP_SERVER_API_H */