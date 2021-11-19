#include <stdio.h>
#include "ui_page_manager.h"
#include <stdlib.h>

/** 是否开启日志 */
#define PAGE_MANAGER_USE_LOG 1

/** --- LOG --- */
#if PAGE_MANAGER_USE_LOG
#  define _PM_LOG(format, ...)      printf("[PM]"format"\r\n", ##__VA_ARGS__)
#  define PM_LOG_INFO(format, ...)  _PM_LOG("[Info] "format, ##__VA_ARGS__)
#  define PM_LOG_WARN(format, ...)  _PM_LOG("[Warn] "format, ##__VA_ARGS__)
#  define PM_LOG_ERROR(format, ...) _PM_LOG("[Error] "format, ##__VA_ARGS__)
#else
#  define PM_LOG_INFO(...)
#  define PM_LOG_WARN(...)
#  define PM_LOG_ERROR(...)
#endif
/** --- LOG --- */

/** --- PageManager private --- */
/* Page animation status */
struct {
    bool is_switch_req;              // Whether to switch request
    bool is_busy;                   // Is switching
    bool is_pushing;                // Whether it is in push state

    pm_page_anim_attr_t current;  // Current animation properties
    pm_page_anim_attr_t global;   // Global animation properties
} anim_state;

/** 私有变量定义 */
/* Page pool<page_base_handler> */
static pm_array_list_t *page_pool = NULL;

/* Page stack<PageBase> */
static pm_stack_t *page_stack = NULL;

/* Previous page */
static page_base_handler page_prev = NULL;

/* The current page */
static page_base_handler page_current = NULL;

/* Page Pool */
page_base_handler find_page_in_pool(const char *name);

/* Page Stack */
page_base_handler find_page_in_stack(const char *name);

page_base_handler get_stack_top();

page_base_handler get_stack_top_after();

void set_stack_clear(bool keep_bottom);

bool fource_unload(page_base_handler base);

/* Anim */
bool get_load_anim_attr(uint8_t anim, load_anim_attr_t *attr);

bool get_is_over_anim(uint8_t anim) {
    return (anim >= LOAD_ANIM_OVER_LEFT && anim <= LOAD_ANIM_OVER_BOTTOM);
}

bool get_is_move_anim(uint8_t anim) {
    return (anim >= LOAD_ANIM_MOVE_LEFT && anim <= LOAD_ANIM_MOVE_BOTTOM);
}

void anim_default_init(lv_anim_t *a);

load_anim_t get_current_load_anim_type() {
    return (load_anim_t) anim_state.current.type;
}

bool get_current_load_anim_attr(load_anim_attr_t *attr) {
    return get_load_anim_attr(get_current_load_anim_type(), attr);
}

/* Root */
static void on_root_drag_event(lv_event_t *event);

static void on_root_anim_finish(lv_anim_t *a);

static void on_root_async_leave(void *data);

void root_enable_drag(lv_obj_t *root);

static void root_get_drag_predict(lv_coord_t *x, lv_coord_t *y);

/* Switch */
void switch_to(page_base_handler new_node, bool is_push_act, const pm_page_stash_t *stash);

static void on_switch_anim_finish(lv_anim_t *a);

void switch_anim_create(page_base_handler base);

void switch_anim_type_update(page_base_handler base);

bool switch_req_check();

bool switch_anim_state_check();

/* State */
pm_page_state_t state_load_execute(page_base_handler base);

pm_page_state_t state_will_appear_execute(page_base_handler base);

pm_page_state_t state_did_appear_execute(page_base_handler base);

pm_page_state_t state_will_disappear_execute(page_base_handler base);

pm_page_state_t state_did_disappear_execute(page_base_handler base);

pm_page_state_t state_unload_execute(page_base_handler base);

void state_update(page_base_handler base);

pm_page_state_t state_get_state() {
    return page_current->priv.state;
}
/** --- PageManager private --- */


/** --- PageBase ---*/
/* Set whether to enable automatic cache */
void set_custom_auto_cache_enable(page_base_handler pb, bool en) {
    pb->priv.req_disable_auto_cache = !en;
}

/* Set whether to manually manage the cache */
void set_custom_cache_enable(page_base_handler pb, bool en) {
    set_custom_auto_cache_enable(pb, false);
    pb->priv.req_enable_cache = en;
}

/* Set custom animation properties  */
void set_custom_load_anim_type(page_base_handler pb,
                               uint8_t anim_type,
                               uint16_t time,
                               lv_anim_path_cb_t path
) {
    pb->priv.anim.attr.type = anim_type;
    pb->priv.anim.attr.time = time;
    pb->priv.anim.attr.path = path;
}

/* Get the data in the stash area */
bool pm_get_stash(page_base_handler pb, void *ptr, uint32_t size) {
    bool retval = false;
    if (pb->priv.stash.ptr != NULL && pb->priv.stash.size == size) {
        memcpy(ptr, pb->priv.stash.ptr, pb->priv.stash.size);
        //lv_mem_free(pb->priv.Stash.ptr);
        //pb->priv.Stash.ptr = NULL;
        retval = true;
    }
    return retval;
}
/** --- PageBase ---*/

/** --- PM_Base --- */
#define PM_EMPTY_PAGE_NAME "EMPTY_PAGE"

/**
  * @brief  Search pages in the page pool
  * @param  name: Page name
  * @retval A pointer to the base class of the page, or NULL if not found
  */
page_base_handler find_page_in_pool(const char *name) {
    for (int i = 0; i < page_pool->length; i++) {
        page_base_handler iter = pm_array_list_get(page_pool, i);
        if (strcmp(name, iter->name) == 0) {
            return iter;
        }
    }
    return NULL;
}

/**
  * @brief  Search pages in the page stack
  * @param  name: Page name
  * @retval A pointer to the base class of the page, or NULL if not found
  */
page_base_handler find_page_in_stack(const char *name) {
    for (int i = 0; i < page_stack->length; ++i) {
        page_base_handler base = stack_get(page_stack, i);
        if (strcmp(name, base->name) == 0) {
            return base;
        }
    }
    return NULL;
}

/**
  * @brief  Install the page, and register the page to the page pool
  * @param  class_name: The class name of the page
  * @param  app_name: Page application name, no duplicates allowed
  * @retval A pointer to the base class of the page, or NULL if wrong
  */
page_base_handler pm_install(const char *class_name, const char *app_name) {
    page_base_handler base = pm_create_page(class_name);
    if (base == NULL) {
        PM_LOG_ERROR("Factory has not %s", class_name);
        return NULL;
    }

    base->root = NULL;
    base->id = 0;
    base->user_data = NULL;
    memset(&base->priv, 0, sizeof(base->priv));

    base->on_custom_attr_config(base);

    if (app_name == NULL) {
        PM_LOG_WARN("appName has not set");
        app_name = class_name;
    }

    PM_LOG_INFO("Install Page[class = %s, name = %s]", class_name, app_name);
    pm_register(base, app_name);

    return base;
}

/**
  * @brief  Uninstall page
  * @param  app_name: Page application name, no duplicates allowed
  * @retval Return true if the uninstallation is successful
  */
