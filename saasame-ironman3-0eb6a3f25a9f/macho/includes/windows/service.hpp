// service.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_SERVICE__
#define __MACHO_WINDOWS_SERVICE__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"

namespace macho{

namespace windows{

struct  service_exception :  public exception_base {};
#define BOOST_THROW_SERVICE_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( service_exception, no, message )
class   service;
typedef std::vector< service > service_table;


class service{
public:

    typedef enum _START_MODES_ENUM{
        SERVICE_BOOT_START_MODE = 0x00000000,
        SERVICE_SYSTEM_START_MODE = 0x00000001,
        SERVICE_AUTO_START_MODE = 0x00000002,
        SERVICE_DEMAND_START_MODE = 0x00000003,
        SERVICE_DISABLED_MODE = 0x00000004
    }START_MODES_ENUM;

    typedef enum _STATES_ENUM{
        SERVICE_STATE_UNKNOWN = 0x00000000,
        SERVICE_STATE_STOPPED = 0x00000001,
        SERVICE_STATE_START_PENDING = 0x00000002,
        SERVICE_STATE_STOP_PENDING = 0x00000003,
        SERVICE_STATE_RUNNING = 0x00000004,
        SERVICE_STATE_CONTINUE_PENDING = 0x00000005,
        SERVICE_STATE_PAUSE_PENDING = 0x00000006,
        SERVICE_STATE_PAUSED = 0x00000007
    }STATES_ENUM;

    //variables 
    stdstring    inline  name()             const { return _name; }
    stdstring    inline  display_name()     const { return _display_name; }
    stdstring    inline  description()      const { return _description; }
    stdstring    inline  path_name()        const { return _path; }
	stdstring    inline  binary_path()            { return _get_binary_path();}
    stdstring    inline  start_name()       const { return _start_name; }
    stdstring    inline  load_order_group() const { return _load_order_group; }
    stdstring    inline  dependencies()     const { return _dependencies; }
    stdstring    inline  computer_name()    const { return _computer_name; } 
    DWORD        inline  type()             const { return _type; }
      
    bool         inline  is_file_systme_driver()   const { return ( type() & SERVICE_FILE_SYSTEM_DRIVER) == SERVICE_FILE_SYSTEM_DRIVER; }
    bool         inline  is_kernel_driver()        const { return ( type() & SERVICE_KERNEL_DRIVER ) == SERVICE_KERNEL_DRIVER; }
    bool         inline  is_driver()               const { return is_kernel_driver()||is_file_systme_driver() ;}
    bool         inline  is_win32_own_process()    const { return ( type() & SERVICE_WIN32_OWN_PROCESS) == SERVICE_WIN32_OWN_PROCESS; }
    bool         inline  is_win32_share_process()  const { return ( type() & SERVICE_WIN32_SHARE_PROCESS) == SERVICE_WIN32_SHARE_PROCESS; }
    bool         inline  is_interactive_process()  const { return ( type() & SERVICE_INTERACTIVE_PROCESS) == SERVICE_INTERACTIVE_PROCESS; }

    START_MODES_ENUM        inline  start_mode()       const { return static_cast<START_MODES_ENUM>( _start_mode ); }  

