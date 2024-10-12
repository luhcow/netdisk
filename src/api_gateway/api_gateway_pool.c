#include "api_gateway.h"
#include "pool.h"

BlockQ pool;
int main(int argc, char* argv[]) {
    // 建线程池
    pthread_t pool_thread;
    pthread_create(&pool_thread, NULL, gateway_pool_build, &pool);

    // 接受连接
    int server_sock = -1;

    unsigned short int port = 0;
    if (argc != 1)
        port = atoi(argv[1]);
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);

    // 在对应端口建立 httpd 服务
    server_sock = startup(&port);
    fprintf(stderr, "httpd running on port %d\n", port);

    // 生产数据
    while (1) {
        /*套接字收到客户端连接请求*/
        client_sock =
                accept(server_sock, (struct sockaddr*)&client_name,
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