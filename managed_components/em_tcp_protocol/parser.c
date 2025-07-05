/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/parser.h"

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <common.h>

static int8_t parse_int8(const uint8_t **buf)
{
  int8_t value = (*buf)[0];
  *buf += sizeof(int8_t);
  return value;
}

static uint8_t parse_uint8(const uint8_t **buf)
{
  uint8_t value = (*buf)[0];
  *buf += sizeof(uint8_t);
  return value;
}

static uint16_t parse_uint16(const uint8_t **buf)
{
  uint16_t value = (uint16_t)(*buf)[0] | (uint16_t)(*buf)[1] << 8;
  *buf += sizeof(uint16_t);
  return value;
}

static int16_t parse_int16(const uint8_t **buf)
{
  int16_t value = (int16_t)(*buf)[0] | (int16_t)(*buf)[1] << 8;
  *buf += sizeof(int16_t);
  return value;
}

static uint32_t parse_uint32(const uint8_t **buf)
{
  uint32_t value =
    (uint32_t)(*buf)[0] | (uint32_t)(*buf)[1] << 8 | (uint32_t)(*buf)[2] << 16 | (uint32_t)(*buf)[3] << 24;
  *buf += sizeof(uint32_t);
  return value;
}

static int32_t parse_int32(const uint8_t **buf)
{
  int32_t value = (int32_t)(*buf)[0] | (int32_t)(*buf)[1] << 8 | (int32_t)(*buf)[2] << 16 | (int32_t)(*buf)[3] << 24;
  *buf += sizeof(int32_t);
  return value;
}

static uint64_t parse_uint64(const uint8_t **buf)
{
  uint64_t value = (uint64_t)(*buf)[0] | (uint64_t)(*buf)[1] << 8 | (uint64_t)(*buf)[2] << 16 |
                   (uint64_t)(*buf)[3] << 24 | (uint64_t)(*buf)[4] << 32 | (uint64_t)(*buf)[5] << 40 |
                   (uint64_t)(*buf)[6] << 48 | (uint64_t)(*buf)[7] << 56;
  *buf += sizeof(uint64_t);
  return value;
}

static int64_t parse_int64(const uint8_t **buf)
{
  int64_t value = (int64_t)(*buf)[0] | (int64_t)(*buf)[1] << 8 | (int64_t)(*buf)[2] << 16 | (int64_t)(*buf)[3] << 24 |
                  (int64_t)(*buf)[4] << 32 | (int64_t)(*buf)[5] << 40 | (int64_t)(*buf)[6] << 48 |
                  (int64_t)(*buf)[7] << 56;
  *buf += sizeof(int64_t);
  return value;
}

static void parse_string(char *str, const uint8_t **buf, size_t length)
{
  memcpy(str, *buf, length);
  *buf += length;
}

uint16_t parse_msg_type(const uint8_t *buf, uint32_t buf_len)
{
  if (buf_len < sizeof(msg_type_t)) {
    return MSGTYPE_INVALID;
  }

  const uint8_t *ptr = buf;
  return parse_uint16(&ptr);
}

connection_status_msg_t parse_connection_status_msg(const uint8_t *buf, uint32_t buf_len)
{
  connection_status_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint64_t) + 3 * sizeof(uint32_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.dev_id = parse_uint64(&ptr);
  msg.unique_id = parse_uint32(&ptr);
  msg.reserved = parse_uint16(&ptr);
  msg.session_id = parse_uint16(&ptr);
  msg.CRC = parse_uint32(&ptr);
  return msg;
}

identify_msg_t parse_identify_msg(const uint8_t *buf, uint32_t buf_len)
{
  identify_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + 2 * sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.mode = parse_uint8(&ptr);
  msg.count = parse_uint8(&ptr);
  return msg;
}

update_start_msg_t parse_update_start_msg(const uint8_t *buf, uint32_t buf_len)
{
  update_start_msg_t msg = {0};

  const uint32_t fixed_size = 3 * sizeof(uint32_t) + 3 * sizeof(uint16_t) + 2 * sizeof(uint8_t);

  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.dev_type = parse_uint16(&ptr);
  msg.hw_ver = parse_uint8(&ptr);
  msg.fw_ver = parse_uint32(&ptr);
  msg.img_size = parse_uint32(&ptr);
  msg.CRC = parse_uint32(&ptr);
  msg.update_policy = parse_uint8(&ptr);

  msg.img_url_len = parse_uint16(&ptr);

  if (buf_len != fixed_size + msg.img_url_len) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  assert(msg.img_url_len < sizeof(msg.img_url) - 1);

  parse_string(msg.img_url, &ptr, msg.img_url_len);
  msg.img_url[msg.img_url_len] = '\0';

  return msg;
}

