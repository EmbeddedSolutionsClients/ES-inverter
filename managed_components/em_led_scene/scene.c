/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/led_scene.h"
#include "em/priv_led.h"
#include "em/scheduler.h"

#include <esp_log.h>
#define LOG_TAG "EM_SCENE"

#define SCENE_BLINK_SEPARATION_MS (200U)

extern const led_interface_t led_if;
static scenes_list_t *scene_list;
static size_t scene_list_size = 0;

typedef struct {
  scene_t scene;
  uint32_t remaining_toggles;
  int32_t remaining_time; // in ms
} active_scene_t;

typedef struct {
  bool rq_led_on;
  rgb_color_t bg_color;
  uint32_t bg_brightness;
  active_scene_t once;
  active_scene_t periodic;
} active_scenes_t;

static active_scenes_t active_scenes = {
  .bg_color = RGB_LEDS_OFF,
  .bg_brightness = 0,
  .once = {.remaining_toggles = 0, .remaining_time = 0, .scene = {.type = SCENE_OFF}},
  .periodic = {.remaining_toggles = 0, .remaining_time = 0, .scene = {.type = SCENE_OFF}},
};

static void on_blink_once(const scene_t *scene);
static void on_blink_periodic(const scene_t *scene);
static void restore_bg_color(void);
static void scene_change_handler(const scene_t *scene);

static void change_led_state(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(user_ctx);
  ESP_UNUSED(param);

  uint32_t next_change_time = UINT32_MAX;

  if (active_scenes.periodic.remaining_time > 0) {
    active_scenes.periodic.remaining_time -= param;
  }

  // ESP_LOGI(LOG_TAG, "change_led_state toggles: %ld/%ld rem: %ld", active_scenes.once.remaining_toggles,
  //          active_scenes.periodic.remaining_toggles, active_scenes.periodic.remaining_time);

  if (active_scenes.once.remaining_toggles > 0) {
    led_if.set_color(active_scenes.once.scene.color);
    led_if.set_brightness(active_scenes.once.scene.brightness);

    if (active_scenes.rq_led_on) {
      next_change_time = active_scenes.once.scene.blink.t_on;
    } else {
      next_change_time = active_scenes.once.scene.blink.t_off;
    }

    --active_scenes.once.remaining_toggles;
  } else if (active_scenes.periodic.remaining_toggles > 0) {
    if (active_scenes.periodic.remaining_time > 0) {
      next_change_time = active_scenes.periodic.remaining_time;
      scheduler_set_callback(change_led_state, next_change_time, SCH_CTX_NONE, next_change_time);
      return;
    } else {
      led_if.set_color(active_scenes.periodic.scene.color);
      led_if.set_brightness(active_scenes.periodic.scene.brightness);

      if (active_scenes.rq_led_on) {
        next_change_time = active_scenes.periodic.scene.blink.t_on;
      } else {
        next_change_time = active_scenes.periodic.scene.blink.t_off;
      }

      --active_scenes.periodic.remaining_toggles;

      if (active_scenes.periodic.remaining_toggles == 0) {
        active_scenes.periodic.remaining_time = active_scenes.periodic.scene.blink.period + next_change_time;
        active_scenes.periodic.remaining_toggles = 2 * active_scenes.periodic.scene.blink.repeats;
      }
    }
  } else {
    restore_bg_color(); // No active scene, restore background color
    return;
  }

  if (next_change_time == UINT32_MAX) {
    // No more changes scheduled, restore background color
    restore_bg_color();
    return;
  }

  if (!active_scenes.rq_led_on) {
    led_if.off();
  } else {
    led_if.on();
  }

  active_scenes.rq_led_on = !active_scenes.rq_led_on;
  scheduler_set_callback(change_led_state, next_change_time, SCH_CTX_NONE, next_change_time);
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(event_data);

  // move to managed component
  for (uint32_t i = 0; i < scene_list_size; i++) {
    if (scene_list[i].event_base == event_base && scene_list[i].event_id == event_id) {
      scene_change_handler(&scene_list[i].scene);
    }
  }
}

