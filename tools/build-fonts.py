# Rad Pro
# Builds Rad Pro fonts
#
# (C) 2022-2026 Gissio
#
# License: MIT
#

import os
from pathlib import Path
import re
import sys

import fontconv
import textproc


def get_radpro_path():
    """
    Get the root directory path of the RadPro project.

    str: The relative path to the RadPro project root directory.
    """
    tools_path, _ = os.path.split(__file__)

    return tools_path + "/../"


def run_fontconv(
    source,
    codepoint_set,
    dest,
    name,
    pixels=None,
    bpp=None,
    ascent=None,
    descent=None,
    cap_height=None,
):
    """
    Convert a font file using the fontconv tool with specified parameters.

    Args:
        source (str): The source font file name located in the fonts directory.
        codepoint_set (str): The set of codepoints to include in the converted font.
        dest (str): The destination file name for the converted font in the platform.io UI fonts directory.
        name (str): The name identifier for the converted font.
        pixels (int, optional): The pixel size of the font. Defaults to None.
        bpp (int, optional): Bits per pixel for the font. Defaults to None.
        ascent (int, optional): The ascent value for the font. Defaults to None.
        descent (int, optional): The descent value for the font. Defaults to None.
        cap_height (int, optional): The cap height value for the font. Defaults to None.
    """
    args = []
    args += ["-s", codepoint_set]
    args += ["-n", name]
    if pixels:
        args += ["-p", str(pixels)]
    if bpp:
        args += ["-b", str(bpp)]
    if ascent:
        args += ["-a", str(ascent)]
    if descent:
        args += ["-d", str(descent)]
    if cap_height:
        args += ["-c", str(cap_height)]
    args += [get_radpro_path() + "fonts/" + source]
    args += [get_radpro_path() + "platform.io/src/ui/fonts/" + dest]
    sys.argv[1:] = args
    fontconv.main()


def get_codepoint_set(text, codepoint_set=""):
    """
    Generate a comma-separated string of codepoints from text and an existing codepoint set.
    This function combines codepoints from a provided codepoint set string with
    codepoints extracted from the given text. It filters C-style strings from the
    text and returns the unified set of codepoints as a formatted string.
    Args:
        text: Input text or iterable of text to extract codepoints from.
        codepoint_set (str, optional): A string representation of codepoints to include.
                                        Defaults to an empty string.
    Returns:
        str: A comma-separated string of codepoints representing the combined set
             of codepoints from both the input text and the codepoint_set parameter.
    """
    text = "".join(text)

    codepoints = textproc.parse_codepoint_set(codepoint_set)

    text = textproc.filter_c_strings(text)

    codepoints.update({ord(char) for char in text})

    return ",".join(textproc.build_codepoint_set(codepoints))


# Codepoint sets
CODEPOINT_SET_SYMBOLS_MONOCHROME = "0x30-0x3e,0x41-0x46,0x49-0x4e,0x50-0x56"
CODEPOINT_SET_SYMBOLS_COLOR = "0x30-0x3e,0x41-0x46"
CODEPOINT_SET_SYMBOLS_COLOR_LOW = "0x30-0x3e"
CODEPOINT_SET_LARGE = "0x2e,0x30-0x39,0x2012"

# Common fonts
run_fontconv(
    "RadPro-Symbols8.bdf",
    CODEPOINT_SET_SYMBOLS_MONOCHROME,
    "font_symbols_monochrome.h",
    "font_symbols",
)
run_fontconv(
    "RadPro-Symbols.ttf",
    CODEPOINT_SET_SYMBOLS_COLOR,
    "font_symbols_color.h",
    "font_symbols",
    pixels=24,
)
run_fontconv(
    "RadPro-Symbols.ttf",
    CODEPOINT_SET_SYMBOLS_COLOR_LOW,
    "font_symbols_color_2bpp.h",
    "font_symbols",
    pixels=24,
    bpp=2,
)
run_fontconv(
    "RadPro-Symbols.ttf",
    CODEPOINT_SET_SYMBOLS_COLOR_LOW,
    "font_symbols_color_1bpp.h",
    "font_symbols",
    pixels=24,
    bpp=1,
)

