#ifndef APP_GPS_H
#define APP_GPS_H

#include <stdint.h>
#include "sys_def.h"
#include "uart_manage.h"

void gps_rtc_app_init(void);   
void process_gps_logic(void);  

#endif 
