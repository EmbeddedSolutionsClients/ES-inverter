/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/rx.h"
#include "em/serial_client.h"
#include "em/serial_client_priv_impl.h"
#include "em/task.h"
#include "em/tx.h"
#include "em/uart.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#define TAG "SC"

static const serial_protocol_t *em_sc_protocol(const em_sc_t *client)
{
  if (client->selected_protocol_idx >= client->supported_protocols_cnt) {
    ESP_LOGE(TAG, "Invalid protocol index %d", client->selected_protocol_idx);
    return NULL;
  }

  return &client->supported_protocols[client->selected_protocol_idx];
}

void em_sc_init(em_sc_t *sc, uint8_t protocol_idx)
{
  sc->last_rq_id = EXP_RSP_INVALID;
  em_sc_set_protocol_idx(sc, protocol_idx);

  static StaticTask_t task_data = {0};
  static StackType_t task_stack[CONFIG_EM_SERIAL_CLIENT_TASK_STACK_SIZE] = {0};

  TaskHandle_t handle = xTaskCreateStatic(task, "sc", CONFIG_EM_SERIAL_CLIENT_TASK_STACK_SIZE, (void *)sc,
                                          CONFIG_EM_SERIAL_CLIENT_TASK_PRIO, task_stack, &task_data);
  assert(handle);

  const serial_protocol_t *proto = em_sc_protocol(sc);
  em_uart_init(proto->baud_rate, proto->parity, proto->stop_bits, 1, uart_rx_cb, (void *)sc);
}

void em_sc_set_protocol_idx(em_sc_t *client, uint8_t idx)
{
  assert((idx < client->supported_protocols_cnt) || (idx == 0xFF));
  client->selected_protocol_idx = idx;
}

int em_sc_send(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t timeout,
               uint8_t retries)
{
  assert(sc != NULL);
  sc_cmd_t cmd = {
    .type = SC_CMD_ADD_RQ, .payload_len = payload_len, .rq_id = rq_id, .rq.timeout = timeout, .rq.retries = retries};
  memcpy(cmd.payload, payload, payload_len);

  if (xQueueSend(sc->cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
    ESP_LOGE(TAG, "cmd queue full");
    return -1;
  }

  return 0;
}

int em_sc_send_periodic(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t period)
{
  assert(sc != NULL);
  sc_cmd_t cmd = {
    .type = SC_CMD_ADD_RQ_PERIODIC, .payload_len = payload_len, .rq_id = rq_id, .periodic_rq.period = period};
  memcpy(cmd.payload, payload, payload_len);

  if (xQueueSend(sc->cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
    ESP_LOGE(TAG, "cmd queue full");
    return -1;
  }

  return 0;
}

int em_sc_remove_periodic(em_sc_t *sc, uint16_t rq_id)
{
  assert(sc != NULL);
  sc_cmd_t cmd = {.type = SC_CMD_RM_PERIODIC_RQ, .rq_id = rq_id};

  if (xQueueSend(sc->cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
    ESP_LOGE(TAG, "cmd queue full");
    return -1;
  }

  return 0;
}
