@echo off
rem
rem Some basic contants for this build setup
rem
set CONFIG_COPYRIGHT=Copyright 2006-2011 FalconStor Software.  All Rights Reserved.
set CONFIG_VERSION_MAJOR=1
set CONFIG_VERSION_MINOR=00

set INAS_MAJOR=1
set INAS_MINOR=0
set INAS_ISHIELD_VERSION=1.00.0

set VER_SUFFIX=

set PROD_REG=FalconStor
set PROD_REG_CLIENT=IPStor
set PROD_REG_VENDOR=FalconStor
set PROD_REG_STORAGESERVER=IPStor
set PROD_REG_SHORTNAME=stdsoap2

set PRODUCT_EVENTLOG_STR=stdsoap2

set PROD_LICENSECODE_V3=0
set PROD_LICENSECODE_V4=165


rem
rem Since there is no easy way to query for input from a lame DOS
rem batch file we force user to specify values during invokation
rem
if "%1%" == "" goto getbuildno
set _buildno=%1
goto havebuildno

:getbuildno

@echo off
set /p _buildno=Enter Build #:
:havebuildno
echo Build number will be %_buildno%!.



rem Now Set OEM Parameters
if "%2"=="generic" goto GenericSetUI
if "%2"=="falcon" goto FalconSetUI
if "%2"=="violin" goto ViolinSetUI


:FalconSetUI
set PRODUCT_VENDOR=FalconStor
set PRODUCT_VENDORNAME=FalconStor Software
set PRODUCT_NAME=stdsoap2
set VENDOR_ID=1
set PRODUCT_VENDOR_STR=FalconStor
set PRODUCT_VENDOR_WSTR=FalconStor
set PRODUCT_FULLNAME_STR=FalconStor� stdsoap2
set PRODUCT_NAME_STR=FalconStor stdsoap2
set PRODUCT_SHORTNAME_STR=stdsoap2
set PRODUCT_SHORTNAME_PREV_STR=stdsoap2
set PRODUCT_MMC_ROOTNODENAME=stdsoap2
set PRODUCT_SETUP_DEFAULT_DIR_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR2=FalconStor\\stdsoap2
set PRODUCT_SETUP_PRODUCT_NAME_STR=%PRODUCT_NAME_STR%
set PRODUCT_HELPFILE_BASE_STR=stdsoap2
goto check


:ViolinSetUI
set PRODUCT_VENDOR=Violin
set PRODUCT_VENDORNAME=Violin Memory
set PRODUCT_NAME=stdsoap2
set VENDOR_ID=6
set PRODUCT_VENDOR_STR=Violin
set PRODUCT_VENDOR_WSTR=Violin
set PRODUCT_FULLNAME_STR=Violin� stdsoap2
set PRODUCT_NAME_STR=Violin stdsoap2
set PRODUCT_SHORTNAME_STR=stdsoap2
set PRODUCT_SHORTNAME_PREV_STR=stdsoap2
set PRODUCT_MMC_ROOTNODENAME=stdsoap2
set PRODUCT_SETUP_DEFAULT_DIR_STR=SANClient\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR2=FalconStor\\stdsoap2
set PRODUCT_SETUP_PRODUCT_NAME_STR=%PRODUCT_NAME_STR%
set PRODUCT_HELPFILE_BASE_STR=stdsoap2
goto check


:GenericSetUI
set PRODUCT_VENDOR=
set PRODUCT_VENDORNAME=
set PRODUCT_NAME=stdsoap2
set VENDOR_ID=1
set PRODUCT_VENDOR_STR=
set PRODUCT_VENDOR_WSTR=
set PRODUCT_FULLNAME_STR=stdsoap2
set PRODUCT_NAME_STR=stdsoap2
set PRODUCT_SHORTNAME_STR=stdsoap2
set PRODUCT_SHORTNAME_PREV_STR=stdsoap2
set PRODUCT_MMC_ROOTNODENAME=stdsoap2
set PRODUCT_SETUP_DEFAULT_DIR_STR=stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_STR=stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR=FalconStor\\stdsoap2
set PRODUCT_SETUP_PROGRAM_MENU_PREV_STR2=FalconStor\\stdsoap2
set PRODUCT_SETUP_PRODUCT_NAME_STR=%PRODUCT_NAME_STR%
set PRODUCT_HELPFILE_BASE_STR=stdsoap2
goto check


