#ifndef USB_RS485_H
#define USB_RS485_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

void usb_rs485_init(void);

void usb_rs485_enable(void);

void usb_rs485_disable(void);

bool usb_rs485_is_enabled(void);

void usb_rs485_set_modbus_handle(TaskHandle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* USB_RS485_H */