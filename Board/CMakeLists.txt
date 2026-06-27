cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIB_NAME board)

add_library(${CMAKE_LIB_NAME} STATIC)

file(GLOB BOARD_SRC "*.c")

target_sources(${CMAKE_LIB_NAME} PRIVATE ${BOARD_SRC})

target_include_directories(${CMAKE_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_LIB_NAME} PRIVATE stm32cubemx synaptix)