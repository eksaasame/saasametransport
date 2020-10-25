// hardware_device.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_DEVICE__
#define __MACHO_WINDOWS_DEVICE__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "..\common\tracelog.hpp"
#include "boost\shared_ptr.hpp"
#include <setupapi.h>
#include <Cfgmgr32.h>

#pragma comment(lib, "setupapi.LIB")
#pragma comment(lib, "Cfgmgr32.LIB")

namespace macho{

namespace windows{

struct hardware_class{
    typedef boost::shared_ptr<hardware_class> ptr;
    typedef std::vector<ptr> vtr;

    stdstring       name;
    stdstring       description;
    GUID            guid;
    hardware_class( GUID _guid ) : guid( _guid ) {}
    hardware_class ( const hardware_class& _class ){
        copy(_class);
    }
    void copy ( const hardware_class& _class ){
        name        = _class.name;
        description = _class.description;
        guid        = _class.guid;
    }
    const hardware_class &operator =( const hardware_class& _class ){
        if ( this != &_class )
            copy( _class );
        return( *this );
    }
};

struct driver_file_path{
    typedef boost::shared_ptr<driver_file_path> ptr;
    typedef std::vector<ptr> vtr;
    stdstring target;
    stdstring source;  // not used for delete operations
    UINT      win32_error;
    DWORD     flags;   // such as SP_COPY_NOSKIP for copy errors
    driver_file_path() : win32_error(0), flags(0) {}
    driver_file_path ( const driver_file_path& _path ){
        copy(_path);
    }
    driver_file_path( const FILEPATHS& filePaths ){
        copy(filePaths);
    }
    void copy ( const FILEPATHS& filePaths ){
        target      = filePaths.Target;
        source      = filePaths.Source;
        win32_error = filePaths.Win32Error;
        flags       = filePaths.Flags;
    }
    void copy ( const driver_file_path& _path ){
        target      = _path.target;
        source      = _path.source;
        win32_error = _path.win32_error;
        flags       = _path.flags;
    }
    const driver_file_path &operator =( const driver_file_path& _path ){
        if ( this != &_path )
            copy( _path );
        return( *this );
    }
};

struct hardware_device{
    typedef boost::shared_ptr<hardware_device> ptr;
    typedef std::vector<ptr> vtr;
    stdstring                 device_instance_id;
    stdstring                 sz_class;
    stdstring                 sz_class_guid;
    stdstring                 device_description;
    stdstring                 parent;
    stdstring                 manufacturer;
    stdstring                 enumeter;
    stdstring                 physical_device_object;
    stdstring                 type;
    stdstring                 location_path;
    stdstring                 location;

    stdstring                 driver;
    stdstring                 inf_name;
    stdstring                 service;

    string_array              hardware_ids;
    string_array              compatible_ids;
    string_array              childs;
    ULONG                     status;
    ULONG                     install_state;
    ULONG                     problem;
  
    hardware_device() : status(0), problem(0), install_state(0) {}
    hardware_device ( const hardware_device& _device ){
        copy(_device);
    }
    void copy ( const hardware_device& _device ){
        device_instance_id      = _device.device_instance_id;
        sz_class                = _device.sz_class;
        sz_class_guid           = _device.sz_class_guid;
        device_description      = _device.device_description;
        parent                  = _device.parent;
        manufacturer            = _device.manufacturer;
        enumeter                = _device.enumeter;
        physical_device_object  = _device.physical_device_object;
        type                    = _device.type;
        location_path           = _device.location_path;
        location                = _device.location;

        service                 = _device.service;
        driver                  = _device.driver;
        inf_name                = _device.inf_name;

        hardware_ids            = _device.hardware_ids;
        compatible_ids          = _device.compatible_ids;
        childs                  = _device.childs;
        status                  = _device.status;
        install_state           = _device.install_state;
        problem                 = _device.problem;
    }

    const hardware_device &operator =( const hardware_device& _device ){
        if ( this != &_device )
            copy( _device );
        return( *this );
    }
    bool                      has_problem();
    bool                      is_disabled();
    bool                      is_oem();
};

struct hardware_driver{
    typedef boost::shared_ptr<hardware_driver> ptr;
    typedef std::vector<ptr> vtr;
    stdstring                 driver;
    stdstring                 inf_name;
    stdstring                 original_catalog_name;
    stdstring                 original_inf_name;
    stdstring                 inf_path;
    stdstring                 inf_section;
    stdstring                 driver_version;
    stdstring                 driver_date;
    stdstring                 service;
    stdstring                 matching_device_id;  
    stdstring                 catalog_file;
    stdstring                 digital_signer;
    stdstring                 digital_signer_version;
    driver_file_path::vtr    driver_files;  
    
