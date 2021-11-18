#ifndef APP_ENTITY_DEF_H
#define APP_ENTITY_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 定义全局实体
 */

/**
 * 测试传输消息
 */
typedef enum {
    TEST_MSG_TEST_DATA,
    TEST_MSG_SYNC_TEST_DATA
}test_msg_t;

/**
 * 定义UI所需的数据格式
 */
typedef struct {
    int attr1;
    int attr2;
    int attr3;
}test_data_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //APP_ENTITY_DEF_H
