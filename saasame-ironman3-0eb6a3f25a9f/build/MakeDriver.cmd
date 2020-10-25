@echo OFF

set SIGN1=signtool sign /v /ac "..\build\MSCV-VSClass3.cer" /f ..\build\saasame.pfx /p albert /n "SaaSaMe Ltd." /fd sha1 /tr http://sha1timestamp.ws.symantec.com/sha1/timestamp
set SIGN=signtool sign /v /ac "..\build\MSCV-VSClass3.cer" /f ..\build\saasame-sha2.pfx /p albert /n "SaaSaMe Ltd." /fd sha256 /tr http://sha256timestamp.ws.symantec.com/sha256/timestamp /td sha256 /as 

if "%1"=="" goto usage
if "%2"=="" goto usage
if "%3"=="" goto usage

set BLD_CONFIG=%1
set BLD_PLATFORM=%2
set BLD_TARGET=%3

if "%4"=="copy" goto copy

if errorlevel == 1 goto error

echo %DATE% %TIME%
echo "VCBT;platform=%BLD_PLATFORM%;config=%BLD_CONFIG%;target=%BLD_TARGET%"

msbuild /t:clean /t:build ..\vcbt\vcbt.sln /p:Configuration="%BLD_TARGET% %BLD_CONFIG%" /p:Platform=%BLD_PLATFORM% /p:TargetVersion="%BLD_TARGET%" > ..\vcbt-%BLD_CONFIG%-%BLD_PLATFORM%.log

if errorlevel == 1 goto error

:copy

rd /S /Q "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%"
rd /S /Q "..\output\%BLD_NO%\driver\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if not exist "..\output"							mkdir "..\output"
if not exist "..\output\%BLD_NO%" 						mkdir "..\output\%BLD_NO%" 
if not exist "..\output\%BLD_NO%\driver" 					mkdir "..\output\%BLD_NO%\driver"
if not exist "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%" 		mkdir "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%"
if not exist "..\output\%BLD_NO%\driver\Symbols" 				mkdir "..\output\%BLD_NO%\driver\Symbols"	
if not exist "..\output\%BLD_NO%\driver\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%" 	mkdir "..\output\%BLD_NO%\driver\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%"

if "%2"=="x64" goto X64
if "%2"=="X64" goto X64

:WIN32

copy /y "..\vcbt\%BLD_TARGET%%BLD_CONFIG%\*.cer" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.sys" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.inf" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.cat" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_TARGET%%BLD_CONFIG%\*.pdb" "..\output\%BLD_NO%\driver\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

%SIGN1% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.cat"
%SIGN% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.cat"
%SIGN1% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.sys"
%SIGN% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.sys"

goto exit

:X64

copy /y "..\vcbt\%BLD_PLATFORM%\%BLD_TARGET%%BLD_CONFIG%\*.cer" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_PLATFORM%\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.sys" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_PLATFORM%\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.inf" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_PLATFORM%\%BLD_TARGET%%BLD_CONFIG%\vcbt Package\*.cat" "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"
copy /y "..\vcbt\%BLD_PLATFORM%\%BLD_TARGET%%BLD_CONFIG%\*.pdb" "..\output\%BLD_NO%\driver\Symbols\%BLD_PLATFORM%_%BLD_CONFIG%\*.*"

%SIGN1% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.cat"
%SIGN% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.cat"
%SIGN1% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.sys"
%SIGN% "..\output\%BLD_NO%\driver\%BLD_PLATFORM%_%BLD_CONFIG%\vcbt.sys"

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