run_fontconv(
    "RadPro-Sans33-Digits.bdf",
    CODEPOINT_SET_LARGE,
    "font_large_monochrome.h",
    "font_large",
)
run_fontconv(
    "NotoSans-SemiBold.ttf",
    CODEPOINT_SET_LARGE,
    "font_large_color_115.h",
    "font_large",
    pixels=115,
    cap_height=82,
)
run_fontconv(
    "NotoSans-SemiBold.ttf",
    CODEPOINT_SET_LARGE,
    "font_large_color_115_1bpp.h",
    "font_large",
    pixels=115,
    bpp=1,
    cap_height=82,
)
run_fontconv(
    "NotoSans-SemiBold.ttf",
    CODEPOINT_SET_LARGE,
    "font_large_color_115_2bpp.h",
    "font_large",
    pixels=115,
    bpp=2,
    cap_height=82,
)
run_fontconv(
    "NotoSans-SemiBold.ttf",
    CODEPOINT_SET_LARGE,
    "font_large_color_84.h",
    "font_large",
    pixels=84,
    cap_height=60,
)

run_fontconv(
    "NotoSans-SemiBold.ttf",
    CODEPOINT_SET_LARGE,
    "font_value_color_54.h",
    "font_value",
    pixels=54,
)
run_fontconv(
    "RadPro-Symbols.ttf",
    CODEPOINT_SET_SYMBOLS_COLOR,
    "font_symbols_color_16.h",
    "font_symbols_small",
    pixels=16,
)


