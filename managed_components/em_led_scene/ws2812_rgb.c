/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/led_scene.h"
#include "em/priv_led.h"
#include "em/scheduler.h"

#include <string.h>
#include <driver/gpio.h>
#include <led_strip.h>

static led_strip_handle_t led_strip;
static uint8_t brightness = 100;
static rgb_color_t color = RGB_LEDS_OFF;

static esp_err_t init(void)
{
  esp_err_t ret = gpio_reset_pin(CONFIG_EM_LED_WS2812_PIN);
  if (ret) {
    return ret;
  }

  ret = gpio_set_direction(CONFIG_EM_LED_WS2812_PIN, GPIO_MODE_OUTPUT);
  if (ret) {
    return ret;
  }

  led_strip_config_t strip_config = {
    .strip_gpio_num = CONFIG_EM_LED_WS2812_PIN,
    /* One LED on board */
    .max_leds = 1,
  };

  led_strip_rmt_config_t rmt_config = {
    /* 10MHz */
    .resolution_hz = 10 * 1000 * 1000,
    .flags.with_dma = false,
  };

  ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
  if (ret) {
    return ret;
  }

  return led_strip_clear(led_strip);
}

static int set_brightness(uint8_t req_brightness)
{
  if (req_brightness == 0 || req_brightness > 100) {
    return ESP_ERR_INVALID_ARG;
  }

  brightness = req_brightness / CONFIG_EM_LED_BRIGHTNESS_DIVIDER;
  return ESP_OK;
}

static void rgb_on(void)
{
  if (color == RGB_LEDS_OFF) {
    led_strip_clear(led_strip);
  } else {
    led_strip_set_pixel(led_strip, 0, (RED_COLOR_MASK(color) * brightness) / 100U,
                        (GREEN_COLOR_MASK(color) * brightness) / 100U, (BLUE_COLOR_MASK(color) * brightness) / 100U);
  }

  led_strip_refresh(led_strip);
}

static void rgb_off(void)
{
  (void)led_strip_clear(led_strip);
  (void)led_strip_refresh(led_strip);
}

static void set_color(rgb_color_t rq_color)
{
  color = rq_color;
}

const led_interface_t led_if = {
  .init = init, .set_brightness = set_brightness, .on = rgb_on, .off = rgb_off, .set_color = set_color,
  //.blink = ws2812_rgb_blink,
  // .blink_color = ws2812_rgb_blink_color,
};
