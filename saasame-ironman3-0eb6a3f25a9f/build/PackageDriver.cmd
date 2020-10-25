@echo OFF

if "%1"=="" goto usage
if "%2"=="" goto usage

set SIGN1=signtool sign /v /ac "..\build\MSCV-VSClass3.cer" /f ..\build\saasame.pfx /p albert /n "SaaSaMe Ltd." /fd sha1 /tr http://sha1timestamp.ws.symantec.com/sha1/timestamp
set SIGN=signtool sign /s my /sha1 CE20909B7751CCBDE0DCF4A58770877D71DE22B7 /t http://timestamp.verisign.com/scripts/timstamp.dll /v
set SIGN2=signtool sign /s my /sha1 CE20909B7751CCBDE0DCF4A58770877D71DE22B7 /t http://timestamp.verisign.com/scripts/timstamp.dll /v

set BLD_NO=%1
set OUTPUT=%2
set CONFIG=release

if "%3"=="yes" goto package

if not exist "..\output\%BLD_NO%\driver" goto error
if not exist %OUTPUT% mkdir %OUTPUT%
if not exist "%OUTPUT%\driver" mkdir "%OUTPUT%\driver"
if not exist "%OUTPUT%\driver\x86" mkdir "%OUTPUT%\driver\x86"
if not exist "%OUTPUT%\driver\x64" mkdir "%OUTPUT%\driver\x64"
if not exist "%OUTPUT%\transport\w2k3_x86" mkdir "%OUTPUT%\transport\w2k3_x86"
if not exist "%OUTPUT%\transport\w2k3_x64" mkdir "%OUTPUT%\transport\w2k3_x64"

:WIN32
copy /y "..\output\%BLD_NO%\win32_release-static\irm_host_packer.exe" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\irm_host_packer.exe" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\setupex.dll" "%OUTPUT%\transport\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static-2k3\setupex.dll" "%OUTPUT%\transport\w2k3_x86\*.*"
copy /y "..\output\%BLD_NO%\driver\win32_%CONFIG%\vcbt.cer" "%OUTPUT%\driver\x86\*.*"
copy /y "..\output\%BLD_NO%\driver\win32_%CONFIG%\vcbt.cat" "%OUTPUT%\driver\x86\*.*"
copy /y "..\output\%BLD_NO%\driver\win32_%CONFIG%\vcbt.inf" "%OUTPUT%\driver\x86\*.*"
copy /y "..\output\%BLD_NO%\driver\win32_%CONFIG%\vcbt.sys" "%OUTPUT%\driver\x86\*.*"

%SIGN% "%OUTPUT%\transport\x86\irm_host_packer.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\irm_host_packer.exe"

%SIGN% "%OUTPUT%\transport\x86\setupex.dll"
%SIGN1% "%OUTPUT%\transport\w2k3_x86\setupex.dll"

:X64
copy /y "..\output\%BLD_NO%\x64_release-static\irm_host_packer.exe" "%OUTPUT%\transport\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static-2k3\irm_host_packer.exe" "%OUTPUT%\transport\w2k3_x64\*.*"

%SIGN% "%OUTPUT%\transport\x64\irm_host_packer.exe"
%SIGN1% "%OUTPUT%\transport\w2k3_x64\irm_host_packer.exe"

copy /y "..\output\%BLD_NO%\driver\x64_%CONFIG%\vcbt.cat" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_%CONFIG%\vcbt.inf" "%OUTPUT%\driver\x64\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_%CONFIG%\vcbt.sys" "%OUTPUT%\driver\x64\*.*"

if "%CONFIG%"=="release" goto package

set TEST=ToolsForTest

copy /y "..\output\%BLD_NO%\driver\x64_%CONFIG%\vcbt.cer" "%OUTPUT%\driver\x64\*.*"

