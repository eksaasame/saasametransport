@echo off
rem This batch file installs a new vstor2-mntapi20-shared service.
rem If a previous binary is installed, then the old service will be
rem removed before the new service is installed.
rem
rem Check the system type
rem

set VSTOR=vstor2-mntapi20-shared
wmic OS get OSArchitecture | findstr 64
if errorlevel 1 (
    echo "This driver can only be installed on a 64-bit system."
    exit /b 1
)
rem
rem Check if the service is installed
rem
sc query %VSTOR% > nul
if errorlevel 1 (
    echo "Service not found, adding the service."
    goto Installit
)
rem
rem The service is installed, remove it.
rem
echo "Service exists, removing it first."
sc stop %VSTOR% > nul
sc delete %VSTOR%
del /f %SystemRoot%\System32\drivers\%VSTOR%.sys
rem The service should be cleaned up.  Now install the new driver
:Installit
copy /v /y /b AMD64\%VSTOR%.sys /b %SystemRoot%\System32\drivers\%VSTOR%.sys /b
sc create %VSTOR% type= kernel start= auto error= normal binpath= System32\drivers\%VSTOR%.sys DisplayName= "Vstor2 MntApi 2.0 Driver (shared)" group= System
if errorlevel 1 (
    echo "Failed to create the service."
    exit /b 2
)
rem echo errorlevel %errorlevel%
echo "Service installed - Starting"
sc start %VSTOR%
exit /b 0
