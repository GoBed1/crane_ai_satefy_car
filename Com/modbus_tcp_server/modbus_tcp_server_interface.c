#include "modbus_tcp_server_interface.h"
#include "cmsis_os.h"

#define MODBUS_MUTEX_TIMEOUT 100
 
uint8_t mb_tcp_init_flag = 0;
static osMutexId_t mb_tcp_mutex_id = NULL;
static mb_api_config_t mb_ctx = {0};

static mb_err_t mb_acquire_mutex(void) {
    if (mb_tcp_mutex_id == NULL) return MB_ERR_NOT_INITIALIZED;
    if (osMutexAcquire(mb_tcp_mutex_id, MODBUS_MUTEX_TIMEOUT) != osOK) return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
    return MB_OK;
}

static void mb_release_mutex(void) {
    if (mb_tcp_mutex_id != NULL) osMutexRelease(mb_tcp_mutex_id);
}

mb_err_t mb_api_init(const mb_api_config_t *config) {
    if (config == NULL) return MB_ERR_NULL_POINTER;
    if (mb_tcp_init_flag != 0) return MB_OK;

    if (mb_tcp_mutex_id == NULL) {
        const osMutexAttr_t mutex_attr = { .name = "MbTcpRegMutex", .attr_bits = osMutexRecursive };
        mb_tcp_mutex_id = osMutexNew(&mutex_attr);
        if (mb_tcp_mutex_id == NULL) return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
    }

    mb_ctx = *config;
    mb_tcp_init_flag = 1;
    return MB_OK;
}

uint16_t mb_get_holding_size(void) { return mb_ctx.holding_len; }
uint16_t mb_get_input_size(void)   { return mb_ctx.input_len; }
uint16_t mb_get_coil_size(void)    { return mb_ctx.coil_len; }

mb_err_t mb_set_holding_reg_by_address(uint16_t address, uint16_t value) {
    if (mb_ctx.holding_regs == NULL || address >= mb_ctx.holding_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    mb_ctx.holding_regs[address] = value;
    mb_release_mutex();
    
    // 触发写操作回调钩子
    if (mb_ctx.flash_save_cb != NULL) mb_ctx.flash_save_cb();
    return MB_OK;
}

mb_err_t mb_get_holding_reg_by_address(uint16_t address, uint16_t *value) {
    if (value == NULL) return MB_ERR_NULL_POINTER;
    if (mb_ctx.holding_regs == NULL || address >= mb_ctx.holding_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    *value = mb_ctx.holding_regs[address];
    mb_release_mutex();
    return MB_OK;
}

mb_err_t mb_set_input_reg_by_address(uint16_t address, uint16_t value) {
    if (mb_ctx.input_regs == NULL || address >= mb_ctx.input_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    mb_ctx.input_regs[address] = value;
    mb_release_mutex();
    return MB_OK;
}

mb_err_t mb_get_input_reg_by_address(uint16_t address, uint16_t *value) {
    if (value == NULL) return MB_ERR_NULL_POINTER;
    if (mb_ctx.input_regs == NULL || address >= mb_ctx.input_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    *value = mb_ctx.input_regs[address];
    mb_release_mutex();
    return MB_OK;
}

/* --- 线圈 --- */
mb_err_t mb_set_coil_reg_by_address(uint16_t address, uint8_t value) {
    if (mb_ctx.coil_regs == NULL || address >= mb_ctx.coil_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    mb_ctx.coil_regs[address] = value ? 1 : 0;
    mb_release_mutex();
    
    // if (mb_ctx.flash_save_cb != NULL) mb_ctx.flash_save_cb();
    return MB_OK;
}

mb_err_t mb_get_coil_reg_by_address(uint16_t address, uint8_t *value) {
    if (value == NULL) return MB_ERR_NULL_POINTER;
    if (mb_ctx.coil_regs == NULL || address >= mb_ctx.coil_len) return MB_ERR_OUT_OF_RANGE;
    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    *value = mb_ctx.coil_regs[address] & 0x01;
    mb_release_mutex();
    return MB_OK;
}