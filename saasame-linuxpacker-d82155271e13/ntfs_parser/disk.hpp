// disk.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_DISK__
#define __MACHO_WINDOWS_DISK__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "..\common\tracelog.hpp"
#include "memory"
#include "boost\shared_ptr.hpp"
#include "volume.hpp"
#include <setupapi.h>   // for SetupDiXxx functions.
#include "WinIoCtl.h"

#pragma comment(lib, "setupapi.lib")

namespace macho{

namespace windows{

DEFINE_GUID(PARTITION_ENTRY_UNUSED_GUID,   0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);    // Entry unused 
DEFINE_GUID(PARTITION_SYSTEM_GUID,         0xC12A7328L, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B);    // EFI system partition 
DEFINE_GUID(PARTITION_MSFT_RESERVED_GUID,  0xE3C9E316L, 0x0B5C, 0x4DB8, 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE);    // Microsoft reserved space                                         
DEFINE_GUID(PARTITION_BASIC_DATA_GUID,     0xEBD0A0A2L, 0xB9E5, 0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7);    // Basic data partition 
DEFINE_GUID(PARTITION_LDM_METADATA_GUID,   0x5808C8AAL, 0x7E8F, 0x42E0, 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3);    // Logical Disk Manager metadata partition 
DEFINE_GUID(PARTITION_LDM_DATA_GUID,       0xAF9B60A0L, 0x1431, 0x4F62, 0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD);    // Logical Disk Manager data partition 
DEFINE_GUID(PARTITION_MSFT_RECOVERY_GUID,  0xDE94BBA4L, 0x06D1, 0x4D40, 0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC);    // Microsoft recovery partition

struct  disk_exception :  public exception_base {};
#define BOOST_THROW_DISK_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( disk_exception, no, message )
#define BOOST_THROW_DISK_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING( disk_exception, message)        
class disk;
typedef std::vector< disk > disk_table;

class disk_implement{

public:
    disk_implement() 
        :
        _port_number( 0 )
        , _path_id( 0 )
        , _target_id( 0 )
        , _lun( 0 )
        ,_number( -1 )
        , _is_readable( false )
        , _is_remove_media( false )
        , _is_dynamic_disk( false )
        , _is_bootable ( false )
        , _length( 0 )
        , _device_type ( 0x0f )
        , _bus_type( BusTypeUnknown )
        , _partition_style(PARTITION_STYLE_RAW) {
        memset( &_geometry, 0, sizeof(DISK_GEOMETRY_EX)); 
    }
    disk_implement( const disk_implement& _disk ){
        copy( _disk );
    }
    virtual ~disk_implement(){}
    void copy ( const disk_implement& _disk ){
        _port_number                    = _disk._port_number;
        _path_id                        = _disk._path_id;
        _target_id                      = _disk._target_id;
        _lun                            = _disk._lun;
        _number                         = _disk._number;
        _device_path                    = _disk._device_path;
        _length                         = _disk._length;
        _device_type                    = _disk._device_type;
        _bus_type                       = _disk._bus_type;
        _vendor_id                      = _disk._vendor_id;
        _product_id                     = _disk._product_id;
        _product_revision               = _disk._product_revision;
        _serial_number                  = _disk._serial_number;
        _vendor_string                  = _disk._vendor_string;              
        _disk_id                        = _disk._disk_id;
        _partition_style                = _disk._partition_style;
        _is_readable                    = _disk._is_readable;
        _is_remove_media                = _disk._is_remove_media;
        _is_dynamic_disk                = _disk._is_dynamic_disk;
        _is_bootable                    = _disk._is_bootable;
        _pdliex                         = _disk._pdliex;
        memcpy( &_geometry, &_disk._geometry, sizeof(DISK_GEOMETRY_EX) );
    }
    const virtual disk_implement &operator =( const disk_implement& _disk ){
        if ( this != &_disk )
            copy( _disk );
        return( *this );
    }
    static bool get_disk( HANDLE handle, disk_implement& _disk );
    static bool get_registry_property( HDEVINFO DevInfo, DWORD Index, stdstring &deviceId );
    static bool get_device_property( HDEVINFO IntDevInfo, DWORD Index, stdstring &devicePath );
    static bool get_disk_number( HANDLE handle, DWORD &number );
    static bool get_disk_geometry( HANDLE handle, DISK_GEOMETRY_EX &disk_geometry );
    static bool get_storage_query_property( HANDLE handle, DWORD &device_type, STORAGE_BUS_TYPE &bus_type, bool &is_remove_media, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &serial_number );
    static bool get_scsi_pass_through_data( HANDLE handle, DWORD &device_type, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &verdor_string );
    static bool get_drive_layout( HANDLE handle, boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex );
    static bool get_disk_length( HANDLE handle, ULONGLONG &length);
    static bool get_disk_scsi_address( HANDLE handle, UCHAR &port_number, UCHAR &path_id, UCHAR &target_id, UCHAR &lun );
private:
    friend class disk;
    static stdstring                                 _get_disk_id( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex );
    static bool                                      _get_is_dynamic_disk( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex );
    static bool                                      _get_is_bootable_disk( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex );
    bool                                             _is_dynamic_disk;
    bool                                             _is_bootable;
    bool                                             _is_readable;
    bool                                             _is_remove_media;

    // SCSI_ADDRESS 
    UCHAR                                            _port_number;
    UCHAR                                            _path_id;
    UCHAR                                            _target_id;
    UCHAR                                            _lun;

