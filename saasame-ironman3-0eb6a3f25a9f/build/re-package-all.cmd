
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

call re-package.cmd %_BLD_NO% %3
call re-create_iso.cmd %_BLD_NO% 8.1
call re-create_iso.cmd %_BLD_NO% 10

if "%2"=="yes" (
NET USE \\192.168.31.253\BuildTransport /u:localhost\dev-albert rOfT$O 
robocopy "..\output\%BLD_NO%" "\\192.168.31.253\BuildTransport\%BLD_NO%" /Z /MIR 
NET USE \\192.168.31.253\BuildTransport /D
)