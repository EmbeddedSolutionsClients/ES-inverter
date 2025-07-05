/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/http_ota.h"

#include <esp_app_desc.h>
#include <esp_encrypted_img.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#define LOG_TAG "em_http_ota"

#define OTA_Q_SUBMIT_TIMEOUT_MS (CONFIG_EM_HTTP_OTA_QUEUE_SUBMIT_TIMEOUT_MS / portTICK_PERIOD_MS)

static_assert((100U % CONFIG_EM_HTTP_OTA_PROGRESS_ACCURACY) == 0, "Invalid OTA progress accuracy");

typedef enum {
  EM_HTTP_OTA_START,
  EM_HTTP_OTA_ABORT,
} http_ota_action_t;

typedef struct {
  http_ota_action_t action;
  void *param;
} http_ota_msg_t;

typedef struct {
  bool img_validated;
  em_http_ota_err_code_t decrypt_err;
  size_t decrypted_size;
} decrypt_ctx_t;

ESP_EVENT_DEFINE_BASE(EM_HTTP_OTA_EVENT);

extern const char rsa_private_pem_start[] asm("_binary_encryption_key_pem_start");
extern const char rsa_private_pem_end[] asm("_binary_encryption_key_pem_end");

static QueueHandle_t ota_q;
static esp_https_ota_handle_t https_ota_handle;
static esp_decrypt_handle_t decrypt_handle;

static esp_err_t http_client_init_cb(esp_http_client_handle_t http_client)
{
  ESP_LOGI(LOG_TAG, "HTTP client ready");
  return ESP_OK;
}

static void abort_ota_handle(em_http_ota_err_code_t err_code)
{
  if (https_ota_handle) {
    ESP_ERROR_CHECK(esp_https_ota_abort(https_ota_handle));
    https_ota_handle = NULL;
  }

  if (decrypt_handle) {
    ESP_ERROR_CHECK(esp_encrypted_img_decrypt_abort(decrypt_handle));
    decrypt_handle = NULL;
  }

  em_http_ota_err_event_data_t evt_data = {.err_code = err_code};
  esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_ABORT, &evt_data, sizeof(evt_data), portMAX_DELAY);
}

static esp_err_t validate_image_header(const esp_app_desc_t *new_app_info)
{
  if (new_app_info == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  const esp_app_desc_t *installed_app_info = esp_app_get_description();
  assert(installed_app_info);

  if (memcmp(new_app_info->version, installed_app_info->version, sizeof(new_app_info->version)) == 0) {
    ESP_LOGW(LOG_TAG, "Installed fw_ver=%s is the same as a new - rejecting update", installed_app_info->version);
    return ESP_FAIL;
  }

  ESP_LOGI(LOG_TAG, "Accepted update %s -> %s", installed_app_info->version, new_app_info->version);

  char version[APP_VERSION_LEN] = {0};
  memcpy(version, new_app_info->version, sizeof(version));

  ESP_ERROR_CHECK(
    esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_VALIDATED, version, sizeof(version), portMAX_DELAY));

  return ESP_OK;
}

