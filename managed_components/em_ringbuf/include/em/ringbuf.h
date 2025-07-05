/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_RINGBUF_H_
#define EM_RINGBUF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *buffer; // Pointer to the buffer storage
  size_t size;     // Size of the buffer
  size_t read;
  size_t write;
  bool is_full; // Flag to indicate if the buffer is full
} em_ringbuf_t;

void em_ringbuf_init(em_ringbuf_t *buf);
int em_ringbuf_write(em_ringbuf_t *buf, const uint8_t *data, size_t len);
size_t em_ringbuf_read(const em_ringbuf_t *buf, uint8_t *data, size_t len);
bool em_ringbuf_is_empty(const em_ringbuf_t *buf);
bool em_ringbuf_is_full(const em_ringbuf_t *buf);
size_t em_ringbuf_clear(em_ringbuf_t *buf, size_t bytes_to_clear);
void em_ringbuf_flush(em_ringbuf_t *buf);
size_t em_ringbuf_len(const em_ringbuf_t *buf);
size_t em_ringbuf_free_len(const em_ringbuf_t *buf);

#endif /* EM_RINGBUF_H_ */
