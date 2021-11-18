#ifndef __STARTUP_VIEW_H
#define __STARTUP_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef struct startup_view_t *startup_view_handler;
struct startup_view_t{
    lv_obj_t *cont;
    lv_obj_t *labelLogo;
    lv_anim_timeline_t *anim_timeline;
};

void startup_view_create(lv_obj_t *root);
void startup_view_delete();

extern startup_view_handler startup_view_ui;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // !__VIEW_H
