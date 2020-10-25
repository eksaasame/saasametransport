// volume.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_VOLUME__
#define __MACHO_WINDOWS_VOLUME__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "..\common\tracelog.hpp"
#include "memory"
#include "boost\shared_ptr.hpp"

namespace macho{

namespace windows{

struct    volume_exception : virtual public exception_base {};
#define BOOST_THROW_VOLUME_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( volume_exception, no, message )

struct disk_extent{
    disk_extent() : disk_number(-1) , starting_offset(0), extent_length(0) {}
    disk_extent( DWORD number, ULONGLONG offset, ULONGLONG length ) 
        : disk_number(number), starting_offset( offset ), extent_length( length ) {} 
    disk_extent( const disk_extent& extent ){
        copy( extent );
    }
    void copy ( const disk_extent& extent ){
        disk_number     = extent.disk_number;
        starting_offset = extent.starting_offset;
        extent_length   = extent.extent_length;
    }
    const disk_extent &operator =( const disk_extent& extent ){
        if ( this != &extent )
            copy( extent );
        return( *this );
    }
    DWORD      disk_number;
    ULONGLONG  starting_offset;
    ULONGLONG  extent_length;
};

typedef std::vector< disk_extent > extent_table;

class   volume;
typedef std::vector< volume > volume_table;

class volume_implement{
public:
    volume_implement(): _drive_type(0), _capacity(0), _free_space(0), _is_dirty_bit_set(false), _is_system(false), _serial_number(0) {}
    volume_implement( const volume_implement& _volume ){
        copy( _volume );
    }
    void copy ( const volume_implement& _volume ){
        _drive_type      = _volume._drive_type;
        _serial_number   = _volume._serial_number;
        _device_id       = _volume._device_id;
        _name            = _volume._name;
        _label           = _volume._label;
        _drive_letter    = _volume._drive_letter;
        _file_system     = _volume._file_system;
        _capacity        = _volume._capacity;
        _free_space      = _volume._free_space;
        _is_dirty_bit_set   = _volume._is_dirty_bit_set;
        _is_system       = _volume._is_system;
        _mount_points    = _volume._mount_points;
        _disk_extents    = _volume._disk_extents;
    }
    const volume_implement &operator =( const volume_implement& _volume ){
        if ( this != &_volume )
            copy( _volume );
        return( *this );
    }
private:
    static volume_implement   get_volume( stdstring device_id );
    static bool               get_volume( stdstring device_id, volume_implement& volume );
    static bool               get_mount_points( stdstring device_id, string_table& mount_points );
    static bool               get_disk_extents( stdstring device_id, extent_table& disk_extents );
    friend class volume;
    DWORD                                    _drive_type;
    DWORD                                    _serial_number;
    stdstring                                _device_id;
    stdstring                                _name;
    stdstring                                _label;
    stdstring                                _drive_letter;
    stdstring                                _file_system;
    ULONGLONG                                _capacity;
    ULONGLONG                                _free_space;
    bool                                     _is_dirty_bit_set; // unused for winxp.
    bool                                     _is_system;
    string_table                             _mount_points;
    extent_table                             _disk_extents;
};

class volume{
public:
    DWORD        inline drive_type()       const { return _impl._drive_type; }
    DWORD        inline serial_number()    const { return _impl._serial_number; }
    stdstring    inline device_id()        const { return _impl._device_id; }
    stdstring    inline name()             const { return _impl._name; }
    stdstring    inline label()            const { return _impl._label; }
    stdstring    inline drive_letter()     const { return _impl._drive_letter; }
    stdstring    inline file_system()      const { return _impl._file_system; }
    ULONGLONG    inline capacity()         const { return _impl._capacity; }
    ULONGLONG    inline free_space()       const { return _impl._free_space; }
    bool         inline is_dirty_bit_set() const { return _impl._is_dirty_bit_set; } // unused for winxp.
    bool         inline is_system()        const { return _impl._is_system; }
    string_table inline mount_points()     const { return _impl._mount_points; }
    extent_table inline disk_extents()     const { return _impl._disk_extents; }

