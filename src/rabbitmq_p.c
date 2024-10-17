#include "rabbitmq_p.h"

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "handler.h"
#include "rabbitma_p.h"
#include "rpc_sending.h"

_Thread_local amqp_connection_state_t conn;
_Thread_local amqp_bytes_t reply_to;

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

int rabbit_consumer(struct handler_map handler) {
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(conn);

    res = amqp_consume_message(conn, &envelope, NULL, 0);

    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        return -1;
    }

    printf("Delivery %u, exchange %.*s routingkey %.*s "
           "correlation_id %.*s "
           "consumer_tag %.*s"
           "replyto %.*s\n",
           (unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
           (char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
           (char *)envelope.routing_key.bytes,
           (int)envelope.message.properties.correlation_id.len,
           (char *)envelope.message.properties.correlation_id.bytes,
           (int)envelope.consumer_tag.len, (char *)envelope.consumer_tag.bytes,
           (int)envelope.message.properties.reply_to.len,
           (char *)envelope.message.properties.reply_to.bytes);

    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
        printf("Content-type: %.*s\n",
               (int)envelope.message.properties.content_type.len,
               (char *)envelope.message.properties.content_type.bytes);
    }
    printf("----\n");

    amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
    printf("working\n");

    amqp_bytes_t send_body = handler_find(
            &handler, envelope.routing_key.bytes)(envelope.message.body);
    rabbit_publish(envelope.message.properties.reply_to, send_body,
                   envelope.message.properties.correlation_id);

    amqp_destroy_envelope(&envelope);

    printf("done\n\n");

    amqp_basic_ack(conn, 1, envelope.delivery_tag, 0);

    return 0;
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