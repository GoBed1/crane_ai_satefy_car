#include "modbus_tcp_server_reg.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdbool.h>
// #include "board_flash_system.h"
// #include "board_manage.h"
// #include "board_flash_system.h"

/* access SNTP/RTC flags from main/eth modules */
extern volatile uint32_t lwip_sntp_timestamp;
extern volatile bool rtc_updated_from_ntp;
// #include "stm32h7xx_hal_cortex.h"

#define MBTR_E(...) LOG_ERR("MBTR", __VA_ARGS__)
#define MBTR_I(...) LOG_INFO("MBTR", __VA_ARGS__)

#define MODBUS_MUTEX_TIMEOUT 100 // ms

uint8_t mb_tcp_init_flag = 0;
static osMutexId_t mb_tcp_mutex_id = NULL;
uint16_t holding_regs_database[REG_DATABASE_HOLDING_SIZE] = {0};
uint16_t input_regs_database[REG_DATABASE_INPUT_SIZE] = {0};
uint8_t coil_regs_database[REG_DATABASE_COILS_SIZE] = {0}; // 1位宽，实际存储为uint8_t

#define RETURN_IF_OUT_OF_RANGE(addr, max) \
    do { \
        if ((addr) >= (max)) { \
            return MB_ERR_OUT_OF_RANGE; \
        } \
    } while (0)


static mb_err_t mb_acquire_mutex(void)
{
    if (osMutexAcquire(mb_tcp_mutex_id, MODBUS_MUTEX_TIMEOUT) != osOK) {
        return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
    }
    return MB_OK;
}

static void mb_release_mutex(void)
{
    osMutexRelease(mb_tcp_mutex_id);
}

mb_err_t mb_init_reg(void)
{
     if (mb_tcp_init_flag != false) {
        return MB_OK;
    }

    if (mb_tcp_mutex_id == NULL) {
        const osMutexAttr_t mutex_attr = {
            .name = "ModbusTcpRegMutex",
            .attr_bits = osMutexRecursive,
            .cb_mem = NULL,
            .cb_size = 0U
        };
        mb_tcp_mutex_id = osMutexNew(&mutex_attr);
        if (mb_tcp_mutex_id == NULL) {
            return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
        }
    }

    // 直接加载默认值，不做任何持久化
    mb_err_t mb_err = mb_default_reg(REG_TYPE_ALL);
    if (mb_err != MB_OK) {
        return mb_err;
    }

    mb_tcp_init_flag = true;
    return MB_OK;
//     if (mb_tcp_init_flag != false) {
//         return MB_OK;
//     }

//     if (mb_tcp_mutex_id == NULL) {
//         const osMutexAttr_t mutex_attr = {
//             .name = "ModbusTcpRegMutex",
//             .attr_bits = osMutexRecursive,
//             .cb_mem = NULL,
//             .cb_size = 0U
//         };
//         mb_tcp_mutex_id = osMutexNew(&mutex_attr);
//         if (mb_tcp_mutex_id == NULL) {
//             return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
//         }
//     }
    
//     fs_err_t err;
//     err = fs_mount_medium();
//     if(err != FS_OK) {
//         MBTR_E("Failed to mount file system for Modbus registers");
//     }else{
//         err = fs_load_modbus_reg();
//         if (err != FS_OK) {
//             int mb_err = mb_default_reg(REG_TYPE_ALL);
//             if (mb_err != 0) {
//                 return FS_ERR_MB_DEFAULT;
//             }
//             err = fs_save_modbus_reg();
//         }
//     }

//    err = fs_load_modbus_reg();
//    if (err != FS_OK) {
//        int mb_err = mb_default_reg(REG_TYPE_ALL);
//        if (mb_err != 0) {
//         return FS_ERR_MB_DEFAULT;
//        }
//        err = fs_save_modbus_reg();
//    }
//    return err;

//     mb_tcp_init_flag = true;
//     return MB_OK;
}

mb_err_t mb_clear_reg(int type)
{
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    if (type & REG_TYPE_HOLDING_REGISTER) {
        for (uint16_t i = 0; i < REG_DATABASE_HOLDING_SIZE; ++i) {
            holding_regs_database[i] = 0;
        }
    }
    if (type & REG_TYPE_INPUT_REGISTER) {
        for (uint16_t i = 0; i < REG_DATABASE_INPUT_SIZE; ++i) {
            input_regs_database[i] = 0;
        }
    }
    if (type & REG_TYPE_COIL) {
        for (uint16_t i = 0; i < REG_DATABASE_COILS_SIZE; ++i) {
            coil_regs_database[i] = 0;
        }
    }
    
    mb_release_mutex();

    return MB_OK;
}

