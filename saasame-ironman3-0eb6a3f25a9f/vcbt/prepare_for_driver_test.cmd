CertMgr.exe /add vcbt.cer /s /r localMachine root
CertMgr.exe /add vcbt.cer /s /r localMachine trustedpublisher
Bcdedit.exe -set TESTSIGNING ON

pause