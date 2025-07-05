/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/scheduler.h"
#include "em/tcp_client.h"
#include "em/wifi.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <esp_event.h>
#include <esp_random.h>
#include <esp_transport.h>
#include <esp_transport_ssl.h>
#include <esp_wifi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <netdb.h>

#include <esp_log.h>
#define LOG_TAG "TCPC"

extern const char ca_crt_start[] asm("_binary_server_cert_pem_start");
extern const char ca_crt_end[] asm("_binary_server_cert_pem_end");

ESP_EVENT_DEFINE_BASE(EM_TCP_CLIENT_EVENT);

typedef enum {
  STATE_DISCONNECTED,
  STATE_CONNECTING,
  STATE_CONNECTED,
} connection_state_t;

typedef enum {
  CMD_CONNECT = 1,
  CMD_DISCONNECT,
  CMD_SEND,
  CMD_LONG_RECONNECTION_DELAY
} cmd_t;

typedef enum {
  TCP_RX_SUSPEND,
  TCP_RX_RESUME,
} rx_task_state_t;

static tcp_client_api_t api;

static connection_state_t conn_state;

static esp_transport_handle_t ssl_handle;
static SemaphoreHandle_t conn_state_mtx;

static QueueHandle_t cmd_q;
static QueueHandle_t send_q;
static QueueHandle_t rx_q;

/********************** COMMON ******************************/

static void submit_tcp_client_disconnect(void)
{
  ESP_LOGI(LOG_TAG, "tcp client disconnect cmd");
  xQueueReset(cmd_q);

  const cmd_t cmd = CMD_DISCONNECT;
  const BaseType_t ret = xQueueSend(cmd_q, &cmd, 0);
  assert(ret == pdTRUE);
}

/********************** RX ******************************/

static int read_socket_data(esp_transport_handle_t t_handle, uint8_t *buf_out, uint32_t buf_out_len)
{
  /* Blocking */
  ESP_LOGD(LOG_TAG, "Socket data reading...");
  int rx_len = esp_transport_read(t_handle, (char *)buf_out, buf_out_len, -1);
  if (rx_len <= 0) {
    ESP_LOGE(LOG_TAG, "Can't read socket data: %d, errno:%d", rx_len, errno);
  }

  ESP_LOGD(LOG_TAG, "Read %i bytes", rx_len);
  return rx_len;
}

static void rx_task(void *param)
{
  ESP_UNUSED(param);
  ESP_LOGI(LOG_TAG, "RX task ready");

  rx_task_state_t task_state = TCP_RX_SUSPEND;
  TickType_t state_duration = portMAX_DELAY;

  while (true) {
    xQueueReceive(rx_q, &task_state, state_duration);
    if (task_state == TCP_RX_SUSPEND) {
      ESP_LOGW(LOG_TAG, "TCP RX Task suspended");
      state_duration = portMAX_DELAY;
      continue;
    }

    int poll = esp_transport_poll_read(ssl_handle, 0);
    if (poll == 0) {
      /* Timeout -> Do not block execution of other tasks and go to sleep */
      state_duration = pdMS_TO_TICKS(CONFIG_EM_TCP_RX_TASK_SLEEPTIME_MS);
      continue;
    }

    if (poll == -1) {
      int err = errno;
      submit_tcp_client_disconnect();
      ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_RX_ERROR, &err, sizeof(err), 0));
      state_duration = portMAX_DELAY;
      continue;
    }

    uint8_t rx_buffer[CONFIG_EM_TCP_RX_BUFFER_SIZE] = {0};

    /* Data available */
    int rx_len = read_socket_data(ssl_handle, rx_buffer, sizeof(rx_buffer));
    if (rx_len <= 0) {
      int err = errno;
      submit_tcp_client_disconnect();
      ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_RX_ERROR, &err, sizeof(err), 0));
      state_duration = portMAX_DELAY;
      continue;
    }

    state_duration = 0UL;

    uint8_t *data_out = NULL;
    uint32_t out_len = 0;

    /* ToDo: Can we somehow use Kconfig to pass appropriate API?
     * Indeed we can just simply have an API struct which describes our TCPclient capabilities
     * Now we only support SLIP but we may also use PPP in future
     */
    if (api.rx_net_proto_cb) {
      /* Call NET protocol callback */
      const esp_err_t ret = api.rx_net_proto_cb(rx_buffer, rx_len, &data_out, &out_len);
      if (ret) {
        ESP_LOGE(LOG_TAG, "Can't decode data through protocol:%i", ret);
        continue;
      }
    } else {
      data_out = rx_buffer;
      out_len = rx_len;
    }

    if (api.msg_disp_cb) {
      /* Call message dispatcher callback */
      const esp_err_t ret = api.msg_disp_cb(data_out, out_len);
      if (ret) {
        ESP_LOGE(LOG_TAG, "Can't dispatch %s data:%i", api.rx_net_proto_cb ? "net protocol" : "raw", ret);
        continue;
      }
    }
  }
}

