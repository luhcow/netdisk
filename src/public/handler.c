#include "public/handler.h"

#include <stdlib.h>

#include "public/uthash.h"

void handler_add(struct handler_map* head, const char* key,
                 void* (*handler)(void*)) {
    struct handler_map* s;

    s = malloc(sizeof *s);
    s->handler = handler;
    s->key = key;
    HASH_ADD_STR(head, key, s); /* id: name of key field */
}

void* (*handler_find(struct handler_map* head, char* key))(void*) {
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
