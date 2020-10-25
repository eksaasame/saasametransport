@ECHO OFF
IF EXIST .\Makefile del .\Makefile
IF EXIST crypto\opensslconf.h del crypto\opensslconf.h
Perl Configure no-asm VC-WIN64A
CALL ms\do_win64a.bat
nmake /NOLOGO /f ms\ntdll.mak clean
ECHO Compiling Release AMD64
nmake /NOLOGO /f ms\ntdll.mak > Make.ReleaseX64.log

IF ERRORLEVEL == 1 GOTO END

IF NOT EXIST ..\LibDebugx64 MKDIR ..\LibDebugx64
IF EXIST ..\LibDebugx64\libeay32.dll del ..\LibDebugx64\libeay32.dll
IF EXIST ..\LibDebugx64\libeay32.lib del ..\LibDebugx64\libeay32.lib
IF EXIST ..\LibDebugx64\ssleay32.dll del ..\LibDebugx64\ssleay32.dll
IF EXIST ..\LibDebugx64\ssleay32.lib del ..\LibDebugx64\ssleay32.lib
copy out32dll\libeay32.dll ..\LibDebugx64
copy out32dll\libeay32.lib ..\LibDebugx64
copy out32dll\ssleay32.dll ..\LibDebugx64
copy out32dll\ssleay32.lib ..\LibDebugx64

IF NOT EXIST ..\LibReleasex64 MKDIR ..\LibReleasex64
IF EXIST ..\LibReleasex64\libeay32.dll del ..\LibReleasex64\libeay32.dll
IF EXIST ..\LibReleasex64\libeay32.lib del ..\LibReleasex64\libeay32.lib
IF EXIST ..\LibReleasex64\ssleay32.dll del ..\LibReleasex64\ssleay32.dll
IF EXIST ..\LibReleasex64\ssleay32.lib del ..\LibReleasex64\ssleay32.lib
copy out32dll\libeay32.dll ..\LibReleasex64
copy out32dll\libeay32.lib ..\LibReleasex64
copy out32dll\ssleay32.dll ..\LibReleasex64
copy out32dll\ssleay32.lib ..\LibReleasex64
:END