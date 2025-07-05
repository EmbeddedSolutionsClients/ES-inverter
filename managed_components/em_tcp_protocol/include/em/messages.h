/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

/* TODO: MAKE THE MESSAGES PACKED AND THEN WE CAN USE SIZEOF IN PARSER */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <stdint.h>
#include <time.h>
#include <sdkconfig.h>

#define IMAGE_URL_MAX_LEN     (128)
#define QUARTERS_IN_DAY       (24UL * 4UL)
#define MAX_DEFINED_MODES     (6)
/* Assuming 15minutes interval modes, 96 entries gives whole week */
#define MAX_SNAPSHOTS_ENTRIES (12)
#define MAX_MEAS_ENTRIES      (48)
#define MAX_MEAS_TYPES        (8)

typedef uint16_t msg_type_t;
enum {
  MSGTYPE_INVALID = 0x00,
  MSGTYPE_CLIENT_INFO = 0x01,
  MSGTYPE_CONNECTION_STATUS = 0x02,

  // SYSTEM
  MSGTYPE_REBOOT = 0x10,
  MSGTYPE_FACTORY_RESET = 0x11,
  MSGTYPE_IDENTIFY = 0x13,

  // UPDATE
  MSGTYPE_UPDATE_INSTALLEDFW = 0x20,
  MSGTYPE_UPDATE_START = 0x21,
  MSGTYPE_UPDATE_INSTALL = 0x22,
  MSGTYPE_UPDATE_STATUS = 0x23,
  MSGTYPE_UPDATE_STOP = 0x24,

  // CONNECTIVITY
  MSGTYPE_WIRELESS_ENABLE = 0x30,          // turn BLE on / off
  MSGTYPE_WIRELESS_STATUS = 0x31,          // RSSI, IP itp.
  MSGTYPE_WIRELESS_SET_CREDENTIALS = 0x32, // Set connection credentials
  MSGTYPE_WIRELESS_CREDENTIALS = 0x33,     // Connection credentials message
  MSGTYPE_WIRELESS_SET_TX_POWER = 0x34,
  MSGTYPE_WIRELESS_TX_POWER = 0x35,
  MSGTYPE_WIRELESS_SET_CHANNEL = 0x36,
  MSGTYPE_WIRELESS_CHANNEL = 0x37,

  // ENERGYPRICE
  MSGTYPE_ENERGYPRICE_DAY_GET = 0x42,
  MSGTYPE_ENERGYPRICE_DAY = 0x43,
  // 0x44 Reserved
  MSGTYPE_ENERGYPRICE_SET_PRICE_THRESHOLD = 0x46,
  MSGTYPE_ENERGYPRICE_PRICE_THRESHOLD = 0x47,
  MSGTYPE_ENERGYPRICE_SET_CHEAP_PERIOD = 0x48,
  MSGTYPE_ENERGYPRICE_CHEAP_PERIOD = 0x49,
  // 4a reserved
  MSGTYPE_ENERGYPRICE_ACTIVE_PRICE_LIMIT = 0x4b,
  MSGTYPE_ENERGYPRICE_SET_TARIFF = 0x4c,
  MSGTYPE_ENERGYPRICE_TARIFF = 0x4d,

  // RELAY
  MSGTYPE_RELAY_SET_MODE = 0x50,
  MSGTYPE_RELAY_MODE = 0x51,
  MSGTYPE_RELAY_SET_STATE = 0x52,
  MSGTYPE_RELAY_STATE = 0x53,
  MSGTYPE_RELAY_SET_DEFAULT_STATE = 0x54,
  MSGTYPE_RELAY_DEFAULT_STATE = 0x55,
  MSGTYPE_RELAY_SET_PWM = 0x56,
  MSGTYPE_RELAY_PWM = 0x57,
  MSGTYPE_RELAY_SET_INVERTED_OUTPUT = 0x58,
  MSGTYPE_RELAY_INVERTED_OUTPUT = 0x59,
  MSGTYPE_RELAY_SET_BUTTON_CONFIG = 0x5a,
  MSGTYPE_RELAY_BUTTON_CONFIG = 0x5b,
  MSGTYPE_RELAY_SET_SCHEDULE = 0x5c,
  MSGTYPE_RELAY_SCHEDULE = 0x5d,
  MSGTYPE_RELAY_SET_ONE_SHOT = 0x5e,
  MSGTYPE_RELAY_ONE_SHOT = 0x5f,
  // 0x60 Reserved
  MSGTYPE_RELAY_SNAPSHOTS = 0x61,

