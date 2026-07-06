#include "app.h"
#include "usb_rs485.h"
#include "board.h"
#include "stdio.h"
#include "string.h"
#include "logger.h"

const char *TAG = "APP";

static osThreadId_t app_task_handle;
static const osThreadAttr_t app_task_attr = {
    .name       = "app_task",
    .stack_size = APP_TASK_STACK_SIZE * 4,
    .priority   = APP_TASK_PRIORITY
};

static void uart_log_write(const char *s){
    bsp_uart_transmit(BSP_LOGGER, (uint8_t *)s, strlen(s), 100);
}

static void app_logger_init(void){
    bsp_uart_init(BSP_LOGGER);
    logger_init(LOGGER_DEBUG, uart_log_write);
    LOGI(TAG, "Logger Init Success");
}

void app_init(void){
    app_logger_init();

    /*another initialize*/
}

void app_task(void *arg){
    (void)arg;
    log_info(TAG, "APP TASK IS CREATING");
}