    DWORD                                            _number;         // Disk number; 
    stdstring                                        _device_path;    // Device path;
    ULONGLONG                                        _length;         // byte;
    DWORD                                            _device_type;
    STORAGE_BUS_TYPE                                 _bus_type;
    stdstring                                        _vendor_id;
    stdstring                                        _product_id;
    stdstring                                        _product_revision;
    stdstring                                        _serial_number;    
    stdstring                                        _vendor_string;    
    DISK_GEOMETRY_EX                                 _geometry;
    stdstring                                        _disk_id;         // MBR is disk signature, GPT is guid.
    PARTITION_STYLE                                  _partition_style;
    const static TCHAR*                              _bus_types[BusTypeMax+1];
    const static TCHAR*                              _device_types[16];
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > _pdliex;
};

class partition 
{
private:
    friend class disk;
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > _pdliex;
    DWORD _disk_number;
    DWORD _index;
    partition( DWORD disk_number, DWORD index, boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > pdliex ) 
        : _disk_number(-1), 
        _index(-1) {
        _disk_number      = disk_number;
        _index            = index;
        _pdliex           = pdliex;
    }
public:
    partition( const partition& _partition ){
        copy( _partition );
    }
    void copy ( const partition& _partiton ){
        _disk_number      = _partiton._disk_number;
        _index            = _partiton._index;
        _pdliex           = _partiton._pdliex;
    }
    const virtual partition &operator =( const partition& _partition ){
        if ( this != &_partition )
            copy( _partition );
        return( *this );
    }
    DWORD            inline disk_number()        const { return _disk_number; }
    DWORD            inline partition_number()   const { return _pdliex->PartitionEntry[_index].PartitionNumber; }
    ULONGLONG        inline starting_offset()    const { return _pdliex->PartitionEntry[_index].StartingOffset.QuadPart; }
    ULONGLONG        inline length()             const { return _pdliex->PartitionEntry[_index].PartitionLength.QuadPart; }
    PARTITION_STYLE  inline partition_style()    const { return _pdliex->PartitionEntry[_index].PartitionStyle; }
    bool             inline is_gpt_partition()   const { return PARTITION_STYLE_GPT == partition_style(); }
    bool             inline is_mbr_partition()   const { return PARTITION_STYLE_MBR == partition_style(); }
    BYTE             inline mbr_partition_type() const { return is_mbr_partition() ? _pdliex->PartitionEntry[_index].Mbr.PartitionType : 0x0; }
    bool             inline mbr_boot_indicator() const { return is_mbr_partition() ? TRUE == _pdliex->PartitionEntry[_index].Mbr.BootIndicator : false; }
    bool             inline mbr_recognized_partition() const { return is_mbr_partition()? (TRUE == _pdliex->PartitionEntry[_index].Mbr.RecognizedPartition) : false; }
    DWORD            inline mbr_hidden_sectors() const { return is_mbr_partition() ? _pdliex->PartitionEntry[_index].Mbr.HiddenSectors : 0; }
    GUID             inline gpt_partition_type() const { return is_gpt_partition() ? _pdliex->PartitionEntry[_index].Gpt.PartitionType : GUID_NULL; }
    GUID             inline gpt_partition_id()   const { return is_gpt_partition() ? _pdliex->PartitionEntry[_index].Gpt.PartitionId : GUID_NULL;   }
    DWORD64          inline gpt_partition_attributes() const { return is_gpt_partition() ? _pdliex->PartitionEntry[_index].Gpt.Attributes : 0 ; }
    std::wstring     inline gpt_partition_name()       const { return is_gpt_partition() ? _pdliex->PartitionEntry[_index].Gpt.Name : L""; }
    stdstring        sz_partition_type();
    bool             is_bootable();
    volume_table     get_volumes();   
};
typedef std::vector< partition > partition_table;

class disk{
public:
    disk( const disk& _disk ){
        copy(_disk);
    }
    void copy ( const disk& _disk ){
        _disk_impl = _disk._disk_impl;
    }
    const virtual disk &operator =( const disk& _disk ){
        if ( this != &_disk )
            copy( _disk );
        return( *this );
    }

    // SCSI_ADDRESS
    UCHAR     inline   port_number()        const { return _disk_impl._port_number;     }
    UCHAR     inline   path_id()            const { return _disk_impl._path_id;         }
    UCHAR     inline   target_id()          const { return _disk_impl._target_id;       }
    UCHAR     inline   lun()                const { return _disk_impl._lun;             }

