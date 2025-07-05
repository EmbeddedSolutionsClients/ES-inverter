/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_BLE_GATT_H_
#define EM_BLE_GATT_H_

#include "host/ble_hs.h"
#include "host/util/util.h"
#include <stdint.h>

typedef int (*read_ch_cb_t)(uint16_t attr_handle, void *chr_val);
typedef int (*write_ch_cb_t)(uint16_t attr_handle, const void *chr_val, uint16_t chr_len);
int default_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

void set_write_chr_cb(write_ch_cb_t);
void set_read_chr_cb(read_ch_cb_t);

#endif /* EM_BLE_GATT_H_ */
