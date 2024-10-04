#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "authorization.h"
#include "json-c/json.h"

char username[] = "admin";
char password[] =
    "884c5d7f9de8d072f05636aab3af8e8c7ebe790f4b4f965561a3b847b39bfa24";
time_t pwd_ts = 1725947910;
// char KEY[] = "iJOIIIJIhidsfioe7837483HUHUHUuhuh";
const char settings_conf[] = "./src/get.json";  // 数据库占位符

const char accept[] = "{\"names\": [\"string\"], \"dir\": \"string\"}";
const char ok[] =
    "{\"code\":200,\"message\":\"success\",\"data\":null}";  // 示例

void deleteDir(const char* path);

int main() {
    // Cookie:
    char jwt[255];
    strcpy(jwt, getenv("AUTHORIZATION"));
    char* content_length_str = getenv("CONTENT_LENGTH");
    int content_length = content_length_str ? atoi(content_length_str) : 0;

    struct l8w8jwt_claim out_claims[2];
    struct l8w8jwt_claim* claim = out_claims;

    if (!decode_jwt(jwt, username, pwd_ts, &claim, 2)) {
        char* error_token =
            "{\"code\":401,\"message\":\"token is "
            "expired\",\"data\":null}";

        printf("Access-Control-Allow-Origin: *\r\n");
        printf("Content-Type: application/json; charset=utf-8\r\n");
        printf("Content-Length: %ld\r\n", strlen(error_token));
        printf("\r\n");

        fputs(error_token, stdout);
        return 0;
    }

    char* post_data = malloc(content_length + 1);
    if (post_data == NULL) {
        // TODO
    }

    fread(post_data, 1, content_length, stdin);
    post_data[content_length] = '\0';

    struct json_object* jobj = json_tokener_parse(post_data);
    const char* dir =
        json_object_get_string(json_object_object_get(jobj, "dir"));
    const char* name = json_object_get_string(
        json_object_array_get_idx(json_object_object_get(jobj, "names"), 0));

    char path[1024] = "./file";
    strcat(path, dir);
    strcat(path, name);

    struct stat path_stat;
    stat(path, &path_stat);

    if (S_ISDIR(path_stat.st_mode)) {
        deleteDir(path);
    } else {
        unlink(path);
    }
    if (errno != 0) {
        char* error_token =
            "{\"code\":401,\"message\":\"delete false\",\"data\":null}";

        printf("Access-Control-Allow-Origin: *\r\n");
        printf("Content-Type: application/json; charset=utf-8\r\n");
        printf("Content-Length: %ld\r\n", strlen(error_token));
        printf("\r\n");

        return 0;
    }

    char* send_data =
        "{\"code\": 200, \"message\": \"success\", \"data\": null}";

    printf("Access-Control-Allow-Origin: *\r\n");
    printf("Content-Type: application/json; charset=utf-8\r\n");
    printf("Content-Length: %ld\r\n", strlen(send_data));
    printf("\r\n");
    fputs(send_data, stdout);
    return 0;
}

void deleteDir(const char* path) {
    // 打开目录
    DIR* path_dir = opendir(path);
    struct dirent* entry;
    // 遍历目录流，依次删除每一个目录项
    while ((entry = readdir(path_dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        if (S_ISDIR(entry->d_type)) {
            char new_src[255];
            sprintf(new_src, "%s/%s", path, entry->d_name);
            deleteDir(new_src);
        } else {
            char new_src[255];
            sprintf(new_src, "%s/%s", path, entry->d_name);
            unlink(new_src);
        }
    }
    // while ((pdirent = readdir(pdir)) != NULL) {
    //  忽略.和..
    //  如果该目录项是目录，则调用deleteDir递归删除
    //  如果该目录项是文件，则调用unlink删除文件
    //}
    // 关闭目录流
    closedir(path_dir);
    rmdir(path);
    // 目录为空了，可以删除该目录了
    // 目录为空了，可以删除该目录了
}