/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef ES_SERVICE_H_
#define ES_SERVICE_H_

#include "em/ble_core.h"
#include "host/ble_hs.h"
#include <stdint.h>

extern const ble_uuid128_t gatt_svr_chr_uuid_ssid;
extern const ble_uuid128_t gatt_svr_chr_uuid_pass;
extern const ble_uuid128_t gatt_svr_chr_uuid_connect;
extern const ble_uuid128_t gatt_svr_chr_uuid_devid;
extern const ble_uuid128_t gatt_svr_chr_uuid_aps;
extern const ble_uuid128_t gatt_svr_chr_uuid_ap_id;

extern uint16_t attr_handle_chr_wifi_ssid;
extern uint16_t attr_handle_chr_wifi_pass;
extern uint16_t attr_handle_chr_wifi_connection;
extern uint16_t attr_handle_chr_wifi_dev_id;
extern uint16_t attr_handle_chr_wifi_aps;
extern uint16_t attr_handle_chr_wifi_ap_id;

void service_init();

int read_chr_handler(uint16_t attr_handle, void *chr_val);
int write_chr_handler(uint16_t attr_handle, const void *chr_val, uint16_t chr_len);

#endif /* ES_SERVICE_H_ */
