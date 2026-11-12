/*
 * Rad Pro
 * Instantaneous rate
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(INSTANTANEOUS_H)
#define INSTANTANEOUS_H

#include <stdbool.h>

#include "../measurements/cumulative.h"
#include "../measurements/pulses.h"
#include "../ui/measurements.h"
#include "../ui/menu.h"
#include "../ui/view.h"

extern const Menu instantaneousMenu;

void resetInstantaneousRate(void);

void updateInstantaneousRate(uint32_t periodTick, PulsePeriod *period);

float getDeadTimeCompensationFactor(void);

float getInstantaneousRate(void);

#if defined(FS5000)
float getInstantaneousRateConfidence(void);

float getInstantaneousMaxRate(void);

MeasurementStyle getInstantaneousRateStyle(void);

void clearInstantaneousRateValues(void);

void drawInstantaneousRateBar(void (*drawBar)(float value, int32_t minExponent, float warningValue, float alarmValue));
#endif

bool isInstantaneousRateConfidenceGood(void);

AlertLevel getInstantaneousRateAlertLevel(void);

void onInstantaneousRateViewEvent(ViewEvent event);
#if defined(DISPLAY_240X320)
void onOverviewViewEvent(ViewEvent event);
#endif

#endif
