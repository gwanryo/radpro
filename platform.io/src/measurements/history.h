/*
 * Rad Pro
 * History
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(HISTORY_H)
#define HISTORY_H

#if defined(FS5000)
#include <stdint.h>
#endif

#include "../ui/view.h"

void resetHistory(void);

void clearHistory(void);

void loadHistory(void);

void updateHistory(void);

#if defined(FS5000)
void selectHistoryTab(int32_t delta);

const uint8_t *getHistoryLogValues(void);

uint32_t getHistoryTimeTicksNum(void);

const char *getHistoryPeriodString(void);

const char *getHistoryHalfString(void);
#endif

void onHistoryViewEvent(ViewEvent event);

#endif
