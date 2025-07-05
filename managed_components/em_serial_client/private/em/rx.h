/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_SERIAL_CLIENT_RX_H_
#define EM_SERIAL_CLIENT_RX_H_

#include "em/ringbuf.h"
#include "em/serial_client.h"
#include "em/serial_client_priv_impl.h"
#include <stdint.h>

#define EXP_RSP_INVALID (0xFFFF)
#define EXP_RSP_DETECT  (0xDDFF)

typedef enum {
  PARSE_STATUS_UNKNOWN = 0,
  PARSE_STATUS_OK = 1,
  PARSE_STATUS_NO_PACKET = 2,
  PARSE_STATUS_INVALID_CRC = 3,
  PARSE_STATUS_RQ_REJECTED = 4,
  PARSE_STATUS_RSP_UNEXPECTED = 5,
  PARSE_STATUS_PACKET_MALFORMED = 6,
} parse_status_t;

typedef struct {
  uint8_t payload[CONFIG_EM_SERIAL_CLIENT_MAX_PACKET_LEN];
  size_t payload_len;
  uint16_t rsp_id;
  parse_status_t status;
} parse_result_t;

void process_data(em_sc_t *cliscent, em_ringbuf_t *ring, const uint8_t *data, size_t data_len);
size_t parse(em_sc_t *sc, const uint8_t *data_in, size_t data_in_len, parse_result_t *result);
int uart_rx_cb(const uint8_t *data, size_t data_len, void *param);
void packet_handler(em_sc_t *client, uint16_t msg_type, void *data, size_t data_len);
void ack_rsp(em_sc_t *sc, uint16_t corresponding_rq);
void deactivate_rq(sc_impl_t *pimpl, uint8_t idx);

#endif /* EM_SERIAL_CLIENT_RX_H_ */