:check


rem
rem We will not write over any existing configuration files
rem that may be here. Just warn the user to clean them up
rem

if exist buildenv.mk goto exists
if exist buildenv.h goto exists
goto allok

:exists
echo.
echo WARNING: You must check out buildenv.h and buildenv.mk first
echo before you run configure.bat
pause
rem if exist buildenv.mk attrib -R buildenv.mk
rem if exist buildenv.h  attrib -R buildenv.h 

if exist buildenv.mk copy buildenv.mk buildenv.mk.bak
if exist buildenv.h copy buildenv.h buildenv.h.bak

:allok
set CONFIG_DEBUG=n

echo Creating the buildenv.h...
echo /*> buildenv.h
echo * Generated by 'Configure.bat' on %DATE% %TIME%-- don't edit!>> buildenv.h
echo */>> buildenv.h
echo #ifndef _IPSTOR_CONFIG_H>> buildenv.h
echo #define _IPSTOR_CONFIG_H>> buildenv.h
echo #define CONFIG_DEBUG "%CONFIG_DEBUG%">> buildenv.h
echo #define CONFIG_BUILD_DATE "%DATE% %TIME%">> buildenv.h
echo #define CONFIG_HOME "%CD%">> buildenv.h
echo #define CONFIG_COPYRIGHT "%CONFIG_COPYRIGHT%">> buildenv.h
echo #define CONFIG_COPYRIGHT_WSTR L"%CONFIG_COPYRIGHT%">> buildenv.h
echo #define CONFIG_VERSION_MAJOR "%CONFIG_VERSION_MAJOR%">> buildenv.h
echo #define CONFIG_VERSION_MAJOR_WSTR L"%CONFIG_VERSION_MAJOR%">> buildenv.h
echo #define CONFIG_VERSION_MINOR "%CONFIG_VERSION_MINOR%">> buildenv.h
echo #define CONFIG_VERSION_MINOR_WSTR L"%CONFIG_VERSION_MINOR%">> buildenv.h
echo #define CONFIG_BUILD_ID "%_buildno%">> buildenv.h
echo #define CONFIG_BUILD_ID_WSTR L"%_buildno%">> buildenv.h
echo #define PRODUCT_LICENSECODE_V4 %PROD_LICENSECODE_V4% >> buildenv.h
echo #define PRODUCT_LICENSECODE_V3 %PROD_LICENSECODE_V3% >> buildenv.h
echo #define PRODUCT_MAJOR %INAS_MAJOR% >> buildenv.h
echo #define PRODUCT_MINOR %INAS_MINOR% >> buildenv.h
echo #define PRODUCT_BUILD %_buildno% >> buildenv.h
echo #define PRODUCT_NAME_STR "%PRODUCT_NAME_STR%">> buildenv.h
echo #define PRODUCT_NAME_WSTR L"%PRODUCT_NAME_STR%">> buildenv.h
echo #define PRODUCT_EVENTLOG_STR "%PRODUCT_EVENTLOG_STR%">> buildenv.h
echo #define PRODUCT_EVENTLOG_WSTR L"%PRODUCT_EVENTLOG_STR%">> buildenv.h
echo #define PRODUCT_FULLNAME_STR "%PRODUCT_FULLNAME_STR%">> buildenv.h
echo #define PRODUCT_FULLNAME_WSTR L"%PRODUCT_FULLNAME_STR%">> buildenv.h
echo #define PRODUCT_SHORTNAME_STR "%PRODUCT_SHORTNAME_STR%">> buildenv.h
echo #define PRODUCT_SHORTNAME_WSTR L"%PRODUCT_SHORTNAME_STR%">> buildenv.h
echo #define PRODUCT_HELPFILE_BASE_STR   "%PRODUCT_HELPFILE_BASE_STR%">> buildenv.h
echo #define PRODUCT_HELPFILE_BASE_WSTR L"%PRODUCT_HELPFILE_BASE_STR%">> buildenv.h
rem PRODUCT_SHORTNAME_PREV_STR not used
echo #define PRODUCT_SHORTNAME_PREV_STR "%PRODUCT_SHORTNAME_PREV_STR%">> buildenv.h
echo #define PRODUCT_INQUIRY_STR "%PRODUCT_INQUIRY_STR%">> buildenv.h
rem iscsiInstanceAttributesTableFunc() in snmp\win32\iscsi_mib.c
echo #define PRODUCT_VENDOR_STR "%PRODUCT_VENDOR_STR%">> buildenv.h
echo #define PRODUCT_VENDOR_WSTR L"%PRODUCT_VENDOR_WSTR%">> buildenv.h
rem PRODUCT_VENDORNAME_STR not used
echo #define PRODUCT_VENDORNAME_STR "%PRODUCT_VENDORNAME%">> buildenv.h
rem PRODUCT_VENDORNAME_WSTR in mmc->About...
echo #define PRODUCT_VENDORNAME_WSTR L"%PRODUCT_VENDORNAME%">> buildenv.h
echo #define PRODUCT_TARGET_PREFIX_STR "%PRODUCT_TARGET_PREFIX_STR%">> buildenv.h
echo #define PRODUCT_TARGET_PREFIX_WSTR L"%PRODUCT_TARGET_PREFIX_STR%">> buildenv.h
echo #define PRODUCT_VERSION_STR "%INAS_MAJOR%.%INAS_MINOR%-%_buildno% %VER_SUFFIX%">> buildenv.h
echo #define PRODUCT_VERSION_WSTR L"%INAS_MAJOR%.%INAS_MINOR%-%_buildno% %VER_SUFFIX%">> buildenv.h
echo #define PRODUCT_MMC_ROOTNODENAME_STR "%PRODUCT_MMC_ROOTNODENAME%">> buildenv.h
rem for root snap-in
echo #define PRODUCT_MMC_ROOTNODENAME_WSTR L"%PRODUCT_MMC_ROOTNODENAME%">> buildenv.h
echo #define PRODUCT_MS_VERSION_STR   "%INAS_MAJOR%,%INAS_MINOR%, 0, %_buildno%" >> buildenv.h

