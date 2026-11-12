/*
 * Rad Pro
 * UI system
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include "../measurements/measurements.h"
#include "../peripherals/rtc.h"
#include "../system/settings.h"
#include "../ui/draw.h"
#include "../ui/system.h"

void drawPowerOff(bool displayBatteryIcon)
{
    char buffer[16];

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    if (displayBatteryIcon)
        setupBatteryIcon(buffer);
    else
        strclr(buffer);
    drawCenteredText(buffer,
                     &(mr_rectangle_t){0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT},
                     &(mr_point_t){(DISPLAY_WIDTH / 2), ((DISPLAY_HEIGHT - FONT_SYMBOLS_LINE_HEIGHT) / 2)});
}

static const ColorIndex alertLevelColorIndex[] = {
    COLOR_ELEMENT_NEUTRAL,
    COLOR_WARNING,
    COLOR_ALARM,
};

#if defined(FS5000)
#define TITLEBAR_ICON_GAP 6
#define TITLEBAR_TIME_GAP 10

static void drawTitleBarItem(const char *text, uint32_t leftGap, mr_rectangle_t *rectangle)
{
    uint32_t rightGap = (rectangle->x == TITLEBAR_WIDTH) ? PADDING_RIGHT : TITLEBAR_ICON_GAP;
    uint32_t width = leftGap + getTextWidth(text) + rightGap;
    rectangle->width = width;
    rectangle->x -= width;
    drawText(text, rectangle, &(mr_point_t){leftGap, (rectangle->height - getTextHeight()) / 2});
}

static void drawTitleBarIcon(const char *text, mr_rectangle_t *rectangle)
{
    drawTitleBarItem(text, 0, rectangle);
}

static void setupTitleBar(mr_rectangle_t *rectangle)
{
    *rectangle = (mr_rectangle_t){
        TITLEBAR_WIDTH,
        TITLEBAR_TOP,
        0,
        TITLEBAR_CONTENT_HEIGHT,
    };
    setFillColor(COLOR_CONTAINER_GLOBAL);
}

static void drawTitleBarIcons(mr_rectangle_t *rectangle)
{
    char buffer[16];

    setupBatteryIcon(buffer);
    setFont(font_symbols_small);
    drawTitleBarIcon(buffer, rectangle);

    AlertLevel alertLevel = getAlertLevel();
    if (isAlertEnabled() || alertLevel)
    {
        bool notificationIconFilled;
        if (!isAlertFlashing())
        {
            alertLevel = ALERTLEVEL_NONE;
            notificationIconFilled = false;
        }
        else
            notificationIconFilled = isAlertPending();

        setStrokeColor(alertLevelColorIndex[alertLevel]);
        drawTitleBarIcon(notificationIconFilled ? ";" : ":", rectangle);
    }

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    if (isLockModeEnabled())
        drawTitleBarIcon("<", rectangle);

    if (isSoundIconActive())
        drawTitleBarIcon(settings.pulseSound ? "9" : "8", rectangle);
}

static bool setupTimeString(char *buffer)
{
    RTCDateTime dateTime;
    getDeviceDateTime(&dateTime);
    if (dateTime.year < RTC_YEAR_MIN)
        return false;

    strclr(buffer);
    if (settings.rtcTimeFormat == RTC_TIMEFORMAT_24_HOUR)
        strcatUInt32(buffer, dateTime.hour, 2);
    else
    {
        uint32_t hour = dateTime.hour % 12;
        strcatUInt32(buffer, (hour == 0) ? 12 : hour, 1);
    }
    strcatChar(buffer, ':');
    strcatUInt32(buffer, dateTime.minute, 2);

    return true;
}

static void drawTitleBarShadow(void)
{
#if TITLEBAR_SHADOW_HEIGHT > 0
    setFillColor(COLOR_CONTAINER_GLOBAL_SHADOW);
    drawRectangle(&(mr_rectangle_t){TITLEBAR_LEFT, TITLEBAR_BOTTOM - TITLEBAR_SHADOW_HEIGHT, TITLEBAR_WIDTH, TITLEBAR_SHADOW_HEIGHT});
#endif

    setFillColor(COLOR_CONTAINER_BACKGROUND);
}

void drawTitleBar(const char *title)
{
    char buffer[16];
    mr_rectangle_t rectangle;

    setupTitleBar(&rectangle);

    setFont(font_tiny);
    if (setupTimeString(buffer))
    {
        setStrokeColor(COLOR_ELEMENT_ACTIVE);
        drawTitleBarItem(buffer, 0, &rectangle);
    }

    drawTitleBarIcons(&rectangle);

    setFont(font_tiny);
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    drawRowLeft(title, &rectangle);

    drawTitleBarShadow();
}

void drawMeasurementTitleBar(const char *averagingString)
{
    char buffer[16];
    mr_rectangle_t rectangle;

    setupTitleBar(&rectangle);
    drawTitleBarIcons(&rectangle);

    setFont(font_tiny);
    uint32_t left = 0;
    uint32_t averagingOffset = PADDING_LEFT;
    if (setupTimeString(buffer))
    {
        left = PADDING_LEFT + getTextWidth(buffer);
        setStrokeColor(COLOR_ELEMENT_ACTIVE);
        drawText(buffer,
                 &(mr_rectangle_t){0, TITLEBAR_TOP, left, TITLEBAR_CONTENT_HEIGHT},
                 &(mr_point_t){PADDING_LEFT, (TITLEBAR_CONTENT_HEIGHT - getTextHeight()) / 2});
        averagingOffset = TITLEBAR_TIME_GAP;
    }

    if (left + averagingOffset + getTextWidth(averagingString) + TITLEBAR_ICON_GAP > (uint32_t)rectangle.x)
        averagingString = "";

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(averagingString,
             &(mr_rectangle_t){left, TITLEBAR_TOP, rectangle.x - left, TITLEBAR_CONTENT_HEIGHT},
             &(mr_point_t){averagingOffset, (TITLEBAR_CONTENT_HEIGHT - getTextHeight()) / 2});

    drawTitleBarShadow();
}

#else
void drawTitleBar(const char *title)
{
    char buffer[16];
    mr_rectangle_t rectangle = {
        TITLEBAR_WIDTH,
        TITLEBAR_TOP,
        0,
        TITLEBAR_CONTENT_HEIGHT,
    };
    setFillColor(COLOR_CONTAINER_GLOBAL);

    // Time
    RTCDateTime dateTime;
    getDeviceDateTime(&dateTime);
    if (dateTime.year >= RTC_YEAR_MIN)
    {
        strclr(buffer);
        if (settings.rtcTimeFormat == RTC_TIMEFORMAT_24_HOUR)
            strcatUInt32(buffer, dateTime.hour, 2);
        else
        {
            uint32_t hour = dateTime.hour % 12;
            strcatUInt32(buffer, (hour == 0) ? 12 : hour, 1);
        }
        strcatChar(buffer, ':');
        strcatUInt32(buffer, dateTime.minute, 2);

        setFont(font_small);
        setStrokeColor(COLOR_ELEMENT_ACTIVE);
        drawRowRight(buffer, &rectangle);
    }

    // Battery icon
    setupBatteryIcon(buffer);
    drawRowRight(buffer, &rectangle);

    // Alert icon
    AlertLevel alertLevel = getAlertLevel();
    if (isAlertEnabled() || alertLevel)
    {
        bool notificationIconFilled;
        if (!isAlertFlashing())
        {
            alertLevel = ALERTLEVEL_NONE;
            notificationIconFilled = false;
        }
        else
            notificationIconFilled = isAlertPending();

        setStrokeColor(alertLevelColorIndex[alertLevel]);
        drawRowRight(notificationIconFilled ? ";" : ":", &rectangle);
    }

    // Lock icon
    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    if (isLockModeEnabled())
        drawRowRight("<", &rectangle);

    // Sound icon
    if (isSoundIconActive())
        drawRowRight(settings.pulseSound ? "9" : "8", &rectangle);

    // Title
    setFont(font_small);
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    drawRowLeft(title, &rectangle);

    // Shadow
#if TITLEBAR_SHADOW_HEIGHT > 0
    setFillColor(COLOR_CONTAINER_GLOBAL_SHADOW);
    drawRectangle(&(mr_rectangle_t){TITLEBAR_LEFT, TITLEBAR_BOTTOM - TITLEBAR_SHADOW_HEIGHT, TITLEBAR_WIDTH, 1});
#endif

    // Set background
    setFillColor(COLOR_CONTAINER_BACKGROUND);
}

#endif

void drawSplash(const char *message)
{
    setFillColor(COLOR_CONTAINER_GLOBAL);

    drawCenteredMultilineText(&(mr_rectangle_t){0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT},
                              message);
}

void drawNotification(const char *title, const char *message)
{
    drawTitleBar(title);

    drawCenteredMultilineText(&contentRectangle, message);
}
