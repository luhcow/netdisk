#include <json.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api_gateway.h"
#include "blockq.h"

BlockQ pool;
pthread_t pool_thread;
pthread_t pthreads[32];

extern char rabbitmq_hostname[16];
extern int rabbitmq_port;
extern char rabbitmq_vhost[16];
extern amqp_channel_t rabbitmq_channel;
extern char rabbitmq_exchange[16];
extern char rabbitmq_exchange_type[16];

unsigned short int port = 0;

int server_sock = -1;

void handler(int sign);
void function(void);
void conf_read(void);
void* gateway_pool_build(void* pool_void);
static void* consumer(void* task);

int main(int argc, char* argv[]) {
    // if (daemon(0, 0) == -1) {
    //     // Error handling
    // }
    // 注册
    signal(SIGINT, handler);
    atexit(function);

    // 建线程池
    pthread_create(&pool_thread, NULL, gateway_pool_build, &pool);

    // 接受连接
    if (argc != 1)
        port = atoi(argv[1]);

    // 在对应端口建立 httpd 服务
    server_sock = startup(&port);
    fprintf(stderr, "httpd running on port %d\n", port);

    // 生产数据
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    while (1) {
        // 套接字收到客户端连接请求
        int client_sock = accept(server_sock, (struct sockaddr*)&client_name,
                                 &client_name_len);
        if (client_sock == -1)
            perror("accept");
        pool.ops->push(&pool, client_sock);
    }

    // 线程池退出
    close(server_sock);
    pthread_cancel(pool_thread);
    pthread_join(pool_thread, NULL);
    exit(0);
    return 0;
}

void handler(int sign) {
    perror("收到 Ctrl C，执行退出处理");
    exit(0);
    return;
}

void function(void) {
    close(server_sock);
    perror("服务端口关闭");
    pthread_cancel(pool_thread);
    perror("向线程池发送 cancel");
    pthread_join(pool_thread, NULL);
    perror("线程池已取消");
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
    port = json_object_get_int(json_object_object_get(jobj, "gateway_port"));
}

static void routine(void* pool_void) {
    perror("线程池正在取消各个线程");
    BlockQ* pool = (BlockQ*)pool_void;
    fprintf(stderr, "wait %d task in queue\n", pool->ops->num(pool));
    for (int i = 0; i < 2; i++) {
        pool->ops->push(pool, -1);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(pthreads[i], NULL);
    }
    perror("各个线程已取消");
    return;
}

void* gateway_pool_build(void* pool_void) {
    BlockQ* pool = (BlockQ*)pool_void;
    blockq_create(pool);
    pthread_cleanup_push(routine, pool_void);
    for (int i = 0; i < 2; i++) {
        pthread_create(pthreads + i, NULL, consumer, pool_void);
    }
    while (1) sleep(100);
    pthread_exit(NULL);
    return NULL;
    pthread_cleanup_pop(pool_void);
}

static void* consumer(void* pool_void) {
    BlockQ* pool = (BlockQ*)pool_void;
    api_gateway_prework();
    for (;;) {
        int num = pool->ops->pop(pool);
        if (num == -1) {
            fprintf(stderr, "0x%lx get sign to exit, done\n", pthread_self());
            return NULL;
        }
        api_gateway_work(num);
    }
    api_gateway_endwork();
    return NULL;
}