bool pm_uninstall(const char *app_name) {
    PM_LOG_INFO("Page(%s) uninstall...", app_name);

    page_base_handler base = find_page_in_pool(app_name);
    if (base == NULL) {
        PM_LOG_ERROR("Page(%s) was not found", app_name);
        return false;
    }

    if (!pm_unregister(app_name)) {
        PM_LOG_ERROR("Page(%s) unregister failed", app_name);
        return false;
    }

    if (base->priv.is_cached) {
        PM_LOG_WARN("Page(%s) has cached, unloading...", app_name);
        base->priv.state = PAGE_STATE_UNLOAD;
        state_update(base);
    } else {
        PM_LOG_INFO("Page(%s) has not cache", app_name);
    }

    free(base);
    base = NULL;

    PM_LOG_INFO("Uninstall OK");
    return true;
}

/**
  * @brief  Register the page to the page pool
  * @param  name: Page application name, duplicate registration is not allowed
  * @retval Return true if the registration is successful
  */
bool pm_register(page_base_handler base, const char *name) {
    if (find_page_in_pool(name) != NULL) {
        PM_LOG_ERROR("Page(%s) was multi registered", name);
        return false;
    }

    base->name = name;

    pm_array_list_insert(page_pool, base, -1);

    return true;
}

/**
  * @brief  Log out the page from the page pool
  * @param  name: Page application name
  * @retval Return true if the logout is successful
  */
bool pm_unregister(const char *name) {
    PM_LOG_INFO("Page(%s) unregister...", name);

    page_base_handler base = find_page_in_stack(name);

    if (base != NULL) {
        PM_LOG_ERROR("Page(%s) was in stack", name);
        return false;
    }

    base = find_page_in_pool(name);
    if (base == NULL) {
        PM_LOG_ERROR("Page(%s) was not found", name);
        return false;
    }

    page_base_handler iter = NULL;
    for (int i = 0; i < page_pool->length; ++i) {
        page_base_handler item = pm_array_list_get(page_pool, i);
        if (item == base) {
            iter = item;
            break;
        }
    }

    if (iter == NULL) {
        PM_LOG_ERROR("Page(%s) was not found in PagePool", name);
        return false;
    }

    pm_array_list_remove_p(page_pool, iter);

    PM_LOG_INFO("Unregister OK");
    return true;
}

/**
  * @brief  Get the top page of the page stack
  * @param  None
  * @retval A pointer to the base class of the page
  */
page_base_handler get_stack_top() {
    return stack_empty(page_stack) ? NULL : stack_top(page_stack);
}

/**
  * @brief  Get the page below the top of the page stack
  * @param  None
  * @retval A pointer to the base class of the page
  */
page_base_handler get_stack_top_after() {
    page_base_handler top = get_stack_top();

    if (top == NULL) {
        return NULL;
    }

    stack_pop(page_stack);

    page_base_handler top_after = get_stack_top();

    stack_push(page_stack, top);

    return top_after;
}

/**
  * @brief  Clear the page stack and end the life cycle of all pages in the page stack
  * @param  keep_bottom: Whether to keep the bottom page of the stack
  * @retval None
  */
void set_stack_clear(bool keep_bottom) {
    while (1) {
        page_base_handler top = get_stack_top();

        if (top == NULL) {
            PM_LOG_INFO("Page stack is empty, breaking...");
            break;
        }

        page_base_handler top_after = get_stack_top_after();

        if (top_after == NULL) {
            if (keep_bottom) {
                page_prev = top;
                PM_LOG_INFO("Keep page stack bottom(%s), breaking...", top->name);
                break;
            } else {
                page_prev = NULL;
            }
        }

        fource_unload(top);

        stack_pop(page_stack);
    }
    PM_LOG_INFO("Stack clear done");
}

/**
  * @brief  Get the name of the previous page
  * @param  None
  * @retval The name of the previous page, if it does not exist, return PM_EMPTY_PAGE_NAME
  */
const char *pm_get_page_prev_name() {
    return page_prev ? page_prev->name : PM_EMPTY_PAGE_NAME;
}
/** --- PM_Base --- */


/** --- PM_Anim --- */
void dir_hor_anim_setter(void *obj, int32_t v) {
    lv_obj_set_x((lv_obj_t *) obj, v);
}

int32_t dir_hor_anim_getter(void *obj) {
    return (int32_t) lv_obj_get_x((lv_obj_t *) obj);
}

void dir_ver_anim_setter(void *obj, int32_t v) {
    lv_obj_set_y((lv_obj_t *) obj, v);
}

int32_t dir_ver_anim_getter(void *obj) {
    return (int32_t) lv_obj_get_y((lv_obj_t *) obj);
}

void opa_anim_setter(void *obj, int32_t v) {
    lv_obj_set_style_bg_opa((lv_obj_t *) obj, (lv_opa_t) v, LV_PART_MAIN);
}

int32_t opa_anim_getter(void *obj) {
    return (int32_t) lv_obj_get_style_bg_opa((lv_obj_t *) obj, LV_PART_MAIN);
}

/**
  * @brief  Get page loading animation properties
  * @param  anim: Animation type
  * @param  attr: Pointer to attribute
  * @retval Whether the acquisition is successful
  */
bool get_load_anim_attr(uint8_t anim, load_anim_attr_t *attr) {
    lv_coord_t hor = LV_HOR_RES;
    lv_coord_t ver = LV_VER_RES;

    switch (anim) {
        case LOAD_ANIM_OVER_LEFT:
            attr->drag_dir = ROOT_DRAG_DIR_HOR;

            attr->push.enter.start = hor;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = 0;

            attr->pop.enter.start = 0;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = hor;
            break;

        case LOAD_ANIM_OVER_RIGHT:
            attr->drag_dir = ROOT_DRAG_DIR_HOR;

            attr->push.enter.start = -hor;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = 0;

            attr->pop.enter.start = 0;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = -hor;
            break;

        case LOAD_ANIM_OVER_TOP:
            attr->drag_dir = ROOT_DRAG_DIR_VER;

            attr->push.enter.start = ver;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = 0;

            attr->pop.enter.start = 0;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = ver;
            break;

        case LOAD_ANIM_OVER_BOTTOM:
            attr->drag_dir = ROOT_DRAG_DIR_VER;

            attr->push.enter.start = -ver;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = 0;

            attr->pop.enter.start = 0;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = -ver;
            break;

        case LOAD_ANIM_MOVE_LEFT:
            attr->drag_dir = ROOT_DRAG_DIR_HOR;

            attr->push.enter.start = hor;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = -hor;

            attr->pop.enter.start = -hor;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = hor;
            break;

        case LOAD_ANIM_MOVE_RIGHT:
            attr->drag_dir = ROOT_DRAG_DIR_HOR;

            attr->push.enter.start = -hor;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = hor;

            attr->pop.enter.start = hor;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = -hor;
            break;

        case LOAD_ANIM_MOVE_TOP:
            attr->drag_dir = ROOT_DRAG_DIR_VER;

            attr->push.enter.start = ver;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = -ver;

            attr->pop.enter.start = -ver;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = ver;
            break;

        case LOAD_ANIM_MOVE_BOTTOM:
            attr->drag_dir = ROOT_DRAG_DIR_VER;

            attr->push.enter.start = -ver;
            attr->push.enter.end = 0;
            attr->push.exit.start = 0;
            attr->push.exit.end = ver;

            attr->pop.enter.start = ver;
            attr->pop.enter.end = 0;
            attr->pop.exit.start = 0;
            attr->pop.exit.end = -ver;
            break;

        case LOAD_ANIM_FADE_ON:
            attr->drag_dir = ROOT_DRAG_DIR_NONE;

            attr->push.enter.start = LV_OPA_TRANSP;
            attr->push.enter.end = LV_OPA_COVER;
            attr->push.exit.start = LV_OPA_COVER;
            attr->push.exit.end = LV_OPA_COVER;

            attr->pop.enter.start = LV_OPA_COVER;
            attr->pop.enter.end = LV_OPA_COVER;
            attr->pop.exit.start = LV_OPA_COVER;
            attr->pop.exit.end = LV_OPA_TRANSP;
            break;

        case LOAD_ANIM_NONE:
            memset(attr, 0, sizeof(load_anim_attr_t));
            return true;

        default:
            PM_LOG_ERROR("Load anim type error: %d", anim);
            return false;
    }

    /* Determine the setter and getter of the animation */
    if (attr->drag_dir == ROOT_DRAG_DIR_HOR) {
        attr->setter = dir_hor_anim_setter;
        attr->getter = dir_hor_anim_getter;
    } else if (attr->drag_dir == ROOT_DRAG_DIR_VER) {
        attr->setter = dir_ver_anim_setter;
        attr->getter = dir_ver_anim_getter;
    } else {
        attr->setter = opa_anim_setter;
        attr->getter = opa_anim_getter;
    }

    return true;
}
/** --- PM_Anim --- */


