// file_version_info.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_FILE_VERSION_INFO__
#define __MACHO_WINDOWS_FILE_VERSION_INFO__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"

namespace macho{

namespace windows{

#pragma comment(lib, "Version.lib")    
struct  file_version_info_exception :  public exception_base {};
#define BOOST_THROW_FILE_VERSION_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( file_version_info_exception, no, message )
  
class file_version_info{
public:
    WORD       inline file_version_major()       const { return HIWORD( _file_version_ms ) ;}
    WORD       inline file_version_minor()       const { return LOWORD( _file_version_ms ) ;}
    WORD       inline file_version_build()       const { return HIWORD( _file_version_ls ) ;}
    WORD       inline file_version_revision()    const { return LOWORD( _file_version_ls ) ;}
    WORD       inline product_version_major()    const { return HIWORD( _product_version_ms ) ;}
    WORD       inline product_version_minor()    const { return LOWORD( _product_version_ms ) ;}
    WORD       inline product_version_build()    const { return HIWORD( _product_version_ls ) ;}
    WORD       inline product_version_revision() const { return LOWORD( _product_version_ls ) ;}
    ULONGLONG  inline file_version()             const { return MAKEDLLVERULL( file_version_major(), file_version_minor(), file_version_build(), file_version_revision()); }
    ULONGLONG  inline product_version()          const { return MAKEDLLVERULL( product_version_major(), product_version_minor(), product_version_build(), product_version_revision()); }
    WORD       inline language()                 const { return _language; }
    WORD       inline charset()                     const { return _charset;  }

    DWORD      inline file_os()                  const { return _file_os; }
    DWORD      inline file_type()                const { return _file_type; }
    DWORD      inline file_subtype()             const { return _file_subtype; }
        
    stdstring  inline company_name()             const { return _company_name; }
    stdstring  inline product_name()             const { return _product_name; }
    stdstring  inline internal_name()            const { return _internal_name; }
    stdstring  inline original_file_name()       const { return _original_file_name; }
    stdstring  inline legal_copy_right()         const { return _legal_copy_right; }

