CertMgr.exe /add vmp.cer /s /r localMachine root
CertMgr.exe /add vmp.cer /s /r localMachine trustedpublisher
Bcdedit.exe -set TESTSIGNING ON

pause