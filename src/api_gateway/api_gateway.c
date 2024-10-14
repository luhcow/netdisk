
#include "api_gateway.h"

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
    reply_to = rabbitmq_rpc_publisher_declare(
            conn, rabbitmq_channel, rabbitmq_exchange, rabbitmq_exchange_type);
}
int api_gateway_work(int client) {
    return accept_request(client);
}
int api_gateway_endwork(void) {
    // 断开连接
    rabbitmq_close(conn, rabbitmq_channel);
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
int accept_request(int client) {
    char buf[1024];
    char method[255];
    char url[255];
    char path[512];
    struct stat st;
    char* query_string = NULL;

    /*得到请求的第一行*/
    int numchars = get_line(client, buf, sizeof(buf));
    size_t i = 0;
    size_t j = 0;
    /*把客户端的请求方法存到 method 数组*/
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    // 如果既不是 GET 又不是 POST 还不是 ... 则无法处理
    // 这里增加了 PUT 方法，显得有点笨笨的，实际上 POST 带
    // multipart/form-data 主体可以增加很多数据 不过 PUT
    // 方法也有自己的优势
    if (strcasecmp(method, "GET") != 0 && strcasecmp(method, "POST") != 0 &&
        strcasecmp(method, "OPTIONS") != 0 && strcasecmp(method, "PUT") != 0) {
        unimplemented(client);
        return -1;
    }
    // 处理OPTIONS
    if (strcasecmp(method, "OPTIONS") == 0) {
        headers_204(client, path);
        return 0;
    }

    /*读取 url 地址*/
    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf))) j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
        /*存下 url */
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    urldecode(url);  // 解码

    /*处理 GET 方法*/
    if (strcasecmp(method, "GET") == 0) {
        /* 待处理请求为 url */
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        /* GET 方法特点，? 后面为参数*/
        if (*query_string == '?') {
            *query_string = '\0';
            query_string++;
        }
    }

    http_request_t request;
    parser(client, method, &request);
    http_content_t readysend_mess;

    //  /api 表示请求了一个 api
    //  /d 只用来下载文件
    if (strncasecmp("/api", url, 4) == 0) {
        // TODO
        // api 请求 网关 rpc 后，将数据发送至 client
        // Token 等相关计算都在网关完成
        if (strncasecmp("/fs", url + 4, 3) == 0) {
            char* fs_api = url + 4 + 1;
            // TODO 检查一下请求是否合法
            // fs 应该都是要检查 token 的
            if (request.authorization_len == 0 ||
                request.authorization == NULL) {
                readysend_mess.bytes = bad_json("need login");
                readysend_mess.len = strlen(readysend_mess.bytes);
            } else if (!decode_jwt(request.authorization, NULL, 0)) {
                readysend_mess.bytes = bad_json("need login");
                readysend_mess.len = strlen(readysend_mess.bytes);
            } else {
                // 核验一下 token 合法了，愉快开跑
                // 注意上传也在这里 可能需要特殊处理一下 先按小文件
                // 直接塞进消息里或者base64
                readysend_mess = remote_procedure_call(fs_api, &request);
            }
            headers_json(client, readysend_mess.len);
            send(client, readysend_mess.bytes, readysend_mess.len,
                 MSG_NOSIGNAL);
        } else if (strncasecmp("/auth", url + 4, 5) == 0) {
            char* auth_api = url + 4 + 1;
            // auth 有多种情况，需要分类一下
            // 先简单分成三类 login admin sign
            // 但我直接
            // TODO
            readysend_mess = remote_procedure_call(auth_api, &request);
            headers_json(client, readysend_mess.len);
            send(client, readysend_mess.bytes, readysend_mess.len,
                 MSG_NOSIGNAL);
        }
    } else if (strncasecmp("/d", url, 2) == 0) {
        // TODO
        // 客户端要求下载文件，让客户端去找 fs server 下载
        // 先验证 Token 是否合法，RPC 后将新的请求链接发至客户端
        char* d_api = url + 1;
        // TODO 检查一下请求是否合法
        if (request.authorization_len == 0 || request.authorization == NULL) {
            readysend_mess.bytes = bad_json("need login");
            readysend_mess.len = strlen(readysend_mess.bytes);
        } else if (!decode_jwt(request.authorization, NULL, 0)) {
            readysend_mess.bytes = bad_json("need login");
            readysend_mess.len = strlen(readysend_mess.bytes);
        } else {
            // 核验一下 token 合法了，愉快开跑
            readysend_mess = remote_procedure_call(d_api, &request);
        }
        // 这里发的是去哪里下载的 JSON, 上传也可以这么做
        headers_json(client, readysend_mess.len);
        send(client, readysend_mess.bytes, readysend_mess.len, MSG_NOSIGNAL);
    } else {
        // 啥也不是, 发个 404
        not_found(client);
    }

    /*断开与客户端的连接（HTTP 特点：无连接）*/
    close(client);
    return 0;
}

