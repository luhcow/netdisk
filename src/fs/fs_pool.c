#include <json.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <rabbitmq-c/amqp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs.h"
#include "nd_mysql.h"
#include "pool.h"

pthread_t pool_thread;

extern char rabbitmq_hostname[16];
extern int rabbitmq_port;
extern char rabbitmq_vhost[16];
extern amqp_channel_t rabbitmq_channel;
extern char rabbitmq_exchange[16];
extern char rabbitmq_exchange_type[16];

void capture_signal(int sign);
void exit_processing(void);
void conf_read(void);

int main(int argc, char *argv[]) {
    // if (daemon(0, 0) == -1) {
    //     // Error handling
    // }
    // 注册
    signal(SIGINT, capture_signal);
    atexit(exit_processing);
    if (mysql_library_init(0, NULL, NULL) != 0)  // 初始化mysql数据库
    {
        fprintf(stderr, "could not initialize MySQL client library\n");
        exit(1);
    }
    // 建线程池
    struct pool_t *pool = pool_create(fs_run, false, 2);
    pool->build(pool);
    // 线程池退出
    pool->cancel(pool);

    exit(0);
    return 0;
}

void capture_signal(int sign) {
    perror("收到 Ctrl C，执行退出处理");
    exit(0);
    return;
}

void exit_processing(void) {
    pthread_cancel(pool_thread);
    perror("向线程池发送 cancel");
    pthread_join(pool_thread, NULL);
    perror("线程池已取消");
    mysql_library_end();
    return;
}

void conf_read(void) {
    FILE *conf = fopen("api_gateway.json", "r");
    fseek(conf, 0, SEEK_END);
    long conf_len = ftell(conf);
    fseek(conf, 0, SEEK_SET);
    char buf[conf_len + 1];
    fread(buf, 1, conf_len, conf);
    struct json_object *jobj = json_tokener_parse(buf);
    strncpy(rabbitmq_hostname,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_hostname")),
            16);
    rabbitmq_port =
            json_object_get_int(json_object_object_get(jobj, "rabbitmq_port"));
    strncpy(rabbitmq_vhost,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_vhost")),
            16);
    rabbitmq_channel = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_channel"));
    strncpy(rabbitmq_exchange,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_exchange")),
            16);
    strncpy(rabbitmq_exchange_type,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_exchange_type")),
            16);
}