/********************** TX ******************************/

static connection_state_t get_conn_state(void)
{
  xSemaphoreTake(conn_state_mtx, portMAX_DELAY);
  connection_state_t ret = conn_state;
  xSemaphoreGive(conn_state_mtx);
  return ret;
}

static void set_conn_state(connection_state_t new_conn_state)
{
  xSemaphoreTake(conn_state_mtx, portMAX_DELAY);
  ESP_LOGI(LOG_TAG, "tcp client change status %d->%d", conn_state, new_conn_state);
  conn_state = new_conn_state;
  xSemaphoreGive(conn_state_mtx);
}

static void submit_tcp_client_connect(void)
{
  ESP_LOGI(LOG_TAG, "tcp client connect cmd state=%d", get_conn_state());
  xQueueReset(cmd_q);

  const cmd_t cmd = CMD_CONNECT;
  const BaseType_t ret = xQueueSend(cmd_q, &cmd, 0);
  assert(ret == pdTRUE);
}

static void connect_handler(uint32_t param, void *user_ctx)
{
  ESP_UNUSED(param);
  ESP_UNUSED(user_ctx);

  submit_tcp_client_connect();
}

static void close_conn_handler(void)
{
  if (ssl_handle == NULL) {
    ESP_LOGI(LOG_TAG, "Handle not found");
    return;
  }

  const int ret = esp_transport_close(ssl_handle);
  if (ret && errno != ENOTCONN) {
    ESP_LOGE(LOG_TAG, "Can't close transport: %d", errno);
  }
}

static esp_err_t open_conn_handler(void)
{
  struct hostent *he = gethostbyname(CONFIG_EM_TCP_HOSTNAME);
  if (he == NULL) {
    ESP_LOGE(LOG_TAG, "gethostbyname NULL");
    return ESP_FAIL;
  }

  char *host_ip = inet_ntoa(*(struct in_addr *)he->h_addr);
  ESP_LOGI(LOG_TAG, "connecting to %s (%s:%d)", he->h_name, host_ip, CONFIG_EM_TCP_PORT);

  esp_err_t ret = esp_transport_connect(ssl_handle, CONFIG_EM_TCP_HOSTNAME, CONFIG_EM_TCP_PORT, -1);
  if (ret != 0) {
    ESP_LOGE(LOG_TAG, "Can't connect: %d", errno);
    return ret;
  }

  ESP_LOGI(LOG_TAG, "##### CONNECTED #####");

  const rx_task_state_t state = TCP_RX_RESUME;
  const BaseType_t ret_q = xQueueSend(rx_q, &state, 0);
  assert(ret_q == pdTRUE);
  return ESP_OK;
}

static esp_err_t send_handler(void)
{
  buffer_t buf = {0};

  if (pdFALSE == xQueueReceive(send_q, &buf, 0)) {
    ESP_LOGE(LOG_TAG, "send_q empty");
    return ESP_ERR_NOT_FOUND;
  }

  if (get_conn_state() != STATE_CONNECTED) {
    em_buffer_free(&buf);
    ESP_LOGE(LOG_TAG, "Not connected dropping data to send");
    return ESP_FAIL;
  }

  if (buf.len == 0) {
    em_buffer_free(&buf);
    return ESP_ERR_INVALID_SIZE;
  }

  uint8_t *data_out = NULL;
  uint32_t out_len = 0;

  if (api.tx_net_proto_cb) {
    const esp_err_t ret = api.tx_net_proto_cb(buf.data, buf.len, &data_out, &out_len);
    if (ret != ESP_OK) {
      em_buffer_free(&buf);
      return ret;
    }
  } else {
    data_out = buf.data;
    out_len = buf.len;
  }

  em_buffer_free(&buf);

  assert(out_len <= INT32_MAX && out_len > 0);
  assert(data_out);

  int32_t len = (int32_t)out_len;
  int32_t widx = 0;

  while (len > 0) {
    /* Do not timeout (-1) write until error or success */
    int32_t wlen = esp_transport_write(ssl_handle, (char *)data_out + widx, len, -1);
    if (wlen <= 0) {
      ESP_LOGE(LOG_TAG, "send failed: errno %d", errno);
      return ESP_FAIL;
    }

    widx += wlen;
    len -= wlen;
  }

  return ESP_OK;
}

