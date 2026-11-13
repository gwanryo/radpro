#!/bin/bash
# Rad Pro update script for the Bosean FS-5000 (flashing + automatic USB clock sync).
cd "$(dirname "$0")"
echo "Update keeps your settings and data log (no mass erase)."
echo "If the new version changes the settings format or firmware size, use install instead."
echo
../lib/flashtool.sh "bosean-fs5000/firmware/radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c \"init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;\"" "-f target/stm32l4x.cfg -c \"init; reset halt; program IMAGE_PATH 0x08000000 verify; reset run; exit;\"" $1
rc=$?
if [ $rc -ne 0 ]; then
    echo "Flashing did not complete successfully, skipping clock sync."
    exit $rc
fi
echo
echo "Synchronizing device clock over USB..."
../lib/sync-time.sh
exit 0
