// reg_native_edit.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_REG_NATIVE_EDIT__
#define __MACHO_WINDOWS_REG_NATIVE_EDIT__

#include "..\config\config.hpp"
#include "reg_edit_base.hpp"
#include "environment.hpp"

namespace macho{

namespace windows{

typedef enum _REG_NATIVE_EDIT_FLAGS_ENUM{
    REG_NATIVE_EDIT_NONE            = 0x00000000,
    REG_NATIVE_EDIT_WOW64_64KEY        = 0x00000001
}REG_NATIVE_EDIT_FLAGS_ENUM;

class reg_native_edit : public reg_edit_base {

private:
    REG_NATIVE_EDIT_FLAGS_ENUM _flag;
public :
    reg_native_edit( REG_NATIVE_EDIT_FLAGS_ENUM flag = REG_NATIVE_EDIT_NONE ) : reg_edit_base() { _flag = flag; }
    virtual ~reg_native_edit(){}

    virtual LONG delete_key(
      __in        HKEY hKey,
      __in        LPCTSTR lpSubKey,
      __in        REGSAM samDesired,
      __reserved  DWORD Reserved
     );
    virtual LONG close_key( __in  HKEY hKey ){
        return ::RegCloseKey( hKey );
    }
    virtual LONG delete_key( __in  HKEY hKey, __in  LPCTSTR lpSubKey ){
        return ::RegDeleteKey( hKey, lpSubKey );
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
        samDesired |= _flag & REG_NATIVE_EDIT_WOW64_64KEY ? KEY_WOW64_64KEY : 0  ;
        return ::RegCreateKeyEx( hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition );
    }

    virtual LONG open_key(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpSubKey,
      __reserved  DWORD ulOptions,
      __in        REGSAM samDesired,
      __out       PHKEY phkResult
    ){
        samDesired |= _flag & REG_NATIVE_EDIT_WOW64_64KEY ? KEY_WOW64_64KEY : 0  ;
        return ::RegOpenKeyEx( hKey, lpSubKey, ulOptions, samDesired, phkResult );
    }

    virtual LONG delete_value( __in HKEY hKey, __in_opt  LPCTSTR lpValueName ){
        return ::RegDeleteValue( hKey, lpValueName );
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
    ){
        return ::RegEnumKeyEx( hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime );
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
        return ::RegEnumValue( hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData );
    }

    virtual LONG query_value(
      __in         HKEY hKey,
      __in_opt     LPCTSTR lpValueName,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpType,
      __out_opt    LPBYTE lpData,
      __inout_opt  LPDWORD lpcbData
    ){
        return ::RegQueryValueEx( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );
    }

    virtual LONG set_value(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpValueName,
      __reserved  DWORD Reserved,
      __in        DWORD dwType,
      __in_opt    const BYTE *lpData,
      __in        DWORD cbData
    ){
        return ::RegSetValueEx( hKey, lpValueName, Reserved, dwType, lpData, cbData );
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
        return ::RegQueryInfoKey( hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime );
    }

