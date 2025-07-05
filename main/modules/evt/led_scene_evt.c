/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/ble_core.h"
#include "em/button.h"
#include "em/http_ota.h"
#include "em/led_scene.h"
#include "em/messages.h"
#include "em/tcp_client.h"
#include "em/wifi.h"

#include <esp_event.h>
#include <esp_wifi.h>
#include <common.h>
#include <driver/gpio.h>

#include <esp_log.h>
#define LOG_TAG "led_evt"

#define SCENE_BG(c, br) .type = SCENE_CONST_BG, .color = (c), .brightness = (br)

#define SCENE_ONCE(c, br, on, off, rep) .type = SCENE_BLINK_ONCE, .color = (c), .brightness = (br), .blink = {.t_on = on, .t_off = (off), .repeats = (rep)}

#define SCENE_PERIODIC(c, br, on, off, rep, p)                                                                                                                 \
  .type = SCENE_BLINK_PERIODIC, .color = (c), .brightness = (br), .blink = {.t_on = on, .t_off = (off), .repeats = (rep), .period = (p)}

#define SCENE_CLEAR_ALL()                                                                                                                                      \
  .type = SCENE_CLEAR_ALL, .color = 0, .brightness = 0,                                                                                                        \
  .blink = {                                                                                                                                                   \
    0,                                                                                                                                                         \
  }

#define SCENE_CLEAR_PERIODIC()                                                                                                                                 \
  .type = SCENE_CLEAR_PERIODIC, .color = 0, .brightness = 0,                                                                                                   \
  .blink = {                                                                                                                                                   \
    0,                                                                                                                                                         \
  }

#define SCENE_CLEAR_ONCE()                                                                                                                                     \
  .type = SCENE_CLEAR_ONCE, .color = 0, .brightness = 0,                                                                                                       \
  .blink = {                                                                                                                                                   \
    0,                                                                                                                                                         \
  }

#define WIFI_STA_CONNECTED_1_SCENE \
(scene_t) { SCENE_CLEAR_ALL() }

#define WIFI_STA_CONNECTED_2_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_PURPLE, 100U, 1000U, 200U, 1) }

#define WIFI_STA_DISCONNECTED \
(scene_t) { SCENE_PERIODIC(RGB_LED_RED, 100U, 1000U, 500U, 2, 10000) }

#define PROVISIONED_SCENE \
(scene_t) { SCENE_CLEAR_ONCE() }

#define WIFI_STA_CONNECTING_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_PURPLE, 50U, 100U, 500U, UINT16_MAX) }

#define BLE_ADV_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_BLUE, 100U, 300U, 400U, 800) }

#define NOT_PROVISIONED_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_BLUE, 100U, 300U, 400U, UINT16_MAX) }

#define BLE_CONNECTED_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_BLUE, 70U, 300U, 400U, 800) }

#define BLE_DEINIT_SCENE \
(scene_t) { SCENE_CLEAR_ONCE() }

#define TCP_ONLINE_1_SCENE \
(scene_t) { SCENE_CLEAR_ALL() }

#define TCP_ONLINE_2_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_LAVENDER, 100U, 1000U, 200U, 1) }

#define TCP_CONN_ERROR_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_RED, 100U, 200U, 200U, 3) }

#define TCP_ERROR_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_RED, 100U, 200U, 200U, 1) }

#define OTA_FAIL_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_RED, 100U, 200U, 200U, 2) }

#define OTA_PROGRESS_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_LAVENDER, 50U, 100U, 200U, 2) }

#define OTA_FINISHED_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_LAVENDER, 50U, 100U, 200U, 1) }

#define SHORT_3_VERY_LONG_1_SCENE \
(scene_t) { SCENE_ONCE(RGB_LED_PURPLE, 70U, 200U, 200U, 4) }

