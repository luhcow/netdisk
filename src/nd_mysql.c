#include "nd_mysql.h"

#include <mysql/mysql.h>

_Thread_local MYSQL mysql;
_Thread_local MYSQL *res;
_Thread_local MYSQL_ROW row;

int mysql_beginwork() {
    // 初始化句柄mysql
    if (mysql_init(&mysql) == NULL) {
        printf("%d : error : %s\n", __LINE__, mysql_error(&mysql));
        exit(1);
    }
    // 与mysql服务器建立连接
    if (NULL ==
        mysql_real_connect(
                &mysql,
                "database-1.cbmwmqg8413g.ap-southeast-1.rds.amazonaws.com",
                "admin", "RuQ89syXgeMNQcFzMfZi", "test", 0, NULL, 0)) {
        printf("%d : error : %s\n", __LINE__, mysql_error(&mysql));
        exit(1);
    }
    // 设置中文字符集
    if (mysql_set_character_set(&mysql, "utf8") != 0) {
        printf("%d : error : %s\n", __LINE__, mysql_error(&mysql));
        exit(1);
    }
    printf("connect success!\n");
}

int mysql_endwork() {
    mysql_close(&mysql);
}