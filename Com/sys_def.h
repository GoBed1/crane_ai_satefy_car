#ifndef SYS_DEF_H
#define SYS_DEF_H

#include <stdint.h>
#include "main.h"
#include "modbus.h"
#include "ModbusConfig.h"
/* ========================================================================= */
/* 日志定义                                                     */
/* ========================================================================= */
#define LOGD(...) printf("[DEBUG] " __VA_ARGS__)
#define LOGI(...) printf("[INFO]  " __VA_ARGS__)
#define LOGE(...) printf("[ERROR] " __VA_ARGS__)
/* ========================================================================= */
/* BMS 通信宏定义                                                         */
/* ========================================================================= */
#define SLAVE_BMS_ID 2                         // 外部 BMS 从机地址
#define REG_BATTERY_LEVEL 0x0000               // 获取：电池电量寄存器地址
#define REG_TOTAL_CURRENT 0x0001               // 获取：总电流
#define REG_TOTAL_VOLTAGE 0x0002               // 获取：总电压
#define REG_REMAIN_DISCHARGE 0x0007            // 获取：剩余放电时间
#define REG_REMAIN_CHARGE 0x0008               // 获取：剩余充电时间
#define INPUT_REG_BMS_BATTERY 17               // [上报] BMS当前电量
#define INPUT_REG_BMS_REMAIN_DISCHARGE_TIME 18 // [上报] BMS剩余放电时间
#define INPUT_REG_BMS_REMAIN_CHARGE_TIME 19    // [上报] BMS剩余充电时间
#define INPUT_REG_BMS_TOTAL_VOLTAGE 20         // [上报] BMS总电压
#define INPUT_REG_BMS_TOTAL_CURRENT 21         // [上报] BMS总电流
#define MODBUS_WAIT_TIMEOUT_MS 1000            // Modbus等待超时时间

#define COIL_REG_BMS_IS_CHARGING 21 // [上报] BMS是否正在充电,0:未充电 1:正在充电

// 充电/放电时间采样缓存数组大小
#define BMS_SAMPLE_BUFFER_SIZE 30 // 充电/放电时间，采样缓存数组大小
#define BMS_SAMPLE_VALID_COUNT 20 // 实际计算平均值的采样数
/* ========================================================================= */
/* MPPT 通信宏定义                                                         */
/* ========================================================================= */
#define SLAVE_MPPT_ID 1 // MPPT 从机默认ID
// MPPT 寄存器地址定义 (读功能码04)
#define REG_MPPT_PV_VOLTAGE 0x3100   // 阵列电压
#define REG_MPPT_PV_CURRENT 0x3101   // 阵列电流
#define REG_MPPT_LOAD_VOLTAGE 0x310C // 负载电压
#define REG_MPPT_LOAD_CURRENT 0x310D // 负载电流
// mppt存放在输入寄存器中的地址定义（上报）
#define INPUT_REG_MPPT_PV_VOLTAGE 22   // [上报] MPPT阵列电压
#define INPUT_REG_MPPT_PV_CURRENT 23   // [上报] MPPT阵列电流
#define INPUT_REG_MPPT_LOAD_VOLTAGE 24 // [上报] MPPT负载电压
#define INPUT_REG_MPPT_LOAD_CURRENT 25 // [上报] MPPT负载电流

#define REG_MPPT_DEVICE_STATUS 0x3201 // 设备充电状态寄存器地址 (功能码 04)
#define INPUT_REG_MPPT_CHARGE_STATUS 25 // [上报] MPPT是否正在充电,0:未充电 1:正在充电 2:错误 3：不适用
/* ========================================================================= */
/* GPS 模块相关配置 和 休眠配置                                           */
/* ========================================================================= */
#define TEST_GPS_NMEA_PARSER 0 // 开启本地假数据测试

#define WT_RTK_UM982 1
#define WT_GPS_UM626N 2
#define WT_GPS_6N 3

#ifndef GPS_TYPE_STD
#define GPS_TYPE_STD WT_GPS_6N
#endif

#define RTC_BKP_MAGIC_NUMBER 0x5AA5 // RTC备份域校验魔数，用于判断掉电保持

// --- 休眠系统内部控制寄存器 ---
#define HOLDING_REG_POWER_ON_TIME 4  // 定时开机时间
#define HOLDING_REG_POWER_OFF_TIME 5 // 定时关机时间
#define HOLDING_REG_RTC_TIME 3       // 当前RTC时间
#define COIL_REG_STANDBY_ENABLE 20   // 休眠使能开关
#define COIL_REG_CMD_IS_ENTRY_STANDBY 5 // 上位机写，1：允许待机 / 0：不允许待机
#define COIL_REG_STATUS_IS_IN_STANDBY 20 //0：正常模式 ， 1：当前省电状态
// 时间格式：高字节=小时 / 低字节=分钟，例如 0x1500 = 21:00
#define POWER_OFF_DEFAULT ((10 << 8) | 22) // 默认关机 22:10
#define POWER_ON_DEFAULT ((10 << 8) | 24)  // 默认开机 24:10

#define TIMEZONE_OFFSET_BEIJING 8 // 北京时间相对于UTC的时区偏移

