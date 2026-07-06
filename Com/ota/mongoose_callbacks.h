#ifndef MONGOOSE_CALLBACKS_H
#define MONGOOSE_CALLBACKS_H

#include "mongoose_glue.h"

#ifdef __cplusplus
extern "C" {
#endif

void mongoose_get_led(struct leds *leds);
void mongoose_set_led(struct leds *leds);

#ifdef __cplusplus
}
#endif

#endif