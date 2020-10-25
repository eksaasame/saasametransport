@echo off
set scriptpath=%~dp0

echo logman create trace "Trace_vcbt" -o "%scriptpath%Trace_vcbt.etl" -nb 128 640 -bs 128
logman create trace "Trace_vcbt" -o "%scriptpath%Trace_vcbt.etl" -nb 128 640 -bs 128
:: vcbt.pdb
echo logman update "Trace_vcbt" -p {9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1} 0xFFFFFFFF 0xFF
logman update "Trace_vcbt" -p {9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1} 0xFFFFFFFF 0xFF
echo logman start -n "Trace_vcbt"
logman start -n "Trace_vcbt"


echo.
echo Trace is running.
echo.
echo If you trace something interesting, copy the following files after this
echo window closes. They will be deleted and overwritten the next time you run
echo this script.
echo.

rem Using "<nul set /p=" instead of "echo" to suppress newline
<nul set /p=TO END THE TRACE ^>^> 
pause

echo logman stop -n Trace_vcbt
logman stop -n Trace_vcbt
echo logman delete -n Trace_vcbt
logman delete -n Trace_vcbt
