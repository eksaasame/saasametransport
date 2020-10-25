// irm_conv_agent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\gen-cpp\saasame_constants.h"
#include "..\irm_converter\boot_ini.h"
#include "..\irm_converter\bcd_edit.h"
#include "..\irm_converter\irm_disk.h"
#include "..\gen-cpp\temp_drive_letter.h"
#include "..\notice\http_client.h"
#include <shellapi.h>
#include <shlobj.h>
#include "difxapi.h"
#include "json_storage.hpp"
#include "VersionHelpers.h"
#include <locale>
#include <sstream>
#include <fstream>
#include <codecvt>

#pragma comment( lib, "difxapi.lib" )
#pragma comment( lib, "shell32.lib" )

using namespace boost;
using namespace macho::windows;
#define BUFSIZE 512

inline application::global_context_ptr this_application() {
    return application::global_context::get();
}

#define REBOOT_TIME						10

std::wstring read_file(boost::filesystem::path p)
{
    std::wifstream wif(p.wstring());
    wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    std::wstringstream wss;
    wss << wif.rdbuf();
    return wss.str();
}

void remove_excluded_paths(storage::volume::ptr& v)
{
    boost::filesystem::path excluded = boost::filesystem::path(v->path()) / L".excluded";
    if (boost::filesystem::exists(excluded)){
        std::vector<std::wstring> paths = stringutils::tokenize2(read_file(excluded), L"\r\n", 0, false);
        foreach(std::wstring path, paths){
            boost::filesystem::path p = boost::filesystem::path(v->path()) / path.substr(1);
            if (boost::filesystem::exists(p)){
                if (boost::filesystem::is_directory(p)){
                    LOG(LOG_LEVEL_WARNING, _T("Remove subfolders and files from excluded folder (%s)."), p.wstring().c_str());
                    std::vector<boost::filesystem::path> sub_folders = environment::get_sub_folders(p, true);
                    std::vector<boost::filesystem::path> files = environment::get_files(p, L"*", true);
                    foreach(boost::filesystem::path file, files){
                        SetFileAttributesW(file.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                        try{
                            boost::filesystem::remove(file);
                        }
                        catch (...){
                            MoveFileExW(file.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                        }
                    }
                    std::sort(sub_folders.begin(), sub_folders.end(), [](boost::filesystem::path const& lhs, boost::filesystem::path const& rhs) { return lhs.wstring().length() > rhs.wstring().length(); });
                    foreach(boost::filesystem::path folder, sub_folders){
                        SetFileAttributesW(folder.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                        try{
                            boost::filesystem::remove_all(folder);
                        }
                        catch (...){
                            MoveFileExW(folder.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                        }
                    }
                }
                else{
                    LOG(LOG_LEVEL_WARNING, _T("Remove excluded file (%s)."), p.wstring().c_str());
                    SetFileAttributesW(p.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                    try{
                        boost::filesystem::remove(p);
                    }
                    catch (...){
                        MoveFileExW(p.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    }
                }
            }
        }
        SetFileAttributesW(excluded.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
        try{
            boost::filesystem::remove(excluded);
        }
        catch (...){
            MoveFileExW(excluded.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }
    }
}

std::wstring get_error_message(DWORD message_id){
    std::wstring str;
    switch (message_id){
    case ERROR_TIMEOUT:
        str = L"This operation returned because the timeout period expired.";
        break;
    case CERT_E_EXPIRED:
        str = L"The signing certificate is expired.";
        break;
    case CERT_E_UNTRUSTEDROOT:
        str = L"The catalog file has an Authenticode signature whose certificate chain terminates in a root certificate that is not trusted.";
        break;
    case CERT_E_WRONG_USAGE:
        str = L"The certificate for the driver package is not valid for the requested usage. If the driver package does not have a valid WHQL signature, DriverPackageInstall returns this error if, in response to a driver signing dialog box, the user chose not to install a driver package, or if the caller specified the DRIVER_PACKAGE_SILENT flag.";
        break;
    case CRYPT_E_FILE_ERROR:
        str = L"The catalog file for the specified driver package was not found; or possibly, some other error occurred when DriverPackageInstall tried to verify the driver package signature.";
        break;
    case ERROR_ACCESS_DENIED:
        str = L"A caller of DriverPackageInstall must be a member of the Administrators group to install a driver package.";
        break;
    case ERROR_BAD_ENVIRONMENT:
        str = L"The current Microsoft Windows version does not support this operation. An old or incompatible version of DIFxApp.dll or DIFxAppA.dll might be present in the system. For more information about these .dll files, see How DIFxApp Works.";
        break;
    case ERROR_CANT_ACCESS_FILE:
        str = L"DriverPackageInstall could not preinstall the driver package because the specified INF file is in the system INF file directory.";
        break;
    case ERROR_FILE_NOT_FOUND:
        str = L"The INF file that was specified by DriverPackageInfPath was not found.";
        break;
    case ERROR_FILENAME_EXCED_RANGE:
        str = L"The INF file path, in characters, that was specified by DriverPackageInfPath is greater than the maximum supported path length. For more information about path length, see Specifying a Driver Package INF File Path.";
        break;
#ifdef ERROR_IN_WOW64
    case ERROR_IN_WOW64:
        str = L"The 32-bit version DIFxAPI does not work on Win64 systems. A 64-bit version of DIFxAPI is required.";
        break;
#endif
    case ERROR_INSTALL_FAILURE:
        str = L"The installation failed.";
        break;
    case ERROR_INVALID_CATALOG_DATA:
        str = L"The catalog file for the specified driver package is not valid or was not found.";
        break;
    case ERROR_INVALID_NAME:
        str = L"The specified INF file path is not valid.";
        break;
    case ERROR_INVALID_PARAMETER:
        str = L"A supplied parameter is not valid.";
        break;
    case ERROR_NO_DEVICE_ID:
        str = L"The driver package does not specify a hardware identifier or compatible identifier that is supported by the current platform.";
        break;
    case ERROR_NO_MORE_ITEMS:
        str = L"The specified driver package was not installed for matching devices because the driver packages already installed for the matching devices are a better match for the devices than the specified driver package.";
        break;
    case ERROR_NO_SUCH_DEVINST:
        str = L"The driver package was not installed on any device because there are no matching devices in the device tree.";
        break;
    case ERROR_OUTOFMEMORY:
        str = L"Available system memory was insufficient to perform the operation.";
        break;
    case ERROR_SHARING_VIOLATION:
        str = L"A component of the driver package in the DIFx driver store is locked by a thread or process. This error can occur if a process or thread, other than the thread or process of the caller, is currently accessing the same driver package as the caller.";
        break;
    case ERROR_NO_CATALOG_FOR_OEM_INF:
        str = L"The catalog file for the specified driver package was not found.";
        break;
    case ERROR_AUTHENTICODE_PUBLISHER_NOT_TRUSTED:
        str = L"The publisher of an Authenticode(tm) signed catalog was not established as trusted.";
        break;
    case ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH:
        str = L"The signing certificate is not valid for the current Windows version or it is expired.";
        break;
    case ERROR_UNSUPPORTED_TYPE:
        str = L"The driver package type is not supported.";
        break;
    case TRUST_E_NOSIGNATURE:
        str = L"The driver package is not signed.";
        break;
    case ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED:
        str = L"The publisher of an Authenticode(tm) signed catalog has not yet been established as trusted.";
        break;
    }
    return str;
}

bool driver_package_install( stdstring driver_package_inf_path, bool is_force ){
    LPTSTR _driver_package_inf_path = (LPTSTR)driver_package_inf_path.c_str();  // An INF file for PnP driver package
    DWORD Flags = DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT | DRIVER_PACKAGE_LEGACY_MODE | DRIVER_PACKAGE_SILENT;
    if (is_force)
        Flags |= DRIVER_PACKAGE_FORCE;

    INSTALLERINFO *pAppInfo = NULL;      // No application association
    BOOL NeedReboot = FALSE;
    DWORD ReturnCode = ERROR_INVALID_PARAMETER;
    bool retry = false;
    setup_inf_file inf;
    if (inf.load(driver_package_inf_path)){
       do{
            if (inf.is_hard_disk_controllers() || inf.is_scsi_raid_controllers()){
                ReturnCode = DriverPackageInstall(_driver_package_inf_path, Flags, pAppInfo, &NeedReboot);
                if (ERROR_SUCCESS != ReturnCode){
                    std::wstring str = get_error_message(ReturnCode);
                    LOG(LOG_LEVEL_WARNING, _T("Install Driver (%s) - Result (0x%08X - %s)\n"), _driver_package_inf_path, ReturnCode, str.c_str());
                }
            }
            if (ERROR_SUCCESS != ReturnCode){
                ReturnCode = DriverPackagePreinstall(_driver_package_inf_path, DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_LEGACY_MODE);
                if (ERROR_SUCCESS != ReturnCode){
                    if (ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED == ReturnCode){
                        certificate_store::ptr trusted_publisher = certificate_store::open_store(CERT_SYSTEM_STORE_LOCAL_MACHINE, L"TrustedPublisher");
                        if (trusted_publisher){
                            boost::filesystem::path inf_file = driver_package_inf_path;
                            boost::filesystem::path catalog_file = inf_file.parent_path() / inf.catalog_file();
                            authenticode_signed_info::ptr signed_info = certificate_store::get_authenticode_signed_info(catalog_file);
                            if (signed_info && signed_info->signer_certificate){
                                std::wstring cert_name = signed_info->signer_certificate->friendly_name();
                                if (retry = trusted_publisher->add_certificate(*signed_info->signer_certificate)){
                                    LOG(LOG_LEVEL_RECORD, _T("Add certificate(%s) into Trusted Publisher."), cert_name.c_str());
                                    continue;
                                }
                            }
                        }
                    }
                    std::wstring str = get_error_message(ReturnCode);
                    LOG(LOG_LEVEL_ERROR, _T("PreInstall Driver (%s) - Result (0x%08X - %s)\n"), _driver_package_inf_path, ReturnCode, str.c_str());
                }
                else{
                    LOG(LOG_LEVEL_INFO, _T("PreInstall Driver (%s) succeeded. \n"), _driver_package_inf_path);
                    return true;
                }
            }
            else{
                LOG(LOG_LEVEL_INFO, _T("Install Driver (%s) succeeded. \n"), _driver_package_inf_path);
                return true;
            }
       } while (retry);
    }
    return false;
}

BOOL shell_execute_ex(const stdstring cmd, const stdstring params){
    BOOL bReturn = FALSE;
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = cmd.c_str();
    ShExecInfo.lpParameters = params.c_str();
    ShExecInfo.lpDirectory = macho::windows::environment::get_working_directory().c_str();
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;

    LOG(LOG_LEVEL_RECORD, TEXT("Run batch file (%s)"), cmd.c_str());
    if (bReturn = ShellExecuteEx(&ShExecInfo))
        WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    else
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to run batch file (%s) - error: 0x%08X."), cmd.c_str(), GetLastError());
    
    return bReturn;
}

class reg_mounted_device{
private:
#define DOSDEVICES                  _T("\\DosDevices\\")
#define VOLDEVICES                  _T("\\??\\Volume")
    std::wstring    _name;
    std::wstring    _drive_letter;
    std::wstring    _path;
    macho::bytes    _bytes;
public:
    typedef struct gpt{
        CHAR            signature[8];
        uuid_t          uuid;
    };
    typedef struct mbr{
        DWORD           signature;
        ULONGLONG       starte_offset;
    };

    typedef boost::shared_ptr<reg_mounted_device> ptr;
    typedef std::vector<ptr>                      vtr;
    typedef std::map<std::wstring, ptr>           map;
    reg_mounted_device(std::wstring &name, macho::bytes &bytes) : _name(name), _bytes(bytes){}
    bool is_gpt()   { return _bytes.length() == 24; }
    bool is_mbr()   { return _bytes.length() == 12; }
    const reg_mounted_device::gpt* to_gpt() { return (reg_mounted_device::gpt*)_bytes.ptr(); }
    const reg_mounted_device::mbr* to_mbr() { return (reg_mounted_device::mbr*)_bytes.ptr(); }
    LPCWSTR                        to_wstring() { (LPCWSTR)_bytes.ptr(); }
    std::wstring name(){ return _name; }
    std::wstring path(){ return is_gpt() ? boost::str(boost::wformat(L"\\\\?\\Volume{%s}\\") % (std::wstring)macho::guid_((GUID)to_gpt()->uuid) ) : _path; }
    std::wstring drive_letter() { 
        if (_name.find(DOSDEVICES) != std::wstring::npos)
            return _name.substr(_tcslen(DOSDEVICES)).append(_T("\\"));
        else if (_name.find(VOLDEVICES) != std::wstring::npos)
            return _drive_letter;
        return _T("");
    }
    virtual ~reg_mounted_device(){}

    static reg_mounted_device::vtr get(std::wstring key_path = L"SYSTEM\\MountedDevices_"){
        reg_mounted_device::vtr devices;
        reg_mounted_device::vtr vol_devices;
        registry reg(REGISTRY_READONLY);
        if (reg.open(key_path)){
            for (size_t i = 0; i < reg.count(); i++){
                if (reg[i].is_binary() && reg[i].name().find(DOSDEVICES) != std::wstring::npos ) {
                    reg_mounted_device::ptr d = reg_mounted_device::ptr(new reg_mounted_device(reg[i].name(), (bytes)reg[i]));
                    if (d->is_gpt()||d->is_mbr())
                        devices.push_back(d);      
                }
            }

            for (size_t i = 0; i < reg.count(); i++){
                if (reg[i].is_binary() && reg[i].name().find(VOLDEVICES) != std::wstring::npos) {
                    reg_mounted_device::ptr d = reg_mounted_device::ptr(new reg_mounted_device(reg[i].name(), (bytes)reg[i]));
                    if (d->is_gpt() || d->is_mbr()){
                        vol_devices.push_back(d);
                        foreach(reg_mounted_device::ptr dd, devices){
                            if (dd->_bytes == d->_bytes){
                                d->_drive_letter = dd->drive_letter();
                                d->_path = dd->_path = boost::str(boost::wformat(L"\\\\?\\Volume%s\\") % d->name().substr(_tcslen(VOLDEVICES)));
                                LOG(LOG_LEVEL_INFO, _T("Found backup reg mounted device: %s - %s"), d->_path.c_str(), d->_drive_letter.c_str());
                            }
                        }
                    }
                }
            }
        }
        return devices;
    }
};

struct disk_extent{
    typedef std::vector<disk_extent> vtr;
    disk_extent() : disk_number(-1), starting_offset(0), extent_length(0) {}
    disk_extent(DWORD number, ULONGLONG offset, ULONGLONG length)
        : disk_number(number), starting_offset(offset), extent_length(length) {}
    disk_extent(const disk_extent& extent){
        copy(extent);
    }
    void copy(const disk_extent& extent){
        disk_number = extent.disk_number;
        starting_offset = extent.starting_offset;
        extent_length = extent.extent_length;
    }
    const disk_extent &operator =(const disk_extent& extent){
        if (this != &extent)
            copy(extent);
        return(*this);
    }
    DWORD      disk_number;
    ULONGLONG  starting_offset;
    ULONGLONG  extent_length;
};

disk_extent::vtr get_volume_disk_extents(std::wstring volume_path){
    BOOL result = TRUE;
    disk_extent::vtr disk_extents;
    std::auto_ptr<VOLUME_DISK_EXTENTS> pDiskExtents((VOLUME_DISK_EXTENTS *) new BYTE[sizeof(VOLUME_DISK_EXTENTS)]);
    memset(pDiskExtents.get(), 0, sizeof(VOLUME_DISK_EXTENTS));
    if ((5 > volume_path.length()) ||
        (volume_path[0] != _T('\\') ||
        volume_path[1] != _T('\\') ||
        volume_path[2] != _T('?') ||
        volume_path[3] != _T('\\'))) {
        result = FALSE;
    }
    else{
        if (volume_path[volume_path.length() - 1] == _T('\\'))
            volume_path.erase(volume_path.length() - 1);
        auto_file_handle device_handle = CreateFileW(volume_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (device_handle.is_invalid()){
            LOG(LOG_LEVEL_ERROR, _T("Can't access volume (%s). error( 0x%08X )"), volume_path.c_str(), GetLastError());
            result = FALSE;
        }
        else{
            DWORD bufferSize;
            if (!(result = DeviceIoControl(device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), sizeof(VOLUME_DISK_EXTENTS), &bufferSize, 0))){
                DWORD error = GetLastError();
                if (((error == ERROR_INSUFFICIENT_BUFFER) || (error == ERROR_MORE_DATA)) && (pDiskExtents->NumberOfDiskExtents > 1)){
                    bufferSize = sizeof(VOLUME_DISK_EXTENTS) + (pDiskExtents->NumberOfDiskExtents*sizeof(DISK_EXTENT));
                    pDiskExtents = std::auto_ptr<VOLUME_DISK_EXTENTS>((VOLUME_DISK_EXTENTS *) new BYTE[bufferSize]);
                    if (!(result = DeviceIoControl(device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), bufferSize, &bufferSize, 0))){
                        LOG(LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), volume_path.c_str(), GetLastError());
                    }
                }
                else{
                    LOG(LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), volume_path.c_str(), error);
                }
            }
        }
    }
    if (TRUE == result){
        for (DWORD i = 0; i < pDiskExtents.get()->NumberOfDiskExtents; i++){
            disk_extents.push_back(disk_extent(pDiskExtents.get()->Extents[i].DiskNumber, pDiskExtents.get()->Extents[i].StartingOffset.QuadPart, pDiskExtents.get()->Extents[i].ExtentLength.QuadPart));
        }
    }
   return disk_extents;
}

class main_app
{
private:

    std::string  read_from_file(boost::filesystem::path file_path){
        std::fstream file(file_path.string(), std::ios::in | std::ios::binary);
        std::string result = std::string();
        if (file.is_open()){
            file.seekg(0, std::ios::end);
            result.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(&result[0], result.size());
            file.close();
        }
        return result;
    }

    struct volume_partiton{
        volume_partiton() : is_boot_disk(false), is_system_volume(false), type(macho::windows::storage::ST_PST_UNKNOWN){}
        typedef boost::shared_ptr<volume_partiton>    ptr;
        typedef std::vector<ptr>                      vtr;
        typedef std::map<std::wstring, ptr>           map;
        std::wstring                                  volume_path;
        macho::windows::storage::volume::ptr          volume;
        macho::windows::storage::partition::vtr       partitions;
        macho::windows::storage::ST_PARTITION_STYLE   type;
        bool                                          is_boot_disk;
        bool                                          is_system_volume;
    };

    struct mounted_volume{
        typedef boost::shared_ptr<mounted_volume>       ptr;
        typedef std::vector<ptr>                        vtr;
        typedef std::map<std::wstring, ptr>             map;
        mounted_volume() : offset(0), partition_number(0), partition_id(GUID_NULL), signature(0), is_boot_disk(false), type(macho::windows::storage::ST_PST_UNKNOWN){}
        DWORD                                           signature;
        macho::windows::storage::ST_PARTITION_STYLE     type;
        uint64_t                                        offset;
        uuid_t                                          partition_id;
        int                                             partition_number;
        bool                                            is_boot_disk;
        std::wstring                                    volume_path;
        std::vector<std::wstring>                       access_paths;
    };

    volume_partiton::map get_volume_partition_map(bool bring_disk_online = false){
        volume_partiton::map results;
        macho::windows::storage::ptr stg = IsWindowsVistaOrGreater() ? macho::windows::storage::local() : macho::windows::storage::get();
        macho::windows::storage::disk::vtr disks = stg->get_disks();
        if (bring_disk_online){
            foreach(macho::windows::storage::disk::ptr &d, disks){
                int trycount = 5;
                if (d->is_offline()){
                    while (trycount--){
                        if (d->online()){
                            LOG(LOG_LEVEL_RECORD, _T("Successed to bring the disk(%d) online."), d->number());
                            boost::this_thread::sleep(boost::posix_time::seconds(10));
                            break;
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Failed to bring the disk(%d) online."), d->number());
                            boost::this_thread::sleep(boost::posix_time::seconds(10));
                        }
                    }
                }
                d->clear_read_only_flag();
            }

            stg = IsWindowsVistaOrGreater() ? macho::windows::storage::local() : macho::windows::storage::get();
            disks = stg->get_disks();

            foreach(macho::windows::storage::disk::ptr &d, disks){
                if (!d->is_offline()){
                    macho::windows::storage::partition::vtr _partitions;
                    int trycount = 5;
                    while (trycount--){
                        try{
                            _partitions = stg->get_partitions(d->number());
                            break;
                        }
                        catch (storage::exception & ex){
                            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                            boost::this_thread::sleep(boost::posix_time::seconds(5));
                        }
                    }
                    LOG(LOG_LEVEL_RECORD, _T("Found %d partitions on disk(%d)."), _partitions.size(), d->number());
                    foreach(macho::windows::storage::partition::ptr &p, _partitions){
                        if (p->is_hidden()){
                            if (0 == p->set_attributes(false,
                                false,
                                p->is_active(),
                                false)){
                                LOG(LOG_LEVEL_RECORD, _T("Successed to set partiton(%d) attributes."), p->partition_number());
                            }
                            else {
                                LOG(LOG_LEVEL_ERROR, _T("Failed to set partiton(%d) attributes."), p->partition_number());
                            }
                        }
                        else{
                            LOG(LOG_LEVEL_RECORD, _T("The partition is not hidden. Skip to set partiton(%d) attributes."), p->partition_number());
                        }
                    }
                    boost::this_thread::sleep(boost::posix_time::seconds(5));
                }
            }
        }

        macho::windows::storage::volume::vtr volumes = stg->get_volumes();
        LOG(LOG_LEVEL_INFO, _T("Found %d volumes."), volumes.size());
        macho::windows::storage::partition::vtr partitions = stg->get_partitions();
        LOG(LOG_LEVEL_INFO, _T("Found %d partitions."), partitions.size());
        
        foreach(macho::windows::storage::volume::ptr &v, volumes){
            remove_excluded_paths(v);
            volume_partiton::ptr s = volume_partiton::ptr(new volume_partiton());
            s->volume = v;
            s->volume_path = v->path();         
            disk_extent::vtr disk_extents = get_volume_disk_extents(s->volume_path);
            foreach(macho::windows::storage::partition::ptr &p, partitions){
                bool found = false;
                foreach(disk_extent& ext, disk_extents){
                    if (ext.disk_number == p->disk_number() && ext.starting_offset == p->offset()){
                        macho::windows::storage::disk::ptr d = stg->get_disk(p->disk_number());
                        if (!s->is_boot_disk)
                            s->is_boot_disk = d->is_boot();
                        s->type = d->partition_style();
                        s->is_system_volume = p->is_boot();
                        s->partitions.push_back(p);
                        found = true;
                        LOG(LOG_LEVEL_INFO, _T("Volume %s -> (%d:%d)."), s->volume_path.c_str(), p->disk_number(), p->partition_number());
                        break;
                    }
                }
                if (found)
                    break;
            }
            results[s->volume_path] = s;
        }
        return results;
    }

    mounted_volume::vtr get_backup_mounted_volumes_info(){
        mounted_volume::vtr     mounted_devices;
        mounted_volume::map     mounted_devices_map;
        if (boost::filesystem::exists(boost::filesystem::path(macho::windows::environment::get_windows_directory()) / L"storage.json")){
            macho::windows::storage::ptr stg = macho::windows::json_storage::get(boost::filesystem::path(macho::windows::environment::get_windows_directory()) / L"storage.json");
            macho::windows::storage::disk::vtr disks = stg->get_disks();
            macho::windows::storage::volume::vtr vols = stg->get_volumes();
            foreach(macho::windows::storage::volume::ptr &v, vols){
                mounted_volume::ptr m = mounted_volume::ptr(new mounted_volume());
                m->access_paths = v->access_paths();
                m->volume_path = v->path();
                mounted_devices_map[m->volume_path] = m;
                mounted_devices.push_back(m);
            }

            foreach(macho::windows::storage::disk::ptr &d, disks){
                macho::windows::storage::partition::vtr partitions = d->get_partitions();
                macho::windows::storage::volume::vtr volumes = d->get_volumes();
                foreach(macho::windows::storage::volume::ptr &v, volumes){
                    if (mounted_devices_map.count(v->path())){
                        mounted_volume::ptr m = mounted_devices_map[v->path()];
                        m->is_boot_disk = d->is_boot();
                        foreach(macho::windows::storage::partition::ptr &partition, partitions){
                            bool found = false;
                            foreach(std::wstring path, partition->access_paths()){
                                if (std::wstring::npos != m->volume_path.find(path)){
                                    m->offset = partition->offset();
                                    m->partition_id = partition->guid().length() ? macho::guid_(partition->guid()) : GUID_NULL;
                                    m->partition_number = partition->partition_number();
                                    m->signature = d->signature();
                                    m->type = d->partition_style();
                                    found = true;
                                    break;
                                }
                            }
                            if (found) break;
                        }
                    }
                }
            }
        }
        else{
            // Remap volume driver letter and mount points
            macho::windows::storage::ptr stg = IsWindowsVistaOrGreater() ? macho::windows::storage::local() : macho::windows::storage::get();
            reg_mounted_device::vtr backup_devices = reg_mounted_device::get();

            foreach(reg_mounted_device::ptr &device, backup_devices){
                if (device->is_gpt()){
                    mounted_volume::ptr v = mounted_volume::ptr(new mounted_volume());
                    v->type = macho::windows::storage::ST_PST_GPT;
                    v->partition_id = device->to_gpt()->uuid;
                    v->access_paths.push_back(device->drive_letter());
                    v->volume_path = device->path();
                    LOG(LOG_LEVEL_INFO, _T("Backup mounted device: %s - %s"), v->volume_path.c_str(), device->drive_letter().c_str());
                    mounted_devices.push_back(v);
                }
                else if (device->is_mbr()){
                    mounted_volume::ptr v = mounted_volume::ptr(new mounted_volume());
                    v->type = macho::windows::storage::ST_PST_MBR;
                    v->signature = device->to_mbr()->signature;
                    v->offset = device->to_mbr()->starte_offset;
                    v->access_paths.push_back(device->drive_letter());
                    v->volume_path = device->path();
                    LOG(LOG_LEVEL_INFO, _T("Backup mounted device: %s - %s"), v->volume_path.c_str(), device->drive_letter().c_str());
                    mounted_devices.push_back(v);
                }
            }
            bool found = false;
            LOG(LOG_LEVEL_INFO, _T("Look for gpt.backup file..."));
            macho::windows::storage::disk::vtr disks = stg->get_disks();
            foreach(macho::windows::storage::disk::ptr &disk, disks){
                if (found)
                    break;
                LOG(LOG_LEVEL_INFO, _T("Disk %d : boot : %s, style : %d, %d partitions"), disk->number(), disk->is_boot() ? L"true" : L"false", disk->partition_style(), disk->number_of_partitions());
                if (disk->is_boot() &&
                    disk->partition_style() == macho::windows::storage::ST_PST_MBR){
                    macho::windows::storage::partition::vtr partitions = disk->get_partitions();
                    LOG(LOG_LEVEL_INFO, _T("Found %d of partitions form disk %d"), partitions.size(), disk->number());
                    foreach(macho::windows::storage::partition::ptr &partition, partitions){
                        if (partition->is_active() && partition->access_paths().size()){
                            LOG(LOG_LEVEL_INFO, _T("Try to look for the gpt.backup file from %d:%d"), partition->disk_number(), partition->partition_number());
                            found = true;
                            boost::filesystem::path p = partition->access_paths()[0];
                            if (boost::filesystem::exists(p / L"gpt.backup")){
                                LOG(LOG_LEVEL_INFO, _T("Found gpt.backup file on %d:%d"), partition->disk_number(), partition->partition_number());
                                std::string gpt = read_from_file(p / L"gpt.backup");
                                if (gpt.length() >= BUFSIZE){
                                    PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&gpt[BUFSIZE];
                                    PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&gpt[BUFSIZE * pGptPartitonHeader->PartitionEntryLBA];
                                    int partition_number = 0;
                                    LOG(LOG_LEVEL_INFO, _T("Number of partition entries: %d"), pGptPartitonHeader->NumberOfPartitionEntries);
                                    for (int i = 0; i < pGptPartitonHeader->NumberOfPartitionEntries; i++){
                                        bool is_match = false;
                                        if (GUID_NULL == pGptPartitionEntries[i].UniquePartitionGuid)
                                            continue;
                                        if (pGptPartitionEntries[i].StartingLBA == 0ULL && pGptPartitionEntries[i].EndingLBA == 0ULL)
                                            continue;
                                        partition_number++;
                                        foreach(mounted_volume::ptr &v, mounted_devices){
                                            if (v->partition_id == pGptPartitionEntries[i].UniquePartitionGuid){
                                                is_match = true;
                                                v->partition_number = partition_number;
                                                v->type = macho::windows::storage::ST_PST_GPT;
                                                v->partition_id = pGptPartitionEntries[i].UniquePartitionGuid;
                                                v->offset = pGptPartitionEntries[i].StartingLBA * disk->logical_sector_size();
                                                v->is_boot_disk = true;
                                                LOG(LOG_LEVEL_INFO, _T("Match backup mounted device: %s - %d"), v->volume_path.c_str(), v->partition_number);
                                                break;
                                            }
                                        }
                                        if (!is_match){
                                            mounted_volume::ptr v = mounted_volume::ptr(new mounted_volume());
                                            v->partition_number = partition_number;
                                            v->type = macho::windows::storage::ST_PST_GPT;
                                            v->partition_id = pGptPartitionEntries[i].UniquePartitionGuid;
                                            v->offset = pGptPartitionEntries[i].StartingLBA * disk->logical_sector_size();
                                            v->is_boot_disk = true;
                                            LOG(LOG_LEVEL_INFO, _T("New backup mounted device: %s - %d"), v->volume_path.c_str(), v->partition_number);
                                            mounted_devices.push_back(v);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return mounted_devices;
    }

    void remap_volumes(){
#define VOLUME_PATH                    _T("\\\\?\\Volume")
        volume_partiton::map volumes_map = get_volume_partition_map(true);
        LOG(LOG_LEVEL_RECORD, _T("Found %d volumes. (1)"), volumes_map.size());
        mounted_volume::vtr backup_mounted_volumes = get_backup_mounted_volumes_info();
        LOG(LOG_LEVEL_RECORD, _T("Found %d backup volumes."), backup_mounted_volumes.size());
        std::vector<std::wstring> cdroms;
        std::vector<std::wstring> dismountedvolumes;
        foreach(volume_partiton::map::value_type &v, volumes_map){
            if (v.second->volume && v.second->volume->drive_type() == macho::windows::storage::ST_CDROM){
                // remove cd/dvd-rom drive letter
                if (v.second->volume->drive_letter().length()){
                    bool found = false;
                    foreach(mounted_volume::ptr &p, backup_mounted_volumes){
                        foreach(std::wstring a, p->access_paths){
                            if (temp_drive_letter::is_drive_letter(a) && _wcsnicmp(a.c_str(), v.second->volume->drive_letter().c_str(), 1) == 0){
                                found = true;
                                if (DeleteVolumeMountPoint(a.c_str())){
                                    cdroms.push_back(v.second->volume_path);
                                    LOG(LOG_LEVEL_RECORD, _T("Succeeded to delete volume mount point (%s) for CD-ROM."), a.c_str());
                                }
                                else{
                                    LOG(LOG_LEVEL_ERROR, _T("Failed to delete volume mount point (%s) for CD-ROM. Error : %d"), a.c_str(), GetLastError());
                                }
                                break;
                            }
                        }
                        if (found) break;
                    }
                }
            }
            else if (v.second->volume && !v.second->volume->drive_letter().empty() &&
                v.second->volume->drive_type() == macho::windows::storage::ST_REMOVABLE &&
                (_wcsnicmp(L"a", v.second->volume->drive_letter().c_str(), 1) == 0 || _wcsnicmp(L"b", v.second->volume->drive_letter().c_str(), 1) == 0)){
                continue;
            }
            else{
                
                //std::vector<boost::filesystem::path> files = environment::get_files(boost::filesystem::path(v.second->volume_path) / L"System Volume Information", L"*{3808876b-c176-4e48-b7ae-04046e6cc752}");
                //foreach(boost::filesystem::path &file, files){
                //    MoveFileExW(file.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                //}

                bool processed = false;
                // remove unmap drive letter and mount point            
                foreach(mounted_volume::ptr& p, backup_mounted_volumes){
                    if (0 == _wcsicmp(v.second->volume_path.c_str(), p->volume_path.c_str()) && !v.second->is_system_volume){
                        processed = true;
                        foreach(std::wstring m, v.second->volume->access_paths()){
                            bool found = false;
                            foreach(std::wstring a, p->access_paths){
                                if (0 == _wcsicmp(m.c_str(), a.c_str())){
                                    found = true;
                                    break;
                                }
                            }
                            if (!found && m.find(VOLUME_PATH) == std::wstring::npos){
                                if (DeleteVolumeMountPoint(m.c_str())){
                                    if (std::find(dismountedvolumes.begin(), dismountedvolumes.end(), v.second->volume_path) == dismountedvolumes.end())
                                        dismountedvolumes.push_back(v.second->volume_path);
                                    LOG(LOG_LEVEL_RECORD, _T("Succeeded to delete volume mount point (%s) for volume (%s), partition(%d)."), m.c_str(), v.second->volume_path.c_str(), p->partition_number);
                                }
                                else{
                                    LOG(LOG_LEVEL_ERROR, _T("Failed to delete volume mount point (%s) for volume (%s), partition(%d). Error : %d"), m.c_str(), v.second->volume_path.c_str(), p->partition_number, GetLastError());
                                }
                            }
                        }
                    }                
                }

                if (!processed){
                    if (!v.second->is_system_volume){
                        foreach(std::wstring m, v.second->volume->access_paths()){
                            if (m.find(VOLUME_PATH) == std::wstring::npos){
                                if (DeleteVolumeMountPoint(m.c_str())){
                                    if (std::find(dismountedvolumes.begin(), dismountedvolumes.end(), v.second->volume_path) == dismountedvolumes.end())
                                        dismountedvolumes.push_back(v.second->volume_path);
                                    LOG(LOG_LEVEL_RECORD, _T("Succeeded to delete volume mount point (%s) for volume (%s)."), m.c_str(), v.second->volume_path.c_str());
                                }
                                else{
                                    LOG(LOG_LEVEL_ERROR, _T("Failed to delete volume mount point (%s) for volume (%s). Error : %d"), m.c_str(), v.second->volume_path.c_str(), GetLastError());
                                }
                            }
                        }
                    }              
                }
            }
        }

        volumes_map = get_volume_partition_map();
        // assign drive letter and mout point
        LOG(LOG_LEVEL_RECORD, _T("Found %d volumes (2)."), volumes_map.size());
        foreach(volume_partiton::map::value_type &v, volumes_map){
            foreach(mounted_volume::ptr& p, backup_mounted_volumes){
                if (0 == _wcsicmp(v.second->volume_path.c_str(), p->volume_path.c_str()) && !v.second->is_system_volume){
                    dismountedvolumes.erase(std::remove(dismountedvolumes.begin(), dismountedvolumes.end(), v.second->volume_path), dismountedvolumes.end());
                    foreach(std::wstring a, p->access_paths){
                        bool found = false;
                        foreach(std::wstring m, v.second->volume->access_paths()){
                            if (0 == _wcsicmp(m.c_str(), a.c_str())){
                                found = true;
                                break;
                            }
                        }
                        if (!found && a.find(VOLUME_PATH) == std::wstring::npos){
                            if (SetVolumeMountPoint(a.c_str(), v.second->volume_path.c_str())){
                                LOG(LOG_LEVEL_RECORD, _T("Succeeded to set volume mount point (%s) for volume (%s)."), a.c_str(), v.second->volume_path.c_str());
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR, _T("Failed to set volume mount point (%s) for volume (%s). Error : %d"), a.c_str(), v.second->volume_path.c_str(), GetLastError());
                            }
                        }
                    }
                }
                else if (v.second->is_boot_disk &&
                    p->is_boot_disk && 
                    ((v.second->type == macho::windows::storage::ST_PST_MBR && p->type == macho::windows::storage::ST_PST_GPT) || 
                    (v.second->type == macho::windows::storage::ST_PST_GPT && p->type == macho::windows::storage::ST_PST_MBR))) {
                    foreach(macho::windows::storage::partition::ptr &partition, v.second->partitions){
                        if (partition->offset() == p->offset){
                            dismountedvolumes.erase(std::remove(dismountedvolumes.begin(), dismountedvolumes.end(), v.second->volume_path), dismountedvolumes.end());
                            foreach(std::wstring a, p->access_paths){
                                bool found = false;
                                foreach(std::wstring m, v.second->volume->access_paths()){
                                    if (0 == _wcsicmp(m.c_str(), a.c_str())){
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found && a.find(VOLUME_PATH) == std::wstring::npos){
                                    if (SetVolumeMountPoint(a.c_str(), v.second->volume_path.c_str())){
                                        dismountedvolumes.erase(std::remove(dismountedvolumes.begin(), dismountedvolumes.end(), v.second->volume_path), dismountedvolumes.end());
                                        LOG(LOG_LEVEL_RECORD, _T("Succeeded to set volume mount point (%s) for volume (%s)."), a.c_str(), v.second->volume_path.c_str());
                                    }
                                    else{
                                        LOG(LOG_LEVEL_ERROR, _T("Failed to set volume mount point (%s) for volume (%s). Error : %d"), a.c_str(), v.second->volume_path.c_str(), GetLastError());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // assign cd/dvd drive letter
        foreach(volume_partiton::map::value_type &v, volumes_map){
            if (v.second->volume && v.second->volume->drive_type() == macho::windows::storage::ST_CDROM){
                // assign cd/dvd-rom drive letter
                if (v.second->volume->drive_letter().empty()){
                    std::wstring drive_letter = temp_drive_letter::find_next_available_drive();
                    if (SetVolumeMountPoint(drive_letter.c_str(), v.second->volume_path.c_str())){
                        cdroms.erase(std::remove(cdroms.begin(), cdroms.end(), v.second->volume_path), cdroms.end());
                        LOG(LOG_LEVEL_RECORD, _T("Succeeded to set volume mount point (%s) for CD-ROM (%s)."), drive_letter.c_str(), v.second->volume_path.c_str());
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, _T("Failed to set volume mount point (%s) for CD-ROM (%s). Error : %d"), drive_letter.c_str(), v.second->volume_path.c_str(), GetLastError());
                    }
                }
                else{
                    LOG(LOG_LEVEL_RECORD, _T("The drive letter of CD-ROM (%s) is not empty."), v.second->volume_path.c_str());
                }
            }
        }
        
        foreach(std::wstring volume, dismountedvolumes){
            std::wstring drive_letter = temp_drive_letter::find_next_available_drive();
            if (SetVolumeMountPoint(drive_letter.c_str(), volume.c_str())){
                LOG(LOG_LEVEL_RECORD, _T("Succeeded to set volume mount point (%s) for volume (%s)."), drive_letter.c_str(), volume.c_str());
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Failed to set volume mount point (%s) for volume (%s). Error : %d"), drive_letter.c_str(), volume.c_str(), GetLastError());
            }
        }

        foreach(std::wstring cdrom, cdroms){
            std::wstring drive_letter = temp_drive_letter::find_next_available_drive();
            if (SetVolumeMountPoint(drive_letter.c_str(), cdrom.c_str())){
                LOG(LOG_LEVEL_RECORD, _T("Succeeded to set volume mount point (%s) for CD-ROM (%s)."), drive_letter.c_str(), cdrom.c_str());
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Failed to set volume mount point (%s) for CD-ROM (%s). Error : %d"), drive_letter.c_str(), cdrom.c_str(), GetLastError());
            }
        }
    }

    bool is_froce_normal_boot(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("ForceNormalBoot")].exists() && ((DWORD)reg[_T("ForceNormalBoot")]) > 0)
                return true;
        }
        return false;
    }

    void clear_force_normal_boot_flag(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("ForceNormalBoot")].exists())
                reg[_T("ForceNormalBoot")].delete_value();
        }
    }

    void set_force_normal_boot_flag(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            reg[_T("ForceNormalBoot")] = 0x1;
        }
    }

    bool is_auto_reboot_disabled(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("DisableAutoReboot")].exists() && ((DWORD)reg[_T("DisableAutoReboot")]) > 0)
                return true;
        }
        return false;
    }

    void clear_disable_auto_reboot_flag(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("DisableAutoReboot")].exists())
                reg[_T("DisableAutoReboot")].delete_value();
        }
    }
    
    bool is_driver_package_force(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("DriverPackageForce")].exists() && ((DWORD)reg[_T("DriverPackageForce")]) > 0)
                return true;
        }
        return false;
    }

    void clear_driver_package_force_flag(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("DriverPackageForce")].exists())
                reg[_T("DriverPackageForce")].delete_value();
        }
    }
    
    void get_callback_info(std::vector<std::wstring> &callbacks, int &timeout){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("CallBacks")].exists() && reg[_T("CallBacks")].is_multi_sz()){
                timeout = 30;
                for (int i = 0; i < reg[_T("CallBacks")].get_multi_count(); i++){
                    std::wstring callback = reg[_T("CallBacks")].get_multi_at(i);
                    if (!callback.empty())
                        callbacks.push_back(callback);
                }
                if (reg[_T("CallBackTimeOut")].exists() && reg[_T("CallBackTimeOut")].is_dword()){
                    timeout = (DWORD)reg[_T("CallBackTimeOut")];
                }                    
            }
        }
    }
    
    void clear_callback_info(){
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[_T("CallBacks")].exists())
                reg[_T("CallBacks")].delete_value();
            if (reg[_T("CallBackTimeOut")].exists())
                reg[_T("CallBackTimeOut")].delete_value();
        }
    }

    class enable_service_in_safe_mode
    {
    public:
        enable_service_in_safe_mode(std::wstring name) {
            _key = boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\%s") % name);
            registry reg;
            if (!(_is_existing = reg.open(_key))){
                registry reg_create(macho::windows::REGISTRY_CREATE);
                if (reg_create.open(_key)){
                    reg_create[_T("")] = _T("Service");
                }
            }
            try{
                service sc = service::get_service(name);
                sc.start();
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Failed to start service (%s).", name.c_str());
            }
        }
        ~enable_service_in_safe_mode(){
            if (!_is_existing){
                registry reg;
                if (reg.open(_key)){
                    reg.delete_key();
                }
            }
        }
    private:
        bool         _is_existing;
        std::wstring _key;
    };

