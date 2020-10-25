// reg_edit_base.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_NETWORK_CONNECTION__
#define __MACHO_WINDOWS_NETWORK_CONNECTION__
#include <boost\shared_ptr.hpp>
#include "..\config\config.hpp"
#include "..\common\tracelog.hpp"
#include "protected_data.hpp"
#include "auto_handle_base.hpp"
#include "environment.hpp"
#include <Winnetwk.h>
#include <Davclnt.h>
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "mpr.lib")

namespace macho{ namespace windows{
class wnet_connection{
private:
    wnet_connection(stdstring path, bool auto_disconnect = true) : _path(path), _remote(path), _is_connected(false), _is_impersonated(false), _is_auto_disconnect(auto_disconnect){}
    bool _is_connected;
    bool _is_impersonated;
    bool _is_auto_disconnect;
    stdstring _path;
    stdstring _remote;
    stdstring _resource_provider;
    static stdstring query_network_resource_provider(stdstring& path);
    static DWORD get_net_used_drives();
    static UINT find_next_available_drive(DWORD drives);
public:
    static std::wstring find_next_available_drive();
    stdstring path() const { return _path; }
    stdstring remote() const { return _remote; }
    stdstring resource_provider() const { return _resource_provider; }
    virtual ~wnet_connection(){
        if (_is_auto_disconnect) disconnect();
    }
    void disconnect();
    typedef boost::shared_ptr<wnet_connection> ptr;
    static wnet_connection::ptr connect(stdstring path, protected_data username, protected_data password, bool auto_disconnect = true, stdstring drive = _T(""), DWORD flags = CONNECT_TEMPORARY);
};

#ifndef MACHO_HEADER_ONLY

DWORD wnet_connection::get_net_used_drives(){
    DWORD ret = 0;
    NET_API_STATUS status;
    USE_INFO_0 *pInfo;
    DWORD i, EntriesRead, TotalEntries;
    status = NetUseEnum(
        NULL,   //UncServerName
        0,      //Level
        (LPBYTE*)&pInfo,
        MAX_PREFERRED_LENGTH, //PreferedMaximumSize
        &EntriesRead,
        &TotalEntries,
        NULL    //ResumeHandle
        );

    if (status == NERR_Success){
        for (i = 0; i < EntriesRead; i++){
            DWORD mask = 1 << (pInfo[i].ui0_local[0] - (TCHAR)'A');
            ret |= mask;
        }

        NetApiBufferFree(pInfo);
    }
    return ret;
}

std::wstring wnet_connection::find_next_available_drive(){
    TCHAR drive[4];
    UINT	nIndex = 0;
    macho::windows::mutex m(L"find_next_available_drive");
    macho::windows::auto_lock lock(m);
    DWORD   drives = GetLogicalDrives() | get_net_used_drives();
    if ((nIndex = find_next_available_drive(drives)) != 0){
        drive[0] = (TCHAR)'A' + (TCHAR)nIndex;
        drive[1] = (TCHAR)':';
        drive[2] = (TCHAR)'\0';
        drive[3] = (TCHAR)'\0';
        return drive;
    }
    return _T("");
}

UINT wnet_connection::find_next_available_drive(DWORD drives){
    UINT i;
    DWORD mask = 4; //from C:

    for (i = 2; i < 26; i++){
        if (!(drives & mask)){
            return i;
        }
        mask <<= 1;
    }
    return 0;
}

stdstring wnet_connection::query_network_resource_provider(stdstring& path){

    stdstring provider;
    DWORD dwBufferSize = sizeof(NETRESOURCE);
    std::auto_ptr<BYTE> lpBuffer;                  // buffer
    NETRESOURCE nr;
    LPTSTR pszSystem = NULL;          // variable-length strings
    DWORD dwRetVal = ERROR_SUCCESS;
    //
    // Set the block of memory to zero; then initialize
    // the NETRESOURCE structure. 
    //
    ZeroMemory(&nr, sizeof(nr));

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_ANY;
    nr.lpRemoteName = (LPTSTR)path.c_str();

    //
    // First call the WNetGetResourceInformation function with 
    // memory allocated to hold only a NETRESOURCE structure. This 
    // method can succeed if all the NETRESOURCE pointers are NULL.
    // If the call fails because the buffer is too small, allocate
    // a larger buffer.
    //
    lpBuffer = std::auto_ptr<BYTE>(new BYTE[dwBufferSize]);

    while ((dwRetVal = WNetGetResourceInformation(&nr, lpBuffer.get(), &dwBufferSize,
        &pszSystem) ) == ERROR_MORE_DATA){
        lpBuffer = std::auto_ptr<BYTE>(new BYTE[dwBufferSize]);
    }

    // Process the contents of the NETRESOURCE structure and the
    // variable-length strings in lpBuffer and set dwValue. When
    // finished, free the memory.
    if (ERROR_SUCCESS == dwRetVal)
        provider = ((NETRESOURCE*)lpBuffer.get())->lpProvider;
    else{
        LOG(LOG_LEVEL_ERROR, _T("Failed to WNetGetResourceInformation(%s) : Error: %d"), path.c_str(), dwRetVal);
    }
    return provider;
}

void wnet_connection::disconnect(){
    if (_is_impersonated) RevertToSelf();
    if (_is_connected) WNetCancelConnection((LPTSTR)_path.c_str(), FALSE);
}

wnet_connection::ptr wnet_connection::connect(stdstring path, protected_data username, protected_data password, bool auto_disconnect, stdstring drive, DWORD flags){
    stdstring p = path;
    stringutils::tolower(p);
    if ( path.length() > 3 &&
        (_T('\\') == path[0] && _T('\\') == path[1]) || 
        (_T('h') == p[0] && _T('t') == p[1] && _T('t') == p[2] && _T('p') == p[3])
        ){

        wnet_connection::ptr p = wnet_connection::ptr(new wnet_connection(path, auto_disconnect));
        if (macho::windows::environment::is_running_as_local_system()){
            HANDLE hToken;
            if (LogonUser(_T("NETWORK SERVICE"), _T("NT AUTHORITY"), NULL, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken)){
                p->_is_impersonated = ImpersonateLoggedOnUser(hToken) ? true : false;
                CloseHandle(hToken);
            }
        }
        NETRESOURCE nr;
        // Zero out the NETRESOURCE struct
        memset(&nr, 0, sizeof(NETRESOURCE));
        nr.dwScope = RESOURCE_GLOBALNET;
        nr.dwType = RESOURCETYPE_DISK;
        nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        nr.lpRemoteName = (LPTSTR)path.c_str();
        nr.lpLocalName = drive.length() ? (LPTSTR)drive.c_str() : NULL;
        nr.lpProvider = NULL;
        DWORD dwRetVal = NO_ERROR;
        stdstring resource_provider;
        if (( resource_provider = query_network_resource_provider(path) ) == _T("NFS Network")){
            nr.lpProvider = _T("NFS Network");
            dwRetVal = WNetAddConnection2(&nr, _T(""), _T(""), flags);
        }
        else{
            dwRetVal = WNetAddConnection2(&nr, ((stdstring)password).c_str(), ((stdstring)username).c_str(), flags);
        }

        if (p->_is_connected = (dwRetVal == NO_ERROR)){
            p->_resource_provider = resource_provider;
            if (_T("Web Client Network") == resource_provider){
                DWORD len = 0;
                DWORD ret = DavGetUNCFromHTTPPath(path.c_str(), NULL, &len);
                if (ret == ERROR_INSUFFICIENT_BUFFER){
                    std::auto_ptr<TCHAR> unc = std::auto_ptr<TCHAR>(new TCHAR[len+1]);
                    memset(unc.get(), 0, (len + 1) * sizeof(TCHAR));
                    if (ERROR_SUCCESS == (ret = DavGetUNCFromHTTPPath(path.c_str(), unc.get(), &len))){
                        p->_path = unc.get();
                    }
                    else{
                        return NULL;
                    }
                }
                else{
                    return NULL;
                }
            }
            return p;
        }
        LOG(LOG_LEVEL_ERROR, L"WNetAddConnection2 Error: %d ", dwRetVal);
    }
    return NULL;
}

#endif

};//namespace windows
};//namespace macho
#endif