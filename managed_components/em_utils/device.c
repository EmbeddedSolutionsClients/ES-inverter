/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <stdint.h>
#include <string.h>
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include <esp_mac.h>
#include <esp_system.h>

#include "em/math.h"
#include "em/utils.h"

dev_type_t em_utils_dev_type(void)
{
  return CONFIG_EM_DEV_TYPE;
}

hw_ver_t em_utils_hw_ver(void)
{
  return CONFIG_EM_HW_VER;
}

/* Device ID is a unique uint64_t const componed of:
 * 1. 48 bits MAC address (lower bytes)
 * 2. 16 bit long sum of ESP UNIQUE ID eFuse register (higher bytes)
 */
uint64_t em_utils_dev_id(void)
{
  uint8_t mac[6] = {
    0,
  };
  ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));

  const esp_efuse_desc_t OPTIONAL_UNIQUE_ID[] = {
    {EFUSE_BLK2, 0, 128}, // [] Optional unique 128-bit ID,F
  };

  const uint8_t efuse_len_in_bits = 128;
  const uint8_t byte_len_of_128_bits = efuse_len_in_bits / 8;
  uint8_t buffer[byte_len_of_128_bits];
  memset(buffer, 0, sizeof(buffer));

  const esp_efuse_desc_t *EFUSES[] = {&OPTIONAL_UNIQUE_ID[0], NULL};
  ESP_ERROR_CHECK(esp_efuse_read_field_blob(EFUSES, &buffer, 128));

  uint16_t devIDSum = 0;

  for (int i = 0; i < 128 / 8; i++) {
    devIDSum += buffer[i];
  }

  return (uint64_t)devIDSum << 48 | (uint64_t)mac[0] << 40 | (uint64_t)mac[1] << 32 | (uint64_t)mac[2] << 24 |
         (uint64_t)mac[3] << 16 | (uint64_t)mac[4] << 8 | (uint64_t)mac[5];
}

uint16_t em_utils_chip_rev(void)
{
  esp_chip_info_t chip_info = {0};
  esp_chip_info(&chip_info);
  return chip_info.revision;
}
