#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BOUNDARY_SIZE 100

int main(void) {
    // 获取请求方法
    char* request_method = getenv("REQUEST_METHOD");
    perror(request_method);

    if (request_method != NULL && strcmp(request_method, "PUT") == 0) {
        // 获取 CONTENT_LENGTH 环境变量
        const char* content_length_str = getenv("CONTENT_LENGTH");
        const int content_length =
            content_length_str ? atoi(content_length_str) : 0;
        fprintf(stderr, "content_length: %d\n", content_length);
        const char* content_type = getenv("CONTENT_TYPE");
        if (content_type == NULL) {
            // TODO
        }
        perror(content_type);
        const char* file_path = getenv("FILE_PATH");
        if (file_path == NULL) {
            // TODO
        }
        perror(file_path);
        // 读取 POST 数据
        char* post_data = malloc(content_length + 1);
        if (post_data) {
            // long read_size = fread(post_data, 1, content_length, stdin);
            // fprintf(stderr, "read: %lld\n", read_size);
            for (int i = 0; i < content_length; i++) {
                read(STDIN_FILENO, &post_data[i], 1);
                fprintf(stderr, "readed: %d\n", i);
            }
            post_data[content_length] = '\0';
            perror(post_data);
            FILE* fpp = fopen("/home/rings/tinyhttpd-0.1.0/file/t", "wb");
            fwrite(post_data, 1, content_length, fpp);
            fclose(fpp);
            // 查找 multipart/form-data 的边界
            const char* boundary = strstr(post_data, "\r\n");
            if (boundary) {
                char boundary_str[BOUNDARY_SIZE];
                strncpy(boundary_str, post_data, boundary - post_data);
                boundary_str[boundary - post_data] = '\0';  // 边界字符串
                fprintf(stderr, "boundeary: %s\n", boundary_str);
                // 找出来filename
                char* filename_start = strstr(post_data, "filename=\"") + 10;
                // 文件名的开始
                char* filename_end =
                    strstr(filename_start, "\"");  // 文件名的结束
                char path[1024] = "./file";        // 相对路径
                strcat(path, file_path);
                // rcat(path, getenv("URL_PATH"));
                //*(strrchr(path, '/') + 1) = '\0'; //删掉cgi的名字
                // strncat(path, filename_start,
                //         filename_end - filename_start); //上传路径
                char filename[1024] = "";
                strncpy(filename, filename_start,
                        filename_end - filename_start);
                // char* new_urlpath = path + 8; // ./htdocs/ --> /
                strcat(path, filename);
                // 查找文件数据
                char* file_start = strstr(post_data, "\r\n\r\n") + 4;
                // 文件内容的起始位置
                perror(file_start);
                // 发来的文件未使用 base64 但是 multipart/form-data 使用
                // 字符串分隔内容, 文件中的 0 会意外截断字符串, memmem()
                // 需要 #define _GNU_SOURCE 在所有 include 之前
                char* file_end =
                    memmem(file_start, &post_data[content_length] - file_start,
                           boundary_str, strlen(boundary_str));
                // 文件内容的结束位置
                perror(file_end);
                long file_size = file_end - file_start;
                fprintf(stderr, "file_size: %ld\n", file_size);
                perror(path);
                // 保存图片到服务器
                FILE* file = fopen(path, "wb");
                if (file) {
                    fwrite(file_start, 1, file_size, file);
                    fclose(file);
                    char send_json[1024];
                    sprintf(send_json,
                            "{\"code\": 200,\"message\": \"success\",\"data\": "
                            "{\"task\": {}}}");

                    // if (strcmp(user_name, username) == 0 &&
                    // strcmp(password, password) == 0)
                    // {
                    printf("Access-Control-Allow-Origin: *\r\n");
                    printf(
                        "Content-Type: application/json; "
                        "charset=utf-8\r\n");
                    printf("Content-Length: %ld\r\n", strlen(send_json));
                    printf("\r\n");
                    fwrite(send_json, 1, strlen(send_json), stdout);
                } else {
                    char* error_token =
                        "{\"code\":401,\"message\":\"Failed to find "
                        "multipart "
                        "boundary\",\"data\":null}";

                    printf("Access-Control-Allow-Origin: *\r\n");
                    printf(
                        "Content-Type: application/json; "
                        "charset=utf-8\r\n");
                    printf("Content-Length: %ld\r\n", strlen(error_token));
                    printf("\r\n");

                    fputs(error_token, stdout);
                }
            } else {
                char* error_token =
                    "{\"code\":401,\"message\":\"Failed to find multipart "
                    "boundary\",\"data\":null}";

                printf("Access-Control-Allow-Origin: *\r\n");
                printf("Content-Type: application/json; charset=utf-8\r\n");
                printf("Content-Length: %ld\r\n", strlen(error_token));
                printf("\r\n");

                fputs(error_token, stdout);
            }

            free(post_data);
        } else {
            char* error_token =
                "{\"code\":401,\"message\":\"Failed to find multipart "
                "boundary\",\"data\":null}";

            printf("Access-Control-Allow-Origin: *\r\n");
            printf("Content-Type: application/json; charset=utf-8\r\n");
            printf("Content-Length: %ld\r\n", strlen(error_token));
            printf("\r\n");

            fputs(error_token, stdout);
        }
    } else {
        char* error_token =
            "{\"code\":401,\"message\":\"This script only handles PUT "
            "requests.\",\"data\":null}";

        printf("Access-Control-Allow-Origin: *\r\n");
        printf("Content-Type: application/json; charset=utf-8\r\n");
        printf("Content-Length: %ld\r\n", strlen(error_token));
        printf("\r\n");

        fputs(error_token, stdout);
    }

    return 0;
}