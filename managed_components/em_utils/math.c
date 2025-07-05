/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include <math.h>
#include <stdint.h>

int32_t em_utils_pow10(int32_t value, int8_t factor)
{
  if (factor == 0) {
    return value;
  }

  int32_t scaler = 1;
  uint8_t abs_factor = factor < 0 ? -factor : factor;

  for (int8_t i = 0; i < abs_factor; i++) {
    scaler *= 10;
  }

  if (factor < 0) {
    scaler = -scaler;
  }

  return value * scaler;
}

void em_utils_decode_scaler(int8_t scaler, int32_t *multiplier, int32_t *divisor)
{
  assert(multiplier);
  assert(divisor);

  *divisor = 1;
  *multiplier = 1;

  if (scaler == 0) {
    return;
  }

  for (uint32_t i = 0; i < (uint8_t)abs(scaler); i++) {
    if (scaler > 0) {
      *multiplier *= 10;
    } else {
      *divisor *= 10;
    }
  }
}
