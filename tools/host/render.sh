#!/bin/bash
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
P="$ROOT/platform.io"
OUT="$HERE/out"
PARTIAL="strings"

mkdir -p "$OUT"
rm -f "$OUT"/*.ppm "$OUT"/*.png

gcc -std=gnu11 -O1 -w -DSIMULATOR -DFS5000 -DDISPLAY_COLOR -DDISPLAY_240X320 -DKEYBOARD_3_KEYS \
    -DBUZZER -DVIBRATOR -DPULSE_LED -DALERT_LED -DGAME -DLANGUAGE='"ko"' \
    -DSTRINGS='"strings/ko.h"' \
    -DFONT_SMALL='"fonts/font_small_ko_color_21.h"' \
    -DFONT_MEDIUM='"fonts/font_medium_ko_color_32.h"' \
    -DFONT_LARGE='"fonts/font_large_color_84.h"' \
    -DFONT_SYMBOLS='"fonts/font_symbols_color.h"' \
    -DFONT_TINY='"fonts/font_tiny_ko_color_14.h"' \
    -DFONT_VALUE='"fonts/font_value_color_54.h"' \
    -DFONT_SYMBOLS_SMALL='"fonts/font_symbols_color_16.h"' \
    -I"$P/lib/mcu-renderer" -I"$P/src" "$HERE/harness.c" \
    "$P/lib/mcu-renderer/mcu-renderer.c" "$P/lib/mcu-renderer/mcu-renderer-st7789.c" \
    "$P/src/ui/draw.c" "$P/src/ui/measurements.c" "$P/src/ui/system.c" \
    "$P/src/system/cstring.c" "$P/src/system/cmath.c" -lm -o "$OUT/harness.exe" || exit 1

(cd "$OUT" && ./harness.exe) || exit 1

status=0
for ppm in "$OUT"/*.ppm; do
    name="$(basename "$ppm" .ppm)"
    magenta="$(python "$HERE/ppm2png.py" "$ppm" "$OUT/$name.png")" || exit 1
    echo "$name: magenta=$magenta"
    case " $PARTIAL " in
        *" $name "*) ;;
        *) [ "$magenta" -ne 0 ] && status=1 ;;
    esac
done
exit $status
