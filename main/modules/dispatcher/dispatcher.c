/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/ble_prov_svc.h"
#include "em/coredump.h"
#include "em/evt.h"
#include "em/http_ota.h"
#include "em/protocol.h"
#include "em/scheduler.h"
#include "em/storage.h"
#include "em/tcp_client.h"
#include "em/update.h"
#include "em/utils.h"
#include "em/wifi.h"

#include <stdlib.h>
#include <esp_err.h>
#include <esp_macros.h>
#include <esp_random.h>
#include <esp_system.h>
#include <common.h>

#include <esp_log.h>
#define LOG_TAG "DIP"

typedef int (*disp_cb_t)(void *data);

typedef struct {
  disp_cb_t cb;
  msg_type_t type;
} tcp_mapper_t;

void __attribute__((weak)) app_confirm_handler(void);

static int wireless_enable_handle(void *data);
static int wireless_set_credentials_handle(void *data);
static int wireless_set_tx_power_handle(void *data);
static int wireless_set_channel_handle(void *data);

static void authentication_delayed_handler(uint32_t param, void *usr_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(usr_ctx);

  coredump_start((coredump_api_t){
    .valid = protocol_send_diag_coredump_start,
    .body = protocol_send_diag_coredump_body,
    .end = protocol_send_diag_coredump_end,
  });

  app_confirm_handler();
}

static esp_err_t connection_status_handler(void *data)
{
  assert(data);

  connection_status_msg_t *msg = (connection_status_msg_t *)data;
  ESP_LOGI(LOG_TAG, "Assigned uniqueID=%lX", msg->unique_id);
  ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_AUTH, &msg->unique_id, sizeof(msg->unique_id), 0));
  // image has to be confirmed before image rollaout timeout APP_IMAGE_CONFIRM_TIMEOUT_MS by default 60s
  const uint32_t delay_ms = (esp_random() & 0x7FFF) + 10000; // 10s + <0,32s> random
  scheduler_set_callback(authentication_delayed_handler, SCH_PARAM_NONE, SCH_CTX_NONE, delay_ms);
  storage_set_device_unique_id(msg->unique_id);
  return ESP_OK;
}

// TODO: this is duplicated function, make it global and move to managed_components
static void delayed_esp_reboot(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  esp_restart();
}

static void delayed_esp_factory_reset(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  storage_set_factory_config();
  em_wifi_factory_reset();

  esp_restart();
}

// TODO: create one function for reboots and fr and place it in managed_components ?
static esp_err_t reboot_handler(void *data)
{
  ESP_UNUSED(data);

  ESP_LOGW(LOG_TAG, "Reboot device");
  scheduler_set_callback(delayed_esp_reboot, SCH_PARAM_NONE, SCH_CTX_NONE, CONFIG_EM_TCP_REBOOT_ON_MSG_DELAY_MS);
  return ESP_OK;
}

static esp_err_t factory_reset_handler(void *data)
{
  ESP_UNUSED(data);

  ESP_LOGW(LOG_TAG, "Factory reset device");
  scheduler_set_callback(delayed_esp_factory_reset, SCH_PARAM_NONE, SCH_CTX_NONE, CONFIG_EM_TCP_REBOOT_ON_MSG_DELAY_MS);
  return ESP_OK;
}

static esp_err_t update_start_handler(void *data)
{
  assert(data);

  em_ble_prov_stop(); // not enough RAM for heap allocations

  update_start_msg_t *msg = (update_start_msg_t *)data;
  return update_start(http_ota_submit_start, msg->dev_type, msg->fw_ver, msg->hw_ver, msg->img_url, msg->img_url_len, msg->update_policy);
}

static esp_err_t update_apply_handler(void *data)
{
  assert(data);

  update_install_msg_t *msg = (update_install_msg_t *)data;
  ESP_LOGI(LOG_TAG, "Apply FW=%d.%d.%d delay=%ld", (uint8_t)(msg->fw_ver >> 24), (uint8_t)(msg->fw_ver >> 16), (uint16_t)(msg->fw_ver), msg->delay);
  update_apply(msg->delay);
  return ESP_OK;
}

static int environment_cloud_cover_handler(void *data)
{
  ESP_UNUSED(data);
  return ESP_OK;
}

static int environment_set_position_handler(void *data)
{
  assert(data);

  environment_position_msg_t *msg = (environment_position_msg_t *)data;
  /*Latitude -90 to 90 , Longitude -180, 180 */
  int32_t multiplier = 1;
  int32_t divisor = 1;
  em_utils_decode_scaler(COORDINATES_FIXED_FACTOR, &multiplier, &divisor);
  const double lati = (msg->latitude * multiplier) / divisor;
  const double longi = (msg->longitude * multiplier) / divisor;

  if ((lati < -90.f) || (lati > 90.f) || (longi < -180.f) || (longi > 180.f)) {
    ESP_LOGE(LOG_TAG, "%s %.5f %.5f", __func__, lati, longi);
    return ESP_ERR_INVALID_ARG;
  }

  storage_set_coordinates(msg->latitude, msg->longitude);
  return ESP_OK;
}

