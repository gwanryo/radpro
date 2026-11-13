/*
 * Rad Pro
 * Cumulative dose start time
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(DOSESTART_H) && defined(FS5000)
#define DOSESTART_H

#include <stdbool.h>
#include <stdint.h>

uint32_t getUpdatedDoseStartTime(uint32_t startTime,
                                 bool *provisional,
                                 uint32_t previousTime,
                                 uint32_t newTime,
                                 bool newTimeValid);

#endif
