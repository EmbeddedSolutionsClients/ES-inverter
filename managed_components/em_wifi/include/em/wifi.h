/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <esp_event.h>
#include <esp_wifi.h>

typedef struct {
  char ssid[32];     /**< SSID of the WiFi network. */
  char password[64]; /**< Password for the WiFi network. */
  bool ssid_set;     /**< Flag indicating if SSID is set. */
  bool password_set; /**< Flag indicating if password is set. */
} em_wifi_credentials_t;

typedef struct {
  char ssid[32]; /**< SSID of the WiFi network. */
  int8_t rssi;
  uint8_t channel;
} __attribute__((packed)) em_wifi_ap_t;

typedef struct {
  size_t len;                                        /**< Number of access points. */
  em_wifi_ap_t aps[CONFIG_EM_WIFI_MAX_SCAN_RECORDS]; /**< List of access points. */
} __attribute__((packed)) em_wifi_ap_list_t;

typedef struct {
  uint8_t local_ip[4];
  uint8_t gw_ip[4];
} __attribute__((packed)) em_wifi_connected_evt_t;

/**
 * @brief WiFi events used by the system.
 *
 * These events are emitted by the WiFi component
 */
typedef enum {
  /* Event indicating that the device has been provisioned with WiFi credentials. */
  EM_WIFI_EVENT_PROVISIONED,
  /* Event indicating that the device has not been provisioned with WiFi credentials. */
  EM_WIFI_EVENT_NOT_PROVISIONED,
  EM_WIFI_EVENT_STA_DISCONNECTED, // disconnected from AP
  EM_WIFI_EVENT_STA_CONNECTED,    // connected and IP assigned
  EM_WIFI_EVENT_SCAN_DONE,
} em_wifi_provisioned_event_t;

/**
 * @brief Declaration of the base event loop for WiFi core events.
 *
 * This macro declares the event base used to post and handle WiFi-related events.
 */
ESP_EVENT_DECLARE_BASE(EM_WIFI_EVENT);

void em_wifi_sta_init(wifi_ps_type_t power_save_mode, bool start_scan);
void em_wifi_factory_reset(void);
void em_wifi_stop_reconnecting(void);
void em_wifi_scan_start(void);

bool em_wifi_sta_is_connected(TickType_t xTicksToWait);
bool em_wifi_sta_is_provisioned(void);
bool em_wifi_sta_is_disconnected_on_demand(void);
void em_wifi_sta_connect(const char *ssid, const char *password);
void em_wifi_sta_disconnect(void);
uint32_t em_wifi_local_ip_addr(void);
esp_err_t em_wifi_sta_ssid(char ssid[32]);
esp_err_t em_wifi_tx_power(int8_t *power);
esp_err_t em_wifi_set_tx_power(int8_t power);
esp_err_t em_wifi_rssi(int *rssi);
esp_err_t em_wifi_set_channel(uint8_t channel);
esp_err_t em_wifi_channel(uint8_t *chan);

#endif /* WIFI_H_ */
