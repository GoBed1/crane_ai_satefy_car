#ifndef APP_SYS_MONITOR_H
#define APP_SYS_MONITOR_H

#include "sys_def.h"

// 初始化系统状态监控任务
void process_sys_monitor_logic(void);
void power_control_logic(void);
void net_ping_monitor(void);
void set_system_info_to_input_regs(void);
#endif // APP_SYS_MONITOR_H