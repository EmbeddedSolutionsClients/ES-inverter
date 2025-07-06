/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */
#include "em/inverter_priv.h"
#include "em/defs.h"
#include "em/rs232_2400_protocol.h"
#include "em/serial_client.h"
#include <esp_log.h>
#include <string.h>

#define LOG_TAG "INV"
#define MAX_PV_INPUTS (1u)

#define METER_DATASET_LEN (11u) // 5 items in the dataset

//static inv_energy_history_t energy_history = {0};

// last know measurements
static inv_grid_meas_t grid_meas =  {.voltage = 0, .power = 0, .freq = 0, .timestamp = 0};
static inv_battery_meas_t battery_meas = {.voltage = 0, .charge_power = 0, .timestamp = 0};
static inv_ac_output_meas_t ac_output_meas = {.voltage = 0, .power_samples = {0,}, .power = 0, .freq = 0, .output_load = 0, .power_factor = 0, .timestamp = 0};
static inv_pv_meas_t pv_meas = {.voltage = 0, .power_samples = {0,}, .power = 0, .strings_cnt = 0, .timestamp = 0};
static inv_energy_meas_t energy_meas = {.grid_consumed = {0}, .grid_provided = {0}, .ac_output = {0}, .pv = {0}, .battery_charge = {0}, .battery_discharge = {0},};
static inv_status_t inv_status = {0,};
static inv_info_t inv_info = {0,};

static const serial_rsp_handler_t rsp_handlers[] = {
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QID,
     .msg_handler = inv_id_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QVFW,
     .msg_handler = inv_fw_ver_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QPIGS,
     .msg_handler = inv_meas_qpigs_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QPICF,
     .msg_handler = inv_fault_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QPIWS,
     .msg_handler = inv_warning_flags_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QMOD,
     .msg_handler = inv_mode_handler},
    {.protocol_idx = 0,
     .msg_type = EM_RS232_2400_QMOD,
     .msg_handler = inv_model_handler},
};

static serial_protocol_t protocols[] = {
    {.parse_func = rs232_2400_rsp_parse,
     .serialize_func = rs232_2400_serialize,
     .baud_rate = 2400,
     .data_bits = 8,
     .stop_bits = 1,
     .parity = false,
     .rsp_handlers = rsp_handlers,
     .handlers_cnt = sizeof(rsp_handlers) / sizeof(rsp_handlers[0])},
};

// bl0942_settings_t bl0942_settings = {.funx_0x18 = UINT8_MAX, .mode_0x19 =
// UINT16_MAX, .gain_cr_0x1A = UINT8_MAX};
em_sc_t sc = {.supported_protocols = protocols,
              .supported_protocols_cnt =
                  sizeof(protocols) / sizeof(protocols[0])};

int inv_init() {
  ESP_LOGI(LOG_TAG, "Init start");

  /*static uint32_t ac_out_power_buf[METER_DATASET_LEN] = {
      0,
  };
  static uint32_t pv_power_buf[METER_DATASET_LEN] = {
      0,
  };*/

  // Initialize datasets
  /*if (em_dataset_init(&ds_ac_out_power, ac_out_power_buf,
                      sizeof(ac_out_power_buf[0]), METER_DATASET_LEN) != 0) {
    ESP_LOGE(LOG_TAG, "Failed init voltage dataset");
    return -1;
  }

  if (em_dataset_init(&ds_pv_power, pv_power_buf, sizeof(pv_power_buf[0]),
                      METER_DATASET_LEN) != 0) {
    ESP_LOGE(LOG_TAG, "Failed init power dataset");
    return -1;
  }*/

  em_sc_init(&sc, 0);

  /* firmware version */
  uint8_t qvfw[] = {'Q', 'V', 'F', 'W'};
  assert(0 <= em_sc_send_periodic(&sc, EM_RS232_2400_QVFW, qvfw, sizeof(qvfw),
                                  10000));

  /* measurements */
  uint8_t qpigs[] = {'Q', 'P', 'I', 'G', 'S'};
  assert(0 <= em_sc_send_periodic(&sc, EM_RS232_2400_QPIGS, qpigs,
                                  sizeof(qpigs), 15000));

  /* warnings */
  uint8_t qpiws[] = {'Q', 'P', 'I', 'W', 'S'};
  assert(0 <= em_sc_send_periodic(&sc, EM_RS232_2400_QPIWS, qpiws,
                                  sizeof(qpiws), 600000));

  /* mode */
  uint8_t qmod[] = {'Q', 'M', 'O', 'D'};
  assert(0 <= em_sc_send_periodic(&sc, EM_RS232_2400_QMOD, qmod, sizeof(qmod),
                                  600000));

  ESP_LOGI(LOG_TAG, "Init done");
  return 0;
}

