#include <rabbitmq-c/amqp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "rabbitmq_p.h"

_Thread_local extern amqp_connection_state_t conn;
_Thread_local extern amqp_bytes_t reply_to;

extern char rabbitmq_hostname[16];
extern int rabbitmq_port;
extern char rabbitmq_vhost[16];
extern amqp_channel_t rabbitmq_channel;
extern char rabbitmq_exchange[16];
extern char rabbitmq_exchange_type[16];

static void conf_read(void) {
    FILE *conf = fopen("api_gateway.json", "r");
    fseek(conf, 0, SEEK_END);
    long conf_len = ftell(conf);
    fseek(conf, 0, SEEK_SET);
    char buf[conf_len + 1];
    fread(buf, 1, conf_len, conf);
    struct json_object *jobj = json_tokener_parse(buf);
    strncpy(rabbitmq_hostname,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_hostname")),
            16);
    rabbitmq_port =
            json_object_get_int(json_object_object_get(jobj, "rabbitmq_port"));
    strncpy(rabbitmq_vhost,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_vhost")),
            16);
    rabbitmq_channel = json_object_get_int(
            json_object_object_get(jobj, "rabbitmq_channel"));
    strncpy(rabbitmq_exchange,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_exchange")),
            16);
    strncpy(rabbitmq_exchange_type,
            json_object_get_string(
                    json_object_object_get(jobj, "rabbitmq_exchange_type")),
            16);
}

static int rabbitmq_rpc_publish(amqp_bytes_t reply_to_queue,
                                const char *message_body,
                                amqp_bytes_t correlation_id) {
    // send the message

    {
        // set properties

        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                       AMQP_BASIC_DELIVERY_MODE_FLAG |
                       AMQP_BASIC_CORRELATION_ID_FLAG;
        props.content_type = amqp_cstring_bytes("string");
        props.delivery_mode = 2; /* persistent delivery mode */
        props.reply_to = amqp_bytes_malloc_dup(reply_to_queue);
        if (props.reply_to.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 1;
        }
        props.correlation_id = correlation_id;

        // publish

        die_on_error(
                amqp_basic_publish(conn, rabbitmq_channel,
                                   amqp_cstring_bytes(""), reply_to_queue, 0, 0,
                                   &props, amqp_cstring_bytes(message_body)),
                "Publishing");

        amqp_bytes_free(props.reply_to);
    }
}

static bool check(const char *hashname, int len) {
    char path[len + 5];
    sprintf(path, "/fs/");
    strncat(path, hashname, len);
    path[len + 4] = '\0';
    struct stat st;
    if (stat(path, &st) == -1 || path[strlen(path) - 1] == '/' ||
        (S_ISDIR(st.st_mode)))
        return false;
    else
        return true;
}

void *filecheck_server(void *arg) {
    listenbegin();

    for (;;) {
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;

        amqp_maybe_release_buffers(conn);

        res = amqp_consume_message(conn, &envelope, NULL, 0);

        if (AMQP_RESPONSE_NORMAL != res.reply_type) {
            break;
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
               (int)envelope.consumer_tag.len,
               (char *)envelope.consumer_tag.bytes,
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
        if (check(envelope.message.body.bytes, envelope.message.body.len)) {
            rabbitmq_rpc_publish(envelope.message.properties.reply_to,
                                 "192.168.7.181:8000",
                                 envelope.message.properties.correlation_id);
        } else {
            rabbitmq_rpc_publish(envelope.message.properties.reply_to, "0",
                                 envelope.message.properties.correlation_id);
        }

        amqp_destroy_envelope(&envelope);

        printf("done\n\n");
        amqp_basic_ack(conn, 1, envelope.delivery_tag, 0);
    }

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                      "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    return 0;
}