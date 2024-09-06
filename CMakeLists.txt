cmake_minimum_required(VERSION 3.10)
project(httpd)

set(SOURCE_FILES httpd.c)

add_executable(httpd ${SOURCE_FILES})
target_link_libraries(httpd  pthread)
