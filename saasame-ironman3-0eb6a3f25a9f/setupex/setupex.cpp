// setupex.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "setupex.h"
#include "macho.h"
#include "boost\filesystem.hpp"
#include "..\vcbt\vcbt\journal.h"

using namespace macho::windows;
using namespace macho;

//
// For Packer package usage
//

BOOL SendDeviceIoControl(std::wstring path, DWORD ioctl, LPVOID input, LONG size_of_input, LPVOID out, DWORD& size_of_out)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    BOOL bResult = FALSE;                 // results flag
    DWORD junk = 0;                     // discard results

    hDevice = CreateFileW(path.c_str(),          // drive to open
        0,                // no access to the drive
        FILE_SHARE_READ | // share mode
        FILE_SHARE_WRITE,
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", path.c_str(), GetLastError());
        return (FALSE);
    }

    if (!(bResult = DeviceIoControl(hDevice,                       // device to be queried
        ioctl,                        // operation to perform
        input, size_of_input,                      // input buffer
        out, size_of_out,                      // output buffer
        &size_of_out,                         // # bytes returned
        (LPOVERLAPPED)NULL)))          // synchronous I/O
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to DeviceIoControl(%s, %d), Error(0x%08x).", path.c_str(), ioctl, GetLastError());
    }
    CloseHandle(hDevice);

    return (bResult);
}

DWORD write_to_file(boost::filesystem::path file_path, LPCVOID buffer, DWORD number_of_bytes_to_write, DWORD mode){
    DWORD dwRetVal;
    DWORD  rc = ERROR_SUCCESS;
    SetFileAttributes(file_path.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
    macho::windows::auto_file_handle hFile = CreateFile((LPTSTR)file_path.wstring().c_str(), // file name 
        GENERIC_WRITE,        // open for write 
        0,                    // do not share 
        NULL,                 // default security 
        mode,               // create mode
        FILE_ATTRIBUTE_NORMAL,// normal file 
        NULL);
    if (hFile.is_valid()){
        SetFilePointer(hFile, 0, NULL, FILE_END);
        if (!WriteFile(hFile, buffer, number_of_bytes_to_write, &dwRetVal, NULL))
            rc = GetLastError();
    }
    else
        rc = GetLastError();
    return rc;
}

DWORD write_to_file(boost::filesystem::path file_path, std::string content, DWORD mode = CREATE_ALWAYS){
    return write_to_file(file_path, content.c_str(), content.length(), mode);
}

SETUPEX_API UINT Install(MSIHANDLE hInstall)
{
    std::wstring result;
    std::wstring cmd = boost::str(boost::wformat(L"%s\\netsh advfirewall firewall add rule name=\"Open Port 18889\" dir=in action=allow protocol=TCP localport=18889") % macho::windows::environment::get_system_directory());
    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
    cmd = boost::str(boost::wformat(L"%s\\sc.exe failure \"irm_host_packer\" reset=86400 actions= restart/60000/restart/60000/restart/600000") % macho::windows::environment::get_system_directory());
    result.clear();
    macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
    LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
    REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
    registry reg(flag);
    bool need_to_check_and_repair = false;
    if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{71a27cdd-812a-11d0-bec7-08002be2092f}")){
        for (int i = 0; i < reg[L"UpperFilters"].get_multi_count(); i++){
            if (0 == _tcsicmp(reg[L"UpperFilters"].get_multi_at(i), _T("vcbt"))){
                need_to_check_and_repair = true;
                break;
            }
        }
        reg.close();
    }

    if (need_to_check_and_repair){
        flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_CREATE : REGISTRY_CREATE_WOW64_64KEY) : REGISTRY_CREATE;
        registry reg2(flag);
        if (!reg.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt")){
            if (reg2.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt")){
                reg2[L"FileNam"] = L"%SystemRoot%\\system32\\LogFiles\\Trace_vcbt.etl";
                reg2[L"BufferSize"] = 0x00000080;
                reg2[L"FlushTimer"] = 0x00000000;
                reg2[L"MaximumBuffers"] = 0x00000280;
                reg2[L"MinimumBuffers"] = 0x00000080;
                reg2[L"Start"] = 0x00000001;
                reg2[L"ClockType"] = 0x00000001;
                reg2[L"MaxFileSize"] = 0x00000000;
                reg2[L"LogFileMode"] = 0x00001200;
                reg2[L"Guid"] = L"{9E506DF1-B7DC-4F70-AE2C-4C5319361AE8}";
                if (reg2.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt\\{9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1}")){
                    reg2[L"MatchAllKeyword"] = (LONGLONG)0x00000000;
                    reg2[L"MatchAnyKeyword"] = (LONGLONG)0xffffffff;
                    reg2[L"Enabled"] = 0x00000001;
                    reg2[L"EnableLevel"] = 0x000000ff;
                    reg2[L"EnableProperty"] = 0x00000000;
                }
            }
        }
        
        if (reg2.open(L"SYSTEM\\CurrentControlSet\\Services\\vcbt")){
            if (!(reg2[L"Start"].exists() && reg2[L"ImagePath"].exists() && reg2[L"Type"].exists() && reg2[L"Group"].exists())){
                reg2[L"DisplayName"] = L"Volume Change Block Tracking Filter Driver";
                reg2[L"ErrorControl"] = (DWORD)0x1;
                reg2[L"Group"] = L"PnP Filter";
                reg2[L"ImagePath"] = L"system32\\DRIVERS\\vcbt.sys";
                reg2[L"Start"] = (DWORD)0x0;
                reg2[L"Tag"] = (DWORD)0x14;
                reg2[L"Type"] = (DWORD)0x1; 
                reg2.subkey(L"Parameters", true)[L"BatchUpdateJournalMetaData"] = (DWORD)0x1;
                reg2.subkey(L"Parameters", true)[L"DisableUMAPFlush"] = (DWORD)0x1;
            }
        }
    }
    return 0;
}