static int coredump_confirmed_handler(void *data)
{
  ESP_UNUSED(data);

  coredump_erase();
  return ESP_OK;
}

static int status_handler(void *data)
{
  assert(data);

  status_msg_t *msg = (status_msg_t *)data;

  switch (msg->rq_type) {
  case MSGTYPE_CLIENT_INFO:
    ESP_LOGW(LOG_TAG, "Client info rejected err=%d", msg->error);
    em_tcp_client_set_long_reconnecting_delay();

    if (msg->error == 3) { // TODO: add enums for errors
      storage_set_device_unique_id(0);
      return ESP_OK;
    }
    break;
  case MSGTYPE_ENERGYPRICE_DAY_GET:
    ESP_LOGW(LOG_TAG, "Energy price day get error=%d", msg->error);
    break;
  default:
    ESP_LOGW(LOG_TAG, "status msg rq=0x%x err=%d", msg->rq_type, msg->error);
  }

  return ESP_OK;
}

static int get_handler(void *data)
{
  assert(data);
  get_msg_t *msg = (get_msg_t *)data;

  switch (msg->rq_type) {
  case MSGTYPE_UPDATE_INSTALLEDFW:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_UPDATE_INSTALLEDFW));
    update_send_installed_fw();
    break;

  case MSGTYPE_UPDATE_STATUS:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_UPDATE_STATUS));
    send_update_status();
    break;

  case MSGTYPE_WIRELESS_STATUS:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_WIRELESS_STATUS));
    int rssi = INT8_MIN;

    if (em_wifi_rssi(&rssi) != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get WiFi rssir");
      return -2;
    }

    protocol_send_wireless_status(PROTOCOL_WIFI, true, em_wifi_local_ip_addr(), (int8_t)rssi, 0xFF);
    break;

  case MSGTYPE_WIRELESS_CREDENTIALS:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_WIRELESS_CREDENTIALS));
    char ssid[33] = {0};
    if (em_wifi_sta_ssid(ssid) != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get WiFi SSID");
      return ESP_FAIL;
    }
    protocol_send_wireless_credentials(PROTOCOL_WIFI, ssid, "");
    break;

  case MSGTYPE_WIRELESS_TX_POWER:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_WIRELESS_TX_POWER));
    int8_t tx_power = INT8_MIN;

    if (em_wifi_tx_power(&tx_power) != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get WiFi tx power");
      return ESP_FAIL;
    }
    protocol_send_wireless_tx_power(PROTOCOL_WIFI, 4 * tx_power); //  2.5 * 4 = 10, send in 0.1dBm
    break;

  case MSGTYPE_WIRELESS_CHANNEL:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_WIRELESS_CHANNEL));
    uint8_t chan = UINT8_MAX;

    if (em_wifi_channel(&chan) != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get WiFi channel");
      return ESP_FAIL;
    }

    protocol_send_wireless_channel(PROTOCOL_WIFI, chan);
    break;

  case MSGTYPE_ENVIRONMENT_POSITION:
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_ENVIRONMENT_POSITION));
    coordinates_t coord = storage_device().perm.coord;
    protocol_send_environment_position(coord.latitude, coord.longitude);
    break;

  case MSGTYPE_METER_MEASUREMENT: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_METER_MEASUREMENT));
    //return send_relay_measurements();
  } break;

  case MSGTYPE_METER_MEASUREMENT_HISTORY: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_METER_MEASUREMENT_HISTORY));
    switch (msg->param) {
    case MEASTYPE_VOLTAGE_RMS:
      //send_relay_voltage_history();
      break;
    case MEASTYPE_POWER_ACTIVE:
      //send_relay_power_history();
      break;
    default:
      return -2;
    }
  } break;

  case MSGTYPE_METER_CURRENT_LIMIT: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_METER_CURRENT_LIMIT));
    //send_relay_meter_current_limit();
  } break;

  case MSGTYPE_ENERGY_ACCUMULATED: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_ENERGY_ACCUMULATED));
    //send_relay_energy_accumulated();
  } break;

  case MSGTYPE_ENERGY_HISTORY: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_ENERGY_HISTORY));
   // send_relay_energy_history();
  } break;

  case MSGTYPE_DIAG_LOGS_SETTINGS: {
    ESP_LOGI(LOG_TAG, "Get %s", __STRINGIFY(MSGTYPE_DIAG_LOGS_SETTINGS));
    uint64_t moduls_levels[6] = {
      0,
    };
    const uint8_t medium_uart = 1;
    protocol_send_diag_logs_settings(medium_uart, moduls_levels);
  } break;

  default:
    ESP_LOGW(LOG_TAG, "Unsupported requested msg_type=0x%x ", msg->rq_type);
    return ESP_ERR_NOT_FOUND;
  }

  return ESP_OK;
}

