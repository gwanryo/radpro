/*
 * Rad Pro
 * Settings autosave
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#if !defined(SETTINGSSAVE_H) && defined(FS5000)
#define SETTINGSSAVE_H

#include "../system/settings.h"

void markSettingsSaved(void);
void saveSettingsIfChanged(void);

#endif
