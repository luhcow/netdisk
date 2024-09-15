#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOUNDARY_SIZE 100

int main(void)
{
    // 设置 Content-type 头，返回HTML内容
    printf("Content-type: text/html; charset=utf-8\n\n");

    // 打印 HTML 页面的头部
    printf("<html><head><title>Image Upload</title></head><body>\n");

    // 获取请求方法
    char* request_method = getenv("REQUEST_METHOD");

    if (request_method != NULL && strcmp(request_method, "POST") == 0)
    {
        // 获取 CONTENT_LENGTH 环境变量
        char* content_length_str = getenv("CONTENT_LENGTH");
        int content_length = content_length_str ? atoi(content_length_str) : 0;

        // 读取 POST 数据
        char* post_data = malloc(content_length + 1);
        if (post_data)
        {
            fread(post_data, 1, content_length, stdin);
            post_data[content_length] = '\0';

            // 查找 multipart/form-data 的边界
            char* boundary = strstr(post_data, "\r\n");
            if (boundary)
            {
                char boundary_str[BOUNDARY_SIZE];
                strncpy(boundary_str, post_data, boundary - post_data);
                boundary_str[boundary - post_data] = '\0'; // 边界字符串
                //找出来filename  
                char* filename_start = strstr(post_data, "filename=\"") + 10;//文件名的开始
                char* filename_end = strstr(filename_start, "\"");//文件名的结束
                char path[1024] = "./";//相对路径
                strcat(path, getenv("URL_PATH"));
                *(strrchr(path, '/') + 1) = '\0';//删掉cgi的名字
                strncat(path, filename_start, filename_end - filename_start);//上传路径

                char* new_urlpath = path + 8;// ./htdocs/ --> /

                // 查找文件数据
                char* file_start = strstr(post_data, "\r\n\r\n") + 4;  // 文件内容的起始位置
                char* file_end = strstr(file_start, boundary_str) - 2; // 文件内容的结束位置
                int file_size = file_end - file_start;

                // 保存图片到服务器
                FILE* image_file = fopen(path, "wb");
                if (image_file)
                {
                    fwrite(file_start, 1, file_size, image_file);
                    fclose(image_file);
                    printf("<h1>Image uploaded successfully!</h1>\n");
                    printf("<p>Image uploaded in %s</p>\n", path);

                    // 生成图片显示的HTML代码
                    printf("<img src=\"%s\" alt=\"Uploaded Image\" />\n", new_urlpath);
                }
                else
                {
                    printf("<h1>Failed to save image.</h1>\n");
                }
            }
            else
            {
                printf("<h1>Failed to find multipart boundary.</h1>\n");
            }

            free(post_data);
        }
        else
        {
            printf("<h1>Failed to allocate memory.</h1>\n");
        }
    }
    else
    {
        printf("<h1>This script only handles POST requests.</h1>\n");
    }

    // 打印 HTML 页面的尾部
    printf("</body></html>\n");

    return 0;
}
