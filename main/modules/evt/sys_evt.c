/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/ble_prov_svc.h"
#include "em/button.h"
#include "em/coredump.h"
#include "em/http_ota.h"
#include "em/protocol.h"
#include "em/scheduler.h"
#include "em/storage.h"
#include "em/tcp_client.h"
#include "em/update.h"
#include "em/utils.h"
#include "em/wifi.h"

#include <esp_random.h>

#include <esp_log.h>
#define LOG_TAG "sys_evt"

// this is duplciated, move to managed_components
static void delayed_esp_factory_reset(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  storage_set_factory_config();
  em_wifi_factory_reset();

  esp_restart();
}

static void button_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(event_data);
  ESP_UNUSED(event_base);

  switch (event_id) {
  case EM_BUTTON_3X_SHORT_1X_VERY_LONG_CLICK_EVENT: {
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_BUTTON_3X_SHORT_1X_VERY_LONG_CLICK_EVENT));
    scheduler_set_callback(delayed_esp_factory_reset, 0, NULL, CONFIG_EM_TCP_REBOOT_ON_MSG_DELAY_MS);
    protocol_send_status(MSGTYPE_FACTORY_RESET, 0);
  } break;

  case EM_BUTTON_2X_CLICK_EVENT: {
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_BUTTON_2X_CLICK_EVENT));
    //send_relay_snapshots_history();
  } break;

  case EM_BUTTON_3X_CLICK_EVENT: {
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_BUTTON_3X_CLICK_EVENT));
    if (update_in_progress()) {
      ESP_LOGW(LOG_TAG, "Update in progress cant start BLE");
      // do not start BLE when update in progress - not enough HEAP
      return;
    }
    ESP_ERROR_CHECK(em_ble_prov_start(60000));
  } break;

  default:
    break;
  }
}

static void http_ota_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(event_base);
  ESP_UNUSED(arg);

  switch (event_id) {
  case EM_HTTP_OTA_EVENT_BEGIN:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_HTTP_OTA_EVENT_BEGIN));
    break;

  case EM_HTTP_OTA_EVENT_VALIDATED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_HTTP_OTA_EVENT_VALIDATED));
    em_http_ota_validated_event_data_t *validated_data = (em_http_ota_validated_event_data_t *)event_data;
    assert(validated_data);

    update_started_handler(validated_data->version);
    break;

  case EM_HTTP_OTA_EVENT_TRANSFER: {
    em_http_ota_transfer_event_data_t *transfer_data = (em_http_ota_transfer_event_data_t *)event_data;
    update_new_progress_handler(transfer_data->progress);
  } break;

  case EM_HTTP_OTA_EVENT_DOWNLOADED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_HTTP_OTA_EVENT_DOWNLOADED));
    update_downloaded_handler();
    break;

  case EM_HTTP_OTA_EVENT_VERIFIED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_HTTP_OTA_EVENT_VERIFIED));
    update_ready_handler();
    break;

  case EM_HTPP_OTA_EVENT_FINISHED:
    ESP_LOGI(LOG_TAG, "%s", __STRINGIFY(EM_HTPP_OTA_EVENT_FINISHED));
    update_apply(0);
    break;

  case EM_HTTP_OTA_EVENT_ABORT:
    em_http_ota_err_event_data_t *ota_err_data = ((em_http_ota_err_event_data_t *)event_data);
    ESP_LOGE(LOG_TAG, "%s reason=%u", __STRINGIFY(EM_HTTP_OTA_EVENT_ABORT), ota_err_data->err_code);
    update_error_handler((uint8_t)ota_err_data->err_code);
    break;

  default:
    ESP_LOGE(LOG_TAG, "Unhandled EM_HTTP_OTA_EVENT=%lu", event_id);
    break;
  }
}

static void tcp_client_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(event_base);
  ESP_UNUSED(arg);

  switch (event_id) {
  case EM_TCP_CLIENT_EVENT_ONLINE: {
    protocol_register_tx_handler(em_tcp_client_send);

    storage_device_t device = storage_device();

    if (device.perm.unique_id == 0) {
      ESP_LOGI(LOG_TAG, "Register");
    }

    if (device.perm.unique_id == UINT32_MAX) {
      ESP_LOGE(LOG_TAG, "Can't read unique_id");
      device.perm.unique_id = 0;
    }

    protocol_send_client_info(device.perm.unique_id, em_utils_dev_id(), em_utils_hw_ver(), update_numeric_fw_ver(NULL), 0, (uint16_t)em_utils_dev_type(),
                              em_wifi_local_ip_addr(), esp_reset_reason(), esp_random());
  } break;

  case EM_TCP_CLIENT_EVENT_OFFLINE: {
    protocol_register_tx_handler(NULL);
    break;
  }

  /* ToDo: that will disappear soon */
  case EM_TCP_CLIENT_EVENT_TX_STATUS: {
    em_tcp_tx_status_event_data_t *tx_status = (em_tcp_tx_status_event_data_t *)event_data;

    coredump_on_data_sent_callback(tx_status->err_code == 0 ? true : false);
    ESP_LOGD(LOG_TAG, "Message sent confirmation");
  } break;

  default:
    break;
  }
}

void sys_evt_init(void)
{
  ESP_ERROR_CHECK(esp_event_handler_instance_register(EM_BUTTON_EVENT, ESP_EVENT_ANY_ID, button_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(EM_HTTP_OTA_EVENT, ESP_EVENT_ANY_ID, http_ota_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(EM_TCP_CLIENT_EVENT, ESP_EVENT_ANY_ID, tcp_client_event_handler, NULL, NULL));
}
