#ifndef USB_RS485_H
#define USB_RS485_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "task.h"

#define BSP_USB_BRIDGE_CH   2   /* CDC interface 2 — USB-RS485 bridge */

void usb_rs485_init(void);

void usb_rs485_enable(void);

void usb_rs485_disable(void);

bool usb_rs485_is_enabled(void);

void usb_rs485_set_modbus_handle(TaskHandle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* USB_RS485_H */