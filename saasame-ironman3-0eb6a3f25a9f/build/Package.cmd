@echo OFF

if "%1"=="" goto usage
if "%2"=="" goto usage

set SIGN=signtool sign /s my /sha1 CE20909B7751CCBDE0DCF4A58770877D71DE22B7 /t http://timestamp.verisign.com/scripts/timstamp.dll /v
set SIGN1=signtool sign /v /ac "..\build\MSCV-VSClass3.cer" /f ..\build\saasame.pfx /p albert /n "SaaSaMe Ltd." /fd sha1 /tr http://sha1timestamp.ws.symantec.com/sha1/timestamp

set BLD_NO=%1
set OUTPUT=%2

if not exist "..\output\%BLD_NO%" goto error
if not exist %OUTPUT% mkdir %OUTPUT%
if not exist "%OUTPUT%\transport" mkdir "%OUTPUT%\transport"
if not exist "%OUTPUT%\transport\x86" mkdir "%OUTPUT%\transport\x86"
if not exist "%OUTPUT%\transport\x64" mkdir "%OUTPUT%\transport\x64"
if not exist "%OUTPUT%\transport\w2k3_x86" mkdir "%OUTPUT%\transport\w2k3_x86"
if not exist "%OUTPUT%\transport\w2k3_x64" mkdir "%OUTPUT%\transport\w2k3_x64"
if not exist "%OUTPUT%\driver" mkdir "%OUTPUT%\driver"
if not exist "%OUTPUT%\driver\x64" mkdir "%OUTPUT%\driver\x64"

:DRIVER

if exist "..\output\%BLD_NO%\driver\x64_release\vmp.sys" (

copy /y "..\output\%BLD_NO%\driver\x64_release\vmp.cat" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_release\vmp.inf" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_release\vmp.sys" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_release\WdfCoinstaller01011.dll" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\Symbols\x64_release\vmp.pdb" "%OUTPUT%\driver\x64\*.*"

%SIGN1% "%OUTPUT%\driver\x64\vmp.cat"
%SIGN1% "%OUTPUT%\driver\x64\vmp.sys"

)

:WIN32

copy /y "..\output\%BLD_NO%\win32_release-static\irm_host_packer.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\setupex.dll" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_time_syc.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\netsh2.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_conv_agent.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\notice.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\vcbtcli.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\uninstall.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_wizard.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_agent.exe" "%OUTPUT%\transport\x86\*.*"

copy /y "..\output\%BLD_NO%\win32_release-static-2k3\irm_host_packer.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\setupex.dll" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\irm_time_syc.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\netsh2.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\irm_conv_agent.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\notice.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\vcbtcli.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\uninstall.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\irm_wizard.exe" "%OUTPUT%\transport\w2k3_x86\*.*"

%SIGN% "%OUTPUT%\transport\x86\irm_host_packer.exe"
%SIGN% "%OUTPUT%\transport\x86\setupex.dll"
%SIGN% "%OUTPUT%\transport\x86\irm_time_syc.exe"
%SIGN% "%OUTPUT%\transport\x86\netsh2.exe"
%SIGN% "%OUTPUT%\transport\x86\irm_conv_agent.exe"
%SIGN% "%OUTPUT%\transport\x86\notice.exe"
%SIGN% "%OUTPUT%\transport\x86\vcbtcli.exe"
%SIGN% "%OUTPUT%\transport\x86\uninstall.exe"
%SIGN% "%OUTPUT%\transport\x86\irm_wizard.exe"
%SIGN% "%OUTPUT%\transport\x86\irm_agent.exe"
copy /y "%OUTPUT%\transport\x86\irm_agent.exe" "%OUTPUT%\apache24\htdocs\portal\_include\_inc\_agent\*.*"

%SIGN1% "%OUTPUT%\transport\w2k3_x86\irm_host_packer.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\setupex.dll"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\irm_time_syc.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\netsh2.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\irm_conv_agent.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\notice.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\vcbtcli.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\uninstall.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\irm_wizard.exe"

:X64
copy /y "..\output\%BLD_NO%\x64_release-static\irm_host_packer.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_time_syc.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\netsh2.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_conv_agent.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\notice.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\vcbtcli.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\uninstall.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_wizard.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\setupex.dll" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\setup.exe" "%OUTPUT%\transport\x64\*.*"

