/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/sntp.h"

#include <time.h>
#include <esp_event.h>
#include <esp_netif_sntp.h>
#include <sys/time.h>

#include <esp_log.h>
#define LOG_TAG "em_sntp"

static void (*datatime_synced_callback)(void);

static void datetime_sync_notify_cb(struct timeval *tv)
{
  ESP_LOGI(LOG_TAG, "%s", __func__);

  struct tm timeinfo;
  time(&tv->tv_sec);
  localtime_r(&tv->tv_sec, &timeinfo);

  char strftime_buf[sizeof(time_t) * sizeof(time_t)] = {0};
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(LOG_TAG, "Europe: Datetime: %s", strftime_buf);

  if (datatime_synced_callback) {
    datatime_synced_callback();
  }
}

static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(event_base);
  ESP_UNUSED(event_id);
  ESP_UNUSED(event_data);

  ESP_LOGI(LOG_TAG, "Starting SNTP");
  ESP_ERROR_CHECK(esp_netif_sntp_start());
}

void sntp_datetime_init(void (*on_sync_cb)(void))
{
  datatime_synced_callback = on_sync_cb;

  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, NULL, NULL));

  esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
    CONFIG_LWIP_SNTP_MAX_SERVERS, ESP_SNTP_SERVER_LIST(CONFIG_EM_SNTP_SERVER_PRIMARY, CONFIG_EM_SNTP_SERVER_BACKUP));
  sntp.sync_cb = datetime_sync_notify_cb;
  sntp.start = false;

  /* Standard format for Central European Time (CET) and Central European Summer Time (CEST) */
  setenv("TZ", CONFIG_EM_SNTP_LOCAL_TIMEZONE, 1);
  tzset();

  ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp));

  ESP_LOGI(LOG_TAG, "SNTP Initialized");

  if (sntp_is_time_in_sync() && datatime_synced_callback) {
    ESP_LOGI(LOG_TAG, "Datetime already synced (RTC)");
    datatime_synced_callback();
  }
}

bool sntp_is_time_in_sync(void)
{
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year < (2024 - 1900)) {
    return false;
  }

  return true;
}
