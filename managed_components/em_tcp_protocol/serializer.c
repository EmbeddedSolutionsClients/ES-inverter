/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/serializer.h"
#include <string.h>
#include <common.h>

__attribute__((unused)) static void serialize_int8(int8_t value, uint8_t **ptr)
{
  **ptr = value;
  *ptr += sizeof(int8_t);
}

static void serialize_uint8(uint8_t value, uint8_t **ptr)
{
  **ptr = value;
  *ptr += sizeof(uint8_t);
}

static void serialize_uint16(uint16_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  *ptr += sizeof(uint16_t);
}

__attribute__((unused)) static void serialize_int16(int16_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  *ptr += sizeof(int16_t);
}

static void serialize_uint32(uint32_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  (*ptr)[2] = (uint8_t)((value >> 16) & 0xFF);
  (*ptr)[3] = (uint8_t)((value >> 24) & 0xFF);
  *ptr += sizeof(uint32_t);
}

static void serialize_int32(int32_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  (*ptr)[2] = (uint8_t)((value >> 16) & 0xFF);
  (*ptr)[3] = (uint8_t)((value >> 24) & 0xFF);
  *ptr += sizeof(int32_t);
}

static void serialize_uint64(uint64_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  (*ptr)[2] = (uint8_t)((value >> 16) & 0xFF);
  (*ptr)[3] = (uint8_t)((value >> 24) & 0xFF);
  (*ptr)[4] = (uint8_t)((value >> 32) & 0xFF);
  (*ptr)[5] = (uint8_t)((value >> 40) & 0xFF);
  (*ptr)[6] = (uint8_t)((value >> 48) & 0xFF);
  (*ptr)[7] = (uint8_t)((value >> 56) & 0xFF);
  *ptr += sizeof(uint64_t);
}

static void serialize_int64(int64_t value, uint8_t **ptr)
{
  (*ptr)[0] = (uint8_t)(value & 0xFF);
  (*ptr)[1] = (uint8_t)((value >> 8) & 0xFF);
  (*ptr)[2] = (uint8_t)((value >> 16) & 0xFF);
  (*ptr)[3] = (uint8_t)((value >> 24) & 0xFF);
  (*ptr)[4] = (uint8_t)((value >> 32) & 0xFF);
  (*ptr)[5] = (uint8_t)((value >> 40) & 0xFF);
  (*ptr)[6] = (uint8_t)((value >> 48) & 0xFF);
  (*ptr)[7] = (uint8_t)((value >> 56) & 0xFF);
  *ptr += sizeof(int64_t);
}

__attribute__((unused)) static void serialize_string(const char *str, uint8_t **ptr, size_t length)
{
  memcpy(*ptr, str, length);
  *ptr += length;
}

