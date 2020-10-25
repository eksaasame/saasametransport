set SIGN1=signtool sign /v /ac "..\build\MSCV-VSClass3.cer" /f ..\build\saasame.pfx /p albert /n "SaaSaMe Ltd." /fd sha1 /tr http://sha1timestamp.ws.symantec.com/sha1/timestamp

%SIGN1% test.exe