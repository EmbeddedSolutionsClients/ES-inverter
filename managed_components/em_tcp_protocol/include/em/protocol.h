/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>
#include <time.h>

#include "em/buffer.h"
#include "em/messages.h"

#define COORDINATES_FIXED_FACTOR        (-5)
#define PRICE_LIMIT_THRESHOLD_UNDEFINED (INT32_MIN)

typedef enum {
  MEASTYPE_VOLTAGE_RMS = 0,  // in 0.1 V
  MEASTYPE_CURRENT_RMS = 1,  // in 0.001 A
  MEASTYPE_POWER_ACTIVE = 2, // in W
  MEASTYPE_POWER_FACTOR = 3, // in 0.01
  MEASTYPE_FREQUENCY = 4,    // in 0.01 Hz

  MEASTYPE_ENERGY_CONSUMED = 0xED, // in kWh
  MEASTYPE_ENERGY_PRODUCED = 0xEF, // in kWh
} meastype_t;

typedef enum {
  PROTOCOL_NONE = 0,
  PROTOCOL_BLE = 1,
  PROTOCOL_WIFI = 2,
  PROTOCOL_LoRa = 3,
  PROTOCOL_Zigbee = 4,
  PROTOCOL_NB_IoT = 5,
  PROTOCOL_CUSTOM = 0xFE,
} protocol_t;

typedef enum {
  METER_ALARM_FLAG_OVERCURRENT = 0x01,
  METER_ALARM_FLAG_UNDER_VOLTAGE = 0x02,
  METER_ALARM_FLAG_OTHER = 0x8000,
} meter_alarm_t;

typedef int (*app_msg_dispatcher_cb_t)(msg_type_t type, void *data);
void protocol_register_rx_handler(app_msg_dispatcher_cb_t handler);

typedef int (*net_tx_medium_cb_t)(const buffer_t *buf);
void protocol_register_tx_handler(net_tx_medium_cb_t handler);

/** RX **/
int protocol_receiver_dispatcher(const uint8_t *data, uint32_t len);

/** TX **/
int protocol_send_client_info(uint32_t unique_id, uint64_t dev_id, uint8_t hw_ver, uint32_t app_ver,
                              uint8_t reconn_reason, uint16_t dev_type, uint32_t ip_addr, uint8_t reset_reason,
                              uint16_t session_id);
int protocol_send_installed_fw(uint32_t fw_ver, uint32_t downloaded_fw_ver, uint8_t hw_ver, time_t build_time);
int protocol_send_update_status(uint32_t fw_ver, uint8_t progress, uint8_t phase, uint8_t error);
int protocol_send_wireless_status(uint8_t protocol, uint8_t enabled, uint32_t ip, int8_t rssi, uint8_t lqi);
int protocol_send_wireless_credentials(uint8_t protocol, const char *network, const char *password);
int protocol_send_wireless_tx_power(uint8_t protocol, int16_t tx_power);
int protocol_send_wireless_channel(uint8_t protocol, uint8_t channel);

int protocol_send_energy_price_day_get(uint32_t req_region, time_t req_day_time);
int protocol_send_energy_threshold(int32_t price);
int protocol_send_energy_cheap_period(uint8_t cheap_period);
int protocol_send_energy_sunnyhours(uint8_t percentage);
int protocol_send_energy_active_price_limit(int32_t price);
int protocol_send_energy_tariff(uint16_t tariff);
int protocol_send_relay_mode(uint8_t mode);
int protocol_send_relay_state(uint8_t state);
int protocol_send_relay_def_state(uint8_t state);
int protocol_send_relay_pwm(uint8_t enabled, uint16_t period_min, uint8_t duty_cycle);
int protocol_send_relay_inverted_output(uint8_t inverted);
int protocol_send_relay_button_config(uint8_t *modes, uint8_t modes_num);
int protocol_send_relay_one_shot(uint8_t enabled, time_t start, time_t end, uint8_t state);
int protocol_send_relay_schedule(uint8_t *schedule, uint8_t len);
int protocol_send_relay_snapshots(time_t ref_timestamp, int8_t factor, uint32_t *t_offset, uint8_t *snapshot,
                                  int16_t *thresholds, uint16_t entries_num);

int protocol_send_environment_position(int32_t latitude, int32_t longitude);

int protocol_send_meter_measurements(time_t ref_timestamp, uint8_t *type, int32_t *meas, uint16_t entries_num);
int protocol_send_meter_measurements_history(time_t ref_timestamp, uint8_t meas_type, uint32_t *t_offset, int32_t *meas,
                                             uint16_t entries_num);
int protocol_send_meter_current_limit(int32_t limit);
int protocol_send_meter_alarms(uint64_t mask);

int protocol_send_energy_accumulated(time_t timestamp, uint8_t meas_type, uint64_t energy);
int protocol_send_energy_history(uint8_t meas_type, time_t ref_timestamp, uint32_t interval, uint32_t *entries,
                                 uint16_t entries_num);

int protocol_send_diag_coredump_start(uint32_t coredump_version);
int protocol_send_diag_coredump_body(uint32_t coredump_version, uint16_t block_len, uint8_t *data);
int protocol_send_diag_coredump_end(uint32_t coredump_version);
int protocol_send_diag_debug_info(const char *info);
int protocol_send_diag_logs_settings(uint8_t medium, uint64_t *moduls_levels);
int protocol_send_status(uint16_t rq_type, uint8_t error);

#endif /* PROTOCOL_H_ */