static esp_err_t ota_img_decrypt_handler(decrypt_cb_arg_t *cb_args, void *user_ctx)
{
  decrypt_ctx_t *decrypt_ctx = (decrypt_ctx_t *)user_ctx;

  if (!cb_args) {
    ESP_LOGE(LOG_TAG, "Can't decrypt, no argument given");
    decrypt_ctx->decrypt_err = EM_HTTP_OTA_ERR_INTERNAL;
    return ESP_ERR_INVALID_ARG;
  }

  pre_enc_decrypt_arg_t pargs = {0};
  pargs.data_in = cb_args->data_in;
  pargs.data_in_len = cb_args->data_in_len;

  esp_err_t ret = esp_encrypted_img_decrypt_data(decrypt_handle, &pargs);
  if (ret != ESP_OK && ret != ESP_ERR_NOT_FINISHED) {
    decrypt_ctx->decrypt_err = EM_HTTP_OTA_ERR_IMG_INVALID;
    ESP_LOGE(LOG_TAG, "Can't decrypt data: %d", ret);
    return ret;
  }

  if (!pargs.data_out_len) {
    cb_args->data_out_len = 0;
    return ESP_OK;
  }

  cb_args->data_out = pargs.data_out;
  cb_args->data_out_len = pargs.data_out_len;
  decrypt_ctx->decrypted_size += pargs.data_out_len;

  if (decrypt_ctx->img_validated) {
    return ESP_OK;
  }

  /* Check if we have enough data to read app desc */
  const uint32_t app_desc_offset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);

  if (cb_args->data_out_len < (app_desc_offset + sizeof(esp_app_desc_t))) {
    ESP_LOGW(LOG_TAG, "App desc not in the first chunk");
    return ESP_OK;
  }

  if (ESP_OK == validate_image_header((const esp_app_desc_t *)&cb_args->data_out[app_desc_offset])) {
    decrypt_ctx->img_validated = true;
    return ESP_OK;
  }

  /* header was not accepted */
  free(cb_args->data_out);
  cb_args->data_out_len = 0;
  decrypt_ctx->decrypt_err = EM_HTTP_OTA_ERR_REJECTED;

  return ESP_FAIL;
}

