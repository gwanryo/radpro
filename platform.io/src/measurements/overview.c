/*
 * Rad Pro
 * Overview
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include "../measurements/average.h"
#include "../measurements/cumulative.h"
#include "../measurements/history.h"
#include "../measurements/instantaneous.h"
#include "../measurements/overview.h"
#include "../peripherals/rtc.h"
#include "../system/cstring.h"
#include "../system/events.h"
#include "../system/settings.h"
#include "../ui/overview.h"
#include "../ui/system.h"

#if defined(FS5000)

typedef enum
{
    OVERVIEW_PAGE_VALUES,
    OVERVIEW_PAGE_GRAPH,

    OVERVIEW_PAGE_NUM,
} OverviewPage;

static OverviewPage overviewPage;

static cstring overviewAveragingStrings[] = {
    STRING_ADAPTIVE_FAST,
    STRING_ADAPTIVE_PRECISION,
    STRING_1_MINUTE,
    STRING_30_SECONDS,
    STRING_10_SECONDS,
};

void resetOverview(void)
{
    overviewPage = OVERVIEW_PAGE_VALUES;

    resetHistoryTab();
}

void selectOverviewFirstPage(void)
{
    overviewPage = OVERVIEW_PAGE_VALUES;
}

static void copyString(char *dest, const char *src, uint32_t size)
{
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

static void buildOverviewValue(char *valueString, uint32_t valueSize, char *unitString, uint32_t unitSize,
                               float value, const Unit *unit, int32_t minMetricPrefixIndex)
{
    char valueBuffer[32] = "";
    char unitBuffer[32] = "";
    buildValueString(valueBuffer, unitBuffer, value, unit, minMetricPrefixIndex);

    const char *unitStart = unitBuffer;
    if (unitStart[0] == ' ')
        unitStart++;

    copyString(valueString, valueBuffer, valueSize);
    copyString(unitString, unitStart, unitSize);
}

static void buildConfidenceString(char *confidenceString, float confidence)
{
    strclr(confidenceString);

    if (confidence != 0)
    {
        strcat(confidenceString, getString(STRING_PLUSMINUS));
        strcatFloat(confidenceString, 100 * confidence, confidence < 0.1F ? 1 : 0);
        strcat(confidenceString, getString(STRING_PERCENT));
    }
}

static void buildStartTimeString(char *timeString)
{
    strclr(timeString);

    uint32_t startTime = getCumulativeDoseStartTime();

    RTCDateTime dateTime;
    getDeviceDateTime(&dateTime);
    if (!startTime || (dateTime.year < RTC_YEAR_MIN))
    {
        strcat(timeString, "--");

        return;
    }

    int32_t timeZoneOffset = 3600 * (int32_t)getDeviceTimeZone();
    getDateTimeFromTime(startTime + timeZoneOffset, &dateTime);

    strcatUInt32(timeString, dateTime.month, 2);
    strcatChar(timeString, '.');
    strcatUInt32(timeString, dateTime.day, 2);
    strcatChar(timeString, ' ');
    if (settings.rtcTimeFormat == RTC_TIMEFORMAT_24_HOUR)
        strcatUInt32(timeString, dateTime.hour, 2);
    else
    {
        uint32_t hour = dateTime.hour % 12;
        strcatUInt32(timeString, (hour == 0) ? 12 : hour, 1);
    }
    strcatChar(timeString, ':');
    strcatUInt32(timeString, dateTime.minute, 2);
}

static void buildCellKey(char *keyString, uint32_t size, const char *word, const char *unitString)
{
    strclr(keyString);

    if (strlen(word) + 1 + strlen(unitString) < size)
    {
        strcat(keyString, word);
        strcat(keyString, " ");
    }

    copyString(keyString + strlen(keyString), unitString, size - strlen(keyString));
}

static const char *getOverviewFaultString(void)
{
    return getTubeFaultAlertLevel() ? getString(STRING_ALERT_FAULT) : NULL;
}

static MeasurementStyle getAverageRateStyle(void)
{
    return isAverageRateDone() ? MEASUREMENTSTYLE_DONE : MEASUREMENTSTYLE_NORMAL;
}

static void buildOverviewRow(OverviewRow *row, const char *keyString, float value, const Unit *unit, MeasurementStyle style)
{
    row->key = keyString;
    buildOverviewValue(row->value, sizeof(row->value), row->unit, sizeof(row->unit),
                       value, unit, doseUnitsMinMetricPrefix[settings.doseUnits]);
    row->style = style;
}

static void drawOverviewBar(void)
{
    drawInstantaneousRateBar(drawMeasurementCompactBar);
}

static void drawOverviewValuesPage(void)
{
    const PulseUnits *units = &pulseUnits[settings.doseUnits];

    OverviewPage1 page;
    memset(&page, 0, sizeof(page));

    buildOverviewValue(page.value, sizeof(page.value), page.unit, sizeof(page.unit),
                       getInstantaneousRate(), &units->rate, doseUnitsMinMetricPrefix[settings.doseUnits]);
    buildConfidenceString(page.confidence, getInstantaneousRateConfidence());
    page.style = getInstantaneousRateStyle();
    page.faultString = getOverviewFaultString();

    buildOverviewRow(&page.rows[0], getString(STRING_AVERAGE), getAverageRate(), &units->rate, getAverageRateStyle());
    buildOverviewRow(&page.rows[1], getString(STRING_MAX), getInstantaneousMaxRate(), &units->rate, MEASUREMENTSTYLE_NORMAL);
    buildOverviewRow(&page.rows[2], getString(STRING_CUMULATIVE_DOSE), getCumulativeDosePulseCount(), &units->dose, getCumulativeDoseStyle());

    page.rows[3].key = getString(STRING_COUNTS);
    strcatUInt32(page.rows[3].value, getCumulativeDosePulseCount(), 0);
    page.rows[3].style = MEASUREMENTSTYLE_NORMAL;

    buildStartTimeString(page.startTime);
    strcatTime(page.elapsedTime, getCumulativeDoseTime());

    drawOverviewPage1(&page, drawOverviewBar);
}

static void drawOverviewGraphPage(void)
{
    const PulseUnits *units = &pulseUnits[settings.doseUnits];
    int32_t minMetricPrefixIndex = doseUnitsMinMetricPrefix[settings.doseUnits];

    OverviewPage2 page;
    memset(&page, 0, sizeof(page));

    char unitString[16];

    buildOverviewValue(page.instantaneousValue, sizeof(page.instantaneousValue), unitString, sizeof(unitString),
                       getInstantaneousRate(), &units->rate, minMetricPrefixIndex);
    buildCellKey(page.instantaneousKey, sizeof(page.instantaneousKey), getString(STRING_INSTANTANEOUS), unitString);
    page.instantaneousStyle = getInstantaneousRateStyle();

    buildOverviewValue(page.averageValue, sizeof(page.averageValue), unitString, sizeof(unitString),
                       getAverageRate(), &units->rate, minMetricPrefixIndex);
    buildCellKey(page.averageKey, sizeof(page.averageKey), getString(STRING_AVERAGE), unitString);
    page.averageStyle = getAverageRateStyle();

    buildOverviewValue(page.secondaryValue, sizeof(page.secondaryValue), page.secondaryKey, sizeof(page.secondaryKey),
                       getInstantaneousRate(), &pulseUnits[settings.secondaryDoseUnits].rate, doseUnitsMinMetricPrefix[settings.secondaryDoseUnits]);

    strcatTime(page.averageTime, getAverageRateTime());
    buildConfidenceString(page.averageConfidence, getAverageRateConfidence());
    page.faultString = getOverviewFaultString();

    float scale = units->rate.scale;
    float alertScale = scale / pulseUnits[DOSE_UNITS_SIEVERTS].rate.scale;

    OverviewGraph graph = {
        getHistoryLogValues(),
        scale,
        units->rate.name,
        3 * minMetricPrefixIndex,
        rateAlerts[settings.rateWarning] * alertScale,
        rateAlerts[settings.rateAlarm] * alertScale,
        getHistoryTimeTicksNum(),
        getHistoryPeriodString(),
        getHistoryHalfString(),
    };

    drawOverviewPage2(&page, &graph);
}

static void drawOverviewView(void)
{
    drawMeasurementTitleBar(getString(overviewAveragingStrings[settings.instantaneousAveraging]));

    if (overviewPage == OVERVIEW_PAGE_VALUES)
        drawOverviewValuesPage();
    else
        drawOverviewGraphPage();
}

void onOverviewViewEvent(ViewEvent event)
{
    if ((event == EVENT_KEY_UP) || (event == EVENT_KEY_DOWN))
    {
        if (overviewPage == OVERVIEW_PAGE_GRAPH)
        {
            selectHistoryTab((event == EVENT_KEY_UP) ? -1 : 1);

            requestViewUpdate();
        }

        return;
    }

    if (onMeasurementViewEvent(event))
        return;

    switch (event)
    {
    case EVENT_KEY_BACK:
        overviewPage++;
        if (overviewPage >= OVERVIEW_PAGE_NUM)
            overviewPage = OVERVIEW_PAGE_VALUES;

        requestViewUpdate();

        break;

    case EVENT_KEY_RESET:
        triggerVibration();

        clearInstantaneousRateValues();
        clearAverageRateValues();

        requestViewUpdate();

        break;

    case EVENT_DRAW:
        drawOverviewView();

        break;

    default:
        break;
    }
}

#endif
