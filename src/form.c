#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOUNDARY_SIZE 100

int main(void) {
    // 获取请求方法
    char* request_method = getenv("REQUEST_METHOD");

    if (request_method != NULL && strcmp(request_method, "POST") == 0) {
        // 获取 CONTENT_LENGTH 环境变量
        const char* content_length_str = getenv("CONTENT_LENGTH");
        const int content_length =
            content_length_str ? atoi(content_length_str) : 0;
        const char* content_type = getenv("CONTENT_TYPE");
        if (content_type == NULL) {
            // TODO
        }
        const char* file_path = getenv("FILE_PATH");
        if (file_path == NULL) {
            // TODO
        }
        // 读取 POST 数据
        char* post_data = malloc(content_length + 1);
        if (post_data) {
            fread(post_data, 1, content_length, stdin);
            post_data[content_length] = '\0';

            // 查找 multipart/form-data 的边界
            const char* boundary = strstr(post_data, "\r\n");
            if (boundary) {
                char boundary_str[BOUNDARY_SIZE];
                strncpy(boundary_str, post_data, boundary - post_data);
                boundary_str[boundary - post_data] = '\0';  // 边界字符串
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

                // 查找文件数据
                char* file_start = strstr(post_data, "\r\n\r\n") + 4;
                // 文件内容的起始位置
                char* file_end = strstr(file_start, boundary_str) - 2;
                // 文件内容的结束位置
                long file_size = file_end - file_start;

                // 保存图片到服务器
                FILE* file = fopen(path, "wb");
                if (file) {
                    fwrite(file_start, 1, file_size, file);
                    fclose(file);
                    char send_json[1024];
                    sprintf(send_json,
                            "{\"code\": 200,\"message\": \"success\",\"data\": "
                            "{\"task\": NULL}}");

                    // if (strcmp(user_name, username) == 0 && strcmp(password,
                    // password) == 0)
                    // {
                    printf("Access-Control-Allow-Origin: *\r\n");
                    printf("Content-Type: application/json; charset=utf-8\r\n");
                    printf("Content-Length: %ld\r\n", strlen(send_json));
                    printf("\r\n");
                    fwrite(send_json, 1, strlen(send_json), stdout);
                } else {
                }
            } else {
                printf("<h1>Failed to find multipart boundary.</h1>\n");
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
            "{\"code\":401,\"message\":\"This script only handles POST "
            "requests.\",\"data\":null}";

        printf("Access-Control-Allow-Origin: *\r\n");
        printf("Content-Type: application/json; charset=utf-8\r\n");
        printf("Content-Length: %ld\r\n", strlen(error_token));
        printf("\r\n");

        fputs(error_token, stdout);
    }

    return 0;
}