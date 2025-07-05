/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef HTTP_OTA_H_
#define HTTP_OTA_H_

#include <esp_app_desc.h>
#include <esp_event.h>

#define APP_VERSION_LEN (32)

typedef enum {
  EM_HTTP_OTA_EVENT_BEGIN = 0,
  EM_HTTP_OTA_EVENT_VALIDATED,
  EM_HTTP_OTA_EVENT_TRANSFER,
  EM_HTTP_OTA_EVENT_DOWNLOADED,
  EM_HTTP_OTA_EVENT_VERIFIED,
  EM_HTPP_OTA_EVENT_FINISHED,
  EM_HTTP_OTA_EVENT_ABORT,
} em_http_ota_event_t;

ESP_EVENT_DECLARE_BASE(EM_HTTP_OTA_EVENT);

typedef enum {
  EM_HTTP_OTA_ERR_NONE = 0,
  EM_HTTP_OTA_ERR_TIMEOUT,
  EM_HTTP_OTA_ERR_REJECTED,
  EM_HTTP_OTA_ERR_ABORTED,
  EM_HTTP_OTA_ERR_IMG_INVALID,
  EM_HTTP_OTA_ERR_VERIFY,
  EM_HTTP_OTA_ERR_BRICKED,
  EM_HTTP_OTA_ERR_ROLLED_BACK,
  EM_HTTP_OTA_ERR_INIT = 0x40,
  EM_HTTP_OTA_ERR_INTERNAL,
  EM_HTTP_OTA_ERR_INCOMPLETE,
  EM_HTTP_OTA_ERR_CLEANUP,
  EM_HTTP_OTA_ERR_OTHER = 0xFE,
} em_http_ota_err_code_t;

typedef struct {
  em_http_ota_err_code_t err_code;
} em_http_ota_err_event_data_t;

typedef struct {
  char version[APP_VERSION_LEN];
} em_http_ota_validated_event_data_t;

typedef struct {
  uint8_t progress;
} em_http_ota_transfer_event_data_t;

/**
 * @brief Initializes the HTTP OTA Service.
 */
void http_ota_init(void);

/**
 * @brief Submits an OTA message to initiate the OTA process.
 *
 * @param url binary URL to download.
 * @return esp_err_t Result of the operation (ESP_OK on success).
 */
esp_err_t http_ota_submit_start(const char *url);

/**
 * @brief Submits an OTA message to abort the OTA process.
 *
 * @return esp_err_t Result of the operation (ESP_OK on success).
 */
esp_err_t http_ota_submit_abort(void);

#endif /* HTTP_OTA_H_ */
