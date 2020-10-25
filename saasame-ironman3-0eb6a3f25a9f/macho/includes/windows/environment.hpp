// environment.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_ENVIRONMENT__
#define __MACHO_WINDOWS_ENVIRONMENT__

#include "..\config\config.hpp"
#include "boost\format.hpp"
#include <iostream>
#include "boost\filesystem.hpp"
#define SECURITY_WIN32 1
#include <Security.h>
#include <lmcons.h>
#include <ntsecapi.h>
#include <lm.h>
#include <memory>
#include <Shlwapi.h>
#include "file_version_info.hpp"

#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace macho{

namespace windows{

struct operating_system {

    operating_system() : type(PRODUCT_UNDEFINED) {
        ZeroMemory(&version_info, sizeof(OSVERSIONINFOEX));
        ZeroMemory(&system_info, sizeof(SYSTEM_INFO));
    }
    
    virtual void copy ( const operating_system& os ) { 
        type = os.type;
        name = os.name;
        memcpy( &version_info, &os.version_info, sizeof(OSVERSIONINFOEX));
        memcpy( &system_info, &os.system_info, sizeof(SYSTEM_INFO));
    }
    
    virtual const operating_system &operator =( const operating_system& os ) {
        if ( this != &os )
            copy( os );
        return( *this );
    }
    inline WORD  suite_mask()           const { return version_info.wSuiteMask; }
    inline BYTE  product_type()         const { return version_info.wProductType; }
    inline DWORD major_version()        const { return version_info.dwMajorVersion; };
    inline DWORD minor_version()        const { return version_info.dwMinorVersion; };
    inline DWORD build_number()         const { return version_info.dwBuildNumber; };
    inline WORD  servicepack_major()    const { return version_info.wServicePackMajor ; };
    inline WORD  servicepack_minor()    const { return version_info.wServicePackMinor ; };

    // Returns a pointer to a null-terminated string that provides arbitrary additional information about the operating system, such as the Service Pack installed
    inline stdstring csd_version() const { return stdstring(version_info.szCSDVersion); };
    // Various OS methods
    inline bool is_win2000()    const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 5) && (version_info.dwMinorVersion == 0)) ? true : false; } // Is this Windows 2000 ?
    inline bool is_winxp()      const { return ( (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 5) && ( (version_info.dwMinorVersion == 1) || (version_info.dwMinorVersion == 2) ) && ( version_info.wProductType == VER_NT_WORKSTATION ) ) ? true : false; }; // Is this Windows XP ?
    inline bool is_win2003()    const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 5) && (version_info.dwMinorVersion == 2) && ( version_info.wProductType != VER_NT_WORKSTATION ) && ( GetSystemMetrics(SM_SERVERR2) == 0 ) ) ? true : false; }; // Is this Windows Server 2003 ?
    inline bool is_win2003r2()  const { return ( (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 5) && (version_info.dwMinorVersion == 2) && ( version_info.wProductType != VER_NT_WORKSTATION ) && ( GetSystemMetrics(SM_SERVERR2) != 0 ) ) ? true : false; }; // Is this Windows Server 2003 R2?
    inline bool is_winvista()   const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 6 ) && (version_info.dwMinorVersion == 0 ) && ( version_info.wProductType == VER_NT_WORKSTATION ) ) ? true : false; }; // Is this Windows vista ?
    inline bool is_win2008()    const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 6 ) && (version_info.dwMinorVersion == 0 ) && ( version_info.wProductType != VER_NT_WORKSTATION ) ) ? true : false; }; // Is this Windows Server 2008 ?
    inline bool is_win2008r2()  const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 6) && (version_info.dwMinorVersion == 1) && ( version_info.wProductType != VER_NT_WORKSTATION ) ) ? true : false; }; // Is this Windows Server 2008 R2 ?
    inline bool is_win7()       const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion == 6) && (version_info.dwMinorVersion == 1) && ( version_info.wProductType == VER_NT_WORKSTATION ) ) ? true : false; }; // Is this Windows 7 ?
    
    inline bool is_workstation() const { return ( version_info.wProductType == VER_NT_WORKSTATION ) ? true : false; }
    inline bool is_server()      const { return ( version_info.wProductType != VER_NT_WORKSTATION ) ? true : false; }
    inline bool is_domain_controller() const { return ( version_info.wProductType == VER_NT_DOMAIN_CONTROLLER) ? true : false; }

    inline bool is_win2003_or_later()  const { return ( (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && ( (version_info.dwMajorVersion >= 6 ) || ( (version_info.dwMajorVersion >= 5) && (version_info.dwMinorVersion >= 2) ) ) ) ? true : false; }
    inline bool is_winxp_or_later()    const { return ( (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && ( (version_info.dwMajorVersion >= 6 ) || ( (version_info.dwMajorVersion >= 5) && (version_info.dwMinorVersion >= 1) ) ) ) ? true : false; }
    inline bool is_winvista_or_later() const { return ( (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion >= 6) ) ? true : false; }
    inline bool is_win8_or_later()     const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (((version_info.dwMajorVersion == 6) && (version_info.dwMinorVersion >= 2)) || (version_info.dwMajorVersion > 6))) ? true : false; }
    inline bool is_win8_1_or_later()   const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (((version_info.dwMajorVersion == 6) && (version_info.dwMinorVersion >= 3)) || (version_info.dwMajorVersion > 6))) ? true : false; }
    inline bool is_win10_or_later()    const { return ((version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (version_info.dwMajorVersion >= 10)) ? true : false; }

    // Processor architectures
    inline bool is_amd64() const { return ( system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) ? true : false; };
    inline bool is_ia64()  const { return ( system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) ? true : false; };
    inline bool is_x86()   const { return ( system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) ? true : false; };
    stdstring   sz_cpu_architecture() {
        stdstring architecture;
        switch( system_info.wProcessorArchitecture ){
            case PROCESSOR_ARCHITECTURE_AMD64: 
                architecture = _T("amd64");
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
                architecture = _T("ia64");
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                architecture = _T("x86");
                break;
        } 
        return architecture;
    }

    // Produce Type
    inline bool is_home_edition() const { return ( ( version_info.wSuiteMask & VER_SUITE_PERSONAL ) || ( type == PRODUCT_HOME_PREMIUM ) ||  ( type == PRODUCT_HOME_BASIC ) ) ? true : false;}

    OSVERSIONINFOEX  version_info;
    SYSTEM_INFO      system_info;
    DWORD            type;
    stdstring        name;
};

