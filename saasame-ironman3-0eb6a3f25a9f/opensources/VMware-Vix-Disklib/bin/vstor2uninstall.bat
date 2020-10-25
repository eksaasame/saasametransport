@echo off
rem This batch file uninstalls the vstor2-mntapi20-shared service.
rem
rem Check the system type
rem
rem Check if the service is installed
rem
set VSTOR=vstor2-mntapi20-shared
sc query %VSTOR% > nul
if errorlevel 1 (
    echo "Service not found. Quitting"
    exit /b 1
)
rem
rem The service is installed, remove it.
rem
echo "Service exists, removing it."
sc stop %VSTOR% > nul
sc delete %VSTOR%
del /f %SystemRoot%\System32\drivers\%VSTOR%.sys
rem The service should be cleaned up.
exit /b 0
