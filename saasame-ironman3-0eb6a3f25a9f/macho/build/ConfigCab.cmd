@echo off

set CABINF=sabrecloudctrl.inf

if exist %CABINF% copy %CABINF% %CABINF%.bak

echo ; INF file for SabreCloudCtrl.dll >> %CABINF%
echo [version] >> %CABINF%
echo ; version signature (same for both NT and Win95) do not remove >> %CABINF%
echo signature="$CHICAGO$" >> %CABINF%
echo AdvancedINF=2.0 >> %CABINF%  
echo [Add.Code] >> %CABINF%
echo SabreCloudCtrl.dll=SabreCloudCtrl.dll >> %CABINF%
echo ; needed DLL >> %CABINF%
echo [SabreCloudCtrl.dll] >> %CABINF%
echo file=thiscab >> %CABINF%
echo clsid={1C80447B-5139-4E45-9EC5-014C615F4C60} >> %CABINF% 
echo FileVersion=%PRODUCT_MS_VERSION_STR% >> %CABINF% 
echo RegisterServer=yes >> %CABINF%  
echo ; end of INF file >> %CABINF%


copy /Y %CABINF%  ..\%CABINF%
del /F  %CABINF% 