@echo OFF

if "%1"=="" goto usage
if "%2"=="" goto usage

set BLD_NO=%1
set OUTPUT=%2

if not exist "..\output\%BLD_NO%" goto error
if not exist %OUTPUT% mkdir %OUTPUT%
if not exist "%OUTPUT%\bin" mkdir "%OUTPUT%\bin"
if not exist "%OUTPUT%\bin\x86" mkdir "%OUTPUT%\bin\x86"
if not exist "%OUTPUT%\bin\x64" mkdir "%OUTPUT%\bin\x64"
if not exist "%OUTPUT%\irm_mgmt_cli" mkdir "%OUTPUT%\irm_mgmt_cli"

:WIN32
copy /y "..\output\%BLD_NO%\win32_release-static\irm_host_agent.exe" "%OUTPUT%\bin\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_time_syc.exe" "%OUTPUT%\bin\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\netsh2.exe" "%OUTPUT%\bin\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\irm_conv_agent.exe" "%OUTPUT%\bin\x86\*.*"
copy /y "..\output\%BLD_NO%\win32_release-static\notice.exe" "%OUTPUT%\bin\x86\*.*"

:X64
copy /y "..\output\%BLD_NO%\x64_release-static\irm_host_agent.exe" "%OUTPUT%\bin\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_time_syc.exe" "%OUTPUT%\bin\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\netsh2.exe" "%OUTPUT%\bin\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_conv_agent.exe" "%OUTPUT%\bin\x64\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\notice.exe" "%OUTPUT%\bin\x64\*.*"

copy /y "..\output\%BLD_NO%\x64_release-static\irm_mgmt_cli.exe" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_mgmt_svc.exe" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_host_cli.exe" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_hv_cli.exe" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_converter_cli.exe" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_hypervisor_ex.dll" "%OUTPUT%\bin\*.*"
copy /y "..\output\%BLD_NO%\x64_release-static\irm_hv_image_transform.exe" "%OUTPUT%\bin\*.*"

copy /y "..\irm_mgmt_cli\*.h" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.cpp" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.rc" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.cpp" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.vcxproj" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.filters" "%OUTPUT%\irm_mgmt_cli\*.*"
copy /y "..\irm_mgmt_cli\*.txt" "%OUTPUT%\irm_mgmt_cli\*.*"

copy /y "..\promise_mgmt.thrift" "%OUTPUT%\*.*"
copy /y "%THRIFT_EXE%" "%OUTPUT%\*.*"

if exist "%OUTPUT%\gen-java" rd /S /Q "%OUTPUT%\gen-java"
call %THRIFT_EXE% --gen java -o "%OUTPUT%" %~dp0..\promise_mgmt.thrift 
call %THRIFT_EXE%  --gen cpp:cob_style -o "%OUTPUT%" %~dp0..\promise_mgmt.thrift
call %THRIFT_EXE%  -r --gen php -o "%OUTPUT%" %~dp0..\promise_mgmt.thrift

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