    hardware_driver() {}
    hardware_driver ( const hardware_driver& _driver ){
        copy(_driver);
    }
    void copy ( const hardware_driver& _driver ){
        driver                  = _driver.driver;
        service                 = _driver.service;
        inf_name                = _driver.inf_name;
        original_catalog_name   = _driver.original_catalog_name;
        original_inf_name       = _driver.original_inf_name;
        inf_path                = _driver.inf_path;
        inf_section             = _driver.inf_section;
        driver_version          = _driver.driver_version;
        driver_date             = _driver.driver_date;
        matching_device_id      = _driver.matching_device_id;
        catalog_file            = _driver.catalog_file;
        digital_signer          = _driver.digital_signer;
        digital_signer_version  = _driver.digital_signer_version;
        driver_files            = _driver.driver_files;    
    }
    const hardware_driver &operator =( const hardware_driver& _driver ){
        if ( this != &_driver )
            copy( _driver );
        return( *this );
    }
    bool    is_oem();
};

class device_manager{
private:
    stdstring               _machine;
    hardware_device::ptr    get_device_info( HDEVINFO &devs, SP_DEVINFO_DATA &devInfo, SP_DEVINFO_LIST_DETAIL_DATA &devInfoListDetail );
    hardware_driver::ptr    get_device_driver_info( HDEVINFO &devs, SP_DEVINFO_DATA &devInfo, SP_DEVINFO_LIST_DETAIL_DATA &devInfoListDetail );
    string_table            get_device_multi_sz(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop);
    stdstring               get_device_string_property(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop);
    stdstring               get_device_description(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo);
    bool                    find_current_driver(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in PSP_DRVINFO_DATA DriverInfoData);
    bool                    dump_device_driver_files(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, hardware_driver* pDriver );
    static UINT CALLBACK    dump_device_drivers_callback(__in PVOID Context, __in UINT Notification, __in UINT_PTR Param1, __in UINT_PTR Param2);
	bool						 update(boost::filesystem::path& Inf, stdstring& HWID);
public:
	bool						 install(boost::filesystem::path Inf, stdstring HWID);
	bool						 remove_device(stdstring device_instance_id){
		bool reboot = false;
		return remove_device(device_instance_id, reboot);
	}
	bool						 remove_device(stdstring device_instance_id, bool& reboot);
    hardware_class::vtr          get_classes();
    HRESULT  static  WINAPI      rescan_devices();
    hardware_device::vtr         get_devices( stdstring base_name = _T("") );
    hardware_device::ptr         get_device( stdstring device_instance_id ); 
    hardware_driver::ptr         get_device_driver( stdstring device_instance_id ); 
    hardware_driver::ptr inline  get_device_driver( const hardware_device& device ){
        return get_device_driver( device.device_instance_id );
    }
    hardware_device::ptr         get_root_device();
    stdstring                    get_device_path( stdstring device_instance_id, LPCGUID pInterfaceClassGuid );
    DWORD                        get_disk_device_number( hardware_device &disk );
    device_manager( stdstring machine = _T("") ) : _machine(machine)  { }
};

#ifndef MACHO_HEADER_ONLY
#include <memory>
#include <regstr.h>

#define MAX_DEVICE_ID_LEN     200
#define MAX_DEVNODE_ID_LEN    MAX_DEVICE_ID_LEN

bool hardware_device::has_problem() { 
    return ( ( status & DN_HAS_PROBLEM ) == DN_HAS_PROBLEM ); 
}

bool hardware_device::is_disabled() {
    return ( has_problem() && ( ( problem & CM_PROB_DISABLED ) == CM_PROB_DISABLED ) ); 
}

bool hardware_device::is_oem(){
    if ( inf_name.length() > 3 ){
        stdstring szInfPrefix = inf_name.substr( 0, 3 );
        return !_tcsicmp( _T("oem"), szInfPrefix.c_str());
    }
    return false;
}
  
bool hardware_driver::is_oem(){
    if ( inf_name.length() > 3 ){
        stdstring szInfPrefix = inf_name.substr( 0, 3 );
        return !_tcsicmp( _T("oem"), szInfPrefix.c_str());
    }
    return false;
}

hardware_class::vtr device_manager::get_classes(){
    hardware_class::vtr    classes;
    DWORD                  reqGuids = 128;
    DWORD                  numGuids;
    std::auto_ptr<GUID>    guids;
    DWORD                  index;
    int                    failcode = 0;

    guids = std::auto_ptr<GUID>(new GUID[reqGuids]);
    if(!guids.get()) {
        failcode = ERROR_OUTOFMEMORY;
        goto final;
    }
    if(!SetupDiBuildClassInfoListEx(0,guids.get(),reqGuids,&numGuids,_machine.c_str(),NULL)) {
        do {
            if( ( failcode = GetLastError() ) != ERROR_INSUFFICIENT_BUFFER) {
                goto final;
            }
            reqGuids = numGuids;
            guids = std::auto_ptr<GUID>(new GUID[reqGuids]);
            if(!guids.get()) {
                failcode = ERROR_OUTOFMEMORY;
                goto final;
            }
        } while(!SetupDiBuildClassInfoListEx(0,guids.get(),reqGuids,&numGuids,_machine.c_str(),NULL));
    }
    for( index = 0; index < numGuids; ++index ) {
        TCHAR className[MAX_CLASS_NAME_LEN];
        TCHAR classDesc[LINE_LEN];
        if(!SetupDiClassNameFromGuidEx( &guids.get()[index], className, MAX_CLASS_NAME_LEN, NULL, _machine.c_str(), NULL)) {        
        }
        else{
            if(!SetupDiGetClassDescriptionEx(&guids.get()[index],classDesc,LINE_LEN,NULL, _machine.c_str(), NULL)) {
            }
            else{
                hardware_class::ptr _class = hardware_class::ptr(new hardware_class(guids.get()[index]));
                _class->name            = className;
                _class->description     = classDesc;
                classes.push_back( _class );
            }
        }
    }

    failcode = 0;

final:
    SetLastError(failcode);
    return classes;
}

hardware_device::ptr device_manager::get_device_info( HDEVINFO &devs, SP_DEVINFO_DATA &devInfo, SP_DEVINFO_LIST_DETAIL_DATA &devInfoListDetail ){
    
    TCHAR                devID[MAX_DEVICE_ID_LEN];
    DEVINST              dnDevInst;
    HKEY                 hDevKey;
    hardware_device::ptr device = hardware_device::ptr(new hardware_device());

    //
    // determine instance ID
    //
    if( CR_SUCCESS != CM_Get_Device_ID_Ex(devInfo.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle) ) {
        devID[0] = TEXT('\0');
    }

    if ( CR_SUCCESS != CM_Get_DevNode_Status_Ex(&device->status,&device->problem,devInfo.DevInst,0,devInfoListDetail.RemoteMachineHandle) ) {
       device->status  = 0;
       device->problem = 0;
    }

    device->device_instance_id       = devID;
    device->hardware_ids             = get_device_multi_sz(devs,&devInfo,SPDRP_HARDWAREID);
    device->compatible_ids           = get_device_multi_sz(devs,&devInfo,SPDRP_COMPATIBLEIDS);
    device->device_description       = get_device_description(devs,&devInfo);
    device->sz_class                 = get_device_string_property(devs,&devInfo,SPDRP_CLASS);
    if (device->sz_class.length()){
        device->sz_class_guid = get_device_string_property(devs, &devInfo, SPDRP_CLASSGUID);
        device->manufacturer = get_device_string_property(devs, &devInfo, SPDRP_MFG);
        device->enumeter = get_device_string_property(devs, &devInfo, SPDRP_ENUMERATOR_NAME);
        device->physical_device_object = get_device_string_property(devs, &devInfo, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
        device->type = get_device_string_property(devs, &devInfo, SPDRP_DEVTYPE);
        device->location = get_device_string_property(devs, &devInfo, SPDRP_LOCATION_INFORMATION);
        device->location_path = get_device_string_property(devs, &devInfo, SPDRP_LOCATION_PATHS);
        device->service = get_device_string_property(devs, &devInfo, SPDRP_SERVICE);
        device->driver = get_device_string_property(devs, &devInfo, SPDRP_DRIVER);
    }

    hDevKey = SetupDiOpenDevRegKey(devs,
                                &devInfo,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                               );

    if(hDevKey != INVALID_HANDLE_VALUE) {
        TCHAR szBuff[MAX_PATH] = {0};
        DWORD RegDataType;
        DWORD RegDataLength = sizeof( szBuff );
        if ( ERROR_SUCCESS == RegQueryValueEx(hDevKey, REGSTR_VAL_INFPATH, NULL, &RegDataType, (PBYTE)szBuff, &RegDataLength ) && 
            ( REG_SZ == RegDataType ) ){     
            device->inf_name = szBuff;
        }
        RegCloseKey(hDevKey);
    }

    if ( CR_SUCCESS == CM_Get_Parent_Ex(&dnDevInst,devInfo.DevInst,0,devInfoListDetail.RemoteMachineHandle) ) {
        devID[0] = TEXT('\0');
        if( CR_SUCCESS == CM_Get_Device_ID_Ex(dnDevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle) ) {
            device->parent = devID;
        }
    }

    if( CR_SUCCESS == CM_Get_Child_Ex( &dnDevInst, devInfo.DevInst,0,devInfoListDetail.RemoteMachineHandle) ) {
        device->childs.clear();
        do{
            devID[0] = TEXT('\0');
            if( CR_SUCCESS == CM_Get_Device_ID_Ex(dnDevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle) ) {
                device->childs.push_back( devID );
            }
        }while ( CR_SUCCESS == CM_Get_Sibling_Ex( &dnDevInst, dnDevInst, 0, devInfoListDetail.RemoteMachineHandle ) );          
    }
    return device;
}

hardware_driver::ptr device_manager::get_device_driver_info( HDEVINFO &devs, SP_DEVINFO_DATA &devInfo, SP_DEVINFO_LIST_DETAIL_DATA &devInfoListDetail ){

    HKEY                    hDevKey;
    hardware_driver::ptr    driver = hardware_driver::ptr(new hardware_driver());
    driver->driver                   = get_device_string_property(devs,&devInfo,SPDRP_DRIVER);
    driver->service                  = get_device_string_property(devs,&devInfo,SPDRP_SERVICE);

    hDevKey = SetupDiOpenDevRegKey(devs,
                                &devInfo,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                               );

    if(hDevKey != INVALID_HANDLE_VALUE) {
        TCHAR szBuff[MAX_PATH] = {0};
        DWORD RegDataType;
        DWORD RegDataLength = sizeof( szBuff );
        if ( ERROR_SUCCESS == RegQueryValueEx(hDevKey, REGSTR_VAL_INFPATH, NULL, &RegDataType, (PBYTE)szBuff, &RegDataLength ) && 
            ( REG_SZ == RegDataType ) ){     
            driver->inf_name = szBuff;
        }

        RegDataLength = sizeof( szBuff );
        if ( ERROR_SUCCESS == RegQueryValueEx(hDevKey, REGSTR_VAL_DRIVERVERSION, NULL, &RegDataType, (PBYTE)szBuff, &RegDataLength ) && 
            ( REG_SZ == RegDataType ) ){     
            driver->driver_version = szBuff;
        }

        RegDataLength = sizeof( szBuff );
        if ( ERROR_SUCCESS == RegQueryValueEx(hDevKey, REGSTR_VAL_MATCHINGDEVID, NULL, &RegDataType, (PBYTE)szBuff, &RegDataLength ) && 
            ( REG_SZ == RegDataType ) ){     
            driver->matching_device_id = szBuff;
        }

        RegDataLength = sizeof( szBuff );
        if ( ERROR_SUCCESS == RegQueryValueEx(hDevKey, REGSTR_VAL_DRIVERDATE, NULL, &RegDataType, (PBYTE)szBuff, &RegDataLength ) && 
            ( REG_SZ == RegDataType ) ){     
            driver->driver_date = szBuff;
        }
        RegCloseKey(hDevKey);
    }

    dump_device_driver_files( devs, &devInfo, driver.get() );
    PSP_INF_INFORMATION pspInfInfo = NULL;
    DWORD               cbSize     = 0;
    SetupGetInfInformation( driver->inf_path.c_str(), INFINFO_INF_NAME_IS_ABSOLUTE, pspInfInfo , cbSize , &cbSize );
    pspInfInfo = (PSP_INF_INFORMATION) malloc(  cbSize );
    if (  pspInfInfo ){
        if ( SetupGetInfInformation( driver->inf_path.c_str(), INFINFO_INF_NAME_IS_ABSOLUTE, pspInfInfo, cbSize, NULL ) ) {
            SP_ORIGINAL_FILE_INFO spOrgInfo = {0};
            spOrgInfo.cbSize = sizeof(SP_ORIGINAL_FILE_INFO);
            if ( SetupQueryInfOriginalFileInformation( pspInfInfo, 0, NULL, &spOrgInfo ) ) {
                driver->original_inf_name        = spOrgInfo.OriginalInfName;
                driver->original_catalog_name    = spOrgInfo.OriginalCatalogName;
            }
        }
        free( pspInfInfo );
    }
    if ( driver->original_catalog_name.length() > 0 ){
        SP_INF_SIGNER_INFO sp;
        sp.cbSize = sizeof(SP_INF_SIGNER_INFO) ;
        if ( SetupVerifyInfFile( driver->inf_path.c_str(), NULL, &sp ) ){
            driver->catalog_file = sp.CatalogFile;
            driver->digital_signer = sp.DigitalSigner;
            driver->digital_signer_version = sp.DigitalSignerVersion;
        }
    }
    return driver;
}

string_table device_manager::get_device_multi_sz(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop){
    LPTSTR          buffer;
    LPTSTR          scan;
    DWORD           size;
    DWORD           reqSize;
    DWORD           dataType;
    string_table    arrSz;
    DWORD           szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if( buffer) {
        while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto failed;
            }
            if(dataType != REG_MULTI_SZ) {
                goto failed;
            }
            size = reqSize;
            delete [] buffer;
            buffer = new TCHAR[(size/sizeof(TCHAR))+2];
            if(!buffer) {
                goto failed;
            }
        }
        szChars = reqSize/sizeof(TCHAR);
        buffer[szChars] = TEXT('\0');
        buffer[szChars+1] = TEXT('\0');

        for(scan = buffer; scan[0] ; ) {
            arrSz.push_back( scan );
            scan += lstrlen(scan)+1;
        }
    }
failed:
    if(buffer) {
        delete [] buffer;
    }
    return arrSz;
}
stdstring  device_manager::get_device_string_property(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop){
    LPTSTR      buffer;
    DWORD       size;
    DWORD       reqSize;
    DWORD       dataType;
    DWORD       szChars;
    stdstring   szStr;

    size = 1024; // initial guess
    buffer = new TCHAR[(size/sizeof(TCHAR))+1];
    if( buffer) {
        while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto failed;
            }
            if(dataType != REG_SZ) {
                goto failed;
            }
            size = reqSize;
            delete [] buffer;
            buffer = new TCHAR[(size/sizeof(TCHAR))+1];
            if(!buffer) {
                goto failed;
            }
        }

