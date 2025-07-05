/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/ble_gatt.h"

#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <esp_log.h>
#define LOG_TAG "em_ble_gatt"

static read_ch_cb_t read_attr_cb = NULL;
static write_ch_cb_t write_attr_cb = NULL;

#define MIN_CHR_LEN (1)                    // minimum length the client can write to a characteristic
#define MAX_CHR_LEN (128)                  // maximum length the client can write to a characteristic
static uint8_t chr_op_buffer[MAX_CHR_LEN]; // buffer for read characteristic

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len)
{
  uint16_t om_len = OS_MBUF_PKTLEN(om);
  if (om_len < min_len || om_len > max_len) {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  int rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
  if (rc != 0) {
    return BLE_ATT_ERR_UNLIKELY;
  }

  return 0;
}

void set_write_chr_cb(write_ch_cb_t cb)
{
  write_attr_cb = cb;
}

void set_read_chr_cb(read_ch_cb_t cb)
{
  read_attr_cb = cb;
}

// Callback function. When ever characteristic will be accessed by user, this function will execute
int default_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    assert(read_attr_cb != NULL);

    int chr_len = read_attr_cb(attr_handle, chr_op_buffer);

    assert(chr_len <= MAX_CHR_LEN);

    if (chr_len <= 0) {
      ESP_LOGE(LOG_TAG, "Failed to read chr=%d ret=%d", attr_handle, chr_len);
      return BLE_ATT_ERR_UNLIKELY;
    }

    int rc = os_mbuf_append(ctxt->om, chr_op_buffer, chr_len);

    if (rc != 0) {
      ESP_LOGE(LOG_TAG, "Failed to append chr=%d len=%d", attr_handle, chr_len);
    }

    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    assert(write_attr_cb != NULL);

    uint16_t chr_len = 0;
    int rc = gatt_svr_chr_write(ctxt->om, MIN_CHR_LEN, MAX_CHR_LEN, chr_op_buffer, &chr_len);

    if (rc != 0) {
      ESP_LOGE(LOG_TAG, "Failed to write chr=%d", attr_handle);
    } else {
      int err = write_attr_cb(attr_handle, chr_op_buffer, chr_len);

      if (err < 0) {
        ESP_LOGE(LOG_TAG, "Failed to write chr=%d", attr_handle);
        return BLE_ATT_ERR_UNLIKELY;
      }
    }

    return rc;
  }

  default:
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
  }
}