/*
void inv_store_meas() {
  if (energy_history.size > 0 && !energy_history.store_rq) {
    return;
  }

  time_t now = time(NULL);
  uint16_t battery_charging_power = battery_meas.charge_current *
battery_meas.voltage / 100; uint16_t battery_discharging_power =
battery_meas.discharge_current * battery_meas.voltage / 100; update_energy(now,
ac_output_meas.power, pv_meas.power, battery_charging_power,
battery_discharging_power);

  // store energy to flash every 15 min
  if(energy_meas.perm_storage_timestamp == 0 ||
    (now - energy_meas.perm_storage_timestamp > 15*60 - 5)) {
    energy_meas.perm_storage_timestamp = now;

    if(energy_history.index == 0){
      energy_history.ref_timestamp = now;
      energy_history.ref_ac_output_energy = energy_meas.ac_output_energy;
      energy_history.ref_pv_energy = energy_meas.energy;
      energy_history.ref_battery_charge = energy_meas.battery_charge;
      energy_history.ref_battery_discharge = energy_meas.battery_discharge;
      energy_history.ac_output_energy[energy_history.index] =
energy_meas.energy; energy_history.pv_energy[energy_history.index] =
energy_meas.energy; energy_history.battery_charge[energy_history.index] =
energy_meas.battery_charge;
      energy_history.battery_discharge[energy_history.index] =
energy_meas.battery_discharge; energy_history.time_offset[energy_history.index]
= 0; } else { energy_history.ac_output_energy[energy_history.index] =
energy_meas.energy - energy_history.ref_ac_output_energy;
      energy_history.pv_energy[energy_history.index] = energy_meas.energy -
energy_history.ref_pv_energy;
      energy_history.battery_charge[energy_history.index] =
energy_meas.battery_charge - energy_history.ref_battery_charge;
      energy_history.battery_discharge[energy_history.index] =
energy_meas.battery_discharge - energy_history.ref_battery_discharge;
      energy_history.time_offset[energy_history.index] = now -
energy_history.ref_timestamp;
    }

    energy_history.index++;

    ESP_LOGI(LOG_TAG, "Total AC out energy: %.2fkWh", energy_meas.energy /
3600.0/1000); ESP_LOGI(LOG_TAG, "Total PV energy: %.2fkWh", energy_meas.energy /
3600.0/1000); ESP_LOGI(LOG_TAG, "Total battery charge: %.2fkWh",
energy_meas.battery_charge / 3600.0/1000); ESP_LOGI(LOG_TAG, "Total battery
discharge: %.2fkWh", energy_meas.battery_discharge / 3600.0/1000);
    ESP_LOGI(LOG_TAG, "Energy history: [kWh]");

    for (int i = 0; i < energy_history.index; i++) {
      ESP_LOGI(LOG_TAG, "%d: %ldmin out:%.2f pv:%.2f chg:%.2f dis:%.2f", i,
        energy_history.time_offset[i]/60,
        energy_history.ac_output_energy[i]/3600.0/1000,
        energy_history.pv_energy[i]/3600.0/1000,
        energy_history.battery_charge[i]/3600.0/1000,
        energy_history.battery_discharge[i]/3600.0/1000);
    }

    if (energy_history.index >= 96) {
      //send to server
      energy_history.index = 0;
      ESP_LOGI(LOG_TAG, "Clear energy history");
    }
  }
}*/

