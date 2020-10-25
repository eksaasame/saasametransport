// executable_file_info.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_EXEUTION_FILE_INFO__
#define __MACHO_WINDOWS_EXEUTION_FILE_INFO__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include <Windows.h>

namespace macho{

namespace windows{

class executable_file_info{
public:
    bool inline is_x86() const { return _machine == IMAGE_FILE_MACHINE_I386; } 
    bool inline is_x64() const { return _machine == IMAGE_FILE_MACHINE_AMD64; } 
    bool inline is_ia64() const { return _machine == IMAGE_FILE_MACHINE_IA64; } 
    bool inline is_valid() const { return _machine != 0 ; }
    bool inline is_invalid() const { return !is_valid(); }
    WORD inline cpu_architecture_type() const { return _machine ; }
    static executable_file_info get_executable_file_info( stdstring path );
private:
    executable_file_info() : _machine(0) {}
    static DWORD get_file_type( LPVOID lpFile );
    WORD        _machine;
};

#ifndef MACHO_HEADER_ONLY

#include "..\common\tracelog.hpp"
#include "auto_handle_base.hpp"

#define NTSIGNATURE(ptr) ((LPVOID)((BYTE *)(ptr) + ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew))
#define SIZE_OF_NT_SIGNATURE (sizeof(DWORD))
#define PEFHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE))
#define OPTHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)))
#define SECHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)+sizeof(IMAGE_OPTIONAL_HEADER)))
#define RVATOVA(base,offset) ((LPVOID)((DWORD)(base)+(DWORD)(offset)))
#define VATORVA(base,offset) ((LPVOID)((DWORD)(offset)-(DWORD)(base)))

executable_file_info executable_file_info::get_executable_file_info( stdstring path ){
    executable_file_info _info;
    auto_file_handle _file = CreateFile( path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ( _file.is_invalid() ){
        LOG( LOG_LEVEL_ERROR, TEXT( "Can't open the file with CreateFile(%s) [Error: %d]" ), path.c_str(), GetLastError() );
    }
    else{
        auto_file_handle _file_mapping =  CreateFileMapping(_file, NULL, PAGE_READONLY, 0, 0, NULL);  
        if ( _file_mapping.is_invalid() ){
            LOG( LOG_LEVEL_ERROR, TEXT( "Can't create the file mapping with CreateFileMapping(%s) [Error: %d]" ), path.c_str(), GetLastError() );
        }
        else{
            auto_view_of_file_handle lp_file_base = MapViewOfFile(_file_mapping, FILE_MAP_READ, 0, 0, 0);
            if ( lp_file_base.is_invalid() ){
                LOG( LOG_LEVEL_ERROR, TEXT( "Couldn't map view of file with MapViewOfFile(%s) [Error: %d]" ), path.c_str(), GetLastError() );
            }
            else{
                //PIMAGE_DOS_HEADER lp_dos_header = (PIMAGE_DOS_HEADER)(PVOID)lp_file_base;
                if( IMAGE_NT_SIGNATURE == get_file_type( (PVOID)lp_file_base ) ){
                    PIMAGE_FILE_HEADER lp_file_header = ( PIMAGE_FILE_HEADER ) PEFHDROFFSET((PVOID)lp_file_base) ;
                    _info._machine = lp_file_header->Machine;
                }
            }
        }
    }
    return _info;
}

DWORD executable_file_info::get_file_type ( LPVOID lpFile ){
    /* DOS file signature comes first. */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE){
        /* Determine location of PE File header from DOS header. */
        if (LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE ||
            LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE_LE )
            return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE (lpFile));
        else if (*(DWORD *)NTSIGNATURE (lpFile) == IMAGE_NT_SIGNATURE )
            return IMAGE_NT_SIGNATURE;
        else
            return IMAGE_DOS_SIGNATURE;
    }
    else
        /* unknown file type */
        return 0;
}
#endif

};//namespace windows
};//namespace macho
#endif