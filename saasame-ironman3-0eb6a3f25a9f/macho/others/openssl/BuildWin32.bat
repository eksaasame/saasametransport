@ECHO OFF
IF EXIST .\Makefile del .\Makefile
IF EXIST crypto\opensslconf.h del crypto\opensslconf.h
Perl Configure no-asm VC-WIN32
CALL ms\do_nt.bat
nmake /NOLOGO /f ms\ntdll.mak clean
ECHO Compiling Release Win32
nmake /NOLOGO /f ms\ntdll.mak > Make.ReleaseWin32.log

IF ERRORLEVEL == 1 GOTO END

IF NOT EXIST ..\LibDebugWin32 MKDIR ..\LibDebugWin32
IF EXIST ..\LibDebugWin32\libeay32.dll del ..\LibDebugWin32\libeay32.dll
IF EXIST ..\LibDebugWin32\libeay32.lib del ..\LibDebugWin32\libeay32.lib
IF EXIST ..\LibDebugWin32\ssleay32.dll del ..\LibDebugWin32\ssleay32.dll
IF EXIST ..\LibDebugWin32\ssleay32.lib del ..\LibDebugWin32\ssleay32.lib
copy out32dll\libeay32.dll ..\LibDebugWin32
copy out32dll\libeay32.lib ..\LibDebugWin32
copy out32dll\ssleay32.dll ..\LibDebugWin32
copy out32dll\ssleay32.lib ..\LibDebugWin32

IF NOT EXIST ..\LibReleaseWin32 MKDIR ..\LibReleaseWin32
IF EXIST ..\LibReleaseWin32\libeay32.dll del ..\LibReleaseWin32\libeay32.dll
IF EXIST ..\LibReleaseWin32\libeay32.lib del ..\LibReleaseWin32\libeay32.lib
IF EXIST ..\LibReleaseWin32\ssleay32.dll del ..\LibReleaseWin32\ssleay32.dll
IF EXIST ..\LibReleaseWin32\ssleay32.lib del ..\LibReleaseWin32\ssleay32.lib
copy out32dll\libeay32.dll ..\LibReleaseWin32
copy out32dll\libeay32.lib ..\LibReleaseWin32
copy out32dll\ssleay32.dll ..\LibReleaseWin32
copy out32dll\ssleay32.lib ..\LibReleaseWin32
:END