static TickType_t handle_ota_process(const char *binary_url, bool restart)
{
  /* handle_ota_process works in machine state manner so as not to block the task */

  /* Machine State controller */
  static em_http_ota_event_t ms = EM_HTTP_OTA_EVENT_BEGIN;

  static esp_http_client_config_t http_cli_config = {
    .keep_alive_enable = true,
    .timeout_ms = CONFIG_EM_HTTP_OTA_SERVER_TIMEOUT_MS,
    /* Pass NUL character to omit TLS Handshaking -> HTTP */
    /* TODO: validate srever's certificate, add client's authentication */
    .cert_pem = (const char *)"",
    .host = CONFIG_EM_HTTP_OTA_SERVER_HOSTNAME,
    .port = CONFIG_EM_HTTP_OTA_SERVER_PORT,
  };

  static esp_decrypt_cfg_t decrypt_cfg = {
    .rsa_priv_key = rsa_private_pem_start,
  };

  static decrypt_ctx_t decrypt_ctx = {
    .img_validated = false,
    .decrypt_err = ESP_OK,
    .decrypted_size = 0,
  };

  static esp_https_ota_config_t ota_config = {
    .http_config = &http_cli_config,
    .http_client_init_cb = http_client_init_cb,
    .partial_http_download = true,
    .max_http_request_size = CONFIG_EM_HTTP_OTA_REQ_SIZE,
    .decrypt_cb = ota_img_decrypt_handler,
    .decrypt_user_ctx = &decrypt_ctx,
  };

  static uint8_t progress_last = UINT8_MAX;

  if (restart) {
    /* Restart means that we received a request for starting the OTA, start in the next loop */
    if (https_ota_handle || decrypt_handle) {
      ESP_LOGW(LOG_TAG, "Aborting ongoing OTA");
      /* We were during OTA processing */
      abort_ota_handle(EM_HTTP_OTA_ERR_ABORTED);
    }

    ms = EM_HTTP_OTA_EVENT_BEGIN;
    return 0U;
  }

  em_http_ota_err_code_t ota_err = EM_HTTP_OTA_ERR_NONE;

  switch (ms) {
  case EM_HTTP_OTA_EVENT_BEGIN: {
    /* Clear decrypt context, prepare it for new OTA handling */
    memset(&decrypt_ctx, 0x00, sizeof(decrypt_ctx));

    /* Clear progress */
    progress_last = UINT8_MAX;

    /* ToDo: Could be static if rsa_priv_key_len can be retrieved in different way */
    decrypt_cfg.rsa_priv_key_len = rsa_private_pem_end - rsa_private_pem_start;
    ESP_LOGI(LOG_TAG, "RSA key len=%u", decrypt_cfg.rsa_priv_key_len);

    /* Ensure that handles are freed */
    assert(!decrypt_handle);
    assert(!https_ota_handle);

    decrypt_handle = esp_encrypted_img_decrypt_start(&decrypt_cfg);
    if (!decrypt_handle) {
      ESP_LOGE(LOG_TAG, "Can't start decryptor");
      ota_err = EM_HTTP_OTA_ERR_INIT;
      break;
    }

    http_cli_config.url = binary_url;
    ota_config.enc_img_header_size = esp_encrypted_img_get_header_size();

    esp_err_t ret = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (ret) {
      ESP_LOGE(LOG_TAG, "Can't begin HTTPS OTA: 0x%x URL=%s", ret, binary_url);
      ota_err = EM_HTTP_OTA_ERR_INTERNAL;
      break;
    }

    ESP_LOGI(LOG_TAG, "Download %s:%d%s", http_cli_config.host, http_cli_config.port, binary_url);

    esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_BEGIN, NULL, 0, portMAX_DELAY);
    ms = EM_HTTP_OTA_EVENT_TRANSFER;
    break;
  }

  case EM_HTTP_OTA_EVENT_TRANSFER: {
    esp_err_t ret = esp_https_ota_perform(https_ota_handle);
    if (ret == ESP_OK) {
      ms = EM_HTTP_OTA_EVENT_DOWNLOADED;
      break;
    } else if (ret != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      ESP_LOGE(LOG_TAG, "Perform OTA err: %d decrypt_err: %d", ret, decrypt_ctx.decrypt_err);
      if (decrypt_ctx.decrypt_err != ESP_OK) {
        ota_err = decrypt_ctx.decrypt_err;
      } else {
        ota_err = ret;
      }
      break;
    }

    int img_size = esp_https_ota_get_image_size(https_ota_handle);
    assert(img_size >= 0);

    em_http_ota_transfer_event_data_t evt_data = {
      .progress = (uint8_t)((decrypt_ctx.decrypted_size * 100U) / img_size),
    };

    if (evt_data.progress == progress_last) {
      break;
    }

    if (!(evt_data.progress % CONFIG_EM_HTTP_OTA_PROGRESS_ACCURACY)) {
      ESP_ERROR_CHECK(
        esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_TRANSFER, &evt_data, sizeof(evt_data), portMAX_DELAY));
    }

    progress_last = evt_data.progress;
    break;
  }

  case EM_HTTP_OTA_EVENT_DOWNLOADED: {
    if (!esp_https_ota_is_complete_data_received(https_ota_handle)) {
      ESP_LOGE(LOG_TAG, "Can't apply image, not all data received");
      ota_err = EM_HTTP_OTA_ERR_INCOMPLETE;
      break;
    }

    esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_DOWNLOADED, NULL, 0, portMAX_DELAY);
    ms = EM_HTPP_OTA_EVENT_FINISHED;
    break;
  }

  case EM_HTPP_OTA_EVENT_FINISHED: {
    esp_err_t ret = esp_encrypted_img_decrypt_end(decrypt_handle);
    if (ret != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Can't cleanup decrypt: %d", ret);
      ota_err = EM_HTTP_OTA_ERR_VERIFY;
      break;
    }

    esp_event_post(EM_HTTP_OTA_EVENT, EM_HTTP_OTA_EVENT_VERIFIED, NULL, 0, portMAX_DELAY);

    ret = esp_https_ota_finish(https_ota_handle);
    if (ret != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Can't finalize OTA: %d", ret);
      ota_err = EM_HTTP_OTA_ERR_CLEANUP;
      break;
    }

    esp_event_post(EM_HTTP_OTA_EVENT, EM_HTPP_OTA_EVENT_FINISHED, NULL, 0, portMAX_DELAY);
    ms = EM_HTTP_OTA_EVENT_BEGIN;
    break;
  }

  default:
    ota_err = EM_HTTP_OTA_ERR_OTHER;
    ESP_LOGE(LOG_TAG, "Invalid machine's state=%d", ms);
    ms = EM_HTTP_OTA_EVENT_BEGIN;
    break;
  }

  if (ota_err != ESP_OK) {
    /* Something went wrong, free the resources and go to initial state */
    ms = EM_HTTP_OTA_EVENT_BEGIN;
    abort_ota_handle(ota_err);
    return portMAX_DELAY;
  }

  if (ms == EM_HTTP_OTA_EVENT_BEGIN) {
    /* OTA processed successfully, EM_HTPP_OTA_EVENT_FINISHED was called last, go to initial state*/
    return portMAX_DELAY;
  }

  /* We are during OTA different states processing */
  return 0U;
}