    stdstring            company_name();
    stdstring            product_name();
    STATES_ENUM          current_state();
    bool                 start();
    bool                 stop();
    bool                 terminate();
	bool				 set_recovery_policy(std::vector<int> intervals, int timeout = 0);
    static service       get_service( stdstring name, stdstring computer_name = stdstring() );
    static service_table get_all_services( stdstring computer_name = stdstring() );
private:
	bool				 _wait(DWORD pid);
    bool                 _stop_dependent_services(auto_service_handle &hSc, auto_service_handle &hSvc);
    static bool          _get_service( SC_HANDLE hSc, service &svc );
    void                 _get_version_info();
    stdstring            _get_binary_path();
    service(stdstring name) :  _start_mode(0), _type(0) {
        _name = name;
    }
    stdstring _name;
    stdstring _display_name;
    stdstring _description;
    stdstring _path;
    stdstring _start_name;
    stdstring _load_order_group;
    stdstring _dependencies;
    stdstring _computer_name;
    stdstring _company_name;
    stdstring _product_name;  
    DWORD     _start_mode;
    DWORD     _type;
};

#ifndef MACHO_HEADER_ONLY

#include "..\common\stringutils.hpp"
#include "..\common\tracelog.hpp"
#include "..\windows\auto_handle_base.hpp"
#include "..\windows\file_version_info.hpp"

#include <Psapi.h>
#pragma comment(lib, "Psapi.LIB")

//SvcControl.cpp
//http://msdn.microsoft.com/zh-tw/library/windows/desktop/bb540474(v=vs.85).aspx

bool service::start(){
    bool result = false;
    auto_service_handle hSc = OpenSCManager(_computer_name.length() > 0 ? _computer_name.c_str() : NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSc.is_invalid()){
        // log error message here;
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenSCManager (%s) [Error: %d]"), _computer_name.length() > 0 ? _computer_name.c_str() : _T(""), GetLastError());
        return false;
    }

    auto_service_handle hSvc = OpenService(hSc, _name.c_str(), SERVICE_ALL_ACCESS);
    if (hSvc.is_invalid()){
        // Log error message
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenService (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;
    if (!QueryServiceStatusEx(
        hSvc,                           // handle to service 
        SC_STATUS_PROCESS_INFO,         // information level
        (LPBYTE)&ssStatus,              // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded))                // size needed if buffer is too small
    {
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return result;
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        LOG(LOG_LEVEL_INFO, TEXT("Cannot start the service (%s) because it is already running"), _name.c_str() );
		return true;
    }
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status until the service is no longer stop pending. 

        if (!QueryServiceStatusEx(
            hSvc,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // size needed if buffer is too small
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
            return result;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                LOG(LOG_LEVEL_ERROR, TEXT("Timeout waiting for service (%s) to stop"), _name.c_str() );
                return result;
            }
        }
    }

    // Attempt to start the service.
	dwStartTickCount = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out
    while (!StartService(
        hSvc,  // handle to service 
        0,           // number of arguments 
        NULL))      // no arguments 
    {
        LOG(LOG_LEVEL_ERROR, TEXT("StartService (%s) failed [Error: %d]"), _name.c_str(), GetLastError());
		if (GetTickCount() - dwStartTickCount > dwTimeout)
			return result;
		boost::this_thread::sleep(boost::posix_time::seconds(1));
    }

    LOG(LOG_LEVEL_INFO, TEXT("Service (%s) start pending..."), _name.c_str() );
    // Check the status until the service is no longer start pending. 

    if (!QueryServiceStatusEx(
        hSvc,                     // handle to service 
        SC_STATUS_PROCESS_INFO,         // info level
        (LPBYTE)&ssStatus,             // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded))              // if buffer too small
    {
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return result;
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status again. 

        if (!QueryServiceStatusEx(
            hSvc,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE)&ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // if buffer too small
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
            break;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    }

    // Determine whether the service is running.

    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
        result = true;
        LOG(LOG_LEVEL_INFO, TEXT("Service (%s) started successfully."), _name.c_str() );
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, TEXT("Service (%s) not started."), _name.c_str());
        LOG(LOG_LEVEL_ERROR, TEXT("  Current State : %d"), ssStatus.dwCurrentState);
        LOG(LOG_LEVEL_ERROR, TEXT("  Exit Code: %d"), ssStatus.dwWin32ExitCode);
        LOG(LOG_LEVEL_ERROR, TEXT("  Check Point: %d"), ssStatus.dwCheckPoint);
        LOG(LOG_LEVEL_ERROR, TEXT("  Wait Hint: %d"), ssStatus.dwWaitHint);
    }        

    return result;
}

bool service::_stop_dependent_services(auto_service_handle &hSc, auto_service_handle &hSvc){
   
    DWORD i;
    DWORD dwBytesNeeded;
    DWORD dwCount;

    LPENUM_SERVICE_STATUS   lpDependencies = NULL;
    ENUM_SERVICE_STATUS     ess;
    SERVICE_STATUS_PROCESS  ssp;

    DWORD dwStartTime = GetTickCount();
    DWORD dwTimeout = 30000; // 30-second time-out

    // Pass a zero-length buffer to get the required buffer size.
    if (EnumDependentServices(hSvc, SERVICE_ACTIVE,
        lpDependencies, 0, &dwBytesNeeded, &dwCount))
    {
        // If the Enum call succeeds, then there are no dependent
        // services, so do nothing.
        return true;
    }
    else
    {
        if (GetLastError() != ERROR_MORE_DATA)
            return false; // Unexpected error

        // Allocate a buffer for the dependencies.
        lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
            GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

        if (!lpDependencies)
            return false;

        auto_heap_free auto_free(lpDependencies);

        // Enumerate the dependencies.
        if (!EnumDependentServices(hSvc, SERVICE_ACTIVE,
            lpDependencies, dwBytesNeeded, &dwBytesNeeded,
            &dwCount))
            return false;

        for (i = 0; i < dwCount; i++)
        {
            ess = *(lpDependencies + i);
            // Open the service.
            auto_service_handle hDepService = OpenService(hSc,
                ess.lpServiceName,
                SERVICE_STOP | SERVICE_QUERY_STATUS);

            if (hDepService.is_valid())
            {
                // Send a stop code.
                if (!ControlService(hDepService,
                    SERVICE_CONTROL_STOP,
                    (LPSERVICE_STATUS)&ssp))
                    return false;

                // Wait for the service to stop.
                while (ssp.dwCurrentState != SERVICE_STOPPED)
                {
                    Sleep(ssp.dwWaitHint);
                    if (!QueryServiceStatusEx(
                        hDepService,
                        SC_STATUS_PROCESS_INFO,
                        (LPBYTE)&ssp,
                        sizeof(SERVICE_STATUS_PROCESS),
                        &dwBytesNeeded))
                        return false;

                    if (ssp.dwCurrentState == SERVICE_STOPPED)
                        break;

                    if (GetTickCount() - dwStartTime > dwTimeout)
                        return false;
                }
            }
        }
    }
    return true;
}

bool service::stop(){

    bool result = false;
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

    // Get a handle to the SCM database. 

    auto_service_handle hSc = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (hSc.is_invalid()){
        // log error message here;
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenSCManager (%s) [Error: %d]"), _computer_name.length() > 0 ? _computer_name.c_str() : _T(""), GetLastError());
        return false;
    }

    auto_service_handle hSvc = OpenService(hSc, _name.c_str(), SERVICE_ALL_ACCESS);
    if (hSvc.is_invalid()){
        // Log error message
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenService (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }


    // Make sure the service is not already stopped.
    if (!QueryServiceStatusEx(
        hSvc,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded))
    {
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED)
    {
        LOG(LOG_LEVEL_INFO, TEXT("Service (%s) is already stopped"), _name.c_str());
        return true;
    }

    // If a stop is pending, wait for it.

    while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
    {
        LOG(LOG_LEVEL_INFO, TEXT("Service (%s) stop pending..."), _name.c_str());
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        dwWaitTime = ssp.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        if (!QueryServiceStatusEx(
            hSvc,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
            return false;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
        {
            LOG(LOG_LEVEL_INFO, TEXT("Service (%s) stopped successfully"), _name.c_str());
            return true;
        }

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Service (%s) stop timed out"), _name.c_str());
            return false;
        }
    }

    // If the service is running, dependencies must be stopped first.

    if (!_stop_dependent_services(hSc, hSvc))
        return false;

    // Send a stop code to the service.

    if (!ControlService(
        hSvc,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS)&ssp))
    {
        LOG(LOG_LEVEL_ERROR, TEXT("ControlService (%s) failed [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }

    // Wait for the service to stop.

    while (ssp.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(ssp.dwWaitHint);
        if (!QueryServiceStatusEx(
            hSvc,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
            return false;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            LOG(LOG_LEVEL_ERROR, TEXT("Service (%s) stop timed out"), _name.c_str());
            return false;
        }
    }
	if (!_wait(ssp.dwProcessId)){
		LOG(LOG_LEVEL_ERROR, TEXT("Service (%s) stop timed out"), _name.c_str());
		return false;
	}
    LOG(LOG_LEVEL_INFO, TEXT("Service (%s) stopped successfully"), _name.c_str());
    return true;
}

service::STATES_ENUM service::current_state(){

    STATES_ENUM state = SERVICE_STATE_UNKNOWN;
    auto_service_handle hSc = OpenSCManager( _computer_name.length() > 0 ? _computer_name.c_str() : NULL, NULL, GENERIC_READ );
    if ( hSc.is_invalid()){
        // log error message here;
        LOG( LOG_LEVEL_ERROR, TEXT( "Failed to OpenSCManager (%s) [Error: %d]" ), _computer_name.length() > 0 ? _computer_name.c_str() : _T(""), GetLastError() );
    }
    else{
        auto_service_handle hSvc = OpenService( hSc, _name.c_str(), GENERIC_READ );     
        if ( hSvc.is_invalid()){
            // Log error message
            LOG( LOG_LEVEL_ERROR, TEXT( "Failed to OpenService (%s) [Error: %d]" ), _name.c_str(), GetLastError() );
        }
        else{   
            SERVICE_STATUS svc_status;
            if ( !QueryServiceStatus( hSvc, &svc_status ) ){
                LOG( LOG_LEVEL_ERROR, TEXT( "Failed to QueryServiceStatus (%s) [Error: %d]" ), _name.c_str(), GetLastError() );
            }
            else{
                state = static_cast<STATES_ENUM>(svc_status.dwCurrentState);
            }   
        }
    }
    return state;
}

stdstring service::company_name(){
    if ( _company_name.length() == 0 )
        _get_version_info();
    return _company_name;
}

stdstring service::product_name(){
    if ( _product_name.length() == 0 )
         _get_version_info();
    return _product_name;
}

stdstring service::_get_binary_path(){

    stdstring binary_path;
    if ( _path.length() > 0 ){
        string_table execution_paths;
        stdstring path = stringutils::tolower(stringutils::remove_begining_whitespaces(environment::get_expand_environment_strings(_path)));
        if ( is_driver() || path[0] == _T('\"') || path[0] == _T('\'') ) {
            execution_paths = stringutils::tokenize( path, _T("\"'"), 0 , false ); 
            if( execution_paths.size() > 0 ){
                string_table paths = stringutils::tokenize( execution_paths[0] , _T("\\/"), 0 , false ); 
                if ( paths.size() > 0 ){
                    if ( paths[0] == _T("systemroot") ){
                       paths[0] =  environment::get_environment_variable( _T("systemroot") );
                    }
                    else if ( ( paths[0] == _T("system32") ) || ( paths[0] == _T("syswow64") ) ){
                        paths.insert( paths.begin(), environment::get_environment_variable( _T("systemroot") ) );
                    }
                    else if ( paths[0] == _T("??") ){
                        paths.erase( paths.begin() );
                    }  

                    foreach( stdstring p, paths ){
                        if ( binary_path.length() > 0 )
                            binary_path.append( _T("\\") );
                        binary_path.append(p);
                    }
                }
            }
        }
        else{ 
            stdstring _checkfilepath;
            string_table _filefolders = stringutils::tokenize( path, _T("\\/"), 0 , false ); 
            for(size_t index = 0; index < _filefolders.size(); ){
                _checkfilepath.append(_filefolders[index]);
                if ( PathFileExists( _checkfilepath.c_str() ) ){
                    binary_path = _checkfilepath;
                }
                else{
                    string_table _files = stringutils::tokenize( _filefolders[index], _T(" "), 0 , true ); 
                    _checkfilepath = binary_path + _T("\\");
                    for( size_t i = 0; i < _files.size();  ){
                        _checkfilepath.append( _files[i] );
                        if ( PathFileExists( _checkfilepath.c_str() ) ){
                            binary_path = _checkfilepath;
                        }
                        if ( ++i < _files.size() )
                            _checkfilepath.append( _T(" ") );
                    }
                    break;
                }
                if ( ++index < _filefolders.size() )
                    _checkfilepath.append(_T("\\") );
            }
        }
    }
    return PathIsDirectory( binary_path.c_str() ) ? _T("") : binary_path;
}

void service::_get_version_info(){

    stdstring path = _get_binary_path();
    try{
        file_version_info info = file_version_info::get_file_version_info( path );
        _company_name = info.company_name();
        _product_name = info.product_name();
    } 
    catch ( ... ){
    }
}

service service::get_service( stdstring name, stdstring computer_name ){

    service svc( name );
    svc._company_name = computer_name;
    auto_service_handle hSc = OpenSCManager( computer_name.length() > 0 ? computer_name.c_str() : NULL, NULL, GENERIC_READ );
    if ( hSc.is_invalid()){
        // log error message here;
        LOG( LOG_LEVEL_ERROR, TEXT( "Failed to OpenSCManager (%s) [Error: %d]" ), computer_name.length() > 0 ? computer_name.c_str() : _T(""), GetLastError() );
#if _UNICODE  
        BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::wformat(L"Failed to OpenSCManager (%s)") % (computer_name.length() > 0 ? computer_name : _T("") ) ) );
#else
        BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::format("Failed to OpenSCManager (%s)") % ( computer_name.length() > 0 ? computer_name : _T("") ) ) );
#endif
    }
    else{
        _get_service( hSc, svc );
    }
    return svc;
}

bool service::_get_service( SC_HANDLE hSc, service &svc ){
    
    auto_service_handle hSvc = OpenService( hSc, svc._name.c_str(), GENERIC_READ );     
    if ( hSvc.is_invalid()){
        // Log error message
        LOG( LOG_LEVEL_ERROR, TEXT( "Failed to OpenService (%s) [Error: %d]" ), svc._name.c_str(), GetLastError() );
#if _UNICODE  
        BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::wformat(L"Failed to OpenService (%s)") %svc._name ) );
#else
        BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::format("Failed to OpenService (%s)") %svc._name ) );
#endif
    }
    else{   
        DWORD SizeNeeded=0;               
        // Query service config info
        std::auto_ptr<QUERY_SERVICE_CONFIG> lpsc;
        if ( (QueryServiceConfig(hSvc,NULL, 0, &SizeNeeded)==0) &&
            ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) && 
            ( NULL != ( lpsc = std::auto_ptr<QUERY_SERVICE_CONFIG>((LPQUERY_SERVICE_CONFIG)new BYTE[SizeNeeded])).get() ) &&
            (QueryServiceConfig( hSvc, lpsc.get(), SizeNeeded, &SizeNeeded )!=0)
        ){
            svc._start_mode       = lpsc->dwStartType;
            svc._type             = lpsc->dwServiceType;
            svc._start_name       = lpsc->lpServiceStartName;
            svc._display_name     = lpsc->lpDisplayName;
            svc._path             = lpsc->lpBinaryPathName;
            if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
                svc._dependencies     = lpsc->lpDependencies;
            if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
                svc._load_order_group = lpsc->lpLoadOrderGroup;
            // Query service description
            std::auto_ptr<SERVICE_DESCRIPTION> lpsd;
            if ( (QueryServiceConfig2( hSvc, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &SizeNeeded)==0) &&
                ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) && 
                ( NULL != ( lpsd = std::auto_ptr<SERVICE_DESCRIPTION>((LPSERVICE_DESCRIPTION)new BYTE[SizeNeeded])).get() ) &&
                (QueryServiceConfig2( hSvc, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)lpsd.get(), SizeNeeded, &SizeNeeded )!=0)
            ){   
                if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)                 
                    svc._description = lpsd->lpDescription;                        
                return true;
            }
            else{
                LOG( LOG_LEVEL_ERROR, TEXT( "Failed to QueryServiceConfig2 (%s) [Error: %d]" ), svc._name.c_str(), GetLastError() );
#if _UNICODE  
                BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::wformat(L"Failed to QueryServiceConfig2 (%s)") %svc._name ) );
#else
                BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::format("Failed to QueryServiceConfig2 (%s)") %svc._name ) );