  // ENVIRONMENT
  MSGTYPE_ENVIRONMENT_SET_SUNNY_TIME = 0x70,
  MSGTYPE_ENVIRONMENT_SUNNY_TIME = 0x71,
  MSGTYPE_ENVIRONMENT_SET_CLOUD_COVER = 0x72,
  MSGTYPE_ENVIRONMENT_CLOUD_COVER = 0x73,
  MSGTYPE_ENVIRONMENT_SET_POSITION = 0x74,
  MSGTYPE_ENVIRONMENT_POSITION = 0x75,

  // BMS
  MSGTYPE_BMS_SOC = 0xA5,
  MSGTYPE_BMS_CYCLES = 0xAB,

  // METER
  MSGTYPE_METER_MEASUREMENT = 0xB0,
  MSGTYPE_METER_MEASUREMENT_HISTORY = 0xB1,
  MSGTYPE_METER_SET_CURRENT_LIMIT = 0xB2,
  MSGTYPE_METER_CURRENT_LIMIT = 0xB3,
  MSGTYPE_METER_CLEAR_ALARMS = 0xB4, // Set alarms mask
  MSGTYPE_METER_ALARMS = 0xB5,       // Get alarms mask

  // ENERGY
  MSGTYPE_ENERGY_ACCUMULATED = 0xC0, // accumulated energy
  MSGTYPE_ENERGY_HISTORY = 0xC1,     // accumulated energy history

  // COREDUMP
  MSGTYPE_DIAG_COREDUMP_START = 0xD0,
  MSGTYPE_DIAG_COREDUMP_BODY = 0xD1,
  MSGTYPE_DIAG_COREDUMP_END = 0xD2,
  MSGTYPE_DIAG_COREDUMP_CONFIRMED = 0xD3,
  MSGTYPE_DIAG_DEBUG_INFO = 0xD4,
  // reserved 0xD5
  MSGTYPE_DIAG_SET_LOGS_SETTINGS = 0xD6, // Set debug logs level
  MSGTYPE_DIAG_LOGS_SETTINGS = 0xD7,     // Get debug logs level

  // STATUS
  MSGTYPE_STATUS = 0xF0,
  MSGTYPE_GET = 0xF1,
};

typedef struct {
  msg_type_t type;
  uint16_t dev_type;
  uint64_t dev_id;
  uint32_t unique_id;
  uint32_t ip_addr;
  uint8_t hw_ver;
  uint32_t app_ver;
  uint8_t reconn_reason;
  uint8_t reset_reason;
  uint8_t reserved;
  uint16_t session_id;
  uint32_t CRC;
} __attribute__((packed)) client_info_msg_t;

typedef struct {
  msg_type_t type;
  uint64_t dev_id; // unique ID provided by specific device
  uint32_t unique_id;
  uint16_t reserved;
  uint16_t session_id;
  uint32_t CRC;
} connection_status_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t mode;
  uint8_t count;
} identify_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t inst_fw_ver;
  uint32_t downloaded_fw_ver;
  uint8_t hw_ver;
  uint64_t build_time;
} installed_fw_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t dev_type;
  uint8_t hw_ver;
  uint32_t fw_ver;
  uint32_t img_size;
  uint32_t CRC;
  uint8_t update_policy;
  uint16_t img_url_len;
  char img_url[IMAGE_URL_MAX_LEN + 1];
} update_start_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t fw_ver;
  uint8_t progress;
  uint8_t phase;
  uint8_t error;
} update_status_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t fw_ver;
  uint32_t delay;
} update_install_msg_t;

