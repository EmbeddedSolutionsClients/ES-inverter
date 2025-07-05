/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <stddef.h>
#include <string.h>

#include "em/buffer.h"
#include "em/messages.h"
#include "em/protocol.h"
#include "em/serializer.h"

#include <esp_log.h>
#include <esp_random.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#define LOG_TAG "em_tcp_tx"

static net_tx_medium_cb_t net_tx_handler;

static int send_data(buffer_t *data)
{
  if (net_tx_handler) {
    int ret = net_tx_handler(data);

    if (ret != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Send data err: %d", ret);
      em_buffer_free(data);
    }
  } else {
    ESP_LOGW(LOG_TAG, "TCP TX handler missing");
    em_buffer_free(data);
    return ESP_ERR_INVALID_STATE;
  }

  return ESP_OK;
}

int protocol_send_client_info(uint32_t unique_id, uint64_t dev_id, uint8_t hw_ver, uint32_t app_ver,
                              uint8_t reconn_reason, uint16_t dev_type, uint32_t ip_addr, uint8_t reset_reason,
                              uint16_t session_id)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);

  client_info_msg_t msg = {.type = (uint16_t)MSGTYPE_CLIENT_INFO,
                           .dev_type = dev_type,
                           .dev_id = dev_id,
                           .unique_id = unique_id,
                           .ip_addr = ip_addr,
                           .hw_ver = hw_ver,
                           .app_ver = app_ver,
                           .reconn_reason = reconn_reason,
                           .reset_reason = reset_reason,
                           .session_id = session_id,
                           .CRC = 0};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(client_info_msg_t));
  serialized.len = serialize_client_info_msg(&msg, serialized.data);

  // calculate crc32 over the whole message excluding the crc field
  msg.CRC = esp_rom_crc32_le(0, serialized.data, serialized.len - sizeof(msg.CRC));

  serialized.len = serialize_client_info_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_installed_fw(uint32_t fw_ver, uint32_t downloaded_fw_ver, uint8_t hw_ver, time_t build_time)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  installed_fw_msg_t msg = {
    .type = (uint16_t)MSGTYPE_UPDATE_INSTALLEDFW,
    .inst_fw_ver = fw_ver,
    .downloaded_fw_ver = downloaded_fw_ver,
    .hw_ver = hw_ver,
    .build_time = build_time,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(installed_fw_msg_t));
  serialized.len = serialize_installed_fw_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_update_status(uint32_t fw_ver, uint8_t progress, uint8_t phase, uint8_t error)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  update_status_msg_t msg = {
    .type = (msg_type_t)MSGTYPE_UPDATE_STATUS,
    .fw_ver = fw_ver,
    .progress = progress,
    .phase = phase,
    .error = error,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(update_status_msg_t));
  serialized.len = serialize_update_status_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_wireless_status(uint8_t protocol, uint8_t enabled, uint32_t ip, int8_t rssi, uint8_t lqi)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);

  size_t len =
    sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(int8_t) + sizeof(uint8_t);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_wireless_status_msg(protocol, enabled, ip, rssi, lqi, serialized.data);
  return send_data(&serialized);
}

int protocol_send_wireless_credentials(uint8_t protocol, const char *network, const char *password)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);

  wireless_credentials_msg_t msg = {
    .type = (uint16_t)MSGTYPE_WIRELESS_SET_CREDENTIALS,
    .protocol = protocol,
  };

  size_t network_len = strnlen(network, sizeof(msg.network));
  size_t password_len = strnlen(password, sizeof(msg.password));

  msg.network_len = (uint16_t)network_len;
  msg.password_len = (uint16_t)password_len;
  memcpy(msg.network, network, network_len);
  memcpy(msg.password, password, password_len);

  size_t len = sizeof(msg.type) + sizeof(msg.protocol) + sizeof(msg.network_len) + network_len +
               sizeof(msg.password_len) + password_len;

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_wireless_credentials_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_wireless_tx_power(uint8_t protocol, int16_t tx_power)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(uint16_t) + sizeof(uint8_t) + sizeof(int16_t));
  serialized.len = serialize_wireless_tx_power_msg(protocol, tx_power, serialized.data);
  return send_data(&serialized);
}

