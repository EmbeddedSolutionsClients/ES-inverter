/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#ifndef EM_PROVISIONING_SVC_H_
#define EM_PROVISIONING_SVC_H_

#include <stdint.h>

void em_ble_prov_init(const char *dev_name, uint64_t id);
void em_ble_prov_deinit(void);
int em_ble_prov_start(uint32_t disconn_delay);
void em_ble_prov_stop(void);

#endif /* EM_PROVISIONING_SVC_H_ */
