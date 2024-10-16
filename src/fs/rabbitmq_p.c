#include "rabbitmq_p.h"

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

int listenbegin() {
    const char *hostname = rabbitmq_hostname;
    int port = rabbitmq_port;

    int status;
    const char *exchange = rabbitmq_exchange;
    amqp_socket_t *socket = NULL;

    amqp_bytes_t queuename;

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        die("creating TCP socket");
    }

    status = amqp_socket_open(socket, hostname, port);
    if (status) {
        die("opening TCP socket");
    }

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0,
                                 AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                      "Logging in");
    amqp_channel_open(conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    amqp_exchange_declare_ok_t *r = amqp_exchange_declare(
            conn, rabbitmq_channel, amqp_cstring_bytes(exchange),
            amqp_cstring_bytes(rabbitmq_exchange_type), 0, 1, 0, 0,
            amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    {
        amqp_queue_declare_ok_t *r = amqp_queue_declare(
                conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 1;
        }
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
                    amqp_empty_bytes, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
                       amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
}

int rabbitmq_beginwork(void) {
    // 建立连接
    conn = rabbitmq_connect_server(rabbitmq_hostname, rabbitmq_port,
                                   rabbitmq_vhost, rabbitmq_channel);

    // 声明队列
    reply_to = rabbitmq_rpc_publisher_declare(
            conn, rabbitmq_channel, rabbitmq_exchange, rabbitmq_exchange_type);
}
// int api_gateway_work(int client) {
//     return accept_request(client);
// }
int rabbitmq_endwork(void) {
    // 断开连接
    rabbitmq_close(conn, rabbitmq_channel);
}