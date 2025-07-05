/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SECONDS_IN_MINUTE  (60U)
#define SECONDS_IN_QUARTER (SECONDS_IN_MINUTE * 15U)
#define SECONDS_IN_MINUTE  (60U)
#define SECONDS_IN_QUARTER (SECONDS_IN_MINUTE * 15U)
#define SECONDS_IN_HOUR    (SECONDS_IN_MINUTE * 60U)
#define SECONDS_IN_DAY     (SECONDS_IN_HOUR * 24U)
#define SECONDS_IN_WEEK    (SECONDS_IN_DAY * 7)
#define MIN_TO_SEC(x)      (x * SECONDS_IN_MINUTE)
#define MIN_TO_MSEC(x)     (MIN_TO_SEC(x) * 1000U)
#define MSEC_TO_SEC(x)     (x / 1000)
#define HOURS_IN_DAY       (24)

// clang-format off
#define CUSTOM_DATA {468, 420, 406, 392, 394, 395, 370, 389, 366, 352, 342, 346, 346, 345, 324, 269, 353, 467, 512, 492, 583, 556, 505, 454}
// clang-format on

typedef struct {
  uint8_t state;
  bool oneshot_enabled;
  uint8_t mode;
  int8_t price_factor;
  int32_t threshold; // scaled, ie. threshold = X * 10^price_factor
} snapshot_t;

typedef struct {
  /* checksum has to be the first member */
  uint32_t checksum;
  time_t day;
  int8_t price_factor;
  int16_t prices[96];
  uint16_t tariff;
} __attribute__((packed)) energyprice_ds_t;

struct sort_price_data_s {
  int16_t price;
  uint16_t idx;
};

static int sort_prices_ascending(const void *a, const void *b)
{
  const struct sort_price_data_s *da = (const struct sort_price_data_s *)a;
  const struct sort_price_data_s *db = (const struct sort_price_data_s *)b;

  if (da->price != db->price) {
    return (da->price - db->price);
  }

  /* Ensure stable sorting by comparing indices when prices are equal */
  return (da->idx - db->idx);
}

uint32_t em_utils_time_diff_ms(time_t to, time_t from)
{
  double diff_sec = difftime(to, from);
  uint32_t diff_ms = (uint32_t)((int64_t)diff_sec * 1000);
  return diff_ms;
}

static uint32_t quarter_in_day(const time_t now)
{
  struct tm tm_midnight;
  localtime_r(&now, &tm_midnight);

  tm_midnight.tm_hour = 0;
  tm_midnight.tm_min = 0;
  tm_midnight.tm_sec = 0;
  const time_t midnight_time = mktime(&tm_midnight);

  double sec_elapsed = MSEC_TO_SEC(em_utils_time_diff_ms(now, midnight_time));

  /* Single IDX represents 15 minutes */
  uint32_t idx = sec_elapsed / SECONDS_IN_QUARTER;
  const time_t start = midnight_time + (idx * SECONDS_IN_QUARTER);

  // em_utils_print_datetime(start, "Current Energy Price entry start");
  return idx;
}

