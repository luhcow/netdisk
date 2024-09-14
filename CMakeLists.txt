cmake_minimum_required(VERSION 3.2)
project(httpd)

include_directories (./lib/urlcode)


# 添加 l8w8jwt 和 json-c 库
add_subdirectory(./lib/l8w8jwt)
add_subdirectory(./lib/json-c)

# 编译 httpd 可执行文件，并链接 pthread (根据平台需要)
add_executable(httpd httpd.c ./lib/urlcode/urlcode.c)
target_link_libraries(httpd pthread)


# 编译 settings，可根据需要输出到特定路径
add_executable(settings ./src/settings.c)
set_target_properties(settings PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/public/)

# 编译 hash，可根据需要输出到特定路径
add_executable(hash ./src/hash.c)
set_target_properties(hash PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/auth/login/)
target_link_libraries(hash PRIVATE l8w8jwt json-c)

# 编译 me，可根据需要输出到特定路径
add_executable(me ./src/me.c)
set_target_properties(me PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ./api/auth/)
target_link_libraries(me PRIVATE l8w8jwt json-c)
