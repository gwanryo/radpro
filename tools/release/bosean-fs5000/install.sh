#!/bin/bash
# Rad Pro install script for the Bosean FS-5000 (flashing + automatic USB clock sync).
cd "$(dirname "$0")"

../lib/flashtool.sh "bosean-fs5000/firmware/radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c \"init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;\"" "-f target/stm32l4x.cfg -c \"init; reset halt; stm32l4x mass_erase 0; program IMAGE_PATH 0x08000000; reset run; exit;\"" $1
rc=$?
if [ $rc -ne 0 ]; then
    echo "Flashing did not complete successfully, skipping clock sync."
    exit $rc
fi
echo
echo "Synchronizing device clock over USB..."
../lib/sync-time.sh
exit 0
