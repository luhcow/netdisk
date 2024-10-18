#include "fs/fs_handler.h"

#include <rabbitmq-c/amqp.h>

#include "public/handler.h"

amqp_bytes_t fs_get(amqp_bytes_t body);

void fs_handler_init(struct handler_map* fs_handler) {
    handler_add(fs_handler, "fs.get", fs_get);
}