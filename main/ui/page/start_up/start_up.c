#include <stdlib.h>
#include "start_up.h"
#include "start_up_view.h"
#include "../../ui_page_manager.h"
#include "../status_bar/status_bar.h"

static page_base_handler pb = NULL;
startup_view_handler startup_view_ui = NULL;

static void start_up_on_timer(lv_timer_t* timer);
static void start_up_on_event(lv_event_t* event);

static void on_custom_attr_config()
{
    // 开启页面缓存
    set_custom_cache_enable(pb, false);
    // 禁用动画
    set_custom_load_anim_type(pb, LOAD_ANIM_NONE, 500, lv_anim_path_ease_out);
}

static void on_view_load()
{
    startup_view_ui = calloc(1, sizeof (struct startup_view_t));
    startup_view_create(pb->root);
    lv_timer_t* timer = lv_timer_create(start_up_on_timer, 2000, pb);
    lv_timer_set_repeat_count(timer, 1);
}

static void on_view_did_load()
{
    lv_obj_fade_out(pb->root, 500, 1500);
}

static void on_view_will_appear()
{
    lv_anim_timeline_start(startup_view_ui->anim_timeline);
}

static void on_view_did_appear()
{

}

static void on_view_will_disappear()
{

}

static void on_view_did_disappear()
{
    status_bar_appear(true);
}

static void on_view_did_unload()
{
    startup_view_delete();
    // 清除页面指针
    free(startup_view_ui);
}

void start_up_on_timer(lv_timer_t* timer)
{
    // 开始跳转到首页
    pm_push("page/dialplate", NULL);
}

void start_up_on_event(lv_event_t* event)
{
//    lv_obj_t* obj = lv_event_get_target(event);
//    lv_event_code_t code = lv_event_get_code(event);
//    Startup* instance = (Startup*)lv_obj_get_user_data(obj);
//
//    if (obj == instance->root)
//    {
//        if (code == LV_EVENT_LEAVE)
//        {
//            //instance->Manager->Pop();
//        }
//    }
}

page_base_handler ui_page_register_start_up(){
    UI_PAGE_FUN_INIT();
    return pb;
}
