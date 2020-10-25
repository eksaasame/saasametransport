REM Identify manufacturer
set $MANUFACTURER=UNKNOWN
systeminfo | find /i "Alibaba Cloud" > nul
if %errorlevel%==0 set $MANUFACTURER=Alibaba

if %$MANUFACTURER%==UNKNOWN goto NORMAL
if %$MANUFACTURER%==Alibaba goto SKIPRESET

:NORMAL
%~dp0netsh2.exe -r
%~dp0netsh2.exe -c %~dp0agent.cfg
%SystemRoot%\system32\netsh.exe advfirewall firewall set rule group="remote desktop" new enable=Yes
%SystemRoot%\system32\netsh.exe advfirewall firewall set rule group="@FirewallAPI.dll,-28752" new enable=Yes
%SystemRoot%\system32\net.exe stop iphlpsvc
%SystemRoot%\system32\net.exe stop Winmgmt
%SystemRoot%\system32\net.exe start Winmgmt
%SystemRoot%\system32\net.exe start iphlpsvc

:SKIPRESET

%~dp0irm_time_syc.exe
ipconfig /all
IF EXIST %~dp0PostScript\RUN.cmd (
%~dp0PostScript\RUN.cmd
)

IF EXIST %systemdrive%\SaaSaMe\RUNONCE.cmd (
%systemdrive%\SaaSaMe\RUNONCE.cmd
)