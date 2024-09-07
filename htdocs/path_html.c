#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// 用于生成每个文件的下载链接
void print_file_link(const char* dir_path, const char* file_name)
{
    //删除dir_path 前部的/htdocs/download
    char relative_path[1024];
    strcpy(relative_path, dir_path);
    strcpy(relative_path, relative_path + 8);
    if (relative_path[strlen(relative_path) - 1] != '/')
        strcat(relative_path, "/");
    printf("<li><a href=\"%s%s\">%s</a></li>\n", relative_path, file_name, file_name);
}

// 显示目录结构
void list_directory(const char* dir_path)
{
    struct dirent* entry;
    DIR* dp = opendir(dir_path);

    if (dp == NULL)
    {
        printf("<p>Could not open directory: %s</p>\n", dir_path);
        return;
    }

    printf("<ul>\n");
    while ((entry = readdir(dp)) != NULL)
    {
        // 忽略"."和".."目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        //如果是目录，递归列出内容-->给filename添加"/"
        // if (entry->d_type == DT_DIR)
        // {
        //     //printf("<li>%s/</li>\n", entry->d_name);
        //     char new_path[1024];
        //     snprintf(new_path, sizeof(new_path), "%s/", entry->d_name);
        //     print_file_link(new_path);
        // }
        // else
        // {
            //如果是文件，生成下载链接
        print_file_link(dir_path, entry->d_name);
        // }
    }
    printf("</ul>\n");

    printf("<form action=\"co.cgi\" method=\"post\" enctype=\"multipart / form - data\">\n");
    printf("<span class=\"form - line\">文件(file): <input type=\"file\" id=\"file\" name=\"file\" /></span>\n");
    printf("<input type=\"submit\" name=\"submit\" value=\"upload\" />\n");

    closedir(dp);
}

void get_file_extension(char szSomeFileName[])
{
    char* pLastSlash = strrchr(szSomeFileName, '/');
    pLastSlash = '/0';
}

int main(int argc, char* argv[])
{
    // 设置Content-type头，告诉浏览器返回HTML内容
    printf("Content-Type: text/html; charset=utf-8\n\n");

    // 打印HTML页面的头部
    printf("<html><head><meta charset=\"UTF-8\"><title>File Tree</title></head><body>\n");
    printf("<h1>File Tree</h1>\n");
    printf("<p>Click on a file to download it.</p>\n");
    // char* path = argv[0];
    // printf("<p>%s</p>\n", path);


    // char* path = getenv("URL_PATH");
    // printf("<p>%s</p>\n", path);
    // char relative_path[1024] = "./";
    // strcat(relative_path, path);

    //printf("<p>%s</p>\n", path);


    printf("<p>%s</p>\n", argv[0]);
    //清除路径中的最后一个斜杠
    //get_file_extension(path);
    //path 前面加上相对地址
    char relative_path[1024] = "./";
    strcat(relative_path, argv[0]);
    printf("<p>%s</p>\n", relative_path);
    // 要展示的目录路径（服务器上的目录）
    const char* base_dir = relative_path;  // 将此路径替换为实际的目录路径

    // 列出目录结构
    list_directory(base_dir);

    // 打印HTML页面的尾部
    printf("</body></html>\n");

    return 0;
}
