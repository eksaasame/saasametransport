// reg_cluster_edit.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_REG_CLUSTER_EDIT__
#define __MACHO_WINDOWS_REG_CLUSTER_EDIT__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "reg_edit_base.hpp"
#include <ClusApi.h>

#pragma comment(lib, "ClusApi.lib") 

namespace macho{

namespace windows{

struct    reg_cluster_edit_exception : virtual public exception_base {};
#define BOOST_THROW_REG_CLUSTER_EDIT_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( reg_cluster_edit_exception, no, message )

class reg_cluster_edit : public reg_edit_base {
public:
    virtual HKEY get_cluster_key( __in  REGSAM samDesired = KEY_ALL_ACCESS );
    virtual LONG close_key( __in  HKEY hKey ){
        return ClusterRegCloseKey( hKey );
    }
    virtual LONG delete_key( __in  HKEY hKey, __in  LPCTSTR lpSubKey ){
        return ClusterRegDeleteKey( hKey, lpSubKey );
    }

    virtual LONG create_key( 
      __in        HKEY hKey,
      __in        LPCTSTR lpSubKey,
      __reserved  DWORD Reserved,
      __in_opt    LPTSTR lpClass,
      __in        DWORD dwOptions,
      __in        REGSAM samDesired,
      __in_opt    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      __out       PHKEY phkResult,
      __out_opt   LPDWORD lpdwDisposition
    ){    
        return ClusterRegCreateKey( hKey, lpSubKey, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition );
    }

    virtual LONG open_key(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpSubKey,
      __reserved  DWORD ulOptions,
      __in        REGSAM samDesired,
      __out       PHKEY phkResult
    ){
        return ClusterRegOpenKey( hKey, lpSubKey, samDesired, phkResult );
    }

    virtual LONG delete_value( __in HKEY hKey, __in_opt  LPCTSTR lpValueName ){
        return ClusterRegDeleteValue( hKey, lpValueName );
    }

    virtual LONG delete_tree( __in HKEY hKey, __in_opt  LPCTSTR lpSubKey );

    virtual LONG enumerate_key(
      __in         HKEY hKey,
      __in         DWORD dwIndex,
      __out        LPTSTR lpName,
      __inout      LPDWORD lpcName,
      __reserved   LPDWORD lpReserved,
      __inout      LPTSTR lpClass,
      __inout_opt  LPDWORD lpcClass,
      __out_opt    PFILETIME lpftLastWriteTime
    )
    {
        return ClusterRegEnumKey( hKey, dwIndex, lpName, lpcName, lpftLastWriteTime );
    }

    virtual LONG enumerate_value(
      __in         HKEY hKey,
      __in         DWORD dwIndex,
      __out        LPTSTR lpValueName,
      __inout      LPDWORD lpcchValueName,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpType,
      __out_opt    LPBYTE lpData,
      __inout_opt  LPDWORD lpcbData
    ){
        return ClusterRegEnumValue( hKey, dwIndex, lpValueName, lpcchValueName, lpType, lpData, lpcbData );
    }

    virtual LONG query_value(
      __in         HKEY hKey,
      __in_opt     LPCTSTR lpValueName,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpType,
      __out_opt    LPBYTE lpData,
      __inout_opt  LPDWORD lpcbData
    ){
        return ClusterRegQueryValue( hKey, lpValueName, lpType, lpData, lpcbData );
    }

    virtual LONG set_value(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpValueName,
      __reserved  DWORD Reserved,
      __in        DWORD dwType,
      __in_opt    const BYTE *lpData,
      __in        DWORD cbData
    ){
        return ClusterRegSetValue( hKey, lpValueName, dwType, lpData, cbData );
    }

    virtual LONG query_info_key(
      __in         HKEY hKey,
      __out        LPTSTR lpClass,
      __inout_opt  LPDWORD lpcClass,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpcSubKeys,
      __out_opt    LPDWORD lpcMaxSubKeyLen,
      __out_opt    LPDWORD lpcMaxClassLen,
      __out_opt    LPDWORD lpcValues,
      __out_opt    LPDWORD lpcMaxValueNameLen,
      __out_opt    LPDWORD lpcMaxValueLen,
      __out_opt    LPDWORD lpcbSecurityDescriptor,
      __out_opt    PFILETIME lpftLastWriteTime
    ){
        return ClusterRegQueryInfoKey( hKey, lpcSubKeys, lpcMaxSubKeyLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime );
    }

