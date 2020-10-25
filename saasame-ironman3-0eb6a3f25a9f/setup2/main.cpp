// Setup.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "macho.h"
#include <boost/program_options.hpp>
#include <iostream>     // std::cout
#include <fstream>
#include <string>
#include <VersionHelpers.h>
#include "resource.h"

#define HOTFIX_CHECK 0
using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

void extract_binary_resource( stdstring custom_resource_name, int resource_id, stdstring output_file_path, bool append ){
    HGLOBAL hResourceLoaded;		// handle to loaded resource 
    HRSRC hRes;						// handle/ptr. to res. info. 
    char *lpResLock;				// pointer to resource data 
    DWORD dwSizeRes;
    // find location of the resource and get handle to it
    hRes = FindResource( NULL, MAKEINTRESOURCE(resource_id), custom_resource_name.c_str() );
    // loads the specified resource into global memory. 
    hResourceLoaded = LoadResource( NULL, hRes ); 
    // get a pointer to the loaded resource!
    lpResLock = (char*)LockResource( hResourceLoaded ); 
    // determine the size of the resource, so we know how much to write out to file!  
    dwSizeRes = SizeofResource( NULL, hRes );
    std::ofstream outputFile(output_file_path.c_str(), append ? std::ios::binary | std::ios::app : std::ios::binary );
    outputFile.write((const char*)lpResLock, dwSizeRes);
    outputFile.close();
}

