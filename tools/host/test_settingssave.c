// SOURCES: system/settingssave.c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "system/settings.h"
#include "system/settingssave.h"

Settings settings;
static int saveCount;
static bool failNext;
static uint32_t startTime;

uint32_t getCumulativeDoseStartTime(void)
{
    return startTime;
}

void saveSettings(void)
{
    saveCount++;
    if (failNext)
    {
        failNext = false;

        return;
    }
    markSettingsSaved();
}

int main(void)
{
    memset(&settings, 0, sizeof(settings));
    markSettingsSaved();

    saveSettingsIfChanged();
    saveSettingsIfChanged();
    assert(saveCount == 0);

    settings.displayTheme = 1;
    saveSettingsIfChanged();
    assert(saveCount == 1);
    saveSettingsIfChanged();
    assert(saveCount == 1);

    settings.rateAlarm = 3;
    settings.tubeType = 1;
    saveSettingsIfChanged();
    assert(saveCount == 2);
    saveSettingsIfChanged();
    assert(saveCount == 2);

    Settings other = settings;
    other.pulseSound = !other.pulseSound;
    settings = other;
    saveSettingsIfChanged();
    assert(saveCount == 3);

    settings.pulseSound = !settings.pulseSound;
    markSettingsSaved();
    saveSettingsIfChanged();
    assert(saveCount == 3);

    failNext = true;
    settings.rateWarning = 4;
    saveSettingsIfChanged();
    assert(saveCount == 4);
    saveSettingsIfChanged();
    assert(saveCount == 5);
    saveSettingsIfChanged();
    assert(saveCount == 5);

    startTime = 1760000000u;
    saveSettingsIfChanged();
    assert(saveCount == 6);
    saveSettingsIfChanged();
    assert(saveCount == 6);

    failNext = true;
    startTime = 0;
    saveSettingsIfChanged();
    saveSettingsIfChanged();
    assert(saveCount == 8);
    saveSettingsIfChanged();
    assert(saveCount == 8);

    memset(&settings, 0xFF, sizeof(settings));
    markSettingsSaved();
    saveSettingsIfChanged();
    assert(saveCount == 8);
    settings.pulseLED = !settings.pulseLED;
    saveSettingsIfChanged();
    assert(saveCount == 9);
    saveSettingsIfChanged();
    assert(saveCount == 9);

    return 0;
}
