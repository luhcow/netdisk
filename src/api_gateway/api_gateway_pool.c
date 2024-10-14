#include <json.h>
#include <signal.h>
#include <stdlib.h>

#include "api_gateway.h"
#include "pool.h"

BlockQ pool;
pthread_t pool_thread;

extern char *rabbitmq_hostname;
extern int rabbitmq_port;
extern char *rabbitmq_vhost;
extern amqp_channel_t rabbitmq_channel;
extern char *rabbitmq_exchange;
extern char *rabbitmq_exchange_type;

unsigned short int port = 0;

int server_sock = -1;

void handler(int sign);
void function(void);
void conf_read(void);

int main(int argc, char *argv[]) {
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
        int client_sock = accept(server_sock, (struct sockaddr *)&client_name,
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
    FILE *conf = fopen("api_gateway.json", "r");
    fseek(conf, 0, SEEK_END);
    long conf_len = ftell(conf);
    fseek(conf, 0, SEEK_SET);
    char buf[conf_len + 1];
    fread(buf, 1, conf_len, conf);
    struct json_object *jobj = json_tokener_parse(buf);
    rabbitmq_hostname = json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_hostname"));
    rabbitmq_port =
            json_object_get_int(json_object_object_get(jobj, "rabbitmq_port"));
    rabbitmq_vhost = json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_vhost"));
    rabbitmq_channel = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_channel"));
    rabbitmq_exchange = json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange"));
    rabbitmq_exchange_type = json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange_type"));
    port = json_object_get_int(json_object_object_get(jobj, "ateway_port"));
}