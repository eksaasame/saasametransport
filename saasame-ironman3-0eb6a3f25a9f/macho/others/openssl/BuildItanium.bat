@ECHO OFF
IF EXIST .\Makefile del .\Makefile
IF EXIST crypto\opensslconf.h del crypto\opensslconf.h
Perl Configure no-asm VC-WIN64I
CALL ms\do_win64i.bat
nmake /NOLOGO /f ms\ntdll.mak clean
ECHO Compiling Release Itanium
nmake /NOLOGO /f ms\ntdll.mak > Make.ReleaseIA64.log

IF ERRORLEVEL == 1 GOTO END

IF NOT EXIST ..\LibDebugItanium MKDIR ..\LibDebugItanium
IF EXIST ..\LibDebugItanium\libeay32.dll del ..\LibDebugItanium\libeay32.dll
IF EXIST ..\LibDebugItanium\libeay32.lib del ..\LibDebugItanium\libeay32.lib
IF EXIST ..\LibDebugItanium\ssleay32.dll del ..\LibDebugItanium\ssleay32.dll
IF EXIST ..\LibDebugItanium\ssleay32.lib del ..\LibDebugItanium\ssleay32.lib
copy out32dll\libeay32.dll ..\LibDebugItanium
copy out32dll\libeay32.lib ..\LibDebugItanium
copy out32dll\ssleay32.dll ..\LibDebugItanium
copy out32dll\ssleay32.lib ..\LibDebugItanium

IF NOT EXIST ..\LibReleaseItanium MKDIR ..\LibReleaseItanium
IF EXIST ..\LibReleaseItanium\libeay32.dll del ..\LibReleaseItanium\libeay32.dll
IF EXIST ..\LibReleaseItanium\libeay32.lib del ..\LibReleaseItanium\libeay32.lib
IF EXIST ..\LibReleaseItanium\ssleay32.dll del ..\LibReleaseItanium\ssleay32.dll
IF EXIST ..\LibReleaseItanium\ssleay32.lib del ..\LibReleaseItanium\ssleay32.lib
copy out32dll\libeay32.dll ..\LibReleaseItanium
copy out32dll\libeay32.lib ..\LibReleaseItanium
copy out32dll\ssleay32.dll ..\LibReleaseItanium
copy out32dll\ssleay32.lib ..\LibReleaseItanium
:END