#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float mcuError;
static int mcuAlternate;
static uint32_t mcuCalls;

static float mcuPerturb(float value)
{
    float error = mcuError;
    if (mcuAlternate && (mcuCalls++ & 1))
        error = -error;
    return value * (1.0f + error);
}

static float mcuPowf(float x, float y)
{
    return mcuPerturb(powf(x, y));
}

static float mcuLogf(float x)
{
    return mcuPerturb(logf(x));
}

static float mcuLog10f(float x)
{
    return mcuPerturb(log10f(x));
}

#define powf mcuPowf
#define logf mcuLogf
#define log10f mcuLog10f
#include "ui/historyaxis.c"
#undef powf
#undef logf
#undef log10f

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

static void label_is(int32_t index, int32_t exponent, const char *expected)
{
    char s[16] = "";
    assert(strcatHistoryAxisIndexLabel(s, index, exponent));
    if (strcmp(s, expected))
        printf("label '%s' != '%s'\n", s, expected);
    assert(!strcmp(s, expected));
}

static void expect_labels(const HistoryAxis *a, int32_t minExponent, int32_t expectedExponent, const char *const *expected, uint32_t n)
{
    int32_t exponent = getHistoryAxisIndexPrefixExponent(a->lineIndices[a->lineNum - 1], minExponent);
    assert(exponent == expectedExponent);
    assert(a->lineNum == n);
    for (uint32_t i = 0; i < n; i++)
        label_is(a->lineIndices[i], exponent, expected[i]);
}

typedef struct
{
    bool ok;
    HistoryAxis axis;
    int32_t ys[HISTORY_AXIS_LINE_MAX];
    char labels[HISTORY_AXIS_LINE_MAX][16];
} AxisResult;

static void fill_result(AxisResult *r, int32_t minExponent)
{
    if (!r->ok)
        return;
    int32_t exponent = getHistoryAxisIndexPrefixExponent(r->axis.lineIndices[r->axis.lineNum - 1], minExponent);
    for (uint32_t i = 0; i < r->axis.lineNum; i++)
    {
        r->ys[i] = getHistoryAxisY(&r->axis, r->axis.lineValues[i], 176);
        if (!strcatHistoryAxisIndexLabel(r->labels[i], r->axis.lineIndices[i], exponent))
            strcpy(r->labels[i], "-");
    }
}

static void build_result(uint8_t minLog, uint8_t maxLog, float scale, float warning, int32_t minExponent, AxisResult *r)
{
    mcuCalls = 0;
    memset(r, 0, sizeof(*r));
    r->ok = buildHistoryAxis(getHistoryValueFromLog(minLog, scale),
                             getHistoryValueFromLog(maxLog, scale),
                             warning, 176, &r->axis);
    fill_result(r, minExponent);
}

static bool same_result(const AxisResult *a, const AxisResult *b)
{
    if (a->ok != b->ok)
        return false;
    if (!a->ok)
        return true;
    if (a->axis.lineNum != b->axis.lineNum)
        return false;
    for (uint32_t i = 0; i < a->axis.lineNum; i++)
        if ((a->axis.lineIndices[i] != b->axis.lineIndices[i]) ||
            (abs(a->ys[i] - b->ys[i]) > 1) ||
            strcmp(a->labels[i], b->labels[i]))
            return false;
    return true;
}

static void check_perturbation(void)
{
    static const float scales[] = {1e-6f / 153.8f, 1e-4f / 153.8f, 1.0f, 1.0f / 60.0f};
    static const int32_t minExponents[] = {-9, -9, 0, 0};
    static const float warnings[] = {0, 0.5e-6f, 1e-6f, 2e-6f, 20e-6f, 1e-3f};
    static const float errors[] = {5e-6f, -5e-6f};
    uint32_t checked = 0;
    uint32_t mismatches = 0;

    for (uint32_t s = 0; s < sizeof(scales) / sizeof(scales[0]); s++)
        for (uint32_t w = 0; w < sizeof(warnings) / sizeof(warnings[0]); w++)
            for (uint32_t minLog = 1; minLog <= 255; minLog++)
                for (uint32_t maxLog = minLog; maxLog <= 255; maxLog++)
                {
                    float warning = warnings[w] * (scales[s] / scales[0]);
                    AxisResult reference;
                    mcuError = 0;
                    mcuAlternate = 0;
                    build_result(minLog, maxLog, scales[s], warning, minExponents[s], &reference);
                    for (uint32_t e = 0; e < 2; e++)
                        for (int alternate = 0; alternate < 2; alternate++)
                        {
                            AxisResult perturbed;
                            mcuError = errors[e];
                            mcuAlternate = alternate;
                            build_result(minLog, maxLog, scales[s], warning, minExponents[s], &perturbed);
                            checked++;
                            if (!same_result(&reference, &perturbed))
                            {
                                if (mismatches < 10)
                                    printf("perturbation mismatch: scale %u warning %g log %u..%u error %g alternate %d\n",
                                           (unsigned)s, warning, (unsigned)minLog, (unsigned)maxLog, errors[e], alternate);
                                mismatches++;
                            }
                        }
                }
    mcuError = 0;
    mcuAlternate = 0;
    printf("perturbation: %u checked, %u mismatches\n", (unsigned)checked, (unsigned)mismatches);
    fflush(stdout);
    assert(mismatches == 0);
}

