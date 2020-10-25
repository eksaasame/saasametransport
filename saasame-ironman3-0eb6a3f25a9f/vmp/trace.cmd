@echo off
set scriptpath=%~dp0

echo logman create trace "Trace_vmp" -o "%scriptpath%Trace_vmp.etl" -nb 128 640 -bs 128
logman create trace "Trace_vmp" -o "%scriptpath%Trace_vmp.etl" -nb 128 640 -bs 128
:: 
echo logman update "Trace_vmp" -p {C689C5E6-5219-4774-BE15-9B1F92F949FD} 0xFFFFFFFF 0xFF
logman update "Trace_vmp" -p {C689C5E6-5219-4774-BE15-9B1F92F949FD} 0xFFFFFFFF 0xFF
echo logman start -n "Trace_vmp"
logman start -n "Trace_vmp"


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

echo logman stop -n Trace_vmp
logman stop -n Trace_vmp
echo logman delete -n Trace_vmp
logman delete -n Trace_vmp
