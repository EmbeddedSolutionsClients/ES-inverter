/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <esp_err.h>

#define MAX_DEFINED_MODES     (6)
#define QUARTERS_IN_DAY       (24UL * 4UL)
#define MAX_METER_ENTRIES     (48)
#define MAX_ENERGY_ENTRIES    (4 * 24) // quarters for the whole day

typedef enum storage_section_e {
  STORAGE,
  /* Contains device's specific data */
  STORAGE_DEVICE,
  /* Contains device's datasets */
  STORAGE_DS,
  STORAGE_MASK = 1U << STORAGE_DEVICE | 1U << STORAGE_DS,
} storage_section_t;

typedef struct {
  /* checksum has to be the first member */
  uint32_t checksum;
} __attribute__((packed)) energy_ds_t;

typedef struct {
  int64_t latitude;
  int64_t longitude;
} __attribute__((packed)) coordinates_t;

typedef struct {
  /* checksum has to be the first member */
  uint32_t checksum;
  uint32_t unique_id;
  coordinates_t coord;
} __attribute__((packed)) flash_device_t;

typedef struct {
  /* checksum has to be the first member */
  uint32_t checksum;
} __attribute__((packed)) flash_ds_t;

typedef struct {
  flash_ds_t perm;
} storage_datasets_t;

typedef struct {
  uint16_t len;
  int32_t current_val;
  time_t current_val_time; // timestamp of the current value
  time_t ref_timestamp;    // timestamp of the first entry in the array
  int32_t entries[MAX_METER_ENTRIES];
  uint32_t t_offset[MAX_METER_ENTRIES];
} storage_meter_t;

typedef struct {
  uint16_t len;
  uint16_t meas_type; // meastype_t
  time_t ref_timestamp;
  uint64_t ref_energy;
  uint32_t interval; // in seconds
  uint32_t entries[MAX_ENERGY_ENTRIES];
} storage_energy_t;

typedef struct {
  flash_device_t perm;
} storage_device_t;

typedef struct database_s {
  storage_device_t device;
  storage_meter_t meter_voltage;
  storage_meter_t meter_current;
  storage_meter_t meter_power;
  storage_energy_t meter_energy;
  storage_datasets_t ds;
} database_t;

void storage_init(void);
void storage_force_dump_panic(void);
void storage_register_on_changed_callback(void (*callback)(void));
void storage_unregister_on_changed_callback(void (*callback)(void));

storage_device_t storage_device(void);
storage_datasets_t storage_datasets(void);

void storage_set_device_unique_id(uint32_t unique_id);
void storage_set_energy_price_list(uint16_t tariff, int8_t price_factor, time_t day, int16_t *prices, uint32_t prices_len);
void storage_increase_energy_accumulated(uint64_t delta_energy);
uint64_t storage_energy_accumulated();
void storage_set_coordinates(int64_t latitude, int64_t longitude);

int storage_set_meter_value(time_t datetime, uint16_t meas_type, int32_t value);
int32_t storage_meter_value(uint16_t meas_type, time_t *timestamp);
bool storage_add_meter_entry(time_t datetime, uint16_t meas_type, int32_t value);
bool storage_add_energy_entry(time_t datetime, uint64_t value);
void storage_set_factory_config(void);

#endif /* STORAGE_H_ */