class environment{
public:
    environment(){}
    virtual ~environment(){}
    static operating_system get_os_version();
    static stdstring        get_command_line() { return GetCommandLine(); }
    static stdstring        get_computer_name();
    static stdstring        get_computer_sid();
    static stdstring        get_computer_name_ex( COMPUTER_NAME_FORMAT format = ComputerNameNetBIOS );
    static stdstring        get_user_name();
    static stdstring        get_user_name_ex( EXTENDED_NAME_FORMAT format = NameSamCompatible );
    static stdstring        get_system_directory();
    static stdstring        get_windows_directory();
    static stdstring        get_working_directory();
    static stdstring        get_execution_filename();
    static stdstring        get_execution_full_path();
    static stdstring        get_environment_variable( stdstring name );
    static stdstring        get_expand_environment_strings( stdstring src );
    static stdstring        get_path_unexpand_environment_strings( stdstring src );
    static stdstring        get_temp_path();
    static stdstring        create_temp_file( stdstring prefix = _T("tmp"), stdstring parent_path = get_temp_path() );
    static stdstring        create_temp_folder( stdstring prefix = _T("tmp"), stdstring parent_path = get_temp_path() );
    static bool             is_64bit_process();
    static bool             is_64bit_operating_system();
    static bool             is_wow64_process();
    static stdstring        get_workgroup_name();
    static stdstring        get_domain_name();
    static stdstring        get_full_domain_name();
    static bool             is_domain_member();
    static bool             is_safe_boot();
    static bool             is_running_as_administrator();
    static bool             is_running_as_local_system();
    static bool             is_winpe();
    static bool             disable_wow64_fs_redirection( PVOID* old_value );
    static bool             revert_wow64_fs_redirection( PVOID old_value );
    static bool             set_token_privilege(stdstring privilege, bool is_enable_privilege);
    static stdstring        get_system_message(int message_id);
    static bool             shutdown_system(bool is_reboot = false, stdstring message = stdstring(), int wait_time = 15);
    static std::vector<boost::filesystem::path>  get_sub_folders(boost::filesystem::path folder, bool is_include_sub_folder = false);
    static std::vector<boost::filesystem::path>  get_files(boost::filesystem::path folder, stdstring filter = _T("*"), bool is_include_sub_folder = false);
    static std::wstring                          get_volume_path_name(const boost::filesystem::path p);
    static std::wstring                          get_volume_name_for_volume_mount_point(const std::wstring p);
    static FILETIME                              ptime_to_file_time(boost::posix_time::ptime const &pt);

    class auto_disable_wow64_fs_redirection {
    public :
        auto_disable_wow64_fs_redirection() : _is_disable_wow64(false), wow64(NULL){
            if ( is_wow64_process() )
                _is_disable_wow64 = disable_wow64_fs_redirection( &wow64 );
        }
        virtual ~auto_disable_wow64_fs_redirection(){
            if ( _is_disable_wow64 ) revert_wow64_fs_redirection( wow64 );
        }
    private :
        PVOID wow64;
        bool  _is_disable_wow64;
    };

    class log_on_user_and_impersonated {
    public:
        struct  exception : virtual public exception_base {};
        //You can generate a LocalService token by using the following code.
        //LogonUser(L"LocalServer", L"NT AUTHORITY", NULL, LOGON32_LOGON_SERVICE, LOGON32_PROVIDER_DEFAULT, &hToken)
        log_on_user_and_impersonated(stdstring user, stdstring password, stdstring domain = _T(""), unsigned int logon_type = LOGON32_LOGON_SERVICE, unsigned int logon_provider = LOGON32_PROVIDER_DEFAULT){
            if (!LogonUser(user.c_str(), domain.length() ? domain.c_str() : NULL, password.length() ? password.c_str() : NULL, logon_type, logon_provider, &_token)){
#if _UNICODE  
                BOOST_THROW_EXCEPTION_BASE(exception, GetLastError(), boost::str(boost::wformat(L"LogonUser(%s) error.") % user));
#else
                BOOST_THROW_EXCEPTION_BASE(exception, GetLastError(), boost::str(boost::format("LogonUser(%s) error.") % user));
#endif
            }
            else{
                if(!ImpersonateLoggedOnUser(_token))
                    BOOST_THROW_EXCEPTION_BASE(exception, GetLastError(), _T("ImpersonateLoggedOnUser() error."));
            }
        }
        virtual ~log_on_user_and_impersonated(){
            RevertToSelf();
            CloseHandle(_token);
        }
    private:
        HANDLE _token;
    };
private :
};

#ifndef MACHO_HEADER_ONLY

#include <sddl.h>               /* for ConvertSidToStringSid function */

stdstring environment::get_temp_path(){
    TCHAR path[MAX_PATH+1] = {0};
    if ( GetTempPath( MAX_PATH+1, path ) )
#if _UNICODE
        return boost::filesystem::path( path ).parent_path().wstring();
#else
        return boost::filesystem::path( path ).parent_path().string();
#endif
    return _T("");
}

stdstring environment::create_temp_file( stdstring prefix, stdstring parent_path ){
    TCHAR temp_file[MAX_PATH+1] = {0};
    if ( ( parent_path.length() > 0 ) &&
        ( parent_path.length() < (MAX_PATH-14) ) &&
        GetTempFileName( parent_path.c_str(), prefix.c_str(), 0, temp_file ) )
        return temp_file;  
    return _T("");
}

stdstring environment::create_temp_folder( stdstring prefix, stdstring parent_path ){
    stdstring path = create_temp_file( prefix, parent_path );
    if ( path.length() > 0 && DeleteFile( path.c_str() ) && CreateDirectory( path.c_str(), NULL ) )
        return path;
    return _T("");
}

stdstring environment::get_computer_name(){
    TCHAR machine_name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1 ;
    if ( GetComputerName( machine_name, &size ) )
        return machine_name;
    return _T("");    
}

