#include "app_bms.h"
#include "Modbus.h"
#include "usart.h"                       
#include "modbus_tcp_server_interface.h" 
#include "FreeRTOS.h"
#include "task.h"



static void init_modbus_rtu_master(modbusHandler_t *handler, UART_HandleTypeDef *huart, uint16_t *buf, uint16_t buf_len)
{
    // 1. 清空传入的缓存数组
    memset(buf, 0, buf_len * sizeof(uint16_t));

    // 2. 配置 Modbus 主机参数
    handler->uModbusType = MB_MASTER;
    handler->port = huart;             
    handler->u8id = 0;
    handler->u16timeOut = 1000;
    handler->EN_Port = NULL;
    handler->EN_Pin = 0;
    handler->u16regs = buf;          
    handler->u16regsize = buf_len;   
    handler->xTypeHW = USART_HW;
    
    ModbusInit(handler);
    ModbusStart(handler);

}

void init_modbus_master(void)
{
    
    // 初始化 Modbus 主机
    init_modbus_rtu_master(
        &bms_mppt_master,
        &huart8,
        modbus_master_buf,
        sizeof(modbus_master_buf) / sizeof(modbus_master_buf[0])
    );
    LOGI(" modbus master init success !\n");
}

void process_bms_logic(void)
{
    // 读取电池电量信息
    ModbusQuery(&bms_mppt_master, bms_read_telegrams[READ_BATT_LEVEL]);
    int err = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS));
    if (err == OP_OK_QUERY)
    {
        mb_set_input_reg_by_address(INPUT_REG_BMS_BATTERY, bms_read_results[READ_BATT_LEVEL]);
        LOGD("bms  master read success,Battery = %d\n", bms_read_results[READ_BATT_LEVEL]);
    
    }
    else
    {
        LOGE("bms  modbus master read failed : %d\n",err);
    }
}