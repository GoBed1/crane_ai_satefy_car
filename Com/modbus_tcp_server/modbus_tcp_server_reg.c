#include "modbus_tcp_server_reg.h"
#include "cmsis_os.h"
#include <string.h>

#define MODBUS_MUTEX_TIMEOUT 100 // ms

static uint16_t normal_area_data[50];

const mb_reg_area_t modbus_manage_table[] = {
    {
        .name = "normal",            // 区块名字
        .type = MB_REG_TYPE_HOLDING,   // 寄存器类型：保持寄存器
        .start_address = 0,            // Modbus 起始地址
        .size = 50,                    // 寄存器数量
        .data_buffer = normal_area_data, // 指向上面定义的数组
        .used = 0                      // 初始填 0 即可
    }
};

uint8_t mb_tcp_init_flag = 0;
static osMutexId_t mb_tcp_mutex_id = NULL;

// 核心登记簿：保存所有注册进来的内存块信息
static mb_reg_area_t mb_manage_areas[MB_MANAGE_MAX_AREAS] = {0};

/* ---------------- 内部工具: 互斥锁 ---------------- */
static mb_err_t mb_acquire_mutex(void)
{
    if (mb_tcp_mutex_id == NULL) return MB_ERR_NOT_INITIALIZED;
    if (osMutexAcquire(mb_tcp_mutex_id, MODBUS_MUTEX_TIMEOUT) != osOK) {
        return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
    }
    return MB_OK;
}

static void mb_release_mutex(void)
{
    if (mb_tcp_mutex_id != NULL) {
        osMutexRelease(mb_tcp_mutex_id);
    }
}

/* ---------------- 内部工具: 核心路由查表 ---------------- */
// 根据想要操作的类型和物理地址，找出它应该落在哪一个注册的内存块里
static mb_reg_area_t* find_area(mb_reg_type_t type, uint16_t address)
{
    for (uint16_t i = 0; i < MB_MANAGE_MAX_AREAS; i++) {
        if (mb_manage_areas[i].used && mb_manage_areas[i].type == type) {
            // 检查地址是否在该块的范围内：[start_address, start_address + size)
            if (address >= mb_manage_areas[i].start_address && 
                address < (mb_manage_areas[i].start_address + mb_manage_areas[i].size)) {
                return &mb_manage_areas[i];
            }
        }
    }
    return NULL; // 地址未注册或越界
}

/* ---------------- 新增：初始化注册表 ---------------- */
mb_err_t mb_manage_init_table(const mb_reg_area_t *table, uint16_t table_size)
{
   if (table == NULL || table_size == 0 || table_size > MB_MANAGE_MAX_AREAS) return MB_ERR_NULL_POINTER;

    if (mb_tcp_mutex_id == NULL) {
        const osMutexAttr_t mutex_attr = {
            .name = "MbTcpRegMutex",
            .attr_bits = osMutexRecursive
        };
        mb_tcp_mutex_id = osMutexNew(&mutex_attr);
        if (mb_tcp_mutex_id == NULL) return MB_ERR_ACQUIRE_MUTEX_TIMEOUT;
    }

    for (uint16_t i = 0; i < table_size; ++i) {
        // --- 1. 严格校验注册边界 ---
        uint32_t end_address = (uint32_t)table[i].start_address + table[i].size;
        
        if (table[i].type == MB_REG_TYPE_HOLDING && end_address > REG_DATABASE_HOLDING_SIZE) {
            printf("Modbus Reg Error: Holding [%s] out of max limit (%d)\n", table[i].name, REG_DATABASE_HOLDING_SIZE);
            return MB_ERR_OUT_OF_RANGE;
        }
        if (table[i].type == MB_REG_TYPE_INPUT && end_address > REG_DATABASE_INPUT_SIZE) {
            printf("Modbus Reg Error: Input [%s] out of max limit (%d)\n", table[i].name, REG_DATABASE_INPUT_SIZE);
            return MB_ERR_OUT_OF_RANGE;
        }
        if (table[i].type == MB_REG_TYPE_COIL && end_address > REG_DATABASE_COILS_SIZE) {
            printf("Modbus Reg Error: Coil [%s] out of max limit (%d)\n", table[i].name, REG_DATABASE_COILS_SIZE);
            return MB_ERR_OUT_OF_RANGE;
        }

        // --- 2. 拷贝信息并清零内存 ---
        mb_manage_areas[i] = table[i];
        mb_manage_areas[i].used = 1;

        if (table[i].data_buffer != NULL) {
            uint32_t byte_size = (table[i].type == MB_REG_TYPE_COIL) ? 
                                 (table[i].size * sizeof(uint8_t)) : 
                                 (table[i].size * sizeof(uint16_t));
            memset(table[i].data_buffer, 0, byte_size);
        }
    }

    mb_tcp_init_flag = 1;
    return MB_OK;
}
mb_err_t mb_init_reg(void)
{
    if (mb_tcp_init_flag != 0) return MB_OK;

    // 内部调用新版的注册逻辑
    return mb_manage_init_table(modbus_manage_table, sizeof(modbus_manage_table)/sizeof(modbus_manage_table[0]));
}

