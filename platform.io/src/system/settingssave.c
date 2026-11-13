/*
 * Rad Pro
 * Settings autosave
 *
 * (C) 2022-2026 Gissio
 *
 * License: MIT
 */

#include "../system/settingssave.h"

#if defined(FS5000)
#include <string.h>

#include "../measurements/cumulative.h"

static Settings savedSettings;
static uint32_t savedDoseStartTime;

void markSettingsSaved(void)
{
    savedSettings = settings;
    savedDoseStartTime = getCumulativeDoseStartTime();
}

void saveSettingsIfChanged(void)
{
    if (memcmp(&savedSettings, &settings, sizeof(Settings)) != 0 ||
        savedDoseStartTime != getCumulativeDoseStartTime())
        saveSettings();
}
#endif