static void check_input_perturbation(float minValue, float maxValue, float warning, int32_t minExponent)
{
    static const float factors[] = {1 - 5e-6f, 1 + 5e-6f};
    AxisResult reference;
    memset(&reference, 0, sizeof(reference));
    reference.ok = buildHistoryAxis(minValue, maxValue, warning, 176, &reference.axis);
    assert(reference.ok);
    fill_result(&reference, minExponent);
    for (uint32_t i = 0; i < 2; i++)
        for (uint32_t j = 0; j < 2; j++)
        {
            AxisResult perturbed;
            memset(&perturbed, 0, sizeof(perturbed));
            perturbed.ok = buildHistoryAxis(minValue * factors[i], maxValue * factors[j], warning, 176, &perturbed.axis);
            fill_result(&perturbed, minExponent);
            assert(same_result(&reference, &perturbed));
        }
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

    assert(getHistoryAxisIndexPrefixExponent(-18, -9) == -6);
    assert(getHistoryAxisIndexPrefixExponent(10, 0) == 3);
    assert(getHistoryAxisIndexPrefixExponent(-1, 0) == 0);
    label_is(-22, -6, "0.05");
    label_is(-18, -6, "1");
    label_is(-12, -6, "100");
    label_is(-1, 0, "0.5");
    char s[16] = "";
    assert(!strcatHistoryAxisIndexLabel(s, -10, 0) && s[0] == 0);
    assert(!strcatHistoryAxisIndexLabel(s, -12, 0) && s[0] == 0);
    assert(!strcatHistoryAxisIndexLabel(s, -13, 0) && s[0] == 0);
    label_is(-9, 0, "0.001");
    label_is(11, 0, "5000");

    assert(buildHistoryAxis(0, 0.128e-6f, 0, 176, &a));
    expect_lines(&a, (float[]){0.1e-6f, 0.2e-6f}, 2);
    assert(getHistoryAxisY(&(HistoryAxis){0.1f, 10.0f, 0, {0}}, 1.0f, 176) == 88);

    assert(buildHistoryAxis(0.99995f, 0.99995f, 0, 176, &a));
    assert(feq(a.bottom, 0.5f));
    assert(buildHistoryAxis(0.8f, 0.83329f, 0, 176, &a));
    assert(feq(a.top, 1.0f));
    assert(getHistoryAxisIndexPrefixExponent(-1, -9) == -3);
    assert(getHistoryAxisIndexPrefixExponent(-22, -9) == -9);

    assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 0.5e-6f, 176, &a));
    assert(a.lineValues[0] == 0.05e-6f && a.lineValues[1] == 0.1e-6f && a.lineValues[2] == 0.2e-6f &&
           a.lineValues[3] == 0.5e-6f && a.lineValues[4] == 1e-6f);
    expect_labels(&a, -9, -6, (const char *const[]){"0.05", "0.1", "0.2", "0.5", "1"}, 5);
    assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 10e-6f, 176, &a));
    expect_labels(&a, -9, -9, (const char *const[]){"50", "100", "200", "500"}, 4);
    assert(buildHistoryAxis(1.5e-3f, 12.47e-3f, 0, 176, &a));
    expect_labels(&a, -9, -3, (const char *const[]){"1", "2", "5", "10", "20"}, 5);
    assert(buildHistoryAxis(0.1e-6f, 50e-6f, 0, 176, &a));
    expect_labels(&a, -9, -6, (const char *const[]){"0.1", "1", "10", "100"}, 4);

    check_input_perturbation(0.09e-6f, 0.317e-6f, 0.5e-6f, -9);
    check_input_perturbation(0.1e-6f, 0.317e-6f, 0.5e-6f, -9);
    check_input_perturbation(0.05e-6f, 1e-6f / 1.2f, 0, -9);
    check_input_perturbation(0.1e-6f, 50e-6f, 0, -9);
    check_input_perturbation(2e-3f, 12.47e-3f, 0, -9);
    check_input_perturbation(0.2f, 20.0f, 0, 0);
    check_perturbation();

    return 0;
}
