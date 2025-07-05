/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <esp_event.h>
#include <esp_log.h>
#define LOG_TAG "EM_BLE"

#include "em/ble_core.h"
#include "em/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static uint8_t own_addr_type = 0xFF;
static bool initialized = false;

ESP_EVENT_DEFINE_BASE(EM_BLE_CORE_EVENT);

static int handle_gap_event_cb(struct ble_gap_event *event, void *arg);

static void start_advertising(void)
{
  if (own_addr_type == 0xFF) {
    ESP_LOGW(LOG_TAG, "own_addr_type not set");
    return;
  }

  if (!initialized) {
    ESP_LOGI(LOG_TAG, "not initialized");
    return;
  }

  struct ble_hs_adv_fields fields = {
    0,
  };

  /* Set the advertisement data included in our advertisements:
   *     o Flags (indicates advertisement type and other general info).
   *     o Advertising tx power.
   *     o Device name.
   */

  /* Advertise two flags:
   *     o Discoverability in forthcoming advertisement (general)
   *     o BLE-only (BR/EDR unsupported).
   */
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  /* Indicate that the TX power level field should be included; have the
   * stack fill this value automatically.  This is done by assigning the
   * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
   */
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  const char *name = ble_svc_gap_device_name();
  fields.name = (uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  int rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG, "Cannot set adv data err=%d\n", rc);
    return;
  }

  struct ble_gap_adv_params adv_params = {
    0,
  };
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  int32_t adv_intvl_period_ms = BLE_HS_FOREVER;

  if (CONFIG_ES_ADVERTISING_INTERVAL_MS > 0) {
    adv_intvl_period_ms = CONFIG_ES_ADVERTISING_INTERVAL_MS;
  }

  rc = ble_gap_adv_start(own_addr_type, NULL, adv_intvl_period_ms, &adv_params, handle_gap_event_cb, NULL);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG, "Cannot enable advertisement err=%d\n", rc);
    return;
  }

  ESP_LOGI(LOG_TAG, "Start adv addr_type=%d period=%ldms", own_addr_type, adv_intvl_period_ms);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * NimBLE uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int handle_gap_event_cb(struct ble_gap_event *event, void *arg)
{
  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT: {
    if (event->connect.status == 0) {
      ESP_LOGI(LOG_TAG, "Connected status=%d ", event->connect.status);
      struct ble_gap_conn_desc desc = {
        0,
      };
      int rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
      assert(rc == 0);
      esp_event_post(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_CONNECTED, NULL, 0, portMAX_DELAY);
    } else {
      ESP_LOGI(LOG_TAG, "Connection failed status=%d ", event->connect.status);
      /* Connection failed; resume advertising. */
      start_advertising();
    }

    return 0;
  }

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(LOG_TAG, "Disconnect reason=%d ", event->disconnect.reason);

    /* Connection terminated; resume advertising. */
    start_advertising();
    esp_event_post(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_DISCONNECTED, &event->disconnect.reason,
                   sizeof(event->disconnect.reason), portMAX_DELAY);
    return 0;

  case BLE_GAP_EVENT_CONN_UPDATE: {
    /* The central has updated the connection parameters. */
    ESP_LOGI(LOG_TAG, "Connection updated status=%d ", event->conn_update.status);
    struct ble_gap_conn_desc desc = {
      0,
    };
    int rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
    assert(rc == 0);
    return 0;
  }

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(LOG_TAG, "Advertise complete reason=%d", event->adv_complete.reason);
    start_advertising();
    return 0;

  case BLE_GAP_EVENT_CONN_UPDATE_REQ:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_CONN_UPDATE_REQ));
    break;
  case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_L2CAP_UPDATE_REQ));
    break;
  case BLE_GAP_EVENT_TERM_FAILURE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_TERM_FAILURE));
    break;
  case BLE_GAP_EVENT_DISC:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_DISC));
    break;
  case BLE_GAP_EVENT_DISC_COMPLETE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_DISC_COMPLETE));
    break;
  case BLE_GAP_EVENT_ENC_CHANGE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_ENC_CHANGE));
    break;
  case BLE_GAP_EVENT_PASSKEY_ACTION:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PASSKEY_ACTION));
    break;
  case BLE_GAP_EVENT_NOTIFY_RX:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_NOTIFY_RX));
    break;
  case BLE_GAP_EVENT_NOTIFY_TX:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_NOTIFY_TX));
    break;
  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_SUBSCRIBE));
    break;
  case BLE_GAP_EVENT_MTU:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_MTU));
    break;
  case BLE_GAP_EVENT_IDENTITY_RESOLVED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_IDENTITY_RESOLVED));
    break;
  case BLE_GAP_EVENT_REPEAT_PAIRING:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_REPEAT_PAIRING));
    break;
  case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PHY_UPDATE_COMPLETE));
    break;
  case BLE_GAP_EVENT_EXT_DISC:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_EXT_DISC));
    break;
  case BLE_GAP_EVENT_PERIODIC_SYNC:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PERIODIC_SYNC));
    break;
  case BLE_GAP_EVENT_PERIODIC_REPORT:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PERIODIC_REPORT));
    break;
  case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PERIODIC_SYNC_LOST));
    break;
  case BLE_GAP_EVENT_SCAN_REQ_RCVD:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_SCAN_REQ_RCVD));
    break;
  case BLE_GAP_EVENT_PERIODIC_TRANSFER:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PERIODIC_TRANSFER));
    break;
  case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PATHLOSS_THRESHOLD));
    break;
  case BLE_GAP_EVENT_TRANSMIT_POWER:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_TRANSMIT_POWER));
    break;
  case BLE_GAP_EVENT_PARING_COMPLETE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_PARING_COMPLETE));
    break;
  case BLE_GAP_EVENT_SUBRATE_CHANGE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_SUBRATE_CHANGE));
    break;
  case BLE_GAP_EVENT_VS_HCI:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_VS_HCI));
    break;
  case BLE_GAP_EVENT_BIGINFO_REPORT:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_BIGINFO_REPORT));
    break;
  case BLE_GAP_EVENT_REATTEMPT_COUNT:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_REATTEMPT_COUNT));
    break;
  case BLE_GAP_EVENT_AUTHORIZE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_AUTHORIZE));
    break;
  case BLE_GAP_EVENT_TEST_UPDATE:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_TEST_UPDATE));
    break;
  case BLE_GAP_EVENT_DATA_LEN_CHG:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_DATA_LEN_CHG));
    break;
  case BLE_GAP_EVENT_CONNLESS_IQ_REPORT:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_CONNLESS_IQ_REPORT));
    break;
  case BLE_GAP_EVENT_CONN_IQ_REPORT:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_CONN_IQ_REPORT));
    break;
  case BLE_GAP_EVENT_CTE_REQ_FAILED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_CTE_REQ_FAILED));
    break;
  case BLE_GAP_EVENT_LINK_ESTAB:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(BLE_GAP_EVENT_LINK_ESTAB));
    break;

  default:
    ESP_LOGW(LOG_TAG, "Unhandled event %d", event->type);
  }

  return 0;
}

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
  char buf[BLE_UUID_STR_LEN] = {
    0,
  };

  switch (ctxt->op) {
  case BLE_GATT_REGISTER_OP_SVC:
    ESP_LOGI(LOG_TAG, "reg svc %s handle=%d", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
    break;

  case BLE_GATT_REGISTER_OP_CHR:
    ESP_LOGI(LOG_TAG,
             "reg chr %s "
             "def_handle=%d val_handle=%d",
             ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf), ctxt->chr.def_handle, ctxt->chr.val_handle);
    break;

  case BLE_GATT_REGISTER_OP_DSC:
    ESP_LOGI(LOG_TAG, "reg desc %s handle=%d", ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf), ctxt->dsc.handle);
    break;

  default:
    assert(0);
    break;
  }
}

