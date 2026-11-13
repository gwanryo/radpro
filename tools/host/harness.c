#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "mcu-renderer-st7789.h"
#include "measurements/measurements.h"
#include "peripherals/rtc.h"
#include "system/cstring.h"
#include "system/settings.h"
#include "measurements/pulses.h"
#include "system/power.h"
#include "system/system.h"
#include "ui/draw.h"
#include "ui/game.h"
#include "ui/history.h"
#include "ui/menu.h"
#include "ui/overview.h"
#include "ui/rng.h"
#include "ui/view.h"
#include "ui/measurements.h"
#include "ui/statistics.h"
#include "ui/system.h"

extern mr_t mr;
Settings settings;

static uint16_t fb[320][240];
static uint8_t cmd, param[4];
static int paramIndex, x0, x1, y0, y1, cx, cy;
static bool isCommand;
static uint8_t textbuffer[88 * 88];

static void onSleep(uint32_t value) {}
static void onReset(bool value) {}
static void onChipselect(bool value) {}
static void onCommand(bool value) { isCommand = value; }
static void pushParam(uint8_t value)
{
    if (paramIndex < 4)
        param[paramIndex++] = value;
    if (paramIndex == 4)
    {
        int a = (param[0] << 8) | param[1], b = (param[2] << 8) | param[3];
        if (cmd == 0x2a) { x0 = a; x1 = b; }
        if (cmd == 0x2b) { y0 = a; y1 = b; }
    }
}
static void onSend(uint16_t value)
{
    if (isCommand)
    {
        cmd = value;
        paramIndex = 0;
        if (cmd == 0x2c) { cx = x0; cy = y0; }
    }
    else
        pushParam(value);
}
static void onSend16(uint16_t value)
{
    if (cmd != 0x2c)
    {
        pushParam(value >> 8);
        pushParam(value & 0xff);
        return;
    }
    if (cx >= 0 && cx < 240 && cy >= 0 && cy < 320)
        fb[cy][cx] = value;
    else
        fprintf(stderr, "out of bounds pixel %d,%d\n", cx, cy);
    if (++cx > x1) { cx = x0; cy++; }
}

static bool rtcSet = true;
static bool iconsAll;

void getDeviceDateTime(RTCDateTime *dateTime)
{
    *dateTime = (RTCDateTime){rtcSet ? 2026 : 0, 11, 12, 15, 14, 0};
}
uint8_t getBatteryLevel(void) { return 3; }
bool isBatteryCharging(void) { return false; }
bool isUSBPowered(void) { return false; }
AlertLevel getAlertLevel(void) { return ALERTLEVEL_NONE; }
bool isAlertEnabled(void) { return true; }
bool isAlertFlashing(void) { return iconsAll; }
bool isAlertPending(void) { return iconsAll; }
bool isLockModeEnabled(void) { return iconsAll; }
bool isSoundIconActive(void) { return iconsAll; }

float getBatteryVoltage(void) { return 3.912F; }
void getDeviceId(char *id) { strcpy(id, "0123456789ABCDEF0123"); }
uint32_t getTubeTime(void) { return 98765; }
uint32_t getTubePulseCount(void) { return 123456; }
float getTubeDeadTime(void) { return 0.000123F; }
void requestViewUpdate(void) {}
void showView(OnViewEvent *onViewEvent) {}

static bool saveFailed;

static void save(const char *path, int height)
{
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        fprintf(stderr, "cannot write %s\n", path);
        saveFailed = true;
        return;
    }
    fprintf(f, "P6\n240 %d\n255\n", height);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < 240; x++)
        {
            uint16_t c = fb[y][x];
            uint8_t rgb[3] = {((c >> 11) & 0x1f) * 255 / 31, ((c >> 5) & 0x3f) * 255 / 63, (c & 0x1f) * 255 / 31};
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}

static void fill(void)
{
    for (int y = 0; y < 320; y++)
        for (int x = 0; x < 240; x++)
            fb[y][x] = 0xf81f;
}

