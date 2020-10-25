Set ROOTPATH=%~dp0.
IF EXIST %windir%\System32\pnpclean.dll (
%windir%\System32\RUNDLL32.exe pnpclean.dll,RunDLL_PnpClean /DEVICES /MAXCLEAN
)

REM Identify OS.
ver | find /i " 10.0." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2016
ver | find /i " 6.3." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2012
ver | find /i " 6.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2012
ver | find /i " 6.1." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i " 6.0." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i " 5.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2003

echo %$VERSIONWINDOWS%
if %$VERSIONWINDOWS%=="" goto END
if %$VERSIONWINDOWS%==Windows2016 goto WIN2K16

%ROOTPATH%\7.4.1.0\xenbus\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.1.0\xeniface\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.1.0\xenvif\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.1.0\xennet\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.1.0\xenvbd\dpinst.exe /A /SE /SW /SA /F
goto END

:WIN2K16
%ROOTPATH%\7.4.3.0\xennet\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.3.0\xenvif\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.3.0\xenvbd\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.3.0\xeniface\dpinst.exe /A /SE /SW /SA /F
%ROOTPATH%\7.4.3.0\xenbus\dpinst.exe /A /SE /SW /SA /F

:END