    bool      inline   is_dynamic_disk()    const { return _disk_impl._is_dynamic_disk; }
    bool      inline   is_bootable_disk()   const { return _disk_impl._is_bootable;     }
    bool      inline   is_readable_disk()   const { return _disk_impl._is_readable;     }
    bool      inline   is_remove_media()    const { return _disk_impl._is_remove_media; }
    DWORD     inline   number()             const { return _disk_impl._number;          } 
    stdstring inline   device_path()        const { return _disk_impl._device_path;     }
    ULONGLONG inline   length()             const { return _disk_impl._length;          }
    DWORD     inline   device_type()        const { return _disk_impl._device_type;     } 
    STORAGE_BUS_TYPE inline bus_type()      const { return _disk_impl._bus_type;        }
    stdstring inline   sz_device_type()     const { return _disk_impl._device_types[_disk_impl._device_type > 0x0F? 0x0F: _disk_impl._device_type ]; }
    stdstring inline   sz_bus_type()        const { return _disk_impl._bus_types[_disk_impl._bus_type > BusTypeMax? BusTypeMax : _disk_impl._bus_type ]; }
    stdstring inline   vendor_id()          const { return _disk_impl._vendor_id;       } 
    stdstring inline   product_id()         const { return _disk_impl._product_id;      } 
    stdstring inline   product_revision()   const { return _disk_impl._product_revision;} 
    stdstring inline   serial_number()      const { return _disk_impl._serial_number;   } 
    stdstring inline   vendor_string()      const { return _disk_impl._vendor_string;   } 
    stdstring inline   disk_id()            const { return _disk_impl._disk_id;         } 
    ULONGLONG  inline  cylinders()           const { return _disk_impl._geometry.Geometry.Cylinders.QuadPart; }
    MEDIA_TYPE inline  media_type()          const { return _disk_impl._geometry.Geometry.MediaType;          }
    DWORD inline       tracks_per_cylinder() const { return _disk_impl._geometry.Geometry.TracksPerCylinder;  }
    DWORD inline       sectors_per_track()   const { return _disk_impl._geometry.Geometry.SectorsPerTrack;    }
    DWORD inline       bytes_per_sector()    const { return _disk_impl._geometry.Geometry.BytesPerSector;     }
    PARTITION_STYLE  inline partition_style() const { return _disk_impl._partition_style;     } 
    bool             inline is_gpt_disk()     const { return PARTITION_STYLE_GPT == partition_style(); }
    bool             inline is_mbr_disk()     const { return PARTITION_STYLE_MBR == partition_style(); }
    bool             inline is_raw_disk()     const { return PARTITION_STYLE_RAW == partition_style(); }
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > inline drive_layout_info() const { return _disk_impl._pdliex; }
    partition_table    get_partitons();
    volume_table       get_volumes();  
    void               refresh() { _disk_impl = get_disk( number() )._disk_impl; }
    static disk_table  get_disks();
    static disk        get_disk( DWORD number );
private:
    disk( const disk_implement& disk_impl ) { 
        _disk_impl = disk_impl; 
    }
    disk_implement _disk_impl;
};

#ifndef MACHO_HEADER_ONLY

#include <initguid.h>   // Guid definition
#include <devguid.h>    // Device guids
#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <ntddscsi.h>
#include "auto_handle_base.hpp"

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH Spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             SenseBuf[32];
    UCHAR             DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

EXTERN_C const GUID PARTITION_SYSTEM_GUID;
EXTERN_C const GUID PARTITION_LDM_DATA_GUID;
EXTERN_C const GUID PARTITION_LDM_METADATA_GUID;
EXTERN_C const GUID PARTITION_BASIC_DATA_GUID;
EXTERN_C const GUID PARTITION_ENTRY_UNUSED_GUID;
EXTERN_C const GUID PARTITION_MSFT_RESERVED_GUID;
EXTERN_C const GUID PARTITION_MSFT_RECOVERY_GUID;

#define  MS_PARTITION_LDM           0x42                   // PARTITION_LDM
#define  CDB6GENERIC_LENGTH         6
#define  SCSIOP_INQUIRY             0x12

MAKE_AUTO_HANDLE_CLASS_EX(auto_setupdi_handle, HDEVINFO, SetupDiDestroyDeviceInfoList, INVALID_HANDLE_VALUE ); 

const TCHAR* disk_implement::_bus_types[BusTypeMax+1] = {
    _T("UNKNOWN"),  // 0x00
    _T("SCSI"),
    _T("ATAPI"),
    _T("ATA"),
    _T("IEEE 1394"),
    _T("SSA"),
    _T("FIBRE"),
    _T("USB"),
    _T("RAID"),
    _T("ISCSI"),
    _T("SAS"),
    _T("SATA"),
    _T("SD"),
    _T("MMC"),
    _T("VIRTUAL"),
    _T("FILEBACKEDVIRTUAL"),
    _T("UNKNOWN")
};

const TCHAR* disk_implement::_device_types[16] = {
    _T("Direct Access Device"), // 0x00
    _T("Tape Device"),          // 0x01
    _T("Printer Device"),       // 0x02
    _T("Processor Device"),     // 0x03
    _T("WORM Device"),          // 0x04
    _T("CDROM Device"),         // 0x05
    _T("Scanner Device"),       // 0x06
    _T("Optical Disk"),         // 0x07
    _T("Media Changer"),        // 0x08
    _T("Comm. Device"),         // 0x09
    _T("ASCIT8"),               // 0x0A
    _T("ASCIT8"),               // 0x0B
    _T("Array Device"),         // 0x0C
    _T("Enclosure Device"),     // 0x0D
    _T("RBC Device"),           // 0x0E
    _T("Unknown Device")        // 0x0F
};

/*********************************************************************
    disk_implement class
**********************************************************************/
stdstring disk_implement::_get_disk_id( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex ){
    stdstring signature;
    TCHAR         szGUID[40];
    switch ( pdliex.get()->PartitionStyle ){
    case PARTITION_STYLE_MBR:
        stringutils::format( signature, _T("0x%08X"), pdliex.get()->Mbr.Signature );
        break;
    case PARTITION_STYLE_GPT:
        StringFromGUID2( pdliex.get()->Gpt.DiskId, szGUID, 40); 
        signature = szGUID;
        break;
    }
    return signature;
}

bool disk_implement::_get_is_dynamic_disk( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex ){
    switch( pdliex.get()->PartitionStyle ){
    case PARTITION_STYLE_GPT:
        for ( DWORD i = 0 ; i < pdliex.get()->PartitionCount; ++ i ){
            if ( ( pdliex.get()->PartitionEntry[i].Gpt.PartitionType == PARTITION_LDM_METADATA_GUID ) ||
                ( pdliex.get()->PartitionEntry[i].Gpt.PartitionType == PARTITION_LDM_DATA_GUID ) )
                return true;
        }
        break;
    case PARTITION_STYLE_MBR:
        for ( DWORD i = 0 ; i < pdliex.get()->PartitionCount; ++ i ){
            if ( pdliex.get()->PartitionEntry[i].Mbr.PartitionType == MS_PARTITION_LDM )
                return true;
        }
        break;
    }
    return false;
}

bool disk_implement::_get_is_bootable_disk( boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex ){
    switch( pdliex.get()->PartitionStyle ){
    case PARTITION_STYLE_GPT:
        for ( DWORD i = 0 ; i < pdliex.get()->PartitionCount; ++ i ){
            if ( pdliex.get()->PartitionEntry[i].Gpt.PartitionType == PARTITION_SYSTEM_GUID ) 
                return true;
        }
        break;
    case PARTITION_STYLE_MBR:
        for ( DWORD i = 0 ; i < pdliex.get()->PartitionCount; ++ i ){
            if ( pdliex.get()->PartitionEntry[i].Mbr.RecognizedPartition && 
                pdliex.get()->PartitionEntry[i].Mbr.BootIndicator )
                return true;
        }
        break;
    }
    return false;
}

bool disk_implement::get_registry_property( HDEVINFO DevInfo, DWORD Index, stdstring &deviceId )
/*++

Routine Description:

    This routine enumerates the disk devices using the Setup class interface
    GUID GUID_DEVCLASS_DISKDRIVE. Gets the Device ID from the Registry 
    property.

Arguments:

    DevInfo - Handles to the device information list

    Index   - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{

    SP_DEVINFO_DATA         deviceInfoData;
    DWORD                   errorCode;
    DWORD                   bufferSize = 0;
    DWORD                   dataType;
    std::auto_ptr<TCHAR>    pbuffer;
    BOOL                    status;

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    status = SetupDiEnumDeviceInfo(
                DevInfo,
                Index,
                &deviceInfoData);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
            LOG( LOG_LEVEL_TRACE, _T("No more devices.\n"));
        }
        else {
            LOG( LOG_LEVEL_ERROR, _T("SetupDiEnumDeviceInfo failed with error: %d\n"), errorCode );
        }
        return false;
    }
        
    //
    // We won't know the size of the HardwareID buffer until we call
    // this function. So call it with a null to begin with, and then 
    // use the required buffer size to Alloc the necessary space.
    // Keep calling we have success or an unknown failure.
    //

    status = SetupDiGetDeviceRegistryProperty(
                DevInfo,
                &deviceInfoData,
                SPDRP_HARDWAREID,
                &dataType,
                (PBYTE)pbuffer.get(),
                bufferSize,
                &bufferSize);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            if ( errorCode == ERROR_INVALID_DATA ) {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                return true;
            }
            else {
                LOG( LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode );
                return false;
            }
        }
    }

    //
    // We need to change the buffer size.
    //
    pbuffer = std::auto_ptr<TCHAR>( new TCHAR [bufferSize/sizeof(TCHAR)] );
    memset( pbuffer.get(), 0, bufferSize );
    status = SetupDiGetDeviceRegistryProperty(
                DevInfo,
                &deviceInfoData,
                SPDRP_HARDWAREID,
                &dataType,
                (PBYTE)pbuffer.get(),
                bufferSize,
                &bufferSize);

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_INVALID_DATA ) {
            //
            // May be a Legacy Device with no HardwareID. Continue.
            //
            return true;
        }
        else {
            LOG( LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode );
            return false;
        }
    }

    LOG( LOG_LEVEL_TRACE, _T("\n\nDevice ID: %s\n"), pbuffer.get() );
    
    deviceId = pbuffer.get();

    return true;
}

bool disk_implement::get_device_property( HDEVINFO IntDevInfo, DWORD Index, stdstring &devicePath )
/*++

Routine Description:

    This routine enumerates the disk devices using the Device interface
    GUID DiskClassGuid. Gets the Adapter & Device property from the port
    driver. Then sends IOCTL through SPTI to get the device Inquiry data.

Arguments:

    IntDevInfo - Handles to the interface device information list

    Index      - Device member 

Return Value:

  TRUE / FALSE. This decides whether to continue or not

--*/
{
    SP_DEVICE_INTERFACE_DATA                         interfaceData;
    std::auto_ptr< SP_DEVICE_INTERFACE_DETAIL_DATA > p_interfaceDetailData;
    BOOL                                             status;
    DWORD                                            interfaceDetailDataSize,
                                                     reqSize,
                                                     errorCode;

    interfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

    status = SetupDiEnumDeviceInterfaces ( 
                IntDevInfo,             // Interface Device Info handle
                0,                      // Device Info data
                (LPGUID)&DiskClassGuid, // Interface registered by driver
                Index,                  // Member
                &interfaceData          // Device Interface Data
                );

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode == ERROR_NO_MORE_ITEMS ) {
            LOG( LOG_LEVEL_TRACE, _T("No more interfaces\n") );
        }
        else {
            LOG( LOG_LEVEL_ERROR, _T("SetupDiEnumDeviceInterfaces failed with error: %d\n"), errorCode  );
        }
        return false;
    }
        