stdstring environment::get_computer_sid(){
    stdstring sid;
    LPUSER_INFO_0 pBuf = NULL;
    LPUSER_INFO_0 pTmpBuf;
    DWORD dwLevel = 0;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;
    DWORD i;
    DWORD dwTotalCount = 0;
    NET_API_STATUS nStatus;
    LPTSTR pszServerName = NULL;
    //   
    // Call the NetUserEnum function, specifying level 0;    
    //   enumerate global user account types only.   
    //   
    do // begin do   
    {
        nStatus = NetUserEnum(pszServerName,
            dwLevel,
            FILTER_NORMAL_ACCOUNT, // global users   
            (LPBYTE*)&pBuf,
            dwPrefMaxLen,
            &dwEntriesRead,
            &dwTotalEntries,
            &dwResumeHandle);
        //   
        // If the call succeeds,   
        //   
        if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA)){
            if ((pTmpBuf = pBuf) != NULL){
                //   
                // Loop through the entries.   
                //   
                for (i = 0; (i < dwEntriesRead); i++){
                    assert(pTmpBuf != NULL);

                    if (pTmpBuf == NULL){
                        LOG(LOG_LEVEL_ERROR, _T("An access violation has occurred"));
                        break;
                    }
                    //   
                    //  Print the name of the user account.   
                    //
                    if (sid.empty()){
                        LPUSER_INFO_4 pBuf = NULL;
                        NET_API_STATUS nStatus2;
                        wprintf(L"\t-- %s\n", pTmpBuf->usri0_name);
                        nStatus2 = NetUserGetInfo(pszServerName, pTmpBuf->usri0_name, 4, (LPBYTE *)& pBuf);
                        if (nStatus2 == NERR_Success){
                            if (pBuf != NULL){
                                LPTSTR sStringSid = NULL;
                                if (ConvertSidToStringSid
                                    (pBuf->usri4_user_sid, &sStringSid)){
                                    wprintf(L"\tUser SID: %s\n", sStringSid);
                                    if (0 == _tcsncicmp(sStringSid, _T("S-1-5-"), 6)){
                                        sid = stdstring(sStringSid).substr(0, 41);
                                    }
                                    LocalFree(sStringSid);
                                }
                                NetApiBufferFree(pBuf);
                            }
                        }
                    }
                    else{
                        break;
                    }
                    pTmpBuf++;
                    dwTotalCount++;
                }
            }
        }
        //   
        // Otherwise, print the system error.   
        //   
        else{
            LOG(LOG_LEVEL_ERROR, _T("A system error has occurred: %d"), nStatus);
        }
        //   
        // Free the allocated buffer.   
        //   
        if (pBuf != NULL){
            NetApiBufferFree(pBuf);
            pBuf = NULL;
        }
    }
    // Continue to call NetUserEnum while    
    //  there are more entries.    
    //    
    while (sid.empty() && nStatus == ERROR_MORE_DATA); // end do   
    //   
    // Check again for allocated memory.   
    //   
    if (pBuf != NULL)
        NetApiBufferFree(pBuf);

    return sid;
}

stdstring environment::get_computer_name_ex( COMPUTER_NAME_FORMAT format ){
    DWORD size = 0;
    GetComputerNameEx( format, NULL, &size );
    if ( size > 0 ){
        std::auto_ptr<TCHAR> p_name = std::auto_ptr<TCHAR>(new TCHAR[size]);
        memset( p_name.get(), 0, size * sizeof(TCHAR) );
        if ( GetComputerNameEx( format, p_name.get(), &size ) )
            return p_name.get();
    }
    return _T("");    
}

stdstring environment::get_user_name(){
    std::auto_ptr<TCHAR> p_name( new TCHAR[UNLEN + 1] );
    DWORD size = UNLEN + 1 ;
    memset( p_name.get(), 0, size * sizeof(TCHAR) );
    if ( GetUserName( p_name.get(), &size ) )
        return p_name.get();
    return _T("");
}

stdstring environment::get_user_name_ex( EXTENDED_NAME_FORMAT format ){
    DWORD size = 0;
    GetUserNameEx( format, NULL, &size ) ;
    if ( size > 0 ){
        std::auto_ptr<TCHAR> p_name = std::auto_ptr<TCHAR>(new TCHAR[size]);
        memset( p_name.get(), 0, size * sizeof(TCHAR) );
        if ( GetUserNameEx( format, p_name.get(), &size ) )
            return p_name.get();
    }
    return _T("");
}

stdstring environment::get_system_directory(){
    DWORD size = GetSystemDirectory( NULL, 0 );
    std::auto_ptr<TCHAR> p_path = std::auto_ptr<TCHAR>(new TCHAR[size]);
    memset( p_path.get(), 0, size * sizeof(TCHAR) );
    if ( GetSystemDirectory( p_path.get(), size ) )
        return p_path.get();
    return _T("");
}

stdstring environment::get_windows_directory(){
    DWORD size = GetWindowsDirectory( NULL, 0 );
    std::auto_ptr<TCHAR>  p_path = std::auto_ptr<TCHAR>(new TCHAR[size]);
    memset( p_path.get(), 0, size * sizeof(TCHAR) );
    if ( GetWindowsDirectory( p_path.get(), size ) )
        return p_path.get();
    return _T("");
}

stdstring environment::get_execution_full_path(){
    std::auto_ptr<TCHAR> p_path;
    DWORD                size = MAX_PATH;
    do{ 
        p_path = std::auto_ptr<TCHAR>(new TCHAR[size]);
        memset( p_path.get(), 0, size * sizeof(TCHAR) );
        size = GetModuleFileName( NULL, p_path.get(), size );
    }while( ERROR_INSUFFICIENT_BUFFER == GetLastError() );
    
    if ( size > 0 ){
        return p_path.get();
    }
    return _T("");
}

stdstring environment::get_working_directory(){
    stdstring            directory;
    boost::filesystem::path p = get_execution_full_path();
    if ( p.has_parent_path() ){
#if _UNICODE
        directory = p.parent_path().wstring();
#else
        directory = p.parent_path().string();
#endif
    }
    return directory;
}

stdstring environment::get_execution_filename(){
    stdstring            filename;
    boost::filesystem::path p = get_execution_full_path();;
    if ( p.has_filename() ){
#if _UNICODE
        filename = p.filename().wstring();
#else
        filename = p.filename().string();
#endif
    }
    return filename;
}

