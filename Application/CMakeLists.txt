cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIB_NAME application)

add_library(${CMAKE_LIB_NAME} STATIC)

file(GLOB APP_SRC "*.c")

target_sources(${CMAKE_LIB_NAME} PRIVATE ${APP_SRC})

target_include_directories(${CMAKE_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})