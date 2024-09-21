#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "authorization.h"
#include "l8w8jwt/claim.h"
#include "l8w8jwt/decode.h"

char username[] = "admin";
char password[] =
    "884c5d7f9de8d072f05636aab3af8e8c7ebe790f4b4f965561a3b847b39bfa24";
time_t pwd_ts = 1725947910;
// char KEY[] = "iJOIIIJIhidsfioe7837483HUHUHUuhuh";
const char settings_conf[] = "./src/me.json";
const char exp_char[] =
    "{\"code\":401,\"message\":\"token is "
    "expired\",\"data\":null}";  // 数据库占位符

// bool encode_jwt(char JWT[], char user_name[], time_t pwd_ts,
//                 struct l8w8jwt_claim **out_claims, int out_calims_length) {
//    struct l8w8jwt_decoding_params params;
//    l8w8jwt_decoding_params_init(&params);

//    params.alg = L8W8JWT_ALG_HS256;

//    params.jwt = (char *)JWT;
//    params.jwt_length = strlen(JWT);

//    params.verification_key = (unsigned char *)KEY;
//    params.verification_key_length = strlen(KEY);

//    char pwd_ts_str[100];
//    sprintf(pwd_ts_str, "%ld", pwd_ts);

//    /*
//     * Not providing params.validate_iss_length makes it use strlen()
//     * Only do this when using properly NUL-terminated C-strings!
//     */
//    //    params.validate_iss = "Black Mesa";
//    //    params.validate_sub = "Gordon Freeman";

//    /* Expiration validation set to false here only because the above example
//     * token is already expired! */
//    params.validate_exp = 1;
//    params.exp_tolerance_seconds = 60;

//    params.validate_iat = 1;
//    params.iat_tolerance_seconds = 60;

//    params.validate_nbf = 1;
//    params.nbf_tolerance_seconds = 60;

//    enum l8w8jwt_validation_result validation_result;

//    size_t out_claims_length = 2;
//    int decode_result = l8w8jwt_decode(&params, &validation_result,
//    out_claims,
//                                       &out_calims_length);

//    if (decode_result == L8W8JWT_SUCCESS && validation_result ==
//    L8W8JWT_VALID) {
//       return true;
//    } else {
//       return false;
//    }

//    /*
//     * decode_result describes whether decoding/parsing the token succeeded or
//     * failed; the output l8w8jwt_validation_result variable contains actual
//     * information about JWT signature verification status and claims
//     validation
//     * (e.g. expiration check).
//     *
//     * If you need the claims, pass an (ideally stack pre-allocated) array of
//     * struct l8w8jwt_claim instead of NULL,NULL into the corresponding
//     * l8w8jwt_decode() function parameters. If that array is heap-allocated,
//     * remember to free it yourself!
//     */

//    return 0;
// }

int main(void) {
   char user_name[100];

   char pwd_ts_str[100];
   // sprintf(pwd_ts_str, "%ld", pwd_ts);

   // struct l8w8jwt_claim claims[2];  // 数据库占位符
   // claims[0].type = L8W8JWT_CLAIM_TYPE_STRING;
   // claims[0].key = "username";
   // claims[0].key_length = strlen("username");
   // claims[0].value = user_name;
   // claims[0].value_length = strlen(user_name);
   // claims[1].type = L8W8JWT_CLAIM_TYPE_INTEGER;
   // claims[1].key = "pwd_ts";
   // claims[1].key_length = strlen("pwd_ts");
   // claims[1].value = pwd_ts_str;
   // claims[1].value_length = strlen(pwd_ts_str);  // 数据库占位符

   // FILE *fp = fopen("hash.txt", "r");
   // fseek(fp, 0, SEEK_END);
   // long int jwt_length = ftell(fp);
   // fseek(fp, 0, SEEK_SET);
   // char jwt[jwt_length + 1];
   // fgets(jwt, jwt_length + 1, fp);
   // fclose(fp);

   char jwt[255];
   strcpy(jwt, getenv("AUTHORIZATION"));

   fputs(jwt, stderr);

   struct l8w8jwt_claim out_claims[2];
   struct l8w8jwt_claim *claim = out_claims;
   if (decode_jwt(jwt, username, pwd_ts, &claim, 2)) {
      FILE *fp = fopen(settings_conf, "r");
      if (fp == NULL) {
         printf("open file error\n");
         return -1;
      }
      printf("Access-Control-Allow-Origin: *\r\n");

      fseek(fp, 0, SEEK_END);
      long file_size = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      printf("Content-Type: application/json; charset=utf-8\r\n");
      printf("Content-Length: %ld\r\n", file_size);
      printf("\r\n");

      int n = 0;
      char buffer[1024];
      size_t bytes_read;
      while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
         n++;
         fwrite(buffer, 1, bytes_read, stdout);
      }
      fclose(fp);
   } else {
      char *error_token =
          "{\"code\":401,\"message\":\"Guest user is disabled, login "
          "please\",\"data\":null}";
      printf("Access-Control-Allow-Origin: *\r\n");
      printf("Content-Type: application/json; charset=utf-8\r\n");
      printf("Content-Length: %ld\r\n", strlen(error_token));
      printf("\r\n");

      fputs(error_token, stdout);
   }
}