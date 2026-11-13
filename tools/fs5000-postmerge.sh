#!/bin/bash
# FS-5000 Korean build: run once after merging upstream (Gissio/radpro).
#
# Steps: regenerate platformio.ini, regenerate the two FS-5000 fonts, clean
# build, size check, glyph checks, host tests and render. Stops at the first
# failure with a non-zero exit code.
#
# Requirements: the Python used (PYTHON) needs freetype-py, bdflib (font tools)
# and pyelftools (sign.py/build). Override with env vars:
#   PYTHON  default C:/Users/USER/.radpro-pio/Scripts/python if present, else python
#   PIO     default C:/Users/USER/.radpro-pio/Scripts/pio if present, else pio
#
# Compiler warnings are printed and counted but do not fail the script.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
ENVNAME=bosean-fs5000-ko

if [ -z "${PYTHON:-}" ]; then
    if [ -x C:/Users/USER/.radpro-pio/Scripts/python.exe ] || [ -x C:/Users/USER/.radpro-pio/Scripts/python ]; then
        PYTHON=C:/Users/USER/.radpro-pio/Scripts/python
    else
        PYTHON=python
    fi
fi
if [ -z "${PIO:-}" ]; then
    if [ -x C:/Users/USER/.radpro-pio/Scripts/pio.exe ] || [ -x C:/Users/USER/.radpro-pio/Scripts/pio ]; then
        PIO=C:/Users/USER/.radpro-pio/Scripts/pio
    else
        PIO=pio
    fi
fi
export PYTHON PIO

fail() { echo "FAIL: $*" >&2; exit 1; }
step() { echo; echo "== $* =="; }
summary=()

cd "$ROOT" || fail "cannot cd to $ROOT"

step "1/7 regenerate platformio.ini"
(cd tools && "$PYTHON" build-env.py) || fail "build-env.py"
summary+=("platformio.ini regenerated")

step "2/7 regenerate FS-5000 fonts"
(cd tools && "$PYTHON" build-fonts.py --fs5000-only) || fail "build-fonts.py --fs5000-only"
# fontconv writes platform-default newlines: keep the repo's CRLF for the two files
for f in font_small_ko_fs5000_color_21.h font_tiny_ko_color_14.h; do
    sed -i -e 's/\o15$//' -e 's/$/\o15/' "platform.io/src/ui/fonts/$f" || fail "CRLF normalize $f"
done
other=$(git status --porcelain platform.io/src/ui/fonts | grep -v -e 'font_small_ko_fs5000_color_21\.h$' -e 'font_tiny_ko_color_14\.h$')
[ -z "$other" ] || fail "font files other than the two FS-5000 headers changed:
$other"
summary+=("fonts regenerated (2 files)")

step "3/7 clean build"
(cd platform.io && "$PIO" run -e $ENVNAME -t clean) || fail "pio clean"
LOG="$(mktemp)"
(cd platform.io && "$PIO" run -e $ENVNAME) 2>&1 | tee "$LOG"
[ "${PIPESTATUS[0]}" -eq 0 ] || { rm -f "$LOG"; fail "pio build"; }
warnings=$(grep -c "warning:" "$LOG")
rm -f "$LOG"
echo "compiler warnings: $warnings"
summary+=("build ok, $warnings warnings")

step "4/7 firmware size"
BIN="platform.io/.pio/build/$ENVNAME/firmware.bin"
[ -f "$BIN" ] || fail "$BIN missing"
fwsize=$(awk -v env="[env:$ENVNAME]" '
    $0 ~ /^\[/ { inenv = (index($0, env) == 1) }
    inenv && /FIRMWARE_SIZE=/ { match($0, /FIRMWARE_SIZE=[0-9a-fA-Fx]+/); print substr($0, RSTART + 14, RLENGTH - 14); exit }
' platform.io/platformio.ini | tr -d '\r')
[ -n "$fwsize" ] || fail "FIRMWARE_SIZE not found for $ENVNAME in platform.io/platformio.ini"
limit=$((fwsize - 4))
size=$(stat -c %s "$BIN")
echo "firmware.bin = $size B, limit = $limit B (FIRMWARE_SIZE $fwsize - 4), free = $((limit - size)) B"
[ "$size" -le "$limit" ] || fail "firmware.bin exceeds limit by $((size - limit)) B"
summary+=("size $size / $limit, free $((limit - size)) B")

step "5-6/7 glyph coverage and unused-string check"
"$PYTHON" tools/host/check-glyphs.py --check-unused || fail "check-glyphs.py (missing glyphs, or a FS5000_UNUSED_KEYS string is now in the image: remove it from the list in tools/build-fonts.py and rerun)"
summary+=("glyphs ok, no unused string in image")

step "7/7 host tests and render"
bash tools/host/run-tests.sh || fail "run-tests.sh"
bash tools/host/render.sh || fail "render.sh (magenta pixels outside strings, or build error)"
summary+=("host tests ok, render magenta=0")

step "summary"
git status --short platform.io/src/ui/fonts
for line in "${summary[@]}"; do echo "- $line"; done
echo "OK"
