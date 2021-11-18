#ifndef UI_PAGE_MANAGER_H
#define UI_PAGE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/** --- common --- */
#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/** --- stack ---*/
#define STACK_INIT_SIZE 10
#define STACK_INCREASE_SIZE 15
typedef struct pm_stack_node_t{
    void* data;
}pm_stack_node_t;
typedef struct pm_stack_t {
    pm_stack_node_t *node;
    unsigned long length;
    unsigned long size;
}pm_stack_t;
pm_stack_t *stack_init();
void *stack_pop(pm_stack_t *stack);
void *stack_top(pm_stack_t *stack);
int stack_empty(pm_stack_t *stack);
int stack_push(pm_stack_t *stack, void *data);
/** --- stack ---*/


/** --- array_list ---*/
//链表初始化大小
#define PM_ARRAY_LIST_INIT_SIZE 10
//链表存满每次增加的大小
#define PM_ARRAY_LIST_INCREASE_SIZE 15

typedef struct pm_array_node_t{
    void* data;//数据域
}pm_array_node_t;

//定义链表结构
typedef struct pm_array_list_t{
    pm_array_node_t *node;
    unsigned long length;
    unsigned long size;
}pm_array_list_t;

pm_array_list_t* pm_array_list_init();
int pm_array_list_insert(pm_array_list_t* p_list, void* p_data, long index);
void* pm_array_list_get(pm_array_list_t* p_list, unsigned long index);
void pm_array_list_remove(pm_array_list_t* p_list, unsigned long index);
void pm_array_list_remove_p(pm_array_list_t *p_list, void *p_data);
void pm_array_list_clear(pm_array_list_t* p_list);
void pm_array_list_free(pm_array_list_t* p_list);
void pm_array_list_init_semaphore(pm_array_list_t* pList);
/** --- array_list ---*/
/** --- common --- */

typedef struct page_base_t *page_base_handler;

/** --- pageBase --- */
/* Page state */
typedef enum {
    PAGE_STATE_IDLE,
    PAGE_STATE_LOAD,
    PAGE_STATE_WILL_APPEAR,
    PAGE_STATE_DID_APPEAR,
    PAGE_STATE_ACTIVITY,
    PAGE_STATE_WILL_DISAPPEAR,
    PAGE_STATE_DID_DISAPPEAR,
    PAGE_STATE_UNLOAD,
    _PAGE_STATE_LAST
} pm_page_state_t;

/* Data area */
typedef struct {
    void *ptr;
    uint32_t size;
} pm_page_stash_t;

/* Page switching animation properties */
typedef struct {
    uint8_t type;
    uint16_t time;
    lv_anim_path_cb_t path;
} pm_page_anim_attr_t;

struct page_base_t {
    lv_obj_t *root;       // UI root node
    const char *name;     // Page name
    uint16_t id;          // Page ID
    void *user_data;       // User data pointer

    /* Private data, Only page manager access */
    struct {
        bool req_enable_cache;        // Cache enable request
        bool req_disable_auto_cache;   // Automatic cache management enable request

        bool is_disable_auto_cache;    // Whether it is automatic cache management
        bool is_cached;              // Cache enable

        pm_page_stash_t stash;              // Stash area
        pm_page_state_t state;              // Page state

        /* Animation state  */
        struct {
            bool is_enter;           // Whether it is the entering party
            bool is_busy;            // Whether the animation is playing
            pm_page_anim_attr_t attr;        // Animation properties
        } anim;
    } priv;

    /* Synchronize user-defined attribute configuration */
    void (*on_custom_attr_config)(page_base_handler pb);

    /* Page load */
    void (*on_view_load)(page_base_handler pb);

    /* Page load complete */
    void (*on_view_did_load)(page_base_handler pb);

    /* Page will be displayed soon  */
    void (*on_view_will_appear)(page_base_handler pb);

    /* The page is displayed  */
    void (*on_view_did_appear)(page_base_handler pb);

    /* Page is about to disappear */
    void (*on_view_will_disappear)(page_base_handler pb);

    /* Page disappeared complete  */
    void (*on_view_did_disappear)(page_base_handler pb);

    /* Page uninstall complete  */
    void (*on_view_did_unload)(page_base_handler pb);
};

/* Set whether to manually manage the cache */
void set_custom_cache_enable(page_base_handler pb, bool en);

/* Set whether to enable automatic cache */
void set_custom_auto_cache_enable(page_base_handler pb, bool en);

/* Set custom animation properties  */
void set_custom_load_anim_type(page_base_handler pb,
                               uint8_t anim_type,
                               uint16_t time,
                               lv_anim_path_cb_t path
);

/* Get the data in the stash area */
bool pm_get_stash(page_base_handler pb, void *ptr, uint32_t size);
/** --- pageBase ---*/

/* Animated setter  */
typedef void(*lv_anim_setter_t)(void *, int32_t);

/* Animated getter  */
typedef int32_t(*lv_anim_getter_t)(void *);

/* Page dragging direction */
typedef enum {
    ROOT_DRAG_DIR_NONE,
    ROOT_DRAG_DIR_HOR,
    ROOT_DRAG_DIR_VER,
} root_drag_dir_t;

/* Animation switching record  */
typedef struct {
    /* As the entered party */
    struct {
        int32_t start;
        int32_t end;
    } enter;

    /* As the exited party */
    struct {
        int32_t start;
        int32_t end;
    } exit;
} anim_value_t;

/* Page switching animation type  */
typedef enum {
    /* Default (global) animation type  */
    LOAD_ANIM_GLOBAL = 0,

    /* New page overwrites old page  */
    LOAD_ANIM_OVER_LEFT,
    LOAD_ANIM_OVER_RIGHT,
    LOAD_ANIM_OVER_TOP,
    LOAD_ANIM_OVER_BOTTOM,

    /* New page pushes old page  */
    LOAD_ANIM_MOVE_LEFT,
    LOAD_ANIM_MOVE_RIGHT,
    LOAD_ANIM_MOVE_TOP,
    LOAD_ANIM_MOVE_BOTTOM,

    /* The new interface fades in, the old page fades out */
    LOAD_ANIM_FADE_ON,

    /* No animation */
    LOAD_ANIM_NONE,

    _LOAD_ANIM_LAST = LOAD_ANIM_NONE
} load_anim_t;

/* Page loading animation properties */
typedef struct {
    lv_anim_setter_t setter;
    lv_anim_getter_t getter;
    root_drag_dir_t drag_dir;
    anim_value_t push;
    anim_value_t pop;
} load_anim_attr_t;

/* Loader */
page_base_handler pm_install(const char *class_name, const char *app_name);

bool pm_uninstall(const char *app_name);

bool pm_register(page_base_handler base, const char *name);

bool pm_unregister(const char *name);

/* Router */
page_base_handler pm_push(const char *name, const pm_page_stash_t *stash);

page_base_handler pm_pop();

bool pm_back_home();

const char *pm_get_page_prev_name();

/* Global Anim */
void pm_set_global_load_anim_type(
        load_anim_t anim,
        uint16_t time,
        lv_anim_path_cb_t path
);

/** --- Factory ---*/
#define APP_CLASS_MATCH(class_name)\
do{\
    if (strcmp(name, #class_name) == 0)\
    {\
        extern page_base_handler ui_page_register_##class_name();\
        return ui_page_register_##class_name();\
    }\
}while(0)

extern page_base_handler pm_create_page(const char *name);

#define UI_PAGE_FUN_INIT() \
do{                        \
    if(pb == NULL){\
        pb = calloc(1, sizeof(struct page_base_t));\
        pb->on_custom_attr_config = on_custom_attr_config;\
        pb->on_view_load = on_view_load;\
        pb->on_view_did_load = on_view_did_load;\
        pb->on_view_will_appear = on_view_will_appear;\
        pb->on_view_did_appear = on_view_did_appear;\
        pb->on_view_will_disappear = on_view_will_disappear;\
        pb->on_view_did_disappear = on_view_did_disappear;\
        pb->on_view_did_unload = on_view_did_unload;\
    }\
}while(0)
/** --- Factory ---*/


/** --- ResourceManager ---*/
typedef struct resource_manager_t *resource_manager_handler;

struct resource_manager_t {
    /** ResourceNode_t */
    pm_array_list_t *node_pool;
    void *default_ptr;
};

bool rm_add_resource(resource_manager_handler rm, const char *name, void *ptr);

bool rm_remove_resource(resource_manager_handler rm, const char *name);

void *rm_get_resource(resource_manager_handler rm, const char *name);

void rm_set_default(resource_manager_handler rm, void *ptr);
/** --- ResourceManager ---*/


/** --- lv_obj_ext_func ---*/
#ifndef __LV_OBJ_EXT_FUNC_H
#define __LV_OBJ_EXT_FUNC_H

#define LV_ANIM_TIME_DEFAULT    400
#define LV_ANIM_EXEC(attr)      (lv_anim_exec_xcb_t)lv_obj_set_##attr

void lv_obj_set_opa_scale(lv_obj_t *obj, int16_t opa);

int16_t lv_obj_get_opa_scale(lv_obj_t *obj);

void lv_label_set_text_add(lv_obj_t *label, const char *text);

void lv_obj_add_anim(
        lv_obj_t *obj, lv_anim_t *a,
        lv_anim_exec_xcb_t exec_cb,
        int32_t start, int32_t end,
        uint16_t time,
        uint32_t delay,
        lv_anim_ready_cb_t ready_cb,
        lv_anim_path_cb_t path_cb
);

void lv_obj_add_anim_def(
        lv_obj_t *obj, lv_anim_t *a,
        lv_anim_exec_xcb_t exec_cb,
        int32_t start, int32_t end

);

#define LV_OBJ_ADD_ANIM(obj, attr, target, time)\
do{\
    lv_obj_add_anim(\
        (obj), NULL,\
        (lv_anim_exec_xcb_t)lv_obj_set_##attr,\
        lv_obj_get_##attr(obj),\
        (target),\
        (time),\
        (0),\
        (NULL),\
        (lv_anim_path_ease_out)\
    );\
}while(0)
#define LV_OBJ_ADD_DELAY_ANIM(obj, attr, target, delay, time)\
do{\
    lv_obj_add_anim(\
        (obj), NULL,\
        (lv_anim_exec_xcb_t)lv_obj_set_##attr,\
        lv_obj_get_##attr(obj),\
        (target),\
        (time),\
        (delay),\
        (NULL),\
        (lv_anim_path_ease_out)\
    );\
}while(0)

lv_indev_t *lv_get_indev(lv_indev_type_t type);

#endif
/** --- lv_obj_ext_func ---*/


/** --- lv_anim_timeline_wrapper ---*/
/*Data of anim_timeline*/
typedef struct {
    uint32_t start_time;
    lv_obj_t *obj;
    lv_anim_exec_xcb_t exec_cb;
    int32_t start;
    int32_t end;
    uint16_t duration;
    lv_anim_path_cb_t path_cb;
    bool early_apply;
} lv_anim_timeline_wrapper_t;

/**********************
* GLOBAL PROTOTYPES
**********************/

/**
 * Start animation according to the timeline
 * @param anim_timeline  timeline array address
 * @param playback       whether to play in reverse
 * @return timeline total time spent
 */
void lv_anim_timeline_add_wrapper(lv_anim_timeline_t *at, const lv_anim_timeline_wrapper_t *wrapper);

/**********************
*      MACROS
**********************/

#define LV_ANIM_TIMELINE_WRAPPER_END {0, NULL}
/** --- lv_anim_timeline_wrapper ---*/


/** --- lv_label_anim_effect ---*/
typedef struct {
    lv_obj_t *label_1;
    lv_obj_t *label_2;
    lv_anim_t anim_now;
    lv_anim_t anim_next;
    lv_coord_t y_offset;
    uint8_t value_last;
} lv_label_anim_effect_t;

void lv_label_anim_effect_init(
        lv_label_anim_effect_t *effect,
        lv_obj_t *cont,
        lv_obj_t *label_copy,
        uint16_t anim_time
);

void lv_label_anim_effect_check_value(
        lv_label_anim_effect_t *effect,
        uint8_t value,
        lv_anim_enable_t anim_enable
);

/** --- lv_label_anim_effect ---*/

/** --- data_center --- */
typedef struct {
    const char *topic;
    void *data;
    uint32_t length;
    uint16_t msg_type;
    bool sync;
    void *sync_result;
}dc_data_msg_t;

typedef struct {
    const char *topic;
    pm_array_list_t *subscribe_list;
} dc_topic_subscribe_t;

typedef void (*dc_callback)(dc_data_msg_t *data_msg);

/**
 * 订阅
 * @param topic 主题
 * @param c 消息处理回调
 */
void dc_subscribe(const char *topic,dc_callback c);
/**
 * 取消订阅
 * @param topic 主题
 * @param c 消息处理回调
 */
void dc_unsubscribe(const char *topic,dc_callback c);
/**
 * 推送消息到主题（异步）
 * @param data_msg 消息
 */
void dc_publish(dc_data_msg_t *data_msg);
/**
 * 同步请求
 * @param data_msg 消息
 */
void dc_sync_req(dc_data_msg_t *data_msg);
/**
 * 创建消息
 * @param topic 主题
 * @return 消息体
 */
dc_data_msg_t *dc_create_msg(const char *topic);
/**
 * 数据中心loop（主线程处理，需与UI在一个线程中）
 */
void data_center();
/** --- data_center --- */


void ui_page_manager_init();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //UI_PAGE_MANAGER_H
