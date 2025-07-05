/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/coredump.h"

#include <string.h>
#include <esp_core_dump.h>
#include <esp_flash.h>
#include <esp_random.h>
#include <sys/param.h>

#include <esp_log.h>
#define LOG_TAG "em_coredump"

static struct {
  coredump_api_t api;
  uint32_t ver;
  size_t addr;
  size_t size_left;
  bool ready;
} coredump;

static bool is_coredump_present(void)
{
  const esp_err_t ret = esp_core_dump_image_check();
  if (ret == ESP_ERR_NOT_FOUND) {
    ESP_LOGI(LOG_TAG, "No coredump found");
    return false;
  }

  if (ret == ESP_ERR_INVALID_SIZE || ret == ESP_ERR_INVALID_CRC) {
    /* We are doing all fine but it seems that esp_core_dump_image_check() returns false-positive error */
    ESP_ERROR_CHECK(esp_core_dump_image_erase());
    return false;
  }

  ESP_ERROR_CHECK(ret);
  return true;
}

int coredump_on_data_sent_callback(bool success)
{
  if (!success) {
    ESP_LOGE(LOG_TAG, "Can't send whole Coredump");
  }

  if (!coredump.ready) {
    return ESP_ERR_INVALID_STATE;
  }

  if (!success || coredump.size_left <= 0) {
    /* Clear handlers */
    memset(&coredump.api, 0x00, sizeof(coredump.api));
    coredump.ready = false;
    return ESP_ERR_INVALID_SIZE;
  }

  uint16_t block_len = MIN(coredump.size_left, CONFIG_EM_COREDUMP_BLOCK_MAX_SIZE);
  bool last_block = ((int32_t)coredump.size_left - block_len) <= 0 ? true : false;

  uint8_t block_data[block_len];
  ESP_ERROR_CHECK(esp_flash_read(esp_flash_default_chip, block_data, coredump.addr, block_len));

  if (coredump.api.body) {
    coredump.api.body(coredump.ver, block_len, block_data);
  }

  if (last_block && coredump.api.end) {
    coredump.api.end(coredump.ver);
  }

  coredump.addr += block_len;
  coredump.size_left -= block_len;
  return ESP_OK;
}

int coredump_start(coredump_api_t cfg)
{
  if (!is_coredump_present()) {
    return ESP_ERR_NOT_FOUND;
  }

  ESP_ERROR_CHECK(esp_core_dump_image_get(&coredump.addr, &coredump.size_left));
  coredump.ver = esp_random();
  coredump.api = cfg;
  coredump.ready = true;

  if (coredump.api.valid) {
    coredump.api.valid(coredump.ver);
  }

  return ESP_OK;
}

int coredump_erase(void)
{
  return (int)esp_core_dump_image_erase();
}