/** --- PM_Drag --- */
#define CONSTRAIN(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

/* The distance threshold to trigger the drag */
#define PM_INDEV_DEF_DRAG_THROW    20

/**
  * @brief  Page drag event callback
  * @param  event: Pointer to event structure
  * @retval None
  */
static void on_root_drag_event(lv_event_t *event) {
    lv_obj_t *root = lv_event_get_target(event);
    page_base_handler base = (page_base_handler) lv_obj_get_user_data(root);

    if (base == NULL) {
        PM_LOG_ERROR("Page base is NULL");
        return;
    }

    lv_event_code_t event_code = lv_event_get_code(event);
    load_anim_attr_t anim_attr;

    if (!get_current_load_anim_attr(&anim_attr)) {
        PM_LOG_ERROR("Can't get current anim attr");
        return;
    }

    if (event_code == LV_EVENT_PRESSED) {
        if (anim_state.is_switch_req)
            return;

        if (!anim_state.is_busy)
            return;

        PM_LOG_INFO("Root anim interrupted");
        lv_anim_del(root, anim_attr.setter);
        anim_state.is_busy = false;
    } else if (event_code == LV_EVENT_PRESSING) {
        lv_coord_t cur = anim_attr.getter(root);

        lv_coord_t max =
                anim_attr.pop.exit.start > anim_attr.pop.exit.end ? anim_attr.pop.exit.start : anim_attr.pop.exit.end;
        lv_coord_t min =
                anim_attr.pop.exit.start < anim_attr.pop.exit.end ? anim_attr.pop.exit.start : anim_attr.pop.exit.end;

        lv_point_t offset;
        lv_indev_get_vect(lv_indev_get_act(), &offset);

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR) {
            cur += offset.x;
        } else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER) {
            cur += offset.y;
        }

        anim_attr.setter(root, CONSTRAIN(cur, min, max));
    } else if (event_code == LV_EVENT_RELEASED) {
        if (anim_state.is_switch_req) {
            return;
        }

        lv_coord_t offset_sum = anim_attr.push.enter.end - anim_attr.push.enter.start;

        lv_coord_t x_predict = 0;
        lv_coord_t y_predict = 0;
        root_get_drag_predict(&x_predict, &y_predict);

        lv_coord_t start = anim_attr.getter(root);
        lv_coord_t end = start;

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR) {
            end += x_predict;
            PM_LOG_INFO("Root drag x_predict = %d", end);
        } else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER) {
            end += y_predict;
            PM_LOG_INFO("Root drag y_predict = %d", end);
        }

        if (abs(end) > abs((int) offset_sum) / 2) {
            lv_async_call(on_root_async_leave, base);
        } else if (end != anim_attr.push.enter.end) {
            anim_state.is_busy = true;

            lv_anim_t a;
            anim_default_init(&a);
            lv_anim_set_var(&a, root);
            lv_anim_set_values(&a, start, anim_attr.push.enter.end);
            lv_anim_set_exec_cb(&a, anim_attr.setter);
            lv_anim_set_ready_cb(&a, on_root_anim_finish);
            lv_anim_start(&a);
            PM_LOG_INFO("Root anim start");
        }
    }
}

/**
  * @brief  Drag animation end event callback
  * @param  a: Pointer to animation
  * @retval None
  */
static void on_root_anim_finish(lv_anim_t *a) {
    PM_LOG_INFO("Root anim finish");
    anim_state.is_busy = false;
}

/**
  * @brief  Enable root's drag function
  * @param  root: Pointer to the root object
  * @retval None
  */
void root_enable_drag(lv_obj_t *root) {
    lv_obj_add_event_cb(
            root,
            on_root_drag_event,
            LV_EVENT_PRESSED,
            NULL
    );
    lv_obj_add_event_cb(
            root,
            on_root_drag_event,
            LV_EVENT_PRESSING,
            NULL
    );
    lv_obj_add_event_cb(
            root,
            on_root_drag_event,
            LV_EVENT_RELEASED,
            NULL
    );
    PM_LOG_INFO("Root drag enabled");
}

/**
  * @brief  Asynchronous callback when dragging ends
  * @param  data: Pointer to the base class of the page
  * @retval None
  */
static void on_root_async_leave(void *data) {
    page_base_handler base = (page_base_handler) data;
    PM_LOG_INFO("Page(%s) send event: LV_EVENT_LEAVE, need to handle...", base->name);
    lv_event_send(base->root, LV_EVENT_LEAVE, NULL);
}

/**
  * @brief  Get drag inertia prediction stop point
  * @param  x: x stop point
  * @param  y: y stop point
  * @retval None
  */
