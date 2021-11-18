#ifndef __STATUS_BAR_H
#define __STATUS_BAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef enum
{
    STATUS_BAR_STYLE_TRANSP,
    STATUS_BAR_STYLE_BLACK,
    STATUS_BAR_STYLE_MAX
} status_bar_style_t;

void status_bar_init(lv_obj_t* par);
void status_bar_set_style(status_bar_style_t style);
void status_bar_appear(bool en);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif
