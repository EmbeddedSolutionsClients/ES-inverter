/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_TIME_H_
#define EM_TIME_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define SECONDS_IN_MINUTE  (60U)
#define SECONDS_IN_QUARTER (SECONDS_IN_MINUTE * 15U)
#define SECONDS_IN_HOUR    (SECONDS_IN_MINUTE * 60U)
#define SECONDS_IN_DAY     (SECONDS_IN_HOUR * 24U)
#define SECONDS_IN_WEEK    (SECONDS_IN_DAY * 7)
#define MIN_TO_SEC(x)      (x * SECONDS_IN_MINUTE)
#define MIN_TO_MSEC(x)     (MIN_TO_SEC(x) * 1000U)
#define MSEC_TO_SEC(x)     (x / 1000)
#define HOURS_IN_DAY       (24)

uint32_t em_utils_time_diff_ms(time_t to, time_t from);
void em_utils_print_datetime(time_t t, const char *text);
bool em_utils_is_leap_year(int32_t year);
time_t em_utils_get_compile_time(void);

#endif /* EM_TIME_H_ */