    //
    // Find out required buffer size, so pass NULL 
    //

    status = SetupDiGetDeviceInterfaceDetail (
                IntDevInfo,         // Interface Device info handle
                &interfaceData,     // Interface data for the event class
                NULL,               // Checking for buffer size
                0,                  // Checking for buffer size
                &reqSize,           // Buffer size required to get the detail data
                NULL                // Checking for buffer size
                );

    //
    // This call returns ERROR_INSUFFICIENT_BUFFER with reqSize 
    // set to the required buffer size. Ignore the above error and
    // pass a bigger buffer to get the detail data
    //

    if ( status == FALSE ) {
        errorCode = GetLastError();
        if ( errorCode != ERROR_INSUFFICIENT_BUFFER ) {
            LOG( LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode );
            return false;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    p_interfaceDetailData = std::auto_ptr< SP_DEVICE_INTERFACE_DETAIL_DATA >( (PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[interfaceDetailDataSize] );
    memset( p_interfaceDetailData.get(), 0, interfaceDetailDataSize );
    if ( NULL == p_interfaceDetailData.get() ) {
        LOG( LOG_LEVEL_ERROR, _T("Unable to allocate memory to get the interface detail data.\n") );
        return false;
    }
    p_interfaceDetailData.get()->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
    status = SetupDiGetDeviceInterfaceDetail (
                  IntDevInfo,               // Interface Device info handle
                  &interfaceData,           // Interface data for the event class
                  p_interfaceDetailData.get(),      // Interface detail data
                  interfaceDetailDataSize,  // Interface detail data size
                  &reqSize,                 // Buffer size required to get the detail data
                  NULL);                    // Interface device info

    if ( status == FALSE ) {
        LOG( LOG_LEVEL_ERROR, _T("Error in SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), GetLastError() );
        return false;
    }

    devicePath = p_interfaceDetailData.get()->DevicePath;
    LOG( LOG_LEVEL_TRACE, _T("Interface: %s\n"), p_interfaceDetailData.get()->DevicePath);
    
    return true;
}

bool disk_implement::get_disk_number( HANDLE handle, DWORD &number ){
    STORAGE_DEVICE_NUMBER      sdn;
    DWORD               cbReturned;
    BOOL               state ;
    if ( state = DeviceIoControl( handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &cbReturned, NULL ) )
    number = sdn.DeviceNumber;
    return ( TRUE == state );
}

bool disk_implement::get_disk_geometry( HANDLE handle, DISK_GEOMETRY_EX &disk_geometry ){
    DWORD               cbReturned;
    return ( TRUE == DeviceIoControl( handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &disk_geometry, sizeof(disk_geometry), &cbReturned, NULL ) );
}

bool disk_implement::get_storage_query_property( HANDLE handle, DWORD &device_type, STORAGE_BUS_TYPE &bus_type, bool &is_remove_media, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &serial_number ){
    bool ret = false;
    STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_DEVICE_DESCRIPTOR          devDesc;
    BOOL                state ;
    PUCHAR                              p;
    UCHAR                               outBuf[512];
    ULONG                               length = 0,
                                        returned = 0,
                                        returnedLength;
    DWORD                               i;
                                     
    memset( outBuf, 0 , sizeof(outBuf) );
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    state = DeviceIoControl( handle,                
                             IOCTL_STORAGE_QUERY_PROPERTY,
                             &query,
                             sizeof( STORAGE_PROPERTY_QUERY ),
                             &outBuf,                   
                             512,                      
                             &returnedLength,
                             NULL );
    if ( !state ) {
        LOG( LOG_LEVEL_ERROR, _T("IOCTL failed with error code%d.\n\n"), GetLastError() );
    }
    else {
        LOG( LOG_LEVEL_TRACE, _T("\nDevice Properties\n"));
        LOG( LOG_LEVEL_TRACE, _T("-----------------\n"));
        devDesc = (PSTORAGE_DEVICE_DESCRIPTOR) outBuf;
        //
        // Our device table can handle only 16 devices.
        //
        LOG( LOG_LEVEL_TRACE, _T("Device Type     : %s (0x%X)\n"), 
            _device_types[devDesc->DeviceType > 0x0F? 0x0F: devDesc->DeviceType ], devDesc->DeviceType);
        if ( devDesc->DeviceTypeModifier ) {
            LOG( LOG_LEVEL_TRACE, _T("Device Modifier : 0x%x\n"), devDesc->DeviceTypeModifier);
        }
        device_type     = devDesc->DeviceType;

        LOG( LOG_LEVEL_TRACE, _T("Bus Type     : %s (0x%X)\n"), 
            _bus_types[devDesc->BusType > BusTypeMax? BusTypeMax : devDesc->BusType ], devDesc->BusType);
        bus_type        = devDesc->BusType;

        is_remove_media = ( devDesc->RemovableMedia == TRUE );
        LOG( LOG_LEVEL_TRACE, _T("Removable Media : %s\n"), devDesc->RemovableMedia ? _T("Yes") : _T("No") );
        p = (PUCHAR) outBuf; 

        if ( devDesc->VendorIdOffset && (devDesc->VendorIdOffset < returnedLength) && p[devDesc->VendorIdOffset] ) {
            std::string szVendorId;
            for ( i = devDesc->VendorIdOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                szVendorId.push_back( p[i] );
            }
#if _UNICODE   
            vendor_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szVendorId));
#else
            vendor_id = stringutils::erase_trailing_whitespaces(szVendorId);
#endif
            LOG( LOG_LEVEL_TRACE, _T("Vendor ID       : %s\n"), vendor_id.c_str() );
        }
        if ( devDesc->ProductIdOffset && (devDesc->ProductIdOffset < returnedLength) && p[devDesc->ProductIdOffset] ) {
            std::string szProductId;
            for ( i = devDesc->ProductIdOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                szProductId.push_back( p[i] );
            }
#if _UNICODE   
            product_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szProductId));
#else
            product_id = stringutils::erase_trailing_whitespaces(szProductId);
#endif
            LOG( LOG_LEVEL_TRACE, _T("Product ID      : %s\n"), product_id.c_str() );
        }

        if ( devDesc->ProductRevisionOffset && (devDesc->ProductRevisionOffset < returnedLength)&& p[devDesc->ProductRevisionOffset] ) {
            std::string szProductRevision;
            for ( i = devDesc->ProductRevisionOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                szProductRevision.push_back( p[i] );
            }
#if _UNICODE   
            product_revision = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szProductRevision));
#else
            product_revision = stringutils::erase_trailing_whitespaces(szProductRevision);
#endif
            LOG( LOG_LEVEL_TRACE, _T("Product Revision: %s\n"), product_revision.c_str() );
        }

