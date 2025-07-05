/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/messages.h"
#include "em/parser.h"
#include "em/protocol.h"

#include <string.h>
#include <esp_err.h>

#include <esp_log.h>
#define LOG_TAG "TCP_RX"

#define BIGGEST_RECV_MESSAGE_SIZE (sizeof(energy_price_day_msg_t))

static app_msg_dispatcher_cb_t app_msg_dispatcher_cb;
static uint8_t msg_buffer[BIGGEST_RECV_MESSAGE_SIZE];

static int parse_message(const uint8_t *data, uint32_t len, msg_type_t *out_type)
{
  assert(data && out_type);
  assert(len <= sizeof(msg_buffer));

  msg_type_t type = parse_msg_type(data, len);
  *out_type = type;

  if (type == MSGTYPE_INVALID) {
    ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE));
    return ESP_ERR_INVALID_SIZE;
  }

  switch (type) {
  case MSGTYPE_CONNECTION_STATUS: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_CONNECTION_STATUS), len);
    connection_status_msg_t msg = parse_connection_status_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_CONNECTION_STATUS));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(connection_status_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_REBOOT: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_REBOOT), len);
    memcpy(msg_buffer, &type, sizeof(msg_type_t));
    return ESP_OK;
  };

  case MSGTYPE_FACTORY_RESET: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_FACTORY_RESET), len);
    memcpy(msg_buffer, &type, sizeof(msg_type_t));
    return ESP_OK;
  };

  case MSGTYPE_IDENTIFY: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_IDENTIFY), len);
    identify_msg_t msg = parse_identify_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_IDENTIFY));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(identify_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_UPDATE_START: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_UPDATE_START), len);
    update_start_msg_t msg = parse_update_start_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_UPDATE_START));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(update_start_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_UPDATE_INSTALL: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_UPDATE_INSTALL), len);
    update_install_msg_t msg = parse_update_install_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_UPDATE_INSTALL));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(update_install_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_WIRELESS_ENABLE: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_WIRELESS_ENABLE), len);
    wireless_enable_msg_t msg = parse_wireless_enable_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_WIRELESS_ENABLE));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(wireless_enable_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_WIRELESS_SET_CREDENTIALS: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_WIRELESS_CREDENTIALS), len);
    wireless_credentials_msg_t msg = parse_wireless_credentials_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_WIRELESS_CREDENTIALS));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(wireless_credentials_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_WIRELESS_SET_TX_POWER: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_WIRELESS_SET_TX_POWER), len);
    wireless_tx_power_msg_t msg = parse_wireless_tx_power_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_WIRELESS_SET_TX_POWER));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(wireless_tx_power_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_WIRELESS_SET_CHANNEL: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_WIRELESS_SET_CHANNEL), len);
    wireless_channel_msg_t msg = parse_wireless_channel_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_WIRELESS_SET_CHANNEL));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(wireless_channel_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENERGYPRICE_DAY: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENERGYPRICE_DAY), len);
    energy_price_day_msg_t msg = parse_energy_price_day_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENERGYPRICE_DAY));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(energy_price_day_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENERGYPRICE_SET_PRICE_THRESHOLD: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_THRESHOLD), len);
    energy_price_threshold_msg_t msg = parse_energy_price_threshold_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_THRESHOLD));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(energy_price_threshold_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENERGYPRICE_SET_CHEAP_PERIOD: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_CHEAP_PERIOD), len);
    energy_price_cheap_period_msg_t msg = parse_energy_price_cheap_period_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_CHEAP_PERIOD));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(energy_price_cheap_period_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENERGYPRICE_SET_TARIFF: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_TARIFF), len);
    energy_price_tariff_msg_t msg = parse_energy_price_tariff_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENERGYPRICE_SET_TARIFF));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(energy_price_tariff_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_MODE: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_MODE), len);
    relay_mode_msg_t msg = parse_relay_mode_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_MODE));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_mode_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_STATE: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_STATE), len);
    relay_state_msg_t msg = parse_relay_state_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_STATE));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_state_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_DEFAULT_STATE: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_DEFAULT_STATE), len);
    relay_default_state_msg_t msg = parse_relay_default_state_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_DEFAULT_STATE));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_default_state_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_PWM: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_PWM), len);
    relay_pwm_msg_t msg = parse_relay_pwm_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_PWM));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_pwm_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_INVERTED_OUTPUT: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_INVERTED_OUTPUT), len);
    relay_inverted_output_msg_t msg = parse_relay_set_inverted_output_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_INVERTED_OUTPUT));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_inverted_output_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_BUTTON_CONFIG: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_BUTTON_CONFIG), len);
    relay_button_config_msg_t msg = parse_relay_button_config_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_BUTTON_CONFIG));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_button_config_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_SCHEDULE: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_SCHEDULE), len);
    relay_schedule_msg_t msg = parse_relay_schedule_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_SCHEDULE));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_schedule_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_RELAY_SET_ONE_SHOT: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_RELAY_SET_ONE_SHOT), len);
    relay_one_shot_msg_t msg = parse_relay_one_shot_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_RELAY_SET_ONE_SHOT));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(relay_one_shot_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENVIRONMENT_SET_SUNNY_TIME: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENVIRONMENT_SET_SUNNY_TIME), len);
    environment_sunny_time_msg_t msg = parse_environment_sunny_time_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENVIRONMENT_SET_SUNNY_TIME));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(environment_sunny_time_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENVIRONMENT_CLOUD_COVER: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENVIRONMENT_CLOUD_COVER), len);
    environment_cloud_cover_msg_t msg = parse_environment_cloud_cover_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENVIRONMENT_CLOUD_COVER));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(environment_cloud_cover_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_ENVIRONMENT_SET_POSITION: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_ENVIRONMENT_SET_POSITION), len);
    environment_position_msg_t msg = parse_environment_position_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_ENVIRONMENT_SET_POSITION));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(environment_position_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_METER_SET_CURRENT_LIMIT: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_METER_SET_CURRENT_LIMIT), len);
    meter_current_limit_msg_t msg = parse_meter_current_limit_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_METER_SET_CURRENT_LIMIT));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(meter_current_limit_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_METER_CLEAR_ALARMS: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_METER_CLEAR_ALARMS), len);
    meter_alarms_msg_t msg = parse_meter_clear_alarms_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_METER_CLEAR_ALARMS));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(meter_alarms_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_DIAG_COREDUMP_CONFIRMED: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_DIAG_COREDUMP_CONFIRMED), len);
    memcpy(msg_buffer, &type, sizeof(msg_type_t));
    return ESP_OK;
  };

  case MSGTYPE_STATUS: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_STATUS), len);
    status_msg_t msg = parse_status_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_STATUS));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(status_msg_t));
    return ESP_OK;
  };

  case MSGTYPE_GET: {
    ESP_LOGI(LOG_TAG, "%s: Len=%ld", __STRINGIFY(MSGTYPE_GET), len);
    get_msg_t msg = parse_get_msg(data, len);

    if (msg.type == MSGTYPE_INVALID) {
      ESP_LOGW(LOG_TAG, "Can't parse %s", __STRINGIFY(MSGTYPE_GET));
      return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(msg_buffer, &msg, sizeof(get_msg_t));
    return ESP_OK;
  };

  default:
    ESP_LOGW(LOG_TAG, "Received unknown message=0x%04x", type);
    break;
  }

  return ESP_ERR_NOT_SUPPORTED;
}

int protocol_receiver_dispatcher(const uint8_t *data, uint32_t len)
{
  assert(sizeof(msg_buffer) >= len);

  msg_type_t msg_type = MSGTYPE_INVALID;

  int ret = parse_message(data, len, &msg_type);
  if (ret) {
    ESP_LOGE(LOG_TAG, "Parse data msg_type=%d len=%ld err: %s", msg_type, len, esp_err_to_name(ret));
    return ret;
  }

  if (app_msg_dispatcher_cb) {
    ret = app_msg_dispatcher_cb(msg_type, msg_buffer);
  }

  memset(msg_buffer, 0x00, sizeof(msg_buffer));

  if (ret) {
    /* Respond to server with an error message */
    ESP_LOGE(LOG_TAG, "Dispatch msg_type=0x%04X:%i", msg_type, ret);
  }

  // successful msg for get request are sent in the app_msg_dispatcher_cb
  if (msg_type != MSGTYPE_GET || ret != ESP_OK) {
    protocol_send_status(msg_type, ret);
  }

  return ESP_OK;
}

void protocol_register_rx_handler(app_msg_dispatcher_cb_t handler)
{
  app_msg_dispatcher_cb = handler;
}