static void root_get_drag_predict(lv_coord_t *x, lv_coord_t *y) {
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t y_predict = 0;
    lv_coord_t x_predict = 0;

    while (vect.y != 0) {
        y_predict += vect.y;
        vect.y = vect.y * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    while (vect.x != 0) {
        x_predict += vect.x;
        vect.x = vect.x * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    *x = x_predict;
    *y = y_predict;
}

/** --- PM_Drag --- */


/** --- PM_Router --- */
/**
  * @brief  Enter a new page, the old page is pushed onto the stack
  * @param  name: The name of the page to enter 
  * @param  stash: Parameters passed to the new page
  * @retval Pointer to the page pushed onto the stack 
  */
page_base_handler pm_push(const char *name, const pm_page_stash_t *stash) {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return NULL;
    }

    /* Check whether the stack is repeatedly pushed  */
    if (find_page_in_stack(name) != NULL) {
        PM_LOG_ERROR("Page(%s) was multi push", name);
        return NULL;
    }

    /* Check if the page is registered in the page pool */
    page_base_handler base = find_page_in_pool(name);

    if (base == NULL) {
        PM_LOG_ERROR("Page(%s) was not install", name);
        return NULL;
    }

    /* Synchronous automatic cache configuration */
    base->priv.is_disable_auto_cache = base->priv.req_disable_auto_cache;

    /* Push into the stack */
    stack_push(page_stack, base);

    PM_LOG_INFO("Page(%s) push >> [Screen] (stash = 0x%p)", name, stash);

    /* Page switching execution */
    switch_to(base, true, stash);

    return base;
}

/**
  * @brief  Pop the current page
  * @param  None
  * @retval Pointer to the next page 
  */
page_base_handler pm_pop() {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return NULL;
    }

    /* Get the top page of the stack */
    page_base_handler top = get_stack_top();

    if (top == NULL) {
        PM_LOG_WARN("Page stack is empty, cat't pop");
        return NULL;
    }

    /* Whether to turn off automatic cache */
    if (!top->priv.is_disable_auto_cache) {
        PM_LOG_INFO("Page(%s) has auto cache, cache disabled", top->name);
        top->priv.is_cached = false;
    }

    PM_LOG_INFO("Page(%s) pop << [Screen]", top->name);

    /* Page popup */
    stack_pop(page_stack);

    /* Get the next page */
    top = get_stack_top();

    if (top != NULL) {
        /* Page switching execution */
        switch_to(top, false, NULL);
    }

    return top;
}

/**
  * @brief  Page switching
  * @param  new_node: Pointer to new page
  * @param  is_push_act: Whether it is a push action
  * @param  stash: Parameters passed to the new page
  * @retval None
  */
void switch_to(page_base_handler new_node, bool is_push_act, const pm_page_stash_t *stash) {
    if (new_node == NULL) {
        PM_LOG_ERROR("newNode is NULL");
        return;
    }

    /* Whether page switching has been requested */
    if (anim_state.is_switch_req) {
        PM_LOG_WARN("Page switch busy, reqire(%s) is ignore", new_node->name);
        return;
    }

    anim_state.is_switch_req = true;

    /* Is there a parameter to pass */
    if (stash != NULL) {
        PM_LOG_INFO("stash is detect, %s >> stash(0x%p) >> %s", pm_get_page_prev_name(), stash, new_node->name);

        void *buffer = NULL;

        if (new_node->priv.stash.ptr == NULL) {
            buffer = lv_mem_alloc(stash->size);
            if (buffer == NULL) {
                PM_LOG_ERROR("stash malloc failed");
            } else {
                PM_LOG_INFO("stash(0x%p) malloc[%d]", buffer, stash->size);
            }
        } else if (new_node->priv.stash.size == stash->size) {
            buffer = new_node->priv.stash.ptr;
            PM_LOG_INFO("stash(0x%p) is exist", buffer);
        }

        if (buffer != NULL) {
            memcpy(buffer, stash->ptr, stash->size);
            PM_LOG_INFO("stash memcpy[%d] 0x%p >> 0x%p", stash->size, stash->ptr, buffer);
            new_node->priv.stash.ptr = buffer;
            new_node->priv.stash.size = stash->size;
        }
    }

    /* Record current page */
    page_current = new_node;

    /* If the current page has a cache */
    if (page_current->priv.is_cached) {
        /* Direct display, no need to load */
        PM_LOG_INFO("Page(%s) has cached, appear driectly", page_current->name);
        page_current->priv.state = PAGE_STATE_WILL_APPEAR;
    } else {
        /* Load page */
        page_current->priv.state = PAGE_STATE_LOAD;
    }

    if (page_prev != NULL) {
        page_prev->priv.anim.is_enter = false;
    }

    page_current->priv.anim.is_enter = true;

    anim_state.is_pushing = is_push_act;

    if (anim_state.is_pushing) {
        /* Update the animation configuration according to the current page */
        switch_anim_type_update(page_current);
    }

    /* Update the state machine of the previous page */
    state_update(page_prev);

    /* Update the state machine of the current page */
    state_update(page_current);

    /* Move the layer, move the new page to the front */
    if (anim_state.is_pushing) {
        PM_LOG_INFO("Page PUSH is detect, move Page(%s) to foreground", page_current->name);
        if (page_prev)lv_obj_move_foreground(page_prev->root);
        lv_obj_move_foreground(page_current->root);
    } else {
        PM_LOG_INFO("Page POP is detect, move Page(%s) to foreground", pm_get_page_prev_name());
        lv_obj_move_foreground(page_current->root);
        if (page_prev)lv_obj_move_foreground(page_prev->root);
    }
}

/**
  * @brief  Force the end of the life cycle of the page without animation 
  * @param  base: Pointer to the page being executed
  * @retval Return true if successful
  */
bool fource_unload(page_base_handler base) {
    if (base == NULL) {
        PM_LOG_ERROR("Page is NULL, Unload failed");
        return false;
    }

    PM_LOG_INFO("Page(%s) Fource unloading...", base->name);

    if (base->priv.state == PAGE_STATE_ACTIVITY) {
        PM_LOG_INFO("Page state is ACTIVITY, Disappearing...");
        base->on_view_will_disappear(base);
        base->on_view_did_disappear(base);
    }

    base->priv.state = state_unload_execute(base);

    return true;
}

/**
  * @brief  Back to the main page (the page at the bottom of the stack) 
  * @param  None
  * @retval Return true if successful
  */
bool pm_back_home() {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return false;
    }

    set_stack_clear(true);

    page_prev = NULL;

    page_base_handler home = get_stack_top();

    switch_to(home, false, NULL);

    return true;
}

/**
  * @brief  Check if the page switching animation is being executed
  * @param  None
  * @retval Return true if it is executing
  */
bool switch_anim_state_check() {
    if (anim_state.is_switch_req || anim_state.is_busy) {
        PM_LOG_WARN(
                "Page switch busy[AnimState.IsSwitchReq = %d,"
                "AnimState.IsBusy = %d],"
                "request ignored",
                anim_state.is_switch_req,
                anim_state.is_busy
        );
        return false;
    }

    return true;
}

/**
  * @brief  Page switching request check 
  * @param  None
  * @retval Return true if all pages are executed
  */
bool switch_req_check() {
    bool ret = false;
    bool last_node_busy = page_prev && page_prev->priv.anim.is_busy;

    if (!page_current->priv.anim.is_busy && !last_node_busy) {
        PM_LOG_INFO("----Page switch was all finished----");
        anim_state.is_switch_req = false;
        ret = true;
        page_prev = page_current;
    } else {
        if (page_current->priv.anim.is_busy) {
            PM_LOG_WARN("Page PageCurrent(%s) is busy", page_current->name);
        } else {
            PM_LOG_WARN("Page PagePrev(%s) is busy", pm_get_page_prev_name());
        }
    }

    return ret;
}

/**
  * @brief  PPage switching animation execution end callback 
  * @param  a: Pointer to animation
  * @retval None
  */
static void on_switch_anim_finish(lv_anim_t *a) {
    page_base_handler base = (page_base_handler) a->user_data;

    PM_LOG_INFO("Page(%s) Anim finish", base->name);

    state_update(base);
    base->priv.anim.is_busy = false;
    bool is_finished = switch_req_check();

    if (!anim_state.is_pushing && is_finished) {
        switch_anim_type_update(page_current);
    }
}

