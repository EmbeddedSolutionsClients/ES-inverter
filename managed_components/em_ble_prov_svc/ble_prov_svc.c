/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <stdbool.h>

#include "em/ble_core.h"
#include "em/ble_prov_svc.h"
#include "em/scheduler.h"
#include "em/wifi.h"

#define LOG_TAG "EM_PROV_SVC"

static char ble_name[32] = {0};
static uint32_t disconnection_delay = 60000;
static uint64_t dev_id;

static em_wifi_credentials_t wifi_credentials = {
  .ssid = {0},
  .ssid_set = false,
  .password = {0},
  .password_set = false,
};

#define CONNECTION_NONE            (-1)
#define CONNECTION_IN_PROGRESS     (-2)
#define CONNECTION_SUCCESSFUL      (-3)
#define DISCONNECTED_STATUS_STRING "Disconnected"
#define CONNECTING_STATUS_STRING   "Connecting..."

static char wifi_conn_status[36] = DISCONNECTED_STATUS_STRING;
static int8_t wifi_conn_result = CONNECTION_NONE;
static uint8_t selected_ap_id = 0;
static em_wifi_ap_list_t ap_list = {0};

uint16_t attr_handle_chr_wifi_ssid;
uint16_t attr_handle_chr_wifi_pass;
uint16_t attr_handle_chr_wifi_connection;
uint16_t attr_handle_chr_wifi_dev_id;
uint16_t attr_handle_chr_wifi_aps;
uint16_t attr_handle_chr_wifi_ap_id;

// c9af9c76-46de-11ed-b878-0242ac120002
const ble_uuid128_t gatt_svr_chr_uuid_ssid =
  BLE_UUID128_INIT(0x02, 0x00, 0x12, 0xac, 0x42, 0x02, 0x78, 0xb8, 0xed, 0x11, 0xde, 0x46, 0x76, 0x9c, 0xaf, 0xc9);

// edca0f5c-8b82-403a-800c-8fd340104481
const ble_uuid128_t gatt_svr_chr_uuid_pass =
  BLE_UUID128_INIT(0x81, 0x44, 0x10, 0x40, 0xd3, 0x8f, 0x0c, 0x80, 0x3a, 0x40, 0x82, 0x8b, 0x5c, 0x0f, 0xca, 0xed);

// write anything to start connection with WIFI, when read returns connection status (disconnect reason)
// TODO: add sending notifications when value changes
// 8c499196-d899-45b8-a23c-2fbfa88947d1
const ble_uuid128_t gatt_svr_chr_uuid_connect =
  BLE_UUID128_INIT(0xd1, 0x47, 0x89, 0xa8, 0xbf, 0x2f, 0x3c, 0xa2, 0xb8, 0x45, 0x99, 0xd8, 0x96, 0x91, 0x49, 0x8c);

// 8c499196-d899-45b8-a23c-2fbfa88947d2
const ble_uuid128_t gatt_svr_chr_uuid_devid =
  BLE_UUID128_INIT(0xd2, 0x47, 0x89, 0xa8, 0xbf, 0x2f, 0x3c, 0xa2, 0xb8, 0x45, 0x99, 0xd8, 0x96, 0x91, 0x49, 0x8c);

// info about AP in format [RSSI][CHANNEL][SSID_LENGTH][SSID]
// RSSI 1B, CHANNEL 1B, SSID_LENGTH 1B, SSID SSID_LENFTH, mAX 32
// AP presented here is selected by gatt_svr_chr_uuid_ap_id
// consider using notifications after wifi scan is done
// 8d499196-d899-45b8-a23c-2fbfa88947d3
const ble_uuid128_t gatt_svr_chr_uuid_aps =
  BLE_UUID128_INIT(0xd3, 0x47, 0x89, 0xa8, 0xbf, 0x2f, 0x3c, 0xa2, 0xb8, 0x45, 0x99, 0xd8, 0x96, 0x91, 0x49, 0x8c);