#endif
            }
        }
        else{
            LOG( LOG_LEVEL_ERROR, TEXT( "Failed to QueryServiceConfig (%s) [Error: %d]" ), svc._name.c_str(), GetLastError() );
#if _UNICODE  
            BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::wformat(L"Failed to QueryServiceConfig (%s)") %svc._name ) );
#else
            BOOST_THROW_SERVICE_EXCEPTION( GetLastError(), boost::str( boost::format("Failed to QueryServiceConfig (%s)") %svc._name ) );
#endif
        }
    }
    return false;
}

service_table service::get_all_services(stdstring computer_name){

    service_table services;
    auto_service_handle hSc = OpenSCManager( computer_name.length() > 0 ? computer_name.c_str() : NULL, NULL, GENERIC_READ );
    if ( hSc.is_invalid()){
        // log error message here;
        LOG( LOG_LEVEL_ERROR, TEXT( "Failed to OpenSCManager (%s) [Error: %d]" ), computer_name.length() > 0 ? computer_name.c_str() : _T(""), GetLastError() );
    }
    else{
        DWORD SizeNeeded=0,returned =0,resume=0,Counter=0;
        std::auto_ptr<ENUM_SERVICE_STATUS> pEnumServiceStatus;
        LPENUM_SERVICE_STATUS p;     
        if    ( (EnumServicesStatus(hSc, SERVICE_WIN32|SERVICE_DRIVER,SERVICE_STATE_ALL,NULL,0,&SizeNeeded,&returned,&resume)==0) &&
            ( GetLastError()==ERROR_MORE_DATA ) && 
            ( NULL != ( pEnumServiceStatus = std::auto_ptr<ENUM_SERVICE_STATUS>((LPENUM_SERVICE_STATUS)new BYTE[SizeNeeded])).get() ) &&
            (EnumServicesStatus(hSc, SERVICE_WIN32|SERVICE_DRIVER,SERVICE_STATE_ALL, pEnumServiceStatus.get(),SizeNeeded,&SizeNeeded,&returned,&resume)!=0)
        ){
            p = pEnumServiceStatus.get();
            while(Counter<returned) {
                try{
                    service svc( p->lpServiceName );
                    svc._company_name = computer_name;
                    if ( _get_service( hSc, svc ) )
                        services.push_back( svc ); 
                }
                catch( ... ){   
                }
                p++;
                Counter++;
            }
        }
        else{
            LOG( LOG_LEVEL_ERROR, TEXT( "Failed to EnumServicesStatus (%s) [Error: %d]" ), computer_name.length() > 0 ? computer_name.c_str() : _T(""), GetLastError() );
        }
    }
    return services;
}

