/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/buffer.h"
#include "em/messages.h"
#include "em/protocol.h"
#include "em/scheduler.h"
#include "em/tcp_client.h"
#include "em/update.h"
#include "em/utils.h"

#include <string.h>
#include <esp_app_desc.h>
#include <esp_ota_ops.h>

#include <esp_log.h>
#define LOG_TAG "UPD"

#define NO_FIRMWARE   (0U)
#define PROGRESS_NONE (0U)

typedef enum {
  UPDATE_PHASE_IDLE = 0,
  UPDATE_PHASE_STARTED = 1,
  UPDATE_PHASE_TRANSFER = 2,
  UPDATE_PHASE_DOWNLOADED = 3,
  UPDATE_PHASE_READY = 4,
  UPDATE_PHASE_APPLY = 5
} update_phase_e;

typedef enum {
  UPDATE_IMMEDIATE,
  UPDATE_DELAYED,
} update_policy_t;

typedef struct {
  uint32_t new_fw_ver;
  uint8_t phase;
  uint8_t progress;
  uint8_t error;
  char new_fw_image_url[IMAGE_URL_MAX_LEN];
  update_policy_t policy;
} update_state_t;

// TODO: add mutex for this struct ?
static update_state_t update_state = {
  .new_fw_ver = NO_FIRMWARE,
  .phase = UPDATE_PHASE_IDLE,
  .progress = PROGRESS_NONE,
  .error = UPDATE_ERROR_NONE,
  .new_fw_image_url = {0},
};

void update_send_installed_fw(void)
{
  const esp_app_desc_t *app_desc = esp_app_get_description();

  ESP_LOGI(LOG_TAG, "Send installed fw=%s build=%s", app_desc->version, app_desc->time);

  uint32_t downloaded_fw = 0xFFFFFFFF;

  if (update_state.phase == UPDATE_PHASE_DOWNLOADED) {
    downloaded_fw = update_state.new_fw_ver;
  }

  protocol_send_installed_fw(update_numeric_fw_ver(app_desc), downloaded_fw, em_utils_hw_ver(), em_utils_get_compile_time());
}

static void update_apply_delayed(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  /* Shutdown handler closes TCP connection gracefully on esp_restart() */
  esp_restart();
}

void send_update_status(void)
{
  uint32_t new_fw_ver = update_state.new_fw_ver;

  if (update_state.phase == UPDATE_PHASE_IDLE) {
    new_fw_ver = update_numeric_fw_ver(NULL);
  }

  protocol_send_update_status(new_fw_ver, update_state.progress, update_state.phase, update_state.error);
}

static bool set_new_fw_url(const char *new_url, uint16_t fw_url_len)
{
  if (fw_url_len == 0 || new_url == NULL) {
    ESP_LOGE(LOG_TAG, "Invalid FW URL");
    return false;
  }

  if (new_url[0] != '/') {
    ESP_LOGE(LOG_TAG, "FW URL does not start with /");
    return false;
  }

  if (fw_url_len > IMAGE_URL_MAX_LEN) {
    ESP_LOGE(LOG_TAG, "FW URL too long");
    return false;
  }

  memset(update_state.new_fw_image_url, 0, sizeof(update_state.new_fw_image_url));
  memcpy(update_state.new_fw_image_url, new_url, fw_url_len);
  return true;
}

static uint32_t get_fw_version_string(char *version)
{
  assert(version);

  char *end_ptr = NULL;
  uint8_t prod = strtoul(version, &end_ptr, 10);
  assert(end_ptr);
  uint8_t feat = strtoul(++end_ptr, &end_ptr, 10);
  assert(end_ptr);
  uint8_t fix = strtoul(++end_ptr, &end_ptr, 10);
  assert(end_ptr);

  return ((uint32_t)prod << 16 | (uint32_t)feat << 8 | (uint32_t)fix);
}

void update_apply(uint32_t delay_ms)
{
  update_state.phase = UPDATE_PHASE_APPLY;

  if (update_state.policy == UPDATE_IMMEDIATE) {
    if (delay_ms) {
      scheduler_set_callback(update_apply_delayed, SCH_PARAM_NONE, SCH_CTX_NONE, delay_ms);
      return;
    }

    esp_restart();
  }
}

