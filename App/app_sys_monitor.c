#include "app_sys_monitor.h"
#include "modbus_tcp_server_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/icmp.h"
#include "lwipopts.h"
#include "lwip/inet_chksum.h"
#include "lwip/netdb.h"
// 系统监控线程主体逻辑
void process_sys_monitor_logic(void)
{

    uint8_t err_bms = 0, err_mppt = 0, err_laser1 = 0, err_laser2 = 0;
    uint8_t err_cctv = 0, err_bridge = 0;

    // 获取当前小车是否在休眠模式
    mb_get_coil_reg_by_address(COIL_REG_IS_IN_SLEEP_STATUS, &is_power_sleep_flag);
    if (is_power_sleep_flag == 1)
    {
        car_main_status = 2; // 休眠模式
        mb_set_holding_reg_by_address(HOLDING_REG_SYSTEM_STATUS, car_main_status);
        return;
    }

    // 读取各个设备的异常状态寄存器
    mb_get_coil_reg_by_address(COIL_REG_BMS_IS_READABLE, &err_bms);
    mb_get_coil_reg_by_address(COIL_REG_MPPT_IS_READABLE, &err_mppt);
    mb_get_coil_reg_by_address(COIL_REG_LASER_01_IS_READABLE, &err_laser1);
    mb_get_coil_reg_by_address(COIL_REG_LASER_02_IS_READABLE, &err_laser2);
    mb_get_coil_reg_by_address(COIL_REG_ERR_STATUS_CCTV, &err_cctv);
    mb_get_coil_reg_by_address(COIL_REG_ERR_STATUS_BRIDGE, &err_bridge);
   // 正常模式下，所有外设必须正常
    if (err_bms == 1 || err_mppt == 1 || err_laser1 == 1 || err_laser2 == 1 || err_cctv == 1 || err_bridge == 1)
    {
        car_main_status = 3; // 异常
    }
    else
    {
        car_main_status = 1; // 正常
    }
    printf("[INFO]car_main_status: %d\n", car_main_status);
    mb_set_holding_reg_by_address(HOLDING_REG_SYSTEM_STATUS, car_main_status);
}
// 系统电源控制
void power_control_logic(void)
{
    uint8_t sleep_cmd = 0, sleep_status = 0;

    // 获取当前的模式指令和状态
    mb_get_coil_reg_by_address(COIL_REG_IS_ENTRY_SLEEP_CMD, &sleep_cmd);
    mb_get_coil_reg_by_address(COIL_REG_IS_IN_SLEEP_STATUS, &sleep_status);
    // ================= 休眠模式 =================
    if (sleep_cmd == 1)
    {
        if (sleep_status == 0) // 刚收到休眠指令
        {
            LOGI("Enter SLEEP Mode: All periphs OFF\n");
            // 物理断电
            HAL_GPIO_WritePin(POWER_3V_GPIO_Port, POWER_3V_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(POWER_LASER_GPIO_Port, POWER_LASER_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(POWER_4G_GPIO_Port, POWER_4G_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BRIDGE_EN_GPIO_Port, BRIDGE_EN_Pin, GPIO_PIN_RESET); 
            // 更新所有单体设备状态为 1(断开)
            mb_set_coil_reg_by_address(COIL_REG_STATUS_3V3, 1);
            mb_set_coil_reg_by_address(COIL_REG_STATUS_5V, 1);
            mb_set_coil_reg_by_address(COIL_REG_STATUS_LASER, 1);
            mb_set_coil_reg_by_address(COIL_REG_STATUS_4G, 1);
            mb_set_coil_reg_by_address(COIL_REG_STATUS_BRIDGE, 1);

            mb_set_coil_reg_by_address(COIL_REG_IS_IN_SLEEP_STATUS, 1); // 反馈已休眠
        }
        return;
    }
    else if (sleep_cmd == 0 && sleep_status == 1)
    {
        LOGI("Exit SLEEP Mode\n");
        is_power_sleep_flag = 0; // 同步本地状态
        mb_set_coil_reg_by_address(COIL_REG_IS_IN_SLEEP_STATUS, 0); // 退出休眠
    }
    
    // ================= 独立设备供电控制 =================
    uint8_t cmd = 0, status = 0;

    // --- 3.3V ---
    mb_get_coil_reg_by_address(COIL_REG_CMD_3V3_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_3V3, &status);
    if (cmd == 0 && status == 1)
    {
        LOGI("3.3V Power ON\n");
        HAL_GPIO_WritePin(POWER_3V_GPIO_Port, POWER_3V_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_3V3, 0);
    }
    else if (cmd == 1 && status == 0)
    {
        LOGI("3.3V Power OFF\n");
        HAL_GPIO_WritePin(POWER_3V_GPIO_Port, POWER_3V_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_3V3, 1);
    }

    // --- 5V ---
    mb_get_coil_reg_by_address(COIL_REG_CMD_5V_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_5V, &status);
    if (cmd == 0 && status == 1)
    {
        LOGI("5V Power ON\n");
        HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_5V, 0);
    }
    else if (cmd == 1 && status == 0)
    {
        LOGI("5V Power OFF\n");
        HAL_GPIO_WritePin(POWER_5V_GPIO_Port, POWER_5V_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_5V, 1);
    }

    // --- LASER ---
    mb_get_coil_reg_by_address(COIL_REG_CMD_LASER_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_LASER, &status);
    if (cmd == 0 && status == 1)
    {
        LOGI("LASER Power ON\n");
        HAL_GPIO_WritePin(POWER_LASER_GPIO_Port, POWER_LASER_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_LASER, 0);
    }
    else if (cmd == 1 && status == 0)
    {
        LOGI("LASER Power OFF\n");
        HAL_GPIO_WritePin(POWER_LASER_GPIO_Port, POWER_LASER_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_LASER, 1);
    }

    // --- 4G ---
    mb_get_coil_reg_by_address(COIL_REG_CMD_4G_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_4G, &status);
    if (cmd == 0 && status == 1)
    {
        LOGI("4G Module Power ON\n");
        HAL_GPIO_WritePin(POWER_4G_GPIO_Port, POWER_4G_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_4G, 0);
    }
    else if (cmd == 1 && status == 0)
    {
        LOGI("4G Module Power OFF\n");
        HAL_GPIO_WritePin(POWER_4G_GPIO_Port, POWER_4G_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_4G, 1);
    }
    // --- BRIDGE---
    mb_get_coil_reg_by_address(COIL_REG_CMD_BRIDGE_EN, &cmd);
    mb_get_coil_reg_by_address(COIL_REG_STATUS_BRIDGE, &status);
    if (cmd == 0 && status == 1)
    {
        LOGI("BRIDGE Power ON\n");
        HAL_GPIO_WritePin(BRIDGE_EN_GPIO_Port, BRIDGE_EN_Pin, GPIO_PIN_SET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_BRIDGE, 0);
    }
    else if (cmd == 1 && status == 0)
    {
        LOGI("BRIDGE Power OFF\n");
        HAL_GPIO_WritePin(BRIDGE_EN_GPIO_Port, BRIDGE_EN_Pin, GPIO_PIN_RESET);
        mb_set_coil_reg_by_address(COIL_REG_STATUS_BRIDGE, 1);
    }
}

// TCP 端口探测
int tcp_port_ping(const char *target_ip, uint16_t port, uint32_t timeout_ms)
{
    int sock;
    struct sockaddr_in target_addr;
    int ret = 0;

    // 创建 TCP 套接字
    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return 0;

    target_addr.sin_family = AF_INET;
    target_addr.sin_port = lwip_htons(port);
    target_addr.sin_addr.s_addr = inet_addr(target_ip);

    // 将 Socket 设置为非阻塞模式，防止 connect 死等
    int flags = lwip_fcntl(sock, F_GETFL, 0);
    lwip_fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // 发起连接
    int conn_res = lwip_connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr));

    if (conn_res == 0)
    {
        ret = 1;
    }
    else if (conn_res < 0 && errno == EINPROGRESS)
    {
        fd_set write_set;
        FD_ZERO(&write_set);
        FD_SET(sock, &write_set);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        // 使用 select 等待连接结果
        int select_res = lwip_select(sock + 1, NULL, &write_set, NULL, &tv);
        if (select_res > 0 && FD_ISSET(sock, &write_set))
        {
            int error = 0;
            socklen_t len = sizeof(error);
            lwip_getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
            if (error == 0)
            {
                ret = 1; // 成功建立连接
            }
        }
    }

    lwip_close(sock);
    return ret;
}
// 网络设备 Ping 监控函数
void net_ping_monitor(void)
{
    if (is_power_sleep_flag == 1)
    {
        return; 
    }
    // 探测 80 端口，给 1.5 秒超时时间
    if (tcp_port_ping(IP_ADDR_CCTV, CCTV_PORT, PING_TIMEOUT_MS) == 1)
    {
        mb_set_coil_reg_by_address(COIL_REG_ERR_STATUS_CCTV, 0); // 在线
        LOGI("CCTV Service is ONLINE!\n");
    }
    else
    {
        mb_set_coil_reg_by_address(COIL_REG_ERR_STATUS_CCTV, 1); // 掉线
        LOGE("CCTV Service is OFFLINE! ! !\n");
    }
    // 探测 网桥 80 端口
    if (tcp_port_ping(IP_ADDR_BRIDGE, BRIDGE_PORT, PING_TIMEOUT_MS) == 1)
    {
        mb_set_coil_reg_by_address(COIL_REG_ERR_STATUS_BRIDGE, 0); // 在线
        LOGI("BRIDGE Service is ONLINE!\n");
    }
    else
    {
        mb_set_coil_reg_by_address(COIL_REG_ERR_STATUS_BRIDGE, 1); // 掉线
        LOGE("BRIDGE Service is OFFLINE! ! !\n");
    }
}