cmake_minimum_required(VERSION 3.20)
project(httpd)

include_directories(./include)

# 添加对gdb的支持
# SET(CMAKE_BUILD_TYPE "Debug")
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# 添加 l8w8jwt 和 json-c 库
add_subdirectory(./lib/l8w8jwt)
add_subdirectory(./lib/json-c)

# 编译 httpd 可执行文件，并链接 pthread 
add_executable(httpd httpd.c ./src/urlcode.c)
set_target_properties(httpd PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../)
target_link_libraries(httpd pthread)


# 编译 settings，输出到特定路径
add_executable(settings ./src/settings.c)
set_target_properties(settings PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/public/)

# 编译 hash，输出到特定路径
add_executable(hash ./src/hash.c)
set_target_properties(hash PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/auth/login/)
target_link_libraries(hash PRIVATE l8w8jwt json-c)

# 编译 me，输出到特定路径
add_executable(me ./src/me.c ./src/authorization.c)
set_target_properties(me PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/auth/)
target_link_libraries(me PRIVATE l8w8jwt json-c)

# 编译 get，输出到特定路径
add_executable(get ./src/get.c ./src/authorization.c)
set_target_properties(get PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/fs/)
target_link_libraries(get PRIVATE l8w8jwt json-c)

# 编译 list，输出到特定路径
add_executable(list ./src/list.c ./src/authorization.c)
set_target_properties(list PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/fs/)
target_link_libraries(list PRIVATE l8w8jwt json-c)

# # 编译 offline_download_tools，输出到特定路径
# add_executable(offline_download_tools ./src/offline_download_tools.c ./src/authorization.c)
# set_target_properties(list PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ./api/public/)
# target_link_libraries(list PRIVATE l8w8jwt json-c)
add_executable(form src/form.c)
set_target_properties(form PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ../api/fs/)
target_link_libraries(form PRIVATE l8w8jwt json-c)