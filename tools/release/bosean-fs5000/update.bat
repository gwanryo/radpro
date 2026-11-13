@echo off
rem Rad Pro update script for the Bosean FS-5000 (flashing + automatic USB clock sync).
setlocal enabledelayedexpansion
cd /d "%~dp0"
echo Update keeps your settings and data log - no mass erase.
echo If the new version changes the settings format or firmware size, use install instead.
echo.

rem Resolve the firmware image here so a missing image can never look like success.
set "IMGARG="
if not "%~1" == "" (
    if not exist "%~1" (
        echo Firmware image not found: %~1
        exit /b 2
    )
    set "IMG=%~f1"
    set "IMGARG=!IMG!"
    if not "!IMG!" == "!IMG: =!" set IMGARG="!IMG!"
    goto imageresolved
)
set COUNT=0
for %%F in (firmware\radpro-bosean-fs5000-*.bin) do (
    set /a COUNT+=1
    set "ONE=%%~nxF"
)
if !COUNT! equ 0 (
    echo No firmware image found in %~dp0firmware.
    exit /b 2
)
if !COUNT! equ 1 (
    set "IMGARG=..\bosean-fs5000\firmware\!ONE!"
    goto imageresolved
)
set "LANGUAGE_CODES="
for %%F in (firmware\radpro-bosean-fs5000-*.bin) do (
    for /f "tokens=4 delims=-" %%l in ("%%~nF") do set "LANGUAGE_CODES=!LANGUAGE_CODES! %%l"
)
echo Available language codes:!LANGUAGE_CODES!
set "LANGUAGE_CODE="
set /p LANGUAGE_CODE="Enter language code for Rad Pro installation: "
echo !LANGUAGE_CODE!| findstr /r /x "[A-Za-z_][A-Za-z_]*" >nul
if errorlevel 1 (
    echo Invalid language code.
    exit /b 2
)
set COUNT=0
for %%F in (firmware\radpro-bosean-fs5000-!LANGUAGE_CODE!-*.bin) do (
    set /a COUNT+=1
    set "ONE=%%~nxF"
)
if !COUNT! neq 1 (
    echo Firmware image not found or ambiguous for language !LANGUAGE_CODE!.
    exit /b 2
)
set "IMGARG=..\bosean-fs5000\firmware\!ONE!"
:imageresolved

call ..\lib\flashtool.bat "bosean-fs5000\firmware\radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c ""init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;""" "-f target/stm32l4x.cfg -c ""init; reset halt; program IMAGE_PATH 0x08000000 verify; reset run; exit;""" !IMGARG!
set "FLASH_RC=!ERRORLEVEL!"
rem flashtool.bat changes directory; return to the script folder.
cd /d "%~dp0"
if not "!FLASH_RC!" == "0" (
    echo Flashing did not complete successfully, skipping clock sync.
    exit /b !FLASH_RC!
)
echo.
echo Synchronizing device clock over USB...
powershell -NoProfile -ExecutionPolicy Bypass -File ..\lib\sync-time.ps1
exit /b 0
