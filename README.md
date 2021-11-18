# X-UI LVGL8 for ESP-IDF

移植自X-TRACK项目的页面栈框架，新增支持异步通信的订阅发布数据中心

## 特点
- 使用C语言重构，方便继承复用
- 核心文件 ui_page_manager.h、ui_page_manager.c 实现页面栈、订阅发布数据中心
- 完整页面生命周期
- MVC架构 数据页面分离，方便后期维护
- 集成LVGL Windows模拟器，使用VS2019启动

## API说明

### 页面生命周期
- on_custom_attr_config - 自定义配置
- on_view_load - 页面加载
- on_view_did_load - 页面加载完成
- on_view_will_appear - 页面将要显示
- on_view_did_appear - 页面已经显示
- on_view_will_disappear - 页面将要不可见
- on_view_did_disappear - 页面已经不可见
- on_view_did_unload - 页面已卸载

### 页面定义
#### 回调注册
> 以dialplate为例，在页面中定义函数page_base_handler ui_page_register_${pageName}()
~~~
page_base_handler ui_page_register_dialplate() {
    UI_PAGE_FUN_INIT();
    return pb;
}
~~~
#### 回调注册
> 添加dialplate的扫描
~~~
page_base_handler pm_create_page(const char* name){
    APP_CLASS_MATCH(start_up);
    APP_CLASS_MATCH(dialplate); // 添加dialplate定义
    return NULL;
}
~~~

### 初始化
~~~
// 初始化页面栈
ui_page_manager_init();
// 初始化码表盘页面控制器
dialplate_controller_init();
// 初始化资源池
resource_pool_init();
// 状态栏初始化
status_bar_init(lv_layer_top());

// 页面装载到页面池
pm_install("start_up",    "page/start_up");
pm_install("dialplate",    "page/dialplate");

// 设置全局页面加载动画
pm_set_global_load_anim_type(LOAD_ANIM_OVER_TOP, 500, lv_anim_path_ease_out);

// 加载
pm_push("page/start_up", NULL);
~~~

### 页面栈
#### 压栈
> 第二个参数为携带参数，在目标页面可通过pm_get_stash()获取参数
~~~
pm_push("page/dialplate", NULL);
~~~

#### 出栈
~~~
pm_pop();
~~~

#### 回到首页
~~~
pm_back_home();
~~~

#### 配置页面缓存
> 在页面生命周期 on_custom_attr_config 中调用
~~~
set_custom_cache_enable(pb, false);
~~~

### 数据中心
> 支持多线程发布消息，底层通过队列分发给订阅者，统一由UI线程进行回调，在处理接收消息时，若需耗时操作需要创建新的线程去处理
#### 订阅主题
> 具体可参考dialplate示例
~~~
// 回调
static void on_data_event(dc_data_msg_t *d) {
    if (strcmp(d->topic, "test_topic") == 0) {
        if (d->msg_type == TEST_MSG_TEST_DATA) {
            test_data_t *ptr = (test_data_t *) d->data;
            *test_data = *ptr;
        }
    }
}
// 订阅
dc_subscribe("test_topic", on_data_event);
~~~

#### 取消订阅
> 具体可参考dialplate示例
~~~
dc_unsubscribe("test_topic", on_data_event);
~~~

#### 发布消息
> 订阅了这个主题的客户都可以收到消息
~~~
// 创建一个消息，指定主题名
dc_data_msg_t *msg = dc_create_msg("test_topic");
// 设置消息类型
msg->msg_type = TEST_MSG_TEST_DATA;
test_data_t *data = calloc(1, sizeof(test_data_t));
data->attr1 = a1;
data->attr2 = a2;
data->attr3 = a3;
// 设置消息
msg->data = data;
// 发布消息
dc_publish(msg);
~~~

#### 同步发送消息
> 不经过队列，通过UI线程直接调用回调，不可进行耗时操作否则会卡死页面
~~~
// UI中同步请求
dc_data_msg_t msg = {
        .topic = "test_topic", // 发送到那个主题
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

// Controller中处理
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
~~~