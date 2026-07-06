cmake_minimum_required(VERSION 3.22)

add_library(synaptix_app STATIC
    app.c
    user/modbus/user_mb_app.c
    user/indicator/indicator.c
    user/rf/rf_app.c
    user/shell/shell_app.c
    user/shell/shell_commands.c
    user/usb_rs485/usb_rs485.c
)
     
target_include_directories(synaptix_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/user/modbus
    ${CMAKE_CURRENT_SOURCE_DIR}/user/rf
    ${CMAKE_CURRENT_SOURCE_DIR}/user/indicator
    ${CMAKE_CURRENT_SOURCE_DIR}/user/shell
    ${CMAKE_CURRENT_SOURCE_DIR}/user/usb_rs485
)

target_link_libraries(synaptix_app PUBLIC
    synaptix_bsp
    board
    synaptix_modbus
    services
    utils
    logger
)