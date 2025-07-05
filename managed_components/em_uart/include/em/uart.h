/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_UART_H_
#define EM_UART_H_

#include "driver/uart.h"
#include "em/uart_rx.h"

void em_uart_init(int baud_rate, uart_parity_t parity, uart_stop_bits_t stop_bits, uint8_t rx_timeout_symbols,
                  data_received_cb_t cb, void *cb_param);
int em_uart_send(const uint8_t *data, size_t len);
void em_uart_change_config(int baud_rate, uart_parity_t parity, uart_stop_bits_t stop_bits, uint8_t rx_timeout);

#endif /* EM_UART_H_ */
