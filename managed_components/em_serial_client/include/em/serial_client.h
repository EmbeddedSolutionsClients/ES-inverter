/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_SERIAL_CLIENT_H_
#define EM_SERIAL_CLIENT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

typedef size_t (*parse_func_t)(uint16_t *cmd, const uint8_t *data_in, size_t len_in, void *packet_out, uint16_t *error);
typedef uint8_t *(*serialize_func_t)(uint16_t cmd, const uint8_t *payload, size_t payload_len, size_t *out_len);
typedef int (*msg_handler_t)(void *data, size_t data_len);

typedef struct {
  uint8_t protocol_idx;
  uint16_t msg_type;
  msg_handler_t msg_handler;
} serial_rsp_handler_t;

typedef struct {
  parse_func_t parse_func;
  serialize_func_t serialize_func;
  int baud_rate;
  uint8_t data_bits;
  uint8_t stop_bits;
  bool parity;
  const serial_rsp_handler_t *rsp_handlers;
  size_t handlers_cnt;
} serial_protocol_t;

typedef struct {
  uint8_t selected_protocol_idx; // protocol type 0xFF means auto detect
  uint16_t last_rq_id;           // recently sent request cmd
  // int rx_error;
  serial_protocol_t *supported_protocols;
  size_t supported_protocols_cnt;
  QueueHandle_t cmd_queue;
  // uint8_t packet_buf[2][CONFIG_EM_SERIAL_CLIENT_MAX_PACKET_LEN]; // swap buffer
  // bool use_first_buf;
  void *pimpl; // private implementation
} em_sc_t;

void em_sc_init(em_sc_t *sc, uint8_t protocol_idx);
void em_sc_set_protocol_idx(em_sc_t *sc, uint8_t idx);
// size_t em_sc_parse(em_sc_t *client, const uint8_t* data_in, size_t data_in_len, packet_msg_t* packet, int* error);
// int em_sc_packet_handler(em_sc_t *client, uint16_t msg_type, void *data, size_t data_len);
int em_sc_send(em_sc_t *sc, uint16_t cmd, const uint8_t *payload, size_t payload_len, uint32_t timeout,
               uint8_t retries);
int em_sc_send_periodic(em_sc_t *sc, uint16_t cmd, const uint8_t *payload, size_t payload_len, uint32_t period);
int em_sc_remove_periodic(em_sc_t *sc, uint16_t rq_id);

#endif /* EM_SERIAL_CLIENT_H_ */
