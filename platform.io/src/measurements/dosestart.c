/*
 * Rad Pro
 * Cumulative dose start time
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include "../measurements/dosestart.h"

#if defined(FS5000)
uint32_t getUpdatedDoseStartTime(uint32_t startTime,
                                 bool *provisional,
                                 uint32_t previousTime,
                                 uint32_t newTime,
                                 bool newTimeValid)
{
    if (*provisional)
        return startTime + (newTime - previousTime);

    if (!startTime && newTimeValid)
    {
        *provisional = true;

        return newTime;
    }

    return startTime;
}
#endif
