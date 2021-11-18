#ifndef __DIALPLATE_VIEW_H
#define __DIALPLATE_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef struct dialplate_view_t *dialplate_view_heandler;

typedef struct
{
    lv_obj_t* cont;
    lv_obj_t* lableValue;
    lv_obj_t* lableUnit;
} SubInfo_t;

struct dialplate_view_t{
    struct
    {
        lv_obj_t* cont;
        lv_obj_t* labelSpeed;
        lv_obj_t* labelUint;
    } topInfo;

    struct
    {
        lv_obj_t* cont;
        SubInfo_t labelInfoGrp[4];
    } bottomInfo;

    struct
    {
        lv_obj_t* cont;
        lv_obj_t* btnMap;
        lv_obj_t* btnRec;
        lv_obj_t* btnMenu;
    } btnCont;

    lv_anim_timeline_t* anim_timeline;
};

void dialplate_view_create(lv_obj_t *root);
void dialplate_view_delete();
void dialplate_view_appear_anim_start(bool reverse);

extern dialplate_view_heandler dialplate_view_ui;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // !__VIEW_H
