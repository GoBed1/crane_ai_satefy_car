#include "app_laser.h"
#include "uart_manage.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdbool.h>

static void process_single_laser(const char *uart_name, uint16_t input_reg_add)
{
    uart_inferface_t *m_obj = uart_manage_get_obj_by_name(uart_name);
    if (m_obj == NULL)
        return;

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
                uint8_t status = buf[3];
                uint16_t distance = 0;

                if (status == 1)
                {
                    distance = (buf[5] << 8) | buf[6];
                    LOGD("[%s] Range Success: %d m\r\n", uart_name, distance);
                    mb_set_input_reg_by_address(input_reg_add, distance);
                }
                else if (status == 0)
                {
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
    if (frame_found)
    {
        lwrb_reset(&m_obj->process_ring_buffer);
    }
}

void process_laser_logic(void)
{

    uart_manage_dma_send_by_name("laser_01", (uint8_t *)laser_single_cmd, sizeof(laser_single_cmd));
    uart_manage_dma_send_by_name("laser_02", (uint8_t *)laser_single_cmd, sizeof(laser_single_cmd));
    // 传感器1 (UART6) -> 写入输入寄存器 32
    process_single_laser("laser_01", INPUT_REG_LASER_01_DISTANCE);

    // 传感器2 (UART2) -> 写入输入寄存器 33
    process_single_laser("laser_02", INPUT_REG_LASER_02_DISTANCE);
}