static int gatt_svr_init(read_ch_cb_t read_cb, write_ch_cb_t write_cb, const struct ble_gatt_svc_def gatt_svr_svcs[])
{
  ble_svc_gap_init();
  ble_svc_gatt_init();

  set_read_chr_cb(read_cb);
  set_write_chr_cb(write_cb);

  int rc = ble_gatts_count_cfg(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  rc = ble_gatts_add_svcs(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  return 0;
}

static void on_reset(int reason)
{
  ESP_LOGE(LOG_TAG, "Reset state reason=%d\n", reason);
}

static void on_sync(void)
{
  int rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  /* Figure out address to use while advertising (no privacy for now). */
  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG, "Cannot determine address type rc=%d\n", rc);
    return;
  }

  uint8_t addr_val[6] = {0};
  rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

  ESP_LOGI(LOG_TAG, "Address %02x:%02x:%02x:%02x:%02x:%02x", addr_val[5], addr_val[4], addr_val[3], addr_val[2],
           addr_val[1], addr_val[0]);

  start_advertising();
}

static void task(void *param)
{
  ESP_LOGI(LOG_TAG, "Task started");
  /* This function will return only when nimble_port_stop() is executed */
  nimble_port_run();
  ESP_LOGI(LOG_TAG, "Task finished");
  nimble_port_freertos_deinit();
}

int em_ble_init(const char *dev_name, read_ch_cb_t read_cb, write_ch_cb_t write_cb,
                const struct ble_gatt_svc_def gatt_svr_svcs[])
{
  if (initialized) {
    ESP_LOGW(LOG_TAG, "Already initialized");
    return ESP_OK;
  }
  /* Below is the sequence of APIs to be called to init/enable NimBLE host and ESP controller. */
  ESP_LOGI(LOG_TAG, "Init %s", dev_name);
  initialized = true;

  esp_err_t ret = nimble_port_init();

  /* Initialize the NimBLE host configuration. */
  ble_hs_cfg.reset_cb = on_reset;
  ble_hs_cfg.sync_cb = on_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  int rc = gatt_svr_init(read_cb, write_cb, gatt_svr_svcs);
  assert(rc == 0);

  /* Set the name of this device. */
  rc = ble_svc_gap_device_name_set(dev_name);
  assert(rc == 0);

  nimble_port_freertos_init(task);
  return ret;
}

// Disable/deinit NimBLE host and ESP controller.
void em_ble_deinit(void)
{
  if (!initialized) {
    return;
  }

  ESP_LOGI(LOG_TAG, "Deinit");
  initialized = false;
  ESP_ERROR_CHECK(esp_event_post(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_DEINIT, NULL, 0, portMAX_DELAY));

  int ret = nimble_port_stop(); // it calls ble_gap_adv_stop internally
  if (ret != 0) {
    ESP_LOGE(LOG_TAG, "nimble_port_stop fail err=%d", ret);
  }

  nimble_port_deinit();
}
