/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/wifi.h"
#include "em/wifi_priv.h"

#include <string.h>
// #include <esp_mac.h>
#include <esp_wifi.h>

// TODO: implement
/* MACSTR macro output length, see esp_mac.h MACSTR for reference */
// #define MACSTR_LEN        17
// #define BASE_MAC_ADDR_LEN 6

ESP_EVENT_DEFINE_BASE(EM_WIFI_EVENT);

EventGroupHandle_t wifi_evt_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(event_base);

  switch (event_id) {
  case WIFI_EVENT_STA_DISCONNECTED: {
    xEventGroupClearBits(wifi_evt_group, CONNECTED_BIT);

    /* Try to reconnect endlessly with one exception that device disconnected on demand.
     * Keep it simple and brutal, without WiFi device cannot be fixed remotely.
     *
     * WIFI_EVENT_STA_DISCONNECTED has to be indicated to allow reprovision the device due to credentials change
     * or other reasons.
     */

    if (em_wifi_sta_is_disconnected_on_demand()) {
      ESP_LOGI(LOG_TAG, "Disconnected on demand");
      xEventGroupClearBits(wifi_evt_group, DISCONNECT_DEMAND_BIT);
      return;
    }

    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
    ESP_LOGI(LOG_TAG, "Disconnected reason=%u reconnect", disconnected->reason);
    const char *reason_str = reason_to_string(disconnected->reason);
    ESP_ERROR_CHECK(
      esp_event_post(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_DISCONNECTED, reason_str, strlen(reason_str) + 1, 0));
    esp_err_t err = esp_wifi_connect();

    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
      ESP_LOGE(LOG_TAG, "Failed to reconnect: %s", esp_err_to_name(err));
      assert(false);
    }
    break;
  }

  case WIFI_EVENT_STA_CONNECTED: {
    // This event comes when connected to AP but without assigned IP so the device is not operational yet
    break;
  }

  case WIFI_EVENT_SCAN_DONE: {
    scan_done_handler();
    break;
  }

  case WIFI_EVENT_STA_START: {
    ESP_LOGI(LOG_TAG, "WIFI_EVENT_STA_START");
    xEventGroupSetBits(wifi_evt_group, STARTED_BIT);
    break;
  }

  case WIFI_EVENT_STA_STOP: {
    ESP_LOGI(LOG_TAG, "WIFI_EVENT_STA_STOP");
    break;
  }

  case WIFI_EVENT_WIFI_READY: {
    ESP_LOGI(LOG_TAG, "WIFI_EVENT_WIFI_READY");
    break;
  }

  case WIFI_EVENT_HOME_CHANNEL_CHANGE: {
    wifi_event_home_channel_change_t *home_channel_change = (wifi_event_home_channel_change_t *)event_data;
    ESP_LOGI(LOG_TAG, "WIFI_EVENT_HOME_CHANNEL_CHANGE: %d -> %d", home_channel_change->old_chan,
             home_channel_change->new_chan);
    break;
  }

  default:
    ESP_LOGW(LOG_TAG, "Unknown base=%s event=%ld (MAX=%d)", event_base, event_id, WIFI_EVENT_MAX);
    break;
  }
}

static void set_credentials(const char *ssid, const char *password)
{
  size_t ssid_len = strnlen(ssid, 32);
  size_t password_len = strnlen(password, 64);

  wifi_config_t wifi_cfg = {.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK, .sta.channel = 1};

  assert(ssid_len < sizeof(wifi_cfg.sta.ssid));
  assert(password_len < sizeof(wifi_cfg.sta.password));

  memset(wifi_cfg.sta.ssid, 0, sizeof(wifi_cfg.sta.ssid));
  memset(wifi_cfg.sta.password, 0, sizeof(wifi_cfg.sta.password));
  memcpy(wifi_cfg.sta.ssid, ssid, ssid_len);
  memcpy(wifi_cfg.sta.password, password, password_len);

  ESP_ERROR_CHECK(esp_wifi_set_storage(CONFIG_EM_WIFI_CREDENTIALS_STORAGE));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
  (void)xEventGroupSetBits(wifi_evt_group, PROVISIONED_BIT);
}

static void ip_assigned_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(base);
  ESP_UNUSED(event_id);
  ESP_UNUSED(data);

  const ip_event_got_ip_t *ip_event = (const ip_event_got_ip_t *)data;

  /* this event may appear when IP has changed, we want to ignore it*/
  if (em_wifi_sta_is_connected(0)) {
    ESP_LOGI("Changed IP=%s", IPSTR, IP2STR(&ip_event->ip_info.ip));
    return;
  }

  em_wifi_connected_evt_t evt;
  memcpy(evt.local_ip, &ip_event->ip_info.ip, sizeof(ip_event->ip_info.ip));
  memcpy(evt.gw_ip, &ip_event->ip_info.gw, sizeof(ip_event->ip_info.gw));

  (void)xEventGroupSetBits(wifi_evt_group, CONNECTED_BIT);
  ESP_ERROR_CHECK(esp_event_post(EM_WIFI_EVENT, EM_WIFI_EVENT_STA_CONNECTED, &evt, sizeof(evt), 0));
}

static em_wifi_credentials_t get_credentials(void)
{
  em_wifi_credentials_t ret = {
    0,
  };

  wifi_config_t wifi_cfg;
  ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));
  int val = strnlen((const char *)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid));

  if (val == 0) {
    return ret;
  }

  memcpy(ret.ssid, wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid));
  memcpy(ret.password, wifi_cfg.sta.password, sizeof(wifi_cfg.sta.password));
  ret.ssid_set = true;
  ret.password_set = true;

  return ret;
}

