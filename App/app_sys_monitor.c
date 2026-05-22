#include "app_sys_monitor.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

// 系统监控线程主体逻辑
void process_sys_monitor_logic(void)
{
   
        uint8_t err_bms = 0, err_mppt = 0, err_laser1 = 0, err_laser2 = 0;
        uint8_t err_cctv = 0, err_bridge = 0;

        // 读取各个设备的异常状态寄存器
        mb_get_coil_reg_by_address(COIL_REG_BMS_IS_READABLE, &err_bms);
        mb_get_coil_reg_by_address(COIL_REG_MPPT_IS_READABLE, &err_mppt);
        mb_get_coil_reg_by_address(COIL_REG_LASER_01_IS_READABLE, &err_laser1);
        mb_get_coil_reg_by_address(COIL_REG_LASER_02_IS_READABLE, &err_laser2);
        
        // mb_get_coil_reg_by_address(COIL_REG_ERR_CCTV, &err_cctv); 
        // mb_get_coil_reg_by_address(COIL_REG_ERR_BRIDGE, &err_bridge); 

        // 获取当前小车是否在待机/省电模式
        mb_get_coil_reg_by_address(COIL_REG_STATUS_IS_IN_STANDBY, &is_power_save_flag);

        if (err_bms == 1 || err_mppt == 1 || err_laser1 == 1 || err_laser2 == 1 || err_cctv == 1 || err_bridge == 1)
        {
            car_main_status = 2; // Fault (异常)
        }
        else
        {
            // 走到这里说明没有任何设备是 '1' (异常)
            if (is_power_save_flag == 1)
            {
                car_main_status = 0; // 省电模式且设备正常
                mb_set_coil_reg_by_address(COIL_REG_CMD_CCTV_EN, 1); // cctv断电
            }
            else
            {
                car_main_status = 1; // 正常模式且设备正常
            }
        }

        // 总状态写入保持寄存器
        mb_set_holding_reg_by_address(HOLDING_REG_SYSTEM_STATUS, car_main_status);

    }


void power_control_logic(void)
{
  uint8_t cmd = 0, status = 0;

    // ================= 3.3V 供电控制 =================
    // 0=供电/开，1=断电/关
    mb_get_coil_reg_by_address(COIL_REG_CMD_3V3_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_3V3, &status);
    
    if (cmd == 0 && status == 1) {
        LOGI("3.3V Power ON\n");
        HAL_GPIO_WritePin(POWER_3V_GPIO_Port, POWER_3V_Pin, GPIO_PIN_SET); 
        mb_set_coil_reg_by_address(COIL_REG_STATUS_3V3, 0);              
    } 
    else if (cmd == 1 && status == 0) {
        LOGI("3.3V Power OFF\n");
        HAL_GPIO_WritePin(POWER_3V_GPIO_Port, POWER_3V_Pin, GPIO_PIN_RESET); 
        mb_set_coil_reg_by_address(COIL_REG_STATUS_3V3, 1);                
    }

    // ================= 5V 供电控制 =================
    mb_get_coil_reg_by_address(COIL_REG_CMD_5V_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_5V, &status);
    
    if (cmd == 0 && status == 1) {
        LOGI("5V Power ON\n");
        HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_5V, 0);
    } 
    else if (cmd == 1 && status == 0) {
        LOGI("5V Power OFF\n");
        HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_5V, 1);
    }

    // ================= CCTV 供电控制 =================
    mb_get_coil_reg_by_address(COIL_REG_CMD_CCTV_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_CCTV, &status);
    
    if (cmd == 0 && status == 1) {
        LOGI("CCTV Power ON\n");
        HAL_GPIO_WritePin(POWER_CCTV_GPIO_Port, POWER_CCTV_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_CCTV, 0);
    } 
    else if (cmd == 1 && status == 0) {
        LOGI("CCTV Power OFF\n");
        HAL_GPIO_WritePin(POWER_CCTV_GPIO_Port, POWER_CCTV_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_CCTV, 1);
    }

    // ================= 4G 模块供电控制 =================
    mb_get_coil_reg_by_address(COIL_REG_CMD_4G_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_4G, &status);
    
    if (cmd == 0 && status == 1) {
        LOGI("4G Module Power ON\n");
        HAL_GPIO_WritePin(POWER_4G_GPIO_Port, POWER_4G_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_4G, 0);
    } 
    else if (cmd == 1 && status == 0) {
        LOGI("4G Module Power OFF\n");
        HAL_GPIO_WritePin(POWER_4G_GPIO_Port, POWER_4G_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_4G, 1);
    }
}
