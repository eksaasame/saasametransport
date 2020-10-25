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

if "%BLD_NO%"=="0" (goto error) else (call Config.cmd %BLD_NO%)

echo BUILD:  %_buildno%

REM call "setenvmsvc2013.cmd"

call makedriver.cmd release win32 win7 

if errorlevel == 1 goto error

call makedriver.cmd release x64 win7 

if errorlevel == 1 goto error

call makedriver.cmd debug win32 win7 

if errorlevel == 1 goto error

call makedriver.cmd debug x64 win7 

if errorlevel == 1 goto error

call packagedriver.cmd %BLD_NO% "..\setup"

goto end

:error
echo "Skip compile"

:end
