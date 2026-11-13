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
#include "../ui/history.h"
#include "../ui/historyaxis.h"
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

#define CELL_LABEL_TOP CONTENT_TOP
#define CELL_LABEL_TEXTTOP 27
#define CELL_VALUE_TOP 49
#define CELL_VALUE_TEXTTOP 47
#define CELL_BOTTOM 78
#define CELL_MIDDLE_LEFT 96
#define CELL_GAP 8

#define INFO_TOP CELL_BOTTOM
#define INFO_BOTTOM 100
#define INFO_GAP 6

#define GRAPH_REGION_TOP INFO_BOTTOM
#define GRAPH_REGION_BOTTOM 280
#define GRAPH_REGION_HEIGHT (GRAPH_REGION_BOTTOM - GRAPH_REGION_TOP)
#define GRAPH_LEFT 13
#define GRAPH_TOP 102
#define GRAPH_WIDTH HISTORY_BIN_NUM
#define GRAPH_HEIGHT 176
#define GRAPH_OFFSETY (GRAPH_TOP - GRAPH_REGION_TOP)
#define GRAPH_TICKS_LEFT (GRAPH_LEFT + GRAPH_WIDTH + TICKS_SPACING)
#define GRAPH_LABEL_MAX (HISTORY_AXIS_LINE_MAX + 2)
#define GRAPH_LABEL_LEFT 4
#define GRAPH_LABEL_RIGHT 6
#define GRAPH_LABEL_PADDING 2
#define GRAPH_LABEL_ASCENT 15
#define GRAPH_LABEL_DESCENT 4
#define GRAPH_LABEL_BASELINE_GAP 3
#define GRAPH_TOP_LABEL_BASELINE 15
#define GRAPH_GRID_DASH 2
#define GRAPH_ALERT_DASH 4
#define GRAPH_ALERT_PERIOD 7
#define GRAPH_UNIT_EXTRA 4
#define GRAPH_DOT_HALO_RADIUS ((int32_t)(sizeof(graphDotHalo) / 2))
#define GRAPH_DOT_RADIUS ((int32_t)(sizeof(graphDot) / 2))
#define GRAPH_DOT_BIN (GRAPH_WIDTH - 2 - GRAPH_DOT_HALO_RADIUS)
#define GRAPH_MARGIN ((uint8_t)(0.1F * HISTORY_DECADE))

#define AXIS_TOP GRAPH_REGION_BOTTOM
#define AXIS_BOTTOM 304
#define AXIS_HEIGHT (AXIS_BOTTOM - AXIS_TOP)
#define AXIS_TEXTTOP 282
#define AXIS_GAP 6
#define TRIANGLE_WIDTH 6
#define TRIANGLE_TOP (AXIS_TEXTTOP + FONT_TINY_ASCENT - FONT_TINY_CAP_HEIGHT - AXIS_TOP)

#define PAGE2_DOTS_TOP 308

typedef struct
{
    uint32_t num;
    mr_rectangle_t rectangles[GRAPH_LABEL_MAX];
} GraphLabels;

typedef void (*DrawTextFunction)(const char *text, const mr_rectangle_t *rectangle, const mr_point_t *offset);

static const uint8_t graphDotHalo[] = {2, 3, 4, 4, 4, 4, 4, 3, 2};
static const uint8_t graphDot[] = {1, 2, 3, 3, 3, 2, 1};

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
    else if (style == MEASUREMENTSTYLE_DONE)
        return COLOR_INSTRUMENT_ENHANCED_SECONDARY;
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
    if (valueLeft < PADDING_LEFT)
        valueLeft = PADDING_LEFT;
    int16_t valueRectWidth = unitLeft - valueLeft;
    if (valueRectWidth > valueWidth + gap)
        valueRectWidth = valueWidth + gap;

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(row->key ? row->key : "",
             &(mr_rectangle_t){0, top, valueLeft, height},
             &(mr_point_t){PADDING_LEFT, offsetY});

    setStrokeColor(getRowColorIndex(row->style));
    drawText(row->value,
             &(mr_rectangle_t){valueLeft, top, valueRectWidth, height},
             &(mr_point_t){0, offsetY});

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(row->unit,
             &(mr_rectangle_t){unitLeft, top, DISPLAY_WIDTH - unitLeft, height},
             &(mr_point_t){0, offsetY});
}

