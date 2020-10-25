
set _BLD_NO=0
set BUILD_DRIVER=NO

if "%1"=="" (
echo.
echo.
echo Enter Build Number
echo.
echo.
set /p BLD_NO=Enter Build Number:

) else (
if not "%1"=="" set _BLD_NO=%1
)

if not "%2"=="" set BUILD_DRIVER=%2

call compile.cmd 1%_BLD_NO% %BUILD_DRIVER%
call compile.cmd %_BLD_NO%