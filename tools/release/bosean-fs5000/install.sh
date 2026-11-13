#!/bin/bash
# Rad Pro install script for the Bosean FS-5000 (flashing + automatic USB clock sync).
ORIG_DIR=$(pwd)
cd "$(dirname "$0")" || exit 1

# Resolve the firmware image here so a missing image can never look like success.
if [ -n "$1" ]; then
    if [ ! -f "$1" ]; then
        # relative paths are relative to the caller's folder
        [ -f "$ORIG_DIR/$1" ] || { echo "Firmware image not found: $1"; exit 2; }
        IMG="$ORIG_DIR/$1"
    else
        case "$1" in /*) IMG="$1";; *) IMG="$ORIG_DIR/$1";; esac
    fi
else
    FILES=()
    for f in firmware/radpro-bosean-fs5000-*.bin; do
        [ -f "$f" ] && FILES+=("$f")
    done
    if [ ${#FILES[@]} -eq 0 ]; then
        echo "No firmware image found in $(pwd)/firmware."
        exit 2
    elif [ ${#FILES[@]} -eq 1 ]; then
        IMG="../bosean-fs5000/${FILES[0]}"
    else
        CODES=""
        for f in "${FILES[@]}"; do
            n=${f##*/}; n=${n%.bin}
            CODES="$CODES $(echo "$n" | cut -d- -f4)"
        done
        echo "Available language codes:$CODES"
        read -p "Enter language code for Rad Pro installation: " LANGUAGE_CODE
        if ! [[ "$LANGUAGE_CODE" =~ ^[A-Za-z_]+$ ]]; then
            echo "Invalid language code."
            exit 2
        fi
        MATCHES=()
        for f in firmware/radpro-bosean-fs5000-"$LANGUAGE_CODE"-*.bin; do
            [ -f "$f" ] && MATCHES+=("$f")
        done
        if [ ${#MATCHES[@]} -ne 1 ]; then
            echo "Firmware image not found or ambiguous for language $LANGUAGE_CODE."
            exit 2
        fi
        IMG="../bosean-fs5000/${MATCHES[0]}"
    fi
fi

../lib/flashtool.sh "bosean-fs5000/firmware/radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c \"init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;\"" "-f target/stm32l4x.cfg -c \"init; reset halt; stm32l4x mass_erase 0; program IMAGE_PATH 0x08000000; reset run; exit;\"" "$IMG"
rc=$?
if [ $rc -ne 0 ]; then
    echo "Flashing did not complete successfully, skipping clock sync."
    exit $rc
fi
echo
echo "Synchronizing device clock over USB..."
../lib/sync-time.sh
exit 0