std::wstring GetResourceString(UINT uID){
    TCHAR buff[255];
    memset(buff, 0, sizeof(buff));
    if (LoadString(GetModuleHandle(NULL), uID, buff, sizeof(buff) / sizeof(buff[0]))){
        return buff;
    }
    return L"";
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow){

    macho::windows::com_init com;
    DWORD dwRet = 0;
    boost::filesystem::path exepath = macho::windows::environment::get_execution_full_path();
    std::wstring working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path logfile = boost::str(boost::wformat(L"\"%s\\%s.log\"")% macho::windows::environment::get_temp_path() % exepath.filename().wstring());
    macho::set_log_file(logfile.wstring());
    macho::set_log_level(LOG_LEVEL_WARNING);

#if _DEBUG
    exepath = L"C:\\Workspace\\ironman2\\output\\771\\linuxlauncher.exe";
#endif
    //https://github.com/binary1248/SFMLInstaller/blob/master/src/Decompress.cpp
    std::ifstream inputFile(exepath.c_str(), std::ios::binary|std::ios::in );
    char sig[4];
    int zip_offset = 0;
    bool found = false;
    while (inputFile && inputFile.read(sig, 4)){
#define MZ_ZIP_LOCAL_DIR_HEADER_SIG 0x04034b50
#define MZ_READ_LE32(p) *((const unsigned int *)(p))
        if (MZ_ZIP_LOCAL_DIR_HEADER_SIG == MZ_READ_LE32(sig)){
            found = true;
            break;
        }
        zip_offset += 4;
    }
    inputFile.close();
    if (!found){
        macho::windows::operating_system os = macho::windows::environment::get_os_version();
        if (os.version_info.dwMajorVersion == 6 && os.version_info.dwMinorVersion == 3){
            std::wstring cmd = L"cmd.exe /C wmic qfe get hotfixid | find \"KB2919355\"";
            std::wstring result;
            macho::windows::process::exec_console_application_with_timeout(cmd, result, -1, true);
            if (std::wstring::npos == result.find(L"KB2919355")){
                if (std::wstring::npos == macho::stringutils::tolower(std::wstring(GetCommandLine())).find(L"--hide"))
                    MessageBox(NULL, GetResourceString(IDS_KB291935).c_str(), GetResourceString(IDS_WARNING).c_str(), MB_TOPMOST | MB_ICONERROR | MB_OK);
                return 1;
            }
        }
		return 0;
    }

    macho::archive::unzip::ptr unzip_ptr = macho::archive::unzip::open(exepath, zip_offset);
    if (NULL == unzip_ptr){
        LOG(LOG_LEVEL_ERROR, _T("Can't find archived data!"));
        dwRet = 1;
    }
    else if (unzip_ptr->file_exists("dpinst.exe")) {
        boost::filesystem::path temp = macho::windows::environment::create_temp_folder();
        std::vector<std::string> extracted_files;
        if (unzip_ptr->decompress_archive(temp.string())){
            boost::filesystem::path dpinst_exe = temp / "dpinst.exe";
            std::wstring ret;
            macho::windows::process::exec_console_application_with_timeout(dpinst_exe.wstring(), ret);
        }
        boost::filesystem::remove_all(temp);
    }
    else if (unzip_ptr->file_exists("upgrade.cmd")) {
        boost::filesystem::path temp = macho::windows::environment::create_temp_folder();
        std::vector<std::string> extracted_files;
        if (unzip_ptr->decompress_archive(temp.string())){
            boost::filesystem::path upgrade_cmd = temp / "upgrade.cmd";
            std::wstring ret;
            macho::windows::process::exec_console_application_with_timeout(upgrade_cmd.wstring(), ret);
            MessageBox(NULL, ret.c_str(), L"Upgrade", MB_OK);
        }
        boost::filesystem::remove_all(temp);
    }
    else if (unzip_ptr->file_exists("linux_launcher.img")) {
        mutex m(L"LinuxMutex");
        bool lock = m.trylock();
        if (!lock){
            MessageBox(NULL, GetResourceString(IDS_EMULATOR_RUNNING).c_str(), GetResourceString(IDS_EMULATOR).c_str(), MB_ICONWARNING | MB_OK);
        }
        else{
            REGISTRY_FLAGS_ENUM flag = environment::is_64bit_operating_system() ? (environment::is_64bit_process() ? REGISTRY_NONE : REGISTRY_WOW64_64KEY) : REGISTRY_NONE;
            registry reg(flag);
            bool found_installation = false;
            if (reg.open(_T("SOFTWARE\\Saasame\\Transport")) && reg[_T("Path")].exists()){
                boost::filesystem::path install_dir = reg[_T("Path")].wstring();
                boost::filesystem::path launcher_dir = install_dir / "Launcher";
                if (boost::filesystem::exists(launcher_dir)){
                    found_installation = true;
                    boost::filesystem::path emulator = launcher_dir / "emulator";
                    if (boost::filesystem::exists(emulator)){
                        int msgboxID = MessageBox(
                            NULL,
                            GetResourceString(IDS_OVERWRITE).c_str(),
                            GetResourceString(IDS_EMULATOR).c_str(),
                            MB_ICONWARNING | MB_OKCANCEL
                            );
                        if (msgboxID == IDCANCEL){
                            m.unlock();
                            return 2;
                        }
                            
                        try{
                            boost::filesystem::remove_all(emulator);
                        }
                        catch (...){
                            MessageBox(NULL, GetResourceString(IDS_CANNOT_REMOVE).c_str(), GetResourceString(IDS_EMULATOR).c_str(), MB_ICONERROR | MB_OK);
                            dwRet = 2;
                        }
                    }
                    else{
                        int msgboxID = MessageBox(
                            NULL,
                            GetResourceString(IDS_INSTALL).c_str(),
                            GetResourceString(IDS_EMULATOR).c_str(),
                            MB_ICONWARNING | MB_OKCANCEL
                            );
                        if (msgboxID == IDCANCEL){
                            m.unlock();
                            return 2;
                        }
                    }
                    boost::filesystem::create_directory(emulator);
                    std::vector<boost::filesystem::path> extracted_files;
                    if (unzip_ptr->decompress_archive(emulator.string(), extracted_files)){
                        MessageBox(NULL, GetResourceString(IDS_SUCCEEDED).c_str(), GetResourceString(IDS_EMULATOR).c_str(), MB_OK);
                    }
                    else{
                        MessageBox(NULL, GetResourceString(IDS_CANNOT_INSTALL).c_str(), GetResourceString(IDS_EMULATOR).c_str(), MB_ICONERROR | MB_OK);
                        try{
                            boost::filesystem::remove(emulator);
                        }
                        catch (...){
                        }
                        dwRet = 2;
                    }
                }
            }
            if (!found_installation){
                MessageBox(NULL, GetResourceString(IDS_CANNOT_FIND).c_str(), GetResourceString(IDS_EMULATOR).c_str(), MB_ICONWARNING | MB_OK);
                dwRet = 2;
            }
            m.unlock();
        }
    }
    else{
        bool extracted_file = true;
        boost::filesystem::path msi_path = boost::str(boost::wformat(L"%s\\packer.msi") % macho::windows::environment::get_temp_path());
        boost::filesystem::remove(msi_path);
#if HOTFIX_CHECK
        bool verified = false;
#else
        bool verified = true;
#endif
        if (IsWindowsVistaOrGreater()){
#if HOTFIX_CHECK
            if (!IsWindows7OrGreater()){
                macho::windows::environment::auto_disable_wow64_fs_redirection disable_wow64_fs_redirection;
                boost::filesystem::path shlwapi = boost::filesystem::path(macho::windows::environment::get_system_directory()) / "Shlwapi.dll";
                if (boost::filesystem::exists(shlwapi)){
                    macho::windows::file_version_info shlwapi_version = macho::windows::file_version_info::get_file_version_info(shlwapi.wstring());
                    //WORD file_version_major = shlwapi_version.file_version_major();
                    //WORD file_version_minor = shlwapi_version.file_version_minor();
                    //WORD file_version_build = shlwapi_version.file_version_build();
                    WORD file_version_revision = shlwapi_version.file_version_revision();

                    if (22000 > file_version_revision){
                        verified = file_version_revision >= 18738;
                    }
                    else{
                        verified = file_version_revision >= 22983;
                    }
                    if (!verified){
                        MessageBox(NULL, L"Critical Windows hotfix for SHA-2, [Hotfix KB2763674], has not been found in the system. \nPlease install the required hotfix and re-run the installation.", L"Warning!!", MB_ICONWARNING | MB_OK);
                    }
                }
            }
            else if (!IsWindows8OrGreater()){
                macho::windows::environment::auto_disable_wow64_fs_redirection disable_wow64_fs_redirection;
                boost::filesystem::path ntoskrnl = boost::filesystem::path(macho::windows::environment::get_system_directory()) / "Ntoskrnl.exe";
                if (boost::filesystem::exists(ntoskrnl)){
                    macho::windows::file_version_info ntoskrnl_version = macho::windows::file_version_info::get_file_version_info(ntoskrnl.wstring());
                    //WORD file_version_major = ntoskrnl_version.file_version_major();
                    //WORD file_version_minor = ntoskrnl_version.file_version_minor();
                    //WORD file_version_build = ntoskrnl_version.file_version_build();
                    WORD file_version_revision = ntoskrnl_version.file_version_revision();
                    if (22000 > file_version_revision){
                        verified = file_version_revision >= 18741;
                    }
                    else{
                        verified = file_version_revision >= 22948;
                    }
                    if (!verified){
                        MessageBox(NULL, L"Critical Windows hotfix for SHA-2, [Hotfix KB3035131, KB3033929], has not been found in the system. \nPlease install the required hotfix and re-run the installation.", L"Warning!!", MB_ICONWARNING | MB_OK);
                    }
                }
            }
            else{
                verified = true;
            }
#endif
            if (verified){
                if (macho::windows::environment::is_64bit_operating_system()){
                    if (!unzip_ptr->decompress_signal_file("packer64.msi", msi_path.string())){
                        LOG(LOG_LEVEL_ERROR, _T("Can't extract packer64.msi file!"));
                        extracted_file = false;
                        dwRet = 2;
                    }
                }
                else{
                    if (!unzip_ptr->decompress_signal_file("packer32.msi", msi_path.string())){
                        LOG(LOG_LEVEL_ERROR, _T("Can't extract packer32.msi file!"));
                        extracted_file = false;
                        dwRet = 2;
                    }
                }
            }
            else{
                extracted_file = false;
                dwRet = 2;
            }
        }
        else if (IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WS03), LOBYTE(_WIN32_WINNT_WS03), 0))
        {
#if HOTFIX_CHECK
            {
                WORD crypt32_file_version_revision = 0;
                WORD wcrypt32_file_version_revision = 0;
                macho::windows::environment::auto_disable_wow64_fs_redirection disable_wow64_fs_redirection;
                boost::filesystem::path crypt32 = boost::filesystem::path(macho::windows::environment::get_system_directory()) / "Crypt32.dll";
                if (boost::filesystem::exists(crypt32)){
                    macho::windows::file_version_info crypt32_version = macho::windows::file_version_info::get_file_version_info(crypt32.wstring());
                    crypt32_file_version_revision = crypt32_version.file_version_revision();
                }

                verified = crypt32_file_version_revision >= 4477;
                /*if (macho::windows::environment::is_64bit_operating_system()){
                    boost::filesystem::path wcrypt32 = boost::filesystem::path(macho::windows::environment::get_system_directory()) / "Wcrypt32.dll";
                    if (boost::filesystem::exists(wcrypt32)){
                        macho::windows::file_version_info wcrypt32_version = macho::windows::file_version_info::get_file_version_info(wcrypt32.wstring());
                        wcrypt32_file_version_revision = wcrypt32_version.file_version_revision();
                    }
                    verified = crypt32_file_version_revision >= 4477 && wcrypt32_file_version_revision >= 4477;
                }
                else{
                    verified = crypt32_file_version_revision >= 4477;
                }*/
                    
                if (!verified){
                    MessageBox(NULL, L"Critical Windows hotfix for SHA-2, [SP2 + Hotfix KB938397, KB968730], has not been found in the system. \nPlease install the required hotfix and re-run the installation.", L"Warning!!", MB_ICONWARNING | MB_OK);
                }
            }
#endif
            if (verified){
                if (macho::windows::environment::is_64bit_operating_system()){
                    if (!unzip_ptr->decompress_signal_file("packer64_2k3.msi", msi_path.string())){
                        LOG(LOG_LEVEL_ERROR, _T("Can't extract packer64_2k3.msi file!"));
                        extracted_file = false;
                        dwRet = 2;
                    }
                }
                else{
                    if (!unzip_ptr->decompress_signal_file("packer32_2k3.msi", msi_path.string())){
                        LOG(LOG_LEVEL_ERROR, _T("Can't extract packer32_2k3.msi file!"));
                        extracted_file = false;
                        dwRet = 2;
                    }
                }
            }
            else{
                extracted_file = false;
                dwRet = 2;
            }
        }

        if (extracted_file){
            std::wstring commandLine = GetCommandLine();
            std::wstring parameter;
            std::size_t found = commandLine.find(exepath.filename().wstring());
            if (found != std::string::npos){
                parameter = commandLine.substr(found + exepath.filename().wstring().length(), -1);
                if (parameter.length() && parameter[0] == L'\"')
                    parameter = parameter.substr(1, -1);
            }
            std::wstring ret;
            std::wstring batch_cmd = boost::str(boost::wformat(L"\"%s\\msiexec.exe\" /i \"%s\" %s") % macho::windows::environment::get_system_directory() % msi_path.wstring() % parameter);
            LOG(LOG_LEVEL_RECORD, _T("Execute the setup command file (%s)!"), batch_cmd.c_str());
            macho::windows::process::exec_console_application_with_timeout(batch_cmd, ret);
        }
        boost::filesystem::remove(msi_path);
    }
    return dwRet;
}

