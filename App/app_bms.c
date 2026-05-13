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
    handler->u16timeOut = 500;
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
        sizeof(modbus_master_buf) / sizeof(modbus_master_buf[0]));
    LOGI(" modbus master init success !\n");
}

void process_bms_logic(void)
{
    static TickType_t last_500ms = 0;
    static TickType_t last_7s = 0;
    static TickType_t last_10s = 0;
    if (xTaskGetTickCount() - last_500ms >= pdMS_TO_TICKS(500))
    {
        last_500ms += pdMS_TO_TICKS(500);

        // 采样放电时间
        ModbusQuery(&bms_mppt_master, bms_read_telegrams[READ_REMAIN_DISCHARGE]);
        int err1 = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS));
        if (err1 == OP_OK_QUERY)
        {
            uint16_t val = bms_read_results[READ_REMAIN_DISCHARGE];
            if (val != 0xFFFF)
            {
                discharge_samples[discharge_idx % BMS_SAMPLE_VALID_COUNT] = val;
                discharge_idx++;
                if (discharge_count < BMS_SAMPLE_VALID_COUNT)
                    discharge_count++;
            }
        }
        else
        {
            LOGE("READ_REMAIN_DISCHARGE read fail : %d \n", err1);
        }

        // 采样充电时间
        ModbusQuery(&bms_mppt_master, bms_read_telegrams[READ_REMAIN_CHARGE]);
        int err2 = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS));
        if (err2 == OP_OK_QUERY)
        {
            uint16_t val = bms_read_results[READ_REMAIN_CHARGE];
            if (val != 0xFFFF)
            {
                charge_samples[charge_idx % BMS_SAMPLE_VALID_COUNT] = val;
                charge_idx++;
                if (charge_count < BMS_SAMPLE_VALID_COUNT)
                    charge_count++;
            }
        }
        else
        {
            LOGE("bms charge time modbus master read fail %d \n", err2);
        }
    }

    // 每 10s 执行一次
    if (xTaskGetTickCount() - last_10s >= pdMS_TO_TICKS(10000))
    {
        last_10s += pdMS_TO_TICKS(10000);
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
            LOGE("bms  modbus master read failed : %d\n", err);
        }
        // 每500ms采样一次放电时间，每10s执行一次平均值放入寄存器（剩余放电时间）
            if (discharge_count > 0)
            {
                uint16_t sum = 0;
                for (uint8_t i = 0; i < discharge_count; i++)
                {
                    sum += discharge_samples[i];
                }
                uint16_t avg = (uint16_t)(sum / discharge_count);

                mb_set_input_reg_by_address(INPUT_REG_BMS_REMAIN_DISCHARGE_TIME, avg);
                LOGD("remain discharge time avg = %d min\n", avg);
            }
            else
            {
                mb_set_input_reg_by_address(INPUT_REG_BMS_REMAIN_DISCHARGE_TIME, 0xFFFF);
                LOGI("no discharge at present , inputReg write 0xFFFF.\n");
            }
            memset(discharge_samples, 0, sizeof(discharge_samples));
            discharge_idx = 0;
            discharge_count = 0;

            // 每500ms采样一次充电时间，每10s执行一次平均值放入寄存器（剩余充电时间）
            if (charge_count > 0)
            {
                uint32_t sum = 0;
                for (uint8_t i = 0; i < charge_count; i++)
                {
                    sum += charge_samples[i];
                }
                mb_set_input_reg_by_address(INPUT_REG_BMS_REMAIN_CHARGE_TIME, (uint16_t)(sum / charge_count));
                LOGD("remain charge time avg = %d min\n", (uint16_t)(sum / charge_count));
            }
            else
            {
                mb_set_input_reg_by_address(INPUT_REG_BMS_REMAIN_CHARGE_TIME, 0xFFFF);
                LOGI("no charge at present , inputReg write 0xFFFF.\n");
            }
            memset(charge_samples, 0, sizeof(charge_samples));
            charge_idx = 0;
            charge_count = 0;
    }
    // 每 7s 读取总电压和电流信息
    if (xTaskGetTickCount() - last_7s >= pdMS_TO_TICKS(7000))
    {
        last_7s += pdMS_TO_TICKS(7000);
         // 读取总电压
            ModbusQuery(&bms_mppt_master, bms_read_telegrams[READ_TOTAL_VOLTAGE]);
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
            {
                mb_set_input_reg_by_address(INPUT_REG_BMS_TOTAL_VOLTAGE, bms_read_results[READ_TOTAL_VOLTAGE]);
                LOGD("bms total voltage = %d\n", bms_read_results[READ_TOTAL_VOLTAGE]);
            }
            else
            {
                LOGE("bms total voltage modbus master read fail  \n");
            }

            // 读取总电流及充放电状态
            ModbusQuery(&bms_mppt_master, bms_read_telegrams[READ_TOTAL_CURRENT]);
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MODBUS_WAIT_TIMEOUT_MS)) == OP_OK_QUERY)
            {
                int16_t val = (int16_t)bms_read_results[READ_TOTAL_CURRENT];
                mb_set_input_reg_by_address(INPUT_REG_BMS_TOTAL_CURRENT, (uint16_t)(-val));
                LOGD("bms total current =%d\n",  val);
            }
            else
            {
                LOGE("bms total current modbus master read fail  \n");
            }
    }

}