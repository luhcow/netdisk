// 如果请求的范围无效（如起始字节超出文件大小），应该返回416 Range Not
// Satisfiable状态码
#include "range.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

int range(int client, const char* range, const char* file_path, int file,
          const char* method) {
    char buf[1024];
    struct stat st;
    fstat(file, &st);
    long file_size = st.st_size;

    long start = 0;
    long end = file_size - 1;
    int is_partial = 0;

    // 如果存在 Range 头，解析范围
    if (range != NULL && strncmp(range, "bytes=", 6) == 0) {
        range += 6;
        char* dash = strchr(range, '-');
        if (dash) {
            *dash = '\0';
            start = atol(range);
            if (dash + 1 && *(dash + 1) != '\0') {
                end = atol(dash + 1);
            }
            if (start > end || start >= file_size) {
                sprintf(buf, "HTTP/1.0 416 Range Not Satisfiable\r\n");
                send(client, buf, strlen(buf), 0);
                sprintf(buf, "Content-Range: bytes */%ld\r\n", file_size);
                send(client, buf, strlen(buf), 0);
                close(file);
                return 1;
            }
            is_partial = 1;
        }
    }

    // 计算需要发送的字节数
    long content_length = end - start + 1;

    // 设置响应头
    if (is_partial) {
        sprintf(buf, "HTTP/1.0 206 Partial Content\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Range: bytes %ld-%ld/%ld\r\n", start, end,
                file_size);
        send(client, buf, strlen(buf), 0);
    } else {
        sprintf(buf, "HTTP/1.0 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
    }
    sprintf(buf, "Content-Type: application/octet-stream\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Length: %ld\r\n", content_length);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Accept-Ranges: bytes\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Disposition: attachment; filename=\"%s\"\r\n",
            file_path);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sendfile(client, file, start, content_length);

    // 关闭文件
    fclose(file);

    return 0;
}
// Content-Range: 指定返回的字节范围，例如Content-Range: bytes
// 500-999/2000，表示总长度为2000字节，返回了500-999字节的部分。 Accept-Ranges:
// bytes: 告诉客户端服务器支持字节范围请求。 Content-Disposition:
// 确保文件是以附件形式下载，而不是直接显示。