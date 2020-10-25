@echo off
set OLD_PATH=%PATH%
Set PROJECTPATH="%~dp0.."
if "%1"=="" (
echo.
echo.
echo Please Enter Output Folder
echo.
echo.
goto error
)

if "%2"=="" (
Set ADK_VERSION=10
) else (
Set ADK_VERSION=%2
)

Set ADK_PATH=C:\Program Files (x86)\Windows Kits\%ADK_VERSION%\Assessment and Deployment Kit
Set Output=%1%

cd "%ADK_PATH%\Deployment Tools"
call DandISetEnv.bat

rd /Q /S "%Output%\WinPE_x86"
call copype x86 "%Output%\WinPE_x86"
Dism /Mount-Image /ImageFile:"%Output%\WinPE_x86\media\sources\boot.wim" /index:1 /MountDir:"%Output%\WinPE_x86\mount"

Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-WMI.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-WMI_en-us.cab"
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-NetFX.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-NetFX_en-us.cab"
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-Scripting.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-Scripting_en-us.cab"
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-PowerShell.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-PowerShell_en-us.cab"
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-StorageWMI.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-StorageWMI_en-us.cab"
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\WinPE-DismCmdlets.cab"  
Dism /Add-Package /Image:"%Output%\WinPE_x86\mount" /PackagePath:"%ADK_PATH%\Windows Preinstallation Environment\x86\WinPE_OCs\en-us\WinPE-DismCmdlets_en-us.cab"

Dism /Get-Packages /Image:"%Output%\WinPE_x86\mount"
Dism /Unmount-Image /MountDir:"%Output%\WinPE_x86\mount" /commit

goto end

:error
echo "Skip...."

:end

cd %PROJECTPATH%\build
set PATH=%OLD_PATH%