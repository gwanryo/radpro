// SOURCES: system/cstring.c system/cmath.c
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "measurements/overview.c"

Settings settings;
PulseUnits pulseUnits[DOSE_UNITS_NUM];
const float rateAlerts[1];
const int8_t doseUnitsMinMetricPrefix[DOSE_UNITS_NUM];

static uint32_t stubStartTime;
static RTCDateTime stubDeviceDateTime;
static RTCDateTime stubStartDateTime;

uint32_t getCumulativeDoseStartTime(void) { return stubStartTime; }
void getDeviceDateTime(RTCDateTime *dateTime) { *dateTime = stubDeviceDateTime; }
void getLocalDateTimeFromDeviceTime(uint32_t time, RTCDateTime *dateTime) { *dateTime = stubStartDateTime; }

void buildValueString(char *valueString, char *unitString, float value, const Unit *unit, int32_t minMetricPrefixIndex) {}
void clearAverageRateValues(void) {}
void clearInstantaneousRateValues(void) {}
void drawInstantaneousRateBar(void (*drawBar)(float value, int32_t minExponent, float warningValue, float alarmValue)) {}
void drawMeasurementCompactBar(float value, int32_t minExponent, float warningValue, float alarmValue) {}
void drawMeasurementTitleBar(const char *averagingString) {}
void drawOverviewPage1(const OverviewPage1 *page, void (*drawBar)(void)) {}
void drawOverviewPage2(const OverviewPage2 *page, const OverviewGraph *graph) {}
float getAverageRate(void) { return 0; }
float getAverageRateConfidence(void) { return 0; }
uint32_t getAverageRateTime(void) { return 0; }
uint32_t getCumulativeDosePulseCount(void) { return 0; }
MeasurementStyle getCumulativeDoseStyle(void) { return MEASUREMENTSTYLE_NORMAL; }
uint32_t getCumulativeDoseTime(void) { return 0; }
const char *getHistoryHalfString(void) { return ""; }
const uint8_t *getHistoryLogValues(void) { return NULL; }
const char *getHistoryPeriodString(void) { return ""; }
uint32_t getHistoryTimeTicksNum(void) { return 0; }
const char *getInstantaneousAveragingString(void) { return ""; }
float getInstantaneousMaxRate(void) { return 0; }
float getInstantaneousRate(void) { return 0; }
float getInstantaneousRateConfidence(void) { return 0; }
MeasurementStyle getInstantaneousRateStyle(void) { return MEASUREMENTSTYLE_NORMAL; }
AlertLevel getTubeFaultAlertLevel(void) { return 0; }
bool isAverageRateDone(void) { return false; }
bool onMeasurementViewEvent(ViewEvent event) { return false; }
void requestViewUpdate(void) {}
void resetHistoryTab(void) {}
void selectHistoryTab(int32_t delta) {}
void triggerVibration(void) {}

static void expect(const char *actual, const char *expected)
{
    if (strcmp(actual, expected))
        printf("'%s' != '%s'\n", actual, expected);
    assert(!strcmp(actual, expected));
}

static void start_is(uint32_t startTime, uint16_t deviceYear, RTCDateTime start, uint32_t format, const char *expected)
{
    char s[32] = "garbage";
    stubStartTime = startTime;
    stubDeviceDateTime = (RTCDateTime){deviceYear, 11, 13, 10, 0, 0};
    stubStartDateTime = start;
    settings.rtcTimeFormat = format;
    buildStartTimeString(s);
    expect(s, expected);
}

static void key_is(uint32_t size, const char *word, const char *unit, const char *expected)
{
    char s[32] = "garbage";
    buildCellKey(s, size, word, unit);
    expect(s, expected);
    assert(strlen(s) < size);
}

static void grouped_is(uint32_t value, const char *expected)
{
    char s[32] = "";
    strcatUInt32Grouped(s, value);
    expect(s, expected);
}

static void confidence_is(float confidence, const char *expected)
{
    char s[32] = "garbage";
    buildConfidenceString(s, confidence);
    expect(s, expected);
}

int main(void)
{
    RTCDateTime morning = {2026, 11, 13, 5, 13, 0};
    RTCDateTime midnight = {2026, 1, 2, 0, 7, 0};
    RTCDateTime evening = {2026, 12, 31, 17, 59, 0};

    start_is(0, 2026, morning, RTC_TIMEFORMAT_24_HOUR, "--");
    start_is(1000, RTC_YEAR_MIN - 1, morning, RTC_TIMEFORMAT_24_HOUR, "--");
    start_is(1000, 2026, morning, RTC_TIMEFORMAT_24_HOUR, "11.13 05:13");
    start_is(1000, 2026, evening, RTC_TIMEFORMAT_24_HOUR, "12.31 17:59");
    start_is(1000, 2026, morning, RTC_TIMEFORMAT_12_HOUR, "11.13 5:13");
    start_is(1000, 2026, midnight, RTC_TIMEFORMAT_12_HOUR, "01.02 12:07");
    start_is(1000, 2026, evening, RTC_TIMEFORMAT_12_HOUR, "12.31 5:59");

    key_is(32, STRING_INSTANTANEOUS, "\xc2\xb5Sv/h", STRING_INSTANTANEOUS " \xc2\xb5Sv/h");
    key_is(32, STRING_AVERAGE, "mrem/h", STRING_AVERAGE " mrem/h");
    key_is(strlen(STRING_AVERAGE) + 1 + strlen("mrem/h") + 1, STRING_AVERAGE, "mrem/h", STRING_AVERAGE " mrem/h");
    key_is(strlen(STRING_AVERAGE) + 1 + strlen("mrem/h"), STRING_AVERAGE, "mrem/h", "mrem/h");
    key_is(8, STRING_INSTANTANEOUS, "\xc2\xb5Sv/h", "\xc2\xb5Sv/h");
    key_is(5, STRING_AVERAGE, "kcpm/h", "kcpm");

    grouped_is(0, "0");
    grouped_is(999, "999");
    grouped_is(1000, "1,000");
    grouped_is(11849, "11,849");
    grouped_is(1002003, "1,002,003");
    grouped_is(UINT32_MAX, "4,294,967,295");

    confidence_is(0, "");
    confidence_is(0.038F, STRING_PLUSMINUS "3.8" STRING_PERCENT);
    confidence_is(0.12F, STRING_PLUSMINUS "12" STRING_PERCENT);

    return 0;
}
