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

NET USE z: \\192.168.100.1\output /u:localhost\albert.wei abc@123 

call "setenvmsvc2013.cmd"

call makeMacho.cmd release-static-2k3 win32 

if errorlevel == 1 goto error

call make2.cmd release-static-2k3 win32 z:

if errorlevel == 1 goto error

call makeMacho.cmd release-static-2k3 x64 

if errorlevel == 1 goto error

call make2.cmd release-static-2k3 x64 z:

if errorlevel == 1 goto error

call makeMacho.cmd release-static win32 

if errorlevel == 1 goto error

call make2.cmd release-static win32 z:

if errorlevel == 1 goto error

goto end

:error
echo "Skip compile"

:end
echo %DATE% %TIME%