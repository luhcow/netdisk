#ifndef ND_WD_rabbitmq_rpc_sending
#define ND_WD_rabbitmq_rpc_sending

#include <assert.h>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

amqp_connection_state_t rabbitmq_connect_server(const char *hostname,
                                                const int port,
                                                const char *vhost,
                                                amqp_channel_t channel);

amqp_bytes_t rabbitmq_rpc_publisher_declare(amqp_connection_state_t conn,
                                            amqp_channel_t channel,
                                            const char *exchange,
                                            const char *type);

int rabbitmq_rpc_publish(amqp_connection_state_t conn, amqp_channel_t channel,
                         const char *exchange, amqp_bytes_t reply_to_queue,
                         const char *routing_key, amqp_bytes_t message_body);

amqp_bytes_t rabbitmq_rpc_wait_answer(amqp_connection_state_t conn,
                                      amqp_channel_t channel,
                                      amqp_bytes_t reply_to_queue);

int rabbitmq_close(amqp_connection_state_t conn, amqp_channel_t channel);
#endif