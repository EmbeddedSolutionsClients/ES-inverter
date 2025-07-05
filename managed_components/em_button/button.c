/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/button.h"

#include <button_gpio.h>
#include <iot_button.h>
#include <sys/time.h>

#include <esp_log.h>
#define LOG_TAG "em_button"

ESP_EVENT_DEFINE_BASE(EM_BUTTON_EVENT);

static void button_handler(void *button_handle, void *usr_data)
{
  static button_event_t evt_previous = BUTTON_NONE_PRESS;
  button_event_t evt = iot_button_get_event(button_handle);

  uint32_t ts = esp_log_timestamp();
  static uint32_t ts_pressed = 0;
  static uint32_t ts_break = 0;

  if (evt == BUTTON_PRESS_DOWN) {
    ts_pressed = ts;

    if ((evt_previous == BUTTON_MULTIPLE_CLICK) && ((ts_pressed - ts_break) > CONFIG_EM_BUTTON_BREAK_TIME_MAX_MS)) {
      ESP_LOGW(LOG_TAG, "3xClick -- Break -- VeryLong sequence failed, Break too long");
      evt_previous = BUTTON_NONE_PRESS;
    }

    /* Return to do not overwrite evt_previous, save the timestamp only */
    return;
  }

  ESP_LOGI(LOG_TAG, "Button event: %d", evt);

  if (evt == BUTTON_SINGLE_CLICK) {
    ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_1X_CLICK_EVENT, NULL, 0, 0));
  } else if (evt == BUTTON_DOUBLE_CLICK) {
    ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_2X_CLICK_EVENT, NULL, 0, 0));
  } else if (evt == BUTTON_MULTIPLE_CLICK) {
    ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_3X_CLICK_EVENT, NULL, 0, 0));
    /* Save current timestamp to calculate the Break time */
    ts_break = ts;
  } else if (evt_previous == BUTTON_MULTIPLE_CLICK && evt == BUTTON_LONG_PRESS_START) {
    ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_3X_SHORT_1X_LONG_CLICK_EVENT, NULL, 0, 0));
    /* Do not overwrite evt_previous to keep tracking 3xClick -- Break -- VeryLong Press sequence */
    return;
  } else if (evt_previous == BUTTON_MULTIPLE_CLICK && evt == BUTTON_LONG_PRESS_HOLD) {
    if ((ts_pressed + CONFIG_EM_BUTTON_VERY_LONG_PRESS_TIME_MS) < ts) {
      ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_3X_SHORT_1X_VERY_LONG_CLICK_EVENT, NULL, 0, 0));
    } else {
      /* CONFIG_EM_BUTTON_VERY_LONG_PRESS_TIME_MS has not elapsed yet  */
      /* Do not overwrite evt_previous to keep tracking 3xClick -- Break -- VeryLong Press sequence */
      return;
    }
  } else if (evt_previous != BUTTON_MULTIPLE_CLICK && evt == BUTTON_LONG_PRESS_START) {
    ESP_ERROR_CHECK(esp_event_post(EM_BUTTON_EVENT, EM_BUTTON_1X_LONG_PRESS_EVENT, NULL, 0, 0));
  }

  evt_previous = evt;
}

void button_init(bool power_safe)
{
  const button_config_t button_cfg = {
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
  };

  button_gpio_config_t gpio_cfg = {
    .gpio_num = CONFIG_EM_BUTTON_PIN,
    .active_level = CONFIG_EM_BUTTON_ACTIVE_LEVEL,
    .enable_power_save = power_safe,
  };

  button_handle_t button;
  ESP_ERROR_CHECK(iot_button_new_gpio_device(&button_cfg, &gpio_cfg, &button));
  assert(button);

  ESP_ERROR_CHECK(iot_button_register_cb(button, BUTTON_PRESS_DOWN, NULL, button_handler, NULL));
  ESP_ERROR_CHECK(iot_button_register_cb(button, BUTTON_SINGLE_CLICK, NULL, button_handler, NULL));
  ESP_ERROR_CHECK(iot_button_register_cb(button, BUTTON_DOUBLE_CLICK, NULL, button_handler, NULL));
  ESP_ERROR_CHECK(iot_button_register_cb(button, BUTTON_LONG_PRESS_START, NULL, button_handler, NULL));
  ESP_ERROR_CHECK(iot_button_register_cb(button, BUTTON_LONG_PRESS_HOLD, NULL, button_handler, NULL));

  button_event_args_t args = {
    .multiple_clicks.clicks = CONFIG_EM_BUTTON_MULTIPLE_CLICKS_NUM,
  };

  iot_button_register_cb(button, BUTTON_MULTIPLE_CLICK, &args, button_handler, NULL);

  ESP_LOGI(LOG_TAG, "Button initialized");
}
