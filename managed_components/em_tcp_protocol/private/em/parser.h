/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "em/messages.h"

#include <stdint.h>

msg_type_t parse_msg_type(const uint8_t *buf, uint32_t buf_len);
connection_status_msg_t parse_connection_status_msg(const uint8_t *buf, uint32_t buffer_len);

identify_msg_t parse_identify_msg(const uint8_t *buf, uint32_t buf_len);

update_start_msg_t parse_update_start_msg(const uint8_t *buf, uint32_t buf_len);
update_install_msg_t parse_update_install_msg(const uint8_t *buf, uint32_t buf_len);
update_stop_msg_t parse_update_stop_msg(const uint8_t *buf, uint32_t buf_len);

wireless_enable_msg_t parse_wireless_enable_msg(const uint8_t *buf, uint32_t buf_len);
wireless_credentials_msg_t parse_wireless_credentials_msg(const uint8_t *buf, uint32_t buf_len);
wireless_tx_power_msg_t parse_wireless_tx_power_msg(const uint8_t *buf, uint32_t buf_len);
wireless_channel_msg_t parse_wireless_channel_msg(const uint8_t *buf, uint32_t buf_len);

energy_price_day_msg_t parse_energy_price_day_msg(const uint8_t *buf, uint32_t buf_len);
energy_price_threshold_msg_t parse_energy_price_threshold_msg(const uint8_t *buf, uint32_t buf_len);
energy_price_cheap_period_msg_t parse_energy_price_cheap_period_msg(const uint8_t *buf, uint32_t buf_len);
energy_price_tariff_msg_t parse_energy_price_tariff_msg(const uint8_t *buf, uint32_t buf_len);

relay_mode_msg_t parse_relay_mode_msg(const uint8_t *buf, uint32_t buf_len);
relay_state_msg_t parse_relay_state_msg(const uint8_t *buf, uint32_t buf_len);
relay_default_state_msg_t parse_relay_default_state_msg(const uint8_t *buf, uint32_t buf_len);
relay_pwm_msg_t parse_relay_pwm_msg(const uint8_t *buf, uint32_t buf_len);
relay_inverted_output_msg_t parse_relay_set_inverted_output_msg(const uint8_t *buf, uint32_t buf_len);
relay_button_config_msg_t parse_relay_button_config_msg(const uint8_t *buf, uint32_t buf_len);
relay_schedule_msg_t parse_relay_schedule_msg(const uint8_t *buf, uint32_t buf_len);
relay_one_shot_msg_t parse_relay_one_shot_msg(const uint8_t *buf, uint32_t buf_len);

environment_sunny_time_msg_t parse_environment_sunny_time_msg(const uint8_t *buf, uint32_t buf_len);
environment_cloud_cover_msg_t parse_environment_cloud_cover_msg(const uint8_t *buf, uint32_t buf_len);
environment_position_msg_t parse_environment_position_msg(const uint8_t *buf, uint32_t buf_len);

meter_current_limit_msg_t parse_meter_current_limit_msg(const uint8_t *buf, uint32_t buf_len);
meter_alarms_msg_t parse_meter_clear_alarms_msg(const uint8_t *buf, uint32_t buf_len);

diag_set_logs_settings_msg_t parse_diag_set_logs_settings(const uint8_t *buf, uint32_t buf_len);

status_msg_t parse_status_msg(const uint8_t *buf, uint32_t buffer_len);
get_msg_t parse_get_msg(const uint8_t *buf, uint32_t buffer_len);

#endif /* PARSER_H_ */