/**
  * @brief  Create page switching animation
  * @param  a: Point to the animated page
  * @retval None
  */
void switch_anim_create(page_base_handler base) {
    load_anim_attr_t anim_attr;
    if (!get_current_load_anim_attr(&anim_attr)) {
        return;
    }

    lv_anim_t a;
    anim_default_init(&a);

    a.user_data = base;
    lv_anim_set_var(&a, base->root);
    lv_anim_set_ready_cb(&a, on_switch_anim_finish);
    lv_anim_set_exec_cb(&a, anim_attr.setter);

    int32_t start = 0;

    if (anim_attr.getter) {
        start = anim_attr.getter(base->root);
    }

    if (anim_state.is_pushing) {
        if (base->priv.anim.is_enter) {
            lv_anim_set_values(
                    &a,
                    anim_attr.push.enter.start,
                    anim_attr.push.enter.end
            );
        } else /* Exit */
        {
            lv_anim_set_values(
                    &a,
                    start,
                    anim_attr.push.exit.end
            );
        }
    } else /* Pop */
    {
        if (base->priv.anim.is_enter) {
            lv_anim_set_values(
                    &a,
                    anim_attr.pop.enter.start,
                    anim_attr.pop.enter.end
            );
        } else /* Exit */
        {
            lv_anim_set_values(
                    &a,
                    start,
                    anim_attr.pop.exit.end
            );
        }
    }

    lv_anim_start(&a);
    base->priv.anim.is_busy = true;
}

/**
  * @brief  Set global animation properties 
  * @param  anim: Animation type
  * @param  time: Animation duration
  * @param  path: Animation curve
  * @retval None
  */
void pm_set_global_load_anim_type(load_anim_t anim, uint16_t time, lv_anim_path_cb_t path) {
    if (anim > _LOAD_ANIM_LAST) {
        anim = LOAD_ANIM_NONE;
    }

    anim_state.global.type = anim;
    anim_state.global.time = time;
    anim_state.global.path = path;

    PM_LOG_INFO("Set global load anim type = %d", anim);
}

/**
  * @brief  Update current animation properties, apply page custom animation
  * @param  base: Pointer to page
  * @retval None
  */
void switch_anim_type_update(page_base_handler base) {
    if (base->priv.anim.attr.type == LOAD_ANIM_GLOBAL) {
        PM_LOG_INFO(
                "Page(%s) Anim.type was not set, use AnimState.global.type = %d",
                base->name,
                anim_state.global.type
        );
        anim_state.current = anim_state.global;
    } else {
        if (base->priv.anim.attr.type > _LOAD_ANIM_LAST) {
            PM_LOG_ERROR(
                    "Page(%s) ERROR custom Anim.type = %d, use AnimState.global.type = %d",
                    base->name,
                    base->priv.anim.attr.type,
                    anim_state.global.type
            );
            base->priv.anim.attr = anim_state.global;
        } else {
            PM_LOG_INFO(
                    "Page(%s) custom Anim.type set = %d",
                    base->name,
                    base->priv.anim.attr.type
            );
        }
        anim_state.current = base->priv.anim.attr;
    }
}

/**
  * @brief  Set animation default parameters
  * @param  a: Pointer to animation
  * @retval None
  */
void anim_default_init(lv_anim_t *a) {
    lv_anim_init(a);

    uint32_t time = (get_current_load_anim_type() == LOAD_ANIM_NONE) ? 0 : anim_state.current.time;
    lv_anim_set_time(a, time);
    lv_anim_set_path_cb(a, anim_state.current.path);
}
/** --- PM_Router --- */


/** --- PM_State --- */
/**
  * @brief  Update page state machine
  * @param  base: Pointer to the updated page
  * @retval None
  */
void state_update(page_base_handler base) {
    if (base == NULL)
        return;

    switch (base->priv.state) {
        case PAGE_STATE_IDLE:
            PM_LOG_INFO("Page(%s) state idle", base->name);
            break;

        case PAGE_STATE_LOAD:
            base->priv.state = state_load_execute(base);
            state_update(base);
            break;

        case PAGE_STATE_WILL_APPEAR:
            base->priv.state = state_will_appear_execute(base);
            break;

        case PAGE_STATE_DID_APPEAR:
            base->priv.state = state_did_appear_execute(base);
            PM_LOG_INFO("Page(%s) state active", base->name);
            break;

        case PAGE_STATE_ACTIVITY:
            PM_LOG_INFO("Page(%s) state active break", base->name);
            base->priv.state = PAGE_STATE_WILL_DISAPPEAR;
            state_update(base);
            break;

        case PAGE_STATE_WILL_DISAPPEAR:
            base->priv.state = state_will_disappear_execute(base);
            break;

        case PAGE_STATE_DID_DISAPPEAR:
            base->priv.state = state_did_disappear_execute(base);
            if (base->priv.state == PAGE_STATE_UNLOAD) {
                state_update(base);
            }
            break;

        case PAGE_STATE_UNLOAD:
            base->priv.state = state_unload_execute(base);
            break;

        default:
            PM_LOG_ERROR("Page(%s) state[%d] was NOT FOUND!", base->name, base->priv.state);
            break;
    }
}

/**
  * @brief  Page loading status
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_load_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state load", base->name);

    if (base->root != NULL) {
        PM_LOG_ERROR("Page(%s) root must be NULL", base->name);
    }

    lv_obj_t *root_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(root_obj, LV_OBJ_FLAG_SCROLLABLE);
    root_obj->user_data = base;
    base->root = root_obj;
    base->on_view_load(base);

    if (get_is_over_anim(get_current_load_anim_type())) {
        page_base_handler bottom_page = get_stack_top_after();

        if (bottom_page != NULL && bottom_page->priv.is_cached) {
            load_anim_attr_t anim_attr;
            if (get_current_load_anim_attr(&anim_attr)) {
                if (anim_attr.drag_dir != ROOT_DRAG_DIR_NONE) {
                    root_enable_drag(base->root);
                }
            }
        }
    }

    base->on_view_did_load(base);

    if (base->priv.is_disable_auto_cache) {
        PM_LOG_INFO("Page(%s) disable auto cache, ReqEnableCache = %d", base->name, base->priv.req_enable_cache);
        base->priv.is_cached = base->priv.req_enable_cache;
    } else {
        PM_LOG_INFO("Page(%s) AUTO cached", base->name);
        base->priv.is_cached = true;
    }

    return PAGE_STATE_WILL_APPEAR;
}

/**
  * @brief  The page is about to show the status
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_will_appear_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state will appear", base->name);
    base->on_view_will_appear(base);
    lv_obj_clear_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    switch_anim_create(base);
    return PAGE_STATE_DID_APPEAR;
}

/**
  * @brief  The status of the page display
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_did_appear_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state did appear", base->name);
    base->on_view_did_appear(base);
    return PAGE_STATE_ACTIVITY;
}

/**
  * @brief  The page is about to disappear
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_will_disappear_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state will disappear", base->name);
    base->on_view_will_disappear(base);
    switch_anim_create(base);
    return PAGE_STATE_DID_DISAPPEAR;
}

/**
  * @brief  Page disappeared end state
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_did_disappear_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state did disappear", base->name);
    if (get_current_load_anim_type() == LOAD_ANIM_FADE_ON) {
        PM_LOG_INFO("AnimState.typeCurrent == LOAD_ANIM_FADE_ON, Page(%s) hidden", base->name);
        lv_obj_add_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    }
    base->on_view_did_disappear(base);
    if (base->priv.is_cached) {
        PM_LOG_INFO("Page(%s) has cached", base->name);
        return PAGE_STATE_WILL_APPEAR;
    } else {
        return PAGE_STATE_UNLOAD;
    }
}

/**
  * @brief  Page unload complete
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
pm_page_state_t state_unload_execute(page_base_handler base) {
    PM_LOG_INFO("Page(%s) state unload", base->name);
    if (base->root == NULL) {
        PM_LOG_WARN("Page is loaded!");
        goto exit;
    }

    if (base->priv.stash.ptr != NULL && base->priv.stash.size != 0) {
        PM_LOG_INFO("Page(%s) free stash(0x%p)[%d]", base->name, base->priv.stash.ptr, base->priv.stash.size);
        lv_mem_free(base->priv.stash.ptr);
        base->priv.stash.ptr = NULL;
        base->priv.stash.size = 0;
    }
    lv_obj_del_async(base->root);
    base->root = NULL;
    base->priv.is_cached = false;
    base->on_view_did_unload(base);

    exit:
    return PAGE_STATE_IDLE;
}
/** --- PM_State --- */


