/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */


#include "em/rs232_2400_protocol.h"
#include "em/serial_client.h"
#include "em/storage.h"
#include "em/inverter_priv.h"
#include <stddef.h>
#include <stdint.h>

#include <esp_log.h>

#define LOG_TAG "RS232"

 int inv_meas_qpigs_handler(void *data, size_t data_len)
 {
   assert(data);
   qpigs_response_t *rsp = (qpigs_response_t *)data;

   uint8_t power_factor = (100.0 * (float)rsp->ac_output_active_power/(float)rsp->ac_output_appearent_power);

   /*ESP_LOGI(LOG_TAG, "---- ");
   ESP_LOGI(LOG_TAG, "Grid : %.1fV %.1fHz", rsp->grid_voltage, rsp->grid_frequency);
   ESP_LOGI(LOG_TAG, "ACout: %.1fV %.1fHz %dW %d%% factor: %.2f",rsp->ac_output_voltage,  rsp->ac_output_frequency,
     rsp->ac_output_active_power, rsp->output_load_percent, (float)power_factor/100.0);
   ESP_LOGI(LOG_TAG, "PV_in: %.1fV %.2fA %dW", rsp->pv_input_voltage, rsp->pv_input_current, rsp->pv_input_power);
   ESP_LOGI(LOG_TAG, "Temp : %d°C BUS voltage: %dV", rsp->temperature, rsp->bus_voltage);
   ESP_LOGI(LOG_TAG, "Device status 1: %#lx 2: %#x", rsp->device_status_1, rsp->device_status_2);*/
   int16_t battery_charge_power = 0;

   if (rsp->battery_discharging_current > 0){
     battery_charge_power = (int16_t)(rsp->battery_discharging_current * rsp->battery_voltage);
     //ESP_LOGI(LOG_TAG, "Battery: %.1fV -%dA -%ldW", rsp->battery_voltage, rsp->battery_discharging_current, battery_charge_power);
     inv_set_meas_battery(10 * rsp->battery_voltage, -battery_charge_power);
   } else {
     battery_charge_power = (int16_t)(rsp->battery_charging_current * rsp->battery_voltage);
     //ESP_LOGI(LOG_TAG, "Battery: %.1fV %dA %ldW", rsp->battery_voltage, rsp->battery_charging_current, battery_charge_power);
     inv_set_meas_battery(10 * rsp->battery_voltage, battery_charge_power);
   }
    //ESP_LOGI(LOG_TAG, "ac: %d bat: %ld pv: %d grid_power=%ldW, cur=%.2f", rsp->ac_output_active_power, battery_charge_power, rsp->pv_input_power, grid_power, grid_current/100.0);
   inv_set_meas_grid(10 * rsp->grid_voltage, 0, rsp->grid_frequency);
   inv_set_meas_ac_out(10 * rsp->ac_output_voltage, rsp->ac_output_active_power, rsp->ac_output_frequency, rsp->output_load_percent, power_factor);
   inv_set_meas_pv(0, 10 * rsp->pv_input_voltage, rsp->pv_input_power);
   inv_set_status(rsp->device_status_1 + (rsp->device_status_2 << 16));
   inv_store_energy_meas();
   return 0;
 }

 int inv_warning_flags_handler(void *data, size_t data_len)
 {
  assert(data);
  inv_set_warnings(*(uint64_t *)data);
  return 0;
 }

 int inv_fault_handler(void *data, size_t data_len)
 {
  assert(data);
  inv_set_fault(*(uint64_t *)data);
  return 0;
 }

 int inv_mode_handler(void *data, size_t data_len)
 {
  assert(data);
  inv_set_mode(*(uint32_t *)data);
  return 0;
 }

 int inv_model_handler(void *data, size_t data_len)
 {
  assert(data);
  inv_set_model((uint32_t*)data, data_len);
  return 0;
 }

 int inv_id_handler(void *data, size_t data_len)
 {
  assert(data);
  uint64_t *rsp = (uint64_t *)data;
  ESP_LOGI(LOG_TAG, "Inverter ID %#llx", *rsp);
  return 0;
 }

 int inv_fw_ver_handler(void *data, size_t data_len)
 {
  assert(data);
  inv_set_fw_ver(*(uint32_t *)data);
  em_sc_remove_periodic(&sc, EM_RS232_2400_QVFW);
  return 0;
 }