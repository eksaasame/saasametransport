@echo off
set OLD_PATH=%PATH%
set BLD_NO=0
Set PROJECTPATH="%~dp0.."
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

if "%BLD_NO%"=="0" (goto error)

if "%2"=="" (
echo.
echo.
echo Please Enter Output Folder
echo.
echo.
goto error
)

if "%3"=="" (
Set ADK_VERSION=10
) else (
Set ADK_VERSION=%3
)

Set ADK_PATH=C:\Program Files (x86)\Windows Kits\%ADK_VERSION%\Assessment and Deployment Kit
Set Output=%2%

cd "%ADK_PATH%\Deployment Tools"

call DandISetEnv.bat

call MakeWinPEMedia /ISO %Output%\winpe_amd64 "%PROJECTPATH%\output\%BLD_NO%\winpe_%ADK_VERSION%_%BLD_NO%.iso"

cd %PROJECTPATH%\build
set PATH=%OLD_PATH%