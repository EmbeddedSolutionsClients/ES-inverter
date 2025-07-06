/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "em/dataset.h"

const uint16_t min_ac_voltage_diff = 20; // 2V
const uint16_t min_dc_voltage_diff = 1; // 0.1V
const uint16_t min_ac_current_diff = 2; // 0.2A
const uint16_t min_dc_current_diff = 10; // 1A
const uint16_t min_power_diff = 50; // W
const uint64_t min_energy_diff = 250 * 3600; // 50Wh
const uint16_t min_freq_diff = 2; // 0.2Hz

typedef struct {
  char model[16];
  uint32_t fw_ver;
  uint32_t protocol_ver;
} inv_info_t;

/*
Power On Mode P Power on mode
Standby Mode S Standby mode
Bypass Mode Y Bypass mode
Line Mode L Line Mode
Battery Mode B Battery mode
Battery Test Mode T Battery test mode
Fault Mode F Fault mode
Shutdown Mode D Shutdown Mode
Grid mode Ｇ Grid mode
Charge mode C Charge mode
*/
typedef struct {
  uint32_t mode; // char 'P' - Power on, 'S' - Standby etc...
  uint64_t status_flags;
  uint64_t warning_flags;
  uint32_t fault_code;
  bool changed;
} inv_status_t;

typedef struct {
  time_t timestamp; // timestamp of the measurement
  uint64_t energy; // in Ws
} energy_t;

typedef struct {
  energy_t grid_consumed;
  energy_t grid_provided;
  energy_t ac_output; // taken from the inverter output
  energy_t pv; // produced by the PV panels
  energy_t battery_charge;
  energy_t battery_discharge;
} inv_energy_meas_t;

typedef struct {
  time_t timestamp;
  uint16_t voltage; // in 0.1V
 //int16_t current;  // in 0.01A
  dataset_t power_samples; // in W// TODO: init
  uint16_t power; // in W
  uint16_t freq; // in 0.1Hz
} inv_grid_meas_t;

typedef struct {
  time_t timestamp;
  uint16_t voltage; // in 0.1V
  //int16_t charge_current;  // in 0.01A
  dataset_t power_samples; // in W// TODO: init
  int32_t charge_power;  // in W, negative for discharge
} inv_battery_meas_t;

typedef struct {
  time_t timestamp;
  uint16_t voltage; // in 0.1V
  //uint16_t current; // in 0.01A
  dataset_t power_samples; // in W // TODO: init
  uint32_t power; // in W
  uint16_t freq; // in 0.1Hz
  uint8_t output_load; // in %
  uint8_t power_factor; // in 0.01
} inv_ac_output_meas_t;

//TODO: some inverters may have more PV strings, voltage[MAX_PV_INPUTS] ?  or table of inv_pv_meas_t ?
typedef struct {
  time_t timestamp;
  uint16_t voltage; // in 0.1V
  //uint16_t current[MAX_PV_INPUTS]; // in 0.1A
  dataset_t power_samples; // in W // TODO: init
  uint32_t power; // in W
  uint8_t strings_cnt;
} inv_pv_meas_t;

typedef struct {
  uint16_t bus_voltage; // in 0.1V
} inv_other_meas_t;

#endif /* DEFS_H */
