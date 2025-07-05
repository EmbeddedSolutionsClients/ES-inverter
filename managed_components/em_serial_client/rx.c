/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/rx.h"
#include "em/serial_client_priv_impl.h"
#include "em/tx.h"

#include "esp_log.h"
#include <string.h>
#define TAG "SC_RX"

void process_data(em_sc_t *sc, em_ringbuf_t *ring, const uint8_t *new_data, size_t new_data_len)
{
  int n = em_ringbuf_write(ring, new_data, new_data_len);

  if (n != new_data_len) {
    ESP_LOGW(TAG, "Failed to write data to ring buffer");
    return;
  }

  uint8_t data[CONFIG_EM_SERIAL_CLIENT_MAX_PACKET_LEN] = {
    0,
  };
  size_t data_len = em_ringbuf_read(ring, data, sizeof(data));

  if (data_len == 0) {
    ESP_LOGE(TAG, "No data in ring buffer");
    return;
  }

  if (sc->selected_protocol_idx == 0xFF) {
    // TODO: implement auto detect
  }

  size_t processed_bytes = 0;
  parse_result_t result = {
    0,
  };

  // so as not to stuck this task for too long limit times of parsing rounds to 8
  for (int i = 0; processed_bytes < data_len && i < 8; i++) {
    processed_bytes += parse(sc, data + processed_bytes, data_len - processed_bytes, &result);

    if (result.status == PARSE_STATUS_OK) {
      // ESP_LOGI(TAG, "Data handler %d B exp_cmd=%x proto=%d", data_len, sc->last_rq_id, sc->selected_protocol_idx);
      packet_handler(sc, sc->last_rq_id, result.payload, result.payload_len);
    }
  }

  if (processed_bytes > 0) {
    em_ringbuf_clear(ring, processed_bytes);
  }
}

// returns number of process bytes or negative value on error
size_t parse(em_sc_t *sc, const uint8_t *data_in, size_t data_in_len, parse_result_t *result)
{
  assert(sc != NULL);
  assert(data_in != NULL);
  assert(sc->selected_protocol_idx < sc->supported_protocols_cnt);

  result->status = PARSE_STATUS_NO_PACKET;
  result->payload_len = 0;
  result->rsp_id = sc->last_rq_id;

  if (sc->last_rq_id == EXP_RSP_INVALID) {
    ESP_LOGE(TAG, "Last command is unknown");
    return 0;
  }

  size_t processed_bytes = 0;

  for (int i = 0; processed_bytes < data_in_len && i < 8; i++) {
    const uint8_t *read_ptr = data_in + processed_bytes;
    size_t read_len = data_in_len - processed_bytes;
    size_t n = sc->supported_protocols[sc->selected_protocol_idx].parse_func(
      &result->rsp_id, read_ptr, read_len, result->payload, (uint16_t *)&result->status);
    processed_bytes += n;
    // ESP_LOGI(TAG, "Parsed %d bytes rsp_id=%#x", n, result->rsp_id);

    switch (result->status) {
    case PARSE_STATUS_OK:
      if (sc->last_rq_id != EXP_RSP_DETECT && result->rsp_id != sc->last_rq_id) {
        ESP_LOGI(TAG, "Unexpected rsp=%x exp=%x", result->rsp_id, sc->last_rq_id);
        result->status = PARSE_STATUS_RSP_UNEXPECTED;
      }

      return processed_bytes;

    case PARSE_STATUS_NO_PACKET:
      break;

    case PARSE_STATUS_INVALID_CRC:
      ESP_LOGW(TAG, "CRC error rsp for rq=%x", sc->last_rq_id);
      return processed_bytes;

    case PARSE_STATUS_RQ_REJECTED:
      ESP_LOGI(TAG, "Rsp=%x denied", sc->last_rq_id);
      ack_rsp(sc, sc->last_rq_id);
      return processed_bytes;

    case PARSE_STATUS_PACKET_MALFORMED:
      ESP_LOGE(TAG, "Malformed packet for rq=%x", sc->last_rq_id);
      result->status = PARSE_STATUS_PACKET_MALFORMED;
      return processed_bytes;

    default:
      ESP_LOGI(TAG, "Unknown parsing status=%x", result->status);
      break;
    }
  }

  return processed_bytes;
}

void packet_handler(em_sc_t *client, uint16_t msg_type, void *data, size_t data_len)
{
  ack_rsp(client, msg_type);
  assert(client->selected_protocol_idx < client->supported_protocols_cnt);
  const serial_protocol_t *active_proto = &client->supported_protocols[client->selected_protocol_idx];

  /* Go through registered handlers */
  for (uint32_t i = 0; i < active_proto->handlers_cnt; i++) {
    if (msg_type != active_proto->rsp_handlers[i].msg_type) {
      continue;
    }

    active_proto->rsp_handlers[i].msg_handler(data, data_len);
    return;
  }

  ESP_LOGW(TAG, "No handler for protocol %d msg %d", client->selected_protocol_idx, msg_type);
  return;
}

// callback
int uart_rx_cb(const uint8_t *data, size_t data_len, void *param)
{
  assert(data != NULL);
  assert(data_len > 0);
  assert(param != NULL);

  sc_cmd_t cmd = {.type = SC_CMD_PROCESS_DATA, .payload_len = data_len};
  memcpy(cmd.payload, data, data_len);

  em_sc_t *sc = (em_sc_t *)param;

  if (xQueueSend(sc->cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
    ESP_LOGE(TAG, "cmd queue full");
    return 0;
  }

  return -1;
}

void ack_rsp(em_sc_t *sc, uint16_t corresponding_rq)
{
  // ESP_LOGI(TAG, "Ack response for cmd %x", corresponding_rq);
  assert(sc != NULL);
  sc_impl_t *pimpl = (sc_impl_t *)sc->pimpl;

  size_t entries_cnt = sizeof(pimpl->rq) / sizeof(pimpl->rq[0]);

  if (sc->last_rq_id == corresponding_rq) {
    // ESP_LOGI(TAG, "Rsp %x received", corresponding_rq);
    sc->last_rq_id = EXP_RSP_INVALID;
  }

  for (uint8_t i = 0; i < entries_cnt; i++) {
    if (!pimpl->rq[i].active) {
      // ESP_LOGI(TAG, "%d Not active", i);
      continue;
    }

    // ESP_LOGI(TAG, "Rsp received[%d] pimpl->rq[i].cmd=%x", i, pimpl->rq[i].cmd);

    if (pimpl->rq[i].rq_id == corresponding_rq) {
      // ESP_LOGI(TAG, "Acked ok cmd %d", corresponding_rq);
      pimpl->rq[i].since_last_sent = 0;
      pimpl->rq[i].retries = 0;

      if (pimpl->rq[i].max_retries > 0) { // if not periodic
        ESP_LOGI(TAG, "Rq[%d]=%x satisfied", i, pimpl->rq[i].rq_id);
        deactivate_rq(pimpl, i);
      }

      break;
    }
  }
}

void deactivate_rq(sc_impl_t *pimpl, uint8_t idx)
{
  pimpl->rq[idx].active = false;

  if (pimpl->rq[idx].payload != NULL) {
    free(pimpl->rq[idx].payload);
    pimpl->rq[idx].payload = NULL;
  }
}