    volume( const volume_implement& impl ) {
        _impl = impl;
    }
    volume ( const volume& _volume ){
        copy(_volume);
    }
    void copy ( const volume& _volume ){
        _impl = _volume._impl;
    }
    const volume &operator =( const volume& _volume ){
        if ( this != &_volume )
            copy( _volume );
        return( *this );
    }
    const volume &operator =( const volume_implement& impl ){
        _impl = impl;
        return( *this );
    }
    static volume       get_volume( stdstring path );
    static volume_table get_volumes();
    
    bool                refresh();

private:
    volume_implement _impl;
};

#ifndef MACHO_HEADER_ONLY
#include "..\common\tracelog.hpp"
#include "..\common\stringutils.hpp"
#include "auto_handle_base.hpp"
#include "environment.hpp"
#include "com_init.hpp"
#include "wmi.hpp"

/*********************************************************************
    volume_implement class
**********************************************************************/
volume_implement volume_implement::get_volume( stdstring device_id ){
    volume_implement impl;
    if ( !get_volume( device_id, impl ) ){
#if _UNICODE  
         BOOST_THROW_VOLUME_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), boost::str( boost::wformat( L"Can't get the volume (%s) info.") % device_id ) );
#else
         BOOST_THROW_VOLUME_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), boost::str( boost::format( "Can't get the volume (%s) info.") % device_id ) );
#endif
    }
    return impl;
}

bool volume_implement::get_volume( stdstring device_id, volume_implement& volume ){  
    bool  result = false;
    TCHAR file_system[MAX_PATH+1];
    TCHAR volume_label[MAX_PATH+1];
    memset( file_system, 0 , sizeof(file_system));
    memset( volume_label, 0 , sizeof(volume_label));
    volume._drive_type  = GetDriveType( device_id.c_str());
    if ( result = get_mount_points( device_id, volume._mount_points ) ){
        foreach( stdstring mount_point, volume._mount_points ){
            if ( ( 0 < mount_point.length() )      &&
                    ( 3 >= mount_point.length() )     &&
                    ( IsCharAlpha( mount_point[0] ) ) &&
                    ( _T(':')  == mount_point[1]   )  &&
                    ((_T('\0') == mount_point[2]) || ((_T('\\') == mount_point[2])))){
                volume._name = volume._drive_letter =  mount_point;
                volume._drive_letter.erase( 2 ); 
            }
        }
        stdstring win_dir = environment::get_windows_directory();
        volume._is_system =  ( ( volume._drive_letter.length() > 0 ) && ( 0 == win_dir.find(volume._drive_letter) ) );
        if ( 3 == volume._drive_type || 2 == volume._drive_type )
            result = get_disk_extents( device_id, volume._disk_extents );
        if ( !result ){
        } 
        else if ( !( result = ( TRUE == GetVolumeInformation( device_id.c_str(), volume_label, MAX_PATH, &volume._serial_number, NULL, NULL, file_system, MAX_PATH ) ) ) ){
            LOG( LOG_LEVEL_ERROR, TEXT( "Failed to GetVolumeInformation (%s) [Error: %d]" ), device_id.c_str(), GetLastError() );
        }
        else if ( !( result = ( TRUE == GetDiskFreeSpaceEx( device_id.c_str(), NULL, (PULARGE_INTEGER) &volume._capacity, (PULARGE_INTEGER) &volume._free_space ) ) ) ){
            LOG( LOG_LEVEL_ERROR, TEXT( "Failed to GetDiskFreeSpaceEx (%s) [Error: %d]" ), device_id.c_str(), GetLastError() );
        }
        else{
            volume._name = volume._device_id  = device_id;
            volume._file_system = file_system;
            volume._label       = volume_label;
        }
    }
    return result;
}