        szChars = reqSize/sizeof(TCHAR);
        buffer[szChars] = TEXT('\0');
        szStr = buffer;
    }

failed:
    if(buffer) {
        delete [] buffer;
    }
    return szStr;
}

stdstring device_manager::get_device_description(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo){
    stdstring desc;
    desc = get_device_string_property(Devs,DevInfo,SPDRP_FRIENDLYNAME);
    if( !desc.length() ) {
        desc = get_device_string_property(Devs,DevInfo,SPDRP_DEVICEDESC);
    }
    return desc;
}

bool device_manager::find_current_driver(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in PSP_DRVINFO_DATA DriverInfoData){
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    WCHAR SectionName[LINE_LEN];
    WCHAR DrvDescription[LINE_LEN];
    WCHAR MfgName[LINE_LEN];
    WCHAR ProviderName[LINE_LEN];
    HKEY hKey = NULL;
    DWORD RegDataLength;
    DWORD RegDataType;
    DWORD c;
    bool match = false;
    long regerr;

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return false;
    }

#ifdef DI_FLAGSEX_INSTALLEDDRIVER
    //
    // Set the flags that tell SetupDiBuildDriverInfoList to just put the
    // currently installed driver node in the list, and that it should allow
    // excluded drivers. This flag introduced in WinXP.
    //
    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if(SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        //
        // we were able to specify this flag, so proceed the easy way
        // we should get a list of no more than 1 driver
        //
        if(!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
            return false;
        }
        if (!SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER,
                                   0, DriverInfoData)) {
            return false;
        }
        //
        // we've selected the current driver
        //
        return true;
    }
    deviceInstallParams.FlagsEx &= ~(DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
#endif
    //
    // The following method works in Win2k, but it's slow and painful.
    //
    // First, get driver key - if it doesn't exist, no driver
    //
    hKey = SetupDiOpenDevRegKey(Devs,
                                DevInfo,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                               );

    if(hKey == INVALID_HANDLE_VALUE) {
        //
        // no such value exists, so there can't be an associated driver
        //
        RegCloseKey(hKey);
        return false;
    }

    //
    // obtain path of INF - we'll do a search on this specific INF
    //
    RegDataLength = sizeof(deviceInstallParams.DriverPath); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_INFPATH,
                             NULL,
                             &RegDataType,
                             (PBYTE)deviceInstallParams.DriverPath,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so no associated driver
        //
        RegCloseKey(hKey);
        return false;
    }

    //
    // obtain name of Provider to fill into DriverInfoData
    //
    RegDataLength = sizeof(ProviderName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_PROVIDER_NAME,
                             NULL,
                             &RegDataType,
                             (PBYTE)ProviderName,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return false;
    }

    //
    // obtain name of section - for final verification
    //
    RegDataLength = sizeof(SectionName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_INFSECTION,
                             NULL,
                             &RegDataType,
                             (PBYTE)SectionName,
                             &RegDataLength
                             );

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return false;
    }

    //
    // driver description (need not be same as device description)
    // - for final verification
    //
    RegDataLength = sizeof(DrvDescription); // bytes!!!
    regerr = RegQueryValueEx(hKey,
                             REGSTR_VAL_DRVDESC,
                             NULL,
                             &RegDataType,
                             (PBYTE)DrvDescription,
                             &RegDataLength
                             );

    RegCloseKey(hKey);

    if((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return false;
    }

    //
    // Manufacturer (via SPDRP_MFG, don't access registry directly!)
    //

    if(!SetupDiGetDeviceRegistryProperty(Devs,
                                        DevInfo,
                                        SPDRP_MFG,
                                        NULL,      // datatype is guaranteed to always be REG_SZ.
                                        (PBYTE)MfgName,
                                        sizeof(MfgName), // bytes!!!
                                        NULL)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return false;
    }

    //
    // now search for drivers listed in the INF
    //
    //
    deviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    deviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if(!SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return false;
    }
    if(!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
        return false;
    }

    //
    // find the entry in the INF that was used to install the driver for
    // this device
    //
    for(c=0;SetupDiEnumDriverInfo(Devs,DevInfo,SPDIT_CLASSDRIVER,c,DriverInfoData);c++) {
        if((_tcscmp(DriverInfoData->MfgName,MfgName)==0)
            &&(_tcscmp(DriverInfoData->ProviderName,ProviderName)==0)) {
            //
            // these two fields match, try more detailed info
            // to ensure we have the exact driver entry used
            //
            SP_DRVINFO_DETAIL_DATA detail;
            detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if(!SetupDiGetDriverInfoDetail(Devs,DevInfo,DriverInfoData,&detail,sizeof(detail),NULL)
                    && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
                continue;
            }
            if((_tcscmp(detail.SectionName,SectionName)==0) &&
                (_tcscmp(detail.DrvDescription,DrvDescription)==0)) {
                match = true;
                break;
            }
        }
    }

    return match;
}