bool service::_wait(DWORD pid){
	bool result = true;
	if (pid){
		DWORD dwTimeout = 30000; // 30-second time-out
		DWORD dwStartTime = GetTickCount();
		while (!result){
			if (GetTickCount() - dwStartTime > dwTimeout)
				break;
			DWORD aProcesses[1024], cbNeeded, cProcesses;
			memset(&aProcesses, 0, sizeof(aProcesses));
			if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)){
				cProcesses = cbNeeded / sizeof(DWORD);
				result = true;
				for (int i = 0; i < cProcesses; i++){
					if (aProcesses[i] != 0 && aProcesses[i] == pid){
						result = false;
						boost::this_thread::sleep(boost::posix_time::seconds(1));
						break;
					}// if aProcesses[i]
				}// End for
			}// End EnumProcess
		}
	}
	return result;
}

bool service::set_recovery_policy(std::vector<int> intervals, int timeout ){
	bool result = false;
	if (intervals.size()){
		auto_service_handle hSc = OpenSCManager(_computer_name.length() > 0 ? _computer_name.c_str() : NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSc.is_invalid()){
			// log error message here;
			LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenSCManager (%s) [Error: %d]"), _computer_name.length() > 0 ? _computer_name.c_str() : _T(""), GetLastError());
		}
		else{
			auto_service_handle hSvc = OpenService(hSc, _name.c_str(), SERVICE_CHANGE_CONFIG | SERVICE_START );
			if (hSvc.is_invalid()){
				// Log error message
				LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenService (%s) [Error: %d]"), _name.c_str(), GetLastError());
			}
			else{
				SERVICE_FAILURE_ACTIONS sfa;
				memset(&sfa, 0, sizeof(SERVICE_FAILURE_ACTIONS));
				int bufsize = (sizeof(SC_ACTION) * intervals.size());
				std::auto_ptr<BYTE> buf(new BYTE[bufsize]);
				memset(buf.get(), 0, bufsize);
				sfa.dwResetPeriod = timeout ? timeout : INFINITE;
				sfa.cActions = intervals.size();
				sfa.lpsaActions = reinterpret_cast<SC_ACTION *>(buf.get());
				for (size_t i = 0; i < intervals.size(); i++){
					sfa.lpsaActions[i].Type = SC_ACTION_RESTART;
					sfa.lpsaActions[i].Delay = intervals[i] ? intervals[i] : 5000;
				}
				if (!(result = (TRUE == ChangeServiceConfig2(hSvc, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa)))){
					LOG(LOG_LEVEL_ERROR, TEXT("Failed to ChangeServiceConfig2 (%s) [Error: %d]"), _name.c_str(), GetLastError());
				}
			}
		}
	}
	return result;
}

