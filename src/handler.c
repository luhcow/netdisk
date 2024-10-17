#include <handler.h>
#include <rabbitmq-c/amqp.h>
#include <uthash.h>

void handler_add(struct handler_map* head, char* key,
                 amqp_bytes_t (*handler)(amqp_bytes_t)) {
    struct handler_map* s;

    s = malloc(sizeof *s);
    s->handler = handler;
    s->key = key;
    HASH_ADD_STR(head, key, s); /* id: name of key field */
}

amqp_bytes_t (*handler_find(struct handler_map* head,
                            char* key))(amqp_bytes_t) {
    struct handler_map* s;
    HASH_FIND_STR(head, key, s);
    return s->handler;
}

void handler_destroy(struct handler_map* head) {
    struct handler_map *s, *tmp;
    HASH_ITER(hh, head, s, tmp) {
        HASH_DEL(head, s);
        free(s);
    }
}