bool volume_implement::get_mount_points( stdstring device_id, string_table& mount_points ){
    
    DWORD                    CharCount = MAX_PATH + 1;
    std::auto_ptr<TCHAR>    pNames;
    TCHAR*                    NameIdx   = NULL;
    BOOL                    Success   = FALSE;
    mount_points.clear();
    if( device_id[ device_id.length()-1 ] != _T('\\') )
        device_id.append(_T("\\"));

    for (;;) {
        //
        //  Allocate a buffer to hold the paths.
        pNames = std::auto_ptr<TCHAR>( new TCHAR[CharCount] );

        if ( !pNames.get() ) //  If memory can't be allocated, return.
            return false;
        //  Obtain all of the paths for this volume.
        if ( Success = GetVolumePathNamesForVolumeName( 
            device_id.c_str(), pNames.get(), CharCount, &CharCount ) )
            break;
        else if ( GetLastError() != ERROR_MORE_DATA ) 
            break;
    }

    if ( Success ){
        //  Get the various paths.
        for ( NameIdx = pNames.get(); 
              NameIdx[0] != _T('\0'); 
              NameIdx += _tcslen(NameIdx) + 1 ){
            mount_points.push_back( stdstring(NameIdx) );
        }
    }
    return ( TRUE == Success );
}

bool volume_implement::get_disk_extents( stdstring device_id, extent_table& disk_extents ){
    BOOL result = TRUE;
    disk_extents.clear();
    std::auto_ptr<VOLUME_DISK_EXTENTS> pDiskExtents( ( VOLUME_DISK_EXTENTS *) new BYTE[ sizeof(VOLUME_DISK_EXTENTS) ] );
    memset( pDiskExtents.get(), 0 , sizeof(VOLUME_DISK_EXTENTS));
    if ( ( 5 > device_id.length() ) || 
        ( device_id[0]     != _T('\\') ||
         device_id[1]      != _T('\\') ||
         device_id[2]      != _T('?')  ||
         device_id[3]      != _T('\\') ||
         device_id[device_id.length()-1] != _T('\\') ) ) {
#if _UNICODE  
         BOOST_THROW_VOLUME_EXCEPTION( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), boost::str( boost::wformat( L"Invalid volume name (%s).") % device_id ) );
#else
         BOOST_THROW_VOLUME_EXCEPTION( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), boost::str( boost::format( "Invalid volume name (%s).") % device_id ) );
#endif
         result = FALSE;
    }
    else{
        device_id.erase(device_id.length()-1);
        auto_file_handle device_handle = CreateFile( device_id.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( device_handle.is_invalid() ){
            LOG( LOG_LEVEL_ERROR, _T("Can't access volume (%s). error( 0x%08X )"), device_id.c_str(), GetLastError() ); 
            result = FALSE;
        }
        else{
            DWORD bufferSize;
            if ( ! ( result = DeviceIoControl( device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), sizeof( VOLUME_DISK_EXTENTS ), &bufferSize, 0 ) ) ){
                DWORD error = GetLastError();
                if ( ( ( error == ERROR_INSUFFICIENT_BUFFER ) || ( error == ERROR_MORE_DATA ) ) && ( pDiskExtents->NumberOfDiskExtents > 1 ) ){
                    bufferSize = sizeof(VOLUME_DISK_EXTENTS) + ( pDiskExtents->NumberOfDiskExtents*sizeof(DISK_EXTENT) );
                    pDiskExtents = std::auto_ptr<VOLUME_DISK_EXTENTS> ( ( VOLUME_DISK_EXTENTS *) new BYTE[bufferSize] );
                    if ( ! ( result = DeviceIoControl( device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), bufferSize, &bufferSize, 0 ) ) ){
                        LOG( LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), device_id.c_str(), GetLastError() );
                    }
                }
                else{
                    LOG( LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), device_id.c_str(), error );
                }
            }
        }
    }

    for( DWORD i = 0 ; i < pDiskExtents.get()->NumberOfDiskExtents; i ++ ){
        disk_extents.push_back( disk_extent( pDiskExtents.get()->Extents[i].DiskNumber, pDiskExtents.get()->Extents[i].StartingOffset.QuadPart, pDiskExtents.get()->Extents[i].ExtentLength.QuadPart ) );
    }
    return ( TRUE == result );
}