// clang-format off
static tcp_mapper_t mapper[] = {
  {.cb = connection_status_handler, .type = MSGTYPE_CONNECTION_STATUS},
  {.cb = reboot_handler, .type = MSGTYPE_REBOOT},
  {.cb = factory_reset_handler, .type = MSGTYPE_FACTORY_RESET},
  {.cb = led_evt_identify_msg_handler, .type = MSGTYPE_IDENTIFY},
  {.cb = update_start_handler, .type = MSGTYPE_UPDATE_START},
  {.cb = update_apply_handler, .type = MSGTYPE_UPDATE_INSTALL},
  {.cb = wireless_enable_handle, .type = MSGTYPE_WIRELESS_ENABLE},
  {.cb = wireless_set_credentials_handle, .type = MSGTYPE_WIRELESS_SET_CREDENTIALS},
  {.cb = wireless_set_tx_power_handle, .type = MSGTYPE_WIRELESS_SET_TX_POWER},
  {.cb = wireless_set_channel_handle, .type = MSGTYPE_WIRELESS_SET_CHANNEL},
  {.cb = environment_cloud_cover_handler, .type = MSGTYPE_ENVIRONMENT_CLOUD_COVER},
  {.cb = environment_set_position_handler, .type = MSGTYPE_ENVIRONMENT_SET_POSITION},
  {.cb = coredump_confirmed_handler, .type = MSGTYPE_DIAG_COREDUMP_CONFIRMED},
  {.cb = status_handler, .type = MSGTYPE_STATUS},
  {.cb = get_handler, .type = MSGTYPE_GET},
};
// clang-format on

int dispatcher_handler(msg_type_t type, void *data)
{
  /* Go through registered handlers */
  for (uint32_t i = 0; i < ARRAY_LENGTH(mapper); i++) {
    if (type == mapper[i].type) {
      return mapper[i].cb(data);
    }
  }

  return ESP_ERR_NOT_SUPPORTED;
}

static int wireless_enable_handle(void *data)
{
  assert(data);
  wireless_enable_msg_t *msg = (wireless_enable_msg_t *)data;
  ESP_LOGI(LOG_TAG, "RX wireless enable %d proto=%d", msg->enabled, msg->protocol);

  if (msg->protocol != PROTOCOL_BLE) {
    return ESP_ERR_INVALID_ARG;
  }

  if (msg->enabled) {
    return em_ble_prov_start(60000);
  } else {
    em_ble_prov_stop();
  }

  return ESP_OK;
}

static int wireless_set_credentials_handle(void *data)
{
  assert(data);
  wireless_credentials_msg_t *msg = (wireless_credentials_msg_t *)data;
  ESP_LOGI(LOG_TAG, "RX wireless credentials proto=%d ssid=%s", msg->protocol, msg->network);
  if (msg->protocol != PROTOCOL_WIFI) {
    return ESP_ERR_INVALID_ARG;
  }

  em_wifi_sta_connect((const char *)msg->network, (const char *)msg->password);
  return ESP_OK;
}

// TODO: add BLE support
static int wireless_set_tx_power_handle(void *data)
{
  assert(data);
  wireless_tx_power_msg_t *msg = (wireless_tx_power_msg_t *)data;
  ESP_LOGI(LOG_TAG, "RX wireless proto=%d power=%d", msg->protocol, msg->tx_power);

  if (msg->protocol != PROTOCOL_WIFI) {
    return ESP_ERR_INVALID_ARG;
  }

  if (msg->tx_power < 20 || msg->tx_power > 210) {
    ESP_LOGW(LOG_TAG, "Invalid tx power");
    return ESP_ERR_INVALID_ARG;
  }

  return em_wifi_set_tx_power((uint8_t)(4 * msg->tx_power / 10)); // convert 0.1dBm to 0.25 dBm
}

static int wireless_set_channel_handle(void *data)
{
  assert(data);
  wireless_channel_msg_t *msg = (wireless_channel_msg_t *)data;
  ESP_LOGI(LOG_TAG, "RX wireles proto=%d channel=%d", msg->protocol, msg->channel);

  if (msg->protocol != PROTOCOL_WIFI) {
    return ESP_ERR_INVALID_ARG;
  }

  int ret = em_wifi_set_channel(msg->channel);
  return ret;
}
