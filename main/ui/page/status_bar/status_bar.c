#include <stdio.h>
#include <stdlib.h>
#include "status_bar.h"

#define BATT_USAGE_HEIGHT (lv_obj_get_style_height(ui.battery.img, 0) - 6)
#define BATT_USAGE_WIDTH  (lv_obj_get_style_width(ui.battery.img, 0) - 4)

#define STATUS_BAR_HEIGHT 22

#include "../../resource/resource_pool.h"
#include "../../ui_page_manager.h"
#include "../../app_entity_def.h"


static void status_bar_anim_create(lv_obj_t* contBatt);

static test_data_t *test_data = NULL;

struct
{
    lv_obj_t* cont;

    struct
    {
        lv_obj_t* img;
        lv_obj_t* label;
    } satellite;

    lv_obj_t* imgSD;
    
    lv_obj_t* labelClock;
    
    lv_obj_t* labelRec;

    struct
    {
        lv_obj_t* img;
        lv_obj_t* objUsage;
        lv_obj_t* label;
    } battery;
} ui;

static void status_bar_con_batt_set_opa(lv_obj_t* obj, int32_t opa)
{
    lv_obj_set_style_opa(obj, opa, 0);
}

static void status_bar_on_anim_opa_finish(lv_anim_t* a)
{
    lv_obj_t* obj = (lv_obj_t*)a->var;
    status_bar_con_batt_set_opa(obj, LV_OPA_COVER);
    status_bar_anim_create(obj);
}

static void status_bar_on_anim_height_finish(lv_anim_t* a)
{
    lv_anim_t a_opa;
    lv_anim_init(&a_opa);
    lv_anim_set_var(&a_opa, a->var);
    lv_anim_set_exec_cb(&a_opa, (lv_anim_exec_xcb_t) status_bar_con_batt_set_opa);
    lv_anim_set_ready_cb(&a_opa, status_bar_on_anim_opa_finish);
    lv_anim_set_values(&a_opa, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_early_apply(&a_opa, true);
    lv_anim_set_delay(&a_opa, 500);
    lv_anim_set_time(&a_opa, 500);
    lv_anim_start(&a_opa);
}

static void status_bar_anim(void *var, int32_t v){
    lv_obj_set_height((lv_obj_t*)var, v);
}

static void status_bar_anim_create(lv_obj_t* contBatt)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, contBatt);
    lv_anim_set_exec_cb(&a, status_bar_anim);
    lv_anim_set_values(&a, 0, BATT_USAGE_HEIGHT);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_ready_cb(&a, status_bar_on_anim_height_finish);
    lv_anim_start(&a);
}

static void status_bar_update(lv_timer_t* timer)
{
    /* satellite */
    lv_label_set_text_fmt(ui.satellite.label, "%d", 7);

    lv_obj_clear_flag(ui.imgSD, LV_OBJ_FLAG_HIDDEN);
//    lv_obj_add_flag(ui.imgSD, LV_OBJ_FLAG_HIDDEN);

    /* clock */
    lv_label_set_text_fmt(ui.labelClock, "%02d:%02d", test_data->attr1, test_data->attr2);

    /* battery */
    lv_label_set_text_fmt(ui.battery.label, "%d", 66);

    bool is_batt_charging = true;
    lv_obj_t* contBatt = ui.battery.objUsage;
    static bool is_batt_charging_anim_active = false;
    if(is_batt_charging)
    {
        if(!is_batt_charging_anim_active)
        {
            status_bar_anim_create(contBatt);
            is_batt_charging_anim_active = true;
        }
    }
    else
    {
        if(is_batt_charging_anim_active)
        {
            lv_anim_del(contBatt, NULL);
            status_bar_con_batt_set_opa(contBatt, LV_OPA_COVER);
            is_batt_charging_anim_active = false;
        }
        lv_coord_t height = lv_map(34, 0, 100, 0, BATT_USAGE_HEIGHT);
        lv_obj_set_height(contBatt, height);
    }
}

