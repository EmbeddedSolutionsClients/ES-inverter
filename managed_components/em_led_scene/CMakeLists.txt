# Copyright (C) 2024 EmbeddedSolutions.pl

set(srcs "scene.c")
set(requires em_scheduler esp_event esp_driver_gpio esp_driver_ledc)

if(CONFIG_EM_LED_PWM_RGB)
  list(APPEND srcs "pwm_rgb.c")
endif()

if(CONFIG_EM_LED_WS2812)
  list(APPEND srcs "ws2812_rgb.c")
  list(APPEND requires espressif__led_strip)
endif()

idf_component_register(
  SRCS
    "${srcs}"
  INCLUDE_DIRS
    "include"
    "private"
  PRIV_REQUIRES
    ${requires}
)
