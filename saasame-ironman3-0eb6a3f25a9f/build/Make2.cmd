@echo on

if "%1"=="" goto usage
if "%2"=="" goto usage

set BLD_CONFIG=%1
set BLD_PLATFORM=%2

if "%4"=="copy" goto copy

if "%3"=="" (
set OUTPUT="..\output"
) else (
set OUTPUT=%3
)

if errorlevel == 1 goto error

rd /S /Q ..\%BLD_CONFIG%
rd /S /Q ..\%BLD_PLATFORM%\%BLD_CONFIG%

echo %DATE% %TIME%
echo "SaaSame;platform=%BLD_PLATFORM%;target=%BLD_CONFIG%"
devenv ..\saasame.sln              /REBUILD "%BLD_CONFIG%|%BLD_PLATFORM%" > ..\SaaSame-%BLD_CONFIG%-%BLD_PLATFORM%.log
if errorlevel == 1 goto error

:copy

rd /S /Q "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%"
rd /S /Q "%OUTPUT%\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if not exist "%OUTPUT%" 						mkdir "%OUTPUT%"
if not exist "%OUTPUT%\%BLD_NO%" 					mkdir "%OUTPUT%\%BLD_NO%"
if not exist "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%" 		mkdir "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%"
if not exist "%OUTPUT%\%BLD_NO%\Symbols" 				mkdir "%OUTPUT%\%BLD_NO%\Symbols"	
if not exist "%OUTPUT%\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%" 	mkdir "%OUTPUT%\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if "%2"=="x64" goto X64
if "%2"=="X64" goto X64

:WIN32

copy /y ..\%BLD_CONFIG%\*.dll "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.pdb "%OUTPUT%\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_CONFIG%\*.exe "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

goto exit

:X64

copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.pdb "%OUTPUT%\%BLD_NO%\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.dll "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y ..\%BLD_PLATFORM%\%BLD_CONFIG%\*.exe "%OUTPUT%\%BLD_NO%\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

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