static void tx_task(void *param)
{
  ESP_UNUSED(param);
  ESP_LOGI(LOG_TAG, "TX task ready");

  const uint32_t short_conn_delay_offset = 1000;   // 1 second
  const uint32_t long_conn_delay_offset = 3600000; // 1 hour
  uint32_t conn_delay_offset_ms = short_conn_delay_offset;

  while (1) {
    cmd_t cmd;
    xQueueReceive(cmd_q, &cmd, portMAX_DELAY);

    switch (cmd) {
    case CMD_CONNECT: {
      set_conn_state(STATE_CONNECTING);
      const esp_err_t ret = open_conn_handler();
      if (ret != 0) {
        submit_tcp_client_disconnect();
        ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_CONN_ERROR, &errno, sizeof(errno), 0));
      } else {
        conn_delay_offset_ms = short_conn_delay_offset;
        set_conn_state(STATE_CONNECTED);
        ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_ONLINE, NULL, 0, 0));
      }
    } break;

    case CMD_DISCONNECT: {
      set_conn_state(STATE_DISCONNECTED);
      close_conn_handler();

      if (em_wifi_sta_is_connected(0)) {
        ESP_LOGI(LOG_TAG, "WiFi available reconnect TCP");
        const uint32_t rand_ms = (esp_random() & CONFIG_EM_TCP_CONN_RANDOM_DELAY_MS) + conn_delay_offset_ms;
        scheduler_set_callback(connect_handler, SCH_PARAM_NONE, SCH_CTX_NONE,
                               CONFIG_EM_TCP_RECONNECT_TIMEOUT_MS + rand_ms);
      } else {
        ESP_LOGI(LOG_TAG, "No WiFI cant reconnect TCP");
      }

      ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_OFFLINE, NULL, 0, 0));
    } break;

    case CMD_SEND: {
      esp_err_t ret = send_handler();

      if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "send TX err: %d", ret);
      }

      em_tcp_tx_status_event_data_t evt_data = {
        .err_code = ret == ESP_FAIL ? errno : ret,
      };

      if (ret == ESP_FAIL) {
        submit_tcp_client_disconnect();
        ESP_ERROR_CHECK(esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_TX_ERROR, &errno, sizeof(errno), 0));
      }

      ESP_ERROR_CHECK(
        esp_event_post(EM_TCP_CLIENT_EVENT, EM_TCP_CLIENT_EVENT_TX_STATUS, &evt_data, sizeof(evt_data), 0));

      break;
    }

    case CMD_LONG_RECONNECTION_DELAY:
      conn_delay_offset_ms = long_conn_delay_offset;
      break;

    default:
      ESP_LOGE(LOG_TAG, "Unknown command: %d", cmd);
      break;
    }
  }
}

static void ip_assigned_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
  /* Without this delay socket get closed on it's own quickly after connecting.
   * Seems that connecting to TCP server upon GOT_IP event might be to early ?
   * TODO: We should debug with Wireshark what happens just after IP assignment
   */

  /* Add randomness to avoid all devices connecting at the same time (TEST STAND) and ensure connection randomness
   */
  const uint32_t rand_ms = (esp_random() & CONFIG_EM_TCP_CONN_RANDOM_DELAY_MS) + 1000;
  scheduler_set_callback(connect_handler, SCH_PARAM_NONE, SCH_CTX_NONE, rand_ms);
}

static void wifi_disconnected_handler(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
  ESP_UNUSED(arg);
  ESP_UNUSED(base);
  ESP_UNUSED(event_id);
  ESP_UNUSED(data);

  if (get_conn_state() == STATE_DISCONNECTED) {
    return;
  }

  ESP_LOGW(LOG_TAG, "Lost WiFi connection TCP disconnected");

  submit_tcp_client_disconnect();

  const rx_task_state_t state = TCP_RX_SUSPEND;
  const BaseType_t ret = xQueueSend(rx_q, &state, 0);
  assert(ret == pdTRUE);
}