/** --- ResourceManager ---*/
typedef struct resource_node_t {
    const char *name;
    void *ptr;
} resource_node_t;


bool search_node(resource_manager_handler rm, const char *name, resource_node_t *node) {
    for (int i = 0; i < rm->node_pool->length; i++) {
        resource_node_t *item = pm_array_list_get(rm->node_pool, i);
        if (strcmp(name, item->name) == 0) {
            *node = *item;
            return true;
        }
    }
    return false;
}

/**
  * @brief  Add resources to the resource pool
  * @param  name: Resource Name
  * @param  ptr: Pointer to the resource
  * @retval Return true if the addition is successful
  */
bool rm_add_resource(resource_manager_handler rm, const char *name, void *ptr) {
    resource_node_t *node = calloc(1, sizeof(struct resource_node_t));
    if (search_node(rm, name, node)) {
        PM_LOG_WARN("Resource: %s was register", name);
        return false;
    }

    node->name = name;
    node->ptr = ptr;

    pm_array_list_insert(rm->node_pool, node, -1);

    PM_LOG_INFO("Resource: %s[0x%p] add success", node->name, node->ptr);

    return true;
}

/**
  * @brief  Remove resources from the resource pool
  * @param  name: Resource Name
  * @retval Return true if the removal is successful
  */
bool rm_remove_resource(resource_manager_handler rm, const char *name) {
    resource_node_t node;
    if (!search_node(rm, name, &node)) {
        PM_LOG_ERROR("Resource: %s was not found", name);
        return false;
    }

    resource_node_t *find = NULL;
    for (int i = 0; i < rm->node_pool->length; ++i) {
        resource_node_t *item = pm_array_list_get(rm->node_pool, i);
        if (strcmp(item->name, name) == 0) {
            find = item;
            break;
        }
    }


    if (find == NULL) {
        PM_LOG_ERROR("Resource: %s was not found", name);
        return false;
    }

    pm_array_list_remove_p(rm->node_pool, find);

    PM_LOG_INFO("Resource: %s remove success", name);

    return true;
}

/**
  * @brief  Get resource address
  * @param  name: Resource Name
  * @retval If the acquisition is successful, return the address of the resource, otherwise return the default resource
  */
void *rm_get_resource(resource_manager_handler rm, const char *name) {
    resource_node_t node;

    if (!search_node(rm, name, &node)) {
        PM_LOG_WARN("Resource: %s was not found, return default[0x%p]", name, rm->default_ptr);
        return rm->default_ptr;
    }

    PM_LOG_INFO("Resource: %s[0x%p] was found", name, node.ptr);

    return node.ptr;
}

/**
  * @brief  Set default resources
  * @param  ptr: Pointer to the default resource
  * @retval None
  */
void rm_set_default(resource_manager_handler rm, void *ptr) {
    rm->default_ptr = ptr;
    PM_LOG_INFO("Resource: set [0x%p] to default", rm->default_ptr);
}

resource_manager_handler resource_manager_init() {
    resource_manager_handler rm = calloc(1, sizeof(struct resource_manager_t));
    rm->default_ptr = NULL;
    rm->node_pool = pm_array_list_init();
    return rm;
}
/** --- ResourceManager ---*/


/** --- lv_obj_ext_func ---*/
void lv_obj_set_opa_scale(lv_obj_t *obj, int16_t opa) {
    lv_obj_set_style_bg_opa(obj, (lv_opa_t) opa, LV_PART_MAIN);
}

int16_t lv_obj_get_opa_scale(lv_obj_t *obj) {
    return lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
}

/**
  * @brief  在label后追加字符串
  * @param  label:被追加的对象
  * @param  text:追加的字符串
  * @retval 无
  */
void lv_label_set_text_add(lv_obj_t *label, const char *text) {
    if (!label)
        return;

    lv_label_ins_text(label, strlen(lv_label_get_text(label)), text);
}

/**
  * @brief  为对象添加动画
  * @param  obj:对象地址
  * @param  a:动画控制器地址
  * @param  exec_cb:控制对象属性的函数地址
  * @param  start:动画的开始值
  * @param  end:动画的结束值
  * @param  time:动画的执行时间
  * @param  delay:动画开始前的延时时间
  * @param  ready_cb:动画结束事件回调
  * @param  path_cb:动画曲线
  * @retval 无
  */
void lv_obj_add_anim(
        lv_obj_t *obj, lv_anim_t *a,
        lv_anim_exec_xcb_t exec_cb,
        int32_t start, int32_t end,
        uint16_t time,
        uint32_t delay,
        lv_anim_ready_cb_t ready_cb,
        lv_anim_path_cb_t path_cb
) {
    lv_anim_t anim_temp;

    if (a == NULL) {
        a = &anim_temp;

        /* INITIALIZE AN ANIMATION
        *-----------------------*/
        lv_anim_init(a);
    }

    /* MANDATORY SETTINGS
     *------------------*/

    /*Set the "animator" function*/
    lv_anim_set_exec_cb(a, exec_cb);

    /*Set the "animator" function*/
    lv_anim_set_var(a, obj);

    /*Length of the animation [ms]*/
    lv_anim_set_time(a, time);

    /*Set start and end values. E.g. 0, 150*/
    lv_anim_set_values(a, start, end);


    /* OPTIONAL SETTINGS
     *------------------*/

    /*Time to wait before starting the animation [ms]*/
    lv_anim_set_delay(a, delay);

    /*Set the path in an animation*/
    lv_anim_set_path_cb(a, path_cb);

    /*Set a callback to call when animation is ready.*/
    lv_anim_set_ready_cb(a, ready_cb);

    /*Set a callback to call when animation is started (after delay).*/
    lv_anim_set_start_cb(a, ready_cb);

    /* START THE ANIMATION
     *------------------*/
    lv_anim_start(a);                             /*Start the animation*/
}

