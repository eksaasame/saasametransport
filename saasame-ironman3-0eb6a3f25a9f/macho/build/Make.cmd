@echo off

if "%1"=="" goto usage
if "%2"=="" goto usage

set BLD_CONFIG=%1
set BLD_PLATFORM=%2

if errorlevel == 1 goto error

echo "SabreCloudNative;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv ..\SabreCloudNative.sln              /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\SabreCloudNative%BLD_CONFIG%-%BLD_PLATFORM%.log
if errorlevel == 1 goto error


:copy

rd /S /Q "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%"
rd /S /Q "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if not exist "..\output" 						mkdir "..\output"
if not exist "..\output\%BLD_NO%" 					mkdir "..\output\%BLD_NO%"
if not exist "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%" 		mkdir "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%"
if not exist "..\output\%BLD_NO%\Symbols" 				mkdir "..\output\%BLD_NO%\Symbols"	
if not exist "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%" 	mkdir "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

copy /y ..\sabrecloudctrl.inf 		      "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y .\sabrecloudctrl.dd		      "..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

set OLDPath=%cd%
cd ..\output\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%
call makecab.exe /f %cd%\sabrecloudctrl.dd

del /f sabrecloudctrl.inf
del /f sabrecloudctrl.dd
del /f setup.*

move /y .\cab\sabrecloud.cab ..\sabrecloud_%BLD_PLATFORM%.cab

rd /S /Q .\cab

cd %OLDPath%


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