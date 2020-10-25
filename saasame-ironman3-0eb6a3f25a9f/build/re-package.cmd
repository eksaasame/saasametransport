@echo ON

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
call "refresh.cmd"
call "setenvmsvc2013.cmd"

set _buildno=%BLD_NO%

set CONFIG_VERSION_MAJOR=1
set CONFIG_VERSION_MINOR=1

echo Build number will be %_buildno%!.

echo Creating Advanced Installer Script File
echo ;aic> installer.aic
echo SetVersion %CONFIG_VERSION_MAJOR%.%CONFIG_VERSION_MINOR%.%_buildno% >> installer.aic
echo Save >> installer.aic
echo Rebuild >> installer.aic

echo Creating PHP Version File
echo %CONFIG_VERSION_MAJOR%.%CONFIG_VERSION_MINOR%.%_buildno%> _version.txt

copy /Y installer.aic ..\installer.aic
copy /Y _version.txt ..\setup\apache24\htdocs\portal\_include\_inc\_version.txt
del /F  installer.aic
del /F  _version.txt 

if "%3"=="yes" (
call copy_exe.cmd %BLD_NO%
)

call package.cmd %BLD_NO% "..\setup"

if "%2"=="yes" (
NET USE \\192.168.31.253\BuildTransport /u:localhost\dev-albert rOfT$O 
robocopy "..\output\%BLD_NO%" "\\192.168.31.253\BuildTransport\%BLD_NO%" /Z /MIR 
NET USE \\192.168.31.253\BuildTransport /D
)
:error