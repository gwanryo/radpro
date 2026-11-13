/*
 * Rad Pro
 * Overview
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(OVERVIEW_H)
#define OVERVIEW_H

#if defined(FS5000)

#include "../ui/view.h"

void resetOverview(void);

void selectOverviewFirstPage(void);

void onOverviewViewEvent(ViewEvent event);

#endif

#endif