void inv_store_energy_meas() {
  time_t now = time(NULL);

  if (energy_history.size == 0) {
    energy_history.ref_timestamp = now;
    energy_history.ref_grid = energy_meas.grid_consumed;
    energy_history.ref_ac_output = energy_meas.ac_output;
    energy_history.ref_pv = energy_meas.pv;
    energy_history.ref_battery_charge = energy_meas.battery_charge;
    energy_history.ref_battery_discharge = energy_meas.battery_discharge;
  } else {
    if (energy_meas.grid_consumed.energy < min_energy_diff &&
        energy_meas.ac_output.energy < min_energy_diff &&
        energy_meas.pv.energy < min_energy_diff &&
        energy_meas.battery_charge.energy < min_energy_diff &&
        energy_meas.battery_discharge.energy < min_energy_diff) {
      /*struct tm timeinfo;
      localtime_r(&now, &timeinfo);

      char strftime_buf[sizeof(time_t) * sizeof(time_t)] = {0};
      strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
      ESP_LOGI(LOG_TAG, "XX INV meas[%d] %s", energy_history.size,
      strftime_buf); ESP_LOGI(LOG_TAG, "Total grid energy consumed: %.2fkWh",
      energy_meas.grid_consumed / 3600.0/1000); ESP_LOGI(LOG_TAG, "Total AC out
      energy: %.2fkWh", energy_meas.ac_output / 3600.0/1000); ESP_LOGI(LOG_TAG,
      "Total PV energy: %.2fkWh", energy_meas.pv / 3600.0/1000);
      ESP_LOGI(LOG_TAG, "Total battery charge: %.2fkWh",
      energy_meas.battery_charge / 3600.0/1000); ESP_LOGI(LOG_TAG, "Total
      battery discharge: %.2fkWh\n", energy_meas.battery_discharge /
      3600.0/1000);*/
      return;
    }
  }

  // block storing entries more frequently then 5 min.

  ++energy_history.size;

  energy_history.grid[energy_history.size - 1] = energy_meas.grid_consumed;
  energy_history.ac_output[energy_history.size - 1] = energy_meas.ac_output;
  energy_history.pv[energy_history.size - 1] = energy_meas.pv;
  energy_history.battery_charge[energy_history.size - 1] =
      energy_meas.battery_charge;
  energy_history.battery_discharge[energy_history.size - 1] =
      energy_meas.battery_discharge;
  energy_history.time_offset[energy_history.size - 1] =
      now - energy_history.ref_timestamp;

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char strftime_buf[sizeof(time_t) * sizeof(time_t)] = {0};
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

  ESP_LOGI(LOG_TAG, "INV energy[%d] %s", energy_history.size - 1, strftime_buf);
  ESP_LOGI(LOG_TAG, "Total grid energy consumed: %.2fkWh",
           energy_meas.grid_consumed.energy / 3600.0 / 1000);
  ESP_LOGI(LOG_TAG, "Total AC out energy: %.2fkWh",
           energy_meas.ac_output.energy / 3600.0 / 1000);
  ESP_LOGI(LOG_TAG, "Total PV energy: %.2fkWh", energy_meas.pv.energy / 3600.0 / 1000);
  ESP_LOGI(LOG_TAG, "Total battery charge: %.2fkWh",
           energy_meas.battery_charge.energy / 3600.0 / 1000);
  ESP_LOGI(LOG_TAG, "Total battery discharge: %.2fkWh",
           energy_meas.battery_discharge.energy / 3600.0 / 1000);

  memset(&energy_meas, 0, sizeof(energy_meas));

  if (energy_history.size >= sizeof(energy_history.time_offset) /
                                 sizeof(energy_history.time_offset[0])) {
    ESP_LOGI(LOG_TAG, "Send and clear INV energy history");
    //protocol_send_inv_energy(&energy_history);
    energy_history.size = 0;
  }
}