/// @brief 把消息发送到 中间件 然后阻塞等待消息发过来 JSON 格式
/// 但是发收都是整个字符串 gateway不解码
/// @param api Url 去除了前面的部分 eg: fs/move 将会在函数内部变为 fs.move
/// @param message parser 的返回值 包括 HTTP 请求消息主体和 有用的标头
/// @return http 响应消息主体和长度
http_content_t remote_procedure_call(const char* api,
                                     const http_request_t* message) {
    char routing_key[strlen(api) + 1];
    strcpy(routing_key, api);
    char* temp;
    while ((temp = strchr(routing_key, '/')) != NULL) *temp = '.';

    amqp_bytes_t message_body;
    message_body.bytes = message->content.bytes;
    message_body.len = message->content.len;

    rabbitmq_rpc_publish(conn, rabbitmq_channel, rabbitmq_exchange, reply_to,
                         routing_key, message_body);

    amqp_bytes_t answer =
            rabbitmq_rpc_wait_answer(conn, rabbitmq_channel, reply_to);

    http_content_t body;
    body.bytes = malloc(answer.len * sizeof(char));
    body.len = answer.len;
    strncpy(body.bytes, answer.bytes, body.len);

    amqp_bytes_free(answer);

    return body;
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client) {
    char buf[1024];

    /*回应客户端错误的 HTTP 请求 */
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), MSG_NOSIGNAL);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), MSG_NOSIGNAL);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), MSG_NOSIGNAL);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), MSG_NOSIGNAL);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE* resource) {
    int n = 0;
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), resource)) > 0) {
        n++;
        send(client, buffer, bytes_read, MSG_NOSIGNAL);
    }
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char* sc) {
    /*出错信息处理 */
    perror(sc);
    exit(1);
}

char* bad_json(const char* mess) {
    char error_token[60];
    sprintf(error_token, "{\"code\":401,\"message\":\"%s\",\"data\":null}",
            mess);

    // printf("Access-Control-Allow-Origin: *\r\n");
    // printf("Content-Type: application/json; charset=utf-8\r\n");
    // printf("Content-Length: %ld\r\n", strlen(error_token));
    // printf("\r\n");
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

/// @brief 对 HTTP 的 头部进行解析，不包括起始行 (已经被解析)
/// @param client 客户端的 socket 描述符
/// @param method 请求方法
/// @param request 解析出来的内容会存在这个结构体里
/// @return 找不到 content-length 会返回 -1
int parser(int client, const char* method, http_request_t* request) {
    char buf[1024];
    // int cgi_output[2];
    // int cgi_input[2];
    // pid_t pid;
    int status;
    // char c;
    int numchars = 1;
    int content_length = -1;
    char authorization[1024] = "\0";
    char content_type[1024] = "\0";
    char file_path[1024] = "\0";

    buf[0] = 'A';
    buf[1] = '\0';
    /*把所有的 HTTP header 读取并丢弃*/
    if (strcasecmp(method, "GET") == 0) {
        while ((numchars > 0) && strcmp("\n", buf)) {
            /* read & discard headers */
            char* str = NULL;
            if ((str = strstr(buf, "Authorization:")) != NULL) {
                strcpy(authorization, str + strlen("Authorization: "));
            }
            numchars = get_line(client, buf, sizeof(buf));
        }
    } else if (strcasecmp(method, "POST") == 0) {
        /* POST */
        char* str = NULL;  // 找出authorization
        /* 对 POST 的 HTTP 请求中找出 content_length */
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf)) {
            if ((str = strstr(buf, "Authorization:")) != NULL) {
                strcpy(authorization, str + strlen("Authorization: "));
            }
            /*利用 \0 进行分隔 */
            buf[15] = '\0';
            /* HTTP 请求的特点*/
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            // 在这里把头读取完都丢弃了
            numchars = get_line(client, buf, sizeof(buf));
        }
        /*没有找到 content_length */
        if (content_length == -1) {
            /*错误请求*/
            bad_request(client);
            return -1;
        }
    } else if (strcasecmp(method, "PUT") == 0) {
        char* str = NULL;  // 找出authorization
        /* 对 POST 的 HTTP 请求中找出 content_length */
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf)) {
            if ((str = strstr(buf, "Authorization:")) != NULL) {
                strcpy(authorization, str + strlen("Authorization: "));
            }
            if ((str = strstr(buf, "Content-Type:")) != NULL) {
                strcpy(content_type, str + strlen("content-type: "));
            }
            if ((str = strstr(buf, "File-Path:")) != NULL) {
                urldecode(buf);
                *(strchr(buf, '\n')) = '\0';
                strcpy(file_path, str + strlen("file-path: "));
            }
            /*利用 \0 进行分隔 */
            buf[15] = '\0';
            /* HTTP 请求的特点*/
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        /*没有找到 content_length */
        if (content_length == -1) {
            /*错误请求*/
            bad_request(client);
            return -1;
        }
    }
    // 组装 request
    // 没有的内容应该会 strlen = 0, 使用前检查 len 即可
    request->authorization_len = strlen(authorization);
    request->authorization =
            malloc((request->authorization_len + 1) * sizeof(char));
    strncpy(request->authorization, authorization,
            request->authorization_len + 1);
    request->content_type_len = strlen(content_type);
    request->content_type =
            malloc((request->content_type_len + 1) * sizeof(char));
    strncpy(request->content_type, content_type, request->content_type_len + 1);
    request->file_path_len = strlen(file_path);
    request->file_path = malloc((request->file_path_len + 1) * sizeof(char));
    strncpy(request->file_path, file_path, request->file_path_len + 1);
    request->content.len = content_length;
    request->content.bytes = malloc((content_length + 1) * sizeof(char));

    if (strcasecmp(method, "POST") == 0 ||
        strcasecmp(method, "PUT") == 0) /*接收 POST 过来的数据*/
        recv(client, request->content.bytes, content_length, MSG_WAITALL);
    // TODO: 错误处理

    /* 正确，HTTP 状态码 200 */
    // 发送的请求正确就应该回复 200 OK 出现错误在 消息主体 中报告
    // 有助于对用户更加友好
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    // 目前错误处理只有在 content_length 找不到 return -1
    return 1;
}