/* ========================================================================= */
/* 激光测距定义                                                               */
/* ========================================================================= */
#define INPUT_REG_LASER_01_DISTANCE 32 // [上报] 激光测距仪01距离
#define INPUT_REG_LASER_02_DISTANCE 33 // [上报] 激光测距仪02距离
/* ========================================================================= */
/* 小车系统异常定义                                                               */
/* ========================================================================= */
#define HOLDING_REG_SYSTEM_STATUS 6  // 总系统状态寄存器地址
#define COIL_REG_BMS_IS_READABLE 23   // 电池BMS是否能读到数据
#define COIL_REG_MPPT_IS_READABLE 24   // MPPT是否能读到数据
#define COIL_REG_LASER_01_IS_READABLE 25   // 激光测距1是否读取数据
#define COIL_REG_LASER_02_IS_READABLE 26   // 激光测距2是否读取数据
/* ========================================================================= */
/* 供电控制与状态宏定义 (根据图片映射)                                        */
/* ========================================================================= */
// 控制命令 (上位机下发, 0=断电, 1=供电, 默认1)
#define COIL_REG_CMD_3V3_EN    1   // 3.3V 供电控制 (LAN8742, 激光)
#define COIL_REG_CMD_5V_EN     2   // 5V 供电控制 (4G, GPS)
#define COIL_REG_CMD_CCTV_EN   3   // CCTV / 省电模式控制
#define COIL_REG_CMD_4G_EN     4   // 4G 模块单独控制

// 状态反馈 (下位机上报, 开=1, 关=0)
#define COIL_REG_STATUS_3V3    16  // 3.3V 供电状态
#define COIL_REG_STATUS_5V     17  // 5V 供电控制状态
#define COIL_REG_STATUS_CCTV   18  // CCTV / 省电模式状态
#define COIL_REG_STATUS_4G     19  // 4G 模块开关状态
/* ========================================================================= */
/* 网络设备 Ping 监控定义                                                      */
/* ========================================================================= */
// 设备 IP 地址 
#define IP_ADDR_CCTV    "192.168.61.52" // CCTV 摄像头 IP
#define IP_ADDR_BRIDGE  "192.168.61.62" // 网桥 IP

#define COIL_REG_ERR_STATUS_BRIDGE   22   // 网桥 是否掉线 (0:正常 1:掉线)
#define COIL_REG_ERR_STATUS_CCTV     27   // CCTV 是否掉线 (0:正常 1:掉线)

#define PING_TIMEOUT_MS 1500 // Ping 超时时间，单位毫秒
#define CCTV_PORT 80          // CCTV 服务端口
/* ====================相关结构体定义===================================================== */
// MPPT 读取消息索引
typedef enum
{
    READ_MPPT_PV_VOLTAGE = 0,
    READ_MPPT_PV_CURRENT,
    READ_MPPT_LOAD_VOLTAGE,
    READ_MPPT_LOAD_CURRENT,
    READ_MPPT_CHARGE_STATUS,
    READ_MPPT_MSG_COUNT
} MpptReadMsgIdx_t;

typedef enum
{
    READ_BATT_LEVEL = 0,
    READ_REMAIN_DISCHARGE,
    READ_REMAIN_CHARGE,
    READ_TOTAL_VOLTAGE,
    READ_TOTAL_CURRENT,
    READ_MSG_COUNT
} BmsReadMsgIdx_t;

typedef enum
{
    ERR_NONE = 0,          // 正常状态
    ERR_BMS_READ_FAIL = 3, // BMS 读取失败
} SystemErrorCode_t;

// MPPT 充电状态枚举
typedef enum {
    MPPT_CHARGE_STATUS_NOT_CHARGING = 0, // 未充电
    MPPT_CHARGE_STATUS_CHARGING     = 1, // 充电中
    MPPT_CHARGE_STATUS_ERROR        = 2, // 错误
    MPPT_CHARGE_STATUS_NA           = 3  // 不适用 (Not Applicable)
} MpptChargeStatus_t;

// 定义 ICMP 报文结构体，摆脱对外部 ping.h
struct icmp_echo_packet {
    uint8_t type;       // 类型 (8 代表请求)
    uint8_t code;       // 代码 (0)
    uint16_t chksum;    // 校验和
    uint16_t id;        // 标识符
    uint16_t seqno;     // 序号
    uint8_t data[32];   // 32字节的填充假数据
};

extern modbusHandler_t bms_mppt_master; // BMS Modbus 主机句柄
extern uint16_t modbus_master_buf[128]; // Modbus 主机共用缓冲区
extern modbus_t bms_read_telegrams[READ_MSG_COUNT];
extern uint16_t bms_read_results[READ_MSG_COUNT]; // 存储从 BMS 读取的结果
// 放电时间全局变量
extern uint16_t discharge_samples[BMS_SAMPLE_BUFFER_SIZE];
extern uint8_t discharge_idx;
extern uint8_t discharge_count;
// 充电时间全局变量
extern uint16_t charge_samples[BMS_SAMPLE_BUFFER_SIZE];
extern uint8_t charge_idx;
extern uint8_t charge_count;

// 外部引用的数据和报文数组
extern uint16_t mppt_read_results[READ_MPPT_MSG_COUNT];
extern modbus_t mppt_read_telegrams[READ_MPPT_MSG_COUNT];

// gps模块相关
extern uint16_t off_hhmm;    // 关机时间
extern uint16_t on_hhmm;     // 开机时间
extern uint8_t is_standby_flag; // 待机模式是否使能,1:使能,可待机 0:不使能，不能待机

// 激光测距相关
extern const uint8_t laser_single_cmd[8]; // 激光测距单次测距指令
// 是否进入省电模式的标志，1:进入省电模式 0:正常模式
extern uint8_t is_power_save_flag; 
// 0：待机省电且设备正常 / 1：正常使用且设备正常 / 2：异常（查看coil-22~27）
extern uint16_t car_main_status;
#endif                       // SYS_DEF_H