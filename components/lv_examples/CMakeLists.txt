if(ESP_PLATFORM)

file(GLOB_RECURSE SOURCES ../../lv_sim_visual_studio/LVGL.Simulator/lv_demos/*.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ../../lv_sim_visual_studio/LVGL.Simulator/lv_demos/src
                       REQUIRES lvgl)

else()
    message(FATAL_ERROR "LVGL LV examples: ESP_PLATFORM is not defined. Try reinstalling ESP-IDF.")
endif()
