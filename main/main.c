/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */
#include "em/ble_prov_svc.h"
#include "em/button.h"
#include "em/dispatcher.h"
#include "em/evt.h"
#include "em/http_ota.h"
#include "em/protocol.h"
#include "em/scheduler.h"
#include "em/slip.h"
#include "em/sntp.h"
#include "em/storage.h"
#include "em/tcp_client.h"
#include "em/update.h"
#include "em/utils.h"
#include "em/wifi.h"

#include <string.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#define LOG_TAG "main"

#define TCP_AUTH_EVT_BIT (1 << 0)

static_assert(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE);
static EventGroupHandle_t app_confirm_group;
static void datetime_sync_handler(void);

#if CONFIG_APP_DEBUG

static void app_debug_print_stats(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  char pcWriteBuffer[650] = {0}; // one task needs ~45B

  ESP_LOGI(LOG_TAG, "Free heap=%ld minimal=%ld kB", esp_get_free_heap_size() / 1000, esp_get_minimum_free_heap_size() / 1000);
  heap_caps_check_integrity_all(true);

  vTaskList(pcWriteBuffer);
  ESP_LOGI(LOG_TAG, "\nTask Name\tStatus\tPrio\tStack\tID\n%s", pcWriteBuffer);

  scheduler_set_callback(app_debug_print_stats, SCH_PARAM_NONE, SCH_CTX_NONE, CONFIG_APP_DEBUG_STATS_INTERVAL_MS);
}

static void alloc_failed_handler(size_t size, uint32_t caps, const char *function_name)
{
  ESP_LOGE(LOG_TAG, "Alloc fail size=%u B caps=0x%lX f=%s free=%ld (%ld) kB ", size, caps, function_name, esp_get_free_heap_size() / 1000,
           esp_get_minimum_free_heap_size() / 1000);
  heap_caps_print_heap_info(caps);
  heap_caps_check_integrity_all(true);
  assert(false);
}

static void app_debug_print_panic(void)
{
  app_debug_print_stats(0, NULL);
}

static void app_debug_init(void)
{
  const uint32_t first_debug_timeout_ms = 10U * 1000U;
  scheduler_set_callback(app_debug_print_stats, SCH_PARAM_NONE, SCH_CTX_NONE, first_debug_timeout_ms);
  heap_caps_register_failed_alloc_callback(alloc_failed_handler);
}

#endif /* CONFIG_APP_DEBUG */

static void app_confirm_wait(void)
{
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  ESP_ERROR_CHECK(esp_ota_get_state_partition(running, &ota_state));
  if (ota_state != ESP_OTA_IMG_PENDING_VERIFY) {
    ESP_LOGI(LOG_TAG, "No image to confirm, state=%d", ota_state);
    return;
  }

  // TODO: add function to tcp_protocol and move it there
  EventBits_t evt_bits = xEventGroupWaitBits(app_confirm_group, TCP_AUTH_EVT_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(CONFIG_APP_IMAGE_CONFIRM_TIMEOUT_MS));
  if (evt_bits & TCP_AUTH_EVT_BIT) {
    /* Passed app image confirmation test */
    const esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
    if (ret) {
      ESP_LOGE(LOG_TAG, "Can't mark app as valid: %d", ret);
      update_error_handler(UPDATE_ERROR_ROLLED_BACK);
      return;
    }

    ESP_LOGI(LOG_TAG, "App image confirmed");
    update_send_installed_fw();
    return;
  }

  const esp_err_t ret = esp_ota_mark_app_invalid_rollback_and_reboot();
  if (ret) {
    ESP_LOGW(LOG_TAG, "Can't rollback to previous app: %d", ret);
    return;
  }
}

/* esp_restart() was called, execute application specific handler */
static void app_shutdown_handlers(void)
{
  storage_force_dump_panic();
  em_tcp_client_disconnect_panic();
  em_wifi_stop_reconnecting();

#if CONFIG_APP_DEBUG
  app_debug_print_panic();
#endif
}

static void app_shutdown_register(void)
{
  /* It is mandatory to use that function only when all of the modules are initialized. For obvious reason and for
   * correct execution order. Last registered handlers are called first. Remember that esp_wifi_stop(),
   * esp_sync_timekeeping_timers() are also being called under the hood. Our handler has to be executed before them,
   * especially before esp_wifi_stop().
   */
  ESP_ERROR_CHECK(esp_register_shutdown_handler((shutdown_handler_t)app_shutdown_handlers));
}

static void app_confirm_init(void)
{
  static StaticEventGroup_t app_confirm_group_data;
  app_confirm_group = xEventGroupCreateStatic(&app_confirm_group_data);
  /* Check for insufficient FreeRTOS heap available */
  assert(app_confirm_group);
}

void app_main(void)
{
  /* app_init module prints out Application information: Project name, Version, Compile time */
  /* Indicate main entry */
  ESP_LOGI(LOG_TAG, "##### ENTERING MAIN #####");
  ESP_LOGI(LOG_TAG, "Reset reason=%d", esp_reset_reason());
  esp_log_level_set("wifi", ESP_LOG_WARN);

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGE(LOG_TAG, "Can't init NVS, recover partition: %d", ret);
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  app_confirm_init();
  scheduler_init();
  storage_init();
  evt_init();
  button_init(false);
  em_ble_prov_init(CONFIG_EM_DEVICE_NAME, em_utils_dev_id());
  em_wifi_sta_init(WIFI_PS_MIN_MODEM, true);
  sntp_datetime_init(datetime_sync_handler);
  em_tcp_client_init((tcp_client_api_t){
    .rx_net_proto_cb = slip_static_data_decoder,
    .tx_net_proto_cb = slip_static_data_encoder,
    .msg_disp_cb = protocol_receiver_dispatcher,
  });

  protocol_register_rx_handler(dispatcher_handler);

  http_ota_init();

#if CONFIG_APP_DEBUG
  app_debug_init();
#endif /* CONFIG_APP_DEBUG */
  ESP_LOGI(LOG_TAG, "dev_id=%llX chip_rev=%d.%d", em_utils_dev_id(), em_utils_chip_rev() >> 8, em_utils_chip_rev() && 0xFF);

  /* Has to be called after all init functions */
  app_shutdown_register();

  /* Has to be called last as it blocks main task until application image is confirmed */
  app_confirm_wait();
}

void app_confirm_handler(void *data)
{
  ESP_UNUSED(data);
  (void)xEventGroupSetBits(app_confirm_group, TCP_AUTH_EVT_BIT);
}

static void datetime_sync_handler(void)
{
  ESP_LOGI(LOG_TAG, "Datetime sync");
}