void em_tcp_client_init(tcp_client_api_t tcp_api)
{
  api = tcp_api;

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_assigned_handler, NULL));
  ESP_ERROR_CHECK(
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_disconnected_handler, NULL));

  ssl_handle = esp_transport_ssl_init();
  assert(ssl_handle);
  esp_transport_ssl_set_tls_version(ssl_handle, ESP_TLS_VER_ANY);

  /* I have no idea why -1 is required but esp_transport_ssl_set_cert_data adds +1 under the hood. Without it the
   * mbedtls reports parsing error
   */
  esp_transport_ssl_set_cert_data(ssl_handle, ca_crt_start, ca_crt_end - ca_crt_start - 1);
  ESP_ERROR_CHECK(esp_transport_set_default_port(ssl_handle, CONFIG_EM_TCP_PORT));

  static StaticSemaphore_t conn_state_mtx_data;
  conn_state_mtx = xSemaphoreCreateMutexStatic(&conn_state_mtx_data);
  configASSERT(conn_state_mtx);

  static StaticQueue_t cmd_q_data = {0};
  static uint8_t cmd_q_storage[CONFIG_EM_TCP_CMD_MSG_Q_MAX * sizeof(cmd_t)] = {0};
  cmd_q = xQueueCreateStatic(CONFIG_EM_TCP_CMD_MSG_Q_MAX, sizeof(cmd_t), cmd_q_storage, &cmd_q_data);
  configASSERT(cmd_q);

  static StaticQueue_t send_q_data = {0};
  static uint8_t send_q_storage[CONFIG_EM_TCP_TX_SEND_MSG_Q_MAX * sizeof(buffer_t)] = {0};
  send_q = xQueueCreateStatic(CONFIG_EM_TCP_TX_SEND_MSG_Q_MAX, sizeof(buffer_t), send_q_storage, &send_q_data);
  configASSERT(send_q);

  static StaticQueue_t rx_q_data = {0};
  static uint8_t rx_q_storage[CONFIG_EM_TCP_RX_STATE_MSG_Q_MAX * sizeof(rx_task_state_t)] = {0};
  rx_q = xQueueCreateStatic(CONFIG_EM_TCP_RX_STATE_MSG_Q_MAX, sizeof(rx_task_state_t), rx_q_storage, &rx_q_data);
  configASSERT(rx_q);

  static StaticTask_t tx_task_data = {0};
  static StackType_t tx_task_stack[CONFIG_EM_TCP_TX_TASK_STACK_SIZE] = {0};
  TaskHandle_t handle = xTaskCreateStatic(tx_task, "tcp_tx", CONFIG_EM_TCP_TX_TASK_STACK_SIZE, NULL,
                                          CONFIG_EM_TCP_TX_TASK_PRIO, tx_task_stack, &tx_task_data);
  configASSERT(handle);

  static StaticTask_t rx_task_data = {0};
  static StackType_t rx_task_stack[CONFIG_EM_TCP_RX_TASK_STACK_SIZE] = {0};
  handle = xTaskCreateStatic(rx_task, "tcp_rx", CONFIG_EM_TCP_RX_TASK_STACK_SIZE, NULL, CONFIG_EM_TCP_RX_TASK_PRIO,
                             rx_task_stack, &rx_task_data);
  configASSERT(handle);
}

/* TODO: TCP: em_tcp_client_send should allow to register ON SEND CALLBACK. I mean that CMD_SEND should allow to pass
 * the function pointer over queue. If NULL then do not execute */

esp_err_t em_tcp_client_send(const buffer_t *buf)
{
  if (buf->len == 0U) {
    return ESP_ERR_INVALID_SIZE;
  }

  assert(buf->data != NULL);

  if (xQueueSend(send_q, buf, pdMS_TO_TICKS(CONFIG_EM_TCP_MSG_SUBMIT_TIMEOUT_MS)) != pdTRUE) {
    ESP_LOGW(LOG_TAG, "send_q full, drop");
    return ESP_FAIL;
  }

  cmd_t cmd = CMD_SEND;
  if (xQueueSend(cmd_q, &cmd, pdMS_TO_TICKS(CONFIG_EM_TCP_MSG_SUBMIT_TIMEOUT_MS)) != pdTRUE) {
    ESP_LOGW(LOG_TAG, "cmd_q full, drop");
    return ESP_FAIL;
  }

  return ESP_OK;
}

void em_tcp_client_disconnect_panic(void)
{
  ESP_LOGI(LOG_TAG, "On panic TCP disconnect");
  /* Do not process the disconnect through TCP TX task as PANIC requires immediate action. As a result reconnection
   * won't happen.
   * That is expected as disconnect_panic shall be called on esp_restart() only.
   */
  set_conn_state(STATE_DISCONNECTED);
  close_conn_handler();
}

void em_tcp_client_set_long_reconnecting_delay(void)
{
  ESP_LOGI(LOG_TAG, "Set long reconnecting delay");
  cmd_t cmd = CMD_LONG_RECONNECTION_DELAY;
  if (xQueueSend(cmd_q, &cmd, pdMS_TO_TICKS(CONFIG_EM_TCP_MSG_SUBMIT_TIMEOUT_MS)) != pdTRUE) {
    ESP_LOGW(LOG_TAG, "cmd_q full, drop");
  }
}
