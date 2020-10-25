
set _BLD_NO=0

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

NET USE \\192.168.31.253\BuildTransport /u:localhost\admin abc@123 
robocopy "..\output\%BLD_NO%" "\\192.168.31.253\BuildTransport\%BLD_NO%" /Z /MIR 
NET USE \\192.168.31.253\BuildTransport /D