echo #define VENDOR_ID_FALCONSTOR     1 >>buildenv.h
echo #define VENDOR_ID_DELL           2 >>buildenv.h
echo #define VENDOR_ID_HP             3 >>buildenv.h
echo #define VENDOR_ID_SYSTEX         4 >>buildenv.h
echo #define VENDOR_ID_SEACHANGE      5 >>buildenv.h
echo #define VENDOR_ID_VIOLIN         6 >>buildenv.h
echo #define PRODUCT_VENDOR_ID        %VENDOR_ID% >> buildenv.h

echo /* For iShield8  */>> buildenv.h
echo #define PRODUCT_SETUP_PRODUCT_NAME_STR      "%PRODUCT_SETUP_PRODUCT_NAME_STR%">> buildenv.h
echo #define PRODUCT_SETUP_DEFAULT_DIR_STR       "%PRODUCT_SETUP_DEFAULT_DIR_STR%">> buildenv.h
echo #define PRODUCT_SETUP_PROGRAM_MENU_STR      "%PRODUCT_SETUP_PROGRAM_MENU_STR%">> buildenv.h

echo /* For registry  */>> buildenv.h
echo #define PROD_REGISTRY_STR "SOFTWARE\\%PROD_REG%">> buildenv.h
echo #define PROD_REGISTRY_WSTR L"SOFTWARE\\%PROD_REG%">> buildenv.h
echo #define PROD_HKLMREGISTRY_STR "HKLM\\SOFTWARE\\%PROD_REG%">> buildenv.h
echo #define PROD_HKLMREGISTRY_WSTR L"HKLM\\SOFTWARE\\%PROD_REG%">> buildenv.h
echo #define PROD_KERNELREGISTRY_STR "\\Registry\\Machine\\Software\\%PROD_REG%">> buildenv.h
echo #define PROD_KERNELREGISTRY_WSTR L"\\Registry\\Machine\\Software\\%PROD_REG%">> buildenv.h