mb_err_t mb_default_reg(int type)
{
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    if (type & REG_TYPE_HOLDING_REGISTER) {
        for (uint16_t i = 0; i < REG_INFO_HOLDING_SIZE && i < REG_DATABASE_HOLDING_SIZE; ++i) {
            holding_regs_database[holding_reg_info[i].address] = holding_reg_info[i].default_value;
        }
    }
    if (type & REG_TYPE_INPUT_REGISTER) {
        for (uint16_t i = 0; i < REG_INFO_INPUT_SIZE && i < REG_DATABASE_INPUT_SIZE; ++i) {
            input_regs_database[input_reg_info[i].address] = input_reg_info[i].default_value;
        }
    }
    if (type & REG_TYPE_COIL) {
        for (uint16_t i = 0; i < REG_INFO_COIL_SIZE && i < REG_DATABASE_COILS_SIZE; ++i) {
            coil_regs_database[coli_reg_info[i].address] = (uint8_t)coli_reg_info[i].default_value;
        }
    }

    mb_release_mutex();
    return MB_OK;
}

// static mb_err_t mb_get_reg(const mb_reg_t *reg, void *value)
// {
//     if (!reg || !value) return MB_ERR_NULL_POINTER;
//     switch (reg->type) {
//         case REG_TYPE_HOLDING_REGISTER:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_HOLDING_SIZE);
//             *(uint16_t*)value = holding_regs_database[reg->address];
//             break;
//         case REG_TYPE_INPUT_REGISTER:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_INPUT_SIZE);
//             *(uint16_t*)value = input_regs_database[reg->address];
//             break;
//         case REG_TYPE_COIL:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_COILS_SIZE);
//             *(uint8_t*)value = coil_regs_database[reg->address];
//             break;
//         default:
//             return MB_ERR_TYPE;
//     }
//     return MB_OK;
// }

// static mb_err_t mb_set_reg(const mb_reg_t *reg, const void *value)
// {
//     if (!reg || !value) return MB_ERR_NULL_POINTER;
//     switch (reg->type) {
//         case REG_TYPE_HOLDING_REGISTER:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_HOLDING_SIZE);
//             holding_regs_database[reg->address] = *(const uint16_t*)value;
//             break;
//         case REG_TYPE_INPUT_REGISTER:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_INPUT_SIZE);
//             input_regs_database[reg->address] = *(const uint16_t*)value;
//             break;
//         case REG_TYPE_COIL:
//             RETURN_IF_OUT_OF_RANGE(reg->address, REG_DATABASE_COILS_SIZE);
//             coil_regs_database[reg->address] = (*(const uint8_t*)value) ? 1 : 0;
//             break;
//         default:
//             return MB_ERR_TYPE;
//     }
//     return MB_OK;
// }

// mb_err_t mb_get_reg_safe(const mb_reg_t *reg, void *value)
// {
//     if (!reg || !value) return MB_ERR_NULL_POINTER;
//     mb_err_t err = mb_acquire_mutex();
//     if (err != MB_OK) return err;
//     err = mb_get_reg(reg, value);
//     mb_release_mutex();
//     return err;
// }

// mb_err_t mb_set_reg_safe(const mb_reg_t *reg, const void *value)
// {
//     if (!reg || !value) return MB_ERR_NULL_POINTER;
//     mb_err_t err = mb_acquire_mutex();
//     if (err != MB_OK) return err;
//     err = mb_set_reg(reg, value);
//     mb_release_mutex();
//     return err;
// }

mb_err_t mb_set_holding_reg_by_address(uint16_t address, uint16_t value)
{
    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_HOLDING_SIZE);
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;
    holding_regs_database[address] = value;
    mb_release_mutex();
    // fs_save_modbus_reg(); // 锁外保存
    return MB_OK;
}

mb_err_t mb_get_holding_reg_by_address(uint16_t address, uint16_t *value)
{
    if (!value) return MB_ERR_NULL_POINTER;

    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_HOLDING_SIZE);
    if (!value) return MB_ERR_NULL_POINTER;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK)return err;

    *value = holding_regs_database[address];

    mb_release_mutex();

    return err;
}

mb_err_t mb_set_input_reg_by_address(uint16_t address, uint16_t value)
{
    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_INPUT_SIZE);

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK)return err;

    input_regs_database[address] = value;

    mb_release_mutex();

    return err;
}

mb_err_t mb_get_input_reg_by_address(uint16_t address, uint16_t *value)
{
    if (!value) return MB_ERR_NULL_POINTER;

    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_INPUT_SIZE);
    if (!value) return MB_ERR_NULL_POINTER;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK)return err;

    *value = input_regs_database[address];

    mb_release_mutex();

    return err;
}

mb_err_t mb_get_coil_reg_by_address(uint16_t address, uint8_t *value)
{
    if (!value) return MB_ERR_NULL_POINTER;
    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_COILS_SIZE);
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;
    *value = coil_regs_database[address] & 0x01; // 只返回最低1位
    mb_release_mutex();
    return err;
}

mb_err_t mb_set_coil_reg_by_address(uint16_t address, uint8_t value)
{
    RETURN_IF_OUT_OF_RANGE(address, REG_DATABASE_COILS_SIZE);
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;
    coil_regs_database[address] = value ? 1 : 0; // 只存最低1位
    mb_release_mutex();
    // fs_save_modbus_reg(); // 锁外保存
    return err;
}