stdstring environment::get_environment_variable( stdstring name ){
    DWORD size = GetEnvironmentVariable( name.c_str(), NULL, 0 );
    if ( size > 0 ){
        std::auto_ptr<TCHAR> p_value = std::auto_ptr<TCHAR> ( new TCHAR[ size ] );
        memset( p_value.get(), 0, size * sizeof(TCHAR) );
        if ( GetEnvironmentVariable( name.c_str(), p_value.get(), size ) )
            return p_value.get();
    }
    return _T("");
}

stdstring environment::get_expand_environment_strings( stdstring src ){
    DWORD size = ExpandEnvironmentStrings( src.c_str(), NULL, 0 );
    if ( size > 0 ){
        std::auto_ptr<TCHAR> p_value = std::auto_ptr<TCHAR> ( new TCHAR[ size ] );
        memset( p_value.get(), 0, size * sizeof(TCHAR) );
        if ( ExpandEnvironmentStrings( src.c_str(), p_value.get(), size ) )
            return p_value.get();
    }
    return _T("");
}

stdstring environment::get_path_unexpand_environment_strings( stdstring src ){

    std::auto_ptr<TCHAR> p_value = std::auto_ptr<TCHAR> ( new TCHAR[ MAX_PATH ] );
    memset( p_value.get(), 0, MAX_PATH * sizeof(TCHAR) );
    if ( PathUnExpandEnvStrings( src.c_str(), p_value.get(), MAX_PATH ) )
        return p_value.get();

    return _T("");
}