copy /y "..\output\%BLD_NO%\x64_release-static-2k3\irm_host_packer.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\setupex.dll" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\irm_time_syc.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\netsh2.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\irm_conv_agent.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\notice.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\vcbtcli.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\uninstall.exe" "%OUTPUT%\transport\w2k3_x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\irm_wizard.exe" "%OUTPUT%\transport\w2k3_x64\*.*"

%SIGN% "%OUTPUT%\transport\x64\irm_host_packer.exe"
%SIGN% "%OUTPUT%\transport\x64\irm_time_syc.exe"
%SIGN% "%OUTPUT%\transport\x64\netsh2.exe"
%SIGN% "%OUTPUT%\transport\x64\irm_conv_agent.exe"
%SIGN% "%OUTPUT%\transport\x64\notice.exe"
%SIGN% "%OUTPUT%\transport\x64\vcbtcli.exe"
%SIGN% "%OUTPUT%\transport\x64\uninstall.exe"
%SIGN% "%OUTPUT%\transport\x64\irm_wizard.exe"
%SIGN% "%OUTPUT%\transport\x64\setup.exe"

%SIGN1% "%OUTPUT%\transport\w2k3_x64\irm_host_packer.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\irm_time_syc.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\netsh2.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\irm_conv_agent.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\notice.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\vcbtcli.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\uninstall.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\irm_wizard.exe"

copy /y "..\output\%BLD_NO%\x64_release\irm_scheduler.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_carrier.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_loader.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_launcher.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_converter_cli.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_hypervisor_ex.dll" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_virtual_packer.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_hv_cli.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_wizard.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\irm_transporter.exe" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\cpprest120_2_9.dll" "%OUTPUT%\transport\*.*"
copy /y "..\output\%BLD_NO%\x64_release\wastorage.dll" "%OUTPUT%\transport\*.*"

%SIGN% "%OUTPUT%\transport\irm_scheduler.exe"
%SIGN% "%OUTPUT%\transport\irm_carrier.exe"
%SIGN% "%OUTPUT%\transport\irm_loader.exe"
%SIGN% "%OUTPUT%\transport\irm_launcher.exe"
%SIGN% "%OUTPUT%\transport\irm_converter_cli.exe"
%SIGN% "%OUTPUT%\transport\irm_hypervisor_ex.dll"
%SIGN% "%OUTPUT%\transport\irm_virtual_packer.exe"
%SIGN% "%OUTPUT%\transport\irm_hv_cli.exe"
%SIGN% "%OUTPUT%\transport\irm_wizard.exe"

call ..\build\upx394w\upx.exe --lzma --best --ultra-brute "%OUTPUT%\transport\irm_transporter.exe"

%SIGN% "%OUTPUT%\transport\irm_transporter.exe"

AdvancedInstaller.com /execute "..\setup\saasame.aip" "..\installer.aic"

move /Y ..\output\transport.exe ..\output\%BLD_NO%\transport.exe 

REM move /Y ..\output\transport.msi ..\output\%BLD_NO%\transport.msi

.\7z\7za.exe a -tzip ..\output\%BLD_NO%\linuxlauncher.zip ..\setup\emulator\*
del /F /Q ..\output\%BLD_NO%\linuxlauncher.exe
copy /b ..\output\%BLD_NO%\x64_release-static\setup.exe + ..\output\%BLD_NO%\linuxlauncher.zip ..\output\%BLD_NO%\linuxlauncher.exe
%SIGN% "..\output\%BLD_NO%\linuxlauncher.exe"
del /F /Q ..\output\%BLD_NO%\linuxlauncher.zip

del /F /Q ..\output\%BLD_NO%\mgmt_src.zip
.\7z\7za.exe a -tzip ..\output\%BLD_NO%\mgmt_src.zip ..\setup\mgmt_source\*

goto exit

:error
echo ***
echo *** ERROR   
echo ***
echo *** Please make sure the build folder exists
echo ***
pause
goto exit

:usage
echo You have to indicate the mode: build number and output folder
pause

:exit
