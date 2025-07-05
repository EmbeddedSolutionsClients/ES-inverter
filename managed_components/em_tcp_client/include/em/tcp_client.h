/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include <stdbool.h>
#include <esp_event.h>

#include "em/buffer.h"

ESP_EVENT_DECLARE_BASE(EM_TCP_CLIENT_EVENT);

typedef esp_err_t (*msg_dispatcher_cb_t)(const uint8_t *data, uint32_t len);
typedef esp_err_t (*tcp_net_proto_cb_t)(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out);

typedef struct {
  msg_dispatcher_cb_t msg_disp_cb;
  tcp_net_proto_cb_t rx_net_proto_cb;
  tcp_net_proto_cb_t tx_net_proto_cb;
} tcp_client_api_t;

typedef enum {
  EM_TCP_CLIENT_EVENT_OFFLINE,
  EM_TCP_CLIENT_EVENT_ONLINE,
  EM_TCP_CLIENT_EVENT_AUTH,
  EM_TCP_CLIENT_EVENT_CONN_ERROR,
  EM_TCP_CLIENT_EVENT_RX_ERROR,
  EM_TCP_CLIENT_EVENT_TX_ERROR,
  EM_TCP_CLIENT_EVENT_AUTH_ERROR,
  EM_TCP_CLIENT_EVENT_TX_STATUS,
} em_evt_tcp_client_t;

typedef struct {
  int err_code;
} em_tcp_tx_status_event_data_t;

void em_tcp_client_init(tcp_client_api_t api);
esp_err_t em_tcp_client_send(const buffer_t *buf);
void em_tcp_client_disconnect_panic(void);
void em_tcp_client_set_long_reconnecting_delay(void);

#endif /* TCP_CLIENT_H_ */
