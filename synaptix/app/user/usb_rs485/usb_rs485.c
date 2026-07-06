#include "usb_rs485.h"
#include "bsp_usb.h"
#include "bsp_board_gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"

/*  Config  */
#define BRIDGE_TASK_STACK_WORDS     512
#define BRIDGE_TASK_PRIORITY        (tskIDLE_PRIORITY + 2)
#define BRIDGE_BUF_SIZE             256
#define BRIDGE_POLL_MS              1

/*  Internal state  */
static TaskHandle_t s_bridge_task_handle = NULL;
static TaskHandle_t s_modbus_task_handle = NULL;
static volatile bool s_enabled = false;

/*  Bridge task  */

static void bridge_task(void *arg)
{
    (void)arg;
    uint8_t buf[BRIDGE_BUF_SIZE];

    for (;;) {
        /* Block here when disabled */
        if (!s_enabled) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        /* USB -> RS485 */
        uint32_t avail = bsp_usb_available(BSP_USB_BRIDGE_CH);
        if (avail > 0) {
            if (avail > BRIDGE_BUF_SIZE) avail = BRIDGE_BUF_SIZE;
            uint32_t len = bsp_usb_receiver(BSP_USB_BRIDGE_CH, buf, avail);
            if (len > 0) {
                bsp_rs485_de_on();
                bsp_uart_transmit(BSP_RS485, buf, len, 100);
                bsp_rs485_de_off();
            }
        }

        /* RS485 -> USB */
        uint32_t rx_avail = bsp_uart_available(BSP_RS485);
        if (rx_avail > 0) {
            if (rx_avail > BRIDGE_BUF_SIZE) rx_avail = BRIDGE_BUF_SIZE;
            uint32_t len = bsp_uart_receive(BSP_RS485, buf, rx_avail, 10);
            if (len > 0 && bsp_usb_connected(BSP_USB_BRIDGE_CH)) {
                bsp_usb_transmit(BSP_USB_BRIDGE_CH, buf, len);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BRIDGE_POLL_MS));
    }
}

void usb_rs485_set_modbus_handle(TaskHandle_t handle)
{
    s_modbus_task_handle = handle;
}

/* Public API  */

void usb_rs485_init(void)
{
    // /* Get Modbus task handle to suspend/resume it */
    // s_modbus_task_handle = xTaskGetHandle("modbusTask");

    xTaskCreate(bridge_task,
                "bridgeTask",
                BRIDGE_TASK_STACK_WORDS,
                NULL,
                BRIDGE_TASK_PRIORITY,
                &s_bridge_task_handle);

}

void usb_rs485_enable(void)
{
    if (s_enabled) return;

    /* Suspend Modbus to release USART1 */
    if (s_modbus_task_handle != NULL) {
        vTaskSuspend(s_modbus_task_handle);
    }

    s_enabled = true;

    /* Wake bridge task */
    if (s_bridge_task_handle != NULL) {
        xTaskNotifyGive(s_bridge_task_handle);
    }
}

void usb_rs485_disable(void)
{
    if (!s_enabled) return;

    s_enabled = false;

    /* Resume Modbus */
    if (s_modbus_task_handle != NULL) {
        vTaskResume(s_modbus_task_handle);
    }
}

bool usb_rs485_is_enabled(void){
    return s_enabled;
}