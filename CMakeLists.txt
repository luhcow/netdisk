cmake_minimum_required(VERSION 3.10)
project(httpd)

set(SOURCE_FILES httpd.c)
include_directories (./liburl)

add_executable(httpd ${SOURCE_FILES} ./liburl/urlcode.c ./liburl/liburl.c)
target_link_libraries(httpd  pthread)