// selected AP presented in chr gatt_svr_chr_uuid_aps
// 8d499196-d899-45b8-a23c-2fbfa88947d4
const ble_uuid128_t gatt_svr_chr_uuid_ap_id =
  BLE_UUID128_INIT(0xdd, 0x47, 0x89, 0xa8, 0xbf, 0x2f, 0x3c, 0xa2, 0xb8, 0x45, 0x99, 0xd8, 0x96, 0x91, 0x49, 0x8c);

// b2bbc642-46da-11ed-b878-0242ac120002
const ble_uuid128_t gatt_svr_svc_uuid =
  BLE_UUID128_INIT(0x02, 0x00, 0x12, 0xac, 0x42, 0x02, 0x78, 0xb8, 0xed, 0x11, 0xda, 0x46, 0x42, 0xc6, 0xbb, 0xb2);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &gatt_svr_svc_uuid.u,
    .characteristics =
      (struct ble_gatt_chr_def[]){
        {
          .uuid = &gatt_svr_chr_uuid_ssid.u, // UUID as given above
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_ssid,
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE, // flags set permissions. In this case User can read this
                                                               // characteristic, can write to it,and get notified.
        },
        {
          .uuid = &gatt_svr_chr_uuid_pass.u,
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_pass,
          .flags = BLE_GATT_CHR_F_WRITE,
        },
        {
          .uuid = &gatt_svr_chr_uuid_connect.u,
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_connection,
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        },
        {
          .uuid = &gatt_svr_chr_uuid_devid.u,
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_dev_id,
          .flags = BLE_GATT_CHR_F_READ,
        },
        {
          .uuid = &gatt_svr_chr_uuid_aps.u,
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_aps,
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        },
        {
          .uuid = &gatt_svr_chr_uuid_ap_id.u,
          .access_cb = default_chr_access_cb,
          .val_handle = &attr_handle_chr_wifi_ap_id,
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        },
        {
          0, // No more characteristics in this service. This is necessary
        }},
  },
  {
    0, // No more services. This is necessary
  },
};

static void wifi_disconnected_hnd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(event_base);

  wifi_conn_result = CONNECTION_NONE;
  memset(wifi_conn_status, 0, sizeof(wifi_conn_status));
  memcpy(wifi_conn_status, (const char *)event_data, strnlen(event_data, sizeof(wifi_conn_status)));
}

static void wifi_connected_hnd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(event_base);

  em_wifi_connected_evt_t *evt = (em_wifi_connected_evt_t *)event_data;
  memset(wifi_conn_status, 0, sizeof(wifi_conn_status));
  sprintf(wifi_conn_status, "Connected  %d.%d.%d.%d", evt->local_ip[0], evt->local_ip[1], evt->local_ip[2],
          evt->local_ip[3]);

  wifi_conn_result = CONNECTION_SUCCESSFUL;
}

static void wifi_scan_done_hnd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ap_list.len = ((em_wifi_ap_list_t *)event_data)->len;
  ESP_LOGI(LOG_TAG, "WiFi scan done, found %d APs", ap_list.len);

  memcpy(&ap_list.aps, ((em_wifi_ap_list_t *)event_data)->aps, ap_list.len * sizeof(em_wifi_ap_t));

  for (uint16_t i = 0; i < ap_list.len; i++) {
    ESP_LOGI(LOG_TAG, "[%d] RSSI=%d SSID=%s", i, ap_list.aps[i].rssi, ap_list.aps[i].ssid);
  }

  if (wifi_conn_result == CONNECTION_IN_PROGRESS || wifi_conn_result == CONNECTION_SUCCESSFUL) {
    return;
  }

  esp_err_t ret = (esp_err_t)em_ble_prov_start(UINT32_MAX);

  if (ret != ESP_OK) {
    ESP_LOGE(LOG_TAG, "Prov service start %s", esp_err_to_name(ret));
  }
}