void update_error_handler(uint8_t error_code)
{
  update_state.error = error_code;
  send_update_status();
}

void update_new_progress_handler(uint8_t new_progress)
{
  if (new_progress != update_state.progress) {
    update_state.progress = new_progress;
    update_state.phase = UPDATE_PHASE_TRANSFER;
    update_state.progress = new_progress;
    ESP_LOGI(LOG_TAG, "Progress=%d%%", new_progress);
    send_update_status();
  }
}

void update_started_handler(char *version)
{
  update_state.phase = UPDATE_PHASE_STARTED;
  update_state.new_fw_ver = get_fw_version_string(version);
  update_state.progress = PROGRESS_NONE;
  update_state.error = UPDATE_ERROR_NONE;
  send_update_status();
}

void update_downloaded_handler(void)
{
  update_state.phase = UPDATE_PHASE_DOWNLOADED;
  update_state.progress = 100;
  ESP_LOGI(LOG_TAG, "Image downloaded");
  send_update_status();
}

void update_ready_handler(void)
{
  update_state.phase = UPDATE_PHASE_READY;
  send_update_status();
}

// TODO: add message to send FW URL to the server
const char *update_fw_url(uint32_t *new_fw_ver)
{
  if (new_fw_ver) {
    *new_fw_ver = update_state.new_fw_ver;
  }

  if (strlen(update_state.new_fw_image_url) == 0) {
    return NULL;
  }

  return update_state.new_fw_image_url;
}

update_error_e update_start(esp_err_t (*start)(const char *url), uint16_t dev_type, uint32_t new_fw_ver, uint8_t hw_ver, const char *new_url,
                            uint16_t fw_url_len, uint8_t policy)
{
  if (dev_type != em_utils_dev_type()) {
    ESP_LOGE(LOG_TAG, "FW for different dev_type=%#x", dev_type);
    update_state.error = UPDATE_ERROR_INVALID_DEV_TYPE;
    return UPDATE_ERROR_INVALID_DEV_TYPE;
  }

  if (hw_ver != em_utils_hw_ver()) {
    ESP_LOGE(LOG_TAG, "FW for different hw_ver=%#x", hw_ver);
    update_state.error = UPDATE_ERROR_INVALID_HW_VER;
    return UPDATE_ERROR_INVALID_HW_VER;
  }

  uint32_t installed_fw = update_numeric_fw_ver(NULL);

  if (new_fw_ver == installed_fw) {
    ESP_LOGE(LOG_TAG, "Already installed fw_ver=%#lx", new_fw_ver);
    update_state.error = UPDATE_ERROR_ALREADY_INSTALLED;
    return UPDATE_ERROR_ALREADY_INSTALLED;
  }

  if (!set_new_fw_url(new_url, fw_url_len)) {
    update_state.error = UPDATE_ERROR_INVALID_URL;
    return UPDATE_ERROR_INVALID_URL;
  }

  update_state.new_fw_ver = new_fw_ver;
  update_state.policy = (update_policy_t)policy;

  esp_err_t err = start(update_state.new_fw_image_url);

  if (err != ESP_OK) {
    update_state.error = UPDATE_ERROR_SW_FAULT;
    return UPDATE_ERROR_SW_FAULT;
  }

  ESP_LOGI(LOG_TAG, "Start update fw=%#lx->%#lx from %s", installed_fw, new_fw_ver, new_url);
  update_state.error = UPDATE_ERROR_NONE;
  return UPDATE_ERROR_NONE;
}

uint32_t update_numeric_fw_ver(const esp_app_desc_t *app_desc)
{
  if (app_desc == NULL) {
    app_desc = esp_app_get_description();
  }

  assert(app_desc);

  return get_fw_version_string((char *)(app_desc->version));
}

bool update_in_progress(void)
{
  return (update_state.phase != UPDATE_PHASE_IDLE);
}