static void drawLabeledTime(const char *label, const char *time, const mr_rectangle_t *rectangle, int16_t offsetY)
{
    int16_t left = rectangle->x;
    int16_t top = rectangle->y;
    int16_t width = rectangle->width;
    int16_t height = rectangle->height;

    char buffer[32];
    strclr(buffer);
    strcat(buffer, label);
    strcat(buffer, " ");
    int16_t labelWidth = getTextWidth(buffer);
    if (labelWidth > width)
        labelWidth = width;

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(buffer,
             &(mr_rectangle_t){left, top, labelWidth, height},
             &(mr_point_t){0, offsetY});
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    if (width > labelWidth)
        drawText(time,
                 &(mr_rectangle_t){left + labelWidth, top, width - labelWidth, height},
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

    drawLabeledTime(getString(STRING_START), page->startTime,
                    &(mr_rectangle_t){PADDING_LEFT, ROWS_BOTTOM, elapsedLeft - PADDING_LEFT, TIMES_BOTTOM - ROWS_BOTTOM},
                    TIMES_TEXTTOP - ROWS_BOTTOM);
    drawLabeledTime(getString(STRING_ELAPSED), page->elapsedTime,
                    &(mr_rectangle_t){elapsedLeft, ROWS_BOTTOM, DISPLAY_WIDTH - elapsedLeft, TIMES_BOTTOM - ROWS_BOTTOM},
                    TIMES_TEXTTOP - ROWS_BOTTOM);

    drawRectangle(&(mr_rectangle_t){0, ROWS_BOTTOM, PADDING_LEFT, TIMES_BOTTOM - ROWS_BOTTOM});
}

void drawOverviewPageDots(uint32_t pageIndex, uint32_t pageNum, int16_t top)
{
    if (pageNum == 0)
        return;

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

static void drawCell(const char *key, const char *value, ColorIndex valueColorIndex, int16_t left, int16_t width, int16_t offsetX, DrawTextFunction draw)
{
    setFont(font_tiny);
    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    draw(key,
         &(mr_rectangle_t){left, CELL_LABEL_TOP, width, CELL_VALUE_TOP - CELL_LABEL_TOP},
         &(mr_point_t){offsetX, CELL_LABEL_TEXTTOP - CELL_LABEL_TOP});

    setFont(font_small);
    setStrokeColor(valueColorIndex);
    draw(value,
         &(mr_rectangle_t){left, CELL_VALUE_TOP, width, CELL_BOTTOM - CELL_VALUE_TOP},
         &(mr_point_t){offsetX, CELL_VALUE_TEXTTOP - CELL_VALUE_TOP});
}

static int16_t getCellWidth(const char *key, const char *value)
{
    setFont(font_tiny);
    int16_t width = getTextWidth(key);
    setFont(font_small);
    int16_t valueWidth = getTextWidth(value);

    return (valueWidth > width) ? valueWidth : width;
}

static const char *getCellUnitKey(const char *key)
{
    const char *unit = strrchr(key, ' ');

    return unit ? unit + 1 : key;
}

static void drawCells(const OverviewPage2 *page)
{
    const char *instantaneousKey = page->instantaneousKey;
    const char *averageKey = page->averageKey;

    int16_t leftWidth = getCellWidth(instantaneousKey, page->instantaneousValue);
    int16_t middleWidth = getCellWidth(averageKey, page->averageValue);
    int16_t rightWidth = getCellWidth(page->secondaryKey, page->secondaryValue);
    if (leftWidth + middleWidth + rightWidth + 2 * CELL_GAP > CONTENT_MULTILINE_WIDTH)
    {
        instantaneousKey = getCellUnitKey(instantaneousKey);
        averageKey = getCellUnitKey(averageKey);
        leftWidth = getCellWidth(instantaneousKey, page->instantaneousValue);
        middleWidth = getCellWidth(averageKey, page->averageValue);
    }

    int16_t rightLeft = DISPLAY_WIDTH - PADDING_RIGHT - rightWidth;
    int16_t middleLeft = CELL_MIDDLE_LEFT;
    if (middleLeft + middleWidth + CELL_GAP > rightLeft)
        middleLeft = rightLeft - CELL_GAP - middleWidth;
    if (middleLeft < PADDING_LEFT + leftWidth + CELL_GAP)
        middleLeft = PADDING_LEFT + leftWidth + CELL_GAP;
    if (rightLeft < middleLeft)
        rightLeft = middleLeft;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    drawCell(instantaneousKey, page->instantaneousValue, getValueColorIndex(page->instantaneousStyle),
             0, middleLeft, PADDING_LEFT, drawText);
    drawCell(averageKey, page->averageValue, getRowColorIndex(page->averageStyle),
             middleLeft, rightLeft - middleLeft, 0, drawText);
    drawCell(page->secondaryKey, page->secondaryValue, COLOR_ELEMENT_ACTIVE,
             rightLeft, DISPLAY_WIDTH - rightLeft, DISPLAY_WIDTH - rightLeft - PADDING_RIGHT, drawRightAlignedText);
}

static void drawInfo(const OverviewPage2 *page)
{
    const int16_t height = INFO_BOTTOM - INFO_TOP;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    setFont(font_tiny);

    int16_t confidenceLeft = DISPLAY_WIDTH - PADDING_RIGHT - INFO_GAP - getTextWidth(page->averageConfidence);
    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    drawRightAlignedText(page->averageConfidence,
                         &(mr_rectangle_t){confidenceLeft, INFO_TOP, DISPLAY_WIDTH - confidenceLeft, height},
                         &(mr_point_t){DISPLAY_WIDTH - confidenceLeft - PADDING_RIGHT, 0});

    if (page->faultString)
    {
        setStrokeColor(COLOR_ALARM);
        drawText(page->faultString,
                 &(mr_rectangle_t){0, INFO_TOP, confidenceLeft, height},
                 &(mr_point_t){PADDING_LEFT, 0});
    }
    else
    {
        drawRectangle(&(mr_rectangle_t){0, INFO_TOP, PADDING_LEFT, height});
        drawLabeledTime(getString(STRING_AVERAGE_TIME), page->averageTime,
                        &(mr_rectangle_t){PADDING_LEFT, INFO_TOP, confidenceLeft - PADDING_LEFT, height}, 0);
    }
}

static void addGraphLabel(GraphLabels *labels, const char *text, ColorIndex colorIndex, int16_t x, int32_t baseline, bool rightAligned)
{
    if (labels->num >= GRAPH_LABEL_MAX)
        return;

    int32_t top = baseline - GRAPH_LABEL_ASCENT;
    int32_t bottom = baseline + GRAPH_LABEL_DESCENT;
    if (top < 1)
        top = 1;
    if (bottom > GRAPH_HEIGHT - 1)
        bottom = GRAPH_HEIGHT - 1;
    if (bottom <= top)
        return;

    int16_t textWidth = getTextWidth(text);
    mr_rectangle_t rectangle = {(rightAligned ? x - textWidth : x) - GRAPH_LABEL_PADDING,
                                GRAPH_TOP + top,
                                textWidth + 2 * GRAPH_LABEL_PADDING,
                                bottom - top};

    for (uint32_t i = 0; i < labels->num; i++)
    {
        const mr_rectangle_t *other = &labels->rectangles[i];
        if ((rectangle.x < other->x + other->width) &&
            (other->x < rectangle.x + rectangle.width) &&
            (rectangle.y < other->y + other->height) &&
            (other->y < rectangle.y + rectangle.height))
            return;
    }

    labels->rectangles[labels->num++] = rectangle;

    setStrokeColor(colorIndex);
    drawText(text,
             &rectangle,
             &(mr_point_t){GRAPH_LABEL_PADDING, baseline - FONT_TINY_ASCENT - top});
}

static int32_t getGraphY(const HistoryAxis *axis, float value)
{
    int32_t y = getHistoryAxisY(axis, value, GRAPH_HEIGHT);
    if (y < 0)
        y = 0;
    else if (y > GRAPH_HEIGHT - 1)
        y = GRAPH_HEIGHT - 1;

    return y;
}

static int32_t getGraphZoneY(const HistoryAxis *axis, float value)
{
    if ((value <= 0) || (value > axis->top))
        return -1;

    return getGraphY(axis, value);
}

static int32_t getGraphLineY(const HistoryAxis *axis, float value)
{
    if ((value <= axis->bottom) || (value >= axis->top))
        return -1;

    return getGraphY(axis, value);
}

static int32_t getGraphDotY(const OverviewGraph *graph, const HistoryAxis *axis)
{
    uint8_t lastValue = graph->logValues[GRAPH_WIDTH - 1];
    if (!lastValue)
        return -1;

    int32_t y = getGraphY(axis, getHistoryValueFromLog(lastValue, graph->scale));
    if (y < 1 + GRAPH_DOT_HALO_RADIUS)
        y = 1 + GRAPH_DOT_HALO_RADIUS;
    else if (y > GRAPH_HEIGHT - 2 - GRAPH_DOT_HALO_RADIUS)
        y = GRAPH_HEIGHT - 2 - GRAPH_DOT_HALO_RADIUS;

    return y;
}

static void addGraphAlertLabel(GraphLabels *labels, const char *text, ColorIndex colorIndex, int32_t lineY, int32_t dotY)
{
    int32_t baseline = lineY - GRAPH_LABEL_BASELINE_GAP;
    int16_t right = GRAPH_LEFT + GRAPH_WIDTH - GRAPH_LABEL_RIGHT;
    if ((dotY >= 0) &&
        (dotY + GRAPH_DOT_HALO_RADIUS >= baseline - GRAPH_LABEL_ASCENT) &&
        (dotY - GRAPH_DOT_HALO_RADIUS < baseline + GRAPH_LABEL_DESCENT))
        right = GRAPH_LEFT + GRAPH_DOT_BIN - GRAPH_DOT_HALO_RADIUS - GRAPH_LABEL_PADDING;

    addGraphLabel(labels, text, colorIndex, right, baseline, true);
}

static void drawGraphLabels(const OverviewGraph *graph, const HistoryAxis *axis, GraphLabels *labels)
{
    int32_t prefixExponent = getHistoryAxisIndexPrefixExponent(axis->lineIndices[axis->lineNum - 1], graph->minPrefixExponent);

    setFont(font_tiny);
    setFillColor(COLOR_INSTRUMENT_FRAME_PRIMARY);

    for (uint32_t i = axis->lineNum; i > 0; i--)
    {
        float value = axis->lineValues[i - 1];
        bool isTop = (i == axis->lineNum);

        char text[32];
        strclr(text);
        if (!strcatHistoryAxisIndexLabel(text, axis->lineIndices[i - 1], prefixExponent))
            continue;
        if (isTop &&
            (strlen(text) + strlen(graph->unitName) + GRAPH_UNIT_EXTRA <= sizeof(text)))
        {
            strcat(text, " ");
            strcatMetricPrefixFromExponent(text, prefixExponent);
            strcat(text, graph->unitName);
        }

        int32_t y = getGraphY(axis, value);
        addGraphLabel(labels, text, COLOR_ELEMENT_NEUTRAL,
                      GRAPH_LEFT + GRAPH_LABEL_LEFT,
                      isTop ? y + GRAPH_TOP_LABEL_BASELINE : y - GRAPH_LABEL_BASELINE_GAP,
                      false);
    }

    int32_t dotY = getGraphDotY(graph, axis);

    int32_t warningY = getGraphLineY(axis, graph->warningValue);
    if (warningY >= 0)
        addGraphAlertLabel(labels, getString(STRING_WARNING), COLOR_WARNING, warningY, dotY);

    int32_t alarmY = getGraphLineY(axis, graph->alarmValue);
    if (alarmY >= 0)
        addGraphAlertLabel(labels, getString(STRING_ALARM), COLOR_ALARM, alarmY, dotY);
}

static void setGraphPixels(mr_color_t *column, int32_t y, int32_t halfHeight, mr_color_t color)
{
    for (int32_t i = y - halfHeight; i <= y + halfHeight; i++)
        if ((i >= 1) && (i < GRAPH_HEIGHT - 1))
            column[i] = color;
}

static void drawGraphColumn(int16_t x, const mr_color_t *line, const GraphLabels *labels)
{
    int16_t y = 0;
    while (y < GRAPH_REGION_HEIGHT)
    {
        int16_t end = GRAPH_REGION_HEIGHT;
        bool skipped = false;

        for (uint32_t i = 0; i < labels->num; i++)
        {
            const mr_rectangle_t *rectangle = &labels->rectangles[i];
            if ((x < rectangle->x) || (x >= rectangle->x + rectangle->width))
                continue;

            int16_t top = rectangle->y - GRAPH_REGION_TOP;
            int16_t bottom = top + rectangle->height;
            if ((top <= y) && (y < bottom))
            {
                y = bottom;
                skipped = true;

                break;
            }
            if ((top > y) && (top < end))
                end = top;
        }

        if (skipped)
            continue;

        drawImage(&(mr_rectangle_t){x, GRAPH_REGION_TOP + y, 1, end - y}, line + y);
        y = end;
    }
}

static void drawGraphColumns(const OverviewGraph *graph, const HistoryAxis *axis, const GraphLabels *labels, ColorIndex dotColorIndex)
{
    mr_color_t backgroundColor = getFillColor(COLOR_CONTAINER_BACKGROUND);
    mr_color_t frameColor = getFillColor(COLOR_INSTRUMENT_FRAME_PRIMARY);
    mr_color_t frameLineColor = getFillColor(COLOR_INSTRUMENT_FRAME_TERTIARY);
    mr_color_t tickColor = getFillColor(COLOR_INSTRUMENT_TICK_MARK_LABEL_SECONDARY);
    mr_color_t warningColor = getFillColor(COLOR_WARNING);
    mr_color_t alarmColor = getFillColor(COLOR_ALARM);
    mr_color_t dotColor = getFillColor(dotColorIndex);
    mr_color_t lineColors[3] = {
        getFillColor(COLOR_INSTRUMENT_ENHANCED_SECONDARY),
        getFillColor(COLOR_INSTRUMENT_ENHANCED_SECONDARY_WARNING),
        getFillColor(COLOR_INSTRUMENT_ENHANCED_SECONDARY_ALARM),
    };
    mr_color_t areaColors[3] = {
        getFillColor(COLOR_INSTRUMENT_ENHANCED_TERTIARY),
        getFillColor(COLOR_INSTRUMENT_ENHANCED_TERTIARY_WARNING),
        getFillColor(COLOR_INSTRUMENT_ENHANCED_TERTIARY_ALARM),
    };

    int32_t gridYs[HISTORY_AXIS_LINE_MAX];
    uint32_t gridNum = 0;
    int32_t warningZoneY = -1;
    int32_t alarmZoneY = -1;
    int32_t warningLineY = -1;
    int32_t alarmLineY = -1;
    int32_t dotY = -1;
    if (axis)
    {
        gridNum = axis->lineNum;
        for (uint32_t i = 0; i < gridNum; i++)
            gridYs[i] = getGraphY(axis, axis->lineValues[i]);

        warningZoneY = getGraphZoneY(axis, graph->warningValue);
        alarmZoneY = getGraphZoneY(axis, graph->alarmValue);
        if (warningZoneY < alarmZoneY)
            warningZoneY = alarmZoneY;
        warningLineY = getGraphLineY(axis, graph->warningValue);
        alarmLineY = getGraphLineY(axis, graph->alarmValue);
        dotY = getGraphDotY(graph, axis);
    }

    mr_color_t line[GRAPH_REGION_HEIGHT];
    mr_color_t *column = line + GRAPH_OFFSETY;
    uint32_t timeTickIndex = 0;
    int16_t nextTimeTick = 0;
    int32_t previousY = 0;
    bool previousData = false;
    for (int16_t x = 0; x < DISPLAY_WIDTH; x++)
    {
        for (int16_t y = 0; y < GRAPH_REGION_HEIGHT; y++)
            line[y] = backgroundColor;

        int16_t bin = x - GRAPH_LEFT;
        if ((bin >= 0) && (bin < GRAPH_WIDTH))
        {
            mr_color_t color = frameColor;
            if (bin == nextTimeTick)
            {
                color = frameLineColor;
                timeTickIndex++;
                nextTimeTick = graph->timeTicksNum ? timeTickIndex * (GRAPH_WIDTH - 1) / graph->timeTicksNum : -1;
            }
            for (int16_t y = 0; y < GRAPH_HEIGHT; y++)
                column[y] = color;

            if (axis)
            {
                uint8_t logValue = graph->logValues[bin];
                int32_t y = getGraphY(axis, getHistoryValueFromLog(logValue, graph->scale));
                if (bin == 0)
                    previousY = y;
                if (logValue || previousData)
                {
                    int32_t lineTop = (y < previousY) ? y : previousY;
                    int32_t lineBottom = ((y > previousY) ? y : previousY) + 1;
                    for (int32_t i = lineTop; i < GRAPH_HEIGHT; i++)
                    {
                        uint32_t zone = (i <= alarmZoneY) ? 2 : (i <= warningZoneY) ? 1 : 0;
                        column[i] = (i <= lineBottom) ? lineColors[zone] : areaColors[zone];
                    }
                }
                previousY = y;
                previousData = (logValue != 0);

                if ((bin & GRAPH_GRID_DASH) == 0)
                    for (uint32_t i = 0; i < gridNum; i++)
                        if ((gridYs[i] != warningLineY) && (gridYs[i] != alarmLineY))
                            column[gridYs[i]] = tickColor;

                if (((bin + GRAPH_ALERT_PERIOD - 1) % GRAPH_ALERT_PERIOD) < GRAPH_ALERT_DASH)
                {
                    if (warningLineY >= 0)
                        column[warningLineY] = warningColor;
                    if (alarmLineY >= 0)
                        column[alarmLineY] = alarmColor;
                }
            }

            if ((bin == 0) || (bin == GRAPH_WIDTH - 1))
                for (int16_t y = 0; y < GRAPH_HEIGHT; y++)
                    column[y] = frameLineColor;
            column[0] = frameLineColor;
            column[GRAPH_HEIGHT - 1] = frameLineColor;

            if (dotY >= 0)
            {
                int32_t dx = bin - GRAPH_DOT_BIN;
                if ((dx >= -GRAPH_DOT_HALO_RADIUS) && (dx <= GRAPH_DOT_HALO_RADIUS))
                    setGraphPixels(column, dotY, graphDotHalo[dx + GRAPH_DOT_HALO_RADIUS], frameColor);
                if ((dx >= -GRAPH_DOT_RADIUS) && (dx <= GRAPH_DOT_RADIUS))
                    setGraphPixels(column, dotY, graphDot[dx + GRAPH_DOT_RADIUS], dotColor);
            }
        }
        else if ((x >= GRAPH_TICKS_LEFT) && (x < GRAPH_TICKS_LEFT + TICKS_LONG))
        {
            for (uint32_t i = 0; i < gridNum; i++)
                column[gridYs[i]] = tickColor;
        }

        drawGraphColumn(x, line, labels);
    }
}

static void drawGraph(const OverviewGraph *graph, ColorIndex dotColorIndex)
{
    uint8_t minLogValue = 0;
    uint8_t maxLogValue = 0;
    for (uint32_t i = 0; i < GRAPH_WIDTH; i++)
    {
        uint8_t logValue = graph->logValues[i];
        if (logValue)
        {
            if (!minLogValue || (logValue < minLogValue))
                minLogValue = logValue;
            if (logValue > maxLogValue)
                maxLogValue = logValue;
        }
    }

    if (minLogValue > GRAPH_MARGIN)
        minLogValue -= GRAPH_MARGIN;

    HistoryAxis axis;
    GraphLabels labels;
    labels.num = 0;
    bool hasAxis = buildHistoryAxis(getHistoryValueFromLog(minLogValue, graph->scale),
                                    getHistoryValueFromLog(maxLogValue, graph->scale),
                                    graph->warningValue,
                                    GRAPH_HEIGHT,
                                    &axis);
    if (hasAxis)
        drawGraphLabels(graph, &axis, &labels);

    drawGraphColumns(graph, hasAxis ? &axis : NULL, &labels, dotColorIndex);
}

static void drawTriangle(int16_t left, bool pointsLeft)
{
    mr_color_t backgroundColor = getFillColor(COLOR_CONTAINER_BACKGROUND);
    mr_color_t color = getFillColor(COLOR_ELEMENT_ACTIVE);
    mr_color_t image[TRIANGLE_WIDTH * AXIS_HEIGHT];

    for (int16_t x = 0; x < TRIANGLE_WIDTH; x++)
    {
        int16_t halfHeight = pointsLeft ? x : (TRIANGLE_WIDTH - 1 - x);
        for (int16_t y = 0; y < AXIS_HEIGHT; y++)
        {
            int16_t distance = y - (TRIANGLE_TOP + TRIANGLE_WIDTH - 1);
            if (distance < 0)
                distance = -distance;
            image[y * TRIANGLE_WIDTH + x] = (distance <= halfHeight) ? color : backgroundColor;
        }
    }

    drawImage(&(mr_rectangle_t){left, AXIS_TOP, TRIANGLE_WIDTH, AXIS_HEIGHT}, image);
}

static void drawTimeAxis(const OverviewGraph *graph)
{
    const int16_t offsetY = AXIS_TEXTTOP - AXIS_TOP;

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    setFont(font_tiny);

    int16_t periodLeft = PADDING_LEFT + TRIANGLE_WIDTH;
    int16_t periodWidth = AXIS_GAP + getTextWidth(graph->periodString) + AXIS_GAP;
    int16_t halfLeft = periodLeft + periodWidth + TRIANGLE_WIDTH;
    int16_t nowLeft = DISPLAY_WIDTH - PADDING_RIGHT - getTextWidth(getString(STRING_NOW));
    if (nowLeft < halfLeft)
        nowLeft = halfLeft;
    int16_t halfTextLeft = GRAPH_LEFT + (GRAPH_WIDTH - getTextWidth(graph->halfString)) / 2;
    if (halfTextLeft < halfLeft + AXIS_GAP)
        halfTextLeft = halfLeft + AXIS_GAP;

    drawRectangle(&(mr_rectangle_t){0, AXIS_TOP, PADDING_LEFT, AXIS_HEIGHT});
    drawTriangle(PADDING_LEFT, true);

    setStrokeColor(COLOR_ELEMENT_ACTIVE);
    drawText(graph->periodString,
             &(mr_rectangle_t){periodLeft, AXIS_TOP, periodWidth, AXIS_HEIGHT},
             &(mr_point_t){AXIS_GAP, offsetY});
    drawTriangle(periodLeft + periodWidth, false);

    setStrokeColor(COLOR_ELEMENT_NEUTRAL);
    drawText(graph->halfString,
             &(mr_rectangle_t){halfLeft, AXIS_TOP, nowLeft - halfLeft, AXIS_HEIGHT},
             &(mr_point_t){halfTextLeft - halfLeft, offsetY});
    drawText(getString(STRING_NOW),
             &(mr_rectangle_t){nowLeft, AXIS_TOP, DISPLAY_WIDTH - nowLeft, AXIS_HEIGHT},
             &(mr_point_t){0, offsetY});
}

void drawOverviewPage2(const OverviewPage2 *page, const OverviewGraph *graph)
{
    drawCells(page);
    drawInfo(page);
    drawGraph(graph, getValueColorIndex(page->instantaneousStyle));
    drawTimeAxis(graph);

    setFillColor(COLOR_CONTAINER_BACKGROUND);
    drawRectangle(&(mr_rectangle_t){0, AXIS_BOTTOM, DISPLAY_WIDTH, PAGE2_DOTS_TOP - AXIS_BOTTOM});
    drawRectangle(&(mr_rectangle_t){0, PAGE2_DOTS_TOP + DOTS_SIZE, DISPLAY_WIDTH, DISPLAY_HEIGHT - PAGE2_DOTS_TOP - DOTS_SIZE});
    drawOverviewPageDots(1, 2, PAGE2_DOTS_TOP);
}

#endif
