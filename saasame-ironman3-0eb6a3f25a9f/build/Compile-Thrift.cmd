@echo OFF

Set PROJECTPATH=%~dp0..

call "%PROJECTPATH%\build\setenvmsvc2013.cmd"

Set BOOST_ROOT="%PROJECTPATH%\macho\includes\boost-1_67"
Set LIBEVENT_ROOT="%PROJECTPATH%\opensources\libevent-2.0.21-stable"
Set OPENSSL_ROOT_DIR="%PROJECTPATH%\opensources\openssl-1.0.2j"
Set THRIFT_CPP="%PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\src"

call "%PROJECTPATH%\build\makeThrift.cmd" Debug-mt win32 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Debug win32 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release-mt win32 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release win32 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release-mt-2k3 win32 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Debug-mt x64 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Debug x64 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release-mt x64 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release x64 
if errorlevel == 1 goto error

call "%PROJECTPATH%\build\makeThrift.cmd" Release-mt-2k3 x64 
if errorlevel == 1 goto error

copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Debug-mt\*.lib %PROJECTPATH%\macho\libs\Debug-StaticWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Debug\*.lib %PROJECTPATH%\macho\libs\DebugWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release-mt\*.lib %PROJECTPATH%\macho\libs\Release-StaticWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release\*.lib %PROJECTPATH%\macho\libs\ReleaseWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release-mt-2k3\*.lib %PROJECTPATH%\macho\libs\Release-Static-2k3Win32 /y

copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Debug-mt\*.pdb %PROJECTPATH%\macho\libs\Debug-StaticWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Debug\*.pdb %PROJECTPATH%\macho\libs\DebugWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release-mt\*.pdb %PROJECTPATH%\macho\libs\Release-StaticWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release\*.pdb %PROJECTPATH%\macho\libs\ReleaseWin32 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\Release-mt-2k3\*.pdb %PROJECTPATH%\macho\libs\Release-Static-2k3Win32 /y

copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Debug-mt\*.lib %PROJECTPATH%\macho\libs\Debug-Staticx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Debug\*.lib %PROJECTPATH%\macho\libs\Debugx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release-mt\*.lib %PROJECTPATH%\macho\libs\Release-Staticx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release\*.lib %PROJECTPATH%\macho\libs\Releasex64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release-mt-2k3\*.lib %PROJECTPATH%\macho\libs\Release-Static-2k3x64 /y

copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Debug-mt\*.pdb %PROJECTPATH%\macho\libs\Debug-Staticx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Debug\*.pdb %PROJECTPATH%\macho\libs\Debugx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release-mt\*.pdb %PROJECTPATH%\macho\libs\Release-Staticx64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release\*.pdb %PROJECTPATH%\macho\libs\Releasex64 /y
copy %PROJECTPATH%\opensources\thrift-0.12.0\lib\cpp\x64\Release-mt-2k3\*.pdb %PROJECTPATH%\macho\libs\Release-Static-2k3x64 /y
goto end

:error
echo "Skip compile"

:end
