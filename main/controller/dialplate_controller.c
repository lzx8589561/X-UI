#include <sys/cdefs.h>
#include <stdio.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include <freertos/task.h>
#include "dialplate_controller.h"
#include "../ui/ui_page_manager.h"
#include "../ui/app_entity_def.h"

static int a1 = 1;
static int a2 = 1;
static int a3 = 1;

/***
 * 消息回调接收 （由UI线程调用）
 * @param data_msg 消息
 */
static void on_data_event(dc_data_msg_t *data_msg) {
    if (data_msg->sync) {
        // 处理同步消息 同步消息不可做耗时操作
        switch (data_msg->msg_type) {
            case TEST_MSG_SYNC_TEST_DATA: {
                char *str = calloc(50, sizeof(char));
                sprintf(str, "s%d", a1);
                // 放入返回数据
                data_msg->sync_result = str;
                break;
            }
        }
    }else{
        // 处理异步消息 如处理需要耗时，创建一个新的任务去处理
    }
}

/**
 * 任务定时向UI发送数据示例
 */
_Noreturn static void dialplate_update_task() {
    while (1) {
        // 申请的内存会在发送完成后自动释放
        dc_data_msg_t *msg = dc_create_msg("test_topic");
        msg->msg_type = TEST_MSG_TEST_DATA;
        test_data_t *data = calloc(1, sizeof(test_data_t));
        data->attr1 = a1;
        data->attr2 = a2;
        data->attr3 = a3;
        msg->data = data;
        // 发布消息
        dc_publish(msg);

        // 变化数据
        a1++;
        a2 += 2;
        a3 += 3;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void dialplate_controller_init() {
    // 订阅主题
    dc_subscribe("test_topic", on_data_event);

    xTaskCreate((TaskFunction_t) dialplate_update_task, "dialplate_update_task",
                2048, NULL, 5,
                NULL);
}