int read_chr_handler(uint16_t attr_handle, void *chr_val)
{
  int ret = 0;

  if (attr_handle == attr_handle_chr_wifi_ssid) {
    ret = strnlen(wifi_credentials.ssid, sizeof(wifi_credentials.ssid));
    if (ret == 0) {
      ret = 1;
      ((uint8_t *)chr_val)[0] = 0; // empty ssid
    } else {
      memcpy(chr_val, wifi_credentials.ssid, ret);
    }
  } else if (attr_handle == attr_handle_chr_wifi_pass) {
    // forbidden to read password
    return BLE_ATT_ERR_READ_NOT_PERMITTED;
  } else if (attr_handle == attr_handle_chr_wifi_connection) {
    ret = sizeof(wifi_conn_status);
    size_t str_len = strnlen(wifi_conn_status, sizeof(wifi_conn_status));
    memset(chr_val, 0, sizeof(wifi_conn_status));
    memcpy(chr_val, wifi_conn_status, str_len);
  } else if (attr_handle == attr_handle_chr_wifi_dev_id) {
    ret = sizeof(uint64_t);
    uint8_t *dev_id_chr_ptr = (uint8_t *)chr_val;
    dev_id_chr_ptr[0] = dev_id & 0xFF;
    dev_id_chr_ptr[1] = (dev_id >> 8) & 0xFF;
    dev_id_chr_ptr[2] = (dev_id >> 16) & 0xFF;
    dev_id_chr_ptr[3] = (dev_id >> 24) & 0xFF;
    dev_id_chr_ptr[4] = (dev_id >> 32) & 0xFF;
    dev_id_chr_ptr[5] = (dev_id >> 40) & 0xFF;
    dev_id_chr_ptr[6] = (dev_id >> 48) & 0xFF;
    dev_id_chr_ptr[7] = (dev_id >> 56) & 0xFF;
  } else if (attr_handle == attr_handle_chr_wifi_aps) {
    if (selected_ap_id >= ap_list.len) {
      // when no entry send empty record
      char *write_ptr = (char *)chr_val;
      *write_ptr = 0;       // rssi
      *(write_ptr + 1) = 0; // channel
      *(write_ptr + 2) = 0; // ssid length
      return 3;
    }

    size_t offset = 0;
    char *write_ptr = (char *)chr_val;

    write_ptr[offset++] = sizeof(ap_list.aps[selected_ap_id].rssi);
    write_ptr[offset++] = sizeof(ap_list.aps[selected_ap_id].channel);
    uint8_t ssid_len = strlen(ap_list.aps[selected_ap_id].ssid);
    write_ptr[offset++] = ssid_len;

    memcpy(write_ptr + offset, ap_list.aps[selected_ap_id].ssid, ssid_len);
    offset += ssid_len;

    ret = offset;
  } else if (attr_handle == attr_handle_chr_wifi_ap_id) {
    if (selected_ap_id >= CONFIG_EM_WIFI_MAX_SCAN_RECORDS) {
      ESP_LOGI(LOG_TAG, "Invalid AP ID");
      return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }

    ret = sizeof(selected_ap_id);
    *(int *)chr_val = selected_ap_id;
  } else {
    ESP_LOGE(LOG_TAG, "Unknown chr=%x", attr_handle);
    return -1;
  }

  return ret;
}

int write_chr_handler(uint16_t attr_handle, const void *chr_val, uint16_t chr_len)
{
  if (attr_handle == attr_handle_chr_wifi_ssid) {
    memset(wifi_credentials.ssid, 0, sizeof(wifi_credentials.ssid));
    memcpy(wifi_credentials.ssid, chr_val, chr_len);
    wifi_credentials.ssid_set = true;
  } else if (attr_handle == attr_handle_chr_wifi_pass) {
    memset(wifi_credentials.password, 0, sizeof(wifi_credentials.password));
    memcpy(wifi_credentials.password, chr_val, chr_len);
    wifi_credentials.password_set = true;
  } else if (attr_handle == attr_handle_chr_wifi_connection) {
    if (wifi_credentials.ssid_set == false || wifi_credentials.password_set == false) {
      ESP_LOGI(LOG_TAG, "SSID or password not set");
      return 0;
    }

    size_t pass_len = strnlen(wifi_credentials.password, sizeof(wifi_credentials.password));
    size_t ssid_len = strnlen(wifi_credentials.ssid, sizeof(wifi_credentials.ssid));

    ESP_LOGI(LOG_TAG, "Provisioned with ssid=%s (%d) pass=%s (%d)", wifi_credentials.ssid, ssid_len,
             wifi_credentials.password,
             pass_len);                        // Print the received value
    wifi_conn_result = CONNECTION_IN_PROGRESS; // start connecting
    em_wifi_sta_connect(wifi_credentials.ssid, wifi_credentials.password);

    memset(wifi_conn_status, 0, sizeof(wifi_conn_status));
    strcpy(wifi_conn_status, CONNECTING_STATUS_STRING);

  } else if (attr_handle == attr_handle_chr_wifi_dev_id) {
    return BLE_ATT_ERR_WRITE_NOT_PERMITTED; // forbidden to write
  } else if (attr_handle == attr_handle_chr_wifi_ap_id) {
    if (chr_len != sizeof(selected_ap_id)) {
      ESP_LOGI(LOG_TAG, "Invalid AP ID length");
      return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
    }

    selected_ap_id = *(uint8_t *)chr_val;
    ESP_LOGI(LOG_TAG, "Selected AP ID=%d", selected_ap_id);
  } else {
    return BLE_ATT_ERR_UNLIKELY;
  }

  return 0;
}

