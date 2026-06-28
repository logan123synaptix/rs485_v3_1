/**
 * @file bsp_board_gpio.c
 * @brief Supplementary BSP implementation for RF_IO_RS485_V3.
 */

#include "bsp_board_gpio.h"

#include "stm32h5xx_hal.h"
#include "gpio.h"

/* ── RS485 DE pin ─────────────────────────────────────────────────────────── */
/* PB13 — Output PP, defined in gpio.c                                        */

void bsp_rs485_de_on(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
}

void bsp_rs485_de_off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
}

/* ── UART TX done callback ────────────────────────────────────────────────── */

typedef struct {
    bsp_uart_tx_cb_t cb;
    void            *arg;
} uart_tx_cb_entry_t;

static uart_tx_cb_entry_t s_tx_cbs[3]; /* indexed by BSP_UART_t */

void bsp_com_set_tx_callback(BSP_UART_t uart, bsp_uart_tx_cb_t cb, void *arg)
{
    if (uart >= 3) return;
    s_tx_cbs[uart].cb  = cb;
    s_tx_cbs[uart].arg = arg;
}

/* Called from HAL_UART_TxCpltCallback (override weak in bsp_callbacks or stm32h5xx_it.c) */
void bsp_uart_tx_done(BSP_UART_t uart)
{
    if (uart >= 3) return;
    if (s_tx_cbs[uart].cb != NULL) {
        s_tx_cbs[uart].cb(s_tx_cbs[uart].arg);
    }
}

/* ── Timer callback registration ──────────────────────────────────────────── */

static bsp_timer_cb_fn_t s_timer_cbs[3]; /* indexed by BSP_TIMER_t */

void bsp_timer_set_handle(BSP_TIMER_t timer, bsp_timer_cb_fn_t cb)
{
    if (timer >= 3) return;
    s_timer_cbs[timer] = cb;
}

/* Override Board weak bsp_timer_cb — dispatches to registered callbacks */
int bsp_timer_cb(BSP_TIMER_t timer, void *arg)
{
    (void)arg;
    if (timer < 3 && s_timer_cbs[timer] != NULL) {
        s_timer_cbs[timer]();
    }
    return 0;
}

/* ── System restart ───────────────────────────────────────────────────────── */

void bsp_restart(void)
{
    NVIC_SystemReset();
}

/* ── Modbus slave address ─────────────────────────────────────────────────── */

uint8_t bsp_get_address(void)
{
    return 1; /* Hardcoded — no DIP switch on V3 (D1) */
}