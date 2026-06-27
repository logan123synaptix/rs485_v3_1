cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIBS_NAME services)

add_library(${CMAKE_LIBS_NAME} STATIC)

file(GLOB SERVICE_SRC "tcp_server/*.c"
                      "socket_server/*.c"
                      "socket_client/*.c"
                      "shell/*.c"
                      "zigbee/*.c"
                      "web_service/*.c")

target_sources(${CMAKE_LIBS_NAME} PRIVATE
    ${SERVICE_SRC}
)

target_include_directories(${CMAKE_LIBS_NAME} PUBLIC
    tcp_server
    socket_server
    socket_client
    zigbee
    shell
    web_service
    ${CMAKE_CURRENT_SOURCE_DIR}/modbus
)

target_link_libraries(${CMAKE_LIBS_NAME} PRIVATE
    libs stm32cubemx synaptix_modbus lwipcore logger utils
)