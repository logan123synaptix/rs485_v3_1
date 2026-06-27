#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum BSP_GPIO_STATE{
    BSP_GPIO_LOW = 0,
    BSP_GPIO_HIGH
}BSP_GPIO_STATE_t;
typedef enum BSP_LED{
    BSP_LED_POWER,
    BSP_LED_LINK,
    BSP_LED_STATUS
}BSP_LED_t;

typedef enum BSP_INPUT{
    BSP_INPUT0 = 0,
    BSP_INPUT1,
    BSP_INPUT2,
    BSP_INPUT3
}BSP_INPUT_t;

typedef enum BSP_OUTPUT{
    BSP_OUTPUT0 = 0,
    BSP_OUTPUT1,
    BSP_OUTPUT2,
    BSP_OUTPUT3
}BSP_OUTPUT_t;

typedef enum BSP_TIMER{
    BSP_TIMER1 = 0,
    BSP_TIMER2,
    BSP_TIMER3
}BSP_TIMER_t;

typedef enum BSP_UART{
    BSP_LOGGER = 0,
    BSP_RS485,
    BSP_ZIGBEE
}BSP_UART_t;

int bsp_init();

int bsp_led_write(BSP_LED_t led, BSP_GPIO_STATE_t state);
int bsp_led_toggle(BSP_LED_t led);

BSP_GPIO_STATE_t bsp_input_read(BSP_INPUT_t input);

int bsp_output_write(BSP_OUTPUT_t output, BSP_GPIO_STATE_t state);
int bsp_output_toggle(BSP_OUTPUT_t output);

int bsp_timer_start(BSP_TIMER_t timer);
int bsp_timer_stop(BSP_TIMER_t timer);

int bsp_timer_cb(BSP_TIMER_t timer, void *arg);

int bsp_uart_init(BSP_UART_t uart);
int bsp_uart_transmit(BSP_UART_t uart,uint8_t *buf,uint32_t len,uint32_t timeout);
int bsp_uart_receive(BSP_UART_t uart,uint8_t *buff,uint32_t len,uint32_t timeout);
int bsp_uart_available(BSP_UART_t uart);
int bsp_uart_flush(BSP_UART_t uart);
int bsp_uart_rx_cb(BSP_UART_t uart, uint8_t *buf,uint32_t len);
int bsp_delay_ms(uint32_t ms);
uint32_t bsp_get_tick();

#ifdef __cplusplus
}
#endif

#endif