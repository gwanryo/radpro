/*
 * Rad Pro
 * UI history axis
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include <math.h>
#include <string.h>

#include "../ui/history.h"
#include "../ui/historyaxis.h"

#define AXIS_EPSILON 2e-5F
#define AXIS_WIDE_RATIO 31.6227766F
#define AXIS_LINE_SPACING 18
#define AXIS_LABEL_DIGITS_MAX 3
#define AXIS_LABEL_ZEROS_MAX 8

static const uint8_t axisMantissas[] = {1, 2, 5};

static void getAxisIndexParts(int32_t index, int32_t *decade, int32_t *remainder)
{
    *decade = index / 3;
    *remainder = index % 3;
    if (*remainder < 0)
    {
        *remainder += 3;
        (*decade)--;
    }
}

static float getAxisValue(int32_t index)
{
    int32_t decade;
    int32_t remainder;
    getAxisIndexParts(index, &decade, &remainder);

    float power = 1.0F;
    for (int32_t i = (decade < 0) ? -decade : decade; i > 0; i--)
        power *= 10.0F;

    float mantissa = axisMantissas[remainder];

    return (decade < 0) ? mantissa / power : mantissa * power;
}

static int32_t getAxisIndexFloor(float value)
{
    float limit = value * (1.0F + AXIS_EPSILON);

    int32_t index = 3 * (int32_t)floorf(log10f(value)) - 3;
    while (getAxisValue(index + 1) <= limit)
        index++;

    return index;
}

float getHistoryValueFromLog(uint8_t logValue, float scale)
{
    if (logValue == 0)
        return 0;

    return scale * HISTORY_VALUE_MIN * powf(10.0F, (float)logValue / HISTORY_DECADE);
}

bool buildHistoryAxis(float minValue, float maxValue, float warningValue, int32_t height, HistoryAxis *axis)
{
    if (maxValue <= 0)
        return false;

    if (minValue <= 0 || minValue > maxValue)
        minValue = maxValue;

    float t = maxValue * 1.2F;
    if (warningValue > 0 &&
        warningValue <= 2 * maxValue &&
        warningValue > t)
        t = warningValue;

    int32_t topIndex = getAxisIndexFloor(t) + 1;
    int32_t bottomIndex = getAxisIndexFloor(minValue);

    axis->bottom = getAxisValue(bottomIndex);
    axis->top = getAxisValue(topIndex);

    bool all = (axis->top / axis->bottom <= AXIS_WIDE_RATIO * (1.0F + AXIS_EPSILON));

    int32_t candidates[HISTORY_AXIS_LINE_MAX];
    uint32_t candidateNum = 0;
    candidates[candidateNum++] = bottomIndex;
    for (int32_t index = bottomIndex + 1;
         index < topIndex && candidateNum < HISTORY_AXIS_LINE_MAX - 1;
         index++)
        if (all || (index % 3) == 0)
            candidates[candidateNum++] = index;
    candidates[candidateNum++] = topIndex;

    int32_t topY = getHistoryAxisY(axis, axis->top, height);
    int32_t lastY = getHistoryAxisY(axis, axis->bottom, height);

    axis->lineNum = 0;
    axis->lineIndices[axis->lineNum++] = candidates[0];
    for (uint32_t i = 1; i < candidateNum - 1; i++)
    {
        int32_t y = getHistoryAxisY(axis, getAxisValue(candidates[i]), height);
        if (lastY - y >= AXIS_LINE_SPACING &&
            y - topY >= AXIS_LINE_SPACING)
        {
            axis->lineIndices[axis->lineNum++] = candidates[i];
            lastY = y;
        }
    }
    axis->lineIndices[axis->lineNum++] = candidates[candidateNum - 1];

    for (uint32_t i = 0; i < axis->lineNum; i++)
        axis->lineValues[i] = getAxisValue(axis->lineIndices[i]);

    return true;
}

int32_t getHistoryAxisY(const HistoryAxis *axis, float value, int32_t height)
{
    int32_t y = height - 1;
    if (value > 0)
    {
        float position = 1.0F - logf(value / axis->bottom) / logf(axis->top / axis->bottom);

        y = (int32_t)floorf(position * (float)(height - 1) + 0.5F);
    }

    if (y < 0)
        y = 0;
    else if (y > height - 1)
        y = height - 1;

    return y;
}

int32_t getHistoryAxisIndexPrefixExponent(int32_t index, int32_t minExponent)
{
    int32_t decade;
    int32_t remainder;
    getAxisIndexParts(index, &decade, &remainder);

    int32_t exponent = decade - ((decade % 3) + 3) % 3;

    return (exponent > minExponent) ? exponent : minExponent;
}

bool strcatHistoryAxisIndexLabel(char *s, int32_t index, int32_t prefixExponent)
{
    int32_t decade;
    int32_t remainder;
    getAxisIndexParts(index, &decade, &remainder);

    int32_t shift = decade - prefixExponent;
    if ((shift < -AXIS_LABEL_DIGITS_MAX) ||
        (shift > AXIS_LABEL_ZEROS_MAX))
        return false;

    char buffer[16];
    uint32_t length = 0;
    if (shift < 0)
    {
        buffer[length++] = '0';
        buffer[length++] = '.';
        for (int32_t i = shift + 1; i < 0; i++)
            buffer[length++] = '0';
    }
    buffer[length++] = '0' + axisMantissas[remainder];
    for (int32_t i = 0; i < shift; i++)
        buffer[length++] = '0';
    buffer[length] = '\0';

    strcat(s, buffer);

    return true;
}
