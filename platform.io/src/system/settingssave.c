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

static Settings savedSettings;

void markSettingsSaved(void)
{
    savedSettings = settings;
}

void saveSettingsIfChanged(void)
{
    if (memcmp(&savedSettings, &settings, sizeof(Settings)) != 0)
        saveSettings();
}
#endif