        if ( devDesc->SerialNumberOffset && (devDesc->SerialNumberOffset < returnedLength) && p[devDesc->SerialNumberOffset] ) {
            std::string szSerialNumber;
            for ( i = devDesc->SerialNumberOffset; p[i] != (UCHAR) NULL && i < returnedLength; i++ ) {
                szSerialNumber.push_back( p[i] );
            }
#if _UNICODE   
            serial_number = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szSerialNumber));
#else
            serial_number = stringutils::erase_trailing_whitespaces(szSerialNumber);
#endif
            LOG( LOG_LEVEL_TRACE, _T("Serial Number   : %s"), serial_number.c_str() );
        }
    }
    return ( TRUE == state );
}

bool disk_implement::get_scsi_pass_through_data( HANDLE handle, DWORD &device_type, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &verdor_string ){
    SCSI_PASS_THROUGH_WITH_BUFFERS      sptwb;
    BOOL                                status;
    ULONG                               length = 0,
                                        returned = 0;
    DWORD                               i,
                                        errorCode;
    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.Spt.PathId = 0;
    sptwb.Spt.TargetId = 1;
    sptwb.Spt.Lun = 0;
    sptwb.Spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.Spt.SenseInfoLength = 24;
    sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.Spt.DataTransferLength = 192;
    sptwb.Spt.TimeOutValue = 2;
    sptwb.Spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf);
    sptwb.Spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,SenseBuf);
    sptwb.Spt.Cdb[0] = SCSIOP_INQUIRY;
    sptwb.Spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf) +
       sptwb.Spt.DataTransferLength;

    status = DeviceIoControl( handle,
                              IOCTL_SCSI_PASS_THROUGH,
                              &sptwb,
                              sizeof(SCSI_PASS_THROUGH),
                              &sptwb,
                              length,
                              &returned,
                              FALSE);
    LOG( LOG_LEVEL_TRACE, _T("Inquiry Data from Pass Through\n"));
    LOG( LOG_LEVEL_TRACE, _T("------------------------------\n"));

    if (!status ) {
        LOG( LOG_LEVEL_ERROR, _T("Error: %d "), errorCode = GetLastError() );
        return FALSE;
    }
    if (sptwb.Spt.ScsiStatus) {
        LOG( LOG_LEVEL_ERROR, _T("Scsi status: %02Xh\n"), sptwb.Spt.ScsiStatus);    
        return FALSE;
    }
    else {

        device_type = sptwb.DataBuf[0] & 0x1f ;
        //
        // Our Device Table can handle only 16 devices.
        //
        LOG( LOG_LEVEL_TRACE, _T("Device Type: %s (0x%X)\n"), _device_types[device_type > 0x0F? 0x0F: device_type], device_type );
        std::string tempstr;
        for (i = 8; i <= 15; i++) {
            tempstr.push_back( sptwb.DataBuf[i] );
        }
#if _UNICODE   
        vendor_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        vendor_id = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG( LOG_LEVEL_TRACE, _T("Vendor ID  : %s"), vendor_id.c_str());
        tempstr.clear();
        for (i = 16; i <= 31; i++) {
            tempstr.push_back( sptwb.DataBuf[i] );
        }
#if _UNICODE   
        product_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        product_id = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG( LOG_LEVEL_TRACE, _T("Product ID : %s"), product_id.c_str());
        tempstr.clear();
        for (i = 32; i <= 35; i++) {
            tempstr.push_back( sptwb.DataBuf[i] );
        }
#if _UNICODE   
        product_revision = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        product_revision = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG( LOG_LEVEL_TRACE, _T("Product Rev: %s"), product_revision.c_str());
        tempstr.clear();
        for (i = 36; i <= 55 && sptwb.DataBuf[i] != NULL; i++) {
            tempstr.push_back( sptwb.DataBuf[i] );
        }
#if _UNICODE   
        verdor_string = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        verdor_string = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG( LOG_LEVEL_TRACE, _T("Vendor Str : %s"), verdor_string.c_str());    
    }
    return ( TRUE == status );
}