void em_wifi_sta_init(wifi_ps_type_t power_save_mode, bool start_scan)
{
  static StaticEventGroup_t wifi_evt_group_data = {0};
  wifi_evt_group = xEventGroupCreateStatic(&wifi_evt_group_data);
  // Check for insufficient FreeRTOS heap available
  assert(wifi_evt_group);

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_assigned_handler, NULL));

  ESP_ERROR_CHECK(esp_netif_init());
  (void)esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_ps(power_save_mode));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  (void)xEventGroupWaitBits(wifi_evt_group, STARTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  em_wifi_credentials_t cred = get_credentials();

  if (cred.ssid_set && cred.password_set) {
    ESP_ERROR_CHECK(esp_event_post(EM_WIFI_EVENT, EM_WIFI_EVENT_PROVISIONED, NULL, 0, 0));
    xEventGroupSetBits(wifi_evt_group, PROVISIONED_BIT);
    em_wifi_sta_connect(cred.ssid, cred.password);
    return;
  }

  ESP_ERROR_CHECK(esp_event_post(EM_WIFI_EVENT, EM_WIFI_EVENT_NOT_PROVISIONED, NULL, 0, 0));
  if (start_scan) {
    em_wifi_scan_start();
  }
}

void em_wifi_sta_connect(const char *ssid, const char *password)
{
  xEventGroupClearBits(wifi_evt_group, DISCONNECT_DEMAND_BIT);

  bool was_provisioned = em_wifi_sta_is_provisioned();

  set_credentials(ssid, password);

  if (was_provisioned) {
    ESP_LOGI(LOG_TAG, "Change WiFi credentials to SSID=%s", ssid);

    if (em_wifi_sta_is_connected(0)) {
      ESP_ERROR_CHECK(esp_wifi_disconnect());
      return; // DISCONNECT event handler will reconnect
    }
  }

  ESP_LOGI(LOG_TAG, "Connect to SSID=%s", ssid);
  esp_err_t err = esp_wifi_connect();

  if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
    ESP_LOGE(LOG_TAG, "Failed to connect: %s", esp_err_to_name(err));
    assert(false);
  }
}

void em_wifi_sta_disconnect(void)
{
  xEventGroupSetBits(wifi_evt_group, DISCONNECT_DEMAND_BIT);
  ESP_ERROR_CHECK(esp_wifi_disconnect());
}

void em_wifi_factory_reset(void)
{
  em_wifi_sta_disconnect();

  wifi_config_t wifi_cfg = {0};
  ESP_ERROR_CHECK(esp_wifi_set_storage(CONFIG_EM_WIFI_CREDENTIALS_STORAGE));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

  ESP_LOGI(LOG_TAG, "WiFi factory reset done");
}

bool em_wifi_sta_is_connected(TickType_t xTicksToWait)
{
  const EventBits_t evt_bits = xEventGroupWaitBits(wifi_evt_group, CONNECTED_BIT, pdFALSE, pdTRUE, xTicksToWait);
  return (evt_bits & CONNECTED_BIT) != 0;
}

bool em_wifi_sta_is_disconnected_on_demand(void)
{
  const EventBits_t evt_bits = xEventGroupWaitBits(wifi_evt_group, DISCONNECT_DEMAND_BIT, pdFALSE, pdTRUE, 0);
  return (evt_bits & DISCONNECT_DEMAND_BIT) != 0;
}

bool em_wifi_sta_is_provisioned(void)
{
  const EventBits_t evt_bits = xEventGroupWaitBits(wifi_evt_group, PROVISIONED_BIT, pdFALSE, pdTRUE, 0);
  return (evt_bits & PROVISIONED_BIT) != 0;
}

void em_wifi_stop_reconnecting(void)
{
  xEventGroupSetBits(wifi_evt_group, DISCONNECT_DEMAND_BIT);
}

uint32_t em_wifi_local_ip_addr(void)
{
  esp_netif_ip_info_t ip_info = {0};
  esp_err_t err = esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_info);

  if (err != ESP_OK) {
    return 0;
  }

  uint32_t localIpAddr = ip_info.ip.addr;
  return ((localIpAddr >> 24) & 0xff) | ((localIpAddr << 8) & 0xff0000) | ((localIpAddr >> 8) & 0xff00) |
         ((localIpAddr << 24) & 0xff000000);
}

esp_err_t em_wifi_sta_ssid(char ssid[32])
{
  wifi_config_t wifi_cfg;
  ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));
  int val = strnlen((const char *)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid));

  if (val == 0) {
    return ESP_ERR_NOT_FOUND;
  }

  memcpy(ssid, wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid));
  ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0'; // Ensure null-termination
  return ESP_OK;
}

esp_err_t em_wifi_tx_power(int8_t *power)
{
  return esp_wifi_get_max_tx_power(power);
}

// power unit is 0.25dBm, range is [8, 84] corresponding to 2dBm - 20dBm.
esp_err_t em_wifi_set_tx_power(int8_t power)
{
  return esp_wifi_set_max_tx_power(power);
}

esp_err_t em_wifi_rssi(int *rssi)
{
  return esp_wifi_sta_get_rssi(rssi);
}

esp_err_t em_wifi_set_channel(uint8_t channel)
{
  return esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

esp_err_t em_wifi_channel(uint8_t *chan)
{
  wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
  return esp_wifi_get_channel(chan, &second);
}
