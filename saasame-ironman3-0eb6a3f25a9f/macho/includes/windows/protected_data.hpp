// protected_data.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_PROTECTED_DATA__
#define __MACHO_WINDOWS_PROTECTED_DATA__

#include "..\config\config.hpp"
#include "..\common\bytes.hpp"
#include "..\common\stringutils.hpp"

#include <Wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

namespace macho{

namespace windows{

class protected_data{
public:
    protected_data(bool per_machine = false) : _per_machine(per_machine){}
    protected_data(std::wstring value) : _per_machine(false){
        (*this) = value;
    }
    protected_data(std::string value) : _per_machine(false){
        (*this) = value;
    }
    protected_data(LPSTR value) : _per_machine(false){
        (*this) = value;
    }
    protected_data(LPWSTR value) : _per_machine(false){
        (*this) = value;
    }
    virtual ~protected_data(){}
    static bytes protect( bytes &source, bool per_machine = false, std::wstring description = L"" );
    static bytes unprotect( bytes &source, bool per_machine = false, std::wstring &description = std::wstring() );
    protected_data&    operator =(LPCWSTR value) { return (*this = std::wstring(value)); }
    protected_data&    operator =(LPWSTR value) { return (*this = std::wstring(value)); }
    protected_data&    operator =(const std::wstring value){
        if (value.length()){
            bytes source;
            source.set((LPBYTE)value.c_str(), sizeof(wchar_t)*value.length());
            _data = protect(source, _per_machine);
        }
        return *this;
    }
    protected_data&    operator =(LPCSTR value) { return (*this = std::string(value)); }
    protected_data&    operator =(LPSTR value) { return (*this = std::string(value)); }
    protected_data&    operator =(const std::string value){
        std::wstring _value = stringutils::convert_ansi_to_unicode(value);
        return  (*this = _value);
    }
    operator std::wstring(){
        bytes value = unprotect(_data, _per_machine);
        if (value.length() && value.ptr()){
            size_t len = value.length() / 2;
            return std::wstring(reinterpret_cast<wchar_t const*>(value.ptr()), len);
        }
        return L"";
    }
    operator std::string(){ 
        std::wstring value = this->operator std::wstring();
        return stringutils::convert_unicode_to_ansi(value);
    }
    std::string    string(){ return operator std::string(); }
    std::wstring   wstring(){ return operator std::wstring(); }
private:
    bool  _per_machine;
    bytes _data;
};

#ifndef MACHO_HEADER_ONLY

bytes protected_data::protect( bytes &unprotected_src, bool per_machine, std::wstring description ){

    bytes data;
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    DataIn.pbData = unprotected_src.ptr();
    DataIn.cbData = (DWORD)unprotected_src.length();
    if(CryptProtectData(
         &DataIn,
         description.c_str(),                   // A description string. 
         NULL,                                  // Optional entropy
         NULL,                                  // Reserved.
         NULL,                                  // Pass a PromptStruct.
         per_machine ? CRYPTPROTECT_LOCAL_MACHINE : 0,
         &DataOut)){
        data.set( DataOut.pbData, DataOut.cbData );
        LocalFree( DataOut.pbData);
    }
    return data;
}

bytes protected_data::unprotect( bytes &protected_src, bool per_machine, std::wstring &description ){

    bytes data;
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    DataIn.pbData = protected_src.ptr();
    DataIn.cbData = (DWORD)protected_src.length();
    LPWSTR pDescrOut =  NULL;
    if(CryptUnprotectData(
         &DataIn,
         &pDescrOut,                            // A description string. 
         NULL,                                  // Optional entropy
         NULL,                                  // Reserved.
         NULL,                                  // Pass a PromptStruct.
         per_machine ? CRYPTPROTECT_LOCAL_MACHINE : 0,
         &DataOut)){
        data.set( DataOut.pbData, DataOut.cbData );
        description = pDescrOut;
        LocalFree( pDescrOut );
        LocalFree( DataOut.pbData);
    }
    return data;
}

#endif

};//namespace windows
};//namespace macho

#endif