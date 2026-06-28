cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

set(CMAKE_LIBS_NAME synaptix)

add_library(${CMAKE_LIBS_NAME} INTERFACE)

add_subdirectory(services)
add_subdirectory(utils)
add_subdirectory(bsp)
add_subdirectory(services/modbus)
add_subdirectory(app)


target_include_directories(${CMAKE_LIBS_NAME} INTERFACE)

target_link_libraries(${CMAKE_LIBS_NAME} INTERFACE
    services
    utils
    synaptix_app
    synaptix_bsp
)