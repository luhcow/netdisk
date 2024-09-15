#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char settings_conf[] = "./src/settings.json";

int main(void) {
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

   return 0;
}