static scenes_list_t scenes_list[] = {
  {.event_base = "EM_BLE_CORE_EVENT", .event_id = EM_BLE_CORE_EVENT_CONNECTED, .scene = BLE_CONNECTED_SCENE},
  {.event_base = "EM_BLE_CORE_EVENT", .event_id = EM_BLE_CORE_EVENT_DEINIT, .scene = BLE_DEINIT_SCENE},
  {.event_base = "EM_TCP_CLIENT_EVENT", .event_id = EM_TCP_CLIENT_EVENT_ONLINE, .scene = TCP_ONLINE_1_SCENE},
  {.event_base = "EM_TCP_CLIENT_EVENT", .event_id = EM_TCP_CLIENT_EVENT_ONLINE, .scene = TCP_ONLINE_2_SCENE},
  {.event_base = "EM_TCP_CLIENT_EVENT", .event_id = EM_TCP_CLIENT_EVENT_CONN_ERROR, .scene = TCP_CONN_ERROR_SCENE},
  {.event_base = "EM_TCP_CLIENT_EVENT", .event_id = EM_TCP_CLIENT_EVENT_RX_ERROR, .scene = TCP_ERROR_SCENE},
  {.event_base = "EM_TCP_CLIENT_EVENT", .event_id = EM_TCP_CLIENT_EVENT_TX_ERROR, .scene = TCP_ERROR_SCENE},
  {.event_base = "EM_WIFI_EVENT", .event_id = EM_WIFI_EVENT_NOT_PROVISIONED, .scene = NOT_PROVISIONED_SCENE},
  {.event_base = "EM_WIFI_EVENT", .event_id = EM_WIFI_EVENT_PROVISIONED, .scene = PROVISIONED_SCENE},
  {.event_base = "EM_WIFI_EVENT", .event_id = EM_WIFI_EVENT_PROVISIONED, .scene = WIFI_STA_CONNECTING_SCENE},
  // {.event_base = "EM_WIFI_EVENT", .event_id = EM_WIFI_EVENT_STA_DISCONNECTED, .scene = WIFI_STA_DISCONNECTED},
  {.event_base = "WIFI_EVENT", .event_id = WIFI_EVENT_STA_CONNECTED, .scene = WIFI_STA_CONNECTED_1_SCENE},
  {.event_base = "WIFI_EVENT", .event_id = WIFI_EVENT_STA_CONNECTED, .scene = WIFI_STA_CONNECTED_2_SCENE},
  {.event_base = "EM_HTTP_OTA_EVENT", .event_id = EM_HTTP_OTA_EVENT_ABORT, .scene = OTA_FAIL_SCENE},
  {.event_base = "EM_HTTP_OTA_EVENT", .event_id = EM_HTTP_OTA_EVENT_TRANSFER, .scene = OTA_PROGRESS_SCENE},
  {.event_base = "EM_HTTP_OTA_EVENT", .event_id = EM_HTTP_OTA_EVENT_VERIFIED, .scene = OTA_FINISHED_SCENE},
  {.event_base = "EM_BUTTON_EVENT", .event_id = EM_BUTTON_3X_CLICK_EVENT, .scene = BLE_ADV_SCENE},
  {.event_base = "EM_BUTTON_EVENT", .event_id = EM_BUTTON_3X_SHORT_1X_VERY_LONG_CLICK_EVENT, .scene = SHORT_3_VERY_LONG_1_SCENE},
};
// clang-format on

void led_evt_init(void)
{
  ESP_ERROR_CHECK(led_scene_init(scenes_list, sizeof(scenes_list) / sizeof(scenes_list_t)));
}

esp_err_t led_evt_identify_msg_handler(void *data)
{
  const identify_msg_t *const msg = (identify_msg_t *)data;
  // clang-format off
  scene_t scene = {
    .type = msg->mode + 1,
    .color = RGB_LED_PINK,
    .brightness = 100U,
    .blink =
      {
        .t_on = 150UL,
        .t_off = 150UL,
        .repeats = msg->count,
        .period = 5000U,
      },
  };
  // clang-format off
  led_scene_trigger(&scene);

  return ESP_OK;
}