if not exist "..\output\%BLD_NO%\%TEST%" mkdir "..\output\%BLD_NO%\%TEST%"
if not exist "..\output\%BLD_NO%\%TEST%\x86" mkdir "..\output\%BLD_NO%\%TEST%\x86"
if not exist "..\output\%BLD_NO%\%TEST%\x64" mkdir "..\output\%BLD_NO%\%TEST%\x64"
copy /y "..\output\%BLD_NO%\driver\win32_%CONFIG%\vcbt.cer" "..\output\%BLD_NO%\%TEST%\x86\*.*"
copy /y "..\output\%BLD_NO%\driver\x64_%CONFIG%\vcbt.cer" "..\output\%BLD_NO%\%TEST%\x64\*.*"
copy /y "..\vcbt\x64\CertMgr.Exe" "..\output\%BLD_NO%\%TEST%\x64\*.*"
copy /y "..\vcbt\CertMgr.Exe" "..\output\%BLD_NO%\%TEST%\x86\*.*"
copy /y "..\vcbt\*.cmd" "..\output\%BLD_NO%\%TEST%\x64\*.*"
copy /y "..\vcbt\*.cmd" "..\output\%BLD_NO%\%TEST%\x86\*.*"

:package

AdvancedInstaller.com /execute "..\setup\packer64.aip" "..\installer.aic"
.\Orca\orca.exe -f VCBT -m ..\setup\DIFxAppMergeModule\x64\DIFxApp.msm -c ..\output\Packer64.msi -! -q
move /Y ..\output\Packer64.msi ..\output\%BLD_NO%\packer64.msi 
move /Y ..\output\packer64_no_driver.msi ..\output\%BLD_NO%\packer64_no_driver.msi 

AdvancedInstaller.com /execute "..\setup\packer64_2k3.aip" "..\installer.aic"
copy /Y ..\output\Packer64_2k3.msi ..\output\%BLD_NO%\packer64_2k3_no_driver.msi 
.\Orca\orca.exe -f VCBT -m ..\setup\DIFxAppMergeModule\x64\DIFxApp.msm -c ..\output\Packer64_2k3.msi -! -q
move /Y ..\output\Packer64_2k3.msi ..\output\%BLD_NO%\packer64_2k3.msi 

AdvancedInstaller.com /execute "..\setup\packer32.aip" "..\installer.aic"
.\Orca\orca.exe -f VCBT -m ..\setup\DIFxAppMergeModule\x86\DIFxApp.msm -c ..\output\Packer32.msi -! -q
move /Y ..\output\Packer32.msi ..\output\%BLD_NO%\packer32.msi
move /Y ..\output\packer32_no_driver.msi ..\output\%BLD_NO%\packer32_no_driver.msi

AdvancedInstaller.com /execute "..\setup\packer32_2k3.aip" "..\installer.aic"
copy /Y ..\output\Packer32_2k3.msi ..\output\%BLD_NO%\packer32_2k3_no_driver.msi
.\Orca\orca.exe -f VCBT -m ..\setup\DIFxAppMergeModule\x86\DIFxApp.msm -c ..\output\Packer32_2k3.msi -! -q
move /Y ..\output\Packer32_2k3.msi ..\output\%BLD_NO%\packer32_2k3.msi

%SIGN% "..\output\%BLD_NO%\packer64.msi"
%SIGN% "..\output\%BLD_NO%\packer64_no_driver.msi"

%SIGN% "..\output\%BLD_NO%\packer32.msi"
%SIGN% "..\output\%BLD_NO%\packer32_no_driver.msi"

%SIGN1% "..\output\%BLD_NO%\packer64_2k3.msi"
%SIGN1% "..\output\%BLD_NO%\packer64_2k3_no_driver.msi"

%SIGN1% "..\output\%BLD_NO%\packer32_2k3.msi"
%SIGN1% "..\output\%BLD_NO%\packer32_2k3_no_driver.msi"

.\7z\7za.exe a -tzip ..\output\%BLD_NO%\packer.zip ..\output\%BLD_NO%\packer64.msi
.\7z\7za.exe a -tzip ..\output\%BLD_NO%\packer.zip ..\output\%BLD_NO%\packer32.msi
.\7z\7za.exe a -tzip ..\output\%BLD_NO%\packer.zip ..\output\%BLD_NO%\packer64_2k3.msi
.\7z\7za.exe a -tzip ..\output\%BLD_NO%\packer.zip ..\output\%BLD_NO%\packer32_2k3.msi

copy /b ..\output\%BLD_NO%\win32_release-static-2k3\setup.exe + ..\output\%BLD_NO%\packer.zip ..\output\%BLD_NO%\packer.exe

%SIGN1% "..\output\%BLD_NO%\packer.exe"
%SIGN% "..\output\%BLD_NO%\packer.exe"

del /F /Q ..\output\%BLD_NO%\packer.zip

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
