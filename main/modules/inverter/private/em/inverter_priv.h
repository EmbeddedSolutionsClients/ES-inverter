/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef INVERTER_PRIV_H
#define INVERTER_PRIV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "em/dataset.h"
#include "em/serial_client.h"

extern em_sc_t sc;

int inv_id_handler(void *data, size_t data_len);
int inv_fw_ver_handler(void *data, size_t data_len);
int inv_model_handler(void *data, size_t data_len);
int inv_meas_qpigs_handler(void *data, size_t data_len);
int inv_warning_flags_handler(void *data, size_t data_len);
int inv_fault_handler(void *data, size_t data_len);
int inv_mode_handler(void *data, size_t data_len);

void inv_store_energy_meas();
void inv_set_meas_grid(uint16_t voltage, int16_t power, uint16_t freq);
void inv_set_meas_battery(uint16_t voltage, int16_t power);
void inv_set_meas_ac_out(uint16_t voltage, uint16_t power, uint16_t freq, uint8_t load, uint8_t power_factor);
void inv_set_meas_pv(uint8_t idx, uint16_t voltage, uint16_t power);
void inv_set_status(uint32_t status_flags);
void inv_set_warnings(uint64_t warning_flags);
void inv_set_fault(uint32_t fault_code);
void inv_set_mode(uint32_t mode);
void inv_set_model(uint32_t* model, size_t data_len);
void inv_set_fw_ver(uint32_t fv_ver);

#endif /* INVERTER_PRIV_H */
