/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef SLIP_H_
#define SLIP_H_

#include <stdint.h>
#include <esp_err.h>

esp_err_t slip_static_data_decoder(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out);
esp_err_t slip_static_data_encoder(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out);

#endif /* SLIP_H_ */
