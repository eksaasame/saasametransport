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

call makeMacho.cmd release-static-2k3 win32 

if errorlevel == 1 goto error

call make.cmd release-static-2k3 win32 

if errorlevel == 1 goto error

call makeMacho.cmd release-static-2k3 x64 

if errorlevel == 1 goto error

call make.cmd release-static-2k3 x64 

if errorlevel == 1 goto error

call makeMacho.cmd release-static win32 

if errorlevel == 1 goto error

call make.cmd release-static win32 

if errorlevel == 1 goto error

call makeMacho.cmd release-static x64 

if errorlevel == 1 goto error

call make.cmd release-static x64 

if errorlevel == 1 goto error

call makeMacho.cmd release x64 

if errorlevel == 1 goto error

call make.cmd release x64 

if errorlevel == 1 goto error

if "%4"=="" (

call makedriver2.cmd release x64 win8

if errorlevel == 1 goto error

)
:makecab
echo %DATE% %TIME%
call package.cmd %BLD_NO% "..\setup"

if "%2"=="yes" (

echo.
echo.
echo Rebuild the driver
echo.
echo.

call CompileDriver.cmd %BLD_NO%

) else (

call PackageDriver.cmd %BLD_NO% "..\setup" yes

)

copy /y "..\output\%BLD_NO%\win32_release-static-2k3\setup.exe" "..\DrProfiler\DrProfiler\res\setup.exe"
call ..\build\upx394w\upx.exe --lzma --best "..\DrProfiler\DrProfiler\res\setup.exe"
call MakeDrProfiler.cmd %BLD_NO%

echo %DATE% %TIME%
call re-create_iso.cmd %BLD_NO% 8.1
echo %DATE% %TIME%
call re-create_iso.cmd %BLD_NO% 10 

if "%3"=="yes" (
goto end
)

NET USE \\192.168.31.253\BuildTransport /u:localhost\dev-albert rOfT$O 
robocopy "..\output\%BLD_NO%" "\\192.168.31.253\BuildTransport\%BLD_NO%" /Z /MIR 
NET USE \\192.168.31.253\BuildTransport /D

goto end

:error
echo "Skip compile"

:end
echo %DATE% %TIME%