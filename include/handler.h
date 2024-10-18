#ifndef ND_WD_HANDLER
#define ND_WD_HANDLER

#include <rabbitmq-c/amqp.h>
#include <uthash.h>

struct handler_map {
    char* key; /* key */
    void* (*handler)(void*);
    UT_hash_handle hh; /* makes this structure hashable */
};

void handler_add(struct handler_map* head, char* key, void* (*handler)(void*));
void* (*handler_find(struct handler_map* head, char* key))(void*);
void handler_destroy(struct handler_map* head);

#endif