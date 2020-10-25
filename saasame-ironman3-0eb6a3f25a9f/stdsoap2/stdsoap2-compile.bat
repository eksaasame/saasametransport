@echo Off
set IPSANUSESDK="%WINSDK%"
if not exist %IPSANUSESDK% set IPSANUSESDK="%ProgramFiles%\Microsoft SDK"
rem echo %IPSANUSESDK% 

rem %1 - Build Type  	(release or debug or PackageOnly)
rem %2 - Platform    	(Win32 or X64)
rem %3 - Compiler	    (optional, 2005 or set MSVC2005ONLY=true to use MSVC2005)

set BLD_TYPE=release
set BLD_PLATFORM=Win32
if "%MSVC2005ONLY%"=="true" (set BLD_COMPILER=MSVC2005) else (set BLD_COMPILER=MSVC60)

if not "%1"=="" set BLD_TYPE=%1
if not "%2"=="" set BLD_PLATFORM=%2
if not "%3"=="" set BLD_COMPILER=%3
if "%3"=="2005" set BLD_COMPILER=MSVC2005


if "%BLD_PLATFORM%"=="X64" set BLD_PLATFORM=x64
if "%BLD_PLATFORM%"=="x64" set BLD_COMPILER=MSVC2005

echo Build with following settings 

echo BUILD:    %BLD_NO%
echo Type:     %BLD_TYPE%
echo Platform: %BLD_PLATFORM% 
echo Compiler: %BLD_COMPILER%

:cleanup
if exist .\build\*.*                         (del /F /Q .\build\*.*      )
del /F /Q *.log


:Compile
if "%BLD_TYPE%"=="PackageOnly" goto done

:cleanCommonDir
if exist ..\..\wincommon\%BLD_TYPE%%BLD_PLATFORM%\*.* del /F /Q ..\..\wincommon\%BLD_TYPE%%BLD_PLATFORM%\*.*


:MSVC2005
echo Compiling with MSVC2005

if not exist .\%BLD_TYPE%%BLD_PLATFORM% mkdir .\%BLD_TYPE%%BLD_PLATFORM%
rem del /F /Q .\%BLD_TYPE%%BLD_PLATFORM%\*

rem if "%BLD_TYPE%"=="debug" (call %IPSANUSESDK%\setenv.bat /X64 /DEBUG) else (call %IPSANUSESDK%\setenv.bat /X64 /RETAIL)
call "..\..\winbatch2005\setenvmsvc2005.bat"

echo.
echo Start to build %BLD_PLATFORM% %BLD_TYPE% version with MSVC2005...
echo.

echo "stdsoap2 - %BLD_PLATFORM% %BLD_TYPE%"
devenv ..\stdsoap2\stdsoap2.sln                      	/REBUILD "%BLD_TYPE%|%BLD_PLATFORM%" > stdsoap2%BLD_TYPE%.log
if errorlevel == 1 goto error

goto done

:usage
echo You have to indicate the mode: Release or Debug and then "generic" for generic build
pause
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

:exit
:done
echo.




