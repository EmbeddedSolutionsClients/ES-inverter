/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/ringbuf.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#define TAG "RINGBUF"

// Initialize the ring buffer
void em_ringbuf_init(em_ringbuf_t *buf)
{
  assert(buf != NULL);
  assert(buf->buffer != NULL);
  assert(buf->size > 0);

  buf->read = 0;
  buf->write = 0;
  buf->is_full = false;

  ESP_LOGI(TAG, "Init with size=%d", buf->size);
}

// Write data to the ring buffer
int em_ringbuf_write(em_ringbuf_t *buf, const uint8_t *data, size_t len)
{
  assert(buf != NULL);
  assert(data != NULL);
  assert(len > 0);

  size_t bytes_written = 0;

  if (buf->is_full) {
    ESP_LOGW(TAG, "Buf is full, overwriting");
  }

  for (size_t i = 0; i < len; i++) {
    if (buf->is_full) {
      buf->read = (buf->read + 1) % buf->size; // Overwrite the oldest data
    }

    buf->buffer[buf->write] = data[i];
    buf->write = (buf->write + 1) % buf->size;
    bytes_written++;

    buf->is_full = buf->read == buf->write;
  }

  return bytes_written;
}

// Read data from the ring buffer
size_t em_ringbuf_read(const em_ringbuf_t *buf, uint8_t *data, size_t len)
{
  assert(buf != NULL);
  assert(data != NULL);
  assert(len > 0);

  if (buf->read == buf->write && !buf->is_full) {
    return 0; // Buffer is empty
  }

  size_t bytes_read = 0;

  for (size_t i = 0; i < len; i++) {
    size_t read_tmp = (buf->read + i) % buf->size;

    if (i > 0 && read_tmp == buf->write) {
      break; // Hit the end
    }

    data[i] = buf->buffer[read_tmp];
    bytes_read++;
  }

  // ESP_LOGI(TAG, "Ringbuf read %d B", bytes_read);
  return bytes_read;
}

// Check if the ring buffer is empty
bool em_ringbuf_is_empty(const em_ringbuf_t *buf)
{
  return (buf->read == buf->write && !buf->is_full);
}

// Check if the ring buffer is full
bool em_ringbuf_is_full(const em_ringbuf_t *buf)
{
  return buf->is_full;
}

size_t em_ringbuf_clear(em_ringbuf_t *buf, size_t bytes_to_clear)
{
  assert(buf != NULL);
  assert(bytes_to_clear > 0);

  size_t buf_len = em_ringbuf_len(buf);

  if (buf_len == 0) {
    ESP_LOGW(TAG, "Buf empty, nothing to clear");
    return 0; // Buffer is empty
  }

  if (bytes_to_clear >= buf_len) {
    em_ringbuf_flush(buf);
    return bytes_to_clear;
  }

  buf->read += bytes_to_clear;
  buf->read %= buf->size;

  // ESP_LOGI(TAG, "Cleared %d B", bytes_to_clear);
  if (bytes_to_clear > 0) {
    buf->is_full = false;
  }

  return bytes_to_clear;
}

void em_ringbuf_flush(em_ringbuf_t *buf)
{
  buf->read = 0;
  buf->write = 0;
  buf->is_full = false;
}

size_t em_ringbuf_len(const em_ringbuf_t *buf)
{
  if (buf->is_full) {
    return buf->size;
  }

  if (buf->write >= buf->read) {
    return buf->write - buf->read;
  } else {
    return buf->size - (buf->read - buf->write);
  }
}

size_t em_ringbuf_free_len(const em_ringbuf_t *buf)
{
  return buf->size - em_ringbuf_len(buf);
}
