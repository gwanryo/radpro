/*
 * Rad Pro
 * UI overview
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(UI_OVERVIEW_H)
#define UI_OVERVIEW_H

#include <stdint.h>

#include "../ui/measurements.h"

#if defined(FS5000)

typedef struct
{
    const char *key;
    char value[16];
    char unit[16];
    MeasurementStyle style;
} OverviewRow;

typedef struct
{
    char value[16];
    char unit[16];
    char confidence[16];
    MeasurementStyle style;
    const char *faultString;
    OverviewRow rows[4];
    char startTime[16];
    char elapsedTime[16];
} OverviewPage1;

void drawOverviewPage1(const OverviewPage1 *page, void (*drawBar)(void));
void drawOverviewPageDots(uint32_t pageIndex, uint32_t pageNum, int16_t top);
void drawMeasurementCompactBar(float value, int32_t minExponent, float warningValue, float alarmValue);

#endif

#endif
