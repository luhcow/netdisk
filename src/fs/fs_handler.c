#include "fs_handler.h"

#include <rabbitmq-c/amqp.h>

#include "handler.h"

amqp_bytes_t fs_get(amqp_bytes_t body);

int fs_hanlder_init(struct handler_map* fs_handler) {
    handler_add(fs_handler, "fs.get", fs_get);
}