void lv_obj_add_anim_def(
        lv_obj_t *obj, lv_anim_t *a,
        lv_anim_exec_xcb_t exec_cb,
        int32_t start, int32_t end
) {
    lv_obj_add_anim(obj, a, exec_cb, start, end, LV_ANIM_TIME_DEFAULT, 0, NULL, lv_anim_path_ease_out);
}

lv_indev_t *lv_get_indev(lv_indev_type_t type) {
    lv_indev_t *cur_indev = NULL;
    for (;;) {
        cur_indev = lv_indev_get_next(cur_indev);
        if (!cur_indev) {
            break;
        }

        if (cur_indev->driver->type == type) {
            return cur_indev;
        }
    }
    return NULL;
}
/** --- lv_obj_ext_func ---*/


/** --- lv_anim_timeline_wrapper ---*/
void lv_anim_timeline_add_wrapper(lv_anim_timeline_t *at, const lv_anim_timeline_wrapper_t *wrapper) {
    for (uint32_t i = 0; wrapper[i].obj != NULL; i++) {
        const lv_anim_timeline_wrapper_t *atw = &wrapper[i];

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, atw->obj);
        lv_anim_set_values(&a, atw->start, atw->end);
        lv_anim_set_exec_cb(&a, atw->exec_cb);
        lv_anim_set_time(&a, atw->duration);
        lv_anim_set_path_cb(&a, atw->path_cb);
        lv_anim_set_early_apply(&a, atw->early_apply);

        lv_anim_timeline_add(at, atw->start_time, &a);
    }
}
/** --- lv_anim_timeline_wrapper ---*/


/** --- lv_label_anim_effect ---*/
void lv_label_anim_effect_init(
        lv_label_anim_effect_t *effect,
        lv_obj_t *cont,
        lv_obj_t *label_copy,
        uint16_t anim_time
) {
    lv_obj_t *label = lv_label_create(cont);
    effect->y_offset = (lv_obj_get_height(cont) - lv_obj_get_height(label_copy)) / 2 + 1;
    lv_obj_align_to(label, label_copy, LV_ALIGN_OUT_BOTTOM_MID, 0, effect->y_offset);
    effect->label_1 = label_copy;
    effect->label_2 = label;

    effect->value_last = 0;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t) lv_obj_set_y);
    lv_anim_set_time(&a, anim_time);
    lv_anim_set_delay(&a, 0);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    effect->anim_now = a;
    effect->anim_next = a;
}

void lv_label_anim_effect_check_value(lv_label_anim_effect_t *effect, uint8_t value, lv_anim_enable_t anim_enable) {
    /*如果值相等则不切换*/
    if (value == effect->value_last)
        return;

    if (anim_enable == LV_ANIM_ON) {
        /*标签对象*/
        lv_obj_t *next_label;
        lv_obj_t *now_label;
        /*判断两个标签的相对位置，确定谁是下一个标签*/
        if (lv_obj_get_y(effect->label_2) > lv_obj_get_y(effect->label_1)) {
            now_label = effect->label_1;
            next_label = effect->label_2;
        } else {
            now_label = effect->label_2;
            next_label = effect->label_1;
        }

        lv_label_set_text_fmt(now_label, "%d", effect->value_last);
        lv_label_set_text_fmt(next_label, "%d", value);
        effect->value_last = value;
        /*对齐*/
        lv_obj_align_to(next_label, now_label, LV_ALIGN_OUT_TOP_MID, 0, -effect->y_offset);
        /*计算需要的Y偏移量*/
        lv_coord_t y_offset = abs(lv_obj_get_y(now_label) - lv_obj_get_y(next_label));

        /*滑动动画*/
        lv_anim_t *a;
        a = &(effect->anim_now);
        lv_anim_set_var(a, now_label);
        lv_anim_set_values(a, lv_obj_get_y(now_label), lv_obj_get_y(now_label) + y_offset);
        lv_anim_start(a);

        a = &(effect->anim_next);
        lv_anim_set_var(a, next_label);
        lv_anim_set_values(a, lv_obj_get_y(next_label), lv_obj_get_y(next_label) + y_offset);
        lv_anim_start(a);
    } else {
        lv_label_set_text_fmt(effect->label_1, "%d", value);
        lv_label_set_text_fmt(effect->label_2, "%d", value);
        effect->value_last = value;
    }
}
/** --- lv_label_anim_effect ---*/


/** --- stack ---*/
pm_stack_t *stack_init() {
    pm_stack_t *pStack = (pm_stack_t *) malloc(sizeof(pm_stack_t));
    if (pStack == NULL) {
        return NULL;
    }
    pStack->node = (pm_stack_node_t *) malloc(STACK_INIT_SIZE * sizeof(pm_stack_node_t));
    if (pStack->node == NULL) {
        free(pStack);
        return NULL;
    }
    for (int i = 0; i < STACK_INIT_SIZE; i++) {
        pStack->node[i].data = NULL;
    }
    pStack->length = 0;
    pStack->size = STACK_INIT_SIZE;
    return pStack;
}

void *stack_pop(pm_stack_t *stack) {
    if (stack == NULL || stack->length == 0)
        return NULL;
    void *data = stack->node[stack->length - 1].data;
    stack->node[stack->length - 1].data = NULL;
    stack->length--;
    return data;
}

void *stack_get(pm_stack_t *stack, int index) {
    if (stack == NULL || stack->length == 0 || index > (stack->length - 1))
        return NULL;
    return stack->node[index].data;
}

void *stack_top(pm_stack_t *stack) {
    if (stack == NULL || stack->length == 0)
        return NULL;
    return stack->node[stack->length - 1].data;
}

int stack_empty(pm_stack_t *stack) {
    if (stack == NULL)
        return 1;
    return stack->length == 0;
}

int stack_push(pm_stack_t *stack, void *data) {
    if (stack == NULL)
        return 0;
    if (stack->length == stack->size - 1) {
        unsigned long realloc_size = stack->size + STACK_INCREASE_SIZE;
        stack->node = (pm_stack_node_t *) realloc(stack->node, realloc_size * sizeof(pm_stack_node_t));
        if (stack->node == NULL) {
            return 0;
        }
        for (unsigned long i = stack->length; i < realloc_size; i++) {
            stack->node[i].data = NULL;
        }
        stack->size = realloc_size;
    }
    stack->node[stack->length].data = data;
    stack->length++;
    return 1;
}
/** --- stack ---*/


/** --- array_list ---*/
pm_array_list_t *pm_array_list_init() {
    pm_array_list_t *p_list = (pm_array_list_t *) malloc(sizeof(pm_array_list_t));
    if (p_list == NULL) {
        return NULL;
    }
    p_list->node = (pm_array_node_t *) malloc(PM_ARRAY_LIST_INIT_SIZE * sizeof(pm_array_node_t));
    if (p_list->node == NULL) {
        free(p_list);
        return NULL;
    }
    for (int i = 0; i < PM_ARRAY_LIST_INIT_SIZE; i++) {
        p_list->node[i].data = NULL;
    }
    p_list->length = 0;
    p_list->size = PM_ARRAY_LIST_INIT_SIZE;
    return p_list;
}

