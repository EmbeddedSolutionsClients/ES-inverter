/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_DATASET_H_
#define EM_DATASET_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef struct {
  size_t item_size; // Size of the one item
  size_t items_cap; // Number of items in the buffer
  size_t items_now; // Number of items currently in the buffer
  uint8_t *buffer;
} dataset_t;

int em_dataset_init(dataset_t *ds, void *buf, size_t item_size, size_t items_cap);
int em_dataset_add(dataset_t *ds, const void *item);
uint64_t em_dataset_avg_without_extreme(const dataset_t *ds);
uint64_t em_dataset_sum(const dataset_t *ds);
size_t em_dataset_items_cnt(const dataset_t *ds);
void em_dataset_clear(dataset_t *ds);

#endif /* EM_DATASET_H_ */
