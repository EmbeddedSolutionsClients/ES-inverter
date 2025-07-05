/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "hal/efuse_hal.h"
#include <stdint.h>
#include <esp_efuse.h>
#include <esp_system.h>

#include "em/math.h"
#include "em/utils.h"

#include <esp_log.h>
#define LOG_TAG "em_utils"

uint32_t em_utils_time_diff_ms(time_t to, time_t from)
{
  double diff_sec = difftime(to, from);
  assert((diff_sec < (UINT32_MAX / 1000)) && diff_sec >= 0);
  uint32_t diff_ms = (uint32_t)((int64_t)diff_sec * 1000);
  return diff_ms;
}

void em_utils_print_datetime(time_t t, const char *text)
{
  struct tm timeinfo;
  localtime_r(&t, &timeinfo);

  char strftime_buf[sizeof(time_t) * sizeof(time_t)] = {0};
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(LOG_TAG, "%s: %s", text, strftime_buf);
}

bool em_utils_is_leap_year(int32_t year)
{
  if (year % 400 == 0) {
    return true;
  }

  if (year % 100 == 0) {
    return true;
  }

  if (year % 4 == 0) {
    return true;
  }

  return false;
}

time_t em_utils_get_compile_time(void)
{
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char month_str[4];
  struct tm t = {0};

  sscanf(__DATE__, "%3s %d %d", month_str, &t.tm_mday, &t.tm_year);
  t.tm_mon = (strstr(months, month_str) - months) / 3;
  t.tm_year -= 1970;
  sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);

  return mktime(&t);
}