typedef struct {
  msg_type_t type;
} update_stop_msg_t;

typedef struct {
  msg_type_t type;
  int32_t limit; // in mA
} meter_current_limit_msg_t;

typedef struct {
  msg_type_t type;
  uint64_t mask;
} meter_alarms_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t tariff;
  time_t day_time;
  int8_t price_factor;
  uint16_t prices_len;
  int16_t prices[QUARTERS_IN_DAY];
} energy_price_day_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t tariff;
  time_t day_time;
} energy_price_day_get_msg_t;

typedef struct {
  msg_type_t type;
  int32_t price;
  uint16_t next;
} energy_price_now_msg_t;

typedef struct {
  msg_type_t type;
  int32_t price;
} energy_price_threshold_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t quarters;
} energy_price_cheap_period_msg_t;

typedef struct {
  msg_type_t type;
  int32_t price;
} energy_price_active_price_limit_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t tariff;
} energy_price_tariff_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t protocol; // WiFi or BLE etc
  uint8_t enabled;  // 0 - disabled, 1 - enabled
} wireless_enable_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t protocol;
  uint8_t enabled;
  uint32_t ip_addr; // IPv4 address
  int8_t rssi;      // in dBm
  uint8_t lqi;
} wireless_status_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t protocol;     // WiFi or BLE
  uint16_t network_len; // length of network name
  uint8_t network[32];
  uint16_t password_len; // length of password
  uint8_t password[64];
} wireless_credentials_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t protocol;
  int16_t tx_power; // in dBm
} wireless_tx_power_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t protocol; // WiFi or BLE
  uint8_t channel;  // WiFi channel
} wireless_channel_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t mode;
} relay_mode_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t state;
} relay_state_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t state;
} relay_default_state_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t enabled;
  uint16_t period;    // in minutes
  uint8_t duty_cycle; // in %
} relay_pwm_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t inverted;
} relay_inverted_output_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t enabled;
  time_t start;
  time_t end;
  uint8_t state;
} relay_one_shot_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t mode_len;
  uint8_t modes[MAX_DEFINED_MODES];
} relay_button_config_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t schedule_len;
  uint8_t schedule[QUARTERS_IN_DAY * 7 / 8]; // Pattern for the whole week. Each bit represent 15 minute interval.
} relay_schedule_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t percentage; // percentage of sunny hours
} environment_sunny_time_msg_t;

typedef struct {
  msg_type_t type;
  time_t ref_timestamp;
  uint16_t period;
  uint16_t percentage_len;
  uint16_t percentage[QUARTERS_IN_DAY];
} environment_cloud_cover_msg_t;

typedef struct {
  msg_type_t type;
  int64_t latitude;
  int64_t longitude;
} environment_position_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t coredump_version;
  uint32_t numeric_app_version;
} diag_coredump_start_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t coredump_version;
  uint16_t block_len;
  uint8_t block_data[CONFIG_EM_COREDUMP_BLOCK_MAX_SIZE];
} diag_coredump_body_msg_t;

typedef struct {
  msg_type_t type;
  uint32_t coredump_version;
} diag_coredump_end_msg_t;

typedef struct {
  msg_type_t type;
  uint16_t len;
  uint8_t data[CONFIG_EM_COREDUMP_BLOCK_MAX_SIZE];
} diag_debug_info_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t medium; // 0 - disabled, 1 - uart, 2 - TCP
  uint64_t level_modules[6];
} diag_logs_settings_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t medium;   // 0 - disabled, 1 - uart, 2 - TCP
  uint8_t level;    // 0 - off, 1 - error, 2 - warning, 3 - info, 4 - debug, 5 - trace
  uint64_t modules; // bitmas
} diag_set_logs_settings_msg_t;

typedef struct {
  msg_type_t type;
  uint8_t error;
  msg_type_t rq_type;
} status_msg_t;

typedef struct {
  msg_type_t type;
  msg_type_t rq_type;
  uint32_t param;
} get_msg_t;

#endif /* MESSAGES_H_ */