# Languages
for language_file in Path("../platform.io/src/system/strings").glob("*.h"):
    language = language_file.stem

    # Get codepoint sets using textproc
    language_text = open(language_file, "rt", encoding="utf-8").readlines()

    font_medium_bpp_low_matches = [
        "STRING_NANO",
        "STRING_MICRO",
        "STRING_MILLI",
        "STRING_KILO",
        "STRING_MEGA",
        "STRING_GIGA",
        "STRING_SV",
        "STRING_SVH",
        "STRING_REM",
        "STRING_REMH",
        "STRING_CPM",
        "STRING_CPS",
        "STRING_COUNT",
        "STRING_COUNTS",
    ]

    font_medium_matches = font_medium_bpp_low_matches.copy() + [
        "STRING_VOLT_PER_METER_UNIT",
        "STRING_TESLA_UNIT",
        "STRING_GAUSS_UNIT",
    ]

    medium_text = [
        line
        for line in language_text
        if any(match in line for match in font_medium_matches)
    ]

    medium_1bpp_text = [
        line
        for line in language_text
        if any(match in line for match in font_medium_bpp_low_matches)
    ]

    codepoint_set_small = get_codepoint_set(language_text, "0x20-0x7e")
    codepoint_set_monochrome_medium = get_codepoint_set(language_text)
    codepoint_set_color_medium = get_codepoint_set(medium_text)
    codepoint_set_color_medium_1bpp = get_codepoint_set(medium_1bpp_text)

    # Set font paths based on language
    if language == "ja":
        font_small_monochrome = "QuanPixel-7.bdf"
        font_medium_monochrome = "FusionPixel-12.bdf"
        font_color = "NotoSansJP-SemiBold.ttf"
    elif language == "ko":
        font_small_monochrome = "QuanPixel-7.bdf"
        font_medium_monochrome = "FusionPixel-12.bdf"
        font_color = "NotoSansKR-SemiBold.ttf"
    elif language == "zh_CN":
        font_small_monochrome = "QuanPixel-7.bdf"
        font_medium_monochrome = "FusionPixel-12.bdf"
        font_color = "NotoSansSC-SemiBold.ttf"
    else:
        font_small_monochrome = "Tiny5.bdf"
        font_medium_monochrome = "RadPro-Sans8.bdf"
        font_color = "NotoSans-SemiBold.ttf"

    if language in ["ja", "ko", "zh_CN"]:
        font_medium_color_32_ascent = 36
        font_medium_color_32_descent = 9
        font_medium_color_32_cap_height = 25
        font_small_color_21_ascent = 23
        font_small_color_21_descent = 7
        font_small_color_21_cap_height = 16
    else:
        font_medium_color_32_ascent = None
        font_medium_color_32_descent = None
        font_medium_color_32_cap_height = None
        font_small_color_21_ascent = None
        font_small_color_21_descent = None
        font_small_color_21_cap_height = None

    if language in ["en"]:
        bpp_low = 2
    else:
        bpp_low = 1

    # Language-specific fonts
    run_fontconv(
        font_medium_monochrome,
        codepoint_set_monochrome_medium,
        f"font_medium_{language}_monochrome.h",
        "font_medium",
    )
    run_fontconv(
        font_color,
        codepoint_set_color_medium,
        f"font_medium_{language}_color_24.h",
        "font_medium",
        pixels=24,
    )
    run_fontconv(
        font_color,
        codepoint_set_color_medium,
        f"font_medium_{language}_color_32.h",
        "font_medium",
        pixels=32,
        ascent=font_medium_color_32_ascent,
        descent=font_medium_color_32_descent,
        cap_height=font_medium_color_32_cap_height,
    )
    run_fontconv(
        font_color,
        codepoint_set_color_medium_1bpp,
        f"font_medium_{language}_color_32_{bpp_low}bpp.h",
        "font_medium",
        pixels=32,
        bpp=bpp_low,
        ascent=font_medium_color_32_ascent,
        descent=font_medium_color_32_descent,
        cap_height=font_medium_color_32_cap_height,
    )

    run_fontconv(
        font_small_monochrome,
        codepoint_set_small,
        f"font_small_{language}_monochrome.h",
        "font_small",
    )
    run_fontconv(
        font_color,
        codepoint_set_small,
        f"font_small_{language}_color_16.h",
        "font_small",
        pixels=16,
    )
    run_fontconv(
        font_color,
        codepoint_set_small,
        f"font_small_{language}_color_21.h",
        "font_small",
        pixels=21,
        ascent=font_small_color_21_ascent,
        descent=font_small_color_21_descent,
        cap_height=font_small_color_21_cap_height,
    )
    run_fontconv(
        font_color,
        codepoint_set_small,
        f"font_small_{language}_color_21_{bpp_low}bpp.h",
        "font_small",
        pixels=21,
        bpp=bpp_low,
        ascent=font_small_color_21_ascent,
        descent=font_small_color_21_descent,
        cap_height=font_small_color_21_cap_height,
    )


# FS-5000 strings not compiled into the firmware image
FS5000_UNUSED_KEYS = {
    "STRING_NUMBERS",
    "STRING_KHZ",
    "STRING_VOLT_PER_METER_UNIT",
    "STRING_TESLA",
    "STRING_GAUSS",
    "STRING_NOTIFICATION_HVCUSTOM",
    "STRING_HISTORY",
    "STRING_ELECTRIC_FIELD",
    "STRING_MAGNETIC_FIELD",
    "STRING_ALERT_DONE",
    "STRING_DATAMODE",
    "STRING_VOICE",
    "STRING_ELECTRIC_FIELD_ALARM",
    "STRING_MAGNETIC_FIELD_ALARM",
    "STRING_MAGNETIC_FIELD_UNITS",
    "STRING_HVPROFILE",
    "STRING_FACTORY_DEFAULT",
    "STRING_ACCURACY",
    "STRING_ENERGY_SAVING",
    "STRING_CUSTOM",
    "STRING_PWMFREQUENCY",
    "STRING_PWMDUTY_CYCLE",
    "STRING_CONTRAST",
    "STRING_ALWAYS_OFF",
    "STRING_PULSEVOLUME",
    "STRING_ALERTVOLUME",
    "STRING_VOICEVOLUME",
    "STRING_NI_MH",
    "STRING_ALKALINE",
    "STRING_TUBE_LIFE",
    "STRING_DEVICE_ID",
}

