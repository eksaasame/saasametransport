@echo off
set MAKE_BUILD_CD=%cd%
if "%1"=="" goto usage
if "%2"=="" goto usage
if "%3"=="" goto usage

cd /d %~dp0
set PROJECT=%1
set BLD_CONFIG=%2
set BLD_PLATFORM=%3
set BLD_NO=%_buildno%

echo "%PROJECT%;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv ..\%PROJECT%\%PROJECT%_2013.sln             /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\%PROJECT%-%BLD_CONFIG%-%BLD_PLATFORM%.log

rd /S /Q "..\output\%BLD_NO%\%PROJECT%\%BLD_PLATFORM%_%BLD_CONFIG%"
rd /S /Q "..\output\%BLD_NO%\%PROJECT%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if not exist "..\output" 						mkdir "..\output"
if not exist "..\output\%BLD_NO%" 					mkdir "..\output\%BLD_NO%"
if not exist "..\output\%BLD_NO%\%PROJECT%" 				mkdir "..\output\%BLD_NO%\%PROJECT%"
if not exist "..\output\%BLD_NO%\%PROJECT%\%BLD_PLATFORM%_%BLD_CONFIG%" mkdir "..\output\%BLD_NO%\%PROJECT%\%BLD_PLATFORM%_%BLD_CONFIG%"
if not exist "..\output\%BLD_NO%\%PROJECT%\Symbols" 				mkdir "..\output\%BLD_NO%\%PROJECT%\Symbols"	
if not exist "..\output\%BLD_NO%\%PROJECT%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%" mkdir "..\output\%BLD_NO%\%PROJECT%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

copy /y ..\%PROJECT%\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "..\output\%BLD_NO%\%PROJECT%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%PROJECT%\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "..\output\%BLD_NO%\%PROJECT%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%PROJECT%\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "..\output\%BLD_NO%\%PROJECT%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

goto exit

:usage
echo You have to indicate the mode: Release or Debug and platform win32 or x64
pause

:exit

cd /d %MAKE_BUILD_CD%