static void renderRegion(const char *name, void (*draw)(void), int height)
{
    char path[256];
    fill();
    draw();
    snprintf(path, sizeof(path), "%s.ppm", name);
    save(path, height);
}

static void render(const char *name, void (*draw)(void))
{
    renderRegion(name, draw, 320);
}

static struct
{
    const char *value, *unit;
    float confidence;
    MeasurementStyle style;
    const char *avg, *avgUnit, *dose, *doseUnit, *max, *maxUnit;
    float barValue;
} ov;

static void drawOverview(void)
{
    drawTitleBar(getString(STRING_MEASUREMENTS));
    drawMeasurementOverviewValue(ov.value, ov.unit, ov.confidence, ov.style);
    if (ov.barValue < 0)
        drawMeasurementOverviewAlert(getString(STRING_ALERT_FAULT));
    else
        drawMeasurementOverviewBar(ov.barValue, -8, 1E-6F, 10E-6F);
    drawMeasurementOverviewRow(0, getString(STRING_AVERAGE), ov.avg, ov.avgUnit);
    drawMeasurementOverviewRow(1, getString(STRING_CUMULATIVE), ov.dose, ov.doseUnit);
    drawMeasurementOverviewRow(2, getString(STRING_MAX), ov.max, ov.maxUnit);
}

static const char *averagingString;

static char zeroValue[16];
static char zeroUnit[16];

static void makeZeroValue(void)
{
    strclr(zeroValue);
    strclr(zeroUnit);
    strcatMetricValue(zeroValue, zeroUnit, 0, -2);
    strcat(zeroUnit, "Sv/h");
}

static OverviewPage1 page1;
static float page1Bar;

static void drawPage1Bar(void)
{
    drawMeasurementCompactBar(page1Bar, -8, 1E-6F, 10E-6F);
}

static void drawPage1Case(void)
{
    drawMeasurementTitleBar(averagingString);
    drawOverviewPage1(&page1, drawPage1Bar);
}

static OverviewPage1 makePage1(const char *value, const char *unit, const char *confidence, MeasurementStyle style,
                               const char *avg, const char *avgUnit, const char *max, const char *maxUnit,
                               const char *dose, const char *doseUnit, MeasurementStyle doseStyle,
                               const char *start, const char *elapsed)
{
    OverviewPage1 page = {0};
    strcpy(page.value, value);
    strcpy(page.unit, unit);
    strcpy(page.confidence, confidence);
    page.style = style;
    page.rows[0] = (OverviewRow){getString(STRING_AVERAGE), "", "", MEASUREMENTSTYLE_NORMAL};
    page.rows[1] = (OverviewRow){getString(STRING_MAX), "", "", MEASUREMENTSTYLE_NORMAL};
    page.rows[2] = (OverviewRow){getString(STRING_CUMULATIVE), "", "", doseStyle};
    page.rows[3] = (OverviewRow){getString(STRING_COUNTS), "", "", MEASUREMENTSTYLE_NORMAL};
    strcpy(page.rows[0].value, avg);
    strcpy(page.rows[0].unit, avgUnit);
    strcpy(page.rows[1].value, max);
    strcpy(page.rows[1].unit, maxUnit);
    strcpy(page.rows[2].value, dose);
    strcpy(page.rows[2].unit, doseUnit);
    strcatUInt32Grouped(page.rows[3].value, 11849);
    strcpy(page.startTime, start);
    strcpy(page.elapsedTime, elapsed);
    return page;
}

#define PAGE2_SCALE (60E-6F / 153.8F)

static uint8_t page2LogValues[HISTORY_BIN_NUM];
static OverviewPage2 page2;
static OverviewGraph page2Graph;

static uint8_t makeLogValue(double value, float scale)
{
    if (value <= 0)
        return 0;
    double logValue = 40 * __builtin_log10(value / (scale * 0.02));
    if (logValue < 1)
        logValue = 1;
    if (logValue > 255)
        logValue = 255;
    return (uint8_t)logValue;
}

