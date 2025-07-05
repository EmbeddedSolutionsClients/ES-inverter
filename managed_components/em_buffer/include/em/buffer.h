/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t *data;
  uint16_t cap;
  uint16_t len;
  bool dynamic;
} buffer_t;

void buffer_pop_front(buffer_t *ptr, uint16_t num);
void buffer_push(buffer_t *ptr, uint8_t byte);
void buffer_dynamic_alloc(buffer_t *ptr, uint16_t cap);
void em_buffer_free(buffer_t *ptr);
void buffer_clean(buffer_t *ptr);

#endif /* BUFFER_H_ */
