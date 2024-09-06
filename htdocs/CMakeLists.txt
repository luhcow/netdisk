cmake_minimum_required(VERSION 3.10)
project(httpd)

set(SOURCE_FILES path_html.c)

add_executable(path ${SOURCE_FILES})