// voltage in 0.1V, current in 0.01A, freq in 0.1Hz
void inv_set_meas_grid(uint16_t voltage, int16_t power, uint16_t freq) {
  time_t now = time(NULL);

  if (grid_meas.timestamp != 0) {
    // ESP_LOGI(LOG_TAG, "GRID: voltage:%.1f current %.2fA %lld sec",
    // voltage/10.0, current/100.0, now - grid_meas.timestamp);
    int64_t delta_energy =
        (grid_meas.power * (now - grid_meas.timestamp) + 500) / 1000;

    if (delta_energy >= 0) {
      energy_meas.grid_consumed.energy += delta_energy;
      // ESP_LOGI(LOG_TAG, "UPDATE Grid consumed: %.3f Wh (+%.3f Wh) %.2fA",
      // energy_meas.grid_consumed/3600.0, delta_energy/3600.0, current/100.0);
    } else {
      energy_meas.grid_provided.energy -= delta_energy;
      // ESP_LOGI(LOG_TAG, "UPDATE Grid produced: %.3f Wh (%.3f Wh)",
      // energy_meas.grid_produced/3600.0, delta_energy/3600.0);
    }
  }

  grid_meas.voltage = voltage;
  grid_meas.power = power;
  grid_meas.freq = freq;
  grid_meas.timestamp = now;
}

// voltage in 0.1V, power in W
void inv_set_meas_battery(uint16_t voltage, int16_t power) {
  time_t now = time(NULL);

  if (battery_meas.timestamp != 0) {
    int64_t delta_energy =
        battery_meas.charge_power * (now - battery_meas.timestamp);

    if (delta_energy > 0) {
      energy_meas.battery_charge.energy += delta_energy;
      // ESP_LOGI(LOG_TAG, "UPDATE bat charge: %.3f Wh (+%.3f Wh)",
      // energy_meas.battery_charge/3600.0, delta_energy/3600.0);
    } else {
      energy_meas.battery_discharge.energy -= delta_energy;
      // ESP_LOGI(LOG_TAG, "UPDATE bat discharge: %.3f Wh (%.3f Wh)",
      // energy_meas.battery_discharge/3600.0, delta_energy/3600.0);
    }
  }

  battery_meas.voltage = voltage;
  battery_meas.charge_power = power;
  battery_meas.timestamp = now;
}

// voltage in 0.1V, power in W, freq in 0.1Hz, load in %, power factor in 0.01
void inv_set_meas_ac_out(uint16_t voltage, uint16_t power, uint16_t freq,
                         uint8_t load, uint8_t power_factor) {
  time_t now = time(NULL);

  if (ac_output_meas.timestamp != 0) {
    int16_t prev_power =
        power_history_last_entry(&ac_out_power_history); // in W
    int64_t delta_energy = prev_power * (now - pv_meas.timestamp);
    energy_meas.ac_output.energy += delta_energy;
    // ESP_LOGI(LOG_TAG, "UPDATE ac output: %.3f Wh (+%.3f Wh)",
    // energy_meas.ac_output/3600.0, delta_energy/3600.0);
  }

  ac_output_meas.voltage = voltage;
  bool power_ready =
      em_utils_set_uint16_add(&ac_output_meas.power, power); // average power
  ac_output_meas.freq = freq;
  ac_output_meas.output_load = load;
  ac_output_meas.power_factor = power_factor;
  ac_output_meas.timestamp = now;

  if (power_ready) {
    uint16_t avg_power = em_utils_set_uint16_avg(&ac_output_meas.power);
    em_utils_set_uint16_clear(&ac_output_meas.power);

    int res = power_history_add_entry(&ac_out_power_history, now, avg_power);

    if (res != 0) {
      ESP_LOGI(LOG_TAG, "AC out new power[%d] %d time=%lld",
               ac_out_power_history.size - 1, avg_power,
               now - ac_out_power_history.ref_timestamp);

      if (res < 0) {
        ESP_LOGI(LOG_TAG, "Send and clear ac out power history");
        protocol_send_inv_ac_out_power_history(&ac_out_power_history);
        ac_out_power_history.size = 0;
      }
    }
  }
}