bool service::terminate(){
    BOOL result = FALSE;
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

    // Get a handle to the SCM database. 

    auto_service_handle hSc = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (hSc.is_invalid()){
        // log error message here;
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenSCManager (%s) [Error: %d]"), _computer_name.length() > 0 ? _computer_name.c_str() : _T(""), GetLastError());
        return false;
    }

    auto_service_handle hSvc = OpenService(hSc, _name.c_str(), SERVICE_ALL_ACCESS);
    if (hSvc.is_invalid()){
        // Log error message
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to OpenService (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }

    // Make sure the service is not already stopped.
    if (!QueryServiceStatusEx(
        hSvc,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded))
    {
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to QueryServiceStatusEx (%s) [Error: %d]"), _name.c_str(), GetLastError());
        return false;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED)
    {
        LOG(LOG_LEVEL_INFO, TEXT("Service (%s) is already stopped"), _name.c_str());
        return true;
    }

    macho::windows::environment::set_token_privilege(SE_DEBUG_NAME, true);
    DWORD dwDesiredAccess = PROCESS_TERMINATE;
    BOOL  bInheritHandle = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, ssp.dwProcessId);
    if (hProcess == NULL)
        return true;
    result = TerminateProcess(hProcess, 0x1);
    if (!result){
        LOG(LOG_LEVEL_ERROR, TEXT("Service (%s) cannot be terminated. %s"), _name.c_str(), macho::windows::environment::get_system_message(GetLastError()).c_str());
    }
    CloseHandle(hProcess);
    return result == TRUE;
}

#endif

};//namespace windows
};//namespace macho
#endif