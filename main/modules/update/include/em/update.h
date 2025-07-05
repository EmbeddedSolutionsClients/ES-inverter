/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_UPDATE_H_
#define EM_UPDATE_H_

#include <stdint.h>
#include <esp_app_desc.h>

typedef enum {
  UPDATE_ERROR_NONE = 0,
  UPDATE_ERROR_TIMEOUT = 1,
  UPDATE_ERROR_REJECTED = 2,
  UPDATE_ERROR_ABORTED = 3,
  UPDATE_ERROR_CORRUPTED = 4,
  UPDATE_ERROR_NOT_VERIFIED = 5,
  UPDATE_ERROR_BRICKED = 6,
  UPDATE_ERROR_ROLLED_BACK = 7,
  UPDATE_ERROR_INVALID_DEV_TYPE = 8,
  UPDATE_ERROR_INVALID_HW_VER = 9,
  UPDATE_ERROR_ALREADY_INSTALLED = 10,
  UPDATE_ERROR_INVALID_URL = 11,
  UPDATE_ERROR_SW_FAULT = 12,
  UPDATE_ERROR_OTHER = 0xFE,
} update_error_e;

/* getters*/

/**
 * @brief Get version buffer from DROM (esp_app_desc_t) and convert application version string
 * to numeric representation.
 *
 * @note
 * X.Y.Z
 * X - 1 byte PROD
 * Y - 1 byte FEAT
 * Z - 2 bytes FIX
 */
uint32_t update_numeric_fw_ver(const esp_app_desc_t *app_desc);
const char *update_fw_url(uint32_t *new_fw_ver);

/* cmds */
update_error_e update_start(esp_err_t (*start)(const char *url), uint16_t dev_type, uint32_t new_fw_ver, uint8_t hw_ver, const char *new_url,
                            uint16_t fw_url_len, uint8_t policy);
void update_apply(uint32_t delay_ms);
void update_send_installed_fw(void);

/* update evt handlers */
void update_started_handler(char *version);
void update_new_progress_handler(uint8_t progress);
void update_downloaded_handler(void);
void update_ready_handler(void);
void update_error_handler(uint8_t error_code);
void send_update_status(void);
bool update_in_progress(void);

#endif /* EM_UPDATE_H_ */
