#include <json.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api_gateway/api_gateway.h"
#include "api_gateway/rpc_sending.h"
#include "public/http.h"

extern char* rabbitmq_hostname;
extern int rabbitmq_port;
extern char* rabbitmq_vhost;
extern amqp_channel_t rabbitmq_channel;
extern char* rabbitmq_exchange;
extern char* rabbitmq_exchange_type;

void conf_read(void);

int main(int argc, char* argv[]) {
    // if (daemon(0, 0) == -1) {
    //     // Error handling
    // }

    handle_func("/fs/", fs_handler);
    handle_func("/d/", fs_handler);
    handle_func("/auth/", auth_handler);
    listen_and_serve(":8081", 2, api_gateway_prework,
                     api_gateway_endwork);

    exit(0);
    return 0;
}

void conf_read(void) {
    FILE* conf = fopen("api_gateway.json", "r");
    fseek(conf, 0, SEEK_END);
    long conf_len = ftell(conf);
    fseek(conf, 0, SEEK_SET);
    char buf[conf_len + 1];
    fread(buf, 1, conf_len, conf);

    struct json_object* jobj = json_tokener_parse(buf);

    rabbitmq_hostname = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_hostname")));
    rabbitmq_port = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_port"));
    rabbitmq_vhost = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_vhost")));
    rabbitmq_channel = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_channel"));
    rabbitmq_exchange = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange")));
    rabbitmq_exchange_type = strdup(json_object_get_string(
            json_object_object_get(jobj, "rabbitmq_exchange_type")));
}