operating_system environment::get_os_version(){
    operating_system os;
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
    os.version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&os.version_info); 
    PGNSI pGNSI = (PGNSI) GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
    if(NULL != pGNSI) pGNSI(&os.system_info);
    else GetSystemInfo(&os.system_info);
    PGPI pGPI = (PGPI) GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
    if( NULL != pGPI ) pGPI( os.version_info.dwMajorVersion, os.version_info.dwMinorVersion, 0, 0, &os.type);

    if ( VER_PLATFORM_WIN32_NT==os.version_info.dwPlatformId && os.version_info.dwMajorVersion > 4 ){
        os.name = TEXT("Microsoft ");
        // Test for the specific product.
        if ( os.version_info.dwMajorVersion == 6 ){
            if( os.version_info.dwMinorVersion == 0 ){
                if( os.version_info.wProductType == VER_NT_WORKSTATION )
                    os.name.append(TEXT("Windows Vista "));
                else os.name.append(TEXT("Windows Server 2008 " ));
            }
            else if ( os.version_info.dwMinorVersion == 1 ){
                if( os.version_info.wProductType == VER_NT_WORKSTATION )
                    os.name.append(TEXT("Windows 7 "));
                else os.name.append(TEXT("Windows Server 2008 R2 " ));
            }
            else if ( os.version_info.dwMinorVersion == 2 ){
                file_version_info info = file_version_info::get_file_version_info(get_system_directory() + _T("\\kernel32.dll"));
                os.version_info.dwMajorVersion = static_cast<DWORD>(info.product_version_major());
                os.version_info.dwMinorVersion = static_cast<DWORD>(info.product_version_minor());
                os.version_info.dwBuildNumber = static_cast<DWORD>(info.product_version_build());
                if (os.version_info.dwMajorVersion == 6){
                    if (os.version_info.dwMinorVersion == 2){
                        if (os.version_info.wProductType == VER_NT_WORKSTATION)
                            os.name.append(TEXT("Windows 8 "));
                        else os.name.append(TEXT("Windows Server 2012 "));
                    }
                    else if (os.version_info.dwMinorVersion == 3){
                        if (os.version_info.wProductType == VER_NT_WORKSTATION)
                            os.name.append(TEXT("Windows 8.1 "));
                        else os.name.append(TEXT("Windows Server 2012 R2 "));
                    }
                }
                else if (os.version_info.dwMajorVersion == 10){
                    if (os.version_info.dwMinorVersion == 0){
                        if (os.version_info.wProductType == VER_NT_WORKSTATION)
                            os.name.append(TEXT("Windows 10 "));
                        else os.name.append(TEXT("Windows Server 2016 "));
                    }
                    else os.name.append(TEXT("UNKNOWN "));
                }
            }
            switch( os.type ) {
            case PRODUCT_ULTIMATE:
                os.name.append(TEXT("Ultimate Edition" ));
                break;
            case PRODUCT_PROFESSIONAL:
                os.name.append(TEXT("Professional" ));
                break;
            case PRODUCT_HOME_PREMIUM:
                os.name.append(TEXT("Home Premium Edition" ));
                break;
            case PRODUCT_HOME_BASIC:
                os.name.append(TEXT("Home Basic Edition" ));
                break;
            case PRODUCT_ENTERPRISE:
                os.name.append(TEXT("Enterprise Edition" ));
                break;
            case PRODUCT_BUSINESS:
                os.name.append(TEXT("Business Edition" ));
                break;
            case PRODUCT_STARTER:
                os.name.append(TEXT("Starter Edition" ));
                break;
            case PRODUCT_CLUSTER_SERVER:
                os.name.append(TEXT("Cluster Server Edition" ));
                break;
            case PRODUCT_DATACENTER_SERVER:
                os.name.append(TEXT("Datacenter Edition" ));
                break;
            case PRODUCT_DATACENTER_SERVER_CORE:
                os.name.append(TEXT("Datacenter Edition (core installation)" ));
                break;
            case PRODUCT_ENTERPRISE_SERVER:
                os.name.append(TEXT("Enterprise Edition" ));
                break;
            case PRODUCT_ENTERPRISE_SERVER_CORE:
                os.name.append(TEXT("Enterprise Edition (core installation)" ));
                break;
            case PRODUCT_ENTERPRISE_SERVER_IA64:
                os.name.append(TEXT("Enterprise Edition for Itanium-based Systems" ));
                break;
            case PRODUCT_SMALLBUSINESS_SERVER:
                os.name.append(TEXT("Small Business Server" ));
                break;
            case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                os.name.append(TEXT("Small Business Server Premium Edition" ));
                break;
            case PRODUCT_STANDARD_SERVER:
                os.name.append(TEXT("Standard Edition" ));
                break;
            case PRODUCT_STANDARD_SERVER_CORE:
                os.name.append(TEXT("Standard Edition (core installation)" ));
                break;
            case PRODUCT_WEB_SERVER:
                os.name.append(TEXT("Web Server Edition" ));
                break;
            }
        }

        if ( os.version_info.dwMajorVersion == 5 && os.version_info.dwMinorVersion == 2 ){
            if( GetSystemMetrics(SM_SERVERR2) )
                os.name.append(TEXT( "Windows Server 2003 R2, "));
            else if ( os.version_info.wSuiteMask & VER_SUITE_STORAGE_SERVER )
                os.name.append(TEXT( "Windows Storage Server 2003"));
            else if ( os.version_info.wSuiteMask & VER_SUITE_WH_SERVER )
                os.name.append(TEXT( "Windows Home Server"));
            else if( os.version_info.wProductType == VER_NT_WORKSTATION && 
                os.system_info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64){
                os.name.append(TEXT( "Windows XP Professional x64 Edition"));
            }
            else os.name.append(TEXT("Windows Server 2003, "));

            // Test for the server type.
            if ( os.version_info.wProductType != VER_NT_WORKSTATION ){
                if ( os.system_info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 ){
                    if( os.version_info.wSuiteMask & VER_SUITE_DATACENTER )
                        os.name.append(TEXT( "Datacenter Edition for Itanium-based Systems" ));
                    else if( os.version_info.wSuiteMask & VER_SUITE_ENTERPRISE )
                        os.name.append(TEXT( "Enterprise Edition for Itanium-based Systems" ));
                }

                else if ( os.system_info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 ){
                    if( os.version_info.wSuiteMask & VER_SUITE_DATACENTER )
                        os.name.append(TEXT( "Datacenter x64 Edition" ));
                    else if( os.version_info.wSuiteMask & VER_SUITE_ENTERPRISE )
                        os.name.append(TEXT( "Enterprise x64 Edition" ));
                    else os.name.append(TEXT( "Standard x64 Edition" ));
                }
                else{
                    if ( os.version_info.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
                        os.name.append(TEXT( "Compute Cluster Edition" ));
                    else if( os.version_info.wSuiteMask & VER_SUITE_DATACENTER )
                        os.name.append(TEXT( "Datacenter Edition" ));
                    else if( os.version_info.wSuiteMask & VER_SUITE_ENTERPRISE )
                        os.name.append(TEXT( "Enterprise Edition" ));
                    else if ( os.version_info.wSuiteMask & VER_SUITE_BLADE )
                        os.name.append(TEXT( "Web Edition" ));
                    else os.name.append(TEXT( "Standard Edition" ));
                }
            }
        }

        if ( os.version_info.dwMajorVersion == 5 && os.version_info.dwMinorVersion == 1 ){
            os.name.append(TEXT("Windows XP "));
            if( os.version_info.wSuiteMask & VER_SUITE_PERSONAL )
            os.name.append(TEXT( "Home Edition" ));
            else os.name.append(TEXT( "Professional" ));
        }

        if ( os.version_info.dwMajorVersion == 5 && os.version_info.dwMinorVersion == 0 ){
            os.name.append(TEXT("Windows 2000 "));

            if ( os.version_info.wProductType == VER_NT_WORKSTATION ){
                os.name.append(TEXT( "Professional" ));
            }
            else {
                if( os.version_info.wSuiteMask & VER_SUITE_DATACENTER )
                    os.name.append(TEXT( "Datacenter Server" ));
                else if( os.version_info.wSuiteMask & VER_SUITE_ENTERPRISE )
                    os.name.append(TEXT( "Advanced Server" ));
                else os.name.append(TEXT( "Server" ));
            }
        }

        // Include service pack (if any) and build number.
        if( _tcslen(os.version_info.szCSDVersion) > 0 ) {
            os.name.append(TEXT(" ") );
            os.name.append(os.version_info.szCSDVersion);
        }

#if    _UNICODE
        os.name = boost::str( boost::wformat(TEXT("%s (build %d)") )%os.name %os.version_info.dwBuildNumber );
#else
        os.name = boost::str( boost::format(TEXT("%s (build %d)") )%os.name %os.version_info.dwBuildNumber );
#endif
        if ( os.version_info.dwMajorVersion >= 6 ){
            if ( os.system_info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
            os.name.append(TEXT( ", 64-bit" ));
            else if (os.system_info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
            os.name.append(TEXT(", 32-bit"));
        }
    }
    return os;
}

bool environment::is_64bit_process(){
    return ( is_64bit_operating_system() && !is_wow64_process() );
}

bool environment::is_64bit_operating_system(){
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    SYSTEM_INFO        system_info;
    PGNSI pGNSI = (PGNSI) GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
    if(NULL != pGNSI) pGNSI(&system_info);
    else GetSystemInfo(&system_info);
    return ( ( system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) || ( system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) );
}

bool environment::is_wow64_process(){
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    BOOL bIsWow64 = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process){
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64)){
        }
    }
    return bIsWow64 ? true : false;
}

stdstring environment::get_workgroup_name(){
    LSA_HANDLE                  policyHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo = NULL;
    NTSTATUS                    status;
    LSA_OBJECT_ATTRIBUTES       ObjectAttributes;
    std::wstring                name;
    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        
    if ( 0 == ( status = LsaOpenPolicy( NULL, &ObjectAttributes, STANDARD_RIGHTS_READ | POLICY_VIEW_LOCAL_INFORMATION, &policyHandle ) ) ){    
        // 
        // You have a handle to the policy object. Now, get the domain information using LsaQueryInformationPolicy.
        // 
        if ( 0 == ( status = LsaQueryInformationPolicy(policyHandle, PolicyPrimaryDomainInformation, (PVOID*)&ppdiDomainInfo) ) ){
            if ( 0 == ppdiDomainInfo->Sid ){
                UINT    nameSize = ppdiDomainInfo->Name.Length + sizeof(WCHAR) ;
                std::auto_ptr<WCHAR> p_name = std::auto_ptr<WCHAR>( new WCHAR[nameSize/sizeof(WCHAR)] );
                if ( p_name.get() ){
                    memset( p_name.get(), 0, nameSize );
                    wcsncpy_s( p_name.get(), nameSize/sizeof(WCHAR), ppdiDomainInfo->Name.Buffer, ppdiDomainInfo->Name.Length/sizeof(WCHAR) );
                    name = p_name.get();
                }
            }
            LsaFreeMemory((LPVOID)ppdiDomainInfo);
        }
        LsaClose(policyHandle);
    }
#if    _UNICODE
    return name;
#else
    return stringutils::convert_unicode_to_ansi( name );
#endif
}