    virtual LONG delete_key(
      __in        HKEY hKey,
      __in        LPCTSTR lpSubKey,
      __in        REGSAM samDesired,
      __reserved  DWORD Reserved
    ){
        return ClusterRegDeleteKey( hKey, lpSubKey );
    }
    
    virtual LONG flush_key( __in  HKEY hKey ){ return ERROR_SUCCESS; }

    reg_cluster_edit( stdstring cluster_name );
    virtual ~reg_cluster_edit();

private :
    HCLUSTER        _cluster;
    stdstring       _cluster_name;
};


#ifndef MACHO_HEADER_ONLY

reg_cluster_edit::reg_cluster_edit( stdstring cluster_name ) : reg_edit_base() {  
    _cluster_name = cluster_name;
    _cluster = OpenCluster( cluster_name.length() ? cluster_name.c_str() : NULL );
    if ( !_cluster ){
        BOOST_THROW_REG_CLUSTER_EDIT_EXCEPTION( GetLastError(), _T("OpenCluster() error.") );
    }
}

reg_cluster_edit::~reg_cluster_edit(){
    CloseCluster(_cluster);
}

HKEY reg_cluster_edit::get_cluster_key( REGSAM samDesired ){

    HKEY hKey = ::GetClusterKey( _cluster, samDesired );
    if ( !hKey ){
        BOOST_THROW_REG_CLUSTER_EDIT_EXCEPTION( GetLastError(), _T("GetClusterKey() error.") );
    }
    return hKey;
}

LONG reg_cluster_edit::delete_tree( __in HKEY hKey, __in_opt  LPCTSTR lpSubKey ){

    DWORD   dwRtn = ERROR_SUCCESS, dwSubKeyLength = MAX_KEY_LENGTH;
    LPTSTR  pSubKey = NULL;
    std::auto_ptr<TCHAR> pszSubKey( new TCHAR[MAX_KEY_LENGTH] );
    memset( pszSubKey.get(), 0, MAX_KEY_LENGTH * sizeof(TCHAR) ); 
    HKEY    hSubKey ;
    if ( ( dwRtn = open_key( hKey, lpSubKey,
        0, KEY_ENUMERATE_SUB_KEYS| KEY_QUERY_VALUE| KEY_SET_VALUE | DELETE , &hSubKey ) ) == ERROR_SUCCESS ) {
        while (dwRtn == ERROR_SUCCESS )    {
            dwSubKeyLength = MAX_KEY_LENGTH ;
            dwRtn = ClusterRegEnumKey(
                              hSubKey,
                              0,       // always index zero
                              pszSubKey.get(),
                              &dwSubKeyLength,
                              NULL
                            );

            if( dwRtn == ERROR_NO_MORE_ITEMS ) {
                if ( lpSubKey &&  lstrlen( lpSubKey ) )
                    dwRtn = ClusterRegDeleteKey( hKey, lpSubKey );
                else{
                    // Add code to empty valuse;
                    std::auto_ptr<TCHAR> pachValue( new TCHAR[MAX_VALUE_NAME] );
                    DWORD cchValue = MAX_VALUE_NAME; 
                    dwRtn = ERROR_SUCCESS;

                    while( dwRtn == ERROR_SUCCESS )    {
                        cchValue = MAX_VALUE_NAME; 
                        memset( pachValue.get(), 0, MAX_VALUE_NAME * sizeof(TCHAR) ); 
                        dwRtn = ClusterRegEnumValue( hSubKey, 0, 
                            pachValue.get(), 
                            &cchValue, 
                            NULL, 
                            NULL,
                            NULL);
             
                        if ( dwRtn == ERROR_SUCCESS ) { 
                            dwRtn = ClusterRegDeleteValue( hSubKey, pachValue.get() );
                        } 
                        else if ( dwRtn == ERROR_NO_MORE_ITEMS ){
                            dwRtn = ERROR_SUCCESS;
                            break;
                        }
                    }
                }
                break;
            }
            else if( dwRtn == ERROR_SUCCESS )
                dwRtn = delete_tree( hSubKey, pszSubKey.get());
        }
        ClusterRegCloseKey( hSubKey );
        // Do not save return code because error
        // has already occurred
    }

    //if ( dwRtn == ERROR_FILE_NOT_FOUND )
    //    dwRtn = ERROR_SUCCESS;

    return dwRtn;
}


#endif

};//namespace windows
};//namespace macho


#endif