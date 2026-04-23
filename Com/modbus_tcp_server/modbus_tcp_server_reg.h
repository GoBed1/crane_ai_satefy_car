#ifndef MODBUS_TCP_SERVER_REG_H
#define MODBUS_TCP_SERVER_REG_H

#include <stdint.h>
#include <stddef.h>

#define REG_DATABASE_HOLDING_SIZE 100
#define REG_DATABASE_INPUT_SIZE  100
#define REG_DATABASE_COILS_SIZE  100
/* ---------------- 错误码定义 ---------------- */
typedef enum
{
    MB_OK                        = 0x00,
    MB_ERR_NOT_WRITEABLE         = 0x01,
    MB_ERR_OUT_OF_RANGE          = 0x02,
    MB_ERR_TYPE                  = 0x04,
    MB_ERR_ACQUIRE_MUTEX_TIMEOUT = 0x08,
    MB_ERR_NULL_POINTER          = 0x10,
    MB_ERR_NOT_INITIALIZED       = 0x20
} mb_err_t;

/* ---------------- 寄存器类型定义 ---------------- */
typedef enum {
    MB_REG_TYPE_HOLDING = 1,  // 保持寄存器 (16 bit) - 可读可写
    MB_REG_TYPE_INPUT   = 2,  // 输入寄存器 (16 bit) - 只读
    MB_REG_TYPE_COIL    = 3   // 线圈 (1 bit，实际按1 byte存储) - 可读可写
} mb_reg_type_t;

#define MB_MANAGE_MAX_AREAS 16U  // 核心引擎支持注册的最大独立内存块数量

/* ---------------- 核心注册表结构体 ---------------- */
typedef struct {
    char name[32];            // 业务区块名称，例如 "SystemConfig"
    mb_reg_type_t type;       // 寄存器类型
    uint16_t start_address;   // 该区块在 Modbus 中的起始物理地址
    uint16_t size;            // 该区块包含的寄存器数量
    void *data_buffer;        // 指向外部具体内存数组的指针
    uint8_t used;             // 内部标志位(外部初始化表时填 0 即可)
} mb_reg_area_t;

extern uint8_t mb_tcp_init_flag;

/* =====================================================================
 * 新增：表驱动初始化接口 (供业务层调用)
 * ===================================================================== */
mb_err_t mb_manage_init_table(const mb_reg_area_t *table, uint16_t table_size);


/* =====================================================================
 * 保持不变：标准的寄存器读写 API (供协议解析层调用)
 * ===================================================================== */

// 保持寄存器 (Holding Register) - 03, 06, 16功能码
mb_err_t mb_get_holding_reg_by_address(uint16_t address, uint16_t *value);
mb_err_t mb_set_holding_reg_by_address(uint16_t address, uint16_t value);

// 输入寄存器 (Input Register) - 04功能码
mb_err_t mb_get_input_reg_by_address(uint16_t address, uint16_t *value);
mb_err_t mb_set_input_reg_by_address(uint16_t address, uint16_t value);

// 线圈 (Coil) - 01, 05, 15功能码
mb_err_t mb_get_coil_reg_by_address(uint16_t address, uint8_t *value);
mb_err_t mb_set_coil_reg_by_address(uint16_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_TCP_SERVER_REG_H */