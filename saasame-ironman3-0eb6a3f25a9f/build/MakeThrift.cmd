@echo off

if "%1"=="" goto usage
if "%2"=="" goto usage

set BLD_CONFIG=%1
set BLD_PLATFORM=%2

if errorlevel == 1 goto error

echo "Thrift;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv ..\opensources\thrift-0.12.0\lib\cpp\thrift-2013.sln  /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\Thrift-%BLD_CONFIG%-%BLD_PLATFORM%.log
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