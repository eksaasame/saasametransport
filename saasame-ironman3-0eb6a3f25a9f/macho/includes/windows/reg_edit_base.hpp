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

#ifndef __MACHO_WINDOWS_REG_EDIT_BASE__
#define __MACHO_WINDOWS_REG_EDIT_BASE__

#include "..\config\config.hpp"

namespace macho{

namespace windows{

#ifndef MAX_KEY_LENGTH
#define MAX_KEY_LENGTH MAX_PATH
#endif
#define MAX_VALUE_NAME 16383
#define REG_DELETE  ( 0xFF ) // Will be Deleted Key Value Type

class reg_edit_base {

public:
    virtual LONG close_key(    __in  HKEY hKey ) = NULL;
    virtual LONG delete_key( __in  HKEY hKey, __in  LPCTSTR lpSubKey ) = NULL;

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
    )= NULL;

    virtual LONG open_key(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpSubKey,
      __reserved  DWORD ulOptions,
      __in        REGSAM samDesired,
      __out       PHKEY phkResult
    )= NULL;

    virtual LONG delete_value( __in HKEY hKey, __in_opt  LPCTSTR lpValueName )= NULL;
    virtual LONG delete_tree( __in HKEY hKey, __in_opt  LPCTSTR lpSubKey )= NULL;

    virtual LONG enumerate_key(
      __in         HKEY hKey,
      __in         DWORD dwIndex,
      __out        LPTSTR lpName,
      __inout      LPDWORD lpcName,
      __reserved   LPDWORD lpReserved,
      __inout      LPTSTR lpClass,
      __inout_opt  LPDWORD lpcClass,
      __out_opt    PFILETIME lpftLastWriteTime
    )= NULL;

    virtual LONG enumerate_value(
      __in         HKEY hKey,
      __in         DWORD dwIndex,
      __out        LPTSTR lpValueName,
      __inout      LPDWORD lpcchValueName,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpType,
      __out_opt    LPBYTE lpData,
      __inout_opt  LPDWORD lpcbData
    )= NULL;

    virtual LONG query_value(
      __in         HKEY hKey,
      __in_opt     LPCTSTR lpValueName,
      __reserved   LPDWORD lpReserved,
      __out_opt    LPDWORD lpType,
      __out_opt    LPBYTE lpData,
      __inout_opt  LPDWORD lpcbData
    )= NULL;

    virtual LONG set_value(
      __in        HKEY hKey,
      __in_opt    LPCTSTR lpValueName,
      __reserved  DWORD Reserved,
      __in        DWORD dwType,
      __in_opt    const BYTE *lpData,
      __in        DWORD cbData
    )= NULL;

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
    )= NULL;

    virtual LONG delete_key(
      __in        HKEY hKey,
      __in        LPCTSTR lpSubKey,
      __in        REGSAM samDesired,
      __reserved  DWORD Reserved
     ) = NULL;
    
    virtual LONG flush_key( __in  HKEY hKey ) = NULL;
    reg_edit_base() { }
    reg_edit_base( reg_edit_base& edit ) {
        copy( edit );
    }
    virtual void copy ( const reg_edit_base& edit ) { 
    }
    virtual const reg_edit_base &operator =( const reg_edit_base& edit  ) {
        if ( this != &edit )
            copy( edit );
        return( *this );
    }
    virtual~reg_edit_base(){}
};

};//namespace windows
};//namespace macho


#endif