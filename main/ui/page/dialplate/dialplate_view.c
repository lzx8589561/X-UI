#include "dialplate_view.h"
#include "../../resource/resource_pool.h"
#include "../../ui_page_manager.h"

void dialplate_view_bottom_info_create(lv_obj_t* par);
void dialplate_view_top_info_create(lv_obj_t* par);
void dialplate_view_btn_cont_create(lv_obj_t* par);
void dialplate_view_sub_info_grp_create(lv_obj_t* par, SubInfo_t* info, const char* unitText);
lv_obj_t* dialplate_view_btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs);

void dialplate_view_create(lv_obj_t* root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);

    dialplate_view_bottom_info_create(root);
    dialplate_view_top_info_create(root);
    dialplate_view_btn_cont_create(root);

    dialplate_view_ui->anim_timeline = lv_anim_timeline_create();

#define ANIM_DEF(start_time, obj, attr, start, end) \
     {start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true}

#define ANIM_OPA_DEF(start_time, obj) \
     ANIM_DEF(start_time, obj, opa_scale, LV_OPA_TRANSP, LV_OPA_COVER)

    lv_coord_t y_tar_top = lv_obj_get_y(dialplate_view_ui->topInfo.cont);
    lv_coord_t y_tar_bottom = lv_obj_get_y(dialplate_view_ui->bottomInfo.cont);
    lv_coord_t h_tar_btn = lv_obj_get_height(dialplate_view_ui->btnCont.btnRec);

    lv_anim_timeline_wrapper_t wrapper[] =
            {
                    ANIM_DEF(0, dialplate_view_ui->topInfo.cont, y, -lv_obj_get_height(dialplate_view_ui->topInfo.cont), y_tar_top),

                    ANIM_DEF(200, dialplate_view_ui->bottomInfo.cont, y, -lv_obj_get_height(dialplate_view_ui->bottomInfo.cont), y_tar_bottom),
                    ANIM_OPA_DEF(200, dialplate_view_ui->bottomInfo.cont),

                    ANIM_DEF(500, dialplate_view_ui->btnCont.btnMap, height, 0, h_tar_btn),
                    ANIM_DEF(600, dialplate_view_ui->btnCont.btnRec, height, 0, h_tar_btn),
                    ANIM_DEF(700, dialplate_view_ui->btnCont.btnMenu, height, 0, h_tar_btn),
                    LV_ANIM_TIMELINE_WRAPPER_END
            };
    lv_anim_timeline_add_wrapper(dialplate_view_ui->anim_timeline, wrapper);
}

void dialplate_view_delete()
{
    if(dialplate_view_ui->anim_timeline)
    {
        lv_anim_timeline_del(dialplate_view_ui->anim_timeline);
        dialplate_view_ui->anim_timeline = NULL;
    }
}

void dialplate_view_top_info_create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 142);

    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);

    lv_obj_set_style_radius(cont, 27, 0);
    lv_obj_set_y(cont, -36);
    dialplate_view_ui->topInfo.cont = cont;

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, resource_pool_get_font("bahnschrift_65"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "00");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 63);
    dialplate_view_ui->topInfo.labelSpeed = label;

    label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, resource_pool_get_font("bahnschrift_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "km/h");
    lv_obj_align_to(label, dialplate_view_ui->topInfo.labelSpeed, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    dialplate_view_ui->topInfo.labelUint = label;
}

void dialplate_view_bottom_info_create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_size(cont, LV_HOR_RES, 90);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 106);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_set_flex_align(
            cont,
            LV_FLEX_ALIGN_SPACE_EVENLY,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER
    );

    dialplate_view_ui->bottomInfo.cont = cont;

    const char* unitText[4] =
            {
                    "AVG",
                    "Time",
                    "Trip",
                    "ABCalorie"
            };

    for (int i = 0; i < ARRAY_SIZE(dialplate_view_ui->bottomInfo.labelInfoGrp); i++)
    {
        dialplate_view_sub_info_grp_create(
                cont,
                &(dialplate_view_ui->bottomInfo.labelInfoGrp[i]),
                unitText[i]
        );
    }
}

void dialplate_view_sub_info_grp_create(lv_obj_t* par, SubInfo_t* info, const char* unitText)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 93, 39);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
            cont,
            LV_FLEX_ALIGN_SPACE_AROUND,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, resource_pool_get_font("bahnschrift_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    info->lableValue = label;

    label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, resource_pool_get_font("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xb3b3b3), 0);
    lv_label_set_text(label, unitText);
    info->lableUnit = label;

    info->cont = cont;
}

void dialplate_view_btn_cont_create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 40);
    lv_obj_align_to(cont, dialplate_view_ui->bottomInfo.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    /*lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_place(
        cont,
        LV_FLEX_PLACE_SPACE_AROUND,
        LV_FLEX_PLACE_CENTER,
        LV_FLEX_PLACE_CENTER
    );*/

    dialplate_view_ui->btnCont.cont = cont;

    dialplate_view_ui->btnCont.btnMap = dialplate_view_btn_create(cont, resource_pool_get_image("locate"), -80);
    dialplate_view_ui->btnCont.btnRec = dialplate_view_btn_create(cont, resource_pool_get_image("start"), 0);
    dialplate_view_ui->btnCont.btnMenu = dialplate_view_btn_create(cont, resource_pool_get_image("menu"), 80);
}

lv_obj_t* dialplate_view_btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs)
{
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 40, 31);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, 0);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_width(obj, 45, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, 25, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x666666), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xbbbbbb), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff931e), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(obj, 9, 0);

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] = { LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(
            &tran,
            prop,
            lv_anim_path_ease_out,
            200,
            0,
            NULL
    );
    lv_obj_set_style_transition(obj, &tran, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_FOCUSED);

    lv_obj_update_layout(obj);

    return obj;
}

void dialplate_view_appear_anim_start(bool reverse)
{
    lv_anim_timeline_set_reverse(dialplate_view_ui->anim_timeline, reverse);
    lv_anim_timeline_start(dialplate_view_ui->anim_timeline);
}