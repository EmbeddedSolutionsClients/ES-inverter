/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EM_MATH_H_
#define EM_MATH_H_

#include <math.h>
#include <stdint.h>

#define DEG_TO_RAD(x) (x * M_PI / 180)
#define RAD_TO_DEG(x) (x * 180 / M_PI)

int32_t em_utils_pow10(int32_t value, int8_t factor);
void em_utils_decode_scaler(int8_t scaler, int32_t *multiplier, int32_t *divisor);

#endif /* EM_MATH_H_ */
