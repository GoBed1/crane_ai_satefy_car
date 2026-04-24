#include "modbus_tcp_server_database.h"
#include "board_flash_system.h" 
#include <string.h>

bool enable_flash_save_cb = false;

 // 1. 实例化这三个数组，注意前两个不能加 static，因为文件系统要强行访问它们
uint16_t holding_regs_database[APP_HOLDING_SIZE] = {0};
uint8_t  coil_regs_database[APP_COIL_SIZE] = {0};
static uint16_t input_regs_database[APP_INPUT_SIZE] = {0}; // 输入寄存器不需要存Flash，可保留static


// 2. Flash 保存回调函数
static void app_flash_save_cb(void) {
    if (!enable_flash_save_cb) return;

    // 严格调用无参版本，它内部会自动去抓上面的 holding_regs_database 和 coil_regs_database
    fs_save_modbus_reg();
}

    // 准备配置丢给核心层的配置结构体
    mb_api_config_t mb_config = {
        .holding_regs  = APP_HOLDING_SIZE > 0 ? holding_regs_database : NULL,
        .holding_len   = APP_HOLDING_SIZE,
        
        .input_regs    = APP_INPUT_SIZE > 0 ? input_regs_database : NULL,
        .input_len     = APP_INPUT_SIZE,
        
        .coil_regs     = APP_COIL_SIZE > 0 ? coil_regs_database : NULL,
        .coil_len      = APP_COIL_SIZE,
        
        .flash_save_cb = app_flash_save_cb
    };



// 3. 初始化默认寄存器值
void mb_default_reg(uint8_t type) {
    if ((type & REG_TYPE_HOLDING) && APP_HOLDING_SIZE > 0) {
        memset(holding_regs_database, 0, sizeof(holding_regs_database));
    }

    if ((type & REG_TYPE_INPUT) && APP_INPUT_SIZE > 0) {
        memset(input_regs_database, 0, sizeof(input_regs_database));
    }

    if ((type & REG_TYPE_COIL) && APP_COIL_SIZE > 0) {
        memset(coil_regs_database, 0, sizeof(coil_regs_database));
    }
}

// 4. 应用层注册入口
void mb_init_reg(void) {

 // 1. 初始化核心 API
    mb_api_init(&mb_config);

    // 2. 挂载硬件 (绝对只能调用这一处！)
    if (fs_mount_medium() != FS_OK) {
        printf("[CRITICAL] W25Q128 挂载失败！检查 QSPI 配置和引脚！\r\n");
        return; 
    } else {
        printf("[OK] W25Q128 挂载成功。\r\n");
    }

    // 3. 尝试从 Flash 加载数据
    fs_err_t load_err = fs_load_modbus_reg(); 
    if (load_err == FS_OK) {
        printf("[OK]The register data was successfully loaded from Flash。\r\n");
    } else {
        printf("[WARN] Flash has no valid data (error code :%d) and is initializing the default value...\r\n", load_err);
        mb_default_reg(REG_TYPE_ALL); // 只有加载失败才去清零
        fs_save_modbus_reg();         // 并创建初始文件
    }
    
    // 4. 允许触发 Flash 写回调
    enable_flash_save_cb = true;
}