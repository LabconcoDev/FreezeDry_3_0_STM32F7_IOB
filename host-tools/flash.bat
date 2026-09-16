@echo off
setlocal enabledelayedexpansion

:: =======================================================
::         STM32F723 ST-Link Flashing Script
:: =======================================================

:: Define paths
set "HEX_FILE=%~dp0..\my-app\build\zephyr\zephyr.hex"

:: 1. Find STM32CubeProgrammer CLI
if defined STM32_PROG_CLI (
    set "PROG_CLI=%STM32_PROG_CLI%"
) else if exist "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" (
    set "PROG_CLI=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
) else (
    set "PROG_CLI=STM32_Programmer_CLI.exe"
)

echo =======================================================
echo         STM32F723 ST-Link Flashing Script
echo =======================================================
echo.

:: Check if the executable exists or is in PATH
where "%PROG_CLI%" >nul 2>nul
if %errorlevel% neq 0 (
    if not exist "%PROG_CLI%" (
        echo [ERROR] STM32CubeProgrammer CLI was not found.
        echo Please install STM32CubeProgrammer, add it to your PATH, 
        echo or set the STM32_PROG_CLI environment variable to the executable path.
        goto :error
    )
)

:: 2. Check if the build artifact exists
if not exist "%HEX_FILE%" (
    echo [ERROR] Firmware HEX file not found:
    echo        "%HEX_FILE%"
    echo.
    echo Please build the project first using VS Code tasks or powershell.
    goto :error
)

echo Programmer CLI: "%PROG_CLI%"
echo Firmware File:  "%HEX_FILE%"
echo.
echo Connecting via ST-Link (SWD) and programming...
echo.

:: 3. Run STM32CubeProgrammer CLI
:: Options:
::   -c port=SWD : Connect via Serial Wire Debug (SWD).
::                 If connection fails, you can try adding "mode=UR" (Under Reset)
::                 or "reset=HwRst" (Hardware Reset) if your reset line is connected.
::                 Example: -c port=SWD mode=UR reset=HwRst
::   -w          : Write the file and verify (-v)
::   -rst        : Reset the microcontroller to execute the program
"%PROG_CLI%" -c port=SWD -w "%HEX_FILE%" -v -rst

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Flashing failed!
    echo Please verify:
    echo   1. The ST-Link is properly plugged into the PC and custom board.
    echo   2. The custom board is powered.
    echo   3. No other application (like STM32CubeProgrammer GUI) is locking the ST-Link.
    goto :error
)

echo.
echo [SUCCESS] Flash completed and device reset successfully!
goto :end

:error
pause
exit /b 1

:end
pause
exit /b 0
