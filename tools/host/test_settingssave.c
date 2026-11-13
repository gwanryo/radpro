// SOURCES: system/settingssave.c
#include <assert.h>
#include <string.h>

#include "system/settings.h"
#include "system/settingssave.h"

Settings settings;
static int saveCount;

void saveSettings(void)
{
    saveCount++;
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
    return 0;
}
