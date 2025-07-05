/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <esp_event.h>

/**
 * @brief Actions to be performed as a result of button events.
 *
 */
typedef enum {
  EM_BUTTON_NOPRESS = 0,
  EM_BUTTON_1X_CLICK_EVENT,
  EM_BUTTON_1X_LONG_PRESS_EVENT,
  EM_BUTTON_2X_CLICK_EVENT,
  EM_BUTTON_3X_CLICK_EVENT,
  EM_BUTTON_3X_SHORT_1X_LONG_CLICK_EVENT,
  EM_BUTTON_3X_SHORT_1X_VERY_LONG_CLICK_EVENT,
} button_event_action_t;

ESP_EVENT_DECLARE_BASE(EM_BUTTON_EVENT);

void button_init(bool power_safe);

#endif /* BUTTON_H_ */
