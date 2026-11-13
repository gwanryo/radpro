// SOURCES: measurements/dosestart.c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "measurements/dosestart.h"

#define T0 1760000000u

int main(void)
{
    bool provisional = false;
    uint32_t start;

    start = getUpdatedDoseStartTime(0, &provisional, 5, T0, true);
    assert(start == T0 && provisional);

    start = getUpdatedDoseStartTime(start, &provisional, T0, T0 + 3600, true);
    assert(start == T0 + 3600 && provisional);

    start = getUpdatedDoseStartTime(start, &provisional, T0 + 3600, T0 + 3600 - 7200, true);
    assert(start == T0 - 3600 && provisional);

    start = getUpdatedDoseStartTime(start, &provisional, T0 + 100, T0 + 160, true);
    assert(start == T0 - 3540 && provisional);

    provisional = false;
    start = getUpdatedDoseStartTime(0, &provisional, 5, 100, false);
    assert(start == 0 && !provisional);

    provisional = false;
    start = getUpdatedDoseStartTime(T0, &provisional, T0 + 10, T0 + 5000, true);
    assert(start == T0 && !provisional);

    start = getUpdatedDoseStartTime(T0, &provisional, T0 + 10, T0 - 5000, true);
    assert(start == T0 && !provisional);

    return 0;
}
