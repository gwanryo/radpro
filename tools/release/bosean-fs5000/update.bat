@echo off
rem Rad Pro update script for the Bosean FS-5000 (flashing + automatic USB clock sync).
cd /d "%~dp0"
echo Update keeps your settings and data log (no mass erase).
echo If the new version changes the settings format or firmware size, use install instead.
echo.
rem flashtool.bat exits with the OpenOCD error code on failure and leaves 0 on success.
call ..\lib\flashtool.bat "bosean-fs5000\firmware\radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c ""init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;""" "-f target/stm32l4x.cfg -c ""init; reset halt; program IMAGE_PATH 0x08000000 verify; reset run; exit;""" %1
set "FLASH_RC=%ERRORLEVEL%"
rem flashtool.bat changes directory; return to the script folder.
cd /d "%~dp0"
if not "%FLASH_RC%" == "0" (
    echo Flashing did not complete successfully, skipping clock sync.
    exit /b %FLASH_RC%
)
echo.
echo Synchronizing device clock over USB...
powershell -NoProfile -ExecutionPolicy Bypass -File ..\lib\sync-time.ps1
exit /b 0