static void makeMockupLogValues(uint32_t seed, double center, double peak, double last, double unit)
{
    double z = 0;
    for (int i = 0; i < HISTORY_BIN_NUM; i++)
    {
        seed = (uint32_t)(((uint64_t)seed * 16807) % 2147483647);
        z += (seed / 2147483647.0 - 0.5) * 0.12 - z * 0.12;
        double v = center * __builtin_exp(z);
        if (i >= 140 && i < 165)
        {
            double p = peak * __builtin_exp(-(i - 140) / 6.0);
            if (p > v)
                v = p;
        }
        if (i == HISTORY_BIN_NUM - 1)
            v = last;
        page2LogValues[i] = makeLogValue(v * unit, PAGE2_SCALE);
    }
}

static OverviewPage2 makePage2(const char *unit, const char *inst, MeasurementStyle instStyle, const char *avg,
                               const char *secondaryKey, const char *secondary, const char *avgTime, const char *confidence)
{
    OverviewPage2 page = {0};
    snprintf(page.instantaneousKey, sizeof(page.instantaneousKey), "%s %s", getString(STRING_INSTANTANEOUS), unit);
    strcpy(page.instantaneousValue, inst);
    page.instantaneousStyle = instStyle;
    snprintf(page.averageKey, sizeof(page.averageKey), "%s %s", getString(STRING_AVERAGE), unit);
    strcpy(page.averageValue, avg);
    page.averageStyle = MEASUREMENTSTYLE_NORMAL;
    strcpy(page.secondaryKey, secondaryKey);
    strcpy(page.secondaryValue, secondary);
    strcpy(page.averageTime, avgTime);
    strcpy(page.averageConfidence, confidence);
    return page;
}

static OverviewGraph makeGraph(float warningValue, float alarmValue, uint32_t timeTicksNum, const char *period, const char *half)
{
    return (OverviewGraph){page2LogValues, PAGE2_SCALE, "Sv/h", -6, warningValue, alarmValue, timeTicksNum, period, half};
}

static void drawPage2Case(void)
{
    drawMeasurementTitleBar(averagingString);
    drawOverviewPage2(&page2, &page2Graph);
}

static void drawPlainTitleCase(void)
{
    drawTitleBar(getString(STRING_NOTIFICATION_CUMULATIVE_RESET_CONFIRM));
}

static void drawTitleBarCase(void)
{
    drawMeasurementTitleBar(averagingString);
}

void drawMenu(void);
static MenuState menuState = {0, 2};

static const char *onMenuGetOption(menu_size_t index, MenuStyle *style)
{
    static const char *const items[] = {
        STRING_INSTANTANEOUS, STRING_HISTORY, STRING_RATE, STRING_CUMULATIVE,
        STRING_DISPLAY_FLASH, STRING_SLEEP, STRING_CLICKS, STRING_RANDOM_GENERATOR,
    };
    *style = (index == 1) ? MENUSTYLE_SUBMENU : (index == 2) ? MENUSTYLE_CHECKED : 0;
    return items[index];
}

static const Menu settingsMenu = {STRING_SETTINGS, &menuState, 8, onMenuGetOption, 0, 0};

static void drawMenuCase(void)
{
    showMenu(&settingsMenu);
    drawMenu();
}

static void drawGameCase(void)
{
    uint8_t board[8][8];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            board[y][x] = (y < 3 && ((x + y) & 1)) ? 1 : (y > 4 && ((x + y) & 1)) ? 2 : 0;
    drawGame(board, (const char[2][16]){"12:34:56", "1:02:03"});
}

static void drawRNGCase(void)
{
    drawRNG(getString(STRING_RANDOM_GENERATOR), false, "0123456789ABCDEF", "50%");
}

static void drawRNGLargeCase(void)
{
    drawRNG(getString(STRING_RANDOM_GENERATOR), true, "100", "");
}