bool device_manager::dump_device_driver_files(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, hardware_driver* pDriver ){
    //
    // do this by 'searching' for the current driver
    // mimmicing a copy-only install to our own file queue
    // and then parsing that file queue
    //
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;
    HSPFILEQ queueHandle = INVALID_HANDLE_VALUE;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD scanResult;
    bool success = false;

    ZeroMemory(&driverInfoData,sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);

    if(!find_current_driver(Devs,DevInfo,&driverInfoData)) {
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - FindCurrentDriver Error." ) );
        goto final;
    }

    //
    // get useful driver information
    //
    driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(Devs,DevInfo,&driverInfoData,&driverInfoDetail,sizeof(SP_DRVINFO_DETAIL_DATA),NULL) &&
       GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        //
        // no information about driver or section
        //
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupDiGetDriverInfoDetail Error." ) );
        goto final;
    }
    if(!driverInfoDetail.InfFileName[0] || !driverInfoDetail.SectionName[0]) {
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - InfFileName or SectionName is empty." ) );
        goto final;
    }

    pDriver->inf_path    = driverInfoDetail.InfFileName;
    pDriver->inf_section = driverInfoDetail.SectionName;

    if ( _machine.length() )
        goto final;
    //
    // pretend to do the file-copy part of a driver install
    // to determine what files are used
    // the specified driver must be selected as the active driver
    //
    if(!SetupDiSetSelectedDriver(Devs, DevInfo, &driverInfoData)) {
        dwRet = GetLastError();
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupDiSetSelectedDriver error (%d)" ), dwRet );
        goto final;
    }

    //
    // create a file queue so we can look at this queue later
    //
    queueHandle = SetupOpenFileQueue();

    if ( queueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        dwRet = GetLastError();
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupOpenFileQueue error (%d)" ), dwRet );
        goto final;
    }

    //
    // modify flags to indicate we're providing our own queue
    //
    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams) ) {
        dwRet = GetLastError();
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupDiGetDeviceInstallParams error (%d)" ), dwRet );
        goto final;
    }

    //
    // we want to add the files to the file queue, not install them!
    //
    deviceInstallParams.FileQueue = queueHandle;
    deviceInstallParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams) ) {
        dwRet = GetLastError();
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupDiSetDeviceInstallParams error (%d)" ), dwRet );
        goto final;
    }

    //
    // now fill queue with files that are to be installed
    // this involves all class/co-installers
    //
    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, Devs, DevInfo) ) {
        dwRet = GetLastError();
        LOG( LOG_LEVEL_ERROR, _T( "DumpDeviceDriverFiles - SetupDiCallClassInstaller error (%d)" ), dwRet );
        goto final;
    }

    scanResult = 0;
    pDriver->driver_files.clear();
    SetupScanFileQueue(queueHandle,SPQ_SCAN_USE_CALLBACKEX,NULL,device_manager::dump_device_drivers_callback,pDriver,&scanResult);
    success = true;

