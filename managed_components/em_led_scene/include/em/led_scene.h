/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef LED_SCENE_H_
#define LED_SCENE_H_

#include <stdint.h>
#include <esp_event.h>

#define BLINK_INFINITE (-1)

/**
 * @brief Enumeration of RGB color values.
 *
 * Defines the RGB color values that can be used for the LED.
 */
typedef enum {
  RGB_LEDS_OFF = 0x000000,
  RGB_LEDS_ON = 0xFFFFFF,
  RGB_LED_RED = 0xFF0000,
  RGB_LED_GREEN = 0x00FF00,
  RGB_LED_BLUE = 0x0000FF,
  RGB_LED_YELLOW = 0xFFFF00,
  RGB_LED_CYAN = 0x00FFFF,
  RGB_LED_MAGENTA = 0xFF00FF,
  RGB_LED_WHITE = 0xFFFFFF,
  RGB_LED_ORANGE = 0xFFA500,
  RGB_LED_PURPLE = 0x800080,
  RGB_LED_PINK = 0xFFC0CB,
  RGB_LED_LAVENDER = 0x2000FF,
} rgb_color_t;

typedef enum {
  SCENE_UNSET,
  SCENE_CONST_BG,
  SCENE_BLINK_ONCE,
  SCENE_BLINK_PERIODIC,
  SCENE_CLEAR_ONCE,
  SCENE_CLEAR_PERIODIC,
  SCENE_CLEAR_ALL,
  SCENE_OFF,
} scene_type_t;

typedef struct {
  scene_type_t type;
  rgb_color_t color;
  struct {
    uint16_t t_on;    // time on in ms
    uint16_t t_off;   // time off in ms
    uint16_t repeats; // blinks count
    uint16_t period;  // in ms for SCENE_BLINK_PERIODIC only
  } blink;
  uint8_t brightness; // 0-100%
} scene_t;

typedef struct {
  esp_event_base_t event_base;
  int32_t event_id;
  scene_t scene;
} scenes_list_t;

int led_scene_init(scenes_list_t *list, size_t list_size);
void led_scene_trigger(const scene_t *scene);

#endif /* LED_SCENE_H_ */
