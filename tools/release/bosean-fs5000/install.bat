@echo off
rem Rad Pro install script for the Bosean FS-5000 (flashing + automatic USB clock sync).

rem Optional image argument (e.g. a backup to restore): make it absolute before changing
rem folders. This runs before delayed expansion is enabled so that "!" stays literal.
rem flashtool.bat passes the path unquoted as a batch argument into the OpenOCD (Tcl)
rem command, so reject characters that would break it: spaces and ! ( ) & ^ %% , ; = * ? $ [ ] { }
rem (the space must be the last delimiter in the for /f options).
rem %~dp0 can be re-resolved against the current folder after a cd when the script
rem was started by a relative path, so remember the script folder first.
setlocal
set "HERE=%~dp0"
set "ARG="
if "%~1" == "" goto argchecked
set "ARG=%~f1"
if not exist "%ARG%" goto argnotfound
for /f "tokens=2 delims=!()&^%%,;=*?$[]{} " %%x in ("x%ARG%x") do goto argbad
:argchecked

setlocal enabledelayedexpansion
cd /d "!HERE!"
echo.

rem Resolve the firmware image here so a missing image can never look like success.
rem Images inside the package are passed relative to flashtool's folder (lib), so the
rem path never contains spaces wherever the package is extracted.
set "IMGARG="
if defined ARG (
    set "IMGARG=!ARG!"
    goto imageresolved
)
set COUNT=0
for %%F in (firmware\radpro-bosean-fs5000-*.bin) do (
    set /a COUNT+=1
    set "ONE=%%~nxF"
)
if !COUNT! equ 0 (
    echo No firmware image found in the firmware folder.
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
if not defined LANGUAGE_CODE goto langbad
rem Letters and "_" only: strip them (case-insensitive) and require nothing to remain.
rem No pipes here: pipe segments run in a child cmd.exe without delayed expansion.
set "REST=!LANGUAGE_CODE!"
for %%c in (a b c d e f g h i j k l m n o p q r s t u v w x y z _) do if defined REST set "REST=!REST:%%c=!"
if defined REST goto langbad
rem Accept only if the code resolves to exactly one existing image.
set COUNT=0
set "ONE="
for %%F in (firmware\radpro-bosean-fs5000-!LANGUAGE_CODE!-*.bin) do (
    set /a COUNT+=1
    set "ONE=%%~nxF"
)
if !COUNT! neq 1 goto langbad
if not exist "firmware\!ONE!" goto langbad
set "IMGARG=..\bosean-fs5000\firmware\!ONE!"

:imageresolved
call ..\lib\flashtool.bat "bosean-fs5000\firmware\radpro-bosean-fs5000" "-f target/stm32l4x.cfg -c ""init; reset halt; dump_image BACKUP_PATH 0x08000000 0x40000; exit;""" "-f target/stm32l4x.cfg -c ""init; reset halt; stm32l4x mass_erase 0; program IMAGE_PATH 0x08000000; reset run; exit;""" !IMGARG!
set "FLASH_RC=!ERRORLEVEL!"
rem flashtool.bat changes directory; return to the script folder.
cd /d "!HERE!"
if not "!FLASH_RC!" == "0" (
    echo Flashing did not complete successfully, skipping clock sync.
    exit /b !FLASH_RC!
)
echo.
echo Synchronizing device clock over USB...
powershell -NoProfile -ExecutionPolicy Bypass -File ..\lib\sync-time.ps1
exit /b 0

:langbad
echo Invalid language code or no matching firmware image.
exit /b 2

:argnotfound
echo Firmware image not found: "%ARG%"
exit /b 2

:argbad
echo The image path must not contain spaces or any of ! ( ) ^& ^^ %% , ; = * ? $ [ ] { }
echo   "%ARG%"
echo Copy the image to a folder without them, or run without an argument to use the packaged firmware.
exit /b 2
