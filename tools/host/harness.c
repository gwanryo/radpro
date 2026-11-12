#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "mcu-renderer-st7789.h"
#include "measurements/measurements.h"
#include "peripherals/rtc.h"
#include "system/cstring.h"
#include "system/settings.h"
#include "ui/draw.h"
#include "ui/measurements.h"
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

void getDeviceDateTime(RTCDateTime *dateTime)
{
    *dateTime = (RTCDateTime){2026, 11, 12, 15, 14, 0};
}
uint8_t getBatteryLevel(void) { return 3; }
bool isBatteryCharging(void) { return false; }
bool isUSBPowered(void) { return false; }
AlertLevel getAlertLevel(void) { return ALERTLEVEL_NONE; }
bool isAlertEnabled(void) { return true; }
bool isAlertFlashing(void) { return false; }
bool isAlertPending(void) { return false; }
bool isLockModeEnabled(void) { return false; }
bool isSoundIconActive(void) { return false; }

static bool saveFailed;

static void save(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        fprintf(stderr, "cannot write %s\n", path);
        saveFailed = true;
        return;
    }
    fprintf(f, "P6\n240 320\n255\n");
    for (int y = 0; y < 320; y++)
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

static void render(const char *name, void (*draw)(void))
{
    char path[256];
    fill();
    draw();
    snprintf(path, sizeof(path), "%s.ppm", name);
    save(path);
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
    ov = (typeof(ov)){"0.000", "\xc2\xb5Sv/h", 0, MEASUREMENTSTYLE_NORMAL,
                      "0.128", " \xc2\xb5Sv/h", "1.284", " \xc2\xb5Sv", "0.317", " \xc2\xb5Sv/h", -1};
    render("overview-fault", drawOverview);

    render("strings", drawStrings);

    return saveFailed ? 1 : 0;
}