SETUPEX_API UINT Maintenance(MSIHANDLE hInstall)
{
    REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE  : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
    registry reg(flag);
    if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\vcbt")){
        reg[L"Start"] = (DWORD)0x0;

        flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_CREATE : REGISTRY_CREATE_WOW64_64KEY) : REGISTRY_CREATE;
        registry reg2(flag);
        if (!reg.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt")){
            if (reg2.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt")){
                reg2[L"FileNam"] = L"%SystemRoot%\\system32\\LogFiles\\Trace_vcbt.etl";
                reg2[L"BufferSize"] = 0x00000080;
                reg2[L"FlushTimer"] = 0x00000000;
                reg2[L"MaximumBuffers"] = 0x00000280;
                reg2[L"MinimumBuffers"] = 0x00000080;
                reg2[L"Start"] = 0x00000001;
                reg2[L"ClockType"] = 0x00000001;
                reg2[L"MaxFileSize"] = 0x00000000;
                reg2[L"LogFileMode"] = 0x00001200;
                reg2[L"Guid"] = L"{9E506DF1-B7DC-4F70-AE2C-4C5319361AE8}";
                if (reg2.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt\\{9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1}")){
                    reg2[L"MatchAllKeyword"] = (LONGLONG)0x00000000;
                    reg2[L"MatchAnyKeyword"] = (LONGLONG)0xffffffff;
                    reg2[L"Enabled"] = 0x00000001;
                    reg2[L"EnableLevel"] = 0x000000ff;
                    reg2[L"EnableProperty"] = 0x00000000;
                }
            }
        }
    }
    return 0;
}

