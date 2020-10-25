@echo off
set OLD_PATH=%PATH%
set BLD_NO=0
Set PROJECTPATH="%~dp0.."

set SIGN=signtool sign /s my /sha1 CE20909B7751CCBDE0DCF4A58770877D71DE22B7 /t http://timestamp.verisign.com/scripts/timstamp.dll /v

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

Set APP_PATH=%Output%\WinPE_amd64\mount\SaaSaMe

cd "%ADK_PATH%\Deployment Tools"
call DandISetEnv.bat

Dism /Mount-Image /ImageFile:"%Output%\WinPE_amd64\media\sources\boot.wim" /index:1 /MountDir:"%Output%\WinPE_amd64\mount"

Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\VMware\2k8\amd64\lsimpt_scsi\lsi_scsi.inf"
Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\VMware\amd64\pvscsi\pvscsi.inf"
Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\VMware\amd64\vmxnet3\vmxnet3ndis6.inf"
Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\virtio-win\vioscsi\2k12R2\amd64\vioscsi.inf"
Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\virtio-win\viostor\2k12R2\amd64\viostor.inf"
Dism /image:"%Output%\WinPE_amd64\mount" /Add-Driver /Driver:"%PROJECTPATH%\setup\virtio-win\NetKVM\2k12R2\amd64\netkvm.inf"

if not exist %APP_PATH% mkdir %APP_PATH%
if not exist "%APP_PATH%\x86" mkdir "%APP_PATH%\x86"
if not exist "%APP_PATH%\x64" mkdir "%APP_PATH%\x64"

copy /y "%PROJECTPATH%\setup\apache24\conf\ssl\server.crt" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\setup\apache24\conf\ssl\server.key" "%APP_PATH%\*.*"

copy /y "%PROJECTPATH%\setup\transport\x64\difxapi.dll" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_wpe.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_host_packer.exe" "%APP_PATH%\*.*"

%SIGN% "%APP_PATH%\irm_wpe.exe"
%SIGN% "%APP_PATH%\irm_host_packer.exe"

REM copy /y "%PROJECTPATH%\setup\Startnet.cmd" "%Output%\WinPE_amd64\mount\windows\system32\*.*"
copy /y "%PROJECTPATH%\setup\Winpeshl.ini" "%Output%\WinPE_amd64\mount\windows\system32\*.*"
Dism /Unmount-Image /MountDir:"%Output%\WinPE_amd64\mount" /commit

goto end

:error

echo "Skip...."

:end

cd %PROJECTPATH%\build
set PATH=%OLD_PATH%