static void http_ota_task(void *param)
{
  ESP_UNUSED(param);
  ESP_LOGI(LOG_TAG, "OTA task ready");

  http_ota_msg_t ota_msg = {0};
  TickType_t ticks_to_wait = portMAX_DELAY;

  while (true) {
    bool new_msg = false;
    if (xQueueReceive(ota_q, &ota_msg, ticks_to_wait)) {
      new_msg = true;
    }

    if (ota_msg.action == EM_HTTP_OTA_START) {
      ticks_to_wait = handle_ota_process(ota_msg.param, new_msg);
      /* yield to prevent the ota task from starving other tasks */
      taskYIELD();
      continue;
    }

    if (ota_msg.action == EM_HTTP_OTA_ABORT) {
      ESP_LOGI(LOG_TAG, "Aborting OTA...");
      abort_ota_handle(EM_HTTP_OTA_ERR_ABORTED);
      ticks_to_wait = portMAX_DELAY;
      continue;
    }

    ESP_LOGW(LOG_TAG, "Unknown message type: %u", ota_msg.action);
    ticks_to_wait = portMAX_DELAY;
  }
}

void http_ota_init(void)
{
  static StaticQueue_t ota_q_data = {0};
  static uint8_t ota_q_buf[CONFIG_EM_HTTP_OTA_MAX_QUEUE_MSGS * sizeof(http_ota_msg_t)] = {0};
  ota_q = xQueueCreateStatic(CONFIG_EM_HTTP_OTA_MAX_QUEUE_MSGS, sizeof(http_ota_msg_t), ota_q_buf, &ota_q_data);
  configASSERT(ota_q);

  static StaticTask_t task_data = {0};
  static StackType_t task_stack[CONFIG_EM_HTTP_OTA_TASK_STACK_SIZE] = {0};
  TaskHandle_t handle = xTaskCreateStatic(http_ota_task, "ota", CONFIG_EM_HTTP_OTA_TASK_STACK_SIZE, NULL,
                                          CONFIG_EM_HTTP_OTA_TASK_PRIO, task_stack, &task_data);
  configASSERT(handle);
  ESP_LOGI(LOG_TAG, "Initialized");
}

esp_err_t http_ota_submit_start(const char *url)
{
  /* Assert, invalid usage */
  assert(url);
  assert(url[0] == '/');

  http_ota_msg_t msg = {.action = EM_HTTP_OTA_START, .param = (void *)url};

  if (xQueueSend(ota_q, &msg, OTA_Q_SUBMIT_TIMEOUT_MS) != pdTRUE) {
    ESP_LOGE(LOG_TAG, "Can't submit OTA_START message");
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}

esp_err_t http_ota_submit_abort(void)
{
  http_ota_msg_t msg = {.action = EM_HTTP_OTA_ABORT, .param = NULL};

  if (xQueueSend(ota_q, &msg, OTA_Q_SUBMIT_TIMEOUT_MS) != pdTRUE) {
    ESP_LOGE(LOG_TAG, "Can't submit OTA_ABORT message");
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}
