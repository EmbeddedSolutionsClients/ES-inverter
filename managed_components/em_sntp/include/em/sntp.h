/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef SNTP_H_
#define SNTP_H_

#include <stdbool.h>

void sntp_datetime_init(void (*on_sync_cb)(void));
bool sntp_is_time_in_sync(void);

#endif /* SNTP_H_*/
