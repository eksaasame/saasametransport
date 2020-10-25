@echo on
rem
rem basic contants for this build setup
rem

if "%1%" == "" goto getbuildno
set _buildno=%1
goto havebuildno

:getbuildno

@echo off
echo.
echo.
set /p _buildno=Enter Build # for DR Profiler:
echo.
echo.

:havebuildno
set SIGN=signtool sign /s my /sha1 CE20909B7751CCBDE0DCF4A58770877D71DE22B7 /t http://timestamp.verisign.com/scripts/timstamp.dll /v
set MAKEBUILDCD=%cd%

IF EXIST "%VS120COMNTOOLS%vsvars32.bat" (

    call ..\build\setenvmsvc2013.cmd
    cd /d %~dp0
    copy /y ..\DrProfiler\DrProfiler\res\DrProfiler.x64.rc2 ..\DrProfiler\DrProfiler\res\DrProfiler.rc2 
    call ..\build\make_build_2013.cmd DrProfiler release-static x64
    %SIGN%  ..\DrProfiler\x64\Release-Static\DrProfiler.exe
    cd /d %~dp0
    copy /y ..\DrProfiler\DrProfiler\res\DrProfiler.x86.rc2 ..\DrProfiler\DrProfiler\res\DrProfiler.rc2 
    call ..\build\make_build_2013.cmd DrProfiler release-static win32

) ELSE (
    goto error	
)

copy /y ..\output\%_buildno%\DrProfiler\win32_release-static\DrProfiler.exe ..\output\%_buildno%\DrProfiler.exe

IF "%2%" == "R" (
call ..\build\upx394w\upx.exe --lzma --best --ultra-brute ..\output\%_buildno%\DrProfiler.exe
) ELSE (
call ..\build\upx394w\upx.exe --lzma --best ..\output\%_buildno%\DrProfiler.exe
)

%SIGN% "..\output\%_buildno%\DrProfiler.exe"

GOTO end

:error

:end

