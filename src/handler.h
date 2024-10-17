#include <rabbitmq-c/amqp.h>
#include <uthash.h>

struct handler_map {
    char* key; /* key */
    void* (*handler)(void*);
    UT_hash_handle hh; /* makes this structure hashable */
};

void handler_add(struct handler_map* head, char* key,
                 amqp_bytes_t (*handler)(amqp_bytes_t));
amqp_bytes_t (*handler_find(struct handler_map* head, char* key))(amqp_bytes_t);
void handler_destroy(struct handler_map* head);