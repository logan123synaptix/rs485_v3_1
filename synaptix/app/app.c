#include "app.h"
#include "usb_rs485.h"
#include "board.h"
#include "stdio.h"
#include "string.h"
#include "logger.h"
#include "indicator.h"
#include "shell_app.h"

static const char *TAG = "APP";

static osThreadId_t app_task_handle;
static const osThreadAttr_t app_task_attr = {
    .name       = "app_task",
    .stack_size = APP_TASK_STACK_SIZE * 4,
    .priority   = APP_TASK_PRIORITY
};

Indicator_t s_indicator;

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
    bsp_init();
    indicator_init(&s_indicator);
    shell_app_init();
    // usb_rs485_init();

    app_task_handle = osThreadNew(app_task, NULL, &app_task_attr);
}

void app_task(void *arg){
    (void)arg;
    LOGI(TAG, "APP TASK IS CREATED");
}