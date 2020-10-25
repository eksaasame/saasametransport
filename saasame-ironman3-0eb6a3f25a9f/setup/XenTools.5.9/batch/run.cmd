Set ROOTPATH=%~dp0.
%ROOTPATH%\copyvif.exe "emulated" %ROOTPATH%\copyvif.log.txt

REG ADD "HKLM\SYSTEM\CurrentControlSet\services\partmgr\Parameters" /v SanPolicy /t REG_DWORD /d 1 /f
REG ADD "HKLM\SYSTEM\CurrentControlSet\Services\xenevtchn" /v NOPVBoot /t REG_DWORD /d 1 /f

%ROOTPATH%\installdriver.exe "/i" "0" %ROOTPATH%\xenvbd.inf
%ROOTPATH%\installdriver.exe "/i" "0" %ROOTPATH%\xevtchn.inf
%ROOTPATH%\installdriver.exe "/i" "0" %ROOTPATH%\xenvif.inf
%ROOTPATH%\installdriver.exe "/i" "0" %ROOTPATH%\xennet.inf
%ROOTPATH%\installdriver.exe "/i" "0" %ROOTPATH%\xeniface.inf

%ROOTPATH%\removedev.exe "/d" "XENBUS\CLASS&IFACE"
%ROOTPATH%\removedev.exe "/d" "XEN\VIF"
%ROOTPATH%\removedev.exe "/d" "XENBUS\CLASS&VIF"

REG ADD "HKLM\SYSTEM\CurrentControlSet\Services\xenvbd\parameters\Device0" /v NumberOfRequests /t REG_DWORD /d 0x000000fe /f

%ROOTPATH%\installdriver.exe "/p" "0" "PCI\VEN_5853&DEV_0001&SUBSYS_00015853" %ROOTPATH%\xenvbd.inf "0"
%ROOTPATH%\installdriver.exe "/d" "0" %ROOTPATH%\xenvbd.inf "xenvbd_inst" "xenvbd_inst.services"

REG ADD "HKLM\SYSTEM\CurrentControlSet\Control\CriticalDeviceDatabase\pci#VEN_5853&dev_0001&subsys_00015853" /v Service /t REG_SZ /d xenvbd /f
REG ADD "HKLM\SYSTEM\CurrentControlSet\Control\CriticalDeviceDatabase\pci#VEN_5853&dev_0001&subsys_00015853" /v ClassGUID /t REG_SZ /d {4D36E97B-E325-11CE-BFC1-08002BE10318} /f

%ROOTPATH%\installdriver.exe "/r" "0" "ROOT\XENEVTCHN" %ROOTPATH%\xevtchn.inf
%ROOTPATH%\installdriver.exe "/p" "0" "XENBUS\CLASS&VIF" %ROOTPATH%\xenvif.inf "0"
%ROOTPATH%\installdriver.exe "/p" "0" "XEN\VIF" %ROOTPATH%\xennet.inf "0"
%ROOTPATH%\installdriver.exe "/p" "0" "XENBUS\CLASS&IFACE" %ROOTPATH%\xeniface.inf "0"

REG ADD "HKLM\SYSTEM\CurrentControlSet\Control" /v ServicesPipeTimeout /t REG_DWORD /d 0x000493e0 /f
REG ADD "HKLM\SYSTEM\CurrentControlSet\Control\CriticalDeviceDatabase\pci#VEN_8086&CC_0101" /v Service /t REG_SZ /d intelide /f
REG ADD "HKLM\SYSTEM\CurrentControlSet\Control\CriticalDeviceDatabase\pci#VEN_8086&CC_0101" /v Service /t REG_SZ /d {4D36E96A-E325-11CE-BFC1-08002BE10318} /f
REG ADD "HKLM\SYSTEM\CurrentControlSet\Services\Disk" /v TimeOutValue /t REG_DWORD /d 0x00000078 /f
REG DELETE "HKLM\SYSTEM\CurrentControlSet\Services\xenevtchn" /v NOPVBoot /f