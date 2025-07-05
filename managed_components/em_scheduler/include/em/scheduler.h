/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

#define SCH_NO_DELAY   (0UL)
#define SCH_PARAM_NONE (0UL)
#define SCH_CTX_NONE   (NULL)

/**
 * @typedef app_callback_t
 * @brief Callback type for application scheduling.
 *
 * This type defines a function pointer for callbacks scheduled by the application.
 *
 * @param param The parameter to be passed to the callback.
 * @param user_ctx The user context data to be passed to the callback.
 */
typedef void (*app_callback_t)(uint32_t param, void *user_ctx);

/**
 * @brief Initializes the application scheduler functionality.
 *
 * This function initializes the scheduler, preparing it to handle
 * application callback scheduling. The function aborts on any error.
 */
void scheduler_init(void);

/**
 * @brief Schedules an application callback.
 *
 * This function schedules a callback to be called after the specified delay.
 * If the same callback is already scheduled, the function reschedules it with
 * the new delay. The function aborts on any error.
 *
 * @param cb The callback function to be scheduled. Callback can schedule itself to create periodicity.
 * @param param The parameter to be passed to the callback function.
 * @param user_ctx The user context to be passed to the callback function.
 * @param delay_ms The delay in milliseconds after which the callback is to be called.
 */
void scheduler_set_callback(app_callback_t cb, uint32_t param, void *user_ctx, uint32_t delay_ms);

/**
 * @brief Cancels a scheduled callback.
 *
 * This function cancels a previously scheduled callback. The function aborts on any error.
 *
 * @param cb The callback function to be canceled.
 */
void scheduler_cancel_callback(app_callback_t cb);

#endif /* SCHEDULE_H_ */
