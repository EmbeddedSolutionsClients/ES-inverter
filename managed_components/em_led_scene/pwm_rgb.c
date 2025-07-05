/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/led_scene.h"
#include "em/priv_led.h"
#include "em/scheduler.h"

#include <stdint.h>
#include <string.h>
#include <esp_macros.h>
#include <driver/ledc.h>

#define R_LED_CHANNEL (LEDC_CHANNEL_0)
#define B_LED_CHANNEL (LEDC_CHANNEL_1)
#define G_LED_CHANNEL (LEDC_CHANNEL_2)

#define PWM_FREQUENCY_HZ (100U)
#define PWM_RESOLUTION   (255U)

static_assert(GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_EM_LED_R_PIN));

#if CONFIG_EM_LED_PWM_CHANNELS > 1
static_assert(GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_EM_LED_B_PIN));
#endif

#if CONFIG_EM_LED_PWM_CHANNELS > 2
static_assert(GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_EM_LED_G_PIN));
#endif

static uint8_t brightness = 100; // Default brightness is 100%
static rgb_color_t color = RGB_LEDS_OFF;

esp_err_t init(void)
{
  const ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    /* Use 8 bit resolution to be in compliance with 8bit resolution of each color component. Simplify the math */
    .duty_resolution = LEDC_TIMER_8_BIT,
    .freq_hz = PWM_FREQUENCY_HZ,
    .clk_cfg = LEDC_AUTO_CLK,
  };

  esp_err_t ret = ledc_timer_config(&ledc_timer);
  if (ret) {
    return ret;
  }

#ifdef CONFIG_EM_LED_PWM_RGB_INVERT
  const bool invert = true;
#else
  const bool invert = false;
#endif

  const ledc_channel_config_t ledc_channel[CONFIG_EM_LED_PWM_CHANNELS] = {
    {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = R_LED_CHANNEL,
      .timer_sel = LEDC_TIMER_0,
      .gpio_num = CONFIG_EM_LED_R_PIN,
      .hpoint = 0,
      .duty = 0,
      .flags.output_invert = invert,
    },
#if CONFIG_EM_LED_PWM_CHANNELS > 1
    {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = B_LED_CHANNEL,
      .timer_sel = LEDC_TIMER_0,
      .gpio_num = CONFIG_EM_LED_B_PIN,
      .hpoint = 0,
      .duty = 0,
      .flags.output_invert = invert,
    },
#endif
#if CONFIG_EM_LED_PWM_CHANNELS > 2
    {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = G_LED_CHANNEL,
      .timer_sel = LEDC_TIMER_0,
      .gpio_num = CONFIG_EM_LED_G_PIN,
      .hpoint = 0,
      .duty = 0,
      .flags.output_invert = invert,
    },
#endif
  };

  for (uint32_t i = 0; i < CONFIG_EM_LED_PWM_CHANNELS; i++) {
    const esp_err_t ret = ledc_channel_config(&ledc_channel[i]);
    if (ret) {
      return ret;
    }
  }

  return ESP_OK;
}

static void rgb_on(void)
{
  ledc_set_duty(LEDC_LOW_SPEED_MODE, R_LED_CHANNEL, (RED_COLOR_MASK(color) * brightness) / 100U);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, R_LED_CHANNEL);
#if CONFIG_EM_LED_PWM_CHANNELS > 1
  ledc_set_duty(LEDC_LOW_SPEED_MODE, B_LED_CHANNEL, (BLUE_COLOR_MASK(color) * brightness) / 100U);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, B_LED_CHANNEL);
#endif
#if CONFIG_EM_LED_PWM_CHANNELS > 2
  ledc_set_duty(LEDC_LOW_SPEED_MODE, G_LED_CHANNEL, (GREEN_COLOR_MASK(color) * brightness) / 100U);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, G_LED_CHANNEL);
#endif
}

static void rgb_off(void)
{
  ledc_set_duty(LEDC_LOW_SPEED_MODE, R_LED_CHANNEL, 0UL);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, R_LED_CHANNEL);
#if CONFIG_EM_LED_PWM_CHANNELS > 1
  ledc_set_duty(LEDC_LOW_SPEED_MODE, B_LED_CHANNEL, 0UL);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, B_LED_CHANNEL);
#endif
#if CONFIG_EM_LED_PWM_CHANNELS > 2
  ledc_set_duty(LEDC_LOW_SPEED_MODE, G_LED_CHANNEL, 0UL);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, G_LED_CHANNEL);
#endif
}

static int set_brightness(uint8_t req_brightness)
{
  if (req_brightness == 0 || req_brightness > 100) {
    return ESP_ERR_INVALID_ARG;
  }

  brightness = req_brightness / CONFIG_EM_LED_BRIGHTNESS_DIVIDER;

  return ESP_OK;
}

static void set_color(rgb_color_t rq_color)
{
  color = rq_color;
}

const led_interface_t led_if = {
  .init = init, .set_brightness = set_brightness, .on = rgb_on, .off = rgb_off, .set_color = set_color};
