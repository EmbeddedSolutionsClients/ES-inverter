/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#include "em/buffer.h"

#include <stdbool.h>
#include <string.h>

void buffer_clean(buffer_t *ptr)
{
  ptr->len = 0;
}

void buffer_pop_front(buffer_t *ptr, uint16_t num)
{
  if (ptr->len <= num) {
    ptr->len = 0;
    return;
  }

  memcpy(ptr->data, ptr->data + num, ptr->len - num);
  ptr->len -= num;
}

void buffer_push(buffer_t *ptr, uint8_t byte)
{
  ptr->data[ptr->len++] = byte;
}

void buffer_dynamic_alloc(buffer_t *ptr, uint16_t cap)
{
  if (ptr->data != NULL && ptr->dynamic) {
    free(ptr->data);
  }

  assert(cap < 1200);

  ptr->data = malloc(cap);
  assert(ptr->data);
  ptr->cap = cap;
  ptr->len = 0;
  ptr->dynamic = true;
}

void em_buffer_free(buffer_t *ptr)
{
  if (ptr->data != NULL && ptr->dynamic) {
    free(ptr->data);
  }

  ptr->data = NULL;
  ptr->cap = 0;
  ptr->len = 0;
}