static void prov_stop_wrapper(uint32_t param, void *user_ctx)
{
  em_ble_prov_stop();
}

static void em_ble_conn_hnd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  scheduler_cancel_callback(prov_stop_wrapper);
}

static void em_ble_disconn_hnd(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  scheduler_set_callback(prov_stop_wrapper, SCH_PARAM_NONE, SCH_CTX_NONE, disconnection_delay);
}

void em_ble_prov_init(const char *dev_name, uint64_t id)
{
  dev_id = id;
  memset(ble_name, 0, sizeof(ble_name));
  size_t dev_id_str_len = sizeof(dev_id) * 2;                  // 2 hex digits per byte
  size_t max_name_len = sizeof(ble_name) - dev_id_str_len - 2; // leave space for '_' and NULL
  size_t str_len = strnlen(dev_name, max_name_len);
  memcpy(ble_name, dev_name, str_len);
  ble_name[str_len] = '_';

  snprintf(ble_name + str_len + 1, dev_id_str_len, "%02llX", dev_id);
  ESP_LOGI(LOG_TAG, "BLE name: %s", ble_name);

  ESP_ERROR_CHECK(esp_event_handler_register(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_CONNECTED, &wifi_connected_hnd, NULL));
  ESP_ERROR_CHECK(
    esp_event_handler_register(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_DISCONNECTED, &wifi_disconnected_hnd, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(EM_WIFI_EVENT, EM_WIFI_EVENT_SCAN_DONE, &wifi_scan_done_hnd, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_CONNECTED, &em_ble_conn_hnd, NULL));
  ESP_ERROR_CHECK(
    esp_event_handler_register(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_DISCONNECTED, &em_ble_disconn_hnd, NULL));
}

void em_ble_prov_deinit(void)
{
  ESP_ERROR_CHECK(esp_event_handler_unregister(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_CONNECTED, &wifi_connected_hnd));
  ESP_ERROR_CHECK(esp_event_handler_unregister(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_DISCONNECTED, &wifi_disconnected_hnd));
  ESP_ERROR_CHECK(esp_event_handler_unregister(EM_WIFI_EVENT, EM_WIFI_EVENT_SCAN_DONE, &wifi_scan_done_hnd));
  ESP_ERROR_CHECK(esp_event_handler_unregister(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_CONNECTED, &em_ble_conn_hnd));
  ESP_ERROR_CHECK(esp_event_handler_unregister(EM_BLE_CORE_EVENT, EM_BLE_CORE_EVENT_DISCONNECTED, &em_ble_disconn_hnd));
}

int em_ble_prov_start(uint32_t disconn_delay)
{
  int ret = (int)em_ble_init(ble_name, read_chr_handler, write_chr_handler, gatt_svr_svcs);

  if (ret == 0 && disconn_delay != UINT32_MAX) {
    disconnection_delay = disconn_delay;
    scheduler_set_callback(prov_stop_wrapper, SCH_PARAM_NONE, SCH_CTX_NONE, disconnection_delay);
  }

  return ret;
}

void em_ble_prov_stop(void)
{
  em_ble_deinit();
}
