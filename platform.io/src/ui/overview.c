/*
 * Rad Pro
 * UI overview
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include "../system/cstring.h"
#include "../ui/draw.h"
#include "../ui/overview.h"

#if defined(FS5000)

#define VALUE_TOP CONTENT_TOP
#define VALUE_BOTTOM 84
#define VALUE_BASELINE 71
#define VALUE_OFFSETY (VALUE_BASELINE - FONT_VALUE_ASCENT - VALUE_TOP)
#define VALUE_COLUMN_GAP 6
#define UNIT_TEXTTOP 30
#define CONFIDENCE_TEXTTOP 59

#define BAR_REGION_TOP VALUE_BOTTOM
#define BAR_REGION_BOTTOM 132

#define ROW_NUM 4
#define ROW_TOP 140
#define ROW_PITCH 30
#define ROW_UNIT_GAP 5
#define ROWS_BOTTOM 262

#define TIMES_BOTTOM 296
#define TIMES_TEXTTOP 270

#define DOTS_REGION_TOP TIMES_BOTTOM
#define DOTS_TOP 302
#define DOTS_SIZE 6
#define DOTS_GAP 6

static ColorIndex getValueColorIndex(MeasurementStyle style)
{
    if (style == MEASUREMENTSTYLE_WARNING)
        return COLOR_WARNING;
    else if (style == MEASUREMENTSTYLE_ALARM)
        return COLOR_ALARM;
    else if (style == MEASUREMENTSTYLE_DONE)
        return COLOR_ELEMENT_ACTIVE;
    else
        return COLOR_INSTRUMENT_ENHANCED_SECONDARY;
}

static ColorIndex getRowColorIndex(MeasurementStyle style)
{
    if (style == MEASUREMENTSTYLE_WARNING)
        return COLOR_WARNING;
    else if (style == MEASUREMENTSTYLE_ALARM)
        return COLOR_ALARM;
    else
        return COLOR_ELEMENT_ACTIVE;
}

static void drawValueBlock(const OverviewPage1 *page)
{
    setFont(font_small);
    int16_t unitWidth = getTextWidth(page->unit);
    setFont(font_tiny);
    int16_t confidenceWidth = getTextWidth(page->confidence);
    int16_t columnWidth = PADDING_RIGHT + VALUE_COLUMN_GAP + ((unitWidth > confidenceWidth) ? unitWidth : confidenceWidth);
    int16_t columnLeft = DISPLAY_WIDTH - columnWidth;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    setFont(font_value);
    setStrokeColor(getValueColorIndex(page->style));
    drawText(page->value,
             &(mr_rectangle_t){0, VALUE_TOP, columnLeft, VALUE_BOTTOM - VALUE_TOP},
             &(mr_point_t){PADDING_LEFT, VALUE_OFFSETY});

    setFont(font_small);
    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawRightAlignedText(page->unit,
                         &(mr_rectangle_t){columnLeft, VALUE_TOP, columnWidth, CONFIDENCE_TEXTTOP - VALUE_TOP},
                         &(mr_point_t){columnWidth - PADDING_RIGHT, UNIT_TEXTTOP - VALUE_TOP});

    setFont(font_tiny);
    setStrokeColor(getValueColorIndex(page->style));
    drawRightAlignedText(page->confidence,
                         &(mr_rectangle_t){columnLeft, CONFIDENCE_TEXTTOP, columnWidth, VALUE_BOTTOM - CONFIDENCE_TEXTTOP},
                         &(mr_point_t){columnWidth - PADDING_RIGHT, 0});
}

static void drawBarRegion(const OverviewPage1 *page, void (*drawBar)(void))
{
    if (page->faultString)
    {
        setFillColor(COLOR_CONTAINER_BACKGROUND);
        setFont(font_small);
        setStrokeColor(COLOR_ALARM);
        drawText(page->faultString,
                 &(mr_rectangle_t){0, BAR_REGION_TOP, DISPLAY_WIDTH, BAR_REGION_BOTTOM - BAR_REGION_TOP},
                 &(mr_point_t){PADDING_LEFT, (BAR_REGION_BOTTOM - BAR_REGION_TOP - FONT_SMALL_LINE_HEIGHT) / 2});
    }
    else
        drawBar();
}

static void drawRow(uint32_t index, const OverviewRow *row)
{
    int16_t top = ROW_TOP + index * ROW_PITCH;
    int16_t bottom = (index == ROW_NUM - 1) ? ROWS_BOTTOM : top + ROW_PITCH;
    int16_t offsetY = 0;
    if (index == 0)
    {
        offsetY = top - BAR_REGION_BOTTOM;
        top = BAR_REGION_BOTTOM;
    }
    int16_t height = bottom - top;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    setFont(font_small);

    int16_t valueWidth = getTextWidth(row->value);
    int16_t gap = row->unit[0] ? ROW_UNIT_GAP : 0;
    int16_t unitLeft = DISPLAY_WIDTH - PADDING_RIGHT - getTextWidth(row->unit);
    int16_t valueLeft = unitLeft - gap - valueWidth;

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(row->key ? row->key : "",
             &(mr_rectangle_t){0, top, valueLeft, height},
             &(mr_point_t){PADDING_LEFT, offsetY});

    setStrokeColor(getRowColorIndex(row->style));
    drawText(row->value,
             &(mr_rectangle_t){valueLeft, top, valueWidth + gap, height},
             &(mr_point_t){0, offsetY});

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(row->unit,
             &(mr_rectangle_t){unitLeft, top, DISPLAY_WIDTH - unitLeft, height},
             &(mr_point_t){0, offsetY});
}

static void drawLabeledTime(const char *label, const char *time, int16_t left, int16_t width)
{
    const int16_t height = TIMES_BOTTOM - ROWS_BOTTOM;
    const int16_t offsetY = TIMES_TEXTTOP - ROWS_BOTTOM;

    char buffer[32];
    strclr(buffer);
    strcat(buffer, label);
    strcat(buffer, " ");
    int16_t labelWidth = getTextWidth(buffer);

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(buffer,
             &(mr_rectangle_t){left, ROWS_BOTTOM, labelWidth, height},
             &(mr_point_t){0, offsetY});
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    drawText(time,
             &(mr_rectangle_t){left + labelWidth, ROWS_BOTTOM, width - labelWidth, height},
             &(mr_point_t){0, offsetY});
}

static void drawTimes(const OverviewPage1 *page)
{
    setFillColor(COLOR_CONTAINER_BACKGROUND);
    setFont(font_tiny);

    char buffer[32];
    strclr(buffer);
    strcat(buffer, getString(STRING_ELAPSED));
    strcat(buffer, " ");
    int16_t elapsedWidth = getTextWidth(buffer) + getTextWidth(page->elapsedTime);
    int16_t elapsedLeft = DISPLAY_WIDTH - PADDING_RIGHT - elapsedWidth;

    drawLabeledTime(getString(STRING_START), page->startTime, PADDING_LEFT, elapsedLeft - PADDING_LEFT);
    drawLabeledTime(getString(STRING_ELAPSED), page->elapsedTime, elapsedLeft, DISPLAY_WIDTH - elapsedLeft);

    drawRectangle(&(mr_rectangle_t){0, ROWS_BOTTOM, PADDING_LEFT, TIMES_BOTTOM - ROWS_BOTTOM});
}

void drawOverviewPageDots(uint32_t pageIndex, uint32_t pageNum, int16_t top)
{
    int16_t width = pageNum * DOTS_SIZE + (pageNum - 1) * DOTS_GAP;
    int16_t left = (DISPLAY_WIDTH - width) / 2;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    drawRectangle(&(mr_rectangle_t){0, top, DISPLAY_WIDTH, DOTS_SIZE});

    for (uint32_t i = 0; i < pageNum; i++)
    {
        setFillColor((i == pageIndex) ? COLOR_ELEMENT_ACTIVE : COLOR_INSTRUMENT_FRAME_TERTIARY);
        drawRectangle(&(mr_rectangle_t){left + i * (DOTS_SIZE + DOTS_GAP), top, DOTS_SIZE, DOTS_SIZE});
    }
}

void drawOverviewPage1(const OverviewPage1 *page, void (*drawBar)(void))
{
    drawValueBlock(page);
    drawBarRegion(page, drawBar);
    for (uint32_t i = 0; i < ROW_NUM; i++)
        drawRow(i, &page->rows[i]);
    drawTimes(page);

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    drawRectangle(&(mr_rectangle_t){0, DOTS_REGION_TOP, DISPLAY_WIDTH, DOTS_TOP - DOTS_REGION_TOP});
    drawRectangle(&(mr_rectangle_t){0, DOTS_TOP + DOTS_SIZE, DISPLAY_WIDTH, DISPLAY_HEIGHT - DOTS_TOP - DOTS_SIZE});
    drawOverviewPageDots(0, 2, DOTS_TOP);
}

#endif
