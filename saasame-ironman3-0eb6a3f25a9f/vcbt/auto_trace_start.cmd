@echo off
set scriptpath=%~dp0

echo logman create trace "autosession\Trace_vcbt" -o "%scriptpath%Trace_vcbt.etl" -nb 128 640 -bs 128
logman create trace "autosession\Trace_vcbt" -o "%scriptpath%Trace_vcbt.etl" -nb 128 640 -bs 128
:: vcbt.pdb
echo logman update "autosession\Trace_vcbt" -p {9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1} 0xFFFFFFFF 0xFF
logman update "autosession\Trace_vcbt" -p {9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1} 0xFFFFFFFF 0xFF
echo.
echo Trace log is not running now.
echo.
echo You must reboot your computer for the trace log to take effect.
echo.

pause