stdstring environment::get_domain_name(){
    LSA_HANDLE                  policyHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo = NULL;
    NTSTATUS                    status;
    LSA_OBJECT_ATTRIBUTES       ObjectAttributes;
    std::wstring                name;
    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        
    if ( 0 == ( status = LsaOpenPolicy( NULL, &ObjectAttributes, STANDARD_RIGHTS_READ | POLICY_VIEW_LOCAL_INFORMATION, &policyHandle ) ) ){    
        // 
        // You have a handle to the policy object. Now, get the domain information using LsaQueryInformationPolicy.
        // 
        if ( 0 == ( status = LsaQueryInformationPolicy(policyHandle, PolicyPrimaryDomainInformation, (PVOID*)&ppdiDomainInfo) ) ){
            if ( ppdiDomainInfo->Sid ){
                UINT    nameSize = ppdiDomainInfo->Name.Length + sizeof(WCHAR) ;
                std::auto_ptr<WCHAR> p_name = std::auto_ptr<WCHAR>( new WCHAR[nameSize/sizeof(WCHAR)] );
                if ( p_name.get() ){
                    memset( p_name.get(), 0, nameSize );
                    wcsncpy_s( p_name.get(), nameSize/sizeof(WCHAR), ppdiDomainInfo->Name.Buffer, ppdiDomainInfo->Name.Length/sizeof(WCHAR) );
                    name = p_name.get();
                }
            }
            LsaFreeMemory((LPVOID)ppdiDomainInfo);
        }
        LsaClose(policyHandle);
    }
#if    _UNICODE
    return name;
#else
    return stringutils::convert_unicode_to_ansi( name );
#endif
}

stdstring environment::get_full_domain_name(){
    stdstring full_domain_name; // get the current full domain name;
    if ( environment::is_domain_member() ){
        stdstring domain_name = environment::get_domain_name();
        stdstring full_computer_name = environment::get_computer_name_ex( ComputerNameDnsFullyQualified );
        size_t pos = stringutils::tolower(full_computer_name).find(stringutils::tolower(domain_name));
        if ( pos != stdstring::npos )
            full_domain_name = full_computer_name.substr( pos, -1 );
    }
    return full_domain_name;
}

bool environment::is_domain_member(){
    LSA_HANDLE                  policyHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo = NULL;
    NTSTATUS                    status;
    LSA_OBJECT_ATTRIBUTES       ObjectAttributes;
    bool                        bIsDomainMember = false;
    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        
    if ( 0 == ( status = LsaOpenPolicy( NULL, &ObjectAttributes, STANDARD_RIGHTS_READ | POLICY_VIEW_LOCAL_INFORMATION, &policyHandle ) ) ){    
        // 
        // You have a handle to the policy object. Now, get the domain information using LsaQueryInformationPolicy.
        // 
        if ( 0 == ( status = LsaQueryInformationPolicy(policyHandle, PolicyPrimaryDomainInformation, (PVOID*)&ppdiDomainInfo) ) ){
            if( ppdiDomainInfo->Sid )
                bIsDomainMember = true;
            LsaFreeMemory((LPVOID)ppdiDomainInfo);
        }
        LsaClose(policyHandle);
    }
    return bIsDomainMember;
}

bool environment::is_safe_boot(){
    DWORD dwBootMode = GetSystemMetrics(SM_CLEANBOOT);
    LOG(LOG_LEVEL_INFO, (TEXT("Boot Mode : %d.\n"), dwBootMode));
    if (dwBootMode > 0)
        return true;
    else
        return false;
}

bool environment::is_running_as_administrator(){
    bool    fAdmin = false;
    HANDLE  hThread;
    TOKEN_GROUPS *ptg = NULL;
    DWORD  cbTokenGroups;
    DWORD  dwGroup;
    PSID   psidAdmin;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    // First we must open a handle to the access token for this thread.
    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread ) ){
        if ( GetLastError() == ERROR_NO_TOKEN ){
            // If the thread does not have an access token, we'll examine the
            // access token associated with the process.

            if ( !OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, &hThread ) )
                return ( false );
        }
        else 
            return ( false );
    }

    // Then we must query the size of the group information associated with
    // the token. Note that we expect a FALSE result from GetTokenInformation
    // because we've given it a NULL buffer. On exit cbTokenGroups will tell
    // the size of the group information.

    if ( GetTokenInformation ( hThread, TokenGroups, NULL, 0, &cbTokenGroups ) )
        return ( false );

    // Here we verify that GetTokenInformation failed for lack of a large
    // enough buffer.

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        return ( false );

    // Now we allocate a buffer for the group information.
    // Since _alloca allocates on the stack, we don't have
    // to explicitly deallocate it. That happens automatically
    // when we exit this function.

    if ( ! ( ptg= ( TOKEN_GROUPS*) _alloca ( cbTokenGroups ) ) ) 
        return ( false );

    // Now we ask for the group information again.
    // This may fail if an administrator has added this account
    // to an additional group between our first call to
    // GetTokenInformation and this one.

    if ( !GetTokenInformation ( hThread, TokenGroups, ptg, cbTokenGroups, &cbTokenGroups ) )
        return ( false );

    // Now we must create a System Identifier for the Admin group.
    if ( ! AllocateAndInitializeSid ( &SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin ) )
        return ( false );

    // Finally we'll iterate through the list of groups for this access
    // token looking for a match against the SID we created above.
    fAdmin= false;

    for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++){
        if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin)){
            fAdmin = true;
            break;
        }
    }
    // Before we exit we must explicity deallocate the SID we created.
    FreeSid ( psidAdmin);
    return ( fAdmin);
}

