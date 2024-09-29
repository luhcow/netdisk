#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "authorization.h"
#include "json-c/json.h"

char username[] = "admin";
char password[] =
    "884c5d7f9de8d072f05636aab3af8e8c7ebe790f4b4f965561a3b847b39bfa24";
time_t pwd_ts = 1725947910;
// char KEY[] = "iJOIIIJIhidsfioe7837483HUHUHUuhuh";
const char settings_conf[] = "./src/get.json";  // 数据库占位符

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
    const char* path =
        json_object_get_string(json_object_object_get(jobj, "path"));
    char dir_path[4096] = "./htdocs";
    strcat(dir_path, path);

    struct dirent* entry;
    DIR* dp = opendir(dir_path);
    if (dp == NULL) {
        // TODO;
    }
    entry = readdir(dp);

    struct stat st;
    stat(dir_path, &st);

    struct json_object* data_json = json_object_new_object();
    json_object_object_add(data_json, "name", json_object_new_string(dir_path));
    json_object_object_add(data_json, "size",
                           json_object_new_int64(st.st_size));
    json_object_object_add(data_json, "is_dir",
                           json_object_new_boolean(S_ISDIR(st.st_mode)));
    json_object_object_add(data_json, "modified",
                           json_object_new_string(ctime(&st.st_mtime)));
    json_object_object_add(data_json, "created",
                           json_object_new_string(ctime(&st.st_ctime)));
    json_object_object_add(data_json, "sign",
                           json_object_new_string(""));  // 签名
    json_object_object_add(data_json, "thumb",
                           json_object_new_string(""));  // 缩略图
    json_object_object_add(
        data_json, "type",
        json_object_new_int(0));  // TODO 根据文件的后缀生成类型
    json_object_object_add(data_json, "hashinfo",
                           json_object_new_string("null"));
    json_object_object_add(data_json, "hash_info",
                           json_object_new_null());  // TODO 以后开发
    json_object_object_add(
        data_json, "raw_url",
        json_object_new_string(
            path));  // TODO 直达该文件的路径，暂时使用真实路径
    json_object_object_add(data_json, "readme", json_object_new_string(""));
    json_object_object_add(data_json, "header", json_object_new_string(""));
    json_object_object_add(data_json, "provider",
                           json_object_new_string("unknown"));  // 扩展用
    json_object_object_add(data_json, "related",
                           json_object_new_null());  // TODO 以后开发
    struct json_object* send_json = json_object_new_object();
    json_object_object_add(send_json, "code", json_object_new_int(200));
    json_object_object_add(send_json, "message",
                           json_object_new_string("success"));
    json_object_object_add(send_json, "data", data_json);
    const char* send_data = json_object_to_json_string(send_json);

    printf("Access-Control-Allow-Origin: *\r\n");
    printf("Content-Type: application/json; charset=utf-8\r\n");
    printf("Content-Length: %ld\r\n", strlen(send_data));
    printf("\r\n");

    fputs(send_data, stdout);

    json_object_put(send_json);

    return 0;
}
