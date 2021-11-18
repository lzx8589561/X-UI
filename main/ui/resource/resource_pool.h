#ifndef RESOURCE_POOL_H
#define RESOURCE_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

void resource_pool_init();
lv_font_t* resource_pool_get_font(const char* name);
const void* resource_pool_get_image(const char* name);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //RESOURCE_POOL_H