/*********************************************************************
    volume class
**********************************************************************/
bool volume::refresh(){
    bool result = false;
    if ( _impl._device_id.length() ){  
        operating_system os = environment::get_os_version();
        if ( os.is_win2003_or_later() && !environment::is_winpe() ) {
            com_init     _init;  
            wmi_services wmi;
            wmi.connect( L"cimv2" );
#if _UNICODE  
            std::wstring w_device_id  = _impl._device_id;
#else
            std::wstring w_device_id  = stringutils::convert_ansi_to_unicode( _impl._device_id );
#endif
            w_device_id.erase(w_device_id.length()-1);
            wmi_object_table win32_volumes = wmi.exec_query( boost::str( boost::wformat( L"Select * from Win32_Volume Where DeviceID = '\\\\\\\\?\\\\%s\\\\'" ) % &w_device_id[4] ) );    
            if ( win32_volumes.size() ){
                volume_implement impl;
                result = true;
                stdstring        win_dir = environment::get_windows_directory();  
                impl._drive_type       = win32_volumes[0][L"DriveType"];
                impl._capacity         = win32_volumes[0][L"Capacity"];
                impl._free_space       = win32_volumes[0][L"FreeSpace"];
                impl._is_dirty_bit_set = win32_volumes[0][L"DirtyBitSet"];
                impl._serial_number    = win32_volumes[0][L"SerialNumber"];
#if _UNICODE  
                impl._device_id     = win32_volumes[0][L"DeviceID"];
                impl._name          = win32_volumes[0][L"Name"];
                impl._label         = win32_volumes[0][L"Label"];
                impl._drive_letter  = win32_volumes[0][L"DriveLetter"];
                impl._file_system   = win32_volumes[0][L"FileSystem"];
#else
                impl._device_id     = static_cast< std::string >(stringutils::convert_unicode_to_ansi((LPCWSTR)win32_volumes[0][L"DeviceID"]));
                impl._name          = static_cast< std::string >(stringutils::convert_unicode_to_ansi((LPCWSTR)win32_volumes[0][L"Name"]));
                impl._label         = static_cast< std::string >(stringutils::convert_unicode_to_ansi((LPCWSTR)win32_volumes[0][L"Label"]));
                impl._drive_letter  = static_cast< std::string >(stringutils::convert_unicode_to_ansi((LPCWSTR)win32_volumes[0][L"DriveLetter"]));
                impl._file_system   = static_cast< std::string >(stringutils::convert_unicode_to_ansi((LPCWSTR)win32_volumes[0][L"FileSystem"]));            
#endif
                impl._is_system =  ( ( impl._drive_letter.length() > 0 ) && ( 0 == win_dir.find(impl._drive_letter) ) );
                
                if ( 3 == impl._drive_type || 2 == impl._drive_type ){
                    result = volume_implement::get_disk_extents( impl._device_id, impl._disk_extents );
                }

                if ( result && ( result = volume_implement::get_mount_points( impl._device_id, impl._mount_points ) ) ){
                    _impl = impl;
                }
            }
        }
        else{
            volume_implement impl;
            if ( result = volume_implement::get_volume(_impl._device_id, impl ) ){
                _impl = impl;
            }
        }
    }
    return result;
}

volume volume::get_volume( stdstring path ){
    int length = (int)path.length() + 2;
    std::auto_ptr<TCHAR> pVolumePath = std::auto_ptr<TCHAR>(new TCHAR[length]);
    if ( pVolumePath.get() != NULL ){
        memset( pVolumePath.get(), 0 , length * sizeof(TCHAR) );
        if ( GetVolumePathName( path.c_str(), pVolumePath.get(), length ) ){
            path = pVolumePath.get();
        }
        else{
            LOG( LOG_LEVEL_ERROR, _T("Can't get volume path name for path (%s). error( 0x%08X )"), path.c_str(), GetLastError() );
        }
    }
    if ( ( 5 > path.length() ) || 
      ( path[0]      != _T('\\') ||
        path[1]      != _T('\\') ||
        path[2]      != _T('?')  ||
        path[3]      != _T('\\') ||
        path[path.length()-1] != _T('\\') ) ) {
        TCHAR volumeName[MAX_PATH] = {0};
        if ( GetVolumeNameForVolumeMountPoint( path.c_str(), volumeName,  MAX_PATH ) ){
            return volume_implement::get_volume(volumeName);
        }
        else{
            LOG( LOG_LEVEL_ERROR, _T("Can't get volume name for volume mount point (%s). error( 0x%08X )"), path.c_str(), GetLastError() );
        }
    }
    return volume_implement::get_volume(path);
}

