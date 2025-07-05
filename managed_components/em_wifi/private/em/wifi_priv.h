/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef WIFI_PRIV_H_
#define WIFI_PRIV_H_

#include "esp_wifi_types_generic.h"
#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#define LOG_TAG "em_wifi"

#define STARTED_BIT           (1 << 0) /**< Event bit for IP assignment event */
#define CONNECTED_BIT         (1 << 1) /**< Event bit for IP assignment event */
#define DISCONNECT_DEMAND_BIT (1 << 2) /**< Event bit for WiFi disconnection request event */
#define PROVISIONED_BIT       (1 << 3) /**< Event bit for WiFi provisioning event */
#define SCAN_IN_PROGRESS_BIT  (1 << 4) /**< Event bit for WiFi scanning event */

extern EventGroupHandle_t wifi_evt_group;

const char *reason_to_string(wifi_err_reason_t reason);
void scan_done_handler();

#endif /* WIFI_PRIV_H_ */
