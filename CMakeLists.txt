cmake_minimum_required(VERSION 3.20)
project(httpd)

include_directories(./include)

# 添加 l8w8jwt 和 json-c 库
add_subdirectory(./lib/l8w8jwt)
add_subdirectory(./lib/json-c)
add_subdirectory(./lib/rabbitmq-c)
aux_source_directory(src/public PUBLIC_SRC)

# 编译 api_gateway 可执行文件，并链接 pthread l8w8jwt rabbitmq::rabbitmq
aux_source_directory(src/api_gateway API_GATEWAY_SRC)
add_executable(api_gateway ${PUBLIC_SRC} ${API_GATEWAY_SRC})
#set_target_properties(api_gateway PROPERTIES
#        RUNTIME_OUTPUT_DIRECTORY ../)
target_link_libraries(api_gateway PRIVATE pthread l8w8jwt rabbitmq::rabbitmq json-c)
file(COPY conf/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/conf/)

aux_source_directory(src/fs FS_SRC)
<<<<<<< HEAD
add_executable(fs  ${PUBLIC_SRC} ${FS_SRC})
=======
add_executable(fs ${PUBLIC_SRC} ${FS_SRC})
>>>>>>> a750234e1a82008a80ef4e4661b016bdbf982cfa
target_link_libraries(fs PRIVATE pthread l8w8jwt rabbitmq::rabbitmq json-c)
