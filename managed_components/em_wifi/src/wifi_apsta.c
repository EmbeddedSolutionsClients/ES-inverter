/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

/*#include "em/scheduler.h"
#include "em/wifi.h"
#include "em/wifi_priv.h"

#include <string.h>
#include <esp_mac.h>
#include <esp_wifi.h>*/

// TODO: implement
/* MACSTR macro output length, see esp_mac.h MACSTR for reference */
// #define MACSTR_LEN        17
// #define BASE_MAC_ADDR_LEN 6

// TODO: add AP_STA mode to restore http_provisioning
/*void apsta_init(void)
{
  char ssid_comb[MAX_SSID_LEN] = {0};
  const uint8_t base_ssid_len = sizeof(CONFIG_EM_WIFI_AP_BASE_SSID_NAME) - 1;
  memcpy(ssid_comb, CONFIG_EM_WIFI_AP_BASE_SSID_NAME, base_ssid_len);

  uint8_t mac[BASE_MAC_ADDR_LEN];
  esp_base_mac_addr_get(mac);
  int ret_len = snprintf(&ssid_comb[base_ssid_len], MAX_SSID_LEN - base_ssid_len, MACSTR, MAC2STR(mac));
  assert(ret_len == MACSTR_LEN);

  // Expected SSID: <CONFIG_EM_WIFI_AP_BASE_SSID_NAME><<AA:BB:CC:DD:EE:FF>

  wifi_config_t wifi_ap_config = {
    .ap =
      {
        .ssid_len = base_ssid_len + MACSTR_LEN,
        .channel = CONFIG_EM_WIFI_AP_DEFAULT_CHANNEL,
        .password = CONFIG_EM_WIFI_AP_PASSWORD,
        .max_connection = 1U,
        .authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg =
          {
            .required = false,
          },
      },
  };

  memcpy(wifi_ap_config.ap.ssid, ssid_comb, base_ssid_len + MACSTR_LEN);

#ifdef CONFIG_EM_WIFI_AP_INITIALIZE_NET_IF*/
/* When using WiFi in AP+STA mode, both these interfaces have to be created
 * see:
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif.html#wi-fi-default-initialization
 */
/* (void)esp_netif_create_default_wifi_ap();
 (void)esp_netif_create_default_wifi_sta();
#endif // CONFIG_EM_WIFI_AP_INITIALIZE_NET_IF

 ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
 ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
 ESP_ERROR_CHECK(esp_wifi_start());

 ESP_LOGI(LOG_TAG, "WiFi APSTA broadcasting started");
}*/
