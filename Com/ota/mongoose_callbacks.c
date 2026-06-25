#include "mongoose_callbacks.h"
#include "main.h"

void mongoose_get_led(struct leds *leds) {
    // leds->led1 = HAL_GPIO_ReadPin(LED0_GPIO_Port, LED0_Pin);  // Read hardware, populate structure
}
void mongoose_set_led(struct leds *leds) {
    // HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, leds->led1); // Read structure, sync to hardware
}