int protocol_send_wireless_channel(uint8_t protocol, uint8_t channel)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(uint16_t) + 2 * sizeof(uint8_t));
  serialized.len = serialize_wireless_tx_power_msg(protocol, channel, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_price_day_get(uint32_t req_tariff, time_t req_day_time)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  energy_price_day_get_msg_t msg = {
    .type = MSGTYPE_ENERGYPRICE_DAY_GET,
    .tariff = req_tariff,
    .day_time = req_day_time,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(energy_price_day_get_msg_t));
  serialized.len = serialize_energy_price_day_get_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_threshold(int32_t price)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  energy_price_threshold_msg_t msg = {.type = (uint16_t)MSGTYPE_ENERGYPRICE_PRICE_THRESHOLD, .price = price};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(energy_price_threshold_msg_t));
  serialized.len = serialize_energy_price_threshold_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_cheap_period(uint8_t quarters)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  energy_price_cheap_period_msg_t msg = {.type = (uint16_t)MSGTYPE_ENERGYPRICE_CHEAP_PERIOD, .quarters = quarters};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(energy_price_cheap_period_msg_t));
  serialized.len = serialize_energy_price_cheap_period_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_active_price_limit(int32_t price)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  energy_price_active_price_limit_msg_t msg = {
    .type = MSGTYPE_ENERGYPRICE_ACTIVE_PRICE_LIMIT,
    .price = price,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(energy_price_active_price_limit_msg_t));
  serialized.len = serialize_energy_price_active_price_limit_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_sunnyhours(uint8_t percentage)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  environment_sunny_time_msg_t msg = {.type = (uint16_t)MSGTYPE_ENVIRONMENT_SUNNY_TIME, .percentage = percentage};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(environment_sunny_time_msg_t));
  serialized.len = serialize_environment_sunny_time_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_tariff(uint16_t tariff)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  energy_price_tariff_msg_t msg = {.type = (uint16_t)MSGTYPE_ENERGYPRICE_TARIFF, .tariff = tariff};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(energy_price_tariff_msg_t));
  serialized.len = serialize_energy_price_tariff_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_mode(uint8_t mode)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_mode_msg_t msg = {.type = (uint16_t)MSGTYPE_RELAY_MODE, .mode = mode};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_mode_msg_t));
  serialized.len = serialize_relay_mode_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_state(uint8_t state)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_state_msg_t msg = {.type = (uint16_t)MSGTYPE_RELAY_STATE, .state = state};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_state_msg_t));
  serialized.len = serialize_relay_state_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_def_state(uint8_t enabled)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_default_state_msg_t msg = {.type = (uint16_t)MSGTYPE_RELAY_DEFAULT_STATE, .state = enabled};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_default_state_msg_t));
  serialized.len = serialize_relay_default_state_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_pwm(uint8_t enabled, uint16_t period_min, uint8_t duty_cycle)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_pwm_msg_t msg = {
    .type = (uint16_t)MSGTYPE_RELAY_PWM, .enabled = enabled, .period = period_min, .duty_cycle = duty_cycle};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_pwm_msg_t));
  serialized.len = serialize_relay_pwm_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_inverted_output(uint8_t inverted)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(uint16_t) + sizeof(uint8_t));
  serialized.len = serialize_relay_output_inverted_msg(inverted, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_button_config(uint8_t *modes, uint8_t mode_len)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_button_config_msg_t msg = {.type = (uint16_t)MSGTYPE_RELAY_BUTTON_CONFIG, .mode_len = mode_len};
  memcpy(msg.modes, modes, mode_len * sizeof(*modes));
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_button_config_msg_t));
  serialized.len = serialize_relay_defined_modes_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_schedule(uint8_t *schedule, uint8_t len)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_schedule_msg_t msg = {.type = (uint16_t)MSGTYPE_RELAY_SCHEDULE, .schedule_len = len};
  memcpy(msg.schedule, schedule, len * sizeof(*schedule));
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_schedule_msg_t));
  serialized.len = serialize_relay_schedule_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_one_shot(uint8_t enabled, time_t start, time_t end, uint8_t state)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  relay_one_shot_msg_t msg = {
    .type = MSGTYPE_RELAY_ONE_SHOT,
    .enabled = enabled,
    .start = start,
    .end = end,
    .state = state,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(relay_one_shot_msg_t));
  serialized.len = serialize_relay_one_shot_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_relay_snapshots(time_t ref_timestamp, int8_t factor, uint32_t *t_offset, uint8_t *snapshot,
                                  int16_t *thresholds, uint16_t entries_num)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);

  size_t len = sizeof(uint16_t) + sizeof(ref_timestamp) + sizeof(factor) +
               entries_num * (sizeof(*t_offset) + sizeof(*snapshot) + sizeof(*thresholds)) + 3 * sizeof(entries_num);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len); // TODO: this structure is not packed so too much space is allocated
  serialized.len =
    serialize_relay_snapshots_msg(ref_timestamp, factor, t_offset, snapshot, thresholds, entries_num, serialized.data);
  return send_data(&serialized);
}

int protocol_send_environment_position(int32_t latitude, int32_t longitude)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  environment_position_msg_t msg = {
    .type = (uint16_t)MSGTYPE_ENVIRONMENT_POSITION,
    .latitude = latitude,
    .longitude = longitude,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(environment_position_msg_t));
  serialized.len = serialize_environment_position_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_meter_measurements(time_t ref_timestamp, uint8_t *meas_types, int32_t *values, uint16_t entries_num)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  size_t len = sizeof(uint16_t) + sizeof(ref_timestamp) + entries_num * (sizeof(*meas_types) + sizeof(*values)) +
               2 * sizeof(entries_num);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_meter_meas_msg(ref_timestamp, meas_types, values, entries_num, serialized.data);
  return send_data(&serialized);
}