bool environment::is_running_as_local_system(){

    bool    fLocalSystem = false;
    HANDLE  hThread;
    BYTE    userSid[256]; 
    DWORD   cb;
    PSID    psidLocalSystem;

    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    // First we must open a handle to the access token for this thread.
    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread ) ){
        if ( GetLastError() == ERROR_NO_TOKEN ){
            // If the thread does not have an access token, we'll examine the
            // access token associated with the process.
            if ( !OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, &hThread ) )
                return ( false );
        }
        else 
            return ( false );
    }
    
    cb = sizeof( userSid );

    if ( !GetTokenInformation ( hThread, TokenUser, userSid, cb, &cb ) )
        return ( false );

    TOKEN_USER* ptu = (TOKEN_USER*)userSid;   
    // Now we must create a System Identifier for the Admin group.
    if ( ! AllocateAndInitializeSid( &SystemSidAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psidLocalSystem ) )
        return ( false );

    // Finally we'll check if this access token looking for a match against the SID we created above.
    fLocalSystem = EqualSid( psidLocalSystem, ptu->User.Sid ) ? true : false;
    // Before we exit we must explicity deallocate the SID we created.
    FreeSid ( psidLocalSystem);
    return ( fLocalSystem );
}

bool environment::is_winpe(){

    HKEY hKey;
    LONG result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\WinPE"), 0, KEY_READ, &hKey ) ;
    if ( ERROR_SUCCESS != result ){
        if ( ERROR_FILE_NOT_FOUND == result ) {
            return false;
        } 
        else {
            return true;
        }
    }
    RegCloseKey( hKey );
    return true;
}

bool environment::disable_wow64_fs_redirection( PVOID* old_value ){
    typedef    BOOL (WINAPI *LPFN_Wow64DisableWow64FsRedirection)( PVOID* );
    BOOL IsDisableWow64 = FALSE;
    LPFN_Wow64DisableWow64FsRedirection fnWow64DisableWow64FsRedirection;
    fnWow64DisableWow64FsRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"Wow64DisableWow64FsRedirection");

    if (NULL != fnWow64DisableWow64FsRedirection){
        IsDisableWow64 = fnWow64DisableWow64FsRedirection(old_value);
    }
    return IsDisableWow64 == TRUE;
}

bool environment::revert_wow64_fs_redirection( PVOID old_value ){
    typedef    BOOL (WINAPI *LPFN_Wow64RevertWow64FsRedirection)( PVOID );
    BOOL IsRevertWow64 = FALSE;
    LPFN_Wow64RevertWow64FsRedirection fnWow64RevertWow64FsRedirection;
    fnWow64RevertWow64FsRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"Wow64RevertWow64FsRedirection");

    if (NULL != fnWow64RevertWow64FsRedirection){
        IsRevertWow64 = fnWow64RevertWow64FsRedirection(old_value);
    }
    return IsRevertWow64 == TRUE;
}


bool environment::set_token_privilege(stdstring privilege, bool is_enable_privilege){

    HANDLE           hToken;          // token handle
    TOKEN_PRIVILEGES tp = { 0 };
    LUID             luid;
    TOKEN_PRIVILEGES tpPrevious = { 0 };
    DWORD            cbPrevious = sizeof(TOKEN_PRIVILEGES);
    BOOL             bReslut;

    if (!(bReslut = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))){
        if (GetLastError() == ERROR_NO_TOKEN){
            if (ImpersonateSelf(SecurityImpersonation)){
                bReslut = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);
            }
        }
    }

    if (!bReslut)
        return false;

    if (LookupPrivilegeValue(NULL, privilege.c_str(), &luid)){
        // 
        // first pass.  get current privilege setting
        // 
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = 0;

        AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

        if (GetLastError() == ERROR_SUCCESS){
            // 
            // second pass.  set privilege based on previous setting
            // 
            tpPrevious.PrivilegeCount = 1;
            tpPrevious.Privileges[0].Luid = luid;

            if (is_enable_privilege) {
                tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
            }
            else {
                tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
                    tpPrevious.Privileges[0].Attributes);
            }

            AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tpPrevious,
                cbPrevious,
                NULL,
                NULL
                );

            CloseHandle(hToken);

            if (GetLastError() == ERROR_SUCCESS) return true;
        }
    }
    return false;
}

std::vector<boost::filesystem::path>  environment::get_sub_folders(boost::filesystem::path folder, bool is_include_sub_folder){

    std::vector<boost::filesystem::path> folders;
    boost::filesystem::path              dest_folder;
    boost::filesystem::path              file_name;
    WIN32_FIND_DATAW                     FindFileData;
    HANDLE                               hFind = INVALID_HANDLE_VALUE;
    DWORD                                dwRet = ERROR_SUCCESS;
    ZeroMemory(&FindFileData, sizeof(FindFileData));

    if (boost::filesystem::exists(folder) && boost::filesystem::is_directory(folder)){
        //folder.remove_leaf();
        file_name = folder / _T("*");
#if _UNICODE
        hFind = FindFirstFile(file_name.wstring().c_str(), &FindFileData);
#else
        hFind = FindFirstFile(file_name.string().c_str(), &FindFileData);
#endif
        if (hFind == INVALID_HANDLE_VALUE){
            dwRet = GetLastError();
        }
        else{
            do{
                DWORD dwRes = _tcscmp(_T("."), FindFileData.cFileName);
                DWORD dwRes1 = _tcscmp(_T(".."), FindFileData.cFileName);
                if (0 == dwRes || 0 == dwRes1)
                    continue;
                dest_folder = folder / FindFileData.cFileName;
                if (FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes){
                    if (!(FILE_ATTRIBUTE_REPARSE_POINT & FindFileData.dwFileAttributes)){
                        folders.push_back(dest_folder);
                        if (is_include_sub_folder){
                            std::vector<boost::filesystem::path> sub_folders = get_sub_folders(dest_folder, is_include_sub_folder);
                            folders.insert(folders.end(), sub_folders.begin(), sub_folders.end());
                        }
                    }
                }
            } while (FindNextFile(hFind, &FindFileData) != 0);
            FindClose(hFind);
        }
    }
    return folders;
}

