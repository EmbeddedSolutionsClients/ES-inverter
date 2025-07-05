/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef SERIALIZER_H_
#define SERIALIZER_H_

#include "em/messages.h"

ptrdiff_t serialize_client_info_msg(const client_info_msg_t *msg, uint8_t *buffer);

ptrdiff_t serialize_installed_fw_msg(const installed_fw_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_update_status_msg(const update_status_msg_t *msg, uint8_t *buffer);

ptrdiff_t serialize_energy_price_day_get_msg(const energy_price_day_get_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_energy_price_threshold_msg(const energy_price_threshold_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_energy_price_cheap_period_msg(const energy_price_cheap_period_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_energy_price_active_price_limit_msg(const energy_price_active_price_limit_msg_t *msg,
                                                        uint8_t *buffer);
ptrdiff_t serialize_energy_price_tariff_msg(const energy_price_tariff_msg_t *msg, uint8_t *buffer);

ptrdiff_t serialize_wireless_status_msg(uint8_t protocol, uint8_t enabled, uint32_t ip, int8_t rssi, uint8_t lqi,
                                        uint8_t *buffer);
ptrdiff_t serialize_wireless_credentials_msg(const wireless_credentials_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_wireless_tx_power_msg(uint8_t protocol, int16_t tx_power, uint8_t *buffer);
ptrdiff_t serialize_wireless_channel_msg(uint8_t protocol, uint8_t channel, uint8_t *buffer);

ptrdiff_t serialize_relay_mode_msg(const relay_mode_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_state_msg(const relay_state_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_default_state_msg(const relay_default_state_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_pwm_msg(const relay_pwm_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_output_inverted_msg(uint8_t inverted, uint8_t *buffer);
ptrdiff_t serialize_relay_defined_modes_msg(const relay_button_config_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_schedule_msg(const relay_schedule_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_one_shot_msg(const relay_one_shot_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_relay_snapshots_msg(time_t ref_timestamp, int8_t price_factor, uint32_t *t_offset,
                                        uint8_t *snapshots, int16_t *thresholds, uint16_t entries_num, uint8_t *buffer);

ptrdiff_t serialize_environment_sunny_time_msg(const environment_sunny_time_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_environment_position_msg(const environment_position_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_environment_cloud_cover_msg(const environment_cloud_cover_msg_t *msg, uint8_t *buffer);

ptrdiff_t serialize_meter_meas_msg(time_t timestamp, uint8_t *meas_type, int32_t *values, uint16_t entries_num,
                                   uint8_t *buffer);
ptrdiff_t serialize_meter_meas_history_msg(time_t ref_timestamp, uint8_t meas_type, uint32_t *t_offset, int32_t *values,
                                           uint16_t entries_num, uint8_t *buffer);
ptrdiff_t serialize_meter_current_limit_msg(int32_t current_limit, uint8_t *buffer);
ptrdiff_t serialize_meter_alarms_msg(uint64_t mask, uint8_t *buffer);

ptrdiff_t serialize_energy_accumulated_msg(time_t timestamp, uint8_t meas_type, uint64_t energy, uint8_t *buffer);
ptrdiff_t serialize_energy_history_msg(time_t timestamp, uint8_t meas_type, uint32_t interval, uint32_t *energy,
                                       uint16_t entries_num, uint8_t *buffer);

ptrdiff_t serialize_diag_coredump_start_msg(const diag_coredump_start_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_diag_coredump_body_msg(const diag_coredump_body_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_diag_coredump_end_msg(const diag_coredump_end_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_diag_debug_info_msg(const diag_debug_info_msg_t *msg, uint8_t *buffer);
ptrdiff_t serialize_diag_logs_settings_msg(const diag_logs_settings_msg_t *msg, uint8_t *buffer);

ptrdiff_t serialize_status_msg(const status_msg_t *msg, uint8_t *buffer);

#endif /* SERIALIZER_H_ */
