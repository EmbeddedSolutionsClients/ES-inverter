/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "driver/uart.h"
#include "em/uart_rx.h"
#include "esp_log.h"
#include <stdlib.h>

#define TAG "UART"

const static uart_port_t uart_num = CONFIG_EM_UART_PORT_NUM;

static uart_config_t uart_config = {
  .baud_rate = 9600,
  .data_bits = UART_DATA_8_BITS,
  .parity = UART_PARITY_DISABLE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  .source_clk = UART_SCLK_DEFAULT,
};

void em_uart_init(int baud_rate, uart_parity_t parity, uart_stop_bits_t stop_bits, uint8_t rx_timeout_symbols,
                  data_received_cb_t cb, void *cb_param)
{
  uart_config.baud_rate = baud_rate;
  uart_config.parity = parity;
  uart_config.stop_bits = stop_bits;
  ESP_ERROR_CHECK(uart_driver_install(uart_num, 2 * CONFIG_EM_UART_READ_BUF_SIZE, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(
    uart_set_pin(uart_num, CONFIG_EM_UART_TXD, CONFIG_EM_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_UART));
  ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, rx_timeout_symbols)); // max value is 102

  em_uart_rx_task_init(cb, cb_param);
}

int em_uart_send(const uint8_t *data, size_t len)
{
  int n = uart_write_bytes(CONFIG_EM_UART_PORT_NUM, (const char *)data, len);

  if (n < 0) {
    return -1;
  } else if ((size_t)n < len) {
    return -2;
  }

  return 0;
}

void em_uart_change_config(int baud_rate, uart_parity_t parity, uart_stop_bits_t stop_bits, uint8_t rx_timeout)
{
  em_uart_rx_suspend();

  uart_config.baud_rate = baud_rate;
  uart_config.parity = parity;
  uart_config.stop_bits = stop_bits;
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, rx_timeout));

  em_uart_rx_resume();
}
