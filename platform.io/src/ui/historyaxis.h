/*
 * Rad Pro
 * UI history axis
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(UI_HISTORYAXIS_H)
#define UI_HISTORYAXIS_H

#include <stdbool.h>
#include <stdint.h>

#define HISTORY_AXIS_LINE_MAX 12

typedef struct
{
    float bottom;
    float top;
    uint32_t lineNum;
    float lineValues[HISTORY_AXIS_LINE_MAX];
} HistoryAxis;

float getHistoryValueFromLog(uint8_t logValue, float scale);
bool buildHistoryAxis(float minValue, float maxValue, float warningValue, int32_t height, HistoryAxis *axis);
int32_t getHistoryAxisY(const HistoryAxis *axis, float value, int32_t height);
int32_t getHistoryAxisPrefixExponent(float top, int32_t minExponent);
bool strcatHistoryAxisLabel(char *s, float value, int32_t prefixExponent);

#endif
