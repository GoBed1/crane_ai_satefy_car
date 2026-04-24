#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C"
{
#endif

void board_config(void);
void board_create_user_tasks(void);
uint32_t board_get_time_ms(void);
uint32_t board_get_time_s(void);

void board_check_task_heap_stack(const char *name);

#ifdef __cplusplus
}
#endif