final:

    SetupDiDestroyDriverInfoList(Devs,DevInfo,SPDIT_CLASSDRIVER);

    if ( queueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        SetupCloseFileQueue(queueHandle);
    }
    return success;
}

UINT device_manager::dump_device_drivers_callback(__in PVOID Context, __in UINT Notification, __in UINT_PTR Param1, __in UINT_PTR Param2){
    
    hardware_driver*    pDriver = (hardware_driver *)Context;
    PFILEPATHS pfile = (PFILEPATHS)Param1;
    if ( pDriver ){
        pDriver->driver_files.push_back(driver_file_path::ptr(new driver_file_path(*pfile)));
    }
    return NO_ERROR;
}

HRESULT WINAPI device_manager::rescan_devices(){
    DWORD           dwIndex  = 0;
    DWORD           dwError  = ERROR_SUCCESS;
    HDEVINFO        hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA symDeviceInfoData;

    memset( &symDeviceInfoData, 0, sizeof( symDeviceInfoData ) );

    hDevInfo = SetupDiGetClassDevs( ( LPGUID )&DiskClassGuid, NULL, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES );
    if ( hDevInfo != INVALID_HANDLE_VALUE ){
        symDeviceInfoData.cbSize = sizeof( symDeviceInfoData );
        for ( dwIndex = 0; SetupDiEnumDeviceInfo( hDevInfo, dwIndex, &symDeviceInfoData ); dwIndex++ ){
            if ( CM_Reenumerate_DevNode( symDeviceInfoData.DevInst, 0 ) != CR_SUCCESS )
                continue;
        }
        SetupDiDestroyDeviceInfoList( hDevInfo );
    }
    else
        dwError = GetLastError();

    return HRESULT_FROM_WIN32( dwError );
}

