/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_SERIAL_CLIENT_TX_H_
#define EM_SERIAL_CLIENT_TX_H_

#include "em/serial_client.h"
#include <stdint.h>

int start_timer(em_sc_t *sc, uint32_t duration_ms);
int add_rq(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t timeout, uint8_t retries);
int add_periodic_rq(em_sc_t *sc, uint16_t rq_id, const uint8_t *payload, size_t payload_len, uint32_t period);
int remove_periodic_rq(em_sc_t *sc, uint16_t rq_id);

#endif /* EM_SERIAL_CLIENT_TX_H_ */
