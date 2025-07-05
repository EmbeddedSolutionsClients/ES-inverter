/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "em/messages.h"
#include <esp_err.h>

esp_err_t dispatcher_handler(msg_type_t type, void *data);

#endif /* DISPATCHER_H_ */
