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
    int32_t lineIndices[HISTORY_AXIS_LINE_MAX];
    float lineValues[HISTORY_AXIS_LINE_MAX];
} HistoryAxis;

float getHistoryValueFromLog(uint8_t logValue, float scale);
bool buildHistoryAxis(float minValue, float maxValue, float warningValue, int32_t height, HistoryAxis *axis);
int32_t getHistoryAxisY(const HistoryAxis *axis, float value, int32_t height);
int32_t getHistoryAxisIndexPrefixExponent(int32_t index, int32_t minExponent);
bool strcatHistoryAxisIndexLabel(char *s, int32_t index, int32_t prefixExponent);

#endif
