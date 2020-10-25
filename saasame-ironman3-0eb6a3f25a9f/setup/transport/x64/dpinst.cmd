"%~dp0dpinst.exe" /sa /se /sw /sh /lm /c

REM Identify OS.
ver | find /i "6.3." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=windows2012
ver | find /i "6.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2012
ver | find /i "6.1." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i "6.0." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i "5.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2003

if %$VERSIONWINDOWS%=="" goto END
if %$VERSIONWINDOWS%==Windows2003 goto WIN2K3

goto END

:WIN2K3
net user saasame /ACTIVE:NO
net user saasame /DELETE
shutdown -l

:END
