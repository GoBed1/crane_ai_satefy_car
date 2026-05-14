#include "app_mppt.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"



/**
 * @brief MPPT 逻辑处理函数，建议放在轮询任务中定期调用
 */
void process_mppt_logic(void)
{
    static TickType_t last_8s = 0;
    
    // 每 8s 轮询一次 MPPT 数据 (时间间隔可根据总线负载情况自行调整)
    if (xTaskGetTickCount() - last_8s >= pdMS_TO_TICKS(8000))
    {
        last_8s += pdMS_TO_TICKS(8000);

        // 1. 读取阵列电压
        ModbusQuery(&bms_mppt_master, mppt_read_telegrams[READ_MPPT_PV_VOLTAGE]);
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
        {
            mb_set_input_reg_by_address(INPUT_REG_MPPT_PV_VOLTAGE, mppt_read_results[READ_MPPT_PV_VOLTAGE]);
            LOGD("MPPT PV Voltage = %d mV\n", mppt_read_results[READ_MPPT_PV_VOLTAGE] );
        }
        else
        {
            LOGE("MPPT PV Voltage read fail\n");
        }

        // 2. 读取阵列电流
        ModbusQuery(&bms_mppt_master, mppt_read_telegrams[READ_MPPT_PV_CURRENT]);
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
        {
            mb_set_input_reg_by_address(INPUT_REG_MPPT_PV_CURRENT, mppt_read_results[READ_MPPT_PV_CURRENT]);
            LOGD("MPPT PV Current = %d mA\n", mppt_read_results[READ_MPPT_PV_CURRENT] );
        }
        else
        {
            LOGE("MPPT PV Current read fail\n");
        }

        // 3. 读取负载电压
        ModbusQuery(&bms_mppt_master, mppt_read_telegrams[READ_MPPT_LOAD_VOLTAGE]);
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
        {
            mb_set_input_reg_by_address(INPUT_REG_MPPT_LOAD_VOLTAGE, mppt_read_results[READ_MPPT_LOAD_VOLTAGE]);
            LOGD("MPPT Load Voltage = %d mV\n", mppt_read_results[READ_MPPT_LOAD_VOLTAGE]);
        }
        else
        {
            LOGE("MPPT Load Voltage read fail\n");
        }

        // 4. 读取负载电流
        ModbusQuery(&bms_mppt_master, mppt_read_telegrams[READ_MPPT_LOAD_CURRENT]);
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
        {
            mb_set_input_reg_by_address(INPUT_REG_MPPT_LOAD_CURRENT, mppt_read_results[READ_MPPT_LOAD_CURRENT]);
            LOGD("MPPT Load Current = %d mA\n", mppt_read_results[READ_MPPT_LOAD_CURRENT]);
        }
        else
        {
            LOGE("MPPT Load Current read fail\n");
        }
    }
}
