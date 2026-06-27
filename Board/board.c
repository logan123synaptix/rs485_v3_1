#include "main.h"
#include "tim.h"
#include "usart.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#define BSP_LED_NUM 3
#define BSP_INPUT_NUM 4
#define BSP_OUTPUT_NUM 4
#define BSP_TIMER_NUM 3
#define BSP_UART_NUM 3
typedef struct bsp_gpio{
    void *port;
    int pin;
}bsp_gpio_t;

typedef struct bsp_uarts{
    void *handle;
    QueueHandle_t rx_queue;
}bsp_uarts_t;

static bsp_uarts_t s_bsp_uarts[BSP_UART_NUM] = {{.handle = &hlpuart1} , {.handle = &huart1} ,{.handle = &huart2}};

static bsp_gpio_t s_leds[BSP_LED_NUM] = {{.pin = GPIO_PIN_13, .port = GPIOC},
                               {.pin = GPIO_PIN_14, .port = GPIOC},
                               {.pin = GPIO_PIN_15, .port = GPIOC}};
static bsp_gpio_t s_inputs[BSP_INPUT_NUM] = {
    {.pin = GPIO_PIN_6, .port = GPIOB},
    {.pin = GPIO_PIN_5, .port = GPIOB},
    {.pin = GPIO_PIN_4, .port = GPIOB},
    {.pin = GPIO_PIN_3, .port = GPIOB},
};
static bsp_gpio_t s_outputs[BSP_OUTPUT_NUM] = {
    {.pin = GPIO_PIN_6, .port = GPIOA},
    {.pin = GPIO_PIN_7, .port = GPIOA},
    {.pin = GPIO_PIN_0, .port = GPIOB},
    {.pin = GPIO_PIN_1, .port = GPIOB},
};

static void *s_timers[BSP_TIMER_NUM] = {&htim1,&htim2,&htim3};

int bsp_led_write(BSP_LED_t led, BSP_GPIO_STATE_t state)
{
    if(led < 0 || led >= BSP_LED_NUM) return -1;
    
    HAL_GPIO_WritePin((GPIO_TypeDef *) s_leds[led].port,(uint16_t)s_leds[led].pin,(GPIO_PinState)state);

    return 0;
}
int bsp_led_toggle(BSP_LED_t led) {
    if(led < 0 || led >= BSP_LED_NUM) return -1;
    
    HAL_GPIO_TogglePin((GPIO_TypeDef *) s_leds[led].port,(uint16_t)s_leds[led].pin);

    return 0;
}

BSP_GPIO_STATE_t bsp_input_read(BSP_INPUT_t input) {
    if(input < 0 || input >= BSP_INPUT_NUM) return -1;

    return (BSP_GPIO_STATE_t) HAL_GPIO_ReadPin((GPIO_TypeDef*)s_inputs[input].port,(uint16_t) s_inputs[input].pin);
}

int bsp_output_write(BSP_OUTPUT_t output, BSP_GPIO_STATE_t state) {
    if(output < 0 || output >= BSP_OUTPUT_NUM)  return -1;

    HAL_GPIO_WritePin((GPIO_TypeDef *) s_outputs[output].port,(uint16_t)s_outputs[output].pin,(GPIO_PinState)state);
    return 0;
}
int bsp_output_toggle(BSP_OUTPUT_t output) {
    if(output < 0 || output >= BSP_OUTPUT_NUM)  return -1;
    HAL_GPIO_TogglePin((GPIO_TypeDef *) s_outputs[output].port,(uint16_t)s_outputs[output].pin);
    return 0;
}

int bsp_timer_start(BSP_TIMER_t timer) {
    if(timer < 0 || timer > BSP_TIMER_NUM) return -1;

    HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)s_timers[timer]);

    return 0;
}
int bsp_timer_stop(BSP_TIMER_t timer) {
    if(timer < 0 || timer > BSP_TIMER_NUM) return -1;

    HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *)s_timers[timer]);

    return 0;
}

__attribute__((weak)) int bsp_timer_cb(BSP_TIMER_t timer, void *arg){
    return 0;
}

int bsp_uart_init(BSP_UART_t uart){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    s_bsp_uarts[uart].rx_queue = xQueueCreate(256,sizeof(uint8_t));
    return 0;
}

int bsp_uart_transmit(BSP_UART_t uart,uint8_t *buf,uint32_t len,uint32_t timeout){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    
    return HAL_UART_Transmit((UART_HandleTypeDef*)s_bsp_uarts[uart].handle,buf,len,timeout);
}
int bsp_uart_receive(BSP_UART_t uart,uint8_t *buff,uint32_t len,uint32_t timeout){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    uint32_t time = bsp_get_tick();
    uint32_t i = 0;
    while (bsp_get_tick() - time < timeout)
    {
        /* code */
        if(pdPASS == xQueueReceive(s_bsp_uarts[uart].rx_queue,&buff[i],1)){
            i++;
            if(i >= len) break;
        }

    }
    return (int)i;
}
int bsp_uart_available(BSP_UART_t uart){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    
    return uxQueueMessagesWaiting(s_bsp_uarts[uart].rx_queue);
}
int bsp_uart_flush(BSP_UART_t uart){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    BaseType_t xStatus = xQueueReset(s_bsp_uarts[uart].rx_queue);
    if(xStatus == pdPASS) {
    // Queue successfully flushed
        return 0;
    }
    return -1;
}
__attribute__((weak)) int bsp_uart_rx_cb(BSP_UART_t uart, uint8_t *buf,uint32_t len){
    if(uart < 0 || uart >= BSP_UART_NUM) return -1;
    BaseType_t xHigherPriorityTaskWoken;
    for(int i = 0;i<len;i++){
        xQueueSendToFrontFromISR( s_bsp_uarts[uart].rx_queue,&buf[i], &xHigherPriorityTaskWoken );
    }

    if(xHigherPriorityTaskWoken){
        taskYIELD();
    }
    return 0;
}
int bsp_delay_ms(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}
uint32_t bsp_get_tick(){
    return xTaskGetTickCount();
}

int bsp_init(){
    return 0;
}

