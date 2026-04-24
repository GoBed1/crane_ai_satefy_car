#include "modbus_tcp_server_database.h"
#include "board_flash_system.h" 
#include <string.h>




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

    // 初始化核心 API
    mb_api_init(&mb_config);
fs_mount_medium();
    // // 文件系统初始化 (无参调用)
    // // 内部如果发现文件损坏，会自动调用上面的 mb_default_reg 并保存
    // fs_init_modbus_reg();
// 2. 先挂载硬件
    if (fs_mount_medium() != FS_OK) {
        printf("[CRITICAL] W25Q128 挂载失败！检查 QSPI 配置和引脚！\r\n");
    } else {
        printf("[OK] W25Q128 挂载成功。\r\n");
    }

    // 3. 尝试从 Flash 加载
    fs_err_t load_err = fs_load_modbus_reg(); 
    if (load_err == FS_OK) {
        printf("[OK] 成功从 Flash 加载了寄存器数据。\r\n");
    } else {
        printf("[WARN] Flash 无有效数据 (错误码:%d)，正在初始化默认值...\r\n", load_err);
        mb_default_reg(REG_TYPE_ALL); // 只有加载失败才去清零
        fs_save_modbus_reg();         // 并创建初始文件
    }
    // 允许触发 Flash 写回调
    enable_flash_save_cb = true;
}