std::vector<boost::filesystem::path> environment::get_files(boost::filesystem::path folder, stdstring filter, bool is_include_sub_folder){

    std::vector<boost::filesystem::path> files;
    boost::filesystem::path              dest_file;
    boost::filesystem::path              file_name;
    WIN32_FIND_DATAW                     FindFileData;
    HANDLE                               hFind = INVALID_HANDLE_VALUE;
    DWORD                                dwRet = ERROR_SUCCESS;
    ZeroMemory(&FindFileData, sizeof(FindFileData));

    if (boost::filesystem::exists(folder) && boost::filesystem::is_directory(folder)){
        //folder.remove_leaf();
        file_name = folder / filter;
#if _UNICODE
        hFind = FindFirstFile(file_name.wstring().c_str(), &FindFileData);
#else
        hFind = FindFirstFile(file_name.string().c_str(), &FindFileData);
#endif
        if (hFind == INVALID_HANDLE_VALUE){
            dwRet = GetLastError();
        }
        else{
            do{
                DWORD dwRes = _tcscmp(_T("."), FindFileData.cFileName);
                DWORD dwRes1 = _tcscmp(_T(".."), FindFileData.cFileName);
                if (0 == dwRes || 0 == dwRes1)
                    continue;
                dest_file = folder / FindFileData.cFileName;
                if (FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes){
                }
                else{
                    files.push_back(dest_file);
                }
            } while (FindNextFile(hFind, &FindFileData) != 0);
            FindClose(hFind);
        }
    }
    if (is_include_sub_folder){
        std::vector<boost::filesystem::path> sub_dirs = get_sub_folders(folder);
        foreach(boost::filesystem::path &sub_dir, sub_dirs){
            std::vector<boost::filesystem::path> sub_files = get_files(sub_dir, filter, is_include_sub_folder);
            files.insert(files.end(), sub_files.begin(), sub_files.end());
        }
    }
    return files;
}

std::wstring environment::get_volume_path_name(const boost::filesystem::path p){
    TCHAR _volume_path_name[MAX_PATH + 1] = { 0 };
    if (!GetVolumePathNameW(p.wstring().c_str(), _volume_path_name, MAX_PATH)){
        LOG(LOG_LEVEL_ERROR, L"GetVolumePathNameW(%s) error. (%d)", p.wstring().c_str(), GetLastError());
    }
    return _volume_path_name;
}

std::wstring environment::get_volume_name_for_volume_mount_point(const std::wstring p){
    TCHAR _volume_name[MAX_PATH + 1] = { 0 };
    if (!GetVolumeNameForVolumeMountPointW(std::wstring(p + (p[p.length() - 1] == L'\\' ? L"" : L"\\")).c_str(), _volume_name, MAX_PATH)) {
        LOG(LOG_LEVEL_ERROR, L"GetVolumeNameForVolumeMountPointW(%s) error. (%d)", p.c_str(), GetLastError());
    }
    return _volume_name;
}

FILETIME environment::ptime_to_file_time(boost::posix_time::ptime const &pt){

    // extract the date from boost::posix_time to SYSTEMTIME
    SYSTEMTIME st;
    boost::gregorian::date::ymd_type ymd = pt.date().year_month_day();

    st.wYear = ymd.year;
    st.wMonth = ymd.month;
    st.wDay = ymd.day;
    st.wDayOfWeek = pt.date().day_of_week();

    // Now extract the hour/min/second field from time_duration
    boost::posix_time::time_duration td = pt.time_of_day();
    st.wHour = static_cast<WORD>(td.hours());
    st.wMinute = static_cast<WORD>(td.minutes());
    st.wSecond = static_cast<WORD>(td.seconds());

    // Although ptime has a fractional second field, SYSTEMTIME millisecond
    // field is 16 bit, and will not store microsecond. We will treat this
    // field separately later.
    st.wMilliseconds = 0;

    // Convert SYSTEMTIME to FILETIME structure
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);

    // Now we are almost done. The FILETIME has date, and time. It is
    // only missing fractional second.

    // Extract the raw FILETIME into a 64 bit integer.
    boost::uint64_t _100nsSince1601 = ft.dwHighDateTime;
    _100nsSince1601 <<= 32;
    _100nsSince1601 |= ft.dwLowDateTime;

    // Add in the fractional second, which is in microsecond * 10 to get
    // 100s of nanosecond
    _100nsSince1601 += td.fractional_seconds() * 10;

    // Now put the time back inside filetime.
    ft.dwHighDateTime = _100nsSince1601 >> 32;
    ft.dwLowDateTime = _100nsSince1601 & 0x00000000FFFFFFFF;

    return ft;
}

stdstring environment::get_system_message(int message_id){
    LPTSTR szTemp = NULL;
    stdstring::size_type pos;
    stdstring str;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        message_id,
        LANG_NEUTRAL,
        (LPTSTR)&szTemp,
        0,
        NULL)){
        str = szTemp;
        LocalFree((HLOCAL)szTemp);
        while (TRUE){
            if (stdstring::npos != (pos = str.find_last_of(_T('\r')))){
                str = str.erase(pos, 1);
                continue;
            }
            if (stdstring::npos != (pos = str.find_last_of(_T('\n')))){
                str = str.erase(pos, 1);
                continue;
            }
            break;
        }
    }
    return str;
}

bool environment::shutdown_system(bool is_reboot, stdstring message, int wait_time){
    BOOL             bResult = FALSE;     // system shutdown flag 
    DWORD            dwRetry = 10;
    DWORD            dwLastErr = ERROR_SUCCESS;
    set_token_privilege(SE_SHUTDOWN_NAME, true);
    // Display the shutdown dialog box and start the countdown. 
    while (!(bResult = InitiateSystemShutdown(
        NULL,    // shut down local computer 
        message.empty() ? NULL : (LPTSTR)message.c_str(),   // message for user
        wait_time,      // time-out period, in seconds 
        TRUE,    // ask user to close apps 
        is_reboot ? TRUE : FALSE)))   // reboot after shutdown 
    {
        dwLastErr = GetLastError();
        LOG(LOG_LEVEL_ERROR, L"InitiateSystemShutdown failed (%d).", dwLastErr);
        if ((dwLastErr == ERROR_NOT_READY) && (dwRetry > 0)){
            dwRetry--;
            LOG(LOG_LEVEL_WARNING, L"Retry the call after 15 seconds.(%d)", 10 - dwRetry);
            Sleep(15000);
        }
        else{
            SetLastError(dwLastErr);
            break;
        }
    }
    return bResult ? true : false;
}
#endif

};//namespace windows
};//namespace macho

#endif