int led_scene_init(scenes_list_t *list, size_t list_size)
{
  int ret = led_if.init();

  if (ret == 0) {
    ESP_ERROR_CHECK(
      esp_event_handler_instance_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, event_handler, NULL, NULL));
    scene_list = list;
    scene_list_size = list_size;
  }

  return ret;
}

void led_scene_trigger(const scene_t *scene)
{
  scene_change_handler(scene);
}

static void scene_change_handler(const scene_t *scene)
{
  switch (scene->type) {
  case SCENE_CONST_BG:
    active_scenes.bg_color = scene->color;
    active_scenes.bg_brightness = scene->brightness;
    if (active_scenes.periodic.remaining_toggles == 0 && active_scenes.once.remaining_toggles == 0) {
      restore_bg_color();
    }
    break;

  case SCENE_BLINK_ONCE:
    on_blink_once(scene);
    break;

  case SCENE_BLINK_PERIODIC:
    on_blink_periodic(scene);
    break;

  case SCENE_CLEAR_ONCE:
    active_scenes.once.remaining_toggles = 0;
    restore_bg_color();
    break;

  case SCENE_CLEAR_PERIODIC:
    active_scenes.periodic.scene.blink.period = 0;
    active_scenes.periodic.remaining_toggles = 0;
    restore_bg_color();
    break;

  case SCENE_CLEAR_ALL:
    active_scenes.periodic.scene.blink.period = 0;
    active_scenes.periodic.remaining_toggles = 0;
    active_scenes.once.remaining_toggles = 0;
    restore_bg_color();
    break;

  case SCENE_OFF:
    scheduler_cancel_callback(change_led_state);
    active_scenes.bg_color = RGB_LEDS_OFF;
    active_scenes.bg_brightness = 0;
    active_scenes.periodic.remaining_toggles = 0;
    active_scenes.once.remaining_toggles = 0;
    led_if.off();
    break;

  default:
    break;
  }
}

static void on_blink_once(const scene_t *scene)
{
  // ESP_LOGI(LOG_TAG, "Blink once: col=%06X on=%u off=%u reps=%d", scene->color, scene->blink.t_on, scene->blink.t_off,
  //        scene->blink.repeats);

  assert(scene->blink.t_on > 0 && scene->blink.t_off > 0 && scene->blink.repeats > 0);
  assert(scene->color != RGB_LEDS_OFF);

  scheduler_cancel_callback(change_led_state);

  active_scenes.once.scene = *scene;
  active_scenes.once.remaining_toggles = 2 * scene->blink.repeats;
  active_scenes.rq_led_on = true;
  led_if.off(); // set led off for constant time to make blinking visible
  scheduler_set_callback(change_led_state, SCENE_BLINK_SEPARATION_MS, SCH_CTX_NONE, SCENE_BLINK_SEPARATION_MS);
}

static void on_blink_periodic(const scene_t *scene)
{
  // ESP_LOGI(LOG_TAG, "Blink period=%d: col=%06X on=%u off=%u reps=%d", scene->color, scene->blink.period,
  //          scene->blink.t_on, scene->blink.t_off, scene->blink.repeats);

  assert(scene->blink.t_on > 0 && scene->blink.t_off > 0 && scene->blink.repeats > 0);
  assert(scene->blink.repeats * (uint32_t)(scene->blink.t_on + scene->blink.t_off) < scene->blink.period);
  assert(scene->color != RGB_LEDS_OFF);

  scheduler_cancel_callback(change_led_state);

  active_scenes.periodic.remaining_toggles = 2 * scene->blink.repeats;
  active_scenes.periodic.remaining_time = SCENE_BLINK_SEPARATION_MS;
  active_scenes.periodic.scene = *scene;
  active_scenes.rq_led_on = true;
  led_if.off(); // set led off for constant time to make blinking visible
  scheduler_set_callback(change_led_state, SCENE_BLINK_SEPARATION_MS, SCH_CTX_NONE, SCENE_BLINK_SEPARATION_MS);
}

static void restore_bg_color(void)
{
  led_if.set_color(active_scenes.bg_color);
  led_if.set_brightness(active_scenes.bg_brightness);
  led_if.on();
}
