/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/rx.h"
#include "em/serial_client.h"
#include "em/serial_client_priv_impl.h"
#include "em/tx.h"
#include "em/uart.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#define TAG "SC_TX"

static void delete_timer(TimerHandle_t *t);
static void sender_timer_cb(TimerHandle_t xTimer);

int add_rq(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t timeout, uint8_t retries)
{
  assert(sc->selected_protocol_idx < sc->supported_protocols_cnt);

  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
  assert(pimpl != NULL);

  size_t entries_cnt = sizeof(pimpl->rq) / sizeof(pimpl->rq[0]);

  uint8_t idx = 0;

  for (idx = 0; idx < entries_cnt; ++idx) {
    if (!pimpl->rq[idx].active) {
      break;
    }
  }

  if (idx >= entries_cnt) {
    ESP_LOGE(TAG, "No free message slots");
    return -1;
  }

  size_t serialized_len = 0;
  serial_protocol_t *protocol = &sc->supported_protocols[sc->selected_protocol_idx];
  assert(pimpl->rq[idx].payload == NULL);

  pimpl->rq[idx].payload = protocol->serialize_func(rq_id, payload, payload_len, &serialized_len);

  if (pimpl->rq[idx].payload == NULL) {
    ESP_LOGE(TAG, "Allocate memory for message");
    return -2;
  }

  pimpl->rq[idx].rq_id = rq_id;
  pimpl->rq[idx].payload_len = serialized_len;
  pimpl->rq[idx].timeout = timeout;
  pimpl->rq[idx].since_last_sent = timeout - idx * 100;
  pimpl->rq[idx].active = true;
  pimpl->rq[idx].retries = 0;
  pimpl->rq[idx].max_retries = retries;
  pimpl->rq[idx].periodic = false;

  ESP_LOGI(TAG, "Add message[%d] cmd=%#x retries=%d timeout=%ld", idx, rq_id, retries, timeout);

  if (start_timer(sc, 2000) != 0) {
    ESP_LOGE(TAG, "Start timer failed");
    return -3;
  }

  return idx;
}

int add_periodic_rq(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t period)
{
  assert(period > 1);
  assert(sc->selected_protocol_idx < sc->supported_protocols_cnt);

  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
  assert(pimpl != NULL);

  size_t entries_cnt = sizeof(pimpl->rq) / sizeof(pimpl->rq[0]);

  uint8_t idx = 0;

  for (idx = 0; idx < entries_cnt; ++idx) {
    if (!pimpl->rq[idx].active) {
      break;
    }
  }

  if (idx >= entries_cnt) {
    ESP_LOGE(TAG, "No free periodic message slots");
    return -1;
  }

  size_t serialized_len = 0;
  serial_protocol_t *protocol = &sc->supported_protocols[sc->selected_protocol_idx];
  assert(pimpl->rq[idx].payload == NULL);

  pimpl->rq[idx].payload = protocol->serialize_func(rq_id, payload, payload_len, &serialized_len);

  if (pimpl->rq[idx].payload == NULL) {
    ESP_LOGE(TAG, "Allocate memory for periodic message");
    return -2;
  }

  pimpl->rq[idx].rq_id = rq_id;
  pimpl->rq[idx].payload_len = serialized_len;
  pimpl->rq[idx].timeout = period;
  pimpl->rq[idx].since_last_sent = period - (idx + 1) * 500; // start sending after some time with offset
  pimpl->rq[idx].active = true;
  pimpl->rq[idx].retries = 0;
  pimpl->rq[idx].max_retries = 0;
  pimpl->rq[idx].periodic = true;

  ESP_LOGI(TAG, "Add message[%d] cmd=%#x period=%ld", idx, rq_id, period);

  if (start_timer(sc, 2000) != 0) {
    ESP_LOGE(TAG, "Start timer failed");
    return -3;
  }

  return idx;
}

int remove_periodic_rq(em_sc_t *sc, uint16_t rq_id)
{
  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
  assert(pimpl != NULL);

  for (size_t i = 0; i < CONFIG_EM_SERIAL_CLIENT_MAX_RQS; ++i) {
    if (!pimpl->rq[i].periodic || pimpl->rq[i].rq_id != rq_id) {
      continue; // skip non periodic requests
    }

    ESP_LOGI(TAG, "Remove periodic rq[%d] cmd=%#x", i, rq_id);
    deactivate_rq(pimpl, i);

    for (size_t i = 0; i < CONFIG_EM_SERIAL_CLIENT_MAX_RQS; ++i) {
      if (pimpl->rq[i].active) {
        return 0;
      }
    }

    // if no more requests, stop the timer
    delete_timer(&pimpl->timer);
    return 0;
  }

  ESP_LOGE(TAG, "Periodic rq[%#x] not found", rq_id);
  return -1; // not found
}
/*
void sender_stop(void)
{
  delete_timer();
  ESP_LOGI(TAG, "Periodic sending stop");
}

void sender_suspend(void)
{
  if (xTimerIsTimerActive(sender_timer)) {
    assert(xTimerStop(sender_timer, portMAX_DELAY) == pdPASS);
  }
}

void sender_resume(void)
{
  if (!xTimerIsTimerActive(sender_timer)) {
    assert(xTimerStart(sender_timer, portMAX_DELAY) == pdPASS);
  }
}*/

int start_timer(em_sc_t *sc, uint32_t duration_ms)
{
  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
  assert(pimpl != NULL);

  if (pimpl->timer == NULL) {
    static StaticTimer_t t_buf = {
      0,
    };
    pimpl->timer =
      xTimerCreateStatic("Sender", pdMS_TO_TICKS(duration_ms), pdFALSE, (void *)sc->cmd_queue, sender_timer_cb, &t_buf);

    if (pimpl->timer == NULL) {
      ESP_LOGE(TAG, "Failed to create periodic timer");
      delete_timer(&pimpl->timer);
      return -1;
    }
  } else {
    xTimerChangePeriod(pimpl->timer, pdMS_TO_TICKS(duration_ms), portMAX_DELAY);
  }

  if (xTimerStart(pimpl->timer, portMAX_DELAY) != pdPASS) {
    return -2;
  }

  return 0;
}

static void delete_timer(TimerHandle_t *t)
{
  if (*t != NULL) {
    xTimerStop(*t, portMAX_DELAY);
    xTimerDelete(*t, portMAX_DELAY);
    *t = NULL;
  }
}

static void sender_timer_cb(TimerHandle_t xTimer)
{
  QueueHandle_t queue = (QueueHandle_t)pvTimerGetTimerID(xTimer);
  sc_cmd_t cmd = {.type = SC_CMD_TIMER};
  (void)xQueueSend(queue, &cmd, portMAX_DELAY);
}