public:
    main_app() {}

    bool set_safe_mode(__in bool is_enable, macho::windows::operating_system& os){
        bool result = false;
        macho::windows::com_init com;
        if (os.is_winvista_or_later()){
            bcd_edit bcd;
            if (result = bcd.set_safe_mode(is_enable, os.is_domain_controller()))
                bcd.set_boot_status_policy_option(false);
        }
        else{
            int retry = 3;
            while (retry > 0){
                try{
                    macho::windows::storage::ptr stg = IsWindowsVistaOrGreater() ? macho::windows::storage::local() : macho::windows::storage::get();
                    macho::windows::storage::disk::vtr disks = stg->get_disks();
                    foreach(macho::windows::storage::disk::ptr &disk, disks){
                        bool found = false;
                        if (disk->partition_style() == macho::windows::storage::ST_PST_MBR){
                            macho::windows::storage::partition::vtr partitions = disk->get_partitions();
                            foreach(macho::windows::storage::partition::ptr &partition, partitions){
                                if (partition->is_active() && partition->access_paths().size()){
                                    boost::filesystem::path boot_ini = partition->access_paths()[0];
                                    boot_ini /= L"boot.ini";
                                    if (boost::filesystem::exists(boot_ini)){
                                        result = boot_ini_edit::set_safe_mode(boot_ini.wstring(), false, os.is_domain_controller());
                                        found = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (found)
                            break;
                    }
                    if (result)
                        break;
                }
                catch (...){
                    if (0 == retry){
                        LOG(LOG_LEVEL_RECORD, (TEXT("Got some problems to disable safe mode boot up. Try to recovery it by restart the system.")));
                        if (macho::windows::environment::shutdown_system(true,_T("irm_conv_agent - auto system reboot."))){
                            LOG(LOG_LEVEL_RECORD, (TEXT("System shutdown succeeded.")));
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, (TEXT("System shutdown failed (%d)."), GetLastError()));
                            LOG(LOG_LEVEL_RECORD, TEXT("shutdown -r -f -t 15"));
                            macho::windows::process::exec_console_application_without_wait(L"shutdown -r -f -t 15");
                        }
                    }
                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                    retry--;
                }
            }
        }    
        return result;
    }

    bool copy_directory(boost::filesystem::path const & source, boost::filesystem::path const & destination){
        namespace fs = boost::filesystem;
        try
        {
            // Check whether the function call is valid
            if (
                !fs::exists(source) ||
                !fs::is_directory(source)
                )
            {
                std::cerr << "Source directory " << source.string()
                    << " does not exist or is not a directory." << '\n'
                    ;
                LOG(LOG_LEVEL_ERROR, L"Source directory %s does not exist or is not a directory.", source.wstring().c_str());
                return false;
            }
            if (fs::exists(destination))
            {
                std::clog << "Destination directory " << destination.string()
                    << " already exists." << '\n'
                    ;
                LOG(LOG_LEVEL_INFO, L"Destination directory %s already exists.", destination.wstring().c_str());
            }
            // Create the destination directory
            else if (!fs::create_directories(destination))
            {
                std::cerr << "Unable to create destination directory"
                    << destination.string() << '\n'
                    ;
                LOG(LOG_LEVEL_ERROR, L"Unable to create destination directory %s", destination.wstring().c_str());
                return false;
            }
        }
        catch (fs::filesystem_error const & e)
        {
            std::cerr << e.what() << '\n';
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(e.what()));
            return false;
        }
        // Iterate through the source directory
        for (
            fs::directory_iterator file(source);
            file != fs::directory_iterator(); ++file
            )
        {
            try
            {
                fs::path current(file->path());
                if (fs::is_directory(current))
                {
                    // Found directory: Recursion
                    if (
                        !copy_directory(
                        current,
                        destination / current.filename()
                        )
                        )
                    {
                        return false;
                    }
                }
                else
                {
                    // Found file: Copy
                    fs::copy_file(
                        current,
                        destination / current.filename()
                        );
                }
            }
            catch (fs::filesystem_error const & e)
            {
                std::cerr << e.what() << '\n';
                LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(e.what()));
                return false;
            }
        }
        return true;
    }

    bool eject_cdrom(const std::wstring& drive_letter){
        bool result = false;
        DWORD bytes = 0;
        DWORD error_code = 0;
        std::wstring device_path = boost::str(boost::wformat(L"\\\\.\\%c:") % drive_letter[0]);
        macho::windows::auto_file_handle handle = CreateFile(device_path.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        if (handle.is_valid()){
            if (DeviceIoControl(handle, FSCTL_LOCK_VOLUME, 0, 0, 0, 0, &bytes, 0)){
                if (DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, 0, 0, 0, 0, &bytes, 0)){
                    if (!DeviceIoControl(handle, IOCTL_STORAGE_EJECT_MEDIA, 0, 0, 0, 0, &bytes, 0)){
                        error_code = GetLastError();
                        LOG(LOG_LEVEL_ERROR, L"DeviceIoControl( IOCTL_STORAGE_EJECT_MEDIA ) : %s(0x%08X)", 
                            macho::windows::environment::get_system_message(error_code).c_str(), HRESULT_FROM_WIN32(error_code));
                    }
                    else{
                        result = true;
                    }
                }
                else{
                    error_code = GetLastError();
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl( FSCTL_DISMOUNT_VOLUME ) : %s(0x%08X)",
                        macho::windows::environment::get_system_message(error_code).c_str(), HRESULT_FROM_WIN32(error_code));
                }
            }
            else{
                error_code = GetLastError();
                LOG(LOG_LEVEL_ERROR, L"DeviceIoControl( FSCTL_LOCK_VOLUME ) : %s(0x%08X)",
                    macho::windows::environment::get_system_message(error_code).c_str(), HRESULT_FROM_WIN32(error_code));
            }
        }
        else{
            error_code = GetLastError();
            LOG(LOG_LEVEL_ERROR, L"CreateFile(%s) : %s(0x%08X)", 
                device_path.c_str(), macho::windows::environment::get_system_message(error_code).c_str(), HRESULT_FROM_WIN32(error_code));
        }
        return result;
    }
    
    DWORD install_vmware_tools(macho::windows::operating_system& os){
        DWORD retcode = 0;
        try{
            macho::windows::com_init com;
            wmi_services wmi;
            wmi.connect(L"CIMV2");
            wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
            std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
            if (std::wstring::npos != manufacturer.find(L"vmware")){
                TCHAR temp[MAX_PATH];
                LPTSTR drive;
                memset(temp, 0, sizeof(temp));
                DWORD ret = GetLogicalDriveStrings(MAX_PATH - 1, temp);
                drive = temp;
                while (ret && drive < (temp + ret)){
                    if (DRIVE_CDROM == GetDriveType(drive)){
                        std::wstring drive_letter = drive;
                        if (drive_letter[drive_letter.length() - 1] == L'\\')
                            drive_letter.erase(drive_letter.length() - 1);
                        if (boost::filesystem::exists(boost::str(boost::wformat(L"%s\\Program Files\\VMware\\VMware Tools") % drive_letter)) &&
                            boost::filesystem::exists(boost::str(boost::wformat(L"%s\\Setup64.exe") % drive_letter)) &&
                            boost::filesystem::exists(boost::str(boost::wformat(L"%s\\Setup.exe") % drive_letter))){
                            LOG(LOG_LEVEL_RECORD, L"VMWare Tools is ready on '%s'.", drive_letter.c_str());
                            try{
                                service sc = service::get_service(L"msiserver");
                                sc.start();
                                macho::windows::file_version_info fs = macho::windows::file_version_info::get_file_version_info(boost::str(boost::wformat(L"%s\\Setup.exe") % drive_letter));
                                std::wstring result;							
                                std::wstring cmd = boost::str(boost::wformat(fs.product_version_major() >= 9 ? L"%s\\%s /S /v\"/qn REBOOT=R\"" : L"%s\\%s /S /v\"/qn REBOOT=ReallySuppress\"") % drive_letter % (os.is_amd64() ? L"Setup64.exe" : L"Setup.exe"));
                                macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                                retcode = GetLastError();
                                LOG(LOG_LEVEL_RECORD, L"%s - result: (%d)\n%s ", cmd.c_str(), retcode, result.c_str());
                                eject_cdrom(drive_letter);
                                break;
                            }
                            catch (...){
                                LOG(LOG_LEVEL_ERROR, L"Failed to start service (msiserver).");
                            }
                        }
                    }
                    drive += _tcslen(drive) + 1;
                }
            }
        }
        catch (macho::exception_base& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
        }
        catch (const boost::filesystem::filesystem_error& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (const boost::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return retcode;
    }

    void worker()
    {
        // application behaviour
        // dump args
        boost::filesystem::path working_directory = macho::windows::environment::get_working_directory();
        try{
            setlocale(LC_ALL, "");
            const std::vector<std::wstring> &arg_vector =
                this_application()->find<application::args>()->arg_vector();

            LOG(LOG_LEVEL_RECORD, _T("---------- Arg List ---------"));

            // only print args on screen
            for (std::vector<std::wstring>::const_iterator it = arg_vector.begin();
                it != arg_vector.end(); ++it) {
                LOG(LOG_LEVEL_RECORD, L"%s", (*it).c_str());
            }

            LOG(LOG_LEVEL_RECORD, _T("-----------------------------"));

            // define options
            po::options_description loglevel("log level options");
            loglevel.add_options()
                ("level,l", po::value<int>(), "change log level")
                ;
            po::variables_map vm;
            po::store(po::parse_command_line(this_application()->find<application::args>()->argc(), this_application()->find<application::args>()->argv(), loglevel), vm);
            boost::system::error_code ec;

            if (vm.count("level"))
                set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());

            std::shared_ptr<application::status> st =
                this_application()->find<application::status>();
            macho::windows::operating_system os = macho::windows::environment::get_os_version();
            macho::windows::reg_native_edit reg_edit;
            stdstring param = boost::str(boost::wformat(L">>\"%s\"") % get_log_file());
            bool _is_safe_boot = macho::windows::environment::is_safe_boot();
            bool _is_force_normal_boot = is_froce_normal_boot() && !_is_safe_boot;
            if ( _is_force_normal_boot || _is_safe_boot ){
                if (_is_force_normal_boot)
                    LOG(LOG_LEVEL_RECORD, L"Boot into First Normal Boot sequence.");
                else
                    LOG(LOG_LEVEL_RECORD, L"Boot into Safe Mode.");
                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                enable_service_in_safe_mode msiserver(_T("msiserver"));
                std::wstring cmd = boost::str(boost::wformat(L"%s\\run.cmd") % working_directory.wstring());
                std::wstring result;
                if (boost::filesystem::exists(cmd)){
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                }
                else{
                    std::vector<boost::filesystem::path> inf_files = macho::windows::environment::get_files(working_directory, _T("*.inf"), true);
                    if (inf_files.size()){
                        bool copy_dpinst_files = false;
                        boost::filesystem::path logfile = get_log_file();
                        foreach(boost::filesystem::path& inf, inf_files){
                            if (!driver_package_install(inf.wstring(), is_driver_package_force())){
                                boost::filesystem::path t = logfile.parent_path() / inf.parent_path().stem();
                                copy_directory(inf.parent_path(), t );
                                copy_dpinst_files = true;
                            }
                        }
                        if (copy_dpinst_files){
                            if (boost::filesystem::exists(working_directory / L"dpinst.xml")){
                                boost::filesystem::copy_file(working_directory / L"dpinst.xml", logfile.parent_path() / L"dpinst.xml", boost::filesystem::copy_option::overwrite_if_exists);
                            }
                            if (boost::filesystem::exists(working_directory / L"dpinst.exe")){
                                boost::filesystem::copy_file(working_directory / L"dpinst.exe", logfile.parent_path() / L"dpinst.exe", boost::filesystem::copy_option::overwrite_if_exists);
                            }
                            if (boost::filesystem::exists(working_directory / L"dpinst.cmd")){
                                boost::filesystem::copy_file(working_directory / L"dpinst.cmd", logfile.parent_path() / L"dpinst.cmd", boost::filesystem::copy_option::overwrite_if_exists);
                            }
                            if (boost::filesystem::exists(working_directory / L"dpinst.exe")){
                                if (!IsWindowsVistaOrGreater()){
                                    registry reg(REGISTRY_CREATE);
                                    if (reg.open(_T("Software\\Policies\\Microsoft\\Windows\\DeviceInstall\\Settings"))){
                                        reg[L"SuppressNewHWUI"] = (DWORD)0x1;
                                        reg.close();
                                    }
                                    if (reg.open(_T("SYSTEM\\CurrentControlSet\\Services\\PlugPlay\\Parametes"))){
                                        reg[L"SuppressUI"] = (DWORD)0x1;
                                        reg.close();
                                    }
                                }

                                registry reg;
                                if (reg.open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"))){
                                    if (boost::filesystem::exists(working_directory / L"dpinst.cmd"))
                                        reg[_T("dpinst")] = boost::str(boost::wformat(L"cmd.exe /c \"%s\\dpinst.cmd\"") % logfile.parent_path().wstring());
                                    else
                                        reg[_T("dpinst")] = boost::str(boost::wformat(L"cmd.exe /c \"%s\\dpinst.exe\" /sa /se /sw /sh /lm /c") % logfile.parent_path().wstring());
                                }
                            }
                        }
                        else
                        {
                            macho::windows::setup_inf_file::earse_unused_oem_driver_packages();
                        }
                        macho::windows::device_manager::rescan_devices();
                    }
                }

                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                //Enable Remote Desktop firewall
                cmd = boost::str(boost::wformat(L"%s\\netsh.exe advfirewall firewall set rule group=\"remote desktop\" new enable=Yes") % macho::windows::environment::get_system_directory());
                macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
				cmd = boost::str(boost::wformat(L"%s\\netsh.exe advfirewall firewall set rule group=\"@FirewallAPI.dll,-28752\" new enable=Yes") % macho::windows::environment::get_system_directory());
				macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
				LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);

                cmd = boost::str(boost::wformat(L"%s\\run-safe.cmd") % working_directory.wstring());
                if (boost::filesystem::exists(cmd)){
                    LOG(LOG_LEVEL_RECORD, L"Run Command : %s ", cmd.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                }

                /*
                //Move script call into run-safe.cmd
                cmd = boost::str(boost::wformat(L"%s\\PostScript\\run-safe.cmd") % working_directory.wstring());
                if (boost::filesystem::exists(cmd)){
                    LOG(LOG_LEVEL_RECORD, L"Run Command : %s ", cmd.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                }*/

                int retry = 3;
                while (retry > 0){
                    try{
                        macho::windows::com_init com;
                        remap_volumes();
                        boost::filesystem::path storage_json = boost::filesystem::path(macho::windows::environment::get_windows_directory()) / L"storage.json";
                        if (boost::filesystem::exists(storage_json)){
                            SetFileAttributesW(storage_json.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                            boost::filesystem::remove(storage_json);
                        }
                        break;
                    }
                    catch (macho::exception_base& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                    }
                    catch (const boost::filesystem::filesystem_error& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (const boost::exception &ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                    }
                    catch (const std::exception& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    }
                    retry--;
                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                }
                reg_edit.delete_tree(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot_");
                reg_edit.delete_tree(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services_");
                reg_edit.delete_tree(HKEY_LOCAL_MACHINE, L"SYSTEM\\MountedDevices_");

                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);

                if (!boost::filesystem::exists(working_directory / L"run-normal.cmd")){
                    try{
                        boost::filesystem::directory_iterator end_itr;
                        // cycle through the directory
                        for (boost::filesystem::directory_iterator itr(working_directory); itr != end_itr; ++itr){
                            // If it's not a directory, list it. If you want to list directories too, just remove this check.
                            if (boost::filesystem::is_regular_file(itr->path())) {
                                if (this_application()->find<application::path>()->executable_path_name() != itr->path()){
                                    SetFileAttributesW(itr->path().wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                                    MoveFileExW(itr->path().wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                                }
                            }
                        }
                    }
                    catch (const boost::filesystem::filesystem_error& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }

                    registry reg;
                    if (reg.open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"))){
                        reg[_T("!RemoveConvFolder")] = boost::str(boost::wformat(L"cmd.exe /c rd /s /q \"%s\"") % working_directory.wstring());
                    }
                    MoveFileExW(this_application()->find<application::path>()->executable_path_name().wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    MoveFileExW(working_directory.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                }

                try{
                    if (!boost::filesystem::exists(working_directory / L"dont_remove_vmware")){
                        macho::windows::com_init com;
                        wmi_services wmi;
                        wmi.connect(L"CIMV2");
                        wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
                        std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
                        if (std::wstring::npos == manufacturer.find(L"vmware")){
                            registry reg;
                            if (reg.open(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall")){
                                reg.refresh_subkeys();
                                int subkey_count = reg.subkeys_count();
                                for (int i = 0; i < subkey_count; i++){
                                    std::wstring display_name = reg.subkey(i)[L"DisplayName"].wstring();
                                    if (display_name.length() && display_name == L"VMware Tools"){
                                        cmd = boost::str(boost::wformat(L"%s\\msiexec.exe /X%s /quiet /norestart") % environment::get_system_directory() % reg.subkey(i).key_name());
                                        std::wstring runonce_cmd = boost::str(boost::wformat(L"%s\\msiexec.exe /X%s /passive /norestart") % environment::get_system_directory() % reg.subkey(i).key_name());
                                        LOG(LOG_LEVEL_RECORD, L"Uninstall VMware Tools : %s", cmd.c_str());
                                        macho::windows::process::exec_console_application_with_timeout(cmd, result, 60 * 5, true);
                                        LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                                        if (reg.open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"))){
                                            reg[_T("!UninstallVMTools")] = runonce_cmd;
                                        }
                                        boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                catch (macho::exception_base& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                }
                catch (const boost::filesystem::filesystem_error& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (const boost::exception &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                }
                catch (const std::exception& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                }

                if (is_auto_reboot_disabled() || (boost::filesystem::exists(working_directory / L"disable_auto_reboot"))){
                    LOG(LOG_LEVEL_RECORD, TEXT("Disabled auto system reboot."));
                }
                else if (macho::windows::environment::shutdown_system( true, _T("irm_conv_agent - auto system reboot."), 5)){
                    LOG(LOG_LEVEL_RECORD, TEXT("System shutdown succeeded."));
                }
                else{
                    LOG(LOG_LEVEL_ERROR, TEXT("System shutdown failed (%d)."), GetLastError());
                    LOG(LOG_LEVEL_RECORD, TEXT("shutdown -r -f -t 5"));
                    macho::windows::process::exec_console_application_without_wait(L"shutdown -r -f -t 5");
                }
                clear_disable_auto_reboot_flag();
                clear_driver_package_force_flag();
                clear_force_normal_boot_flag();
                if (set_safe_mode(false, os) && !boost::filesystem::exists(working_directory / L"run-normal.cmd")){
                    cmd = boost::str(boost::wformat(L"%s -u") % this_application()->find<application::path>()->executable_path_name().wstring());
                    macho::windows::process::exec_console_application_without_wait(cmd);
                }         
                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
            }
            else{
                LOG(LOG_LEVEL_RECORD, L"Boot into Normal Mode.");
                std::wstring cmd = boost::str(boost::wformat(L"%s\\run-install.cmd") % working_directory.wstring());
                std::wstring ran = boost::str(boost::wformat(L"%s\\run-install.cmd.ran") % working_directory.wstring());
                std::wstring result;
                if (boost::filesystem::exists(cmd)){
                    LOG(LOG_LEVEL_RECORD, L"Run Command : %s ", cmd.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    SetFileAttributesW(cmd.c_str(), FILE_ATTRIBUTE_NORMAL);
                    boost::filesystem::rename(cmd, ran);
                }
                cmd = boost::str(boost::wformat(L"%s\\run-normal.cmd") % working_directory.wstring());
                ran = boost::str(boost::wformat(L"%s\\run-normal.cmd.ran") % working_directory.wstring());
                if (boost::filesystem::exists(cmd)){
                    LOG(LOG_LEVEL_RECORD, L"Run Command : %s ", cmd.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    SetFileAttributesW(cmd.c_str(), FILE_ATTRIBUTE_NORMAL);
                    boost::filesystem::rename(cmd, ran);
                }
                /*
                //Move script call into run-normal.cmd
                cmd = boost::str(boost::wformat(L"%s\\PostScript\\run.cmd") % working_directory.wstring());
                if (boost::filesystem::exists(cmd)){
                    LOG(LOG_LEVEL_RECORD, L"Run Command : %s ", cmd.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
                    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                }*/
                DWORD retcode = install_vmware_tools(os);
                if (ERROR_SUCCESS_REBOOT_REQUIRED == retcode && (!boost::filesystem::exists(working_directory / L"dont_reboot_vmware_tools"))){
                    if (macho::windows::environment::shutdown_system(true, _T("irm_conv_agent - auto system reboot for vmware tools installation."), 5)){
                        LOG(LOG_LEVEL_RECORD, TEXT("System reboot for vmware tools installation succeeded."));
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, TEXT("System reboot(vmware tools installation) failed (%d)."), GetLastError());
                        LOG(LOG_LEVEL_RECORD, TEXT("shutdown -r -f -t 5"));
                        macho::windows::process::exec_console_application_without_wait(L"shutdown -r -f -t 5");
                    }
                    boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
                    return;
                }
                std::wstring ret;
                long      response = 0;
                std::vector<std::wstring> callbacks;
                int timeout = 30;
                get_callback_info(callbacks, timeout);
                if (callbacks.size()){
                    boost::posix_time::ptime callback_start = boost::posix_time::second_clock::universal_time();
                    bool has_notified = false;
                    while (!has_notified) {
                        foreach(std::wstring &callback, callbacks){
                            http_client client(callback.find(L"https://") != std::wstring::npos);
                            CURLcode code = client.get(callback, L"", L"", response, ret);
                            LOG(LOG_LEVEL_RECORD, _T("sent notify url (%s) and return code is %d"), callback.c_str(), response ? response : code);
                            if (response == 200 || response == 202){
                                has_notified = true;
                                break;
                            }
                        }                
                        if (!has_notified){
                            boost::posix_time::time_duration duration(boost::posix_time::second_clock::universal_time() - callback_start);
                            if (duration.total_seconds() > timeout)
                                break;
                            boost::this_thread::sleep(boost::posix_time::seconds(5));
                        }
                    };
                }
                clear_callback_info();
                try{
                    boost::filesystem::directory_iterator end_itr;
                    // cycle through the directory
                    for (boost::filesystem::directory_iterator itr(working_directory); itr != end_itr; ++itr){
                        // If it's not a directory, list it. If you want to list directories too, just remove this check.
                        if (boost::filesystem::is_regular_file(itr->path())) {
                            if (this_application()->find<application::path>()->executable_path_name() != itr->path()){
                                SetFileAttributesW(itr->path().wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                                MoveFileExW(itr->path().wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                            }
                        }
                    }
                }
                catch (const boost::filesystem::filesystem_error& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }

                registry reg;
                if (reg.open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"))){                    
                    reg[_T("!RemoveConvFolder")] = boost::str(boost::wformat(L"cmd.exe /c rd /s /q \"%s\"") % working_directory.wstring());
                }
            
                MoveFileExW(this_application()->find<application::path>()->executable_path_name().wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                MoveFileExW(working_directory.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

                cmd = boost::str(boost::wformat(L"%s -u") % this_application()->find<application::path>()->executable_path_name().wstring());
                macho::windows::process::exec_console_application_without_wait(cmd);

                boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
            }
            /*
            while (st->state() != application::status::stopped)
            {
                if (st->state() == application::status::paused)
                    LOG(LOG_LEVEL_WARNING, _T("paused..."));
                else{
                    LOG(LOG_LEVEL_TRACE, _T("running..."));
                }
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            */
        }
        catch (macho::exception_base& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
        }
        catch (const boost::filesystem::filesystem_error& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (const boost::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        boost::filesystem::copy_file(get_log_file(), working_directory / boost::filesystem::path(get_log_file()).stem(), boost::filesystem::copy_option::overwrite_if_exists);
    }

    // param
    int operator()()
    {
        TCHAR szLogPath[MAX_PATH];
        SecureZeroMemory(szLogPath, sizeof(szLogPath));
        boost::filesystem::path logfile;
        if (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szLogPath)){
            logfile = boost::str(boost::wformat(L"%s\\logs\\%s.log") % szLogPath %this_application()->find<application::path>()->executable_name().stem().wstring());
        }
        else{
            logfile = this_application()->find<application::path>()->executable_path().string() + "/logs/" + this_application()->find<application::path>()->executable_name().stem().string() + ".log";
        }
        boost::filesystem::create_directories(logfile.parent_path());
        set_log_file(logfile.wstring());
        set_log_level(LOG_LEVEL_WARNING);
        // launch a work thread
        boost::thread thread(&main_app::worker, this);

        this_application()->find<application::wait_for_termination_request>()->wait();

        return 0;
    }

    // windows/posix

    bool stop()
    {
        LOG(LOG_LEVEL_RECORD, _T("Stopping application..."));
        return true; // return true to stop, false to ignore
    }

    // windows specific (ignored on posix)

    bool pause()
    {
        //LOG(LOG_LEVEL_RECORD, _T("Pause application..."));
        return false; // return true to pause, false to ignore
    }

    bool resume()
    {
        //LOG(LOG_LEVEL_RECORD, _T("Resume application..."));
        return false; // return true to resume, false to ignore
    }

private:
};

bool setup(application::context& context)
{
    strict_lock<application::aspect_map> guard(context);

    std::shared_ptr<application::args> myargs
        = context.find<application::args>(guard);

    std::shared_ptr<application::path> mypath
        = context.find<application::path>(guard);

    // provide setup for windows service
#if defined(BOOST_WINDOWS_API)
#if !defined(__MINGW32__)

    // get our executable path name
    boost::filesystem::path executable_path_name = mypath->executable_path_name();

    // define our simple installation schema options
    po::options_description install("service options");
    install.add_options()
        ("help", "produce a help message")
        (",i", "install service")
        (",u", "unistall service")
        ("name", po::wvalue<std::wstring>()->default_value(mypath->executable_name().stem().wstring(), mypath->executable_name().stem().string()), "service name")
        ("display", po::wvalue<std::wstring>()->default_value(L"IronMan Converter Service", "IronMan Converter Service"), "service display name (optional, installation only)")
        ("description", po::wvalue<std::wstring>()->default_value(L"IronMan Converter Service", "IronMan Converter Service"), "service description (optional, installation only)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(myargs->argc(), myargs->argv(), install), vm);
    boost::system::error_code ec;

    if (vm.count("help"))
    {
        std::cout << install << std::endl;
        return true;
    }

    if (vm.count("-i"))
    {
        application::example::install_windows_service(
            application::setup_arg(vm["name"].as<std::wstring>()),
            application::setup_arg(vm["display"].as<std::wstring>()),
            application::setup_arg(vm["description"].as<std::wstring>()),
            application::setup_arg(executable_path_name)).install(ec);

        std::cout << ec.message() << std::endl;

        return true;
    }

    if (vm.count("-u"))
    {
        application::example::uninstall_windows_service(
            application::setup_arg(vm["name"].as<std::wstring>()),
            application::setup_arg(executable_path_name)).uninstall(ec);
        
        std::cout << ec.message() << std::endl;
        
        try{
            macho::windows::service sc = macho::windows::service::get_service(vm["name"].as<std::wstring>());
            sc.stop();
        }
        catch (...){
        }
        return true;
    }

#endif
#endif

    return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
    WSADATA wsaData = {};
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    application::global_context_ptr ctx = application::global_context::create();

    // auto_handler will automatically add termination, pause and resume (windows) handlers
    application::auto_handler<main_app> app(ctx);
    // application::detail::handler_auto_set<myapp> app(app_context);

    // my server aspects

    this_application()->insert<application::path>(
        std::make_shared<application::path_default_behaviour>(argc, argv));

    this_application()->insert<application::args>(
        std::make_shared<application::args>(argc, argv));

    // check if we need setup

    if (setup(*ctx.get()))
    {
        std::cout << "[I] Setup changed the current configuration." << std::endl;
        application::global_context::destroy();
        return 0;
    }

    // my server instantiation
#if _DEBUG
    app.worker();
    return 0;
#else
    boost::system::error_code ec;
    int result = application::launch<application::server>(app, ctx, ec);

    if (ec)
    {
        std::cout << "[E] " << ec.message()
            << " <" << ec.value() << "> " << std::endl;
    }

    application::global_context::destroy();
    return result;
#endif
}
