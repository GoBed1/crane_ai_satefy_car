#include "app_mppt.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"

static MpptChargeStatus_t parse_mppt_charge_status(uint16_t reg_3201_val)
{
    // 默认为不适用
    MpptChargeStatus_t final_status = MPPT_CHARGE_STATUS_NA; 

    // 1. 提取 D3~D2 位 (充电状态) -> 00:未充电, 01:浮充, 02:提升, 03:均衡
    uint8_t charge_state = (reg_3201_val >> 2) & 0x03;
    
    // 2. 提取 D15~D14 (输入电压状态) -> 00:正常, 01:未接入, 02:过高, 03:错误
    uint8_t input_voltage_state = (reg_3201_val >> 14) & 0x03;
    
    // 3. 提取 D13~D10 (硬件错误标志：MOS管短路/断路、输入过流)
    uint8_t hardware_errors = (reg_3201_val >> 10) & 0x0F;

    // 4. 综合状态机判断 (硬件错误的优先级最高)
    if (input_voltage_state == 2 || input_voltage_state == 3 || hardware_errors != 0) 
    {
        // 发现输入电压过高/错误，或者 MOS管/过流 等硬件故障
        final_status = MPPT_CHARGE_STATUS_ERROR; 
    }
    else if (charge_state == 0)
    {
        final_status = MPPT_CHARGE_STATUS_NOT_CHARGING; 
    }
    else if (charge_state >= 1 && charge_state <= 3)
    {
        final_status = MPPT_CHARGE_STATUS_CHARGING; 
    }

    return final_status;
}

/**
 * @brief MPPT 逻辑处理函数，建议放在轮询任务中定期调用
 */
void process_mppt_logic(void)
{
    static TickType_t last_8s = 0;
    
    // 每 8s 轮询一次 MPPT 数据 
    if (xTaskGetTickCount() - last_8s >= pdMS_TO_TICKS(8000))
    {
        last_8s += pdMS_TO_TICKS(8000);

        ModbusQuery(&bms_mppt_master, mppt_read_telegrams[READ_MPPT_CHARGE_STATUS]);
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
        {
            mb_set_coil_reg_by_address(COIL_REG_MPPT_IS_READABLE, 0); // 正常
            // 读取成功，进行数据解析分类
            MpptChargeStatus_t status = parse_mppt_charge_status(mppt_read_results[READ_MPPT_CHARGE_STATUS]);
            // 将分类后的状态写入 Modbus TCP 寄存器，供上位机读取
            mb_set_input_reg_by_address(INPUT_REG_MPPT_CHARGE_STATUS, (uint16_t)status);
            
            LOGD("MPPT Charge Status = %d (Raw Reg: %04X)\n", status, mppt_read_results[READ_MPPT_CHARGE_STATUS]);
        }
        else
        {
            LOGE("MPPT Charge Status read fail\n");
            // 出现通讯故障（读取失败）时，保险起见向上位机汇报为“不适用”状态
            mb_set_input_reg_by_address(INPUT_REG_MPPT_CHARGE_STATUS, (uint16_t)MPPT_CHARGE_STATUS_NA);
            mb_set_coil_reg_by_address(COIL_REG_MPPT_IS_READABLE, 1); // 异常
        }
    }
}