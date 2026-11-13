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

typedef struct
{
    const uint8_t *logValues;
    float scale;
    const char *unitName;
    int32_t minPrefixExponent;
    float warningValue;
    float alarmValue;
    uint32_t timeTicksNum;
    const char *periodString;
    const char *halfString;
} OverviewGraph;

typedef struct
{
    char instantaneousKey[24];
    char instantaneousValue[16];
    MeasurementStyle instantaneousStyle;
    char averageKey[24];
    char averageValue[16];
    MeasurementStyle averageStyle;
    char secondaryKey[16];
    char secondaryValue[16];
    char averageTime[16];
    char averageConfidence[16];
    const char *faultString;
} OverviewPage2;

void drawOverviewPage1(const OverviewPage1 *page, void (*drawBar)(void));
void drawOverviewPage2(const OverviewPage2 *page, const OverviewGraph *graph);
void drawOverviewPageDots(uint32_t pageIndex, uint32_t pageNum, int16_t top);
void drawMeasurementCompactBar(float value, int32_t minExponent, float warningValue, float alarmValue);

#endif

#endif
