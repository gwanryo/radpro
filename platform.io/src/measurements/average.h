/*
 * Rad Pro
 * Average rate
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(AVERAGE_H)
#define AVERAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "../measurements/pulses.h"
#include "../ui/menu.h"
#include "../ui/view.h"

extern const Menu averageMenu;

void resetAverageRate(void);

void updateAverageRate(PulsePeriod *period);

float getAverageRate(void);

#if defined(FS5000)
float getAverageRateConfidence(void);

uint32_t getAverageRateTime(void);

bool isAverageRateDone(void);

void clearAverageRateValues(void);
#endif

void onAverageRateViewEvent(ViewEvent event);

#endif
