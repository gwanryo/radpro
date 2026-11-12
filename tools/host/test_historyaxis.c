// SOURCES: ui/historyaxis.c
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ui/historyaxis.h"

static bool feq(float a, float b)
{
    return fabsf(a - b) <= 1e-4f * fabsf(b);
}

static void expect_lines(const HistoryAxis *a, const float *v, uint32_t n)
{
    if (a->lineNum != n)
    {
        printf("lineNum %u != %u\n", (unsigned)a->lineNum, (unsigned)n);
        for (uint32_t i = 0; i < a->lineNum; i++)
            printf("  %g\n", a->lineValues[i]);
    }
    assert(a->lineNum == n);
    for (uint32_t i = 0; i < n; i++)
    {
        if (!feq(a->lineValues[i], v[i]))
            printf("line %u: %g != %g\n", (unsigned)i, a->lineValues[i], v[i]);
        assert(feq(a->lineValues[i], v[i]));
    }
    assert(feq(a->bottom, v[0]));
    assert(feq(a->top, v[n - 1]));
}

static void label_is(float value, int32_t exponent, const char *expected)
{
    char s[16] = "";
    assert(strcatHistoryAxisLabel(s, value, exponent));
    if (strcmp(s, expected))
        printf("label '%s' != '%s'\n", s, expected);
    assert(!strcmp(s, expected));
}

int main(void)
{
    HistoryAxis a;

    assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 0.5e-6f, 176, &a));
    expect_lines(&a, (float[]){0.05e-6f, 0.1e-6f, 0.2e-6f, 0.5e-6f, 1e-6f}, 5);
    assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 10e-6f, 176, &a));
    expect_lines(&a, (float[]){0.05e-6f, 0.1e-6f, 0.2e-6f, 0.5e-6f}, 4);
    assert(buildHistoryAxis(0.1e-6f, 50e-6f, 0, 176, &a));
    expect_lines(&a, (float[]){0.1e-6f, 1e-6f, 10e-6f, 100e-6f}, 4);
    assert(buildHistoryAxis(0.128e-6f, 0.128e-6f, 0, 176, &a));
    expect_lines(&a, (float[]){0.1e-6f, 0.2e-6f}, 2);
    assert(buildHistoryAxis(0.05f, 0.9f, 0, 60, &a));
    expect_lines(&a, (float[]){0.05f, 2.0f}, 2);
    assert(!buildHistoryAxis(0, 0, 0, 176, &a));

    assert(getHistoryAxisY(&(HistoryAxis){0.05f, 1.0f, 0, {0}}, 1.0f, 176) == 0);
    assert(getHistoryAxisY(&(HistoryAxis){0.05f, 1.0f, 0, {0}}, 0.05f, 176) == 175);

    assert(getHistoryValueFromLog(0, 3.0f) == 0);
    assert(feq(getHistoryValueFromLog(40, 3.0f), 0.6f));

    assert(getHistoryAxisPrefixExponent(1e-6f, -9) == -6);
    assert(getHistoryAxisPrefixExponent(2000.0f, 0) == 3);
    assert(getHistoryAxisPrefixExponent(0.5f, 0) == 0);
    label_is(0.05e-6f, -6, "0.05");
    label_is(1e-6f, -6, "1");
    label_is(100e-6f, -6, "100");
    label_is(0.5f, 0, "0.5");
    char s[16] = "";
    assert(!strcatHistoryAxisLabel(s, 0.0005f, 0) && s[0] == 0);

    return 0;
}
