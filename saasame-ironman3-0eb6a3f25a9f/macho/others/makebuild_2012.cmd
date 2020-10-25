@echo off
set OTHERCD=%cd%
if "%1"=="" goto usage
if "%2"=="" goto usage
cd /d %~dp0
set BLD_CONFIG=%1
set BLD_PLATFORM=%2

echo "others_2012;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv others_2012.sln             /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\others%BLD_CONFIG%-%BLD_PLATFORM%.log

goto exit

:usage
echo You have to indicate the mode: Release or Debug and platform win32 or x64
pause

:exit
cd /d %OTHERCD%