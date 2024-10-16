#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "authorization.h"
#include "filecheck.h"
#include "range.h"

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request(int);
void bad_request(int);
void cat(int, int, long);

void error_die(const char *);

int get_line(int, char *, int);
long headers(int, const char *);
void not_found(int);
void serve_file(int, const char *, const char *);
int startup(u_short *);
void unimplemented(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client) {
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    char *query_string = NULL;

    /*得到请求的第一行*/
    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    j = 0;
    /*把客户端的请求方法存到 method 数组*/
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    /*如果既不是 GET 又不是 POST 则无法处理 */
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
        unimplemented(client);
        return;
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

    /*格式化 url 到 path 数组，html 文件都在 htdocs 中*/
    sprintf(path, "/fs%s", url);
    if (stat(path, &st) == -1 || path[strlen(path) - 1] == '/' ||
        S_ISDIR(st.st_mode)) {
        /*把所有 headers 的信息都丢弃*/
        while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        /*回应客户端找不到*/
        not_found(client);
    } else {
        serve_file(client, path, method);
    }

    /*断开与客户端的连接（HTTP 特点：无连接）*/
    close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client) {
    char buf[1024];

    /*回应客户端错误的 HTTP 请求 */
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, int resource, long len) {
    sendfile(client, resource, 0, len);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc) {
    /*出错信息处理 */
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    /*把终止条件统一为 \n 换行符，标准化 buf 数组*/
    while ((i < size - 1) && (c != '\n')) {
        /*一次仅接收一个字节*/
        n = recv(sock, &c, 1, 0);
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
long headers(int client, const char *filename) {
    char buf[1024];
    struct stat st;
    stat(filename, &st);
    /*正常的 HTTP header */
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Length: %ld\r\n", st.st_size);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Accept-Ranges: bytes\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: application/octet-stream\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    return st.st_size;
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client) {
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename, const char *method) {
    int numchars = 1;
    char buf[1024];
    char range_str[1024] = {'\0'};
    char authorization[1024] = {'\0'};

    /*读取并丢弃 header */
    buf[0] = 'A';
    buf[1] = '\0';
    char *str = NULL;
    while ((numchars > 0) && strcmp("\n", buf)) {
        if ((strncasecmp(buf, "Range: ", strlen("Range: "))) == 0) {
            strcpy(range_str, str + strlen("Range: "));
        }
        if ((str = strncasecmp(buf, "Authorization: ",
                               strlen("Authorization: ")) != NULL)) {
            strcpy(authorization, str + strlen("Authorization: "));
        }
        numchars = get_line(client, buf, sizeof(buf));
    }
    if (authorization[0] == '0' || !decode_jwt(authorization, NULL, NULL)) {
        not_found(client);
        return;
    }
    long content_length;
    /*打开 sever 的文件*/
    int resource = open(filename, O_RDONLY);
    if (resource <= 0)
        not_found(client);
    else {
        if (range_str[0] == '\0') {
            content_length = headers(client, filename);
            if (strncasecmp(method, "GET", 3) == 0) {
                cat(client, resource, content_length);
            }
        } else if (strncasecmp(method, "GET", 3) == 0) {
            range(client, range_str, filename, resource, method);
        }
    }
    close(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port) {
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
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    /*如果当前指定端口是 0，则动态随机分配一个端口*/
    if (*port == 0) /* if dynamically allocating a port */
    {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
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
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void) {
    int server_sock = -1;
    u_short port = 0;
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    pthread_t newthread;
    pthread_t checkthread;
    pthread_create(&checkthread, NULL, filecheck_server, NULL);
    /*在对应端口建立 httpd 服务*/
    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1) {
        /*套接字收到客户端连接请求*/
        client_sock = accept(server_sock, (struct sockaddr *)&client_name,
                             &client_name_len);
        if (client_sock == -1)
            error_die("accept");
        /*派生新线程用 accept_request 函数处理新请求*/
        /* accept_request(client_sock); */
        if (pthread_create(&newthread, NULL, accept_request, client_sock) != 0)
            perror("pthread_create");
    }

    close(server_sock);

    return (0);
}