    static file_version_info get_file_version_info( stdstring path );
private:
    file_version_info() : 
       _file_version_ms(0), 
       _file_version_ls(0), 
       _product_version_ms(0),
       _product_version_ls(0),
       _file_flags_mask(0),
       _file_flags(0),
       _file_os(0),
       _file_type(0),
       _file_subtype(0),
       _file_date_ms(0),
       _file_date_ls(0)
       {}
    DWORD           _file_version_ms;
    DWORD           _file_version_ls;
    DWORD           _product_version_ms;
    DWORD           _product_version_ls;
    DWORD           _file_flags_mask;
    DWORD           _file_flags;
    DWORD           _file_os;
    DWORD           _file_type;
    DWORD           _file_subtype;
    DWORD           _file_date_ms;
    DWORD           _file_date_ls;
    WORD            _language;
    WORD            _charset;
    stdstring       _company_name;
    stdstring       _product_name;
    stdstring        _internal_name;
    stdstring        _original_file_name;
    stdstring        _legal_copy_right;
};

#ifndef MACHO_HEADER_ONLY

#include "..\common\tracelog.hpp"

file_version_info file_version_info::get_file_version_info( stdstring path ){
    
    file_version_info version_info;
    DWORD dwLen, dwUseless;
    if ( path.length() == 0 ) {
        LOG( LOG_LEVEL_ERROR, TEXT( "File path is empty" ) );
        BOOST_THROW_FILE_VERSION_EXCEPTION( ERROR_INVALID_PARAMETER, _T("File path is empty.") );
    }
    else if (  0 == ( dwLen = GetFileVersionInfoSize((LPTSTR)path.c_str(), &dwUseless) ) ){
        LOG( LOG_LEVEL_ERROR, TEXT( "Can't get the file version info (%s) [Error: %d]" ), path.c_str(), GetLastError() );
#if _UNICODE  
        BOOST_THROW_FILE_VERSION_EXCEPTION( GetLastError(), boost::str( boost::wformat(L"Can't get the file version info (%s)") %path ) );
#else
        BOOST_THROW_FILE_VERSION_EXCEPTION( GetLastError(), boost::str( boost::format("Can't get the file version info (%s)") %path ) );
#endif
    }
    else{
        LPTSTR lpVI;
        lpVI = (LPTSTR) GlobalAlloc(GPTR, dwLen);
        if (lpVI){
            DWORD dwBufSize;
            VS_FIXEDFILEINFO* lpFFI;
            BOOL bRet = FALSE;
            WORD* langInfo;
            UINT cbLang;
            TCHAR tszVerStrName[128];
            LPVOID lpt;
            UINT cbBufSize;
            GetFileVersionInfo((LPTSTR)path.c_str(), NULL, dwLen, lpVI);
            if (VerQueryValue(lpVI, _T("\\"), (LPVOID *) &lpFFI, (UINT *) &dwBufSize)) {
                //We now have the VS_FIXEDFILEINFO in lpFFI
                version_info._file_version_ms = lpFFI->dwFileVersionMS;
                version_info._file_version_ls = lpFFI->dwFileVersionLS;
                version_info._product_version_ms = lpFFI->dwProductVersionMS;
                version_info._product_version_ls = lpFFI->dwProductVersionLS;
                version_info._file_flags_mask    = lpFFI->dwFileFlagsMask;
                version_info._file_flags         = lpFFI->dwFileFlags;
                version_info._file_os            = lpFFI->dwFileOS;
                version_info._file_type          = lpFFI->dwFileType;
                version_info._file_subtype       = lpFFI->dwFileSubtype;
                version_info._file_date_ms       = lpFFI->dwFileDateMS;
                version_info._file_date_ls       = lpFFI->dwFileDateLS;
            }
            //Get the Company Name.
            //First, to get string information, we need to get
            //language information.
            VerQueryValue(lpVI, _T("\\VarFileInfo\\Translation"),(LPVOID*)&langInfo, &cbLang);
            //Prepare the label -- default lang is bytes 0 & 1
            //of langInfo
            if ( cbLang > 1 ){
                version_info._language = langInfo[0];
                version_info._charset  = langInfo[1];
                _stprintf_s(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),langInfo[0], langInfo[1], _T("CompanyName"));
                //Get the string from the resource data
                if (VerQueryValue(lpVI, tszVerStrName, &lpt, &cbBufSize))
                    version_info._company_name.assign((LPTSTR)lpt);    //*must* save this

                _stprintf_s(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),langInfo[0], langInfo[1], _T("ProductName"));
                if (VerQueryValue(lpVI, tszVerStrName, &lpt, &cbBufSize))
                    version_info._product_name.assign((LPTSTR)lpt);    //*must* save this

                _stprintf_s(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),langInfo[0], langInfo[1], _T("InternalName"));
                if (VerQueryValue(lpVI, tszVerStrName, &lpt, &cbBufSize))
                    version_info._internal_name.assign((LPTSTR)lpt);    //*must* save this

                _stprintf_s(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),langInfo[0], langInfo[1], _T("LegalCopyright"));
                if (VerQueryValue(lpVI, tszVerStrName, &lpt, &cbBufSize))
                    version_info._legal_copy_right.assign((LPTSTR)lpt);    //*must* save this

                _stprintf_s(tszVerStrName, _T("\\StringFileInfo\\%04x%04x\\%s"),langInfo[0], langInfo[1], _T("OriginalFilename"));
                if (VerQueryValue(lpVI, tszVerStrName, &lpt, &cbBufSize))
                    version_info._original_file_name.assign((LPTSTR)lpt);    //*must* save this
            }
            //Cleanup
            GlobalFree((HGLOBAL)lpVI);
        } 
    }
    return version_info;
}

#endif

};//namespace windows
};//namespace macho
#endif