%~dp0netsh2.exe -r
%~dp0irm_time_syc.exe
%~dp0vcbtcli.exe -a -d

REM Identify manufacturer
set $MANUFACTURER=UNKNOWN
systeminfo | find /i "OpenStack" > nul
if %errorlevel%==0 set $MANUFACTURER=OpenStack
systeminfo | find /i "Alibaba Cloud" > nul
if %errorlevel%==0 set $MANUFACTURER=Alibaba

REM Identify OS.
set $VERSIONWINDOWS=UNKNOWN
ver | find /i " 10.0." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=windows2016
ver | find /i " 6.3." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=windows2012
ver | find /i " 6.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2012
ver | find /i " 6.1." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i " 6.0." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2008
ver | find /i " 5.2." > nul
if %errorlevel%==0 set $VERSIONWINDOWS=Windows2003

if %$MANUFACTURER%==UNKNOWN goto END
if %$VERSIONWINDOWS%==UNKNOWN goto END

if %$VERSIONWINDOWS%==Windows2003 goto WIN2K3

goto END

:WIN2K3

net user saasame /DELETE
net user saasame abc@123 /ADD /PASSWORDCHG:NO
net localgroup administrators saasame /add

REG ADD "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" /v DefaultUserName /t REG_SZ /d saasame /f
REG ADD "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" /v DefaultPassword /t REG_SZ /d abc@123 /f
REG ADD "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon" /v AutoAdminLogon /t REG_SZ /d 1 /f

:END

IF EXIST %~dp0PostScript\RUN-SAFE.cmd (
%~dp0PostScript\RUN-SAFE.cmd
)