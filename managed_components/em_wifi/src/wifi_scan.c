/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/wifi.h"
#include "em/wifi_priv.h"

#include <string.h>

void em_wifi_scan_start(void)
{
  if ((xEventGroupWaitBits(wifi_evt_group, SCAN_IN_PROGRESS_BIT, pdFALSE, pdTRUE, 0) & SCAN_IN_PROGRESS_BIT) != 0) {
    ESP_LOGI(LOG_TAG, "WiFi scan already in progress");
    return;
  }

  esp_err_t err = esp_wifi_scan_start(NULL, false);

  if (err != ESP_OK) {
    ESP_LOGE(LOG_TAG, "Start WiFi scan: %s", esp_err_to_name(err));
    return;
  }

  (void)xEventGroupSetBits(wifi_evt_group, SCAN_IN_PROGRESS_BIT);

  ESP_LOGI(LOG_TAG, "WiFi scanning...");
}

void scan_done_handler()
{
  uint16_t max_len = CONFIG_EM_WIFI_MAX_SCAN_RECORDS;
  wifi_ap_record_t scanned_aps[CONFIG_EM_WIFI_MAX_SCAN_RECORDS];

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_len, scanned_aps));
  assert(max_len <= CONFIG_EM_WIFI_MAX_SCAN_RECORDS);

  em_wifi_ap_list_t ap_list = {.len = (uint8_t)max_len};
  const size_t msg_len = sizeof(ap_list.len) + ap_list.len * sizeof(em_wifi_ap_t);

  for (uint16_t i = 0; i < ap_list.len; i++) {
    memcpy(ap_list.aps[i].ssid, scanned_aps[i].ssid, sizeof(scanned_aps[i].ssid));
    ap_list.aps[i].rssi = scanned_aps[i].rssi;
    ap_list.aps[i].channel = scanned_aps[i].primary;
  }

  ESP_LOGI(LOG_TAG, "scan list len=%d ", msg_len);
  ESP_ERROR_CHECK(esp_event_post(EM_WIFI_EVENT, EM_WIFI_EVENT_SCAN_DONE, &ap_list, msg_len, 0));

  (void)xEventGroupClearBits(wifi_evt_group, SCAN_IN_PROGRESS_BIT);
}
