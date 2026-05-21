#include "app_laser.h"
#include "uart_manage.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdbool.h>
static uint32_t laser_offline_cnt_01 = 0;
static uint32_t laser_offline_cnt_02 = 0;
static void process_single_laser(const char *uart_name, uint16_t input_reg_add, uint16_t err_reg_add, uint32_t *offline_cnt)
{

    uart_inferface_t *m_obj = uart_manage_get_obj_by_name(uart_name);
    if (m_obj == NULL) return;

    lwrb_sz_t available = lwrb_get_full(&m_obj->process_ring_buffer);
    bool frame_found = false;

    while (available >= 8 && !frame_found)
    {
        uint8_t buf[8];
        lwrb_peek(&m_obj->process_ring_buffer, 0, buf, 3);
        if (buf[0] == 0x55 && buf[1] == 0xAA && buf[2] == 0x88)
        {
            lwrb_read(&m_obj->process_ring_buffer, buf, 8);
            uint8_t checksum = buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6];
            
            if (checksum == buf[7])
            {
                // 收到合法数据，清零离线计数器，并上报正常 
                *offline_cnt = 0; 
                mb_set_coil_reg_by_address(err_reg_add, 0); 
                
                uint8_t status = buf[3];
                uint16_t distance = 0;
                if (status == 1) {
                    distance = (buf[5] << 8) | buf[6];
                    LOGD("[%s] Range Success: %d m\r\n", uart_name, distance);
                    mb_set_input_reg_by_address(input_reg_add, distance);
                } else if (status == 0) {
                    distance = 0;
                    LOGE("[%s] Measure Failed! Write 0m.\r\n", uart_name);
                    mb_set_input_reg_by_address(input_reg_add, distance);
                }
                frame_found = true;
            }
             else
            {
                LOGE("[%s]  Checksum Error! Calc:0x%02X, Recv:0x%02X\r\n", uart_name, checksum, buf[7]);
            }

            // 更新可用字节数
            available = lwrb_get_full(&m_obj->process_ring_buffer);
        }
        else
        {
            // 帧头不匹配，丢弃1个字节，继续向后寻找正确的帧头
            lwrb_read(&m_obj->process_ring_buffer, buf, 1);
            available--;
        }
    }
    if (frame_found)//防止回显数据干扰正常数据解析，增加一个条件判断，只有当成功解析到一帧数据时才清空环形缓冲区
    {
        lwrb_reset(&m_obj->process_ring_buffer);
    }
    else
    {
        // 超时逻辑 
        (*offline_cnt)++;
        // 假设该函数1秒调用一次，大于3次(3秒)没解析到新数据，视为掉线
        if (*offline_cnt > 3) 
        {
            mb_set_coil_reg_by_address(err_reg_add, 1); // 标记异常
            mb_set_input_reg_by_address(input_reg_add, 0); // 距离强制写 0
        }
    }
}

void process_laser_logic(void)
{

    uart_manage_dma_send_by_name("laser_01", (uint8_t *)laser_single_cmd, sizeof(laser_single_cmd));
    uart_manage_dma_send_by_name("laser_02", (uint8_t *)laser_single_cmd, sizeof(laser_single_cmd));
    // 传感器1 (UART6) -> 写入输入寄存器 32
    process_single_laser("laser_01", INPUT_REG_LASER_01_DISTANCE, COIL_REG_LASER_01_IS_READABLE, &laser_offline_cnt_01);
    // 传感器2 (UART2) -> 写入输入寄存器 33
    process_single_laser("laser_02", INPUT_REG_LASER_02_DISTANCE, COIL_REG_LASER_02_IS_READABLE, &laser_offline_cnt_02);
}
