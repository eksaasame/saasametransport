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
if not exist "%APP_PATH%\w2k3_x86" mkdir "%APP_PATH%\w2k3_x86"
if not exist "%APP_PATH%\w2k3_x64" mkdir "%APP_PATH%\w2k3_x64"

copy /y "%PROJECTPATH%\macho\libs\Release-Staticx64\aws-cpp-sdk-core.dll" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\macho\libs\Release-Staticx64\aws-cpp-sdk-s3.dll" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\setup\transport\x64\libexpat.dll" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\setup\apache24\conf\ssl\server.crt" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\setup\apache24\conf\ssl\server.key" "%APP_PATH%\*.*"

copy /y "%PROJECTPATH%\setup\transport\x64\difxapi.dll" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\setup\transport\x64\difxapi.dll" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\x86\difxapi.dll" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x64\difxapi.dll" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x86\difxapi.dll" "%APP_PATH%\w2k3_x86\*.*"

copy /y "%PROJECTPATH%\setup\transport\x64\dpinst.xml" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\x64\dpinst.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\x64\*.cmd" "%APP_PATH%\x64\*.*"

copy /y "%PROJECTPATH%\setup\transport\x86\dpinst.xml" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\setup\transport\x86\dpinst.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\setup\transport\x86\*.cmd" "%APP_PATH%\x86\*.*"

copy /y "%PROJECTPATH%\setup\transport\w2k3_x86\dpinst.xml" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x86\dpinst.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x86\*.cmd" "%APP_PATH%\w2k3_x86\*.*"

copy /y "%PROJECTPATH%\setup\transport\w2k3_x64\dpinst.xml" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x64\dpinst.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\setup\transport\w2k3_x64\*.cmd" "%APP_PATH%\w2k3_x64\*.*"

del /F /Q %APP_PATH%\irm_host_packer.exe

copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_wpe.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_loader.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_launcher.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_converter_cli.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_hv_cli.exe" "%APP_PATH%\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_hypervisor_ex.dll" "%APP_PATH%\*.*"

%SIGN% "%APP_PATH%\irm_wpe.exe"
%SIGN% "%APP_PATH%\irm_loader.exe"
%SIGN% "%APP_PATH%\irm_launcher.exe"
%SIGN% "%APP_PATH%\irm_converter_cli.exe"
%SIGN% "%APP_PATH%\irm_hv_cli.exe"
%SIGN% "%APP_PATH%\irm_hypervisor_ex.dll"

copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\irm_time_syc.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\netsh2.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\irm_conv_agent.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\notice.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\vcbtcli.exe" "%APP_PATH%\x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static\uninstall.exe" "%APP_PATH%\x86\*.*"

%SIGN% "%APP_PATH%\x86\irm_time_syc.exe"
%SIGN% "%APP_PATH%\x86\netsh2.exe"
%SIGN% "%APP_PATH%\x86\irm_conv_agent.exe"
%SIGN% "%APP_PATH%\x86\notice.exe"
%SIGN% "%APP_PATH%\x86\vcbtcli.exe"
%SIGN% "%APP_PATH%\x86\uninstall.exe"

copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\irm_time_syc.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\netsh2.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\irm_conv_agent.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\notice.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\vcbtcli.exe" "%APP_PATH%\w2k3_x86\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\win32_release-static-2k3\uninstall.exe" "%APP_PATH%\w2k3_x86\*.*"

%SIGN% "%APP_PATH%\w2k3_x86\irm_time_syc.exe"
%SIGN% "%APP_PATH%\w2k3_x86\netsh2.exe"
%SIGN% "%APP_PATH%\w2k3_x86\irm_conv_agent.exe"
%SIGN% "%APP_PATH%\w2k3_x86\notice.exe"
%SIGN% "%APP_PATH%\w2k3_x86\vcbtcli.exe"
%SIGN% "%APP_PATH%\w2k3_x86\uninstall.exe"

copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_time_syc.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\netsh2.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\irm_conv_agent.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\notice.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\vcbtcli.exe" "%APP_PATH%\x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static\uninstall.exe" "%APP_PATH%\x64\*.*"

%SIGN% "%APP_PATH%\x64\irm_time_syc.exe"
%SIGN% "%APP_PATH%\x64\netsh2.exe"
%SIGN% "%APP_PATH%\x64\irm_conv_agent.exe"
%SIGN% "%APP_PATH%\x64\notice.exe"
%SIGN% "%APP_PATH%\x64\vcbtcli.exe"
%SIGN% "%APP_PATH%\x64\uninstall.exe"

copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\irm_time_syc.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\netsh2.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\irm_conv_agent.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\notice.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\vcbtcli.exe" "%APP_PATH%\w2k3_x64\*.*"
copy /y "%PROJECTPATH%\output\%BLD_NO%\x64_release-static-2k3\uninstall.exe" "%APP_PATH%\w2k3_x64\*.*"

%SIGN% "%APP_PATH%\w2k3_x64\irm_time_syc.exe"
%SIGN% "%APP_PATH%\w2k3_x64\netsh2.exe"
%SIGN% "%APP_PATH%\w2k3_x64\irm_conv_agent.exe"
%SIGN% "%APP_PATH%\w2k3_x64\notice.exe"
%SIGN% "%APP_PATH%\w2k3_x64\vcbtcli.exe"
%SIGN% "%APP_PATH%\w2k3_x64\uninstall.exe"

REM copy /y "%PROJECTPATH%\setup\Startnet.cmd" "%Output%\WinPE_amd64\mount\windows\system32\*.*"
copy /y "%PROJECTPATH%\setup\Winpeshl.ini" "%Output%\WinPE_amd64\mount\windows\system32\*.*"
xcopy "%PROJECTPATH%\setup\virtio-win" "%APP_PATH%\virtio-win" /E /Y /I
xcopy "%PROJECTPATH%\setup\vmware" "%APP_PATH%\vmware" /E /Y /I
xcopy "%PROJECTPATH%\setup\hyperv" "%APP_PATH%\hyperv" /E /Y /I
Dism /Unmount-Image /MountDir:"%Output%\WinPE_amd64\mount" /commit
xcopy "%PROJECTPATH%\setup\emulator" "%Output%\WinPE_amd64\media\emulator" /E /Y /I
goto end

:error

echo "Skip...."

:end

cd %PROJECTPATH%\build
set PATH=%OLD_PATH%