int protocol_send_meter_measurements_history(time_t ref_timestamp, uint8_t meas_type, uint32_t *t_offset,
                                             int32_t *values, uint16_t entries_num)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  size_t len = sizeof(uint16_t) + sizeof(ref_timestamp) + sizeof(meas_type) +
               entries_num * (sizeof(*t_offset) + sizeof(*values)) + 2 * sizeof(entries_num);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len =
    serialize_meter_meas_history_msg(ref_timestamp, meas_type, t_offset, values, entries_num, serialized.data);
  return send_data(&serialized);
}

int protocol_send_meter_current_limit(int32_t limit)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  size_t len = sizeof(uint16_t) + sizeof(limit);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_meter_current_limit_msg(limit, serialized.data);
  return send_data(&serialized);
}

int protocol_send_meter_alarms(uint64_t mask)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  size_t len = sizeof(uint16_t) + sizeof(mask);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_meter_alarms_msg(mask, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_accumulated(time_t timestamp, uint8_t meas_type, uint64_t energy)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  size_t len = sizeof(uint16_t) + sizeof(timestamp) + sizeof(meas_type) + sizeof(energy);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len);
  serialized.len = serialize_energy_accumulated_msg(timestamp, meas_type, energy, serialized.data);
  return send_data(&serialized);
}

int protocol_send_energy_history(uint8_t meas_type, time_t ref_timestamp, uint32_t interval, uint32_t *entries,
                                 uint16_t entries_num)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);

  size_t len = sizeof(uint16_t) + sizeof(meas_type) + sizeof(ref_timestamp) + sizeof(interval) +
               entries_num * sizeof(*entries) + sizeof(entries_num);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, len); // TODO: this structure is not packed so too much space is allocated
  serialized.len =
    serialize_energy_history_msg(ref_timestamp, meas_type, interval, entries, entries_num, serialized.data);
  return send_data(&serialized);
}

int protocol_send_diag_coredump_start(uint32_t coredump_version)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  diag_coredump_start_msg_t msg = {
    .type = MSGTYPE_DIAG_COREDUMP_START,
    .numeric_app_version = 0,
    .coredump_version = coredump_version,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(diag_coredump_start_msg_t));
  serialized.len = serialize_diag_coredump_start_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_diag_coredump_body(uint32_t coredump_version, uint16_t block_len, uint8_t *data)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  diag_coredump_body_msg_t msg = {
    .type = MSGTYPE_DIAG_COREDUMP_BODY,
    .block_len = block_len,
    .coredump_version = coredump_version,
  };

  assert(block_len <= sizeof(msg.block_data));
  memcpy(msg.block_data, data, block_len);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(diag_coredump_body_msg_t));
  serialized.len = serialize_diag_coredump_body_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_diag_coredump_end(uint32_t coredump_version)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  diag_coredump_end_msg_t msg = {
    .type = MSGTYPE_DIAG_COREDUMP_END,
    .coredump_version = coredump_version,
  };

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(diag_coredump_end_msg_t));
  serialized.len = serialize_diag_coredump_end_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_diag_debug_info(const char *info)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  diag_debug_info_msg_t msg = {
    .type = MSGTYPE_DIAG_DEBUG_INFO,
  };

  msg.len = (uint16_t)strnlen(info, sizeof(msg.data));
  memcpy(msg.data, info, msg.len);

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(msg.type) + sizeof(msg.len) + msg.len);
  serialized.len = serialize_diag_debug_info_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_diag_logs_settings(uint8_t medium, uint64_t *moduls_levels)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  diag_logs_settings_msg_t msg = {.type = MSGTYPE_DIAG_LOGS_SETTINGS, .medium = 0};

  for (int i = 0; i < 6; i++) {
    msg.level_modules[i] = moduls_levels[i];
  }

  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(msg.type) + sizeof(msg.medium) + 6 * sizeof(msg.level_modules));
  serialized.len = serialize_diag_logs_settings_msg(&msg, serialized.data);
  return send_data(&serialized);
}

int protocol_send_status(uint16_t rq_type, uint8_t error)
{
  ESP_LOGD(LOG_TAG, "%s", __func__);
  status_msg_t msg = {.type = (uint16_t)MSGTYPE_STATUS, .error = error, .rq_type = rq_type};
  buffer_t serialized = {0};
  buffer_dynamic_alloc(&serialized, sizeof(status_msg_t));
  serialized.len = serialize_status_msg(&msg, serialized.data);
  return send_data(&serialized);
}

void protocol_register_tx_handler(net_tx_medium_cb_t handler)
{
  net_tx_handler = handler;
}