hardware_device::vtr device_manager::get_devices( stdstring base_name ){

    DWORD                       dwRet           = ERROR_SUCCESS;
    GUID                        cls;
    DWORD                       numClass        = 0;
    HDEVINFO                    devs            = INVALID_HANDLE_VALUE;
    DWORD                       devIndex;
    SP_DEVINFO_DATA             devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    hardware_device::vtr        devices;

    if ( base_name.length() ){
        if ( !SetupDiClassGuidsFromNameEx( base_name.c_str(), &cls, 1, &numClass, _machine.c_str(), NULL ) )
            dwRet = GetLastError();
    }

    if ( numClass || !base_name.length() ){       
        devs = SetupDiGetClassDevsEx(numClass ? &cls : NULL,
                                     NULL,
                                     NULL,
                                     ( numClass ? 0 : DIGCF_ALLCLASSES ) | DIGCF_PRESENT,
                                     NULL,
                                     _machine.length() ? _machine.c_str() : NULL,
                                     NULL);

        if(devs == INVALID_HANDLE_VALUE) {
            dwRet = GetLastError();
        }
        else{
            devInfoListDetail.cbSize = sizeof(devInfoListDetail);
            if(!SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) {
                dwRet = GetLastError();
            }
            else{
                devInfo.cbSize = sizeof(devInfo);
                for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) {
                    hardware_device::ptr device = get_device_info( devs, devInfo, devInfoListDetail );
                    devices.push_back( device );
                }
            }
            SetupDiDestroyDeviceInfoList(devs);      
        }
    }

    SetLastError( dwRet );
    return devices;
}

hardware_device::ptr device_manager::get_device( stdstring device_instance_id ){

    DWORD                       dwRet = ERROR_SUCCESS;
    HDEVINFO                    hDeviceInfoList = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA             deviceInfo = { 0 };
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail = { 0 };
    hardware_device::ptr        device;
    deviceInfo.cbSize = sizeof(deviceInfo);
    hDeviceInfoList = SetupDiCreateDeviceInfoListEx( NULL, NULL, _machine.length() ? _machine.c_str() : NULL, NULL );
    if( hDeviceInfoList == INVALID_HANDLE_VALUE ) {
        dwRet = GetLastError();
    }
    else{
        if ( !SetupDiOpenDeviceInfo( hDeviceInfoList, device_instance_id.c_str(), NULL, 0, &deviceInfo) ){
            dwRet = GetLastError();
        }
        else{
            devInfoListDetail.cbSize = sizeof(devInfoListDetail);
            if(!SetupDiGetDeviceInfoListDetail( hDeviceInfoList, &devInfoListDetail ) ) {
                dwRet = GetLastError();
            }
            else{
                device = get_device_info( hDeviceInfoList, deviceInfo, devInfoListDetail );
            }
        }
        SetupDiDestroyDeviceInfoList( hDeviceInfoList );
    }
    SetLastError(dwRet); 
    return device;
}

