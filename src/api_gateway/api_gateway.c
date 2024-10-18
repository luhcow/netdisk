
#include "api_gateway/api_gateway.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "api_gateway/rpc_sending.h"
#include "public/authorization.h"
#include "public/http.h"
#include "public/uthash.h"

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: netfile/0.1.0\r\n"

_Thread_local amqp_connection_state_t conn;
_Thread_local amqp_bytes_t reply_to;

char rabbitmq_hostname[16] = "52.77.251.3";
int rabbitmq_port = 5672;
char rabbitmq_vhost[16] = "/";
amqp_channel_t rabbitmq_channel = 1;
char rabbitmq_exchange[16] = "gateway";
char rabbitmq_exchange_type[16] = "topic";

int api_gateway_prework(void) {
    // 建立连接
    conn = rabbitmq_connect_server(rabbitmq_hostname, rabbitmq_port,
                                   rabbitmq_vhost, rabbitmq_channel);

    // 声明队列
    reply_to = rabbitmq_rpc_publisher_declare(conn, rabbitmq_channel,
                                              rabbitmq_exchange,
                                              rabbitmq_exchange_type);

    return 0;
}

int api_gateway_endwork(void) {
    // 断开连接
    rabbitmq_close(conn, rabbitmq_channel);

    return 0;
}

int fs_handler(int client, request_t request) {
    // TODO 检查一下请求是否合法 比如方法
    // fs 应该都是要检查 token 的
    struct str_map* s;
    HASH_FIND(hh, request.heads, "authorization",
              strlen("authorization"), s);
    if (!decode_jwt(s->value, NULL, NULL)) {
        free(request.body);
        request.body = bad_json("need login");
        request.content_length = strlen(request.body);
    } else {
        // 核验一下 token 合法了，愉快开跑
        // 注意上传也在这里 可能需要特殊处理一下 先按小文件
        // 直接塞进消息里或者base64
        remote_procedure_call(&request);
    }
    headers_json(client, request.content_length);
    return send(client, request.body, request.content_length,
                MSG_NOSIGNAL);
}

int auth_handler(int client, request_t request) {
    // 唯一的区别是不用检查授权，我写这玩意儿到底有啥用？
    // 想想自己还写了个路由，真的有必要吗
    // TODO
    remote_procedure_call(&request);

    headers_json(client, request.content_length);
    return send(client, request.body, request.content_length,
                MSG_NOSIGNAL);
}

/// @brief 把消息发送到 中间件 然后阻塞等待消息发过来 JSON 格式
/// 但是发收都是整个字符串 gateway不解码
/// @param api Url 去除了前面的部分 eg: fs/move 将会在函数内部变为
/// fs.move
/// @param message parser 的返回值 包括 HTTP 请求消息主体和 有用的标头
/// @return http 响应消息主体和长度
int remote_procedure_call(request_t* request) {
    char routing_key[strlen(request->uri_residue) + 1];
    strcpy(routing_key, request->uri_residue);
    char* temp;
    while ((temp = strchr(routing_key, '/')) != NULL) *temp = '.';

    amqp_bytes_t message_body;
    message_body.bytes = request->body;
    message_body.len = request->content_length;

    rabbitmq_rpc_publish(conn, rabbitmq_channel, rabbitmq_exchange,
                         reply_to, routing_key, message_body);

    amqp_bytes_t answer = rabbitmq_rpc_wait_answer(
            conn, rabbitmq_channel, reply_to);

    free(request->body);
    request->body = malloc(answer.len * sizeof(char));
    request->content_length = answer.len;
    strncpy(request->body, answer.bytes, request->content_length);

    amqp_bytes_free(answer);

    return 0;
}

char* bad_json(const char* mess) {
    char error_token[60];
    sprintf(error_token,
            "{\"code\":401,\"message\":\"%s\",\"data\":null}", mess);

    char temp[255];
    sprintf(temp,
            "Access-Control-Allow-Origin: *\r\nContent-Type: "
            "application/json; charset=utf-8\r\nContent-Length: "
            "%ld\r\n\r\n%s",
            strlen(error_token), error_token);

    char* re_str = malloc((strlen(temp) + 1) * sizeof(char));
    strncpy(re_str, temp, (strlen(temp) + 1));
    return re_str;
}

void headers_json(int client, long len) {
    char buf[1024];
    /*正常的 HTTP header */
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Access-Control-Allow-Origin: *\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Content-Type: application/json; charset=utf-8\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Length: %ld\r\n", len);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}
