#include "ui_page_manager.h"

/**
 * 页面定义
 * @param name 页面名称
 * @return 页面处理器
 */
page_base_handler pm_create_page(const char* name){
    APP_CLASS_MATCH(start_up);
    APP_CLASS_MATCH(dialplate);
    return NULL;
}