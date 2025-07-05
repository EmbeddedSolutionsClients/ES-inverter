/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/ringbuf.h"
#include "em/rx.h"
#include "em/serial_client.h"
#include "em/serial_client_priv_impl.h"
#include "em/task.h"
#include "em/tx.h"
#include "em/uart.h"

#include "esp_log.h"
#include <string.h>
#define TAG "SC"

static void schedule_next_timer_evt(em_sc_t *sc, int32_t next_rq_delay);
static void send_rq(em_sc_t *sc);

void task(void *param)
{
  ESP_UNUSED(param);
  assert(param != NULL);

  em_sc_t *sc = (em_sc_t *)param;
  assert(sc);

  StaticQueue_t cmd_q_data = {0};
  uint8_t cmd_q_buf[2 * sizeof(sc_cmd_t)];
  sc->cmd_queue = xQueueCreateStatic(2, sizeof(sc_cmd_t), cmd_q_buf, &cmd_q_data);
  assert(sc->cmd_queue);

  uint8_t ring_buf_storage[2 * CONFIG_EM_SERIAL_CLIENT_MAX_PACKET_LEN];

  em_ringbuf_t ring = {
    .size = sizeof(ring_buf_storage),
    .buffer = ring_buf_storage,
  };

  em_ringbuf_init(&ring);

  sc_impl_t pimpl = {.rq = {{
                       0,
                     }},
                     .timer = NULL};
  sc->pimpl = &pimpl;

  ESP_LOGI(TAG, "Initialized protocol=%d", sc->selected_protocol_idx);

  while (true) {
    sc_cmd_t cmd = {0};
    if (xQueueReceive(sc->cmd_queue, &cmd, portMAX_DELAY) != pdPASS) {
      continue;
    }

    switch (cmd.type) {
    case SC_CMD_ADD_RQ:
      add_rq(sc, cmd.rq_id, cmd.payload, cmd.payload_len, cmd.rq.timeout, cmd.rq.retries);
      break;

    case SC_CMD_ADD_RQ_PERIODIC:
      add_periodic_rq(sc, cmd.rq_id, cmd.payload, cmd.payload_len, cmd.periodic_rq.period);
      break;

    case SC_CMD_RM_PERIODIC_RQ:
      remove_periodic_rq(sc, cmd.rq_id);
      break;

    case SC_CMD_PROCESS_DATA:
      process_data(sc, &ring, cmd.payload, cmd.payload_len);
      break;

    case SC_CMD_TIMER:
      send_rq(sc);
      break;
    default:
      ESP_LOGE(TAG, "Unknown cmd=%d", cmd.type);
      break;
    }
  }
}

static void send_rq(em_sc_t *sc)
{
  assert(sc != NULL);
  assert(sc->pimpl != NULL);

  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
  assert(pimpl != NULL);

  size_t entries_cnt = sizeof(pimpl->rq) / sizeof(pimpl->rq[0]);
  size_t send_idx = UINT32_MAX;
  const uint32_t response_delay_ms = 100;

  // find the earliest request to send
  int32_t earliest_rq_delay = INT32_MAX;

  for (size_t i = 0; i < entries_cnt; ++i) {
    if (!pimpl->rq[i].active) {
      continue;
    }

    if (pimpl->rq[i].timeout - pimpl->rq[i].since_last_sent < earliest_rq_delay) {
      earliest_rq_delay = pimpl->rq[i].timeout - pimpl->rq[i].since_last_sent;
      send_idx = i;
    }
  }

  if (earliest_rq_delay > 50) { // if les then 50ms to send the period message, can be send already
    schedule_next_timer_evt(sc, earliest_rq_delay);
    return;
  }

  if (send_idx >= entries_cnt) {
    assert(false);
    return;
  }

  assert(pimpl->rq[send_idx].payload_len > 0);
  sc->last_rq_id = pimpl->rq[send_idx].rq_id; // remember id of last sent request
  int err = em_uart_send(pimpl->rq[send_idx].payload, pimpl->rq[send_idx].payload_len);

  if (err != 0) {
    ESP_LOGE(TAG, "Failed to send rq idx=%d err=%d", send_idx, err);
    schedule_next_timer_evt(sc, 500); // retry after 500ms
    return;
  }

  ++pimpl->rq[send_idx].retries;

  if (!pimpl->rq[send_idx].periodic) {
    pimpl->rq[send_idx].since_last_sent = 0; // retry after timeout if rsp not received

    if (pimpl->rq[send_idx].retries > pimpl->rq[send_idx].max_retries) {
      ESP_LOGW(TAG, "No rsp for rq[%d]=%x after %d retries", send_idx, pimpl->rq[send_idx].rq_id,
               pimpl->rq[send_idx].retries);
      pimpl->rq[send_idx].retries = 0;
      deactivate_rq(pimpl, send_idx);
    }
  } else {
    if (pimpl->rq[send_idx].retries > 2) {
      pimpl->rq[send_idx].since_last_sent = 0; // give up retry for periodic
      ESP_LOGW(TAG, "No rsp for periodic rq[%d]=%x after %d retries", send_idx, pimpl->rq[send_idx].rq_id,
               pimpl->rq[send_idx].retries);
      pimpl->rq[send_idx].retries = 0;
    } else {
      pimpl->rq[send_idx].since_last_sent =
        pimpl->rq[send_idx].since_last_sent - response_delay_ms; // retry after 100ms
    }
  }

  // find the next request to send
  int32_t next_rq_delay = INT32_MAX;

  for (size_t i = 0; i < entries_cnt; ++i) {
    if (!pimpl->rq[i].active) {
      continue;
    }

    if (pimpl->rq[i].timeout - pimpl->rq[i].since_last_sent < next_rq_delay) {
      next_rq_delay = pimpl->rq[i].timeout - pimpl->rq[i].since_last_sent;
    }
  }

  if (next_rq_delay < response_delay_ms) {
    next_rq_delay = response_delay_ms; // keep waiting at least 100ms for the previous rq response
  }

  schedule_next_timer_evt(sc, next_rq_delay);
}

static void schedule_next_timer_evt(em_sc_t *sc, int32_t delay_ms)
{
  if (delay_ms != INT32_MAX) {
    sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;
    int err = start_timer(sc, delay_ms);

    if (err != 0) {
      ESP_LOGE(TAG, "Failed to start timer err=%d", err);
    }

    size_t entries_cnt = sizeof(pimpl->rq) / sizeof(pimpl->rq[0]);

    for (size_t i = 0; i < entries_cnt; ++i) {
      if (!pimpl->rq[i].active) {
        continue;
      }

      pimpl->rq[i].since_last_sent += delay_ms;
    }
  } else {
    ESP_LOGI(TAG, "No more rq to send");
  }
}
