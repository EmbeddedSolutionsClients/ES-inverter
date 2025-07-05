/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_PRIV_IMPL_H_
#define EM_PRIV_IMPL_H_

#include <stdbool.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

typedef enum {
  SC_CMD_ADD_RQ = 1,
  SC_CMD_ADD_RQ_PERIODIC,
  SC_CMD_RM_PERIODIC_RQ,
  SC_CMD_PROCESS_DATA,
  SC_CMD_TIMER,
} sc_cmd_type_t;

typedef struct {
  sc_cmd_type_t type;
  size_t payload_len;
  uint8_t payload[CONFIG_EM_SERIAL_CLIENT_MAX_PACKET_LEN];
  uint16_t rq_id;

  union { // for SC_CMD_ADD_RQ
    struct {
      uint32_t timeout;
      uint8_t retries;
    } rq;

    struct {           // for SC_CMD_RM_PERIODIC_RQ
      uint32_t period; // command to remove
    } periodic_rq;
  };
} sc_cmd_t;

typedef struct {
  uint8_t *payload;
  uint16_t rq_id;
  uint16_t timeout;
  int32_t since_last_sent;
  uint8_t retries;
  uint8_t max_retries;
  uint8_t payload_len;
  bool active;
  bool periodic;
} __attribute__((packed)) rq_t;

typedef struct {
  rq_t rq[CONFIG_EM_SERIAL_CLIENT_MAX_RQS];
  TimerHandle_t timer;
} sc_impl_t;

#endif /* EM_PRIV_IMPL_H_ */
