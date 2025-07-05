/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "driver/uart.h"
#include "em/uart_rx.h"
#include "esp_log.h"
#include <stdlib.h>

#define TAG "UART_RX"

// RTS for RS485 Half-Duplex Mode manages DE/~RE
// #define ECHO_TEST_RTS   (CONFIG_EM_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
// #define ECHO_TEST_CTS   (UART_PIN_NO_CHANGE)

static data_received_cb_t data_received_cb = NULL;
static void *data_received_cb_param = NULL;

static void rx_task(void *arg);

void em_uart_rx_task_init(data_received_cb_t cb, void *cb_param)
{
  if (cb == NULL) {
    ESP_LOGE(TAG, "Callback function is NULL");
    return;
  }

  data_received_cb = cb;
  data_received_cb_param = cb_param;

  xTaskCreate(rx_task, "uart_rx", CONFIG_EM_UART_TASK_STACK_SIZE, NULL, CONFIG_EM_UART_TASK_PRIO, NULL);
}

void em_uart_rx_suspend(void)
{
  vTaskSuspend(NULL);
}

void em_uart_rx_resume(void)
{
  vTaskResume(NULL);
}

static void rx_task(void *arg)
{
  const int uart_num = CONFIG_EM_UART_PORT_NUM;
  ESP_LOGI(TAG, "start thread");

  while (true) {
    uint8_t data[CONFIG_EM_UART_READ_BUF_SIZE] = {0};
    int data_len = uart_read_bytes(uart_num, data, sizeof(data), 1);

    if (data_len != 0) {
      data_received_cb(data, data_len, data_received_cb_param);
    }
  }
}
