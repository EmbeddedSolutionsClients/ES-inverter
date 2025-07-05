/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef COREDUMP_H_
#define COREDUMP_H_

#include <stdint.h>
#include <esp_err.h>
#include <esp_system.h>

typedef int (*coredump_valid_t)(uint32_t coredump_version);
typedef int (*coredump_body_t)(uint32_t coredump_version, uint16_t block_len, uint8_t *block_data);
typedef int (*coredump_end_t)(uint32_t coredump_version);

typedef struct {
  coredump_valid_t valid;
  coredump_body_t body;
  coredump_end_t end;
} coredump_api_t;

int coredump_start(coredump_api_t cfg);
int coredump_on_data_sent_callback(bool success);
int coredump_erase(void);

#endif /* COREDUMP_H_*/
