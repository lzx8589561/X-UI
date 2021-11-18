#include "resource_pool.h"
#include "../ui_page_manager.h"
#include <stdlib.h>

static resource_manager_handler font_;
static resource_manager_handler image_;

#define IMPORT_FONT(name) \
do{\
    LV_FONT_DECLARE(font_##name)\
    rm_add_resource(font_, #name, (void*)&font_##name);\
}while(0)

#define IMPORT_IMG(name) \
do{\
    LV_IMG_DECLARE(img_src_##name)\
    rm_add_resource(image_, #name, (void*)&img_src_##name);\
}while (0)

static void resource_init()
{
    /* Import Fonts */
    IMPORT_FONT(bahnschrift_13);
    IMPORT_FONT(bahnschrift_17);
    IMPORT_FONT(bahnschrift_32);
    IMPORT_FONT(bahnschrift_65);
    IMPORT_FONT(agencyb_36);

    /* Import Images */
    IMPORT_IMG(alarm);
    IMPORT_IMG(battery);
    IMPORT_IMG(battery_info);
    IMPORT_IMG(bicycle);
    IMPORT_IMG(compass);
    IMPORT_IMG(gps_arrow_default);
    IMPORT_IMG(gps_arrow_dark);
    IMPORT_IMG(gps_arrow_light);
    IMPORT_IMG(gps_pin);
    IMPORT_IMG(gyroscope);
    IMPORT_IMG(locate);
    IMPORT_IMG(map_location);
    IMPORT_IMG(menu);
    IMPORT_IMG(origin_point);
    IMPORT_IMG(pause);
    IMPORT_IMG(satellite);
    IMPORT_IMG(sd_card);
    IMPORT_IMG(start);
    IMPORT_IMG(stop);
    IMPORT_IMG(storage);
    IMPORT_IMG(system_info);
    IMPORT_IMG(time_info);
    IMPORT_IMG(trip);
}

void resource_pool_init()
{
    font_ = calloc(1, sizeof(struct resource_manager_t));
    font_->node_pool = pm_array_list_init();
    image_ = calloc(1, sizeof(struct resource_manager_t));
    image_->node_pool = pm_array_list_init();
    resource_init();
    font_->default_ptr = ((void*)&lv_font_montserrat_14);
}

lv_font_t* resource_pool_get_font(const char* name)
{
    return (lv_font_t*) rm_get_resource(font_, name);
}
const void* resource_pool_get_image(const char* name)
{
    return rm_get_resource(image_, name);
}