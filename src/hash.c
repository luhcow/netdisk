#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "l8w8jwt/claim.h"
#include "l8w8jwt/encode.h"

char username[] = "admin";
char password[] =
    "884c5d7f9de8d072f05636aab3af8e8c7ebe790f4b4f965561a3b847b39bfa24";
time_t pwd_ts = 1725947910;
// 数据库占位符

int encode_jwt(struct json_object* jobj, char* jwt[], size_t* jwt_length,
               char user_name[], time_t pwd_ts) {
    char pwd_ts_str[100];
    sprintf(pwd_ts_str, "%ld", pwd_ts);

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    struct l8w8jwt_claim claims[2];  // 数据库占位符
    claims[0].type = L8W8JWT_CLAIM_TYPE_STRING;
    claims[0].key = "username";
    claims[0].key_length = strlen("username");
    claims[0].value = user_name;
    claims[0].value_length = strlen(user_name);
    claims[1].type = L8W8JWT_CLAIM_TYPE_INTEGER;
    claims[1].key = "pwd_ts";
    claims[1].key_length = strlen("pwd_ts");
    claims[1].value = pwd_ts_str;
    claims[1].value_length = strlen(pwd_ts_str);

    params.additional_payload_claims = claims;
    params.additional_payload_claims_count = 2;

    params.alg = L8W8JWT_ALG_HS256;

    params.iat = l8w8jwt_time(NULL);
    params.exp =
        l8w8jwt_time(NULL) +
        259200; /* Set to expire after 10 minutes (600 seconds). 3days*/
    params.nbf = params.iat;

    params.secret_key = (unsigned char*)"iJOIIIJIhidsfioe7837483HUHUHUuhuh";
    params.secret_key_length = strlen(params.secret_key);

    params.out = jwt;
    params.out_length = jwt_length;

    int r = l8w8jwt_encode(&params);

    // printf("\n l8w8jwt example HS256 token: %s \n",
    //        r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");

    /* Always free the output jwt string! */
    // l8w8jwt_free(jwt);

    return 0;
}

int main(void) {
    char json_buffer[1024];
    char send_json[1024];
    char* content_length = getenv("CONTENT_LENGTH");
    int len = atoi(content_length);
    // printf("Content-Length: %d\r\n", len);
    fread(json_buffer, 1, len, stdin);
    json_buffer[len] = '\0';

    struct json_object* jobj = json_tokener_parse(json_buffer);
    char* user_name =
        json_object_get_string(json_object_object_get(jobj, "username"));
    char* pass_word =
        json_object_get_string(json_object_object_get(jobj, "password"));

    char* jwt;
    size_t jwt_length;
    time_t pwd_ts = 1725947910;
    encode_jwt(jobj, &jwt, &jwt_length, user_name, pwd_ts);

    FILE* fp = fopen("hash.txt", "w");
    fputs(jwt, fp);
    fclose(fp);

    sprintf(send_json,
            "{\"code\": 200,\"message\": \"success\",\"data\": "
            "{\"token\":\"%s\"}}",
            jwt);

    // if (strcmp(user_name, username) == 0 && strcmp(password, password) == 0)
    // {
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("Content-Type: application/json; charset=utf-8\r\n");
    printf("Content-Length: %ld\r\n", strlen(send_json));
    printf("\r\n");
    fwrite(send_json, 1, strlen(send_json), stdout);
    // } else {
    //     printf("Access-Control-Allow-Origin: *\r\n");
    //     printf("Content-Type: application/json; charset=utf-8\r\n");
    //     printf("Content-Length: 58\r\n");
    //     printf("\r\n");
    //     printf("%s  %d\r\n", json_buffer, len);
    //     printf(
    //         "{\"code\":400,\"message\":\"password is "
    //         "incorrect\",\"data\":null}");
    // }
    free(jwt);
    json_object_put(jobj);
}