/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_DEVICE_H_
#define EM_DEVICE_H_

#include <math.h>
#include <stdint.h>
#include <time.h>

#define SECONDS_IN_MINUTE  (60U)
#define SECONDS_IN_QUARTER (SECONDS_IN_MINUTE * 15U)
#define SECONDS_IN_HOUR    (SECONDS_IN_MINUTE * 60U)
#define SECONDS_IN_DAY     (SECONDS_IN_HOUR * 24U)
#define SECONDS_IN_WEEK    (SECONDS_IN_DAY * 7)
#define MIN_TO_SEC(x)      (x * SECONDS_IN_MINUTE)
#define MIN_TO_MSEC(x)     (MIN_TO_SEC(x) * 1000U)
#define MSEC_TO_SEC(x)     (x / 1000)
#define HOURS_IN_DAY       (24)

#define DEG_TO_RAD(x) (x * M_PI / 180)
#define RAD_TO_DEG(x) (x * 180 / M_PI)

typedef enum {
  HW_VER_INVALID = 0x00,
  HW_VER_RESERVED = 0x01,
  HW_VER_ESP32C3_DKM1 = 0x02,
  HW_VER_ENERGYPLUG_V1 = 0x03,
  HW_VER_ENERGYPLUG_V2 = 0x04,
  HW_VER_ENERGYPLUG_V3 = 0x05
} hw_ver_t;

typedef enum {
  EM_DEV_TYPE_INVALID = 0x00,
  EM_DEV_TYPE_HUB = 0x01,
  EM_DEV_TYPE_ENERGYPLUG = 0x02,
  EM_DEV_TYPE_ADAPTER232 = 0x03,
  EM_DEV_TYPE_INVERTER = 0x04,
  EM_DEV_TYPE_BMS = 0x05,
} dev_type_t;

hw_ver_t em_utils_hw_ver(void);
dev_type_t em_utils_dev_type(void);
uint64_t em_utils_dev_id(void);
uint16_t em_utils_chip_rev(void);

#endif /* EM_DEVICE_H_ */
