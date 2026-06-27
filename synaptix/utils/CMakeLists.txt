cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIBS_NAME utils)

add_library(${CMAKE_LIBS_NAME} STATIC)

file(GLOB UTILS_SRC "*.c"
                    "cqueue/*.c"
    )

target_sources(${CMAKE_LIBS_NAME} PRIVATE 
    ${UTILS_SRC}
)

target_include_directories(${CMAKE_LIBS_NAME} PUBLIC
    cqueue
)

target_link_libraries(${CMAKE_LIBS_NAME} PRIVATE)