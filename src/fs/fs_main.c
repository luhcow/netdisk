#define _POSIX_C_SOURCE 200809L

#include <json.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <rabbitmq-c/amqp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs/fs.h"
#include "public/pool.h"

struct pool_t* pool;

extern char* rabbitmq_hostname;
extern int rabbitmq_port;
extern char* rabbitmq_vhost;
extern amqp_channel_t rabbitmq_channel;
extern char* rabbitmq_exchange;
extern char* rabbitmq_exchange_type;

void capture_signal(int sign);
void exit_processing(void);
void conf_read(void);

int main(int argc, char* argv[]) {
    // if (daemon(0, 0) == -1) {
    //     // Error handling
    // }
    conf_read();
    // 注册
    signal(SIGINT, capture_signal);
    atexit(exit_processing);

    // 初始化mysql数据库
    if (mysql_library_init(0, NULL, NULL) != 0) {
        fprintf(stderr,
                "could not initialize MySQL client library\n");
        exit(1);
    }
    // 建线程池
    pool = pool_create(fs_run, false, 2);
    pool->build(pool);
    while (1) sleep(100);

    exit(0);
}

void capture_signal(int sign) {
    perror("收到 Ctrl C，执行退出处理");
    exit(0);
}

void exit_processing(void) {
    perror("向线程池发送 cancel");
    pool->cancel(pool);
    perror("线程池已取消");
    mysql_library_end();
    return;
}

void conf_read(void) {
    FILE* conf = fopen("api_gateway.json", "r");
    fseek(conf, 0, SEEK_END);
    long conf_len = ftell(conf);
    fseek(conf, 0, SEEK_SET);
    char buf[conf_len + 1];
    fread(buf, 1, conf_len, conf);

    struct json_object* jobj = json_tokener_parse(buf);

    rabbitmq_hostname = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_hostname")));
    rabbitmq_port = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_port"));
    rabbitmq_vhost = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_vhost")));
    rabbitmq_channel = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_channel"));
    rabbitmq_exchange = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange")));
    rabbitmq_exchange_type = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange_type")));
}