/* =====================================================================
 * 原有 API 完美保留：协议层只需调用这些接口，无需关心底层存储逻辑
 * ===================================================================== */

/* --------- 保持寄存器 --------- */
mb_err_t mb_set_holding_reg_by_address(uint16_t address, uint16_t value)
{
    mb_reg_area_t *area = find_area(MB_REG_TYPE_HOLDING, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    // 计算实际数组偏移量 (目标地址 - 该区块的起始地址)
    uint16_t offset = address - area->start_address;
    uint16_t *buf = (uint16_t *)area->data_buffer;
    buf[offset] = value;

    mb_release_mutex();
    
    // 如果需要文件系统保存 Flash，可在此触发事件
    // fs_save_modbus_reg(); 
    return MB_OK;
}

mb_err_t mb_get_holding_reg_by_address(uint16_t address, uint16_t *value)
{
    if (value == NULL) return MB_ERR_NULL_POINTER;

    mb_reg_area_t *area = find_area(MB_REG_TYPE_HOLDING, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    uint16_t offset = address - area->start_address;
    uint16_t *buf = (uint16_t *)area->data_buffer;
    *value = buf[offset];

    mb_release_mutex();
    return MB_OK;
}

/* --------- 输入寄存器 --------- */
mb_err_t mb_set_input_reg_by_address(uint16_t address, uint16_t value)
{
    mb_reg_area_t *area = find_area(MB_REG_TYPE_INPUT, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    uint16_t offset = address - area->start_address;
    uint16_t *buf = (uint16_t *)area->data_buffer;
    buf[offset] = value;

    mb_release_mutex();
    return MB_OK;
}

mb_err_t mb_get_input_reg_by_address(uint16_t address, uint16_t *value)
{
    if (value == NULL) return MB_ERR_NULL_POINTER;

    mb_reg_area_t *area = find_area(MB_REG_TYPE_INPUT, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    uint16_t offset = address - area->start_address;
    uint16_t *buf = (uint16_t *)area->data_buffer;
    *value = buf[offset];

    mb_release_mutex();
    return MB_OK;
}

/* --------- 线圈 --------- */
mb_err_t mb_set_coil_reg_by_address(uint16_t address, uint8_t value)
{
    mb_reg_area_t *area = find_area(MB_REG_TYPE_COIL, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    uint16_t offset = address - area->start_address;
    uint8_t *buf = (uint8_t *)area->data_buffer;
    buf[offset] = value ? 1 : 0; // 确保线圈的值只写入 0 或 1

    mb_release_mutex();
    return MB_OK;
}

mb_err_t mb_get_coil_reg_by_address(uint16_t address, uint8_t *value)
{
    if (value == NULL) return MB_ERR_NULL_POINTER;

    mb_reg_area_t *area = find_area(MB_REG_TYPE_COIL, address);
    if (area == NULL || area->data_buffer == NULL) return MB_ERR_OUT_OF_RANGE;

    mb_err_t err = mb_acquire_mutex();
    if (err != MB_OK) return err;

    uint16_t offset = address - area->start_address;
    uint8_t *buf = (uint8_t *)area->data_buffer;
    *value = buf[offset] & 0x01; // 安全保障，取出最低位

    mb_release_mutex();
    return MB_OK;
}