/// @brief Get a line from a socket, whether the line ends in a
/// newline, carriage return, or a CRLF combination.  Terminates the
/// string read with a null character.  If no newline indicator is
/// found before the end of the buffer, the string is terminated with
/// a null.  If any of the above three line terminators is read, the
/// last character of the string will be a linefeed and the string
/// will be terminated with a null character.
/// @param sock the socket descriptor
/// @param buf the buffer to save the data in
/// @param size the size of the buffer
/// @return the number of bytes stored (excluding null)
int get_line(int sock, char* buf, int size) {
    int i = 0;
    char c = '\0';

    /*把终止条件统一为 \n 换行符，标准化 buf 数组*/
    while ((i < size - 1) && (c != '\n')) {
        /*一次仅接收一个字节*/
        int n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0) {
            /*收到 \r 则继续接收下个字节，因为换行符可能是 \r\n */
            if (c == '\r') {
                /*使用 MSG_PEEK
                 * 标志使下一次读取依然可以得到这次读取的内容，可认为接收窗口不滑动*/
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                /*但如果是换行符则把它吸收掉*/
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            /*存到缓冲区*/
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';

    /*返回 buf 数组大小*/
    return (i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char* filename, FILE* resource) {
    char buf[1024];
    char content_type[100];

    strcpy(content_type, "application/octet-stream");
    /*正常的 HTTP header */
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: %s\r\n", content_type);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    if (strcmp(content_type, "text/html") != 0) {
        fseek(resource, 0, SEEK_END);
        long file_size = ftell(resource);
        fseek(resource, 0, SEEK_SET);
        sprintf(buf,
                "Content-Disposition: attachment; "
                "filename=\"%s\"\r\n",
                filename);
        send(client, buf, strlen(buf), MSG_NOSIGNAL);
        sprintf(buf, "Content-Length: %ld\r\n", file_size);
        send(client, buf, strlen(buf), MSG_NOSIGNAL);
    }
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

void headers_204(int client, const char* filename) {
    char buf[1024];
    strcpy(buf, "HTTP/1.0 204 No Content\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Access-Control-Allow-Headers: *\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Access-Control-Allow-Methods: *\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Access-Control-Allow-Origin: *\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Access-Control-Max-Age: 43200\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

void headers_json(int client, long len) {
    char buf[1024];
    strcpy(buf, "Access-Control-Allow-Origin: *\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "Content-Type: application/json; charset=utf-8\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Length: %ld\r\n", len);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client) {
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);

    FILE* not_found_html = fopen("./htdocs/404.html", "r");
    cat(client, not_found_html);
    fclose(not_found_html);
}

void not_found_debug(int client, const char* path) {
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    send(client, path, strlen(path), MSG_NOSIGNAL);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(unsigned short int* port) {
    int httpd = 0;
    struct sockaddr_in name;

    /*建立 socket */
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr*)&name, sizeof(name)) < 0)
        error_die("bind");
    /*如果当前指定端口是 0，则动态随机分配一个端口*/
    if (*port == 0) {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr*)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    /*开始监听*/
    if (listen(httpd, 5) < 0)
        error_die("listen");
    /*返回 socket id */
    return (httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client) {
    char buf[1024];

    /* HTTP method 不被支持*/
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), MSG_NOSIGNAL);
}
