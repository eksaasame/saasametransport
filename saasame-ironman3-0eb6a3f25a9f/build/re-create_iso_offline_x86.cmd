@echo OFF

set BLD_NO=0

if "%1"=="" (
echo.
echo.
echo Enter Build Number
echo.
echo.
set /p BLD_NO=Enter Build Number:

) else (
if not "%1"=="" set BLD_NO=%1
)

if "%2"=="" (
Set ADK_VERSION=10
) else (
Set ADK_VERSION=%2
)
call setenvmsvc2013.cmd
call create_winpe_x86.cmd c: %ADK_VERSION%
call customize_offline_x86.cmd %BLD_NO% c: %ADK_VERSION%
call create_iso_offline_x86.cmd %BLD_NO% c: %ADK_VERSION%