volume_table volume::get_volumes(){
    volume_table volumes;
    operating_system os = environment::get_os_version();
    if ( os.is_win2003_or_later() && !environment::is_winpe() ) {
        com_init     _init;
        stdstring    win_dir = environment::get_windows_directory();
        wmi_services wmi;
        wmi.connect( L"cimv2" );
        wmi_object_table win32_volumes = wmi.exec_query( boost::str( boost::wformat( L"Select * from Win32_Volume" ) ) );
        foreach ( wmi_object vol, win32_volumes ){
            volume_implement impl;
            impl._drive_type       = vol[L"DriveType"];
            impl._capacity         = vol[L"Capacity"];
            impl._free_space       = vol[L"FreeSpace"];
            impl._is_dirty_bit_set = vol[L"DirtyBitSet"];
            impl._serial_number    = vol[L"SerialNumber"];
#if _UNICODE  
            impl._device_id     = vol[L"DeviceID"];
            impl._name          = vol[L"Name"];
            impl._label         = vol[L"Label"];
            impl._drive_letter  = vol[L"DriveLetter"];
            impl._file_system   = vol[L"FileSystem"];
#else
            impl._device_id     = static_cast< std::string >(stringutils::convert_unicode_to_ansi(vol[L"DeviceID"]));
            impl._name          = static_cast< std::string >(stringutils::convert_unicode_to_ansi(vol[L"Name"]));
            impl._label         = static_cast< std::string >(stringutils::convert_unicode_to_ansi(vol[L"Label"]));
            impl._drive_letter  = static_cast< std::string >(stringutils::convert_unicode_to_ansi(vol[L"DriveLetter"]));
            impl._file_system   = static_cast< std::string >(stringutils::convert_unicode_to_ansi(vol[L"FileSystem"]));            
#endif
            impl._is_system =  ( ( impl._drive_letter.length() > 0 ) && ( 0 == win_dir.find(impl._drive_letter) ) );
            bool result = true;
            if ( 3 == impl._drive_type || 2 == impl._drive_type ){
                result = volume_implement::get_disk_extents( impl._device_id, impl._disk_extents );
            }
            if ( result && volume_implement::get_mount_points( impl._device_id, impl._mount_points ) )   
                volumes.push_back(impl);
        }
    }
    else {
        TCHAR volume_name[MAX_PATH];
        memset( volume_name, 0, sizeof( volume_name ) );
        auto_find_volume_handle handle = FindFirstVolume( volume_name, ARRAYSIZE( volume_name ) );
        if ( handle.is_invalid() ){
             LOG( LOG_LEVEL_ERROR, TEXT( "Failed to find first volume info [Error: %d]" ), GetLastError() );
        }
        else{

            for(;;){
                size_t index = _tcsclen( volume_name ) - 1;
                if ( volume_name[0]      != TEXT( '\\' ) ||
                     volume_name[1]      != TEXT( '\\' ) ||
                     volume_name[2]      != TEXT( '?'  ) ||
                     volume_name[3]      != TEXT( '\\' ) ||
                     volume_name[index]  != TEXT( '\\' ) ) {    
                    LOG( LOG_LEVEL_ERROR, TEXT( "Invalid path of FindFirstVolume()/FindNextVolume() returned: %s" ), volume_name );
                }
                else{
                    volume_implement impl;
                    if ( volume_implement::get_volume(volume_name, impl) )
                        volumes.push_back( impl );    
                }
                // Move on to the next volume
                if ( !FindNextVolume( handle, volume_name, ARRAYSIZE( volume_name ) ) ){
                    DWORD error = GetLastError();
                    if ( error == ERROR_NO_MORE_FILES )
                        break;
                    LOG( LOG_LEVEL_ERROR, TEXT( "Failed to find next volume [Error: %d]" ), error );
                    break;
                }
            }
        }
    }
    return volumes;
}

#endif

};

};

#endif