/*
 * Rad Pro
 * Cumulative dose
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(CUMULATIVE_H)
#define CUMULATIVE_H

#include <stdbool.h>

#include "../measurements/pulses.h"
#if defined(FS5000)
#include "../ui/measurements.h"
#include "../ui/menu.h"
#endif
#include "../ui/view.h"

void resetCumulativeDose(void);

void updateCumulativeDose(PulsePeriod *period);

void setCumulativeDoseTime(uint32_t value);
uint32_t getCumulativeDoseTime(void);
void setCumulativeDosePulseCount(uint32_t value);
uint32_t getCumulativeDosePulseCount(void);

#if defined(FS5000)
MeasurementStyle getCumulativeDoseStyle(void);

uint32_t getCumulativeDoseStartTime(void);
void setCumulativeDoseStartTime(uint32_t value);

void seedCumulativeDoseStartTime(void);
void updateCumulativeDoseStartTime(uint32_t previousDeviceTime);

void clearCumulativeDoseValues(void);

extern const Menu cumulativeDoseMenu;
#endif

AlertLevel getCumulativeDoseAlertLevel(void);
bool isCumulativeDoseAlertTriggered(void);

void onCumulativeDoseViewEvent(ViewEvent viewEvent);

#endif