    virtual LONG flush_key( __in  HKEY hKey ){
        return ::RegFlushKey( hKey );
    }
};

#ifndef MACHO_HEADER_ONLY

LONG reg_native_edit::delete_key(
    __in        HKEY hKey,
  __in        LPCTSTR lpSubKey,
  __in        REGSAM samDesired,
  __reserved  DWORD Reserved
 ){
    typedef LONG (WINAPI *LPFN_RegDeleteKeyEx)( __in HKEY hKey, __in LPCTSTR lpSubKey, __in REGSAM samDesired, __reserved DWORD Reserved );
    LONG lReturn;
    operating_system os = environment::get_os_version();
    if ( environment::is_wow64_process() && os.is_winxp_or_later() && ( os.is_amd64() || os.is_ia64() ) && 
        ( ( samDesired & KEY_WOW64_64KEY ) || ( _flag & REG_NATIVE_EDIT_WOW64_64KEY ) ) ){
        LPFN_RegDeleteKeyEx fnRegDeleteKeyEx;
#ifdef UNICODE
        fnRegDeleteKeyEx = (LPFN_RegDeleteKeyEx)GetProcAddress(
            GetModuleHandle(TEXT("Advapi32")),"RegDeleteKeyExW");
#else
        fnRegDeleteKeyEx = (LPFN_RegDeleteKeyEx)GetProcAddress(
            GetModuleHandle(TEXT("Advapi32")),"RegDeleteKeyExA");
#endif // !UNICODE
        if (NULL != fnRegDeleteKeyEx)
            lReturn = fnRegDeleteKeyEx( hKey, lpSubKey, (_flag & REG_NATIVE_EDIT_WOW64_64KEY ? KEY_WOW64_64KEY|samDesired : samDesired ), Reserved );
        else
            lReturn = GetLastError();
    }        
    else
        lReturn = RegDeleteKey( hKey, lpSubKey );

    return lReturn;
}

LONG reg_native_edit::delete_tree( __in HKEY hKey, __in_opt  LPCTSTR lpSubKey ){
    typedef LONG (WINAPI *LPFN_RegDeleteTree)( __in HKEY hKey, __in_opt LPCTSTR lpSubKey );
    DWORD   dwRtn = ERROR_SUCCESS;
    LPFN_RegDeleteTree fnRegDeleteTree;

#ifdef UNICODE
    fnRegDeleteTree = (LPFN_RegDeleteTree)GetProcAddress(
        GetModuleHandle(TEXT("Advapi32")),"RegDeleteTreeW");
#else
    fnRegDeleteTree = (LPFN_RegDeleteTree)GetProcAddress(
        GetModuleHandle(TEXT("Advapi32")),"RegDeleteTreeA");
#endif // !UNICODE

    if (NULL != fnRegDeleteTree)
        dwRtn = fnRegDeleteTree( hKey, lpSubKey );
    else{
        DWORD   dwSubKeyLength = MAX_KEY_LENGTH;
        LPTSTR  pSubKey = NULL;
        //TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
        std::auto_ptr<TCHAR> pszSubKey( new TCHAR[MAX_KEY_LENGTH] );
        memset( pszSubKey.get(), 0, MAX_KEY_LENGTH * sizeof(TCHAR) ); 
        HKEY    hSubKey ;
        if ( ERROR_SUCCESS == ( dwRtn = open_key( hKey, lpSubKey, 0, KEY_ENUMERATE_SUB_KEYS| KEY_QUERY_VALUE| KEY_SET_VALUE | DELETE , &hSubKey ) ) ){
            while (dwRtn == ERROR_SUCCESS ){
                dwSubKeyLength = MAX_KEY_LENGTH ;
                // always index zero
                dwRtn = RegEnumKeyEx( hSubKey, 0, pszSubKey.get(), &dwSubKeyLength, NULL, NULL, NULL, NULL );
                if( dwRtn == ERROR_NO_MORE_ITEMS ){
                    if ( lpSubKey &&  lstrlen( lpSubKey ) )
                        dwRtn = delete_key( hKey, lpSubKey, ( _flag & REG_NATIVE_EDIT_WOW64_64KEY ? KEY_WOW64_64KEY : 0  ), 0 );
                    else{
                        // Add code to empty valuse;
                        //TCHAR achValue[MAX_VALUE_NAME];
                        std::auto_ptr<TCHAR> pachValue( new TCHAR[MAX_VALUE_NAME] );
                        DWORD cchValue = MAX_VALUE_NAME; 
                        dwRtn = ERROR_SUCCESS;

                        while( dwRtn == ERROR_SUCCESS ){
                            cchValue = MAX_VALUE_NAME; 
                            memset( pachValue.get(), 0, MAX_VALUE_NAME * sizeof(TCHAR) ); 
                            dwRtn = RegEnumValue( hSubKey, 0, pachValue.get(), &cchValue, NULL, NULL, NULL, NULL);             
                            if ( dwRtn == ERROR_SUCCESS ){ 
                                dwRtn = RegDeleteValue( hSubKey, pachValue.get() );
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
                    dwRtn = delete_tree( hSubKey, pszSubKey.get() );
            }

            RegCloseKey( hSubKey );
            // Do not save return code because error
            // has already occurred
        }
    }
    //if ( dwRtn == ERROR_FILE_NOT_FOUND )
    //    dwRtn = ERROR_SUCCESS;
    return dwRtn;
}


#endif

};//namespace windows
};//namespace macho


#endif