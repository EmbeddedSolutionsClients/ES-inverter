/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/dataset.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int em_dataset_init(dataset_t *ds, void *buf, size_t item_size, size_t items_cap)
{
  assert(buf != NULL);
  assert(item_size > 0);
  assert(items_cap > 0);
  assert(ds != NULL);

  if (item_size != sizeof(uint8_t) && item_size != sizeof(uint16_t) && item_size != sizeof(uint32_t) &&
      item_size != sizeof(uint64_t)) {
    return -1;
  }

  ds->item_size = item_size;
  ds->items_cap = items_cap;
  ds->items_now = 0;
  ds->buffer = (uint8_t *)buf;
  memset(ds->buffer, 0, ds->item_size * ds->items_cap);
  return 0;
}

int em_dataset_add(dataset_t *ds, const void *item)
{
  assert(ds != NULL);
  assert(item != NULL);

  if (ds->items_now >= ds->items_cap) {
    return -1;
  }

  size_t write_offset = (ptrdiff_t)ds->buffer + ds->items_now * ds->item_size;
  memcpy((void *)write_offset, item, ds->item_size);
  ds->items_now++;

  return 0;
}

uint64_t em_dataset_avg_without_extreme(const dataset_t *ds)
{
  if (ds == NULL) {
    return -1;
  }

  if (ds->items_now < 3) {
    return -2;
  }

  uint64_t min_item = UINT64_MAX;
  uint64_t max_item = 0;
  uint64_t sum = 0;

  for (size_t i = 0; i < ds->items_now; i++) {
    void *current_item = ds->buffer + i * ds->item_size;

    switch (ds->item_size) {
    case sizeof(uint8_t): {
      uint8_t val = *(uint8_t *)current_item;
      if (val < min_item) {
        min_item = val;
      }

      if (val > max_item) {
        max_item = val;
      }
      sum += val;
      break;
    }

    case sizeof(uint16_t): {
      uint16_t val = *(uint16_t *)current_item;
      if (val < min_item) {
        min_item = val;
      }

      if (val > max_item) {
        max_item = val;
      }
      sum += val;
      break;
    }

    case sizeof(uint32_t): {
      uint32_t val = *(uint32_t *)current_item;
      if (val < min_item) {
        min_item = val;
      }

      if (val > max_item) {
        max_item = val;
      }
      sum += val;
      break;
    }

    case sizeof(uint64_t): {
      uint64_t val = *(uint64_t *)current_item;
      if (val < min_item) {
        min_item = val;
      }

      if (val > max_item) {
        max_item = val;
      }
      sum += val;
      break;
    }

    default:
      return -3;
    }
  }

  return (sum - min_item - max_item) / (ds->items_now - 2); // Exclude min and max from the average
}

uint64_t em_dataset_sum(const dataset_t *ds)
{
  uint64_t sum = 0;

  for (size_t i = 0; i < ds->items_now; i++) {
    void *current_item = ds->buffer + i * ds->item_size;

    switch (ds->item_size) {
    case sizeof(uint8_t): {
      sum += *(uint8_t *)current_item;
      break;
    }

    case sizeof(uint16_t): {
      sum += *(uint16_t *)current_item;
      break;
    }

    case sizeof(uint32_t): {
      sum += *(uint32_t *)current_item;
      break;
    }

    case sizeof(uint64_t): {
      sum += *(uint64_t *)current_item;
      break;
    }

    default:
      return 0;
    }
  }

  return sum;
}

size_t em_dataset_items_cnt(const dataset_t *ds)
{
  return ds->items_now;
}

void em_dataset_clear(dataset_t *ds)
{
  assert(ds != NULL);

  ds->items_now = 0;
}