int pm_array_list_insert(pm_array_list_t *p_list, void *p_data, long index) {
    if (p_list == NULL)
        return -1;
    if (index < -1 || (index > p_list->length && index != -1)) {
        return -1;
    }
    if (p_list->length == p_list->size - 1) {
        unsigned long realloc_size = p_list->size + PM_ARRAY_LIST_INCREASE_SIZE;
        p_list->node = (pm_array_node_t *) realloc(p_list->node, realloc_size * sizeof(pm_array_node_t));
        if (p_list->node == NULL) {
            return -1;
        }
        for (int i = p_list->length; i < realloc_size; i++) {
            p_list->node[i].data = NULL;
        }
        p_list->size = realloc_size;
    }
    if (index == -1) {
        p_list->node[p_list->length].data = p_data;
        p_list->length++;
        return 0;
    } else if (index == 0) {
        for (int i = p_list->length; i > 0; i--) {
            p_list->node[i] = p_list->node[i - 1];
        }
        p_list->node[0].data = p_data;
        p_list->length++;
        return 0;
    } else {
        int i;
        for (i = p_list->length; i > index; i--) {
            p_list->node[i] = p_list->node[i - 1];
        }
        p_list->node[i].data = p_data;
        p_list->length++;
        return 0;
    }
    return 0;
}

void pm_array_list_remove(pm_array_list_t *p_list, unsigned long index) {
    if (p_list == NULL)
        return;
    if (index < 0 || index >= p_list->length)
        return;
    for (int i = index; i < p_list->length; i++) {
        p_list->node[i] = p_list->node[i + 1];
    }
    p_list->node[p_list->length - 1].data = NULL;
    p_list->length--;
}

void pm_array_list_remove_p(pm_array_list_t *p_list, void *p_data) {
    if (p_list == NULL)
        return;
    for (int i = 0; i < p_list->length; ++i) {
        if (p_list->node[i].data == p_data) {
            pm_array_list_remove(p_list, i);
            break;
        }
    }
}

void *pm_array_list_get(pm_array_list_t *p_list, unsigned long index) {
    void *p_data = NULL;
    if (p_list == NULL) {
        return NULL;
    }
    if (index < 0 || index >= p_list->length) {
        return NULL;
    }
    p_data = p_list->node[index].data;
    return p_data;
}

void pm_array_list_clear(pm_array_list_t *p_list) {
    int i = 0;
    if (p_list == NULL) {
        return;
    }
    for (i = 0; i < p_list->length; i++) {
        if (p_list->node[i].data != NULL) {
            p_list->node[i].data = NULL;
        }
    }
    p_list->length = 0;
}

void pm_array_list_free(pm_array_list_t *p_list) {
    if (p_list == NULL)
        return;
    if (p_list->node != NULL) {
        free(p_list->node);
        p_list->node = NULL;
    }
    free(p_list);
    p_list = NULL;
}
/** --- array_list ---*/


/** --- data_center --- */

/** <dc_topic_subscribe_t> */
static pm_array_list_t *dc_topic_list = NULL;

/** <dc_page_data_msg_t> */
static pm_array_list_t *dc_msg_list = NULL;

static pm_array_list_t *dc_temp_del_list = NULL;

static dc_topic_subscribe_t *find_topic(const char *topic){
    dc_topic_subscribe_t *topic_subscribe = NULL;
    for (int i = 0; i < dc_topic_list->length; ++i) {
        dc_topic_subscribe_t *item = pm_array_list_get(dc_topic_list, i);
        if(strcmp(item->topic,topic) == 0){
            topic_subscribe = item;
            break;
        }
    }
    return topic_subscribe;
}
void dc_subscribe(const char *topic,dc_callback c){
    dc_topic_subscribe_t *topic_subscribe = find_topic(topic);
    if(!topic_subscribe){
        topic_subscribe = calloc(1, sizeof (dc_topic_subscribe_t));
        topic_subscribe->topic = topic;
        topic_subscribe->subscribe_list = pm_array_list_init();
        pm_array_list_insert(dc_topic_list, topic_subscribe, -1);
    }
    bool find_c = false;
    for (int i = 0; i < topic_subscribe->subscribe_list->length; ++i) {
        dc_callback c_item = pm_array_list_get(topic_subscribe->subscribe_list, i);
        if(c == c_item){
            find_c = true;
            break;
        }
    }
    if(!find_c){
        pm_array_list_insert(topic_subscribe->subscribe_list, c, -1);
    }
}
void dc_unsubscribe(const char *topic,dc_callback c){
    dc_topic_subscribe_t *topic_subscribe = find_topic(topic);
    if(topic_subscribe){
        pm_array_list_remove_p(topic_subscribe->subscribe_list, c);
    }
}
void dc_publish(dc_data_msg_t *data_msg){
    if(data_msg){
        data_msg->sync = false;
        pm_array_list_insert(dc_msg_list, data_msg, -1);
    }
}
void dc_sync_req(dc_data_msg_t *data_msg){
    data_msg->sync = true;
    dc_topic_subscribe_t *topic_subscribe = find_topic(data_msg->topic);
    if(topic_subscribe){
        if(topic_subscribe->subscribe_list->length > 0){
            for (int j = 0; j < topic_subscribe->subscribe_list->length; ++j) {
                dc_callback c = pm_array_list_get(topic_subscribe->subscribe_list, j);
                if(c){
                    c(data_msg);
                }
            }
        }
    }
}
dc_data_msg_t * dc_create_msg(const char *topic){
    dc_data_msg_t *msg = calloc(1, sizeof (dc_data_msg_t));
    msg->topic = topic;
    return msg;
}
void data_center(){
    if(dc_msg_list->length > 0){
        for (int i = 0; i < dc_msg_list->length; ++i) {
            dc_data_msg_t *data_msg = pm_array_list_get(dc_msg_list, i);
            dc_topic_subscribe_t *topic_subscribe = find_topic(data_msg->topic);
            if(topic_subscribe){
                for (int j = 0; j < topic_subscribe->subscribe_list->length; ++j) {
                    dc_callback c = pm_array_list_get(topic_subscribe->subscribe_list, j);
                    if(c){
                        c(data_msg);
                    }
                }
            }
            pm_array_list_insert(dc_temp_del_list, data_msg, -1);
        }
    }

    if(dc_temp_del_list->length > 0){
        for (int i = 0; i < dc_temp_del_list->length; ++i) {
            dc_data_msg_t *data_msg = pm_array_list_get(dc_temp_del_list, i);
            pm_array_list_remove_p(dc_msg_list, data_msg);
            if(data_msg->data){
                free(data_msg->data);
            }
            free(data_msg);
        }
        pm_array_list_clear(dc_temp_del_list);
    }
}
/** --- data_center --- */

/**
  * @brief  Page manager constructor
  * @param  factory: Pointer to the page factory
  * @retval None
  */
void ui_page_manager_init() {
    page_prev = NULL;
    page_current = NULL;
    memset(&anim_state, 0, sizeof(anim_state));
    page_stack = stack_init();
    page_pool = pm_array_list_init();

    // data center init
    dc_topic_list = pm_array_list_init();
    dc_msg_list = pm_array_list_init();
    dc_temp_del_list = pm_array_list_init();

    pm_set_global_load_anim_type(LOAD_ANIM_OVER_LEFT, 500, lv_anim_path_ease_out);
}