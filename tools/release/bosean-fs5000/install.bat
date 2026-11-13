@echo off
rem Rad Pro install script for the Bosean FS-5000 (flashing + automatic USB clock sync).
cd /d "%~dp0"

rem flashtool.bat exits with the OpenOCD error code on failure and leaves 0 on success.
call ..\lib\flashtool.bat "bosean-fs5000\firmware\radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c ""init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;""" "-f target/stm32l4x.cfg -c ""init; reset halt; stm32l4x mass_erase 0; program IMAGE_PATH 0x08000000; reset run; exit;""" %1
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