bool disk_implement::get_drive_layout( HANDLE handle, boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex ){
    DWORD                            bufsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX)*3;
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > pdli( ( DRIVE_LAYOUT_INFORMATION_EX *) new BYTE[bufsize] );
    BOOL                            fResult;
    DWORD                            dwBytesReturned;

    while (!( fResult = DeviceIoControl ( handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
        NULL, 0, pdli.get(), bufsize, &dwBytesReturned, NULL))){
        if ((GetLastError() == ERROR_MORE_DATA) ||   
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER)){
            // Create enough space for four more partition table entries.
            bufsize += sizeof(PARTITION_INFORMATION_EX)*4;
            pdli = boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > ( ( DRIVE_LAYOUT_INFORMATION_EX *) new BYTE[bufsize] );
        }
        else{
            LOG( LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed") );
            break;
        }
    }
    if ( fResult ) pdliex = pdli;
    return ( TRUE == fResult );
}

bool disk_implement::get_disk_length( HANDLE handle, ULONGLONG &length){
    GET_LENGTH_INFORMATION      gLength;
    BOOL                        state;
    DWORD                       cbReturned;
    if ( state = DeviceIoControl( handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gLength, sizeof(gLength), &cbReturned, NULL ) )
        length = gLength.Length.QuadPart;
    return ( TRUE == state );
}