// TODO: idx not used
void inv_set_meas_pv(uint8_t idx, uint16_t voltage, uint16_t power) {
  if (idx >= MAX_PV_INPUTS) {
    return;
  }

  time_t now = time(NULL);

  if (pv_meas.timestamp != 0) {
    int16_t prev_power = power_history_last_entry(&pv_power_history); // in W
    int64_t delta_energy = prev_power * (now - pv_meas.timestamp);
    energy_meas.pv.energy += delta_energy;
    // ESP_LOGI(LOG_TAG, "UPDATE pv: %.3f Wh (+%.3f Wh)\n",
    // energy_meas.pv/3600.0, delta_energy/3600.0);
  }

  pv_meas.voltage = voltage;
  bool power_ready =
      em_utils_set_uint16_add(&pv_meas.power, power); // average power
  pv_meas.timestamp = now;

  if (power_ready) {
    uint16_t avg_power = em_utils_set_uint16_avg(&pv_meas.power);
    em_utils_set_uint16_clear(&pv_meas.power);

    int res = power_history_add_entry(&pv_power_history, now, avg_power);

    if (res != 0) {
      ESP_LOGI(LOG_TAG, "PV new power[%d] %d time=%lld",
               pv_power_history.size - 1, avg_power,
               now - pv_power_history.ref_timestamp);

      if (res < 0) {
        protocol_send_inv_pv_power_history(&pv_power_history);
        pv_power_history.size = 0;
      }
    }
  }
}
/*
11110110 - day, charging 30A from PV, almost 100% SoC
*/
void inv_set_status(uint32_t status_flags) {
  if (inv_status.status_flags != status_flags) {
    inv_status.status_flags = status_flags;
    ESP_LOGI(LOG_TAG, "Status flags: %lx", status_flags);
    inv_status.changed = true;
  }
}

void inv_set_warnings(uint64_t warning_flags) {
  if (inv_status.warning_flags != warning_flags) {
    inv_status.warning_flags = warning_flags;
    ESP_LOGW(LOG_TAG, "Warning flags: %llx", warning_flags);
    inv_status.changed = true;
  }
}

void inv_set_fault(uint32_t fault_code) {
  if (inv_status.fault_code != fault_code) {
    inv_status.fault_code = fault_code;
    ESP_LOGW(LOG_TAG, "Fault code: %c%c / %x", (char)(fault_code >> 24),
             (char)(fault_code >> 16), (uint16_t)fault_code);
    inv_status.changed = true;
  }
}

void inv_set_mode(uint32_t mode) {
  if (inv_status.mode != mode) {
    inv_status.mode = mode;
    ESP_LOGW(LOG_TAG, "Mode: %c / %lx", (char)mode, mode);
    inv_status.changed = true;
  }

  if (inv_status.changed) {
    // TOOD: send this whewhen assured if all data received
    inv_status.changed = false;
    protocol_send_inv_status(&inv_status);
  }
}

void inv_set_model(uint32_t *model, size_t data_len) {
  if (data_len > sizeof(inv_info.model) - 1) {
    data_len = sizeof(inv_info.model) - 1;
  }

  ESP_LOGW(LOG_TAG, "Model: %s", inv_info.model);
}

void inv_set_fw_ver(uint32_t fw_ver) {
  if (inv_info.fw_ver != fw_ver) {
    inv_info.fw_ver = fw_ver;
    ESP_LOGW(LOG_TAG, "FW version: %d.%d.%d.%d", (uint8_t)(fw_ver >> 24),
             (uint8_t)(fw_ver >> 16), (uint8_t)(fw_ver >> 8), (uint8_t)fw_ver);
  }
}