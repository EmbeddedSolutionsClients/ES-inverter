/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_BLE_CORE_H_
#define EM_BLE_CORE_H_

#include "em/ble_gatt.h"
#include <esp_event.h>

ESP_EVENT_DECLARE_BASE(EM_BLE_CORE_EVENT);

typedef enum {
  EM_BLE_CORE_EVENT_DISCONNECTED = 0,
  EM_BLE_CORE_EVENT_CONNECTED,
  EM_BLE_CORE_EVENT_DEINIT,
} em_ble_core_event_t;

int em_ble_init(const char *dev_name, read_ch_cb_t read_cb, write_ch_cb_t write_cb,
                const struct ble_gatt_svc_def gatt_svr_svcs[]);
void em_ble_deinit(void);

#endif /* EM_BLE_CORE_H_ */
