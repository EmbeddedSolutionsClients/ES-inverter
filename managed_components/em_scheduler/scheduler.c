/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/scheduler.h"

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#define LOG_TAG "em_scheduler"

#define ENTRY_NOT_FOUND UINT32_MAX

typedef struct {
  uint32_t param;
  void *user_ctx;
  app_callback_t cb;
} cb_msg_t;

typedef struct {
  TimerHandle_t handle;
  bool scheduled;
  cb_msg_t msg;
} scheduler_entry_t;

static scheduler_entry_t scheduler_entry[CONFIG_EM_SCHEDULER_MAX_ENTRIES];

static StaticTimer_t scheduler_timer_data[CONFIG_EM_SCHEDULER_MAX_ENTRIES];
static SemaphoreHandle_t scheduler_mtx;

static void clear_entry(uint32_t entry_idx)
{
  TimerHandle_t handle = scheduler_entry[entry_idx].handle;

  BaseType_t ret = xTimerStop(handle, portMAX_DELAY);
  assert(ret);

  scheduler_entry[entry_idx].scheduled = false;
  memset(&scheduler_entry[entry_idx].msg, 0x00, sizeof(cb_msg_t));

  /* Timer handle shall never be cleared, it is used for the whole device runtime */
  assert(handle);
}

static uint32_t find_entry(app_callback_t cb)
{
  for (uint32_t i = 0; i < CONFIG_EM_SCHEDULER_MAX_ENTRIES; i++) {
    if (!scheduler_entry[i].scheduled) {
      continue;
    }

    if (scheduler_entry[i].msg.cb != cb) {
      continue;
    }

    return i;
  }

  return ENTRY_NOT_FOUND;
}

static void reschedule_entry(uint32_t param, void *user_ctx, uint32_t delay_ms, uint32_t idx)
{
  TimerHandle_t handle = scheduler_entry[idx].handle;
  assert(handle);

  scheduler_entry[idx].msg.param = param;
  scheduler_entry[idx].msg.user_ctx = user_ctx;

  BaseType_t ret;

  ret = xTimerStop(handle, portMAX_DELAY);
  assert(ret);

  ret = xTimerChangePeriod(handle, pdMS_TO_TICKS(delay_ms), portMAX_DELAY);
  assert(ret);

  ret = xTimerStart(handle, portMAX_DELAY);
  assert(ret);

  ESP_LOGD(LOG_TAG, "Callback=%p rescheduled %lums", scheduler_entry[idx].msg.cb, delay_ms);
}

static void schedule_entry(app_callback_t cb, uint32_t param, void *user_ctx, uint32_t delay_ms)
{
  bool started = false;

  for (uint32_t i = 0; i < CONFIG_EM_SCHEDULER_MAX_ENTRIES; i++) {

    if (scheduler_entry[i].scheduled) {
      /* We are in schedule_entry, looking for a place for new one. Verify if each already scheduled callback is
       * different than given one.
       */
      assert(cb != scheduler_entry[i].msg.cb);
      continue;
    }

    scheduler_entry[i].scheduled = true;

    scheduler_entry[i].msg.cb = cb;
    scheduler_entry[i].msg.param = param;
    scheduler_entry[i].msg.user_ctx = user_ctx;

    TimerHandle_t handle = scheduler_entry[i].handle;
    assert(handle);

    BaseType_t ret = xTimerChangePeriod(handle, pdMS_TO_TICKS(delay_ms), portMAX_DELAY);
    assert(ret);

    ret = xTimerStart(handle, portMAX_DELAY);
    assert(ret);

    started = true;
    break;
  }

  /* Indicate that application needs more timers to run properly */
  assert(started);
}

static void callback_wrapper(TimerHandle_t timer_handle)
{
  xSemaphoreTake(scheduler_mtx, portMAX_DELAY);

  /* see scheduler_init() to understand why we are casting void pointer to uint32_t value */
  uint32_t idx = (uint32_t)pvTimerGetTimerID(timer_handle);

  assert(idx < CONFIG_EM_SCHEDULER_MAX_ENTRIES);
  assert(scheduler_entry[idx].scheduled);
  assert(scheduler_entry[idx].msg.cb);

  cb_msg_t cb_data = scheduler_entry[idx].msg;

  /* Cleanup before callback call to allow user scheduling itself */
  clear_entry(idx);
  xSemaphoreGive(scheduler_mtx);

  /* Call callback */
  cb_data.cb(cb_data.param, cb_data.user_ctx);
}

void scheduler_cancel_callback(app_callback_t cb)
{
  xSemaphoreTake(scheduler_mtx, portMAX_DELAY);

  uint32_t idx = find_entry(cb);
  if (idx != ENTRY_NOT_FOUND) {
    assert(idx < CONFIG_EM_SCHEDULER_MAX_ENTRIES);
    clear_entry(idx);
    ESP_LOGD(LOG_TAG, "Callback=%p cancelled", cb);
  }

  xSemaphoreGive(scheduler_mtx);
}

void scheduler_set_callback(app_callback_t cb, uint32_t param, void *user_ctx, uint32_t delay_ms)
{
  assert(cb);

  if (delay_ms == SCH_NO_DELAY) {
    /* Run from caller task */
    cb(param, user_ctx);
    return;
  }

  /* Scheduler runs on top of the freeRTOS timers. These timers millisecond resolution is defined as
   * (1000UL / CONFIG_FREERTOS_HZ)
   * We need to scale down delay_ms to lowest acceptable value to avoid freeRTOS timer assertion.
   */
  if (delay_ms < (1000UL / CONFIG_FREERTOS_HZ)) {
    delay_ms = (1000UL / CONFIG_FREERTOS_HZ);
  }

  xSemaphoreTake(scheduler_mtx, portMAX_DELAY);

  uint32_t idx = find_entry(cb);
  if (idx == ENTRY_NOT_FOUND) {
    schedule_entry(cb, param, user_ctx, delay_ms);
  } else {
    assert(idx < CONFIG_EM_SCHEDULER_MAX_ENTRIES);
    reschedule_entry(param, user_ctx, delay_ms, idx);
  }

  xSemaphoreGive(scheduler_mtx);
}

void scheduler_init(void)
{
  static StaticSemaphore_t scheduler_mtx_data;
  scheduler_mtx = xSemaphoreCreateMutexStatic(&scheduler_mtx_data);
  assert(scheduler_mtx);

  for (uint32_t i = 0; i < CONFIG_EM_SCHEDULER_MAX_ENTRIES; i++) {
    /* xTimerCreateStatic allow user to pass pvTimerID of created timer, arg: void *const pvTimerID,
     * Typically this would be used in the timer callback function to identify which timer expired when the same
     * callback function is assigned to more than one timer.
     * pvTimerID has to be statically defined because the internal timer queue saves only the pointer to data.
     *
     * Knowing that pvTimerID is a void type pointer we are passing the addresses in range of 0 to
     * CONFIG_EM_SCHEDULER_MAX_ENTRIES.
     * These pointers are treated as values in callback_wrapper() and SHALL NOT be dereferenced.
     * As a result we achieve link between timer handle and the scheduler entry.
     */

    scheduler_entry[i].handle =
      xTimerCreateStatic("", portMAX_DELAY, pdFALSE, (void *)i, callback_wrapper, &scheduler_timer_data[i]);
    assert(scheduler_entry[i].handle);
  }
}
