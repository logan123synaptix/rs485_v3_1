/**
 * @file bsp_board_gpio.h
 * @brief Supplementary BSP functions not present in Board/board.h.
 *
 * Board/board.h provides generic GPIO, UART, timer and LED APIs.
 * This file adds platform-specific extensions needed by synaptix services:
 *
 *   - RS485 DE pin control       (portserial.c)
 *   - UART TX done callback      (portserial.c)
 *   - Timer callback registration (porttimer.c)
 *   - System restart             (shell_commands.c)
 *   - Modbus slave address read  (user_mb_app.c)
 *   - Named LED aliases          (indicator.c)
 */

#ifndef BSP_BOARD_GPIO_H
#define BSP_BOARD_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "board.h"

/* ── RS485 DE pin ─────────────────────────────────────────────────────────── */

void bsp_rs485_de_on(void);
void bsp_rs485_de_off(void);

/* ── UART TX done callback ────────────────────────────────────────────────── */
/* Called when a UART TX transfer completes (used by Modbus port layer).       */

typedef void (*bsp_uart_tx_cb_t)(void *arg);
void bsp_com_set_tx_callback(BSP_UART_t uart, bsp_uart_tx_cb_t cb, void *arg);

/* ── Timer callback registration ──────────────────────────────────────────── */
/* Register a callback for a hardware timer period-elapsed event.              */

typedef void (*bsp_timer_cb_fn_t)(void);
void bsp_timer_set_handle(BSP_TIMER_t timer, bsp_timer_cb_fn_t cb);

/* ── System restart ───────────────────────────────────────────────────────── */

void bsp_restart(void);

/* ── Modbus slave address ─────────────────────────────────────────────────── */
/* Returns hardcoded 1 (no DIP switch on V3).                                 */

uint8_t bsp_get_address(void);

/* ── Named LED aliases (used by indicator.c) ──────────────────────────────── */
/* Maps to Board LED enum: POW=BSP_LED_POWER, NET=BSP_LED_LINK,               */
/*                         STATUS=BSP_LED_STATUS                               */

static inline void bsp_led_pow_on(void)      { bsp_led_write(BSP_LED_POWER,  BSP_GPIO_HIGH); }
static inline void bsp_led_pow_off(void)     { bsp_led_write(BSP_LED_POWER,  BSP_GPIO_LOW);  }
static inline void bsp_led_pow_toggle(void)  { bsp_led_toggle(BSP_LED_POWER);                }

static inline void bsp_led_net_on(void)      { bsp_led_write(BSP_LED_LINK,   BSP_GPIO_HIGH); }
static inline void bsp_led_net_off(void)     { bsp_led_write(BSP_LED_LINK,   BSP_GPIO_LOW);  }
static inline void bsp_led_net_toggle(void)  { bsp_led_toggle(BSP_LED_LINK);                 }

static inline void bsp_led_status_on(void)   { bsp_led_write(BSP_LED_STATUS, BSP_GPIO_HIGH); }
static inline void bsp_led_status_off(void)  { bsp_led_write(BSP_LED_STATUS, BSP_GPIO_LOW);  }
static inline void bsp_led_status_toggle(void){ bsp_led_toggle(BSP_LED_STATUS);              }

/* ── Input/Output aliases (compatibility with old bsp_gpio naming) ─────────── */

static inline int  bsp_get_input(int n)         { return (int)bsp_input_read((BSP_INPUT_t)(n-1));                    }
static inline void bsp_output_on(int n)          { bsp_output_write((BSP_OUTPUT_t)(n-1), BSP_GPIO_HIGH);             }
static inline void bsp_output_off(int n)         { bsp_output_write((BSP_OUTPUT_t)(n-1), BSP_GPIO_LOW);              }
static inline void bsp_output_toggle_n(int n)    { bsp_output_toggle((BSP_OUTPUT_t)(n-1));                           }

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_GPIO_H */