static lv_obj_t* status_bar_create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);

    lv_obj_set_size(cont, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_y(cont, -STATUS_BAR_HEIGHT);

    /* style1 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), LV_STATE_DEFAULT);

    /* style2 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_60, LV_STATE_USER_1);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_width(cont, 10, LV_STATE_USER_1);

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] =
    {
        LV_STYLE_BG_COLOR,
        LV_STYLE_OPA,
        LV_STYLE_PROP_INV
    };
    lv_style_transition_dsc_init(
        &tran,
        prop,
        lv_anim_path_ease_out,
        200,
        0,
        NULL
    );
    lv_obj_set_style_transition(cont, &tran, LV_STATE_USER_1);

    ui.cont = cont;

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_text_font(&style, resource_pool_get_font("bahnschrift_13"));

    /* satellite */
    lv_obj_t* img = lv_img_create(cont);
    lv_img_set_src(img, resource_pool_get_image("satellite"));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 14, 0);
    ui.satellite.img = img;

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_add_style(label, &style, 0);
    lv_obj_align_to(label, ui.satellite.img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_text(label, "0");
    ui.satellite.label = label;

    /* sd card */
    img = lv_img_create(cont);
    lv_img_set_src(img, resource_pool_get_image("sd_card"));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 50, -1);
    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    ui.imgSD = img;

    /* clock */
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, "00:00");
    lv_obj_center(label);
    ui.labelClock = label;

    /* recorder */
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style, 0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, -50, 0);
    lv_label_set_text(label, "");
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    ui.labelRec = label;

    /* battery */
    img = lv_img_create(cont);
    lv_img_set_src(img, resource_pool_get_image("battery"));
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_img_t* img_ext = (lv_img_t*)img;
    lv_obj_set_size(img, img_ext->w, img_ext->h);
    ui.battery.img = img;

    lv_obj_t* obj = lv_obj_create(img);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_size(obj, BATT_USAGE_WIDTH, BATT_USAGE_HEIGHT);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -2);
    ui.battery.objUsage = obj;

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style, 0);
    lv_obj_align_to(label, ui.battery.img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_text(label, "100%");
    ui.battery.label = label;

    status_bar_set_style(STATUS_BAR_STYLE_TRANSP);

    lv_timer_t* timer = lv_timer_create(status_bar_update, 1000, NULL);
    lv_timer_ready(timer);

    return ui.cont;
}

void status_bar_set_style(status_bar_style_t style)
{
    lv_obj_t* cont = ui.cont;
    if (style == STATUS_BAR_STYLE_TRANSP)
    {
        lv_obj_add_state(cont, LV_STATE_DEFAULT);
        lv_obj_clear_state(cont, LV_STATE_USER_1);
    }
    else if (style == STATUS_BAR_STYLE_BLACK)
    {
        lv_obj_add_state(cont, LV_STATE_USER_1);
    }
    else
    {
        return;
    }
}

static void on_data_event(dc_data_msg_t *d){
    if(strcmp(d->topic,"test_topic") == 0){
        if(d->msg_type == TEST_MSG_TEST_DATA){
            test_data_t *ptr = (test_data_t *) d->data;
            *test_data = *ptr;
        }
    }
}

void status_bar_init(lv_obj_t* par)
{
    test_data = calloc(1, sizeof (test_data_t));
    status_bar_create(par);
    dc_subscribe("test_topic", on_data_event);
}

void status_bar_appear(bool en)
{
    int32_t start = -STATUS_BAR_HEIGHT;
    int32_t end = 0;

    if (!en)
    {
        int32_t temp = start;
        start = end;
        end = temp;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.cont);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 1000);
    lv_anim_set_exec_cb(&a, LV_ANIM_EXEC(y));
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_early_apply(&a, true);
    lv_anim_start(&a);
}