int apply_price_cheap_period(int q_now, uint8_t cheap_quarters, const energyprice_ds_t *const energy_price, uint8_t *state, uint32_t *th)
{
  *state = 0;

  if (cheap_quarters == 0) {
    /* There is no quarter to search for */
    *state = 0;
    return 3;
  }

  struct sort_price_data_s sorted_prices[96] = {
    0,
  };

  for (uint32_t i = 0; i < 96; i++) {
    sorted_prices[i].price = energy_price->prices[i];
    sorted_prices[i].idx = i;
  }

  qsort(&sorted_prices, 96, sizeof(struct sort_price_data_s), sort_prices_ascending);

  int16_t threshold = sorted_prices[cheap_quarters - 1].price;
  *th = threshold;

  *state = 0;

  // for (uint32_t q = q_now; q < 96; q++) {
  //  go further if above the 96
  if (energy_price->prices[q_now] > threshold) {
    return 444;
  }

  // if below the threshold for sure turn it on
  if (energy_price->prices[q_now] < threshold) {
    *state = 1;
    return 333;
  }

  size_t equal_on_quarters = 0;
  size_t below_threshold_quarters = 0;

  for (size_t i = 0; i < 96; i++) {
    if (energy_price->prices[i] < threshold) {
      below_threshold_quarters++;
    }
  }

  equal_on_quarters = cheap_quarters - below_threshold_quarters;
  size_t remaining = equal_on_quarters;

  size_t last_idx_on_asc = UINT32_MAX;
  size_t last_idx_on_desc = UINT32_MAX;

  for (size_t q = 0; remaining > 0 && q < 96; ++q) {
    bool prev_on = true;
    bool next_on = true;

    for (int i = 0; i < 96; i++) {
      if (energy_price->prices[i] != threshold || i == last_idx_on_asc || i == last_idx_on_desc) {
        continue;
      }

      if (i > 0) {
        prev_on = energy_price->prices[i - 1] < threshold || last_idx_on_asc == i - 1;
      }

      if (i < 95) {
        next_on = energy_price->prices[i + 1] < threshold || last_idx_on_desc == i + 1;
      }

      if (next_on || prev_on) {
        if (i == q_now) {
          *state = 1;
          return 333;
        }

        --remaining;
        last_idx_on_asc = i;

        if (remaining == 0) {
          return 555;
        }
      }
    }

    prev_on = true;
    next_on = true;

    for (int k = 95; k >= 0; k--) {
      if (energy_price->prices[k] != threshold || k == last_idx_on_asc || k == last_idx_on_desc) {
        continue;
      }

      if (k > 0) {
        size_t next_idx = k - 1;
        next_on = energy_price->prices[next_idx] < threshold || last_idx_on_desc == next_idx || last_idx_on_asc == next_idx;
      }

      if (k < 95) {
        size_t prev_idx = k + 1;
        prev_on = energy_price->prices[prev_idx] < threshold || last_idx_on_desc == prev_idx || last_idx_on_asc == prev_idx;
      }

      if (next_on || prev_on || (equal_on_quarters - q) <= remaining) {
        if (k == q_now) {
          *state = 1;
          return 333;
        }

        --remaining;
        last_idx_on_desc = k;

        if (remaining == 0) {
          return 555;
        }
      }
    }
  }

  return 444444;
}

int test(energyprice_ds_t ed)
{
  printf("\n\nPrices: ");

  for (int i = 0; i < 96; i++) {
    printf("%d ", ed.prices[i]);
  }
  printf("\n");

  uint8_t state = 0;

  for (int q = 0; q < 96; q++) {
    int32_t threshold;

    int check = 0;

    uint8_t tab[96] = {0};

    for (int i = 0; i < 96; i++) {
      apply_price_cheap_period(i, q, &ed, &state, &threshold);
      if (state) {
        tab[i] = 1;
        check++;
      }
    }

    printf("%02dq: ", q);

    if (check != q) {
      printf(" th=%d check=%d ? %d\n", threshold, check, q);
    }

    // printf(" th=%d check=%d ? %d\n", threshold, check, q);
    for (int i = 0; i < 96; i++) {
      if (tab[i]) {
        printf("| ");
      } else {
        printf(". ");
      }
    }
    printf(" th=%d\n", threshold);
    // }
  }
}

int main()
{
  printf("Hello World\n");

  energyprice_ds_t ed = {.day = 0, .price_factor = 0, .prices = {0}, .tariff = 0};

  for (int i = 0; i < 96; i++) {
    ed.prices[i] = (i + 8) / 4;
    if (i < 48) {
      ed.prices[i] = (i + 8) / 4;
    } else {
      ed.prices[i] = (100 - i) / 8;
    }
  }

  test(ed);

  srand(time(NULL));

  for (int i = 0; i < 96; i += 4) {
    ed.prices[i] = rand() % 1000;
    ed.prices[i + 1] = ed.prices[i];
    ed.prices[i + 2] = ed.prices[i];
    ed.prices[i + 3] = ed.prices[i];
  }

  test(ed);

  int16_t custom_data[] = CUSTOM_DATA;

  for (int i = 0; i < 96 / 4; i++) {
    ed.prices[4 * i] = custom_data[i];
    ed.prices[4 * i + 1] = custom_data[i];
    ed.prices[4 * i + 2] = custom_data[i];
    ed.prices[4 * i + 3] = custom_data[i];
  }

  test(ed);

  return 0;
}