cmake_minimum_required(VERSION 3.10)
project(httpd)

include_directories (./liburl)
include_directories ( /usr/local/include/json-c )

add_subdirectory(./lib/l8w8jwt)

add_executable(httpd httpd.c ./liburl/urlcode.c)
target_link_libraries(httpd  pthread)

add_executable(settings ./api/public/settings.c)
set_target_properties(settings PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/public/)

add_executable(hash ./api/auth/login/hash.c)
set_target_properties(hash PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/auth/login/)
target_link_libraries(hash PRIVATE l8w8jwt json-c)

add_executable(me ./src/me.c)
set_target_properties(me PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/auth/)
target_link_libraries(me PRIVATE l8w8jwt json-c)
