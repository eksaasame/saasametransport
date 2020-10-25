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

call "setenvmsvc2013.cmd"

call makeMacho.cmd release x64 

if errorlevel == 1 goto error

call make.cmd release x64 

if errorlevel == 1 goto error

goto end

:error
echo "Skip compile"

:end
echo %DATE% %TIME%