SETUPEX_API UINT Uninstall(MSIHANDLE hInstall)
{
    REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
    registry reg(flag);
    bool need_to_remove = true;

#if 0
    if (reg.open(L"SOFTWARE\\SaaSaMe\\Packer for Windows x64"))
        need_to_remove = !reg[L"Path"].exists();
    else if (reg.open(L"SOFTWARE\\SaaSaMe\\Packer for Windows x86"))
        need_to_remove = !reg[L"Path"].exists();
#endif
    
    if (need_to_remove){
        
        if (reg.open(L"SOFTWARE\\Saasame\\Transport"))
        {
            reg[L"MgmtAddr"].delete_value();
            reg[L"SessionId"].delete_value();
            reg[L"CompressedByPacker"].delete_value();
            reg[L"AllowMultiple"].delete_value();
        }

        if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{71a27cdd-812a-11d0-bec7-08002be2092f}")){
            reg[L"UpperFilters"].remove_multi(L"vcbt");
            if (0 == reg[L"UpperFilters"].get_multi_count())
                reg[L"UpperFilters"].delete_value();
        }

        if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\vcbt")){
            reg.delete_key();
        }

        if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\Trace_vcbt")){
            reg.delete_key();
        }

        macho::windows::com_init com;
        std::wstring result;
        std::wstring cmd = boost::str(boost::wformat(L"%s\\netsh advfirewall firewall delete rule name=\"Open Port 18889\"") % macho::windows::environment::get_system_directory());
        macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
        LOG(LOG_LEVEL_RECORD, L"%s - result: \n%s", cmd.c_str(), result.c_str());
        std::vector<macho::guid_> guids;
        macho::windows::storage::ptr stg = macho::windows::storage::get();
        macho::windows::storage::disk::vtr disks = stg->get_disks();
        foreach(macho::windows::storage::disk::ptr d, disks){
            macho::windows::storage::volume::vtr volumes = d->get_volumes();
            foreach(macho::windows::storage::volume::ptr v, volumes){
                if (v->access_paths().size()){
                    guids.push_back(v->id());
                }
            }
        }
        DWORD input_length = sizeof(VCBT_COMMAND_INPUT) + (sizeof(VCBT_COMMAND) * guids.size());
        DWORD output_length = sizeof(VCBT_COMMAND_RESULT) + (sizeof(VCOMMAND_RESULT) * guids.size());
        std::auto_ptr<VCBT_COMMAND_INPUT> command = std::auto_ptr<VCBT_COMMAND_INPUT>((PVCBT_COMMAND_INPUT)new BYTE[input_length]);
        std::auto_ptr<VCBT_COMMAND_RESULT> ret = std::auto_ptr<VCBT_COMMAND_RESULT>((PVCBT_COMMAND_RESULT)new BYTE[output_length]);
        memset(command.get(), 0, input_length);
        memset(ret.get(), 0, output_length);

        command->NumberOfCommands = guids.size();
        for (int i = 0; i < guids.size(); i++){
            command->Commands[i].VolumeId = guids[i];
        }
        if (SendDeviceIoControl(VCBT_WIN32_DEVICE_NAME, IOCTL_VCBT_DISABLE, (LPVOID)command.get(), input_length, (LPVOID)ret.get(), output_length)){
            LOG(LOG_LEVEL_RECORD, L"Succeed to send command(0x%08x).", IOCTL_VCBT_DISABLE);
            std::cout << "Succeed to send command." << std::endl;
        }
    }
#if 0
    boost::filesystem::path drv = environment::get_system_directory(); 
    drv /= L"drivers";
    drv /= L"vcbt.sys";
    if (boost::filesystem::exists(drv)){
        MoveFileExW(drv.wstring().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }
#endif
    return 0;
}

