/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_UART_RX_H_
#define EM_UART_RX_H_

#include <stddef.h>
#include <stdint.h>

typedef int (*data_received_cb_t)(const uint8_t *data, size_t len, void *param);

void em_uart_rx_task_init(data_received_cb_t cb, void *cb_param);
void em_uart_rx_suspend(void);
void em_uart_rx_resume(void);

#endif /* EM_UART_RX_H_ */