static void drawNotificationCase(void)
{
    drawNotification(getString(STRING_CUMULATIVE), getString(STRING_NOTIFICATION_CUMULATIVE_RESET_CONFIRM));
}

static void drawStrings(void)
{
    static const char *const lines[] = {
        STRING_INSTANTANEOUS " " STRING_HISTORY " " STRING_RATE,
        STRING_RATE_WARNING, STRING_RATE_ALARM,
        STRING_50_CONFIDENCE, STRING_RANDOM_GENERATOR,
        STRING_DISPLAY_FLASH " " STRING_INDICATION,
        STRING_SLEEP " " STRING_SHORT " " STRING_LONG,
        STRING_CLICKS " " STRING_CHIRPS " " STRING_BEEPS,
        STRING_YEAR " " STRING_24_HOUR " " STRING_EVERY_HOUR,
    };
    drawTitleBar(getString(STRING_SETTINGS));
    setFont(font_small);
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    for (int i = 0; i < 9; i++)
        drawText(lines[i], &(mr_rectangle_t){0, 48 + 30 * i, 240, i == 8 ? 32 : 30}, &(mr_point_t){12, 0});
}

int main(void)
{
    mr_st7789_init(&mr, 240, 320, MR_DISPLAY_ROTATION_0, textbuffer, sizeof(textbuffer),
                   onSleep, onReset, onChipselect, onCommand, onSend, onSend16);

    settings.displayTheme = DISPLAY_THEME_DAY;
    ov = (typeof(ov)){"0.142", "\xc2\xb5Sv/h", 0.12F, MEASUREMENTSTYLE_NORMAL,
                      "0.128", " \xc2\xb5Sv/h", "1.284", " \xc2\xb5Sv", "0.317", " \xc2\xb5Sv/h", 0.142E-6F};
    render("overview-day", drawOverview);

    settings.displayTheme = DISPLAY_THEME_NIGHT;
    ov = (typeof(ov)){"12.47", "\xc2\xb5Sv/h", 0.021F, MEASUREMENTSTYLE_ALARM,
                      "3.906", " \xc2\xb5Sv/h", "18.62", " \xc2\xb5Sv", "12.91", " \xc2\xb5Sv/h", 12.47E-6F};
    render("overview-night", drawOverview);

    settings.displayTheme = DISPLAY_THEME_DUSK;
    ov = (typeof(ov)){"1234", getString(STRING_CPM), 0.5F, MEASUREMENTSTYLE_WARNING,
                      "987.6", " cpm", "9999", " " STRING_COUNTS, "12.34", " kcpm", 2000};
    render("overview-cpm", drawOverview);

    settings.displayTheme = DISPLAY_THEME_DAY;
    makeZeroValue();
    ov = (typeof(ov)){zeroValue, zeroUnit, 0, MEASUREMENTSTYLE_NORMAL,
                      "0.128", " \xc2\xb5Sv/h", "1.284", " \xc2\xb5Sv", "0.317", " \xc2\xb5Sv/h", -1};
    render("overview-fault", drawOverview);

    render("strings", drawStrings);

    averagingString = getString(STRING_ADAPTIVE_FAST);
    page1Bar = 0.142E-6F;
    page1 = makePage1("0.142", "\xc2\xb5Sv/h", "\xc2\xb1" "12%", MEASUREMENTSTYLE_NORMAL,
                      "0.128", "\xc2\xb5Sv/h", "0.317", "\xc2\xb5Sv/h", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "10:01:12");
    settings.displayTheme = DISPLAY_THEME_DAY;
    render("page1-day", drawPage1Case);
    settings.displayTheme = DISPLAY_THEME_DUSK;
    render("page1-dusk", drawPage1Case);
    settings.displayTheme = DISPLAY_THEME_NIGHT;
    render("page1-night", drawPage1Case);

    settings.displayTheme = DISPLAY_THEME_DAY;
    page1Bar = 12.47E-6F;
    page1 = makePage1("12.47", "\xc2\xb5Sv/h", "\xc2\xb1" "2.1%", MEASUREMENTSTYLE_ALARM,
                      "3.906", "\xc2\xb5Sv/h", "12.91", "\xc2\xb5Sv/h", "18.62", "\xc2\xb5Sv", MEASUREMENTSTYLE_ALARM,
                      "11.13 05:13", "10:01:12");
    render("page1-alarm", drawPage1Case);

    page1 = makePage1(zeroValue, zeroUnit, "", MEASUREMENTSTYLE_NORMAL,
                      "0.128", "\xc2\xb5Sv/h", "0.317", "\xc2\xb5Sv/h", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "10:01:12");
    page1.faultString = getString(STRING_ALERT_FAULT);
    render("page1-fault", drawPage1Case);

    page1Bar = 12.47E-3F;
    page1 = makePage1("12.47", "mSv/h", "\xc2\xb1" "0.5%", MEASUREMENTSTYLE_WARNING,
                      "9.876", "mSv/h", "13.52", "mSv/h", "123.4", "mSv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "10:01:12");
    render("page1-big", drawPage1Case);

    page1Bar = 0.142E-6F;
    page1 = makePage1("0.142", "\xc2\xb5Sv/h", "\xc2\xb1" "12%", MEASUREMENTSTYLE_NORMAL,
                      "0.128", "\xc2\xb5Sv/h", "0.317", "\xc2\xb5Sv/h", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "--", "1234:56:12");
    render("page1-long", drawPage1Case);

    page1Bar = 0.142E-6F;
    page1 = makePage1("0.142", "\xc2\xb5Sv/h", "\xc2\xb1" "12%", MEASUREMENTSTYLE_NORMAL,
                      "0.128", "\xc2\xb5Sv/h", "0.317", "\xc2\xb5Sv/h", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "1234:56:12");
    render("page1-longboth", drawPage1Case);

    page1Bar = 0.142E-6F;
    page1 = makePage1("123456", "cpm", "\xc2\xb1" "5%", MEASUREMENTSTYLE_NORMAL,
                      "98765", "cpm", "123456", "cpm", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "10:01:12");
    render("page1-sixdigits", drawPage1Case);

    page1Bar = 0.142E-6F;
    page1 = makePage1("0.142", "\xc2\xb5Sv/h", "\xc2\xb1" "12%", MEASUREMENTSTYLE_NORMAL,
                      "0.128", "\xc2\xb5Sv/h", "0.317", "\xc2\xb5Sv/h", "1.284", "\xc2\xb5Sv", MEASUREMENTSTYLE_NORMAL,
                      "11.13 05:13", "10:01:12");
    page1.rows[0].style = MEASUREMENTSTYLE_DONE;
    strclr(page1.rows[3].value);
    strcatUInt32Grouped(page1.rows[3].value, UINT32_MAX);
    render("page1-done", drawPage1Case);
    settings.displayTheme = DISPLAY_THEME_DAY;

    averagingString = getString(STRING_ADAPTIVE_FAST);
    makeMockupLogValues(7, 0.128, 0.317, 0.142, 1E-6);
    page2 = makePage2("\xc2\xb5Sv/h", "0.142", MEASUREMENTSTYLE_NORMAL, "0.128", "cpm", "21.8", "2:15:40", "\xc2\xb1" "3.8%");
    page2Graph = makeGraph(0.5E-6F, 0, 10, getString(STRING_HISTORY_10_MINUTES), getString(STRING_HISTORY_HALF_10_MINUTES));
    render("page2-10min", drawPage2Case);

    settings.displayTheme = DISPLAY_THEME_NIGHT;
    render("page2-night", drawPage2Case);
    settings.displayTheme = DISPLAY_THEME_DAY;

    makeMockupLogValues(23, 0.128, 0.317, 0.142, 1E-6);
    page2Graph = makeGraph(0.5E-6F, 0, 7, getString(STRING_HISTORY_1_WEEK), getString(STRING_HISTORY_HALF_1_WEEK));
    render("page2-1week", drawPage2Case);

    memset(page2LogValues, 0, sizeof(page2LogValues));
    page2 = makePage2(zeroUnit, zeroValue, MEASUREMENTSTYLE_NORMAL, zeroValue, "cpm", zeroValue, "0:00:00", "");
    page2Graph = makeGraph(0.5E-6F, 0, 10, getString(STRING_HISTORY_10_MINUTES), getString(STRING_HISTORY_HALF_10_MINUTES));
    render("page2-nodata", drawPage2Case);

    makeMockupLogValues(7, 0.128, 0.317, 0.142, 1E-6);
    page2 = makePage2("\xc2\xb5Sv/h", zeroValue, MEASUREMENTSTYLE_NORMAL, "0.128", "cpm", zeroValue, "2:15:40", "\xc2\xb1" "3.8%");
    snprintf(page2.instantaneousKey, sizeof(page2.instantaneousKey), "%s %s", getString(STRING_INSTANTANEOUS), zeroUnit);
    page2.faultString = getString(STRING_ALERT_FAULT);
    render("page2-fault", drawPage2Case);

    makeMockupLogValues(7, 3.2, 9.5, 12.47, 1E-3);
    page2 = makePage2("mSv/h", "12.47", MEASUREMENTSTYLE_ALARM, "3.906", "kcpm", "1918", "0:12:05", "\xc2\xb1" "0.5%");
    page2.averageStyle = MEASUREMENTSTYLE_WARNING;
    page2Graph = makeGraph(5E-3F, 10E-3F, 10, getString(STRING_HISTORY_10_MINUTES), getString(STRING_HISTORY_HALF_10_MINUTES));
    render("page2-big", drawPage2Case);

    makeMockupLogValues(7, 0.128, 0.16, 0.142, 1E-3);
    page2 = makePage2("mrem/h", "14.20", MEASUREMENTSTYLE_NORMAL, "12.80", "kcpm", "1.538", "2:15:40", "\xc2\xb1" "3.8%");
    page2Graph = makeGraph(0, 0, 10, getString(STRING_HISTORY_10_MINUTES), getString(STRING_HISTORY_HALF_10_MINUTES));
    page2Graph.scale = PAGE2_SCALE * 100;
    page2Graph.unitName = "rem/h";
    page2Graph.minPrefixExponent = -3;
    render("page2-rem", drawPage2Case);

    settings.displayTheme = DISPLAY_THEME_DAY;
    averagingString = getString(STRING_ADAPTIVE_FAST);
    renderRegion("titlebar-normal", drawTitleBarCase, 24);
    averagingString = getString(STRING_10_SECONDS);
    renderRegion("titlebar-fixed", drawTitleBarCase, 24);
    iconsAll = true;
    settings.pulseSound = true;
    averagingString = getString(STRING_ADAPTIVE_PRECISION);
    renderRegion("titlebar-icons", drawTitleBarCase, 24);
    iconsAll = false;
    rtcSet = false;
    averagingString = getString(STRING_ADAPTIVE_FAST);
    renderRegion("titlebar-nortc", drawTitleBarCase, 24);
    rtcSet = true;
    settings.rtcTimeFormat = RTC_TIMEFORMAT_12_HOUR;
    renderRegion("titlebar-12h", drawTitleBarCase, 24);
    settings.rtcTimeFormat = RTC_TIMEFORMAT_24_HOUR;
    iconsAll = true;
    averagingString = "Adaptive (precision) 10 seconds long";
    renderRegion("titlebar-omit", drawTitleBarCase, 24);
    iconsAll = true;
    renderRegion("titlebar-plain-icons", drawPlainTitleCase, 24);
    iconsAll = false;

    render("menu-settings", drawMenuCase);
    render("statistics", drawStatistics);
    render("game", drawGameCase);
    render("rng", drawRNGCase);
    render("rng-large", drawRNGLargeCase);
    render("notification", drawNotificationCase);

    return saveFailed ? 1 : 0;
}
