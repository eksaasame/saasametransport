ECHO OFF

IF "%1"=="" GOTO ERRORBOOSTZIP 
IF "%2"=="" GOTO ERRORBOOST
IF "%3"=="" ( 
SET BUILDPARAM=all 
) ELSE (
SET BUILDPARAM=%3
)

REM Set BOOST=boost_1_48_0
REM Set BOOSTINCSFOLDER=boost-1_48

Set BOOST=%1
Set BOOSTINCSFOLDER=%2

Set OLDCD=%cd%

REM %~dp0 == currrent batch folder. %0 == current batch file path. 
Set PROJECTPATH="%~dp0.."

set BOOSTFOLDER=%PROJECTPATH%\%BOOST%
set BOOSTBUILDLOG=%PROJECTPATH%\Boost-Config.log

date /t > %BOOSTBUILDLOG%
time /t >> %BOOSTBUILDLOG%

IF NOT EXIST %BOOSTFOLDER% (
 ECHO "Unzip %BOOST%.zip file....."	
 cd /d %~dp0
 call unzip.exe %PROJECTPATH%\%BOOST%.zip -d %PROJECTPATH% >> %BOOSTBUILDLOG%
)

set OUTPUTPATH=%PROJECTPATH%
Set INCLUDEPATH=%OUTPUTPATH%\includes
Set LIBSPATH=%OUTPUTPATH%\libs\%BOOSTINCSFOLDER%

IF NOT EXIST %BOOSTFOLDER%\b2.exe (
 cd /d %BOOSTFOLDER%
 call .\bootstrap.bat
)

md %LIBSPATH%
cd /d %~dp0
ECHO "Set VC 2013 build environment....."
call "setenvmsvc2013.cmd"

REM dynamic link standard c++ libs 

cd /d %BOOSTFOLDER%

GOTO %BUILDPARAM%

:all
ECHO "Build boost x86 library....."
ECHO "Build boost x86 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 stage >> %BOOSTBUILDLOG%

ECHO "Install boost x86 library....."
ECHO "Install boost x86 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\Win32 install >> %BOOSTBUILDLOG%

ECHO "Build boost x64 library....."
ECHO "Build boost x64 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 stage >> %BOOSTBUILDLOG%

ECHO "Install boost x64 library....."
ECHO "Install boost x64 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\x64 install >> %BOOSTBUILDLOG%

ECHO "Clean up temp boost output libraries..."
ECHO "Clean up temp boost output libraries..." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe --clean >> %BOOSTBUILDLOG%

REM Static link standard c++ libs

ECHO "Build boost x86 static library....."
ECHO "Build boost x86 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 stage debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

ECHO "Install boost x86 static library....."
ECHO "Install boost x86 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\Win32 install debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

ECHO "Build boost x64 static library....."
ECHO "Build boost x64 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 stage debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

ECHO "Install boost x64 static library....."
ECHO "Install boost x64 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\x64 install debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

Goto UPDATEENV

:x86
ECHO "Build boost x86 library....."
ECHO "Build boost x86 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 stage >> %BOOSTBUILDLOG%

ECHO "Install boost x86 library....."
ECHO "Install boost x86 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\Win32 install >> %BOOSTBUILDLOG%
Goto UPDATEENV

:x64
ECHO "Build boost x64 library....."
ECHO "Build boost x64 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 stage >> %BOOSTBUILDLOG%

ECHO "Install boost x64 library....."
ECHO "Install boost x64 library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\x64 install >> %BOOSTBUILDLOG%
Goto UPDATEENV

:x86-static
ECHO "Build boost x86 static library....."
ECHO "Build boost x86 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 stage debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

ECHO "Install boost x86 static library....."
ECHO "Install boost x86 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\Win32 install debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%
Goto UPDATEENV

:x64-static
ECHO "Build boost x64 static library....."
ECHO "Build boost x64 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 stage debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%

ECHO "Install boost x64 static library....."
ECHO "Install boost x64 static library....." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe msvc architecture=x86 address-model=64 --includedir=%INCLUDEPATH% --libdir=%LIBSPATH%\x64 install debug release threading=multi link=static runtime-link=static >> %BOOSTBUILDLOG%
Goto UPDATEENV

:UPDATEENV
ECHO "Clean up temp boost output libraries..."
ECHO "Clean up temp boost output libraries..." >> %BOOSTBUILDLOG%
call %BOOSTFOLDER%\bjam.exe --clean >> %BOOSTBUILDLOG%

ECHO "Update boost environment variables..."
cd /d %~dp0
call SetEnv.exe BOOSTINCS %INCLUDEPATH%\%BOOSTINCSFOLDER%
call SetEnv.exe BOOSTLIBS %LIBSPATH%

cd /d "%OLDCD%"
rd %BOOSTFOLDER% /S /Q

Goto END
:ERRORBOOST
Echo "Please input Boost folder name"
Goto END

:ERRORBOOSTZIP
Echo "Please input Boost Zip file name"
Goto END

:END
REM =========================================