# FS-5000 tiny font (Korean)
def get_tiny_font_keys():
    keys = {
        "STRING_ADAPTIVE_FAST",
        "STRING_ADAPTIVE_PRECISION",
        "STRING_1_MINUTE",
        "STRING_30_SECONDS",
        "STRING_10_SECONDS",
        "STRING_START",
        "STRING_ELAPSED",
        "STRING_AVERAGE_TIME",
        "STRING_NOW",
        "STRING_WARNING",
        "STRING_ALARM",
        "STRING_ALERT_FAULT",
        "STRING_INSTANTANEOUS",
        "STRING_AVERAGE",
        "STRING_COUNT",
        "STRING_COUNTS",
    }

    source_path = Path("../platform.io/src")
    source_text = ""
    for source_file in source_path.glob("**/*.c"):
        source_text += open(source_file, "rt", encoding="utf-8").read() + "\n"

    keys.update(re.findall(r"\bSTRING_HISTORY_\w+", open(source_path / "system/strings/ko.h", "rt", encoding="utf-8").read()))
    menu_loose = re.findall(r"\bMenu\s+(\w+)\s*=\s*\{", source_text)
    menu_titles = re.findall(r"\bMenu\s+\w+\s*=\s*\{\s*(?:getString\()?(STRING_\w+)", source_text)
    if len(menu_loose) != len(menu_titles):
        raise Exception(
            f"Tiny font: {len(menu_loose)} Menu initializers found but only {len(menu_titles)} have a STRING_ title"
        )
    keys.update(menu_titles)
    keys.update(re.findall(r"drawTitleBar\(getString\((STRING_\w+)\)\)", source_text))
    keys.update(re.findall(r"drawNotification\(getString\((STRING_\w+)\)", source_text))

    rng_options = re.search(r"rngModeMenuOptions\[\]\s*=\s*\{(.*?)\};", source_text, re.DOTALL)
    if rng_options is None:
        raise Exception("Tiny font: rngModeMenuOptions[] not found in sources")
    keys.update(re.findall(r"STRING_\w+", rng_options.group(1)))

    return keys


tiny_keys = get_tiny_font_keys() - FS5000_UNUSED_KEYS
ko_lines = open("../platform.io/src/system/strings/ko.h", "rt", encoding="utf-8").readlines()
tiny_text = [
    line
    for line in ko_lines
    if line.startswith("#define ") and line.split()[1] in tiny_keys
]
found_keys = {line.split()[1] for line in tiny_text}
missing_keys = tiny_keys - found_keys
if missing_keys:
    raise Exception(f"Tiny font: strings not found: {sorted(missing_keys)}")

codepoint_set_tiny = get_codepoint_set(tiny_text, "0x20-0x7e,0xb1,0xb5")

run_fontconv(
    "NotoSansKR-SemiBold.ttf",
    codepoint_set_tiny,
    "font_tiny_ko_color_14.h",
    "font_tiny",
    pixels=14,
)

# FS-5000 small font (Korean)
small_excluded_keys = {
    "STRING_START",
    "STRING_ELAPSED",
    "STRING_AVERAGE_TIME",
    "STRING_NOW",
    "STRING_WARNING",
    "STRING_ALARM",
}
small_excluded_keys.update(FS5000_UNUSED_KEYS)
small_excluded_keys.update(
    key for key in re.findall(r"\bSTRING_HISTORY_\w+", "".join(ko_lines))
)
small_text = [
    line
    for line in ko_lines
    if not (line.startswith("#define ") and line.split()[1] in small_excluded_keys)
]

run_fontconv(
    "NotoSansKR-SemiBold.ttf",
    get_codepoint_set(small_text, "0x20-0x7e"),
    "font_small_ko_fs5000_color_21.h",
    "font_small",
    pixels=21,
    ascent=23,
    descent=7,
    cap_height=16,
)
