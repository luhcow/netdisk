#include "rpc_sending.h"

amqp_connection_state_t rabbitmq_connect_server(
    const char *hostname, const int port, const char *vhost,
    amqp_channel_t channel) {
    amqp_connection_state_t conn;
    conn = amqp_new_connection();
    amqp_socket_t *socket = NULL;
    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        die("creating TCP socket");
    }

    int status = amqp_socket_open(socket, hostname, port);
    if (status) {
        die("opening TCP socket");
    }

    die_on_amqp_error(
        amqp_login(conn, vhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                   "guest", "guest"),
        "Logging in");
    amqp_channel_open(conn, channel);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    return conn;
}

amqp_bytes_t rabbitmq_rpc_publisher_declare(
    amqp_connection_state_t conn, amqp_channel_t channel,
    const char *exchange, const char *type) {
    amqp_bytes_t reply_to_queue;
    if (type == NULL) {
        const char *type1 = "topic";
        type = type1;
    }
    amqp_exchange_declare_ok_t *r = amqp_exchange_declare(
        conn, channel, amqp_cstring_bytes(exchange),
        amqp_cstring_bytes(type), 0, 1, 0, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    // return amqp_empty_bytes;
    // create private reply_to queue
    {
        amqp_queue_declare_ok_t *r =
            amqp_queue_declare(conn, channel, amqp_empty_bytes, 0, 0,
                               1, 1, amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn),
                          "Declaring queue");
        reply_to_queue = amqp_bytes_malloc_dup(r->queue);
        if (reply_to_queue.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return amqp_empty_bytes;
        }
    }

    return reply_to_queue;
}

int rabbitmq_rpc_publish(amqp_connection_state_t conn,
                         amqp_channel_t channel, const char *exchange,
                         amqp_bytes_t reply_to_queue,
                         const char *routing_key,
                         const char *message_body) {
    /*
   send the message
*/

    {
        /*
          set properties
        */
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                       AMQP_BASIC_DELIVERY_MODE_FLAG |
                       AMQP_BASIC_REPLY_TO_FLAG |
                       AMQP_BASIC_CORRELATION_ID_FLAG;
        props.content_type = amqp_cstring_bytes("application/json");
        props.delivery_mode = 2; /* persistent delivery mode */
        props.reply_to = amqp_bytes_malloc_dup(reply_to_queue);
        if (props.reply_to.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 1;
        }
        props.correlation_id = amqp_cstring_bytes("1");

        /*
          publish
        */
        die_on_error(amqp_basic_publish(
                         conn, channel, amqp_cstring_bytes(exchange),
                         amqp_cstring_bytes(routing_key), 0, 0,
                         &props, amqp_cstring_bytes(message_body)),
                     "Publishing");

        amqp_bytes_free(props.reply_to);
    }
}

amqp_bytes_t rabbitmq_rpc_wait_answer(amqp_connection_state_t conn,
                                      amqp_channel_t channel,
                                      amqp_bytes_t reply_to_queue) {
    /*
     wait an answer
   */

    {
        amqp_basic_consume(conn, channel, reply_to_queue,
                           amqp_empty_bytes, 0, 1, 0,
                           amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

        {
            amqp_frame_t frame;
            int result;

            amqp_basic_deliver_t *d;
            amqp_basic_properties_t *p;
            size_t body_target;
            size_t body_received;

            for (;;) {
                amqp_maybe_release_buffers(conn);
                result = amqp_simple_wait_frame(conn, &frame);
                if (result < 0) {
                    break;
                }

                printf("Frame type: %u channel: %u\n",
                       frame.frame_type, frame.channel);
                if (frame.frame_type != AMQP_FRAME_METHOD) {
                    continue;
                }

                printf("Method: %s\n",
                       amqp_method_name(frame.payload.method.id));
                if (frame.payload.method.id !=
                    AMQP_BASIC_DELIVER_METHOD) {
                    continue;
                }

                d = (amqp_basic_deliver_t *)
                        frame.payload.method.decoded;
                printf(
                    "Delivery: %u exchange: %.*s "
                    "routingkey: %.*s\n",
                    (unsigned)d->delivery_tag, (int)d->exchange.len,
                    (char *)d->exchange.bytes,
                    (int)d->routing_key.len,
                    (char *)d->routing_key.bytes);

                result = amqp_simple_wait_frame(conn, &frame);
                if (result < 0) {
                    break;
                }

                if (frame.frame_type != AMQP_FRAME_HEADER) {
                    fprintf(stderr, "Expected header!");
                    abort();
                }
                p = (amqp_basic_properties_t *)
                        frame.payload.properties.decoded;
                if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
                    printf("Content-type: %.*s\n",
                           (int)p->content_type.len,
                           (char *)p->content_type.bytes);
                }
                printf("----\n");

                body_target =
                    (size_t)frame.payload.properties.body_size;
                body_received = 0;

                while (body_received < body_target) {
                    result = amqp_simple_wait_frame(conn, &frame);
                    if (result < 0) {
                        break;
                    }

                    if (frame.frame_type != AMQP_FRAME_BODY) {
                        fprintf(stderr, "Expected body!");
                        abort();
                    }

                    body_received += frame.payload.body_fragment.len;
                    assert(body_received <= body_target);

                    amqp_dump(frame.payload.body_fragment.bytes,
                              frame.payload.body_fragment.len);
                }

                if (body_received != body_target) {
                    /* Can only happen when
                     * amqp_simple_wait_frame returns <= 0
                     */
                    /* We break here to close the connection
                     */
                    break;
                }

                /* everything was fine, we can quit now
                 * because we received the reply */
                amqp_bytes_t answer = amqp_bytes_malloc_dup(
                    frame.payload.body_fragment);
                if (answer.bytes == NULL) {
                    fprintf(stderr,
                            "Out of memory while copying queue name");
                    return amqp_empty_bytes;
                }
                // TODO Warning 可能的内存泄露 查阅文档以确认问题
                // amqp_bytes_free(frame.payload.body_fragment);
                return answer;
                break;
            }
        }
    }
    return amqp_empty_bytes;
}

int rabbitmq_close(amqp_connection_state_t conn,
                   amqp_channel_t channel) {
    die_on_amqp_error(
        amqp_channel_close(conn, channel, AMQP_REPLY_SUCCESS),
        "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    return 0;
}