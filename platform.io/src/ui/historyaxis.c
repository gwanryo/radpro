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

#include "ui/history.h"
#include "ui/historyaxis.h"

#define AXIS_EPSILON 1e-6F
#define AXIS_EXPONENT_EPSILON 1e-5F
#define AXIS_WIDE_RATIO 31.6227766F
#define AXIS_LINE_SPACING 18

static float getAxisValue(int32_t index)
{
    static const float mantissa[] = {1, 2, 5};

    int32_t decade = index / 3;
    int32_t remainder = index % 3;
    if (remainder < 0)
    {
        remainder += 3;
        decade--;
    }

    return mantissa[remainder] * powf(10.0F, (float)decade);
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

    float candidates[HISTORY_AXIS_LINE_MAX];
    uint32_t candidateNum = 0;
    candidates[candidateNum++] = axis->bottom;
    for (int32_t index = bottomIndex + 1;
         index < topIndex && candidateNum < HISTORY_AXIS_LINE_MAX - 1;
         index++)
        if (all || (index % 3) == 0)
            candidates[candidateNum++] = getAxisValue(index);
    candidates[candidateNum++] = axis->top;

    int32_t topY = getHistoryAxisY(axis, axis->top, height);
    int32_t lastY = getHistoryAxisY(axis, axis->bottom, height);

    axis->lineNum = 0;
    axis->lineValues[axis->lineNum++] = candidates[0];
    for (uint32_t i = 1; i < candidateNum - 1; i++)
    {
        int32_t y = getHistoryAxisY(axis, candidates[i], height);
        if (lastY - y >= AXIS_LINE_SPACING &&
            y - topY >= AXIS_LINE_SPACING)
        {
            axis->lineValues[axis->lineNum++] = candidates[i];
            lastY = y;
        }
    }
    axis->lineValues[axis->lineNum++] = candidates[candidateNum - 1];

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

int32_t getHistoryAxisPrefixExponent(float top, int32_t minExponent)
{
    int32_t exponent = 3 * (int32_t)floorf(log10f(top) / 3 + AXIS_EXPONENT_EPSILON);

    return (exponent > minExponent) ? exponent : minExponent;
}

bool strcatHistoryAxisLabel(char *s, float value, int32_t prefixExponent)
{
    if (value < 0)
        return false;

    float scaled = value / powf(10.0F, (float)prefixExponent);

    uint32_t scale = 1;
    uint32_t digits = 0;
    uint32_t number = 0;
    bool found = false;
    for (digits = 0; digits <= 3; digits++)
    {
        float product = scaled * (float)scale;
        float rounded = floorf(product + 0.5F);
        bool match = (rounded > 0)
                         ? (fabsf(product - rounded) <= AXIS_EPSILON * rounded)
                         : (product == 0);
        if (match &&
            rounded < 1e9F)
        {
            number = (uint32_t)rounded;
            found = true;
            break;
        }
        scale *= 10;
    }
    if (!found)
        return false;

    char buffer[16];
    uint32_t length = 0;
    uint32_t integer = number / scale;
    uint32_t fraction = number % scale;

    char reversed[10];
    uint32_t reversedLength = 0;
    do
    {
        reversed[reversedLength++] = '0' + (integer % 10);
        integer /= 10;
    } while (integer);
    while (reversedLength)
        buffer[length++] = reversed[--reversedLength];

    if (digits)
    {
        buffer[length++] = '.';
        for (uint32_t i = digits; i > 0; i--)
        {
            scale /= 10;
            buffer[length++] = '0' + (fraction / scale) % 10;
        }
    }
    buffer[length] = '\0';

    strcat(s, buffer);

    return true;
}