bool device_manager::remove_device(stdstring device_instance_id, bool& reboot){
	DWORD                       dwRet = ERROR_SUCCESS;
	HDEVINFO                    hDeviceInfoList = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA             deviceInfo = { 0 };
	SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail = { 0 };
	hardware_device::ptr        device;
	deviceInfo.cbSize = sizeof(deviceInfo);
	hDeviceInfoList = SetupDiCreateDeviceInfoListEx(NULL, NULL, _machine.length() ? _machine.c_str() : NULL, NULL);
	if (hDeviceInfoList == INVALID_HANDLE_VALUE) {
		dwRet = GetLastError();
	}
	else{
		if (!SetupDiOpenDeviceInfo(hDeviceInfoList, device_instance_id.c_str(), NULL, 0, &deviceInfo)){
			dwRet = GetLastError();
		}
		else{
			devInfoListDetail.cbSize = sizeof(devInfoListDetail);
			if (!SetupDiGetDeviceInfoListDetail(hDeviceInfoList, &devInfoListDetail)) {
				dwRet = GetLastError();
			}
			else{
				SP_REMOVEDEVICE_PARAMS rmdParams;
				SP_DEVINSTALL_PARAMS devParams;
				rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
				rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
				rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
				rmdParams.HwProfile = 0;
				if (!SetupDiSetClassInstallParams(hDeviceInfoList, &deviceInfo, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) ||
					!SetupDiCallClassInstaller(DIF_REMOVE, hDeviceInfoList, &deviceInfo)) {
					dwRet = GetLastError();
				} 
				else {
					devParams.cbSize = sizeof(devParams);
					if (SetupDiGetDeviceInstallParams(hDeviceInfoList, &deviceInfo, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
						reboot = true;
					}
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDeviceInfoList);
	}
	SetLastError(dwRet);
	return ERROR_SUCCESS == dwRet;
}

hardware_device::ptr device_manager::get_root_device(){
    HDEVINFO                    hDeviceInfoList = INVALID_HANDLE_VALUE;
    TCHAR                       devID[MAX_DEVICE_ID_LEN];
    DWORD                       dwRet = ERROR_SUCCESS;
    hardware_device::ptr        root_device;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail = { 0 };
    hDeviceInfoList = SetupDiCreateDeviceInfoListEx( NULL, NULL, _machine.c_str(), NULL );
    if( hDeviceInfoList == INVALID_HANDLE_VALUE ) {
        dwRet = GetLastError();
    }
    else{
        CONFIGRET crResult  = CR_SUCCESS;
        devInfoListDetail.cbSize = sizeof(devInfoListDetail);
        if(!SetupDiGetDeviceInfoListDetail( hDeviceInfoList, &devInfoListDetail ) ) {
            dwRet = GetLastError();
        }
        else{
            DEVNODE dnRoot;
            if ( CR_SUCCESS != ( crResult = CM_Locate_DevNode_Ex(&dnRoot, NULL, 0, devInfoListDetail.RemoteMachineHandle ) ) ) {   
            }
            else if( CR_SUCCESS == ( crResult = CM_Get_Device_ID_Ex(dnRoot,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle) ) ){
                root_device = get_device( devID );
            }
            dwRet = crResult;
            SetupDiDestroyDeviceInfoList( hDeviceInfoList );
        }
    }

    SetLastError( dwRet );
    return root_device;
}

stdstring  device_manager::get_device_path( stdstring device_instance_id, LPCGUID pInterfaceClassGuid ){
    stdstring                           szDevPath;
    HDEVINFO                            IntDevInfo              = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA            interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    interfaceDetailData     = NULL;
    BOOL                                status;
    ULONG                               length = 0,
                                        returned = 0;
    DWORD                               interfaceDetailDataSize,
                                        reqSize,
                                        errorCode;
    
    IntDevInfo = SetupDiGetClassDevsEx( (LPGUID)pInterfaceClassGuid, 
        device_instance_id.c_str(), 
        NULL, 
        (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE ), 
        NULL, 
        _machine.c_str(), 
        NULL );

    if(IntDevInfo == INVALID_HANDLE_VALUE) {
        errorCode = GetLastError();
    }
    else{
        interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

        status = SetupDiEnumDeviceInterfaces ( 
                    IntDevInfo,             // Interface Device Info handle
                    0,                      // Device Info data
                    (LPGUID)pInterfaceClassGuid, // Interface registered by driver
                    0,                      // Member
                    &interfaceData          // Device Interface Data
                    );

        if ( status == FALSE ) {
            errorCode = GetLastError();
            if ( errorCode == ERROR_NO_MORE_ITEMS ) {
            }
        }
        else{           
            //
            // Find out required buffer size, so pass NULL 
            //
            status = SetupDiGetDeviceInterfaceDetail (
                        IntDevInfo,         // Interface Device info handle
                        &interfaceData,     // Interface data for the event class
                        NULL,               // Checking for buffer size
                        0,                  // Checking for buffer size
                        &reqSize,           // Buffer size required to get the detail data
                        NULL                // Checking for buffer size
                        );

            //
            // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
            // set to the required buffer size. Ignore the above error and
            // pass a bigger buffer to get the detail data
            //

            if ( status == FALSE ) {
                errorCode = GetLastError();
            }
            
            if(errorCode == ERROR_INSUFFICIENT_BUFFER ){
                //
                // Allocate memory to get the interface detail data
                // This contains the devicepath we need to open the device
                //
                errorCode = ERROR_SUCCESS;
                interfaceDetailDataSize = reqSize;
                interfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc (interfaceDetailDataSize);

                if ( interfaceDetailData == NULL ) {
                    errorCode = ERROR_OUTOFMEMORY;
                }
                else {
                    interfaceDetailData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
                    status = SetupDiGetDeviceInterfaceDetail (
                                  IntDevInfo,               // Interface Device info handle
                                  &interfaceData,           // Interface data for the event class
                                  interfaceDetailData,      // Interface detail data
                                  interfaceDetailDataSize,  // Interface detail data size
                                  &reqSize,                 // Buffer size required to get the detail data
                                  NULL);                    // Interface device info

                    if ( !status ) {
                        errorCode = GetLastError();
                    }
                    else {
                        //
                        // Now we have the device path. Open the device interface
                        // to send Pass Through command
                        szDevPath = interfaceDetailData->DevicePath;
                    }
                    free (interfaceDetailData);               
                }
            }
        }
        SetupDiDestroyDeviceInfoList( IntDevInfo );
    }
    
    SetLastError( errorCode );
    return szDevPath;
}

DWORD                   device_manager::get_disk_device_number( hardware_device &disk ){
    DWORD                       dwDeviceNumber  = -1;
    DWORD                       errorCode       = ERROR_SUCCESS;
    DWORD                       cbReturned;
    HANDLE                      hHandle         = INVALID_HANDLE_VALUE;
    STORAGE_DEVICE_NUMBER       sdn;

    if ( !_tcsicmp( disk.sz_class.c_str(), _T("DISKDRIVE") ) ){

        stdstring szDevPath = get_device_path( disk.device_instance_id, (LPGUID)&GUID_DEVINTERFACE_DISK );        
        if ( !szDevPath.length() )
            errorCode = GetLastError();
        else{
            hHandle =  CreateFile( szDevPath.c_str(), 
                                    GENERIC_READ | GENERIC_WRITE, 
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL, 
                                    OPEN_EXISTING, 
                                    FILE_FLAG_NO_BUFFERING, 
                                    NULL );
            if ( hHandle == INVALID_HANDLE_VALUE )
                errorCode = GetLastError();
            else {
                if ( !DeviceIoControl( hHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &cbReturned, NULL ) )
                    errorCode = GetLastError();
                else
                    dwDeviceNumber = sdn.DeviceNumber;
                CloseHandle( hHandle );
            }
        }
    }
    SetLastError( errorCode );
    return dwDeviceNumber;
}

hardware_driver::ptr  device_manager::get_device_driver( stdstring device_instance_id ){
    DWORD                       dwRet = ERROR_SUCCESS;
    HDEVINFO                    hDeviceInfoList = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA             deviceInfo = { 0 };
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail = { 0 };
    hardware_driver::ptr        driver;
    deviceInfo.cbSize = sizeof(deviceInfo);
    hDeviceInfoList = SetupDiCreateDeviceInfoListEx( NULL, NULL, _machine.length() ? _machine.c_str() : NULL, NULL );
    if( hDeviceInfoList == INVALID_HANDLE_VALUE ) {
        dwRet = GetLastError();
    }
    else{
        if ( !SetupDiOpenDeviceInfo( hDeviceInfoList, device_instance_id.c_str(), NULL, 0, &deviceInfo) ){
            dwRet = GetLastError();
        }
        else{
            devInfoListDetail.cbSize = sizeof(devInfoListDetail);
            if(!SetupDiGetDeviceInfoListDetail( hDeviceInfoList, &devInfoListDetail ) ) {
                dwRet = GetLastError();
            }
            else{
                driver = get_device_driver_info( hDeviceInfoList, deviceInfo, devInfoListDetail );
            }
        }
        SetupDiDestroyDeviceInfoList( hDeviceInfoList );
    }
    SetLastError(dwRet); 
    return driver;
}

bool device_manager::update(boost::filesystem::path& Inf, stdstring& HWID){
	bool result = false;
	HMODULE newdevMod = NULL;
	DWORD Flags;

#define INSTALLFLAG_FORCE               0x00000001  // Force the installation of the specified driver

	typedef BOOL(WINAPI *UpdateDriverForPlugAndPlayDevicesProto) (
		HWND   hwndParent,
		LPCTSTR HardwareId,
		LPCTSTR FullInfPath,
		DWORD  InstallFlags,
		PBOOL  bRebootRequired
		);

	UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
	BOOL reboot = FALSE;
	LPCTSTR inf = NULL;
	DWORD flags = 0;
	DWORD res;
	TCHAR InfPath[MAX_PATH];

	if (!_machine.empty() || HWID.empty()) {
		//
		// must be local machine
		//
		return false;
	}
	//
	// Inf must be a full pathname
	//
	res = GetFullPathName(Inf.wstring().c_str(), MAX_PATH, InfPath, NULL);
	if ((res >= MAX_PATH) || (res == 0)) {
		//
		// inf pathname too long
		//
		return false;
	}
	
	if (GetFileAttributes(InfPath) == (DWORD)(-1)) {
		//
		// inf doesn't exist
		//
		return false;
	}

	inf = InfPath;
	flags |= INSTALLFLAG_FORCE;

	//
	// make use of UpdateDriverForPlugAndPlayDevices
	//
	
	newdevMod = LoadLibrary(TEXT("newdev.dll"));
	if (!newdevMod) {
		goto final;
	}
	
	UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod, "UpdateDriverForPlugAndPlayDevicesW");
	if (!UpdateFn)
	{
		goto final;
	}

	if (!UpdateFn(NULL, HWID.c_str(), inf, flags, &reboot)) {
		goto final;
	}

	result = true;

final :

	if (newdevMod) {
		FreeLibrary(newdevMod);
	}
	return result;
}

bool device_manager::install(boost::filesystem::path Inf, stdstring HWID){
	bool result = false;
	HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA DeviceInfoData;
	GUID ClassGUID;
	TCHAR ClassName[MAX_CLASS_NAME_LEN];
	TCHAR hwIdList[LINE_LEN + 4];
	TCHAR InfPath[MAX_PATH];

	if (!_machine.empty() || Inf.empty() || HWID.empty()) {
		//
		// must be local machine
		//
		return false;
	}
	//
	// Inf must be a full pathname
	//
	if (GetFullPathName(Inf.wstring().c_str(), MAX_PATH, InfPath, NULL) >= MAX_PATH) {
		//
		// inf pathname too long
		//
		return false;
	}

	//
	// List of hardware ID's must be double zero-terminated
	//
	ZeroMemory(hwIdList, sizeof(hwIdList));
	if (FAILED(_tcscpy_s(hwIdList, LINE_LEN, HWID.c_str()))) {
		goto final;
	}

	//
	// Use the INF File to extract the Class GUID.
	//
	if (!SetupDiGetINFClass(InfPath, &ClassGUID, ClassName, sizeof(ClassName) / sizeof(ClassName[0]), 0))
	{
		goto final;
	}

	//
	// Create the container for the to-be-created Device Information Element.
	//
	DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, 0);
	if (DeviceInfoSet == INVALID_HANDLE_VALUE)
	{
		goto final;
	}

	//
	// Now create the element.
	// Use the Class GUID and Name from the INF file.
	//
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
		ClassName,
		&ClassGUID,
		NULL,
		0,
		DICD_GENERATE_ID,
		&DeviceInfoData))
	{
		goto final;
	}

	//
	// Add the HardwareID to the Device's HardwareID property.
	//
	if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
		&DeviceInfoData,
		SPDRP_HARDWAREID,
		(LPBYTE)hwIdList,
		((DWORD)_tcslen(hwIdList) + 1 + 1)*sizeof(TCHAR)))
	{
		goto final;
	}

	//
	// Transform the registry element into an actual devnode
	// in the PnP HW tree.
	//
	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
		DeviceInfoSet,
		&DeviceInfoData))
	{
		goto final;
	}

	//
	// update the driver for the device we just created
	//
	result = update(Inf, HWID);

	final:

	if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}

	return result;
}

#endif

};// namespace windows

};// namespace macho

#endif