// Serialization and parsing functions for each structure
ptrdiff_t serialize_client_info_msg(const client_info_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->dev_type, &ptr);
  serialize_uint64(msg->dev_id, &ptr);
  serialize_uint32(msg->unique_id, &ptr);
  serialize_uint32(msg->ip_addr, &ptr);
  serialize_uint8(msg->hw_ver, &ptr);
  serialize_uint32(msg->app_ver, &ptr);
  serialize_uint8(msg->reconn_reason, &ptr);
  serialize_uint8(msg->reset_reason, &ptr);
  serialize_uint8(msg->reserved, &ptr);
  serialize_uint16(msg->session_id, &ptr);
  serialize_uint32(msg->CRC, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_installed_fw_msg(const installed_fw_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint32(msg->inst_fw_ver, &ptr);
  serialize_uint32(msg->downloaded_fw_ver, &ptr);
  serialize_uint32(msg->hw_ver, &ptr);
  serialize_uint64(msg->build_time, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_update_status_msg(const update_status_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint32(msg->fw_ver, &ptr);
  serialize_uint8(msg->progress, &ptr);
  serialize_uint8(msg->phase, &ptr);
  serialize_uint8(msg->error, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_price_day_get_msg(const energy_price_day_get_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->tariff, &ptr);
  serialize_uint64(msg->day_time, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_active_price_limit_msg(const energy_price_active_price_limit_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_int32(msg->price, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_price_cheap_period_msg(const energy_price_cheap_period_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->quarters, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_price_active_price_limit_msg(const energy_price_active_price_limit_msg_t *msg,
                                                        uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_int32(msg->price, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_price_tariff_msg(const energy_price_tariff_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->tariff, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_wireless_status_msg(uint8_t protocol, uint8_t enabled, uint32_t ip, int8_t rssi, uint8_t lqi,
                                        uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_WIRELESS_STATUS, &ptr);
  serialize_uint8(protocol, &ptr);
  serialize_uint8(enabled, &ptr);
  serialize_uint32(ip, &ptr);
  serialize_int8(rssi, &ptr); // rssi is signed in the original message
  serialize_uint8(lqi, &ptr); // lqi is unsigned in the original message
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_wireless_credentials_msg(const wireless_credentials_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_WIRELESS_CREDENTIALS, &ptr);
  serialize_uint8(msg->protocol, &ptr);

  assert(msg->network_len < sizeof(msg->network));
  serialize_uint16(msg->network_len, &ptr);
  serialize_string((const char *)msg->network, &ptr, msg->network_len);

  assert(msg->password_len < sizeof(msg->password));
  serialize_uint16(msg->password_len, &ptr);
  serialize_string((const char *)msg->password, &ptr, msg->password_len);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_wireless_tx_power_msg(uint8_t protocol, int16_t tx_power, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_WIRELESS_TX_POWER, &ptr);
  serialize_uint8(protocol, &ptr);
  serialize_int16(tx_power, &ptr); // tx_power is signed in the original message
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_wireless_channel_msg(uint8_t protocol, uint8_t channel, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_WIRELESS_CHANNEL, &ptr);
  serialize_uint8(protocol, &ptr);
  serialize_uint8(channel, &ptr); // channel is unsigned in the original message
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_mode_msg(const relay_mode_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->mode, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_default_state_msg(const relay_default_state_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->state, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_state_msg(const relay_state_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->state, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_pwm_msg(const relay_pwm_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->enabled, &ptr);
  serialize_uint16(msg->period, &ptr);
  serialize_uint8(msg->duty_cycle, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_output_inverted_msg(uint8_t inverted, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_RELAY_INVERTED_OUTPUT, &ptr);
  serialize_uint8(inverted, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_one_shot_msg(const relay_one_shot_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->enabled, &ptr);
  serialize_uint64(msg->start, &ptr);
  serialize_uint64(msg->end, &ptr);
  serialize_uint8(msg->state, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_defined_modes_msg(const relay_button_config_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->mode_len, &ptr);

  for (int i = 0; i < msg->mode_len; i++) {
    serialize_uint8(msg->modes[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_schedule_msg(const relay_schedule_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->schedule_len, &ptr);

  assert(msg->schedule_len == ARRAY_LENGTH(msg->schedule));

  for (int i = 0; i < ARRAY_LENGTH(msg->schedule); i++) {
    serialize_uint8(msg->schedule[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_relay_snapshots_msg(time_t ref_timestamp, int8_t price_factor, uint32_t *t_offset,
                                        uint8_t *snapshots, int16_t *thresholds, uint16_t entries_num, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_RELAY_SNAPSHOTS, &ptr);
  serialize_uint64(ref_timestamp, &ptr);
  serialize_int8(price_factor, &ptr);
  serialize_uint16(entries_num, &ptr);
  for (uint32_t i = 0; i < entries_num; i++) {
    serialize_uint32(t_offset[i], &ptr);
  }

  serialize_uint16(entries_num, &ptr);
  for (uint32_t i = 0; i < entries_num; i++) {
    serialize_uint8(snapshots[i], &ptr);
  }

  serialize_uint16(entries_num, &ptr);
  for (uint32_t i = 0; i < entries_num; i++) {
    serialize_int16(thresholds[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_price_threshold_msg(const energy_price_threshold_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_int32(msg->price, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_environment_sunny_time_msg(const environment_sunny_time_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->percentage, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_environment_cloud_cover_msg(const environment_cloud_cover_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint64(msg->ref_timestamp, &ptr);
  serialize_uint16(msg->period, &ptr);
  serialize_uint16(msg->percentage_len, &ptr);
  for (uint16_t i = 0; i < msg->percentage_len; i++) {
    serialize_uint16(msg->percentage[i], &ptr);
  }
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_environment_position_msg(const environment_position_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_int64(msg->latitude, &ptr);
  serialize_int64(msg->longitude, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_meter_meas_msg(time_t timestamp, uint8_t *meas_type, int32_t *values, uint16_t entries_num,
                                   uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_METER_MEASUREMENT, &ptr);
  serialize_uint64(timestamp, &ptr);

  serialize_uint16(entries_num, &ptr);
  for (uint16_t i = 0; i < entries_num; i++) {
    serialize_uint8(meas_type[i], &ptr);
  }

  serialize_uint16(entries_num, &ptr);
  for (uint16_t i = 0; i < entries_num; i++) {
    serialize_int32(values[i], &ptr);
  }
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_meter_meas_history_msg(time_t ref_timestamp, uint8_t meas_type, uint32_t *t_offset, int32_t *values,
                                           uint16_t entries_num, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_METER_MEASUREMENT_HISTORY, &ptr);
  serialize_uint64(ref_timestamp, &ptr);
  serialize_uint8(meas_type, &ptr);

  serialize_uint16(entries_num, &ptr);
  for (uint16_t i = 0; i < entries_num; i++) {
    serialize_uint32(t_offset[i], &ptr);
  }

  serialize_uint16(entries_num, &ptr);
  for (uint16_t i = 0; i < entries_num; i++) {
    serialize_int32(values[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_meter_current_limit_msg(int32_t current_limit, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_METER_CURRENT_LIMIT, &ptr);
  serialize_int32(current_limit, &ptr);

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_meter_alarms_msg(uint64_t mask, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_METER_ALARMS, &ptr);
  serialize_uint64(mask, &ptr);

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_accumulated_msg(time_t timestamp, uint8_t meas_type, uint64_t energy, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_ENERGY_ACCUMULATED, &ptr);
  serialize_uint64(timestamp, &ptr);
  serialize_uint8(meas_type, &ptr);
  serialize_uint64(energy, &ptr);

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_energy_history_msg(time_t ref_timestamp, uint8_t meas_type, uint32_t interval, uint32_t *energy,
                                       uint16_t entries_num, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(MSGTYPE_ENERGY_HISTORY, &ptr);
  serialize_uint64(ref_timestamp, &ptr);
  serialize_uint8(meas_type, &ptr);
  serialize_uint32(interval, &ptr);

  serialize_uint16(entries_num, &ptr);
  for (uint32_t i = 0; i < entries_num; i++) {
    serialize_uint32(energy[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_diag_coredump_start_msg(const diag_coredump_start_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint32(msg->coredump_version, &ptr);
  serialize_uint32(msg->numeric_app_version, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_diag_coredump_body_msg(const diag_coredump_body_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint32(msg->coredump_version, &ptr);
  serialize_uint16(msg->block_len, &ptr);
  for (uint16_t i = 0; i < msg->block_len; i++) {
    serialize_uint8(msg->block_data[i], &ptr);
  }

  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_diag_coredump_end_msg(const diag_coredump_end_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint32(msg->coredump_version, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_diag_debug_info_msg(const diag_debug_info_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint16(msg->len, &ptr);
  for (uint16_t i = 0; i < msg->len; i++) {
    serialize_uint8(msg->data[i], &ptr);
  }
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_diag_logs_settings_msg(const diag_logs_settings_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->medium, &ptr);
  for (int i = 0; i < 6; i++) {
    serialize_uint64(msg->level_modules[i], &ptr);
  }
  return (ptrdiff_t)(ptr - buffer);
}

ptrdiff_t serialize_status_msg(const status_msg_t *msg, uint8_t *buffer)
{
  uint8_t *ptr = buffer;
  serialize_uint16(msg->type, &ptr);
  serialize_uint8(msg->error, &ptr);
  serialize_uint16(msg->rq_type, &ptr);
  return (ptrdiff_t)(ptr - buffer);
}