update_install_msg_t parse_update_install_msg(const uint8_t *buf, uint32_t buf_len)
{
  update_install_msg_t msg = {0};

  if (buf_len != 2 * sizeof(uint32_t) + sizeof(uint16_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.fw_ver = parse_uint32(&ptr);
  msg.delay = parse_uint32(&ptr);
  return msg;
}

update_stop_msg_t parse_update_stop_msg(const uint8_t *buf, uint32_t buf_len)
{
  update_stop_msg_t msg = {0};

  if (buf_len != sizeof(msg_type_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  return msg;
}

meter_current_limit_msg_t parse_meter_current_limit_msg(const uint8_t *buf, uint32_t buf_len)
{
  meter_current_limit_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(int32_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.limit = parse_int32(&ptr);
  return msg;
}

meter_alarms_msg_t parse_meter_clear_alarms_msg(const uint8_t *buf, uint32_t buf_len)
{
  meter_alarms_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint64_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.mask = parse_uint64(&ptr);
  return msg;
}

energy_price_day_msg_t parse_energy_price_day_msg(const uint8_t *buf, uint32_t buf_len)
{
  energy_price_day_msg_t msg = {0};

  const uint32_t fixed_size = sizeof(msg.day_time) + sizeof(msg.price_factor) + sizeof(msg.type) + sizeof(msg.tariff) +
                              sizeof(msg.prices_len) + sizeof(msg.prices);
  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.tariff = parse_uint16(&ptr);
  msg.day_time = parse_uint64(&ptr);
  msg.price_factor = parse_int8(&ptr);
  msg.prices_len = parse_uint16(&ptr);

  for (uint32_t i = 0; i < ARRAY_LENGTH(msg.prices); i++) {
    msg.prices[i] = parse_int16(&ptr);
  }

  return msg;
}

energy_price_threshold_msg_t parse_energy_price_threshold_msg(const uint8_t *buf, uint32_t buf_len)
{
  energy_price_threshold_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(int32_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.price = parse_int32(&ptr);
  return msg;
}

energy_price_cheap_period_msg_t parse_energy_price_cheap_period_msg(const uint8_t *buf, uint32_t buf_len)
{
  energy_price_cheap_period_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.quarters = parse_uint8(&ptr);
  return msg;
}

energy_price_tariff_msg_t parse_energy_price_tariff_msg(const uint8_t *buf, uint32_t buf_len)
{
  energy_price_tariff_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint16_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.tariff = parse_uint16(&ptr);
  return msg;
}

wireless_enable_msg_t parse_wireless_enable_msg(const uint8_t *buf, uint32_t buf_len)
{
  wireless_enable_msg_t msg = {0};
  if (buf_len != 2 * sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.protocol = parse_uint8(&ptr);
  msg.enabled = parse_uint8(&ptr);
  return msg;
}

wireless_credentials_msg_t parse_wireless_credentials_msg(const uint8_t *buf, uint32_t buf_len)
{
  wireless_credentials_msg_t msg = {0};

  const uint32_t fixed_size = sizeof(uint16_t) + sizeof(msg.protocol) + 2 * sizeof(uint16_t);
  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.protocol = parse_uint8(&ptr);

  assert(sizeof(msg.network) < sizeof(msg.network_len) - 1);
  assert(sizeof(msg.password) < sizeof(msg.password_len) - 1);

  parse_string((char *)msg.network, &ptr, sizeof(msg.network_len));
  parse_string((char *)msg.password, &ptr, sizeof(msg.password_len));
  msg.network[msg.network_len] = '\0';
  msg.password[msg.password_len] = '\0';

  return msg;
}

wireless_tx_power_msg_t parse_wireless_tx_power_msg(const uint8_t *buf, uint32_t buf_len)
{
  wireless_tx_power_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t) + sizeof(int8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.protocol = parse_uint8(&ptr);
  msg.tx_power = parse_int16(&ptr);
  return msg;
}

wireless_channel_msg_t parse_wireless_channel_msg(const uint8_t *buf, uint32_t buf_len)
{
  wireless_channel_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.protocol = parse_uint8(&ptr);
  msg.channel = parse_uint8(&ptr);
  return msg;
}

relay_mode_msg_t parse_relay_mode_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_mode_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.mode = parse_uint8(&ptr);
  return msg;
}

relay_state_msg_t parse_relay_state_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_state_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.state = parse_uint8(&ptr);
  return msg;
}

relay_default_state_msg_t parse_relay_default_state_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_default_state_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.state = parse_uint8(&ptr);
  return msg;
}

relay_pwm_msg_t parse_relay_pwm_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_pwm_msg_t msg = {0};

  if (buf_len != 2 * sizeof(uint16_t) + 2 * sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.enabled = parse_uint8(&ptr);
  msg.period = parse_uint16(&ptr);
  msg.duty_cycle = parse_uint8(&ptr);
  return msg;
}

relay_inverted_output_msg_t parse_relay_set_inverted_output_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_inverted_output_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.inverted = parse_uint8(&ptr);
  return msg;
}

relay_one_shot_msg_t parse_relay_one_shot_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_one_shot_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.enabled = parse_uint8(&ptr);
  msg.start = parse_uint64(&ptr);
  msg.end = parse_uint64(&ptr);
  msg.state = parse_uint8(&ptr);

  return msg;
}

relay_button_config_msg_t parse_relay_button_config_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_button_config_msg_t msg = {0};

  uint32_t fixed_size = sizeof(uint16_t) + sizeof(uint16_t);
  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.mode_len = parse_uint16(&ptr);

  if (buf_len != fixed_size + msg.mode_len * sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  for (int i = 0; i < ARRAY_LENGTH(msg.modes); i++) {
    msg.modes[i] = parse_uint8(&ptr);
  }

  return msg;
}

relay_schedule_msg_t parse_relay_schedule_msg(const uint8_t *buf, uint32_t buf_len)
{
  relay_schedule_msg_t msg = {0};

  uint32_t fixed_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(msg.schedule);
  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.schedule_len = parse_uint16(&ptr);

  assert(msg.schedule_len == ARRAY_LENGTH(msg.schedule));

  for (int i = 0; i < ARRAY_LENGTH(msg.schedule); i++) {
    msg.schedule[i] = parse_uint8(&ptr);
  }

  return msg;
}

environment_sunny_time_msg_t parse_environment_sunny_time_msg(const uint8_t *buf, uint32_t buf_len)
{
  environment_sunny_time_msg_t msg = {0};

  if (buf_len != (sizeof(uint16_t) + sizeof(uint8_t))) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.percentage = parse_uint8(&ptr);
  return msg;
}

environment_cloud_cover_msg_t parse_environment_cloud_cover_msg(const uint8_t *buf, uint32_t buf_len)
{
  environment_cloud_cover_msg_t msg = {0};
  uint32_t fixed_size = sizeof(uint16_t) + sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint16_t);

  if (buf_len < fixed_size) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.ref_timestamp = parse_uint64(&ptr);
  msg.period = parse_uint16(&ptr);
  msg.percentage_len = parse_uint16(&ptr);
  assert(msg.percentage_len == ARRAY_LENGTH(msg.percentage));
  for (int i = 0; i < ARRAY_LENGTH(msg.percentage); i++) {
    msg.percentage[i] = parse_uint16(&ptr);
  }
  return msg;
}

environment_position_msg_t parse_environment_position_msg(const uint8_t *buf, uint32_t buf_len)
{
  environment_position_msg_t msg = {0};

  if (buf_len < (sizeof(uint16_t) + 2 * sizeof(int32_t))) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.latitude = parse_int64(&ptr);
  msg.longitude = parse_int64(&ptr);
  return msg;
}

diag_set_logs_settings_msg_t parse_diag_set_logs_settings(const uint8_t *buf, uint32_t buf_len)
{
  diag_set_logs_settings_msg_t msg = {0};

  if (buf_len != sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t) + 6 * sizeof(uint64_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.medium = parse_uint8(&ptr);
  msg.level = parse_uint8(&ptr);
  msg.modules = parse_uint64(&ptr);

  return msg;
}

status_msg_t parse_status_msg(const uint8_t *buf, uint32_t buf_len)
{
  status_msg_t msg = {0};

  if (buf_len != 2 * sizeof(uint16_t) + sizeof(uint8_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.error = parse_uint8(&ptr);
  msg.rq_type = parse_uint16(&ptr);
  return msg;
}

get_msg_t parse_get_msg(const uint8_t *buf, uint32_t buf_len)
{
  get_msg_t msg = {0};

  if (buf_len != 2 * sizeof(uint16_t) + sizeof(uint32_t)) {
    msg.type = (msg_type_t)MSGTYPE_INVALID;
    return msg;
  }

  const uint8_t *ptr = buf;
  msg.type = parse_uint16(&ptr);
  msg.rq_type = parse_uint16(&ptr);
  msg.param = parse_uint32(&ptr);
  return msg;
}
