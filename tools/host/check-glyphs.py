import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parent.parent.parent
platform = root / "platform.io"
strings_path = platform / "src/system/strings/ko.h"
fonts_path = platform / "src/ui/fonts"
args = [a for a in sys.argv[1:] if not a.startswith("--")]
check_unused = "--check-unused" in sys.argv[1:]
binary_path = Path(args[0]) if args else platform / ".pio/build/bosean-fs5000-ko/firmware.bin"

firmware = binary_path.read_bytes()
ko_text = strings_path.read_text(encoding="utf-8")
definitions = re.findall(r'#define (STRING_\w+) "((?:[^"\\]|\\.)*)"', ko_text)


def unescape(text):
    return text.replace("\\n", "\n").replace('\\"', '"').replace("\\\\", "\\")


def get_codepoints(path):
    codepoints = set()
    for part in re.search(r"Codepoints: (.*)", path.read_text(encoding="utf-8")).group(1).split(","):
        part = part.strip()
        if "-" in part:
            low, high = part.split("-")
            codepoints.update(chr(x) for x in range(int(low, 16), int(high, 16) + 1))
        elif part:
            codepoints.add(chr(int(part, 16)))
    return codepoints


def get_small_excluded_keys():
    script = (root / "tools/build-fonts.py").read_text(encoding="utf-8")
    block = re.search(r"small_excluded_keys = \{(.*?)\}", script, re.DOTALL).group(1)
    keys = set(re.findall(r'"(STRING_\w+)"', block))
    keys.update(re.findall(r"\bSTRING_HISTORY_\w+", ko_text))
    return keys


small = get_codepoints(fonts_path / "font_small_ko_fs5000_color_21.h")
tiny = get_codepoints(fonts_path / "font_tiny_ko_color_14.h")
tiny_only = get_small_excluded_keys()

failed = False
checked = 0
for key, value in definitions:
    text = unescape(value)
    if text.encode("utf-8") not in firmware:
        continue
    checked += 1
    chars = {c for c in text if ord(c) >= 0x20}
    if key in tiny_only:
        missing = chars - tiny
        font = "font_tiny"
    else:
        missing = chars - small
        font = "font_small"
    if missing:
        failed = True
        print(f"{key} missing in {font}: {''.join(sorted(missing))}")

if check_unused:
    # Strings excluded from the FS-5000 fonts (FS5000_UNUSED_KEYS) must not be in the image.
    script = (root / "tools/build-fonts.py").read_text(encoding="utf-8")
    block = re.search(r"FS5000_UNUSED_KEYS = \{(.*?)\}", script, re.DOTALL).group(1)
    values = dict(definitions)
    for key in re.findall(r'"(STRING_\w+)"', block):
        if key not in values:
            print(f"warning: {key} in FS5000_UNUSED_KEYS is not defined in ko.h (renamed upstream?)")
            continue
        if unescape(values[key]).encode("utf-8") in firmware:
            failed = True
            print(f"{key} is in FS5000_UNUSED_KEYS but present in {binary_path.name}")

print(f"checked {checked} strings present in {binary_path.name}")
sys.exit(1 if failed else 0)
