#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "rpc_sending.h"

_Thread_local amqp_connection_state_t conn;
_Thread_local amqp_bytes_t reply_to;

char rabbitmq_hostname[16] = "52.77.251.3";
int rabbitmq_port = 5672;
char rabbitmq_vhost[16] = "/";
amqp_channel_t rabbitmq_channel = 1;
char rabbitmq_exchange[16] = "gateway";
char rabbitmq_exchange_type[16] = "topic";

int rabbitmq_beginwork(void) {
    // 建立连接
    conn = rabbitmq_connect_server(rabbitmq_hostname, rabbitmq_port,
                                   rabbitmq_vhost, rabbitmq_channel);

    // 声明队列
    reply_to = rabbitmq_rpc_publisher_declare(
            conn, rabbitmq_channel, rabbitmq_exchange, rabbitmq_exchange_type);
}
int api_gateway_work(int client) {
    return accept_request(client);
}
int rabbitmq_endwork(void) {
    // 断开连接
    rabbitmq_close(conn, rabbitmq_channel);
}