#include <stdio.h>
#include <stdlib.h>
#include "../../ui_page_manager.h"
#include "../status_bar/status_bar.h"
#include "dialplate_view.h"
#include "dialplate.h"
#include "../../app_entity_def.h"

static page_base_handler pb = NULL;
dialplate_view_heandler dialplate_view_ui = NULL;
static lv_obj_t *lastFocus = NULL;
static lv_timer_t *timer = NULL;

static test_data_t *test_data = NULL;


void dialplate_update();

void dialplateon_timer_update(lv_timer_t *timer);

void dialplate_attach_event(lv_obj_t *obj);

static void on_data_event(dc_data_msg_t *d);

static void on_custom_attr_config() {
    set_custom_load_anim_type(pb, LOAD_ANIM_NONE, 500, lv_anim_path_ease_out);
}

static void on_view_load() {
    test_data = calloc(1, sizeof(test_data_t));
    dialplate_view_ui = calloc(1, sizeof(struct dialplate_view_t));
    dialplate_view_create(pb->root);
    dialplate_attach_event(dialplate_view_ui->btnCont.btnMap);
    dialplate_attach_event(dialplate_view_ui->btnCont.btnRec);
    dialplate_attach_event(dialplate_view_ui->btnCont.btnMenu);

    // 订阅主题
    dc_subscribe("test_topic", on_data_event);
}

static void on_view_did_load() {

}

static void on_view_will_appear() {
    lv_group_focus_obj(dialplate_view_ui->btnCont.btnRec);
    status_bar_set_style(STATUS_BAR_STYLE_TRANSP);
    dialplate_update();
    dialplate_view_appear_anim_start(false);
}

static void on_view_did_appear() {
    timer = lv_timer_create(dialplateon_timer_update, 1000, NULL);
}

static void on_view_will_disappear() {
    lv_timer_del(timer);
}

static void on_view_did_disappear() {

}

static void on_view_did_unload() {
    dialplate_view_delete();
    // 清除页面指针
    free(dialplate_view_ui);
    // 取消订阅
    dc_unsubscribe("test_topic", on_data_event);
}

static void on_data_event(dc_data_msg_t *d) {
    if (strcmp(d->topic, "test_topic") == 0) {
        if (d->msg_type == TEST_MSG_TEST_DATA) {
            test_data_t *ptr = (test_data_t *) d->data;
            *test_data = *ptr;
        }
    }
}

void dialplate_on_btn_clicked(lv_obj_t *btn) {
    if (btn == dialplate_view_ui->btnCont.btnMap) {
    } else if (btn == dialplate_view_ui->btnCont.btnMenu) {
        // 点击菜单按钮 取消订阅
        dc_unsubscribe("test_topic", on_data_event);
    } else if (btn == dialplate_view_ui->btnCont.btnRec) {
        // 点击录制按钮 订阅主题
        dc_subscribe("test_topic", on_data_event);
    }
}

void dialplate_on_event(lv_event_t *event) {
    lv_obj_t *obj = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_SHORT_CLICKED) {
        dialplate_on_btn_clicked(obj);
    }

}

void dialplate_attach_event(lv_obj_t *obj) {
    lv_obj_add_event_cb(obj, dialplate_on_event, LV_EVENT_ALL, NULL);
}

void dialplateon_timer_update(lv_timer_t *timer) {
    dialplate_update();
}

void dialplate_update() {
    // 同步请求
    dc_data_msg_t msg = {
            .topic = "test_topic",
            .msg_type = TEST_MSG_SYNC_TEST_DATA,
            .data = "hello",
    };
    dc_sync_req(&msg);

    char buf[16] = "0";
    if(msg.sync_result){
        sprintf(buf, "%s", (char *) msg.sync_result);
        // 释放发来的数据
        free(msg.sync_result);
    }

    lv_label_set_text_fmt(dialplate_view_ui->topInfo.labelSpeed, "%s", buf);


    lv_label_set_text_fmt(dialplate_view_ui->bottomInfo.labelInfoGrp[0].lableValue, "%d km/h", 0);
    lv_label_set_text_fmt(
            dialplate_view_ui->bottomInfo.labelInfoGrp[1].lableValue,
            "a1:%d", test_data->attr1
    );
    lv_label_set_text_fmt(
            dialplate_view_ui->bottomInfo.labelInfoGrp[2].lableValue,
            "a2:%d",
            test_data->attr2
    );
    lv_label_set_text_fmt(
            dialplate_view_ui->bottomInfo.labelInfoGrp[3].lableValue,
            "a3:%d",
            test_data->attr3
    );
}

page_base_handler ui_page_register_dialplate() {
    UI_PAGE_FUN_INIT();
    return pb;
}
