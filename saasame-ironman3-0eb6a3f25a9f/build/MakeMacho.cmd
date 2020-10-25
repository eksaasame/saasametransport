@echo off

if "%1"=="" goto usage
if "%2"=="" goto usage

set BLD_CONFIG=%1
set BLD_PLATFORM=%2

if "%3"=="copy" goto copy

if errorlevel == 1 goto error

echo %DATE% %TIME%
echo "macho_2013;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv ..\macho\macho\macho_2013.sln              /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\Macho-%BLD_CONFIG%-%BLD_PLATFORM%.log
if errorlevel == 1 goto error

goto exit

:error
echo ***
echo *** ERROR   
echo ***
echo *** COMPILATION ERROR %0
echo ***
echo Press Ctrl C to stop the build.
pause
goto exit

:usage
echo You have to indicate the mode: Release or Debug and platform win32 or x64
pause

:exit