bool disk_implement::get_disk_scsi_address(HANDLE handle, UCHAR &port_number, UCHAR &path_id, UCHAR &target_id, UCHAR &lun)
{
    SCSI_ADDRESS                scsiAddress;
    BOOL                        state;
    DWORD                       cbReturned;
    
    memset(&scsiAddress, 0, sizeof(SCSI_ADDRESS));
    scsiAddress.Length = sizeof(SCSI_ADDRESS);
    if (state = DeviceIoControl(handle, IOCTL_SCSI_GET_ADDRESS, NULL, 0, &scsiAddress, sizeof(SCSI_ADDRESS), &cbReturned, NULL)){
        port_number = scsiAddress.PortNumber;
        path_id = scsiAddress.PathId;
        target_id = scsiAddress.TargetId;
        lun = scsiAddress.Lun;
    }
    return (TRUE == state);
}

bool disk_implement::get_disk( HANDLE handle, disk_implement& _disk ){
    
    bool status = false;
    if ( INVALID_HANDLE_VALUE != handle ){
        if ( ! ( status = get_disk_number( handle, _disk._number ) ) ){
            LOG( LOG_LEVEL_ERROR, _T("Can't get disk number (%s). error( 0x%08X )"), _disk._device_path.c_str(), GetLastError());      
        }
        else{
            _disk._device_path    = stringutils::format(_T("\\\\.\\PhysicalDrive%d"), _disk._number); 
            if ( ! ( status = get_disk_geometry( handle, _disk._geometry ) ) ){
                LOG( LOG_LEVEL_ERROR, _T("Can't get disk geometry (%s). error( 0x%08X )"), _disk._device_path.c_str(), GetLastError()); 
            }
            else if ( !( status = get_storage_query_property( handle, _disk._device_type, _disk._bus_type, _disk._is_remove_media, 
                _disk._vendor_id, _disk._product_id, _disk._product_revision, _disk._serial_number ) ) ){
                LOG( LOG_LEVEL_ERROR, _T("Can't get storage query property information (%s). error( 0x%08X )"), _disk._device_path.c_str(), GetLastError());
                if ( ! ( status = get_scsi_pass_through_data( handle, _disk._device_type, _disk._vendor_id, _disk._product_id, _disk._product_revision, _disk._vendor_string ) ) ){
                    LOG( LOG_LEVEL_ERROR, _T("Can't get query scsi pass through data (%s). error( 0x%08X )"), _disk._device_path.c_str(), GetLastError() ); 
                }
                else if ( stdstring::npos != _disk._vendor_id.find( _T("FALCON") ) ){ // If the disk is IPStor disk. the vendor string is equal as the serial number.
                    _disk._serial_number = _disk._vendor_string;
                }
            }
            else{
                DWORD type;
                stdstring vender_id, product_id, product_revision;
                if ( !get_scsi_pass_through_data( handle, type, vender_id, product_id, product_revision, _disk._vendor_string ) ){
                    LOG( LOG_LEVEL_ERROR, _T("Can't get query scsi pass through data (%s). error( 0x%08X )"), _disk._device_path.c_str(), GetLastError() );    
                }
            }

            if ( status ){

                boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > pdliex;
                if (!( status = get_drive_layout( handle, pdliex ) ) ){
                    LOG( LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX (%s) failed. error( 0x%08X )"), _disk._device_path.c_str(), GetLastError() );
                }
                else{
                    if ( !get_disk_length( handle, _disk._length ) ){
                        _disk._length = _disk._geometry.DiskSize.QuadPart;
                    }
                    _disk._pdliex          = pdliex;
                    _disk._is_readable     = true;
                    if ( pdliex->PartitionStyle == PARTITION_STYLE_MBR && pdliex->Mbr.Signature == 0 && pdliex->PartitionCount == 0 )
                        _disk._partition_style = PARTITION_STYLE_RAW;
                    else
                        _disk._partition_style = (PARTITION_STYLE)pdliex->PartitionStyle;

                    _disk._disk_id         = _get_disk_id( pdliex );
                    _disk._is_dynamic_disk = _get_is_dynamic_disk( pdliex );
                    _disk._is_bootable     = _get_is_bootable_disk( pdliex );
                    status = get_disk_scsi_address(handle, _disk._port_number, _disk._path_id, _disk._target_id, _disk._lun);
                }
            }
        }
    }
    return status;
}

/*********************************************************************
    partition class
**********************************************************************/

stdstring  partition::sz_partition_type(){

    stdstring partiton_type = _T("Unknown");
    switch( partition_style() ){
    case PARTITION_STYLE_MBR:{
            switch ( mbr_partition_type() ) {
            case PARTITION_EXTENDED: 
            case PARTITION_XINT13_EXTENDED:
                partiton_type = _T("Extended partition");
                break;
            default:
                partiton_type = _T("Primary partition");
            }  
        }
        break;
    case PARTITION_STYLE_GPT: {
            if ( gpt_partition_type() == PARTITION_SYSTEM_GUID ){
                partiton_type = _T("EFI system partition");
            }
            else if ( gpt_partition_type() == PARTITION_MSFT_RESERVED_GUID ){
                partiton_type = _T("Microsoft reserved partition");
            }
            else if ( gpt_partition_type() == PARTITION_BASIC_DATA_GUID ){
                partiton_type = _T("Basic data partition");
            }
            else if ( gpt_partition_type() == PARTITION_LDM_METADATA_GUID ){
                partiton_type = _T("Logical Disk Manager metadata partition");
            }
            else if ( gpt_partition_type() == PARTITION_LDM_DATA_GUID ){
                partiton_type = _T("Logical Disk Manager data partition");
            }
            else if ( gpt_partition_type() == PARTITION_MSFT_RECOVERY_GUID ){
                partiton_type = _T("Microsoft recovery partition");
            }
            else if ( gpt_partition_type() == PARTITION_ENTRY_UNUSED_GUID ){
                partiton_type = _T("Entry unused");
            }
        }
        break;
    }
    return partiton_type;
}

bool partition::is_bootable(){
    switch( partition_style() ){
    case PARTITION_STYLE_MBR:
        return ( mbr_boot_indicator() && mbr_recognized_partition() );
        break;
    case PARTITION_STYLE_GPT:
        return ( gpt_partition_type() == PARTITION_SYSTEM_GUID ) ? true : false;
        break;
    }
    return false;
}

volume_table partition::get_volumes(){
    volume_table partition_volumes;
    volume_table volumes = volume::get_volumes();
    foreach( volume v, volumes ){
        foreach( disk_extent e, v.disk_extents() ){
            if ( ( e.disk_number == disk_number() ) &&
               ( e.starting_offset >= starting_offset() ) && 
               ( e.starting_offset < ( starting_offset() + length() ) ) &&
               ( e.extent_length <= length() ) ){
                partition_volumes.push_back(v);
                break;
            }
        }
    }
    return partition_volumes;
}

/*********************************************************************
    disk class
**********************************************************************/

partition_table disk::get_partitons(){
    partition_table partitions;
    for( int i = 0 ; i < (int)_disk_impl._pdliex->PartitionCount ; i++ ){
        partition _partition( _disk_impl._number, i, _disk_impl._pdliex );
        if ( 0 != _partition.length() ) 
            partitions.push_back(_partition);
    }
    return partitions;
}

disk_table disk::get_disks(){
    
    disk_table disks;
    auto_setupdi_handle   dev_info, int_dev_info;
    DWORD                  index;
    BOOL                  status = FALSE;
    BOOL                  result = FALSE;

    dev_info = SetupDiGetClassDevs(
                    (LPGUID) &GUID_DEVCLASS_DISKDRIVE,
                    NULL,
                    NULL, 
                    DIGCF_PRESENT  ); // All devices present on system
    if ( dev_info.is_invalid() ){
        LOG( LOG_LEVEL_ERROR, _T("SetupDiGetClassDevs failed with error: %d\n"),  GetLastError()  );
    }
    else{
        //
        // Open the device using device interface registered by the driver
        //
        //
        // Get the interface device information set that contains all devices of event class.
        //
        int_dev_info = SetupDiGetClassDevs (
                     (LPGUID)&DiskClassGuid,
                     NULL,                                   // Enumerator
                     NULL,                                   // Parent Window
                     (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
                     ));

        if(  int_dev_info.is_invalid() ) {
            LOG( LOG_LEVEL_ERROR, _T("SetupDiGetClassDevs failed with error: %d\n"), GetLastError() );
        }
        else{
            //
            //  Enumerate all the disk devices
            //
            index = 0;
            while (TRUE) {
                stdstring            devicePath, deviceId;
                auto_file_handle    device_handle;
                LOG( LOG_LEVEL_TRACE, _T("Properties for Device %d"), index+1);
                status = disk_implement::get_registry_property( dev_info, index, deviceId);
                if ( status == FALSE ) {
                    break;
                }

                status = disk_implement::get_device_property( int_dev_info, index, devicePath );
                if ( status == FALSE ) {
                    break;
                }
                index++;        
                device_handle = CreateFile(
                        devicePath.c_str(),    // device interface name
                        GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                        NULL,                               // lpSecurityAttributes
                        OPEN_EXISTING,                      // dwCreationDistribution
                        0,                                  // dwFlagsAndAttributes
                        NULL                                // hTemplateFile
                        );
                if ( device_handle.is_invalid() ){
                    LOG( LOG_LEVEL_ERROR, _T("CreateFile(%s) failed. error( 0x%08X )"), devicePath.c_str(), GetLastError() ); 
                }
                else{
                    disk_implement disk_impl;
                    disk_impl._device_path = devicePath;
                    if ( disk_implement::get_disk( device_handle, disk_impl ) )
                        disks.push_back( disk ( disk_impl ) );
                }
            }       
        }
    }

    LOG( LOG_LEVEL_TRACE, _T("***  End of Device List  ***"));
    return disks;
}

disk disk::get_disk( DWORD number ){
    disk_implement disk_impl;
    stdstring device_path = stringutils::format( _T("\\\\.\\PhysicalDrive%d"), number );
    auto_file_handle device_hadle = CreateFile(
                device_path.c_str(),                // device interface name
                GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
    if ( device_hadle.is_invalid() ){
#if _UNICODE  
        stdstring msg = boost::str( boost::wformat(L"CreateFile (%s) failed.") % device_path );
#else
        stdstring msg = boost::str( boost::format("CreateFile (%s) failed.") % device_path );
#endif
        LOG( LOG_LEVEL_ERROR, _T("CreateFile(%s) failed. error( 0x%08X )"), device_path.c_str(), GetLastError() ); 
        BOOST_THROW_DISK_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), msg );  
    }
    else if ( !disk_implement::get_disk( device_hadle, disk_impl ) ){
#if _UNICODE  
        stdstring msg = boost::str( boost::wformat(L"Failed to get disk (%s) info.") % device_path );
#else
        stdstring msg = boost::str( boost::format("Failed to get disk (%s) info.") % device_path );
#endif
        LOG( LOG_LEVEL_ERROR, _T("Failed to get disk (%s) info. error( 0x%08X )"), device_path.c_str(), GetLastError() ); 
        BOOST_THROW_DISK_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), msg );  
    }
    return disk_impl;
}

volume_table disk::get_volumes(){
    volume_table disk_volumes;
    volume_table volumes = volume::get_volumes();
    foreach( volume v, volumes ){
        foreach( disk_extent e, v.disk_extents() ){
            if ( e.disk_number == number() ){
                disk_volumes.push_back(v);
                break;
            }
        }
    }
    return disk_volumes;
}

#endif

};

};

#endif