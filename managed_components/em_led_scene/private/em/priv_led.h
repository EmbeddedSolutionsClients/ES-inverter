/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef LED_PRIV_H_
#define LED_PRIV_H_

#include "em/led_scene.h"
#include <stdint.h>

#define RED_COLOR_MASK(color)   ((color >> 16) & 0xFF)
#define GREEN_COLOR_MASK(color) ((color >> 8) & 0xFF)
#define BLUE_COLOR_MASK(color)  (color & 0xFF)

/*
typedef struct {
  rgb_color_t color;
  uint32_t t_on;
  uint32_t t_off;
  uint8_t repeats;
  uint8_t period; // repeat blink patter every `period` seconds
  on_blink_end_cb_t app_cb;
} blink_data_t;*/

/* For fading, rainbows and other fancy stuff */
// typedef void (*on_blink_end_cb_t)(led_type_t);

typedef struct {
  int (*init)(void);
  int (*set_brightness)(uint8_t brightness);
  void (*set_color)(rgb_color_t color);
  void (*on)(void);
  void (*off)(void);
  // int (*blink)(uint32_t t_on_ms, uint32_t t_off_ms, int32_t repeats, on_blink_end_cb_t app_cb);
  // int (*blink_color)(rgb_color_t color, uint32_t t_on_ms, uint32_t t_off_ms, int16_t repeats, on_blink_end_cb_t
  // app_cb);
} led_interface_t;

#endif /* LED_PRIV_H_ */