echo /* For WOW registry  */>> buildenv.h
echo #define PROD_WOW_REGISTRY_STR "SOFTWARE\\Wow6432Node\\%PROD_REG%">> buildenv.h
echo #define PROD_WOW_REGISTRY_WSTR L"SOFTWARE\\Wow6432Node\\%PROD_REG%">> buildenv.h
echo #define PROD_WOW_HKLMREGISTRY_STR "HKLM\\SOFTWARE\\Wow6432Node\\%PROD_REG%">> buildenv.h
echo #define PROD_WOW_HKLMREGISTRY_WSTR L"HKLM\\SOFTWARE\\Wow6432Node\\%PROD_REG%">> buildenv.h
echo #define PROD_WOW_KERNELREGISTRY_STR "\\Registry\\Machine\\Software\\Wow6432Node\\%PROD_REG%">> buildenv.h
echo #define PROD_WOW_KERNELREGISTRY_WSTR L"\\Registry\\Machine\\Software\\Wow6432Node\\%PROD_REG%">> buildenv.h

echo #define PROD_REGISTRY_SHORTNAME_STR "%PROD_REG_SHORTNAME%">> buildenv.h
echo #define PROD_REGISTRY_SHORTNAME_WSTR L"%PROD_REG_SHORTNAME%">> buildenv.h

echo #define PROD_REGISTRY_CURRENTVERSION_STR "SOFTWARE\\%PROD_REG_VENDOR%\\%PROD_REG_STORAGESERVER%\\CurrentVersion">> buildenv.h
echo #define PROD_REGISTRY_CURRENTVERSION_WSTR L"SOFTWARE\\%PROD_REG_VENDOR%\\%PROD_REG_STORAGESERVER%\\CurrentVersion">> buildenv.h
echo #define PROD_WOW_REGISTRY_CURRENTVERSION_STR "SOFTWARE\\Wow6432Node\\%PROD_REG_VENDOR%\\%PROD_REG_STORAGESERVER%\\CurrentVersion">> buildenv.h
echo #define PROD_WOW_REGISTRY_CURRENTVERSION_WSTR L"SOFTWARE\\Wow6432Node\\%PROD_REG_VENDOR%\\%PROD_REG_STORAGESERVER%\\CurrentVersion">> buildenv.h


echo /* For Local and Remote Client  */>> buildenv.h

echo #define PROD_REGISTRY_CLIENT_STR "SOFTWARE\\%PROD_REG%\\%PROD_REG_CLIENT%">> buildenv.h
echo #define PROD_REGISTRY_CLIENT_WSTR L"SOFTWARE\\%PROD_REG%\\%PROD_REG_CLIENT%">> buildenv.h
echo #define PROD_HKLMREGISTRY_CLIENT_STR "HKLM\\SOFTWARE\\%PROD_REG%\\%PROD_REG_CLIENT%">> buildenv.h
echo #define PROD_HKLMREGISTRY_CLIENT_WSTR L"HKLM\\SOFTWARE\\%PROD_REG%\\%PROD_REG_CLIENT%">> buildenv.h

echo #endif /* ifndef _IPSTOR_CONFIG_H */>> buildenv.h

echo Creating the buildenv.mk...
echo CONFIG_DEBUG=%CONFIG_DEBUG%> buildenv.mk
echo CONFIG_BUILD_DATE=%DATE% %TIME%>> buildenv.mk
echo CONFIG_HOME=%CD%>> buildenv.mk
echo CONFIG_COPYRIGHT=%CONFIG_COPYRIGHT%>> buildenv.mk
echo CONFIG_VERSION_MAJOR=%CONFIG_VERSION_MAJOR% >> buildenv.mk
echo CONFIG_VERSION_MINOR=%CONFIG_VERSION_MINOR% >> buildenv.mk
echo CONFIG_BUILD_ID=%_buildno%>> buildenv.mk
echo CONFIG_NAS_VBDI=n>> buildenv.mk


rem copy /Y buildenv.h  ..\..\buildenv.h
rem copy /Y buildenv.mk ..\..\buildenv.mk
rem del /F  buildenv.h
rem del /F  buildenv.mk

rem if exist buildenv.mk attrib +R buildenv.mk
rem if exist buildenv.h  attrib +R buildenv.h 


goto fin

:usage
echo usage: Configure BuildNumber [VersionSuffix]
echo    BuildNumber - Specify the IPStor build number
echo    VersionSuffix - Extra text to append to the product version string
echo       e.g. "RC2", "Beta3"
goto fin

:fin