#define PRODUCT_CODE_LENGTH 100
SETUPEX_API UINT Upgrade(MSIHANDLE hInstall)
{
    TCHAR szProductCode[PRODUCT_CODE_LENGTH];
    TCHAR szOldProductCode[PRODUCT_CODE_LENGTH];
    DWORD dwLen = sizeof(szProductCode) / sizeof(szProductCode[0]);
    UINT res = ::MsiGetProperty(hInstall, _T("ProductCode"), szProductCode, &dwLen);
    if (res != ERROR_SUCCESS)
    {
        return 1;
    }

    dwLen = sizeof(szOldProductCode) / sizeof(szOldProductCode[0]);
    res = ::MsiGetProperty(hInstall, _T("OLDPRODUCTS"), szOldProductCode, &dwLen);
    if (res != ERROR_SUCCESS)
    {
        return 1;
    }

    if (0 == _tcsicmp(szProductCode, szOldProductCode))
    {
        REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
        registry reg(flag);
        if (reg.open(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"))
        {
            reg.refresh_subkeys();
            for (int i = 0; i < reg.subkeys_count(); i++)
            {
                if (reg.subkey(i)[L"UninstallPath"].exists())
                {
                    std::wstring UninstallPath = reg.subkey(i)[L"UninstallPath"].wstring();
                    if (std::wstring::npos != UninstallPath.find(szOldProductCode))
                    {
                        reg.subkey(i).delete_key();
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

//
// For Transport Installation package usage 
//
SETUPEX_API UINT StartDB(MSIHANDLE hInstall)
{
    try
    {
        macho::windows::service MariaDB = macho::windows::service::get_service(L"MariaDB");
        if (!MariaDB.start())
            return 1;
    }
    catch (...)
    {
        return 1;
    }
    return 0;
}

SETUPEX_API UINT SetNetworksMTU(MSIHANDLE hInstall)
{
    try
    {
        macho::windows::com_init com;
        wmi_services wmi;
        wmi.connect(L"CIMV2");
        wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
        std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
        std::wstring model = macho::stringutils::tolower((std::wstring)computer_system[L"Model"]);
        if (std::wstring::npos != manufacturer.find(L"openstack") || std::wstring::npos != model.find(L"openstack")){
            macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
            std::wstring wdefault_mtu = L"1400";
            foreach(macho::windows::network::adapter::ptr adapter, adapters){
                macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                std::wstring caption = stringutils::tolower(adapter_settings->caption());
                if ((!adapter_settings->ip_enabled()) || (caption.find(_T("loopback")) != std::wstring::npos) || (caption.find(_T("loopback")) != std::wstring::npos))
                    continue;
                LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                LOG(LOG_LEVEL_RECORD, L"mtu : %s", wdefault_mtu.c_str());

                std::wstring command = _T("cmd.exe /C netsh interface ipv4 set subinterface \"");
                command += adapter->net_connection_id();
                command += _T("\" mtu=");
                command += wdefault_mtu;
                command += _T(" store=persistent");
                macho::windows::process::exec_console_application_with_retry(command);
            }
        }
    }
    catch (...)
    {
        return 1;
    }
    return 0;
}

SETUPEX_API UINT UninstallTransport(MSIHANDLE hInstall){
    REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
    registry reg(flag);
    if (reg.open(L"SOFTWARE\\Saasame\\Transport"))
    {
        reg[L"MgmtAddr"].delete_value();
        reg[L"SessionId"].delete_value();
    }
    return 0;
}

SETUPEX_API UINT UpgradeTransport(MSIHANDLE hInstall){
	try
	{
        using namespace macho;
		std::wstring webroot = macho::windows::environment::get_environment_variable(L"WEBROOT");
		if (!webroot.empty()){
			macho::windows::service MariaDB = macho::windows::service::get_service(L"MariaDB");
			if (!MariaDB.start())
				return 1;
			std::time_t start_time = std::time(nullptr);
			boost::filesystem::path upgrade_backup_file = boost::filesystem::path(webroot) / boost::str(boost::wformat(L"backup_%1%.zip") % start_time);
			boost::filesystem::path mysqldump = boost::filesystem::path(MariaDB.binary_path()).parent_path() / L"mysqldump.exe";
			boost::filesystem::path sqllit_db = boost::filesystem::path(webroot) / L"apache24\\htdocs\\portal\\_include\\_inc\\pixiu.sqlite";
			archive::zip::ptr zip_ptr = archive::zip::open(upgrade_backup_file);
			if (zip_ptr){
				if (boost::filesystem::exists(mysqldump)){
					std::wstring cmd = boost::str(boost::wformat(L"cmd.exe /C \"%1%\" -uroot -psaasameFTW --all-databases") % mysqldump.wstring());
					std::wstring result;
					macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
					zip_ptr->add("Backup.sql", macho::stringutils::convert_unicode_to_utf8(result));
				}
				if (boost::filesystem::exists(sqllit_db)){
					zip_ptr->add(sqllit_db);
				}
				zip_ptr->close();
			}
		}
	}
	catch (...)
	{
	}
	return 0;
}
