/**
 * @file rf_app.c
 * @brief RF application — Zigbee only for RF_IO_RS485_V3.
 *
 * Ported from RS485_v2 rf_app.c.
 * Changes from V2:
 *   - Removed LoRa (LORA_ENABLE not used in V3)
 *   - board.h → bsp_uart.h
 *   - bsp_get_tick() → HAL_GetTick()
 *   - log_info/log_error/log_debug → LOGI/LOGE/LOGD
 *   - indicator wired to bsp_gpio LED functions
 */

#include "rf_app.h"
#include "bsp_uart.h"
#include "bsp_gpio.h"
#include "indicator.h"
#include "logger.h"

#include "stm32h5xx_hal.h"

static const char *TAG        = "RFApp";
static const char *TAG_Driver = "RFDriver";

/* ── Zigbee instance ──────────────────────────────────────────────────────── */
ZigbeeMesh_t   zigbee;
ZigbeeDriver_t zigbee_driver;

static Indicator_t indicator;
static uint32_t    rf_time_stamp = 0;

/* ── Zigbee callbacks ─────────────────────────────────────────────────────── */

static void zigbee_connect_cb(ZigbeeMesh_t *zb, uint8_t isSuccess, void *arg)
{
    (void)arg;
    if (isSuccess == ZIGBEE_RES_SUCCESS) {
        LOGI(TAG, "Connect to Zigbee module success");
        LOGI(TAG, "Zigbee FW Version : V%d.%d",
             zb->fw_version[0], zb->fw_version[1]);
        indicator_set_net_status(&indicator, EVENT_NET_CONNECTED);
    } else if (isSuccess == ZIGBEE_RES_FAIL) {
        LOGE(TAG, "Connect to Zigbee module fail");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    } else {
        LOGE(TAG, "Connect to Zigbee module timeout");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    }
}

static void zigbee_read_cb(ZigbeeMesh_t *zb, uint8_t isSuccess, void *arg)
{
    (void)arg;
    if (isSuccess == ZIGBEE_RES_SUCCESS) {
        ZigbeeParameter_t *param = &zb->param;
        LOGI(TAG, "Read Zigbee module param success");
        LOGI(TAG, "PAN ID       : 0x%04X", param->PAN_ID);
        LOGI(TAG, "Channel      : %d",     param->Channel);
        LOGI(TAG, "User Address : 0x%04X", param->userAddress);
        LOGI(TAG, "Short Address: 0x%04X", param->shortAddress);
        indicator_set_net_status(&indicator, EVENT_NET_CONNECTED);
    } else if (isSuccess == ZIGBEE_RES_FAIL) {
        LOGE(TAG, "Read Zigbee module param fail");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    } else {
        LOGE(TAG, "Read Zigbee module param timeout");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    }
}

static void zigbee_reset_cb(ZigbeeMesh_t *zb, uint8_t isSuccess, void *arg)
{
    (void)zb; (void)arg;
    if (isSuccess == ZIGBEE_RES_SUCCESS) {
        LOGI(TAG, "Reset Zigbee module success");
        indicator_set_net_status(&indicator, EVENT_NET_CONNECTED);
    } else if (isSuccess == ZIGBEE_RES_FAIL) {
        LOGE(TAG, "Reset Zigbee module fail");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    } else {
        LOGE(TAG, "Reset Zigbee module timeout");
        indicator_set_net_status(&indicator, EVENT_NET_DISCONNECTED);
    }
}

/* ── Driver I/O ───────────────────────────────────────────────────────────── */

static uint32_t rf_get_available(void)
{
    return bsp_com_available(BSP_RF_COM_PORT);
}

static void rf_write(uint8_t *buff, uint32_t len)
{
    LOGD(TAG_Driver, "RF Write : %u", len);
    log_print_hex(LOGGER_DEBUG, TAG_Driver, buff, (uint16_t)len);
    bsp_com_write(BSP_RF_COM_PORT, buff, len);
}

static void rf_read(uint8_t *buff, uint32_t len)
{
    uint32_t length = bsp_com_read(BSP_RF_COM_PORT, buff, len);
    if (length > 0) {
        LOGD(TAG_Driver, "RF Read : %u", length);
        log_print_hex(LOGGER_INFO, TAG_Driver, buff, (uint16_t)length);
    }
}

/* ── Indicator wiring ─────────────────────────────────────────────────────── */

static void ind_on(int num)     { bsp_output_on(num); }
static void ind_off(int num)    { bsp_output_off(num); }
static void ind_toggle(int num) { bsp_output_toggle(num); }

/* ── Public API ───────────────────────────────────────────────────────────── */

void rf_app_init(void)
{
    indicator.on     = ind_on;
    indicator.off    = ind_off;
    indicator.toggle = ind_toggle;
    indicator_init(&indicator);

    zigbee_driver.write     = rf_write;
    zigbee_driver.read      = rf_read;
    zigbee_driver.available = rf_get_available;

    zigbee_init(&zigbee, &zigbee_driver);
    zigbee_connect(&zigbee, zigbee_connect_cb, NULL);
    zigbee_read_module(&zigbee, zigbee_read_cb, NULL);
    zigbee_reset_module(&zigbee, zigbee_reset_cb, NULL);

    rf_time_stamp = HAL_GetTick();
}

void rf_app_poll(void)
{
    if (HAL_GetTick() - rf_time_stamp >= 1) {
        zigbee_poll(&zigbee, 1);
        indicator_poll(&indicator, 1);
        rf_time_stamp = HAL_GetTick();
    }
}

bool rf_app_busy(void)
{
    return zigbee.event.event != ZB_EVENT_NONE;
}