#ifndef ND_WD_RABBITMQ
#define ND_WD_RABBITMQ

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "handler.h"
#include "rpc_sending.h"

char* rabbitmq_hostname = "52.77.251.3";
int rabbitmq_port = 5672;
char* rabbitmq_vhost = "/";
amqp_channel_t rabbitmq_channel = 1;
char* rabbitmq_exchange = "gateway";
char* rabbitmq_exchange_type = "topic";

int listenbegin();
int rabbit_consumer(struct handler_map handler);

#endif