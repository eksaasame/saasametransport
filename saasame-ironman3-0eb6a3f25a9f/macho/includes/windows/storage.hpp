// storage.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_STORAGE__
#define __MACHO_WINDOWS_STORAGE__

#include "boost\shared_ptr.hpp"
#include "common\exception_base.hpp"
#pragma comment(lib, "setupapi.lib")

namespace macho{

namespace windows{

class storage
{
public:

    typedef enum {
        ST_PST_UNKNOWN = 0,
        ST_PST_MBR = 1,
        ST_PST_GPT = 2
    }ST_PARTITION_STYLE;

    typedef enum {
        ST_VT_UNKNOWN = 0,
        ST_VT_SIMPLE = 1,
        ST_VT_SPAN = 2,
        ST_VT_STRIPE = 3,
        ST_VT_MIRROR = 4,
        ST_VT_PARITY = 5
    }ST_VOLUME_TYPE;

    typedef enum {
        ST_UNKNOWN = 0,
        ST_NO_ROOT_PATH = 1,
        ST_REMOVABLE = 2,
        ST_FIXED = 3,
        ST_REMOTE = 4,
        ST_CDROM = 5,
        ST_RAMDISK = 6
    }ST_DRIVE_TYPE;

    typedef enum {
        ST_FAT12 = 1,
        ST_FAT16 = 4,
        ST_EXTENDED = 5,
        ST_HUGE = 6,
        ST_IFS = 7,
        ST_FAT32 = 12
    }ST_MBR_PARTITION_TYPE;

    typedef boost::shared_ptr<storage> ptr;
    struct  exception : virtual public exception_base {};
    class partition{
    public:
        typedef boost::shared_ptr<partition> ptr;
        typedef std::vector<ptr> vtr;
        virtual uint32_t       disk_number() = 0;
        virtual uint32_t       partition_number() = 0;
        virtual std::wstring   drive_letter() = 0;
        virtual string_array_w access_paths() = 0;
        virtual uint64_t     offset() = 0;
        virtual uint64_t     size() = 0;
        virtual uint16_t     mbr_type() = 0;
        virtual std::wstring gpt_type() = 0;
        virtual std::wstring guid() = 0;
        virtual bool         is_read_only() = 0;
        virtual bool         is_offline() = 0;
        virtual bool         is_system() = 0;
        virtual bool         is_boot() = 0;
        virtual bool         is_active() = 0;
        virtual bool         is_hidden() = 0;
        virtual bool         is_shadow_copy() = 0;
        virtual bool         no_default_drive_letter() = 0;
        virtual uint32_t     set_attributes(bool _is_read_only, bool _no_default_drive_letter, bool _is_active, bool _is_hidden) = 0;
    };

    class volume {
    public:
        typedef boost::shared_ptr<volume> ptr;
        typedef std::vector<ptr> vtr;
        virtual std::wstring id() = 0;
        virtual ST_VOLUME_TYPE type() = 0;
        virtual std::wstring   drive_letter() = 0;
        virtual std::wstring   path() = 0;
        virtual uint16_t       health_status() = 0;
        virtual std::wstring   file_system() = 0;
        virtual std::wstring   file_system_label() = 0;
        virtual uint64_t       size() = 0;
        virtual uint64_t       size_remaining() = 0;
        virtual ST_DRIVE_TYPE  drive_type() = 0;
        virtual string_array_w access_paths() = 0;
        virtual bool           mount() = 0;
        virtual bool           dismount(bool force = false, bool permanent = false) = 0;
        virtual bool           format(
            std::wstring file_system,
            std::wstring file_system_label,
            uint32_t allocation_unit_size,
            bool full = false,
            bool force = false,
            bool compress = false,
            bool short_file_name_support = false,
            bool set_integrity_streams = false,
            bool use_large_frs = false,
            bool disable_heat_gathering = false) = 0;
    };

    class disk {
    public:
        typedef boost::shared_ptr<disk> ptr;
        typedef std::vector<ptr> vtr;
        //virtual bool initialize(uint16_t partition_style) = 0;
        virtual bool online() = 0;
        virtual bool offline() = 0;
        virtual bool clear_read_only_flag() = 0;
        virtual bool set_read_only_flag() = 0;
        virtual bool initialize(ST_PARTITION_STYLE partition_style = ST_PST_GPT) = 0;
        virtual partition::ptr create_partition(uint64_t size = 0,
            bool use_maximum_size = true, 
            uint64_t offset = 0, 
            uint32_t alignment = 0, 
            std::wstring drive_letter = L"", 
            bool assign_drive_letter = true, 
            ST_MBR_PARTITION_TYPE mbr_type = ST_IFS,
            std::wstring  gpt_type = L"{ebd0a0a2-b9e5-4433-87c0-68b6b72699c7}",
            bool is_hidden = false,
            bool is_active = false
            ) = 0;
        virtual std::wstring        unique_id() = 0;
        virtual uint16_t            unique_id_format() = 0;
        virtual std::wstring        path() = 0;
        virtual std::wstring        location() = 0;
        virtual std::wstring        friendly_name() = 0;
        virtual uint32_t            number() = 0;
        virtual std::wstring        serial_number() = 0;
        virtual std::wstring        firmware_version() = 0;
        virtual std::wstring        manufacturer() = 0;
        virtual std::wstring        model() = 0;
        virtual uint64_t            size() = 0;
        virtual uint64_t            allocated_size() = 0;
        virtual uint32_t            logical_sector_size() = 0;
        virtual uint32_t            physical_sector_size() = 0;
        virtual uint64_t            largest_free_extent() = 0;
        virtual uint32_t            number_of_partitions() = 0;
        virtual uint16_t            health_status() = 0;
        virtual uint16_t            bus_type() = 0;
        virtual ST_PARTITION_STYLE  partition_style() = 0;
        virtual uint32_t            signature() = 0;
        virtual std::wstring        guid() = 0;
        virtual bool                is_offline() = 0;
        virtual uint16_t            offline_reason() = 0;
        virtual bool                is_read_only() = 0;
        virtual bool                is_system() = 0;
        virtual bool                is_clustered() = 0;
        virtual bool                is_boot() = 0;
        virtual bool                boot_from_disk() = 0;
        virtual bool                is_dynmaic();
        virtual uint32_t            sectors_per_track() = 0;
        virtual uint32_t            tracks_per_cylinder() = 0;
        virtual uint64_t            total_cylinders() = 0;
        virtual uint32_t            scsi_bus() = 0; 
        virtual uint16_t            scsi_logical_unit() = 0; 
        virtual uint16_t            scsi_port() = 0; 
        virtual uint16_t            scsi_target_id() = 0; 
        virtual volume::vtr         get_volumes() = 0;
        virtual partition::vtr      get_partitions() = 0;
    };

    virtual ~storage(){}
    virtual void rescan() = 0;
    virtual disk::vtr get_disks() = 0;
    virtual volume::vtr get_volumes() = 0;
    virtual partition::vtr get_partitions() = 0;

    virtual disk::ptr get_disk(uint32_t disk_number) = 0;
    virtual volume::vtr get_volumes(uint32_t disk_number) = 0;
    virtual partition::vtr get_partitions(uint32_t disk_number) = 0;


    static storage::ptr get(std::wstring host = L"", std::wstring user = L"", std::wstring password = L"");
    static storage::ptr local(); // Need to confirm if the output is correct.
};
};
};

#ifndef MACHO_HEADER_ONLY
#include <windows.h>
#include <initguid.h>
#include <vds.h>
#include "windows\wmi.hpp"
#include "common\stringutils.hpp"
#include "common\tracelog.hpp"
#include "common\guid.hpp"
#include "windows\auto_handle_base.hpp"
#include "windows\environment.hpp"
#include "memory"
#include <setupapi.h>   // for SetupDiXxx functions.
#include "WinIoCtl.h"
#include <devguid.h>    // Device guids
#include <cfgmgr32.h>   // for SetupDiXxx functions.
#include <ntddscsi.h>

#define IOCTL_DISK_IS_CLUSTERED             CTL_CODE(IOCTL_DISK_BASE, 0x003e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define BOOST_THROW_STORAGE_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( storage::exception, no, message )
#define BOOST_THROW_STORAGE_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING(storage::exception, message )

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH Spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             SenseBuf[32];
    UCHAR             DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

#define  MS_PARTITION_LDM           0x42                   // PARTITION_LDM
#define  CDB6GENERIC_LENGTH         6
#define  SCSIOP_INQUIRY             0x12

using namespace macho;
using namespace macho::windows;

MAKE_AUTO_HANDLE_CLASS_EX(auto_setupdi_handle, HDEVINFO, SetupDiDestroyDeviceInfoList, INVALID_HANDLE_VALUE);

class win32_logical_disk{
public:
    typedef boost::shared_ptr<win32_logical_disk> ptr;
    typedef std::vector<ptr> vtr;
    win32_logical_disk(wmi_object &obj) : _obj(obj){}
    virtual ~win32_logical_disk(){}
    virtual uint16_t access() { return _obj[L"Access"]; }
    virtual uint16_t availability() { return _obj[L"Availability"]; }
    virtual uint64_t block_size() { return _obj[L"BlockSize"]; }
    virtual std::wstring caption() { return _obj[L"Caption"]; }
    virtual bool compressed() { return _obj[L"Compressed"]; }
    virtual uint32_t config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
    virtual bool config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
    virtual std::wstring creation_class_name() { return _obj[L"CreationClassName"]; }
    virtual std::wstring description() { return _obj[L"Description"]; }
    virtual std::wstring device_id() { return _obj[L"DeviceID"]; }
    virtual uint32_t drive_type() { return _obj[L"DriveType"]; }
    virtual bool error_cleared() { return _obj[L"ErrorCleared"]; }
    virtual std::wstring error_description() { return _obj[L"ErrorDescription"]; }
    virtual std::wstring error_methodology() { return _obj[L"ErrorMethodology"]; }
    virtual std::wstring file_system() { return _obj[L"FileSystem"]; }
    virtual uint64_t free_space() { return _obj[L"FreeSpace"]; }
    //virtual datetime InstallDate() { return _obj[L"InstallDate"]; }
    virtual uint32_t last_error_code() { return _obj[L"LastErrorCode"]; }
    virtual uint32_t maximum_component_length() { return _obj[L"MaximumComponentLength"]; }
    virtual uint32_t media_type() { return _obj[L"MediaType"]; }
    virtual std::wstring name() { return _obj[L"Name"]; }
    virtual uint64_t number_of_blocks() { return _obj[L"NumberOfBlocks"]; }
    virtual std::wstring pnp_device_id() { return _obj[L"PNPDeviceID"]; }
    virtual uint16_table power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
    virtual bool power_management_supported() { return _obj[L"PowerManagementSupported"]; }
    virtual std::wstring provider_name() { return _obj[L"ProviderName"]; }
    virtual std::wstring purpose() { return _obj[L"Purpose"]; }
    virtual bool quotas_disabled() { return _obj[L"QuotasDisabled"]; }
    virtual bool quotas_incomplete() { return _obj[L"QuotasIncomplete"]; }
    virtual bool quotas_rebuilding() { return _obj[L"QuotasRebuilding"]; }
    virtual uint64_t size() { return _obj[L"Size"]; }
    virtual std::wstring status() { return _obj[L"Status"]; }
    virtual uint64_t status_info() { return _obj[L"StatusInfo"]; }
    virtual bool supports_disk_quotas() { return _obj[L"SupportsDiskQuotas"]; }
    virtual bool supports_file_based_compression() { return _obj[L"SupportsFileBasedCompression"]; }
    virtual std::wstring system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
    virtual std::wstring system_name() { return _obj[L"SystemName"]; }
    virtual bool volume_dirty() { return _obj[L"VolumeDirty"]; }
    virtual std::wstring volume_name() { return _obj[L"VolumeName"]; }
    virtual std::wstring volume_serial_number() { return _obj[L"VolumeSerialNumber"]; }

private:
    macho::windows::wmi_object _obj;
};

class win32_disk_partition{ //Win32_DiskDriveToDiskPartition 
public:
    typedef boost::shared_ptr<win32_disk_partition> ptr;
    typedef std::vector<ptr> vtr;
    win32_disk_partition(wmi_object &obj) : _obj(obj){}
    virtual ~win32_disk_partition(){}
    virtual uint16_t access() { return _obj[L"Access"]; }
    virtual uint16_t availability() { return _obj[L"Availability"]; }
    virtual uint64_t block_size() { return _obj[L"BlockSize"]; }
    virtual bool bootable() { return _obj[L"Bootable"]; }
    virtual bool boot_partition() { return _obj[L"BootPartition"]; }
    virtual std::wstring caption() { return _obj[L"Caption"]; }
    virtual uint32_t config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
    virtual bool config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
    virtual std::wstring creation_class_name() { return _obj[L"CreationClassName"]; }
    virtual std::wstring description() { return _obj[L"Description"]; }
    virtual std::wstring device_id() { return _obj[L"DeviceID"]; }
    virtual uint32_t disk_index() { return _obj[L"DiskIndex"]; }
    virtual bool error_cleared() { return _obj[L"ErrorCleared"]; }
    virtual std::wstring error_description() { return _obj[L"ErrorDescription"]; }
    virtual std::wstring error_methodology() { return _obj[L"ErrorMethodology"]; }
    virtual uint32_t hidden_sectors() { return _obj[L"HiddenSectors"]; }
    virtual uint32_t index() { return _obj[L"Index"]; }
    //virtual datetime InstallDate() { return _obj[L"InstallDate"]; }
    virtual uint32_t last_error_code() { return _obj[L"LastErrorCode"]; }
    virtual std::wstring name() { return _obj[L"Name"]; }
    virtual uint64_t number_of_blocks() { return _obj[L"NumberOfBlocks"]; }
    virtual std::wstring pnp_device_id() { return _obj[L"PNPDeviceID"]; }
    virtual uint16_table power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
    virtual bool power_management_supported() { return _obj[L"PowerManagementSupported"]; }
    virtual bool primary_partition() { return _obj[L"PrimaryPartition"]; }
    virtual std::wstring purpose() { return _obj[L"Purpose"]; }
    virtual bool rewrite_partition() { return _obj[L"RewritePartition"]; }
    virtual uint64_t size() { return _obj[L"Size"]; }
    virtual uint64_t starting_offset() { return _obj[L"StartingOffset"]; }
    virtual std::wstring status() { return _obj[L"Status"]; }
    virtual uint16_t status_info() { return _obj[L"StatusInfo"]; }
    virtual std::wstring system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
    virtual std::wstring system_name() { return _obj[L"SystemName"]; }
    virtual std::wstring type() { return _obj[L"Type"]; }

    win32_logical_disk::vtr get_logical_disks(){
        win32_logical_disk::vtr logical_disks;
        wmi_object_table objs = _obj.get_relateds(L"Win32_LogicalDisk", L"Win32_LogicalDiskToPartition");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            logical_disks.push_back(win32_logical_disk::ptr(new win32_logical_disk(*obj)));
        }
        return logical_disks;
    }

private:
    macho::windows::wmi_object _obj;
};

class win32_disk_drive{
public:
    typedef boost::shared_ptr<win32_disk_drive> ptr;
    typedef std::vector<ptr> vtr;
    win32_disk_drive(wmi_object &obj) : _obj(obj){}
    virtual ~win32_disk_drive(){}
    
    virtual uint16_t availability() { return _obj[L"Availability"]; }
    virtual uint32_t bytes_per_sector() { return _obj[L"BytesPerSector"]; }
    virtual uint16_table capabilities() { return _obj[L"Capabilities"]; }
    virtual string_array_w capabilityDescriptions() { return _obj[L"CapabilityDescriptions"]; }
    virtual std::wstring caption() { return _obj[L"Caption"]; }
    virtual std::wstring compression_method() { return _obj[L"CompressionMethod"]; }
    virtual uint32_t config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
    virtual bool config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
    virtual std::wstring creation_class_name() { return _obj[L"CreationClassName"]; }
    virtual uint64_t default_block_size() { return _obj[L"DefaultBlockSize"]; }
    virtual std::wstring description() { return _obj[L"Description"]; }
    virtual std::wstring device_id() { return _obj[L"DeviceID"]; }
    virtual bool error_cleared() { return _obj[L"ErrorCleared"]; }
    virtual std::wstring error_description() { return _obj[L"ErrorDescription"]; }
    virtual std::wstring error_methodology() { return _obj[L"ErrorMethodology"]; }
    virtual std::wstring firmware_revision() { return _obj[L"FirmwareRevision"]; }
    virtual uint32_t index() { return _obj[L"Index"]; }
    //virtual datetime InstallDate() { return _obj[L"InstallDate"]; }
    virtual std::wstring interface_type() { return _obj[L"InterfaceType"]; }
    virtual uint32_t last_error_code() { return _obj[L"LastErrorCode"]; }
    virtual std::wstring manufacturer() { return _obj[L"Manufacturer"]; }
    virtual uint64_t max_block_size() { return _obj[L"MaxBlockSize"]; }
    virtual uint64_t max_media_size() { return _obj[L"MaxMediaSize"]; }
    virtual bool media_loaded() { return _obj[L"MediaLoaded"]; }
    virtual std::wstring media_type() { return _obj[L"MediaType"]; }
    virtual uint64_t min_block_size() { return _obj[L"MinBlockSize"]; }
    virtual std::wstring model() { return _obj[L"Model"]; }
    virtual std::wstring name() { return _obj[L"Name"]; }
    virtual bool deeds_cleaning() { return _obj[L"NeedsCleaning"]; }
    virtual uint32_t number_of_media_supported() { return _obj[L"NumberOfMediaSupported"]; }
    virtual uint32_t partitions() { return _obj[L"Partitions"]; }
    virtual std::wstring pnp_device_id() { return _obj[L"PNPDeviceID"]; }
    virtual uint16_table power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
    virtual bool power_management_supported() { return _obj[L"PowerManagementSupported"]; }
    virtual uint32_t scsi_bus() { return _obj[L"SCSIBus"]; }
    virtual uint16_t scsi_logical_unit() { return _obj[L"SCSILogicalUnit"]; }
    virtual uint16_t scsi_port() { return _obj[L"SCSIPort"]; }
    virtual uint16_t scsi_target_id() { return _obj[L"SCSITargetId"]; }
    virtual uint32_t sectors_per_track() { return _obj[L"SectorsPerTrack"]; }
    virtual std::wstring serial_number() { return _obj[L"SerialNumber"]; }
    virtual uint32_t signature() { return _obj[L"Signature"]; }
    virtual uint64_t size() { return _obj[L"Size"]; }
    virtual std::wstring status() { return _obj[L"Status"]; }
    virtual uint16_t status_info() { return _obj[L"StatusInfo"]; }
    virtual std::wstring system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
    virtual std::wstring system_name() { return _obj[L"SystemName"]; }
    virtual uint64_t total_cylinders() { return _obj[L"TotalCylinders"]; }
    virtual uint32_t total_heads() { return _obj[L"TotalHeads"]; }
    virtual uint64_t total_sectors() { return _obj[L"TotalSectors"]; }
    virtual uint64_t total_tracks() { return _obj[L"TotalTracks"]; }
    virtual uint32_t tracks_per_cylinder() { return _obj[L"TracksPerCylinder"]; }

    win32_disk_partition::vtr get_partitons(){
        win32_disk_partition::vtr partitions;
        wmi_object_table objs = _obj.get_relateds(L"Win32_DiskPartition", L"Win32_DiskDriveToDiskPartition");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            partitions.push_back(win32_disk_partition::ptr(new win32_disk_partition(*obj)));
        }
        return partitions;
    }

private:
    macho::windows::wmi_object _obj;
};

class win32_directory{ // Win32_MountPoint 
public:
    typedef boost::shared_ptr<win32_directory> ptr;
    typedef std::vector<ptr> vtr;
    win32_directory(wmi_object &obj) : _obj(obj){}
    virtual ~win32_directory(){}
    virtual uint32_t         access_mask() { return _obj[L"AccessMask"]; }
    virtual bool             archive() { return _obj[L"Archive"]; }
    virtual std::wstring     caption() { return _obj[L"Caption"]; }
    virtual bool             compressed() { return _obj[L"Compressed"]; }
    virtual std::wstring     compressionMethod() { return _obj[L"CompressionMethod"]; }
    virtual std::wstring     creationClassName() { return _obj[L"CreationClassName"]; }
    //virtual datetime       CreationDate() { return _obj[L"CreationDate"]; }
    virtual std::wstring     cs_creation_class_name() { return _obj[L"CSCreationClassName"]; }
    virtual std::wstring     cs_name() { return _obj[L"CSName"]; }
    virtual std::wstring     description() { return _obj[L"Description"]; }
    virtual std::wstring     drive() { return _obj[L"Drive"]; }
    virtual std::wstring     eight_dot_three_file_name() { return _obj[L"EightDotThreeFileName"]; }   
    virtual bool             encrypted() { return _obj[L"Encrypted"]; }
    virtual std::wstring     encryption_method() { return _obj[L"EncryptionMethod"]; }
    virtual std::wstring     extension() { return _obj[L"Extension"]; }
    virtual std::wstring     fileName() { return _obj[L"FileName"]; }
    virtual uint64_t         file_size() { return _obj[L"FileSize"]; }
    virtual std::wstring     file_type() { return _obj[L"FileType"]; }
    virtual std::wstring     fs_creation_class_name() { return _obj[L"FSCreationClassName"]; }
    virtual std::wstring     fs_name() { return _obj[L"FSName"]; }
    virtual bool             hidden() { return _obj[L"Hidden"]; }
    //virtual datetime       install_date() { return _obj[L"InstallDate"]; }
    virtual uint64_t         in_use_count() { return _obj[L"InUseCount"]; }
    //virtual datetime       LastAccessed() { return _obj[L"LastAccessed"]; }
    //virtual datetime       LastModified() { return _obj[L"LastModified"]; }
    virtual std::wstring     name() { return _obj[L"Name"]; }
    virtual std::wstring     path() { return _obj[L"Path"]; }
    virtual bool             readable() { return _obj[L"Readable"]; }
    virtual std::wstring     status() { return _obj[L"Status"]; }
    virtual bool             system() { return _obj[L"System"]; }
    virtual bool             writeable() { return _obj[L"Writeable"]; }

private:
    macho::windows::wmi_object _obj;
};

class win32_volume{
public:
    typedef boost::shared_ptr<win32_volume> ptr;
    typedef std::vector<ptr> vtr;
    win32_volume(wmi_object &obj) : _obj(obj){}
    virtual ~win32_volume(){}
    virtual uint16_t access() { return _obj[L"Access"]; }
    virtual bool     automount() { return _obj[L"Automount"]; }
    virtual uint16_t availability() { return _obj[L"Availability"]; }
    virtual uint64_t block_size() { return _obj[L"BlockSize"]; }
    virtual uint64_t capacity() { return _obj[L"Capacity"]; }
    virtual std::wstring caption() { return _obj[L"Caption"]; }
    virtual bool compressed() { return _obj[L"Compressed"]; }
    virtual uint32_t config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
    virtual bool config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
    virtual std::wstring creation_class_name() { return _obj[L"CreationClassName"]; }
    virtual std::wstring description() { return _obj[L"Description"]; }
    virtual std::wstring device_id() { return _obj[L"DeviceID"]; }
    virtual bool         dirty_bit_set() { return _obj[L"DirtyBitSet"]; }
    virtual std::wstring drive_letter() { return _obj[L"DriveLetter"]; }
    virtual storage::ST_DRIVE_TYPE drive_type() { return (storage::ST_DRIVE_TYPE)(uint32_t)_obj[L"DriveType"]; }
    virtual bool         error_cleared() { return _obj[L"ErrorCleared"]; }
    virtual std::wstring error_description() { return _obj[L"ErrorDescription"]; }
    virtual std::wstring error_methodology() { return _obj[L"ErrorMethodology"]; }
    virtual std::wstring file_system() { return _obj[L"FileSystem"]; }
    virtual uint64_t freeSpace() { return _obj[L"FreeSpace"]; }
    virtual bool indexing_enabled() { return _obj[L"IndexingEnabled"]; }
    //virtual datetime InstallDate() { return _obj[L"InstallDate"]; }
    virtual std::wstring label() { return _obj[L"Label"]; }
    virtual uint32_t last_error_code() { return _obj[L"LastErrorCode"]; }
    virtual uint32_t maximum_file_name_length() { return _obj[L"MaximumFileNameLength"]; }
    virtual std::wstring name() { return _obj[L"Name"]; }
    virtual uint64_t number_of_blocks() { return _obj[L"NumberOfBlocks"]; }
    virtual std::wstring pnp_device_id() { return _obj[L"PNPDeviceID"]; }
    virtual uint16_table power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
    virtual bool power_management_supported() { return _obj[L"PowerManagementSupported"]; }
    virtual std::wstring purpose() { return _obj[L"Purpose"]; }
    virtual bool quotas_enabled() { return _obj[L"QuotasEnabled"]; }
    virtual bool quotas_incomplete() { return _obj[L"QuotasIncomplete"]; }
    virtual bool quotas_rebuilding() { return _obj[L"QuotasRebuilding"]; }
    virtual std::wstring status() { return _obj[L"Status"]; }
    virtual uint16_t status_info() { return _obj[L"StatusInfo"]; }
    virtual std::wstring system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
    virtual std::wstring system_name() { return _obj[L"SystemName"]; }
    virtual uint32_t serial_number() { return _obj[L"SerialNumber"]; }
    virtual bool supports_disk_quotas() { return _obj[L"SupportsDiskQuotas"]; }
    virtual bool supports_file_based_compression() { return _obj[L"SupportsFileBasedCompression"]; }

    virtual bool           mount(){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        in = _obj.get_input_parameters(L"Mount");
        hr = _obj.exec_method(L"Mount", in, out, ret);
        if (SUCCEEDED(hr) && 0 == ret)
            return true;
        else
            LOG(LOG_LEVEL_ERROR, L"Error (%d) - (0x%08X)", ret, hr);
        return false;
    }

    virtual bool           dismount(bool force, bool permanent){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        in = _obj.get_input_parameters(L"Dismount");
        in.set_parameter(L"Force", force);
        in.set_parameter(L"Permanent", permanent);
        hr = _obj.exec_method(L"Dismount", in, out, ret);
        if (SUCCEEDED(hr) && 0 == ret)
            return true;
        else
            LOG(LOG_LEVEL_ERROR, L"Error (%d) - (0x%08X)", ret, hr);
        return false;
    }

    win32_directory::vtr get_mount_points(){
        win32_directory::vtr mount_points;
        wmi_object_table objs = _obj.get_relateds(L"Win32_Directory", L"Win32_MountPoint");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            mount_points.push_back(win32_directory::ptr(new win32_directory(*obj)));
        }
        return mount_points;
    }
private:
    macho::windows::wmi_object _obj;
};

class win32_storage{
public:
    typedef boost::shared_ptr<win32_storage> ptr;
    win32_storage(std::wstring host, std::wstring domain, std::wstring user, std::wstring password) : _cache_mode (false){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        macho::windows::wmi_services root;
        if (host.length())
            hr = root.connect(L"", host, domain, user, password);
        else
            hr = root.connect(L"");
        hr = root.open_namespace(L"CIMV2", _cimv2);
    }
    win32_storage(macho::windows::wmi_services& root) : _cache_mode(false){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        hr = root.open_namespace(L"CIMV2", _cimv2);
    }
    virtual ~win32_storage(){}

    win32_disk_drive::vtr get_win32_disk_drives(){
        if (_cache_mode && _cache_win32_disk_drives.size())
            return _cache_win32_disk_drives;
        win32_disk_drive::vtr disks;
        wmi_object_table objs = _cimv2.query_wmi_objects(L"Win32_DiskDrive");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            disks.push_back(win32_disk_drive::ptr(new win32_disk_drive(*obj)));
        }
        _cache_win32_disk_drives = disks;
        return disks;
    }

    win32_disk_drive::ptr get_win32_disk_drive(uint32_t disk_number){
        win32_disk_drive::ptr disk;
        std::wstring query = boost::str(boost::wformat(L"Select * From Win32_DiskDrive Where Index=%d") % disk_number);
        wmi_object_table objs = _cimv2.exec_query(query);
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            disk = win32_disk_drive::ptr(new win32_disk_drive(*obj));
        }
        return disk;
    }

    win32_volume::vtr get_win32_volumes(){
        if (_cache_mode && _cache_win32_volumes.size())
            return _cache_win32_volumes;
        win32_volume::vtr volumes;
        wmi_object_table objs = _cimv2.query_wmi_objects(L"Win32_Volume");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            volumes.push_back(win32_volume::ptr(new win32_volume(*obj)));
        }
        _cache_win32_volumes = volumes;
        return volumes;
    }

    win32_volume::ptr get_win32_volume(std::wstring volume_device_id){
        win32_volume::ptr volume;
        if ((volume_device_id[0] == _T('\\')) &&
            (volume_device_id[1] == _T('\\')) &&
            (volume_device_id[2] == _T('?')) &&
            (volume_device_id[3] == _T('\\'))){
            if (volume_device_id[volume_device_id.length() - 1] == _T('\\'))
                volume_device_id.erase(volume_device_id.length());
            try{
                std::wstring query = boost::str(boost::wformat(L"Select * from Win32_Volume where DeviceID = '\\\\\\\\?\\\\%s\\\\'") % &volume_device_id[4]);
                wmi_object_table objs = _cimv2.exec_query(query);
                for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                    volume = win32_volume::ptr(new win32_volume(*obj));
                }
            }
            catch (macho::windows::wmi_exception &ex){
            }
        }
        return volume;
    }

    void enable_cache_mode(){ _cache_mode = true; }
    void disable_cache_mode() { 
        _cache_mode = false;  
        if (_cache_win32_volumes.size())
            _cache_win32_volumes.clear();
        if (_cache_win32_disk_drives.size())
            _cache_win32_disk_drives.clear();
    }
private:
    macho::windows::wmi_services _cimv2;
    bool                         _cache_mode;
    win32_volume::vtr            _cache_win32_volumes;
    win32_disk_drive::vtr        _cache_win32_disk_drives;
};

class vds_storage : virtual public storage {
public:
    vds_storage(std::wstring host, std::wstring domain, std::wstring user, std::wstring password) :
        _host(host),
        _domain(domain),
        _user(user),
        _password(password),
        _local(false){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        ATL::CComPtr<IVdsServiceLoader> pLoader;
        if (0 == _host.length()){
            hr = CoCreateInstance(CLSID_VdsLoader,
                NULL,
                CLSCTX_LOCAL_SERVER,
                IID_IVdsServiceLoader,
                (void **)&pLoader);
            if (FAILED(hr))
                BOOST_THROW_STORAGE_EXCEPTION(hr, L"CoCreateInstanceEx failed.");
            _local = true;
        }
        else{
            COAUTHIDENTITY      ident = { 0 };
            COAUTHINFO          auth = { 0 };
            COSERVERINFO        server_info = { 0 };
            MULTI_QI            mqi = { 0 };
            WCHAR               principal[128] = L"";

            if (password.length()){
                ident.User = (unsigned short*)user.c_str();
                ident.UserLength = (unsigned long)user.length();
                ident.Password = (unsigned short*)password.c_str();
                ident.PasswordLength = (unsigned long)password.length();
                ident.Domain = (unsigned short*)domain.c_str();
                ident.DomainLength = (unsigned long)domain.length();
                ident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                swprintf_s(principal, L"RPCSS/%s", _host.c_str());
                auth.dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
                auth.dwAuthnSvc = RPC_C_AUTHN_DEFAULT;
                auth.dwAuthzSvc = RPC_C_AUTHZ_NONE;
                auth.dwCapabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
                auth.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
                auth.pwszServerPrincName = principal;
                auth.pAuthIdentityData = &ident;
                server_info.pAuthInfo = &auth;
            }
            
            server_info.pwszName = (LPWSTR)_host.c_str();
            mqi.pIID = &__uuidof(IVdsServiceLoader);

            hr = CoCreateInstanceEx(
                CLSID_VdsLoader,
                NULL,
                CLSCTX_REMOTE_SERVER,
                &server_info,
                1,
                &mqi);
            if (FAILED(hr))
                BOOST_THROW_STORAGE_EXCEPTION(hr, L"CoCreateInstanceEx failed.");
            
            CComPtr<IUnknown> pUnknown;
            pUnknown.Attach(mqi.pItf);
            hr = pUnknown->QueryInterface(__uuidof(IVdsServiceLoader), (void **)&pLoader);
            if (FAILED(hr))
                BOOST_THROW_STORAGE_EXCEPTION(hr, L"pUnknown->QueryInterface failed.");
            set_auth(pLoader);
        }
        hr = pLoader->LoadService( _host.length() ? (LPWSTR) _host.c_str() : NULL, &_vds);
        if (FAILED(hr))
            BOOST_THROW_STORAGE_EXCEPTION(hr, L"LoadService failed.");

        if (host.length() > 0)
            set_auth(_vds);
        hr = _vds->WaitForServiceReady();
        _win32_storage = win32_storage::ptr(new win32_storage(host, domain, user, password));
#ifdef _WIN2K3
        get_system_device_number();
#endif
    }
    virtual ~vds_storage(){}
    virtual void rescan(){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        if(FAILED( hr = _vds->Reenumerate()))
            BOOST_THROW_STORAGE_EXCEPTION(hr, L"_vds->Reenumerate failed.");
        ::Sleep(2000);
        if(FAILED( hr = _vds->Refresh()))
            BOOST_THROW_STORAGE_EXCEPTION(hr, L"_vds->Refresh failed.");
    }
    virtual storage::disk::vtr get_disks();
    virtual storage::partition::vtr get_partitions();
    virtual storage::volume::vtr    get_volumes();

    virtual storage::disk::ptr get_disk(uint32_t disk_number){
        storage::disk::vtr disks = get_disks();
        for (storage::disk::vtr::iterator d = disks.begin(); d != disks.end(); d++)
            if ((*d)->number() == disk_number) return (*d);
        return NULL;
    }
    virtual storage::partition::vtr get_partitions(uint32_t disk_number){
        storage::disk::vtr disks = get_disks();
        for (storage::disk::vtr::iterator d = disks.begin(); d != disks.end(); d++)
            if ((*d)->number() == disk_number) return (*d)->get_partitions();
        return storage::partition::vtr();
    }
    virtual storage::volume::vtr    get_volumes(uint32_t disk_number){
        storage::disk::vtr disks = get_disks();
        for (storage::disk::vtr::iterator d = disks.begin(); d != disks.end(); d++)
            if ((*d)->number() == disk_number) return (*d)->get_volumes();
        return storage::volume::vtr();
    }
    virtual win32_volume::vtr       get_win32_volumes(){
        return _win32_storage->get_win32_volumes();
    }
private:
#ifdef _WIN2K3
    void                                get_system_device_number();
#endif
    friend class vds_disk;
    friend class vds_volume;
    storage::disk::vtr                  get_unallocated_disks();
    storage::disk::vtr                  get_disks_from_enum_sw_provider();
    storage::disk::vtr                  get_disks_from_vds_pack(ATL::CComPtr<IVdsPack>& vds_pack);
    storage::volume::vtr                get_volumes_from_vds_pack(ATL::CComPtr<IVdsPack>& vds_pack);
    HRESULT                             enumerate_objects(ATL::CComPtr<IEnumVdsObject> &pEnumeration, std::vector<ATL::CComPtr<IUnknown> > &vpUnknown);
    HRESULT                             query_packs(ATL::CComPtr<IVdsSwProvider> &pSwProvider, std::vector<ATL::CComPtr<IVdsPack> > &vPacks);
    void set_auth(IUnknown *pUnknown){
        HRESULT hr = S_OK;  
        if (!_local){
            FUN_TRACE_HRESULT(hr);
            SEC_WINNT_AUTH_IDENTITY_W auth;
            memset(&auth, 0, sizeof(auth));
            std::wstring user     = _user;
            std::wstring password = _password;
            std::wstring domain   = _domain;
            auth.User = (unsigned short*)user.c_str();
            auth.UserLength = (unsigned long)user.length();
            auth.Password = (unsigned short*)password.c_str();
            auth.PasswordLength = (unsigned long)password.length();
            auth.Domain = (unsigned short*)domain.c_str();
            auth.DomainLength = (unsigned long)domain.length();
            auth.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            hr = CoSetProxyBlanket(
                pUnknown,
                RPC_C_AUTHN_DEFAULT,
                RPC_C_AUTHZ_NONE,
                COLE_DEFAULT_PRINCIPAL,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                &auth,
                EOAC_MUTUAL_AUTH);
            if (FAILED(hr))
                BOOST_THROW_STORAGE_EXCEPTION(hr, L"CoSetProxyBlanket failed.");
        }
    }
    win32_storage::ptr         _win32_storage;
    ATL::CComPtr<IVdsService>  _vds;
    std::wstring               _host;
#ifdef _DEBUG
    std::wstring               _domain;
    std::wstring               _user;
    std::wstring               _password;
#else
    protected_data             _domain;
    protected_data             _user;
    protected_data             _password;
#endif
    bool                       _local;
#ifdef _WIN2K3
    uint32_t                   _boot_disk;
    uint32_t                   _system_partition;
#endif
};

#ifndef CheckWstr
#define CheckWstr(x) x ? x : L""
#endif

struct vds_extent{
    typedef boost::shared_ptr<vds_extent> ptr;
    typedef std::vector<ptr> vtr;
    vds_extent(int disk_number, VDS_DISK_EXTENT& extent) :_disk_number(disk_number){
        CopyMemory(&_extent, &extent, sizeof(VDS_DISK_EXTENT));
    }
    virtual ~vds_extent(){
    }
    VDS_DISK_EXTENT _extent;
    int             _disk_number;
};

class vds_volume : virtual public storage::volume{
public:
    vds_volume(vds_storage& vds, ATL::CComPtr<IVdsVolume>& vdsvolume) : _vds(vds), _vds_volume(vdsvolume), _drive_type(storage::ST_DRIVE_TYPE::ST_UNKNOWN){
        ZeroMemory(&_vol_fs_prop, sizeof(VDS_FILE_SYSTEM_PROP));
        refresh();
    }
    virtual ~vds_volume(){ cleanup(); }
    // Only for vds_volume
    virtual bool         is_offline() { return VDS_VS_OFFLINE == (VDS_VS_OFFLINE & (_vds_volume2 ? _vol_prop2_ptr->status : _vol_prop_ptr->status)); }
    virtual bool         is_system() { return VDS_VF_SYSTEM_VOLUME == (VDS_VF_SYSTEM_VOLUME & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_boot() { return VDS_VF_BOOT_VOLUME == (VDS_VF_BOOT_VOLUME  & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_active() { return VDS_VF_ACTIVE == (VDS_VF_ACTIVE & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_readonly() { return VDS_VF_READONLY == (VDS_VF_READONLY & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_hidden() { return VDS_VF_HIDDEN == (VDS_VF_HIDDEN & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         can_extend() { return VDS_VF_CAN_EXTEND == (VDS_VF_CAN_EXTEND & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         can_shrink() { return VDS_VF_CAN_SHRINK == (VDS_VF_CAN_SHRINK & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         has_page_file() { return VDS_VF_PAGEFILE == (VDS_VF_PAGEFILE & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         has_crash_dump() { return VDS_VF_CRASHDUMP == (VDS_VF_CRASHDUMP & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         can_installable() { return VDS_VF_INSTALLABLE == (VDS_VF_INSTALLABLE & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_formatiing() { return VDS_VF_FORMATTING == (VDS_VF_FORMATTING & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         not_formattable() { return VDS_VF_NOT_FORMATTABLE == (VDS_VF_NOT_FORMATTABLE & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         no_default_drive_letter() { return VDS_VF_NO_DEFAULT_DRIVE_LETTER == (VDS_VF_NO_DEFAULT_DRIVE_LETTER & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         dismounted() { return VDS_VF_PERMANENTLY_DISMOUNTED == (VDS_VF_PERMANENTLY_DISMOUNTED & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         support_dismount() { return VDS_VF_PERMANENT_DISMOUNT_SUPPORTED == (VDS_VF_PERMANENT_DISMOUNT_SUPPORTED & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_shadow_copy() { return VDS_VF_SHADOW_COPY == (VDS_VF_SHADOW_COPY & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    virtual bool         is_dirty() { return VDS_VF_DIRTY == (VDS_VF_DIRTY & (_vds_volume2 ? _vol_prop2_ptr->ulFlags : _vol_prop_ptr->ulFlags)); }
    std::wstring         volume_id() { return  macho::guid_(_vds_volume2 ? _vol_prop2_ptr->id : _vol_prop_ptr->id).wstring(); }

    virtual bool         mount(){
        HRESULT hr = E_FAIL;
        FUN_TRACE_HRESULT(hr);
        if (_vds_volume_mf){
            hr = _vds_volume_mf->Mount();
            return S_OK == hr;
        }
        return false;
    }
    virtual bool         dismount(bool force = false, bool permanent = false){
        HRESULT hr = E_FAIL;
        FUN_TRACE_HRESULT(hr);
        if (_vds_volume_mf){
            hr = _vds_volume_mf->Dismount( force, permanent);
            return (S_OK == hr || VDS_E_VOLUME_TEMPORARILY_DISMOUNTED == hr);
        }
        return false;
    }

    virtual bool           format(
        std::wstring file_system,
        std::wstring file_system_label,
        uint32_t allocation_unit_size,
        bool full,
        bool force,
        bool compress,
        bool short_file_name_support,
        bool set_integrity_streams,
        bool use_large_frs,
        bool disable_heat_gathering){
        HRESULT hr = E_FAIL;
        FUN_TRACE_HRESULT(hr);
        if (_vds_volume_mf){
            VDS_FILE_SYSTEM_TYPE type = VDS_FST_UNKNOWN;
            if (!_wcsicmp(file_system.c_str(), L"ExFAT"))
                type = VDS_FST_EXFAT;
            else if (!_wcsicmp(file_system.c_str(), L"FAT"))
                type = VDS_FST_FAT;
            else if (!_wcsicmp(file_system.c_str(), L"FAT32"))
                type = VDS_FST_FAT32;
            else if (!_wcsicmp(file_system.c_str(), L"NTFS"))
                type = VDS_FST_NTFS;
            else if (!_wcsicmp(file_system.c_str(), L"ReFS"))
                type = (VDS_FILE_SYSTEM_TYPE)(VDS_FST_NTFS + 5); // VDS_FST_REFS       
            else
                return false;
            ATL::CComQIPtr<IVdsAsync> async;
            hr = _vds_volume_mf->Format(type, (LPWSTR)file_system_label.c_str(), allocation_unit_size, force, !full, compress, &async);
            return (S_OK == hr);
        }
        return false;
    }

    // General Functions
    virtual std::wstring   id() {
        if (!_volume_guid_path.length()) return L"";
        std::wstring::size_type first = _volume_guid_path.find_first_of(L"{");
        std::wstring::size_type last = _volume_guid_path.find_last_of(L"}");
        if (first != std::wstring::npos && last != std::wstring::npos)
            return _volume_guid_path.substr(first + 1, last - first - 1);
        else
            return L"";
    }

    virtual std::wstring   drive_letter() {
        string_array_w _access_paths = access_paths();
        foreach(std::wstring p, _access_paths){
            if (p.length() == 3)
                return p.substr(0, 1);
        }
        return L"";
    }
    
    virtual storage::ST_VOLUME_TYPE type(){
        return (storage::ST_VOLUME_TYPE)((_vds_volume2 ? _vol_prop2_ptr->type : _vol_prop_ptr->type) - VDS_VT_SIMPLE + 1);
    }

    virtual std::wstring   path() { return _volume_guid_path; }
    virtual uint16_t       health_status() { return _vds_volume2 ? _vol_prop2_ptr->health : _vol_prop_ptr->health; }
    virtual std::wstring   file_system() { 
        std::wstring fs;
        ATL::CComQIPtr<IVdsVolumeMF2>  _vds_volume_mf2 = _vds_volume;
        if (_vds_volume_mf2){
            LPWSTR pwszFileSystemTypeName = NULL;
            _vds.set_auth(_vds_volume_mf2);
            HRESULT hr = _vds_volume_mf2->GetFileSystemTypeName(&pwszFileSystemTypeName);
            if (SUCCEEDED(hr)){
                fs = pwszFileSystemTypeName;
                CoTaskMemFree(pwszFileSystemTypeName);
            }
        }
        else{
            switch (_vol_fs_prop.type){
            case VDS_FST_EXFAT:
                fs = L"EXFAT"; break;
            case VDS_FST_UDF:
                fs = L"UDF"; break;
            case VDS_FST_CDFS:
                fs = L"CDFS"; break;
            case VDS_FST_NTFS:
                fs = L"NTFS"; break;
            case VDS_FST_FAT32:
                fs = L"FAT32"; break;
            case VDS_FST_FAT:
                fs = L"FAT"; break;
            case VDS_FST_RAW:
                fs = L"RAW"; break;
            case VDS_FST_UNKNOWN:
            default:
                fs = L"UNKNOWN";
            }
        }
        return fs;
    }
    virtual std::wstring   file_system_label() { return CheckWstr(_vol_fs_prop.pwszLabel); }
    virtual uint64_t       size() { return _vol_fs_prop.ullTotalAllocationUnits * _vol_fs_prop.ulAllocationUnitSize; }
    virtual uint64_t       size_remaining() { return _vol_fs_prop.ullAvailableAllocationUnits * _vol_fs_prop.ulAllocationUnitSize; }
    virtual storage::ST_DRIVE_TYPE  drive_type() { return _drive_type; }
    virtual string_array_w access_paths() {
        string_array_w _access_paths;
        if (_vds_volume_mf){
            LPWSTR *pwszPathArray;
            LONG lNumberOfAccessPaths = 0;
            HRESULT hr = _vds_volume_mf->QueryAccessPaths(&pwszPathArray, &lNumberOfAccessPaths);
            if (SUCCEEDED(hr)){
                for (LONG i = 0; i < lNumberOfAccessPaths; i++){
                    _access_paths.push_back(pwszPathArray[i]);
                    CoTaskMemFree(pwszPathArray[i]);
                }
                CoTaskMemFree(pwszPathArray);
            }
        }
        if (_volume_guid_path.length())_access_paths.push_back(_volume_guid_path);
        return _access_paths;
    }

    virtual uint32_t set_attributes(bool is_read_only, bool _no_default_drive_letter, bool _is_hidden){
        HRESULT hr = S_OK;
        if (is_read_only != is_readonly()){
            hr = is_read_only ? _vds_volume->SetFlags(VDS_VF_READONLY, FALSE) : _vds_volume->ClearFlags(VDS_VF_READONLY);
        }
        if (SUCCEEDED(hr) && _no_default_drive_letter != no_default_drive_letter()){
            hr = _no_default_drive_letter ? _vds_volume->SetFlags(VDS_VF_NO_DEFAULT_DRIVE_LETTER, FALSE) : _vds_volume->ClearFlags(VDS_VF_NO_DEFAULT_DRIVE_LETTER);
        }
        if (SUCCEEDED(hr) && _is_hidden != is_hidden()){
            hr = _is_hidden ? _vds_volume->SetFlags(VDS_VF_HIDDEN, FALSE) : _vds_volume->ClearFlags(VDS_VF_HIDDEN);
        }
        refresh();
        return hr;
    }

private:

    virtual std::wstring   get_volume_guid_path() {
        ATL::CComQIPtr<IVdsVolumeMF3>  _vds_volume_mf3 = _vds_volume;
        string_array_w _paths;
        if (_vds_volume_mf3){
            LPWSTR *pwszPathArray = NULL;
            ULONG  ulNumberOfPath = 0;
            _vds.set_auth(_vds_volume_mf3);
            HRESULT hr = _vds_volume_mf3->QueryVolumeGuidPathnames(&pwszPathArray, &ulNumberOfPath);
            if (SUCCEEDED(hr)){
                for (ULONG i = 0; i < ulNumberOfPath; i++){
                    _paths.push_back(pwszPathArray[i]);
                    CoTaskMemFree(pwszPathArray[i]);
                }
                CoTaskMemFree(pwszPathArray);
            }
        }
        return _paths.size() ? _paths[0] : L"";
    }
    void refresh(){
        HRESULT hr = S_OK;
        cleanup();
        if (!_vds_volume2){
            _vds_volume2 = _vds_volume;
            if (_vds_volume2)
                _vds.set_auth(_vds_volume2);
        }
        if (_vds_volume2){
            
            if (!_vol_prop2_ptr)
                _vol_prop2_ptr = boost::shared_ptr<VDS_VOLUME_PROP2>(new VDS_VOLUME_PROP2());
            ZeroMemory(_vol_prop2_ptr.get(), sizeof(VDS_VOLUME_PROP2));
            hr = _vds_volume2->GetProperties2(_vol_prop2_ptr.get());
        }
        else{
            if (!_vol_prop_ptr)
                _vol_prop_ptr = boost::shared_ptr<VDS_VOLUME_PROP>(new VDS_VOLUME_PROP());
            ZeroMemory(_vol_prop_ptr.get(), sizeof(VDS_VOLUME_PROP));
            hr = _vds_volume->GetProperties(_vol_prop_ptr.get());
        }

        if (!_vds_volume_mf){
            _vds_volume_mf = _vds_volume;
            if (_vds_volume_mf)
                _vds.set_auth(_vds_volume_mf);
        }

        if (_vds_volume_mf){
            _vds_volume_mf->GetFileSystemProperties(&_vol_fs_prop);
        }
        _volume_guid_path = get_volume_guid_path();
        win32_volume::vtr win32_volumes = _vds.get_win32_volumes();
        if (_volume_guid_path.length()){
            for (win32_volume::vtr::iterator ppWin32Vol = win32_volumes.begin(); ppWin32Vol != win32_volumes.end(); ppWin32Vol++){
                if (0 == _wcsnicmp((*ppWin32Vol)->device_id().c_str(), _volume_guid_path.c_str(), (*ppWin32Vol)->device_id().length())){
                    _drive_type = (*ppWin32Vol)->drive_type();
                    break;
                }
            }
        }
        else{
            string_array_w _access_paths = access_paths();
            if (_access_paths.size()){
                bool isFound = false;
                for (win32_volume::vtr::iterator ppWin32Vol = win32_volumes.begin(); ppWin32Vol != win32_volumes.end() && !isFound; ppWin32Vol++){ 
                    if (_access_paths[0].length() == 3){          
                        if ((*ppWin32Vol)->drive_letter().length() && 
                            (0 == _wcsnicmp(_access_paths[0].c_str(), (*ppWin32Vol)->drive_letter().c_str(),2))){
                            _volume_guid_path = (*ppWin32Vol)->device_id();
                            _drive_type = (*ppWin32Vol)->drive_type();
                            isFound = true;
                            break;
                        }
                    }
                    else{
                        win32_directory::vtr mount_points = (*ppWin32Vol)->get_mount_points();
                        for (win32_directory::vtr::iterator m = mount_points.begin(); m != mount_points.end(); m++){
                            std::wstring name = (*m)->name();
                            if (0 == _wcsicmp(_access_paths[0].c_str(), (*m)->name().c_str())){
                                _volume_guid_path = (*ppWin32Vol)->device_id();
                                _drive_type = (*ppWin32Vol)->drive_type();
                                isFound = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    void cleanup(){
        if (_vol_prop_ptr)
            CoTaskMemFree(_vol_prop_ptr->pwszName);
        if (_vol_prop2_ptr){
            CoTaskMemFree(_vol_prop2_ptr->pwszName);
            CoTaskMemFree(_vol_prop2_ptr->pUniqueId);
        }
        CoTaskMemFree(_vol_fs_prop.pwszLabel);
    }
    ATL::CComPtr<IVdsVolume>                               _vds_volume;
    ATL::CComQIPtr<IVdsVolume2>                            _vds_volume2;
    ATL::CComQIPtr<IVdsVolumeMF>                           _vds_volume_mf;
    boost::shared_ptr<VDS_VOLUME_PROP>                     _vol_prop_ptr;
    boost::shared_ptr<VDS_VOLUME_PROP2>                    _vol_prop2_ptr;
    VDS_FILE_SYSTEM_PROP                                   _vol_fs_prop;
    vds_storage                                            _vds;
    std::wstring                                           _volume_guid_path;
    storage::ST_DRIVE_TYPE                                 _drive_type;
};

class vds_partition : virtual public storage::partition{
public:
    vds_partition(int disk_number, VDS_PARTITION_PROP& prop, storage::volume::vtr& volumes) :_disk_number(disk_number), _volumes(volumes){
        CopyMemory(&_prop, &prop, sizeof(VDS_PARTITION_PROP));
    }
    virtual ~vds_partition(){ }

    virtual uint32_t       disk_number() { return _disk_number; }
    virtual uint32_t       partition_number() { return _prop.ulPartitionNumber; }
    virtual std::wstring   drive_letter() { return _volumes.size() ? _volumes[0]->drive_letter() : L""; }
    virtual string_array_w access_paths() { 
        string_array_w _access_paths;
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            string_array_w _paths = (*v)->access_paths();
            _access_paths.insert(_access_paths.end(), _paths.begin(), _paths.end());
        }
        return _access_paths;
    }

    virtual uint64_t     offset() { return _prop.ullOffset; }
    virtual uint64_t     size() { return _prop.ullSize; }
    virtual uint16_t     mbr_type() { return _prop.Mbr.partitionType; }
    virtual std::wstring gpt_type() { return _prop.Gpt.partitionType  == GUID_NULL ? L"" : boost::str(boost::wformat(L"{%s}") %macho::guid_(_prop.Gpt.partitionType).wstring()); }
    virtual std::wstring guid() { return _prop.Gpt.partitionId == GUID_NULL ? L"" : boost::str(boost::wformat(L"{%s}") %macho::guid_(_prop.Gpt.partitionId).wstring()); }

    virtual bool         is_read_only(){
        //return _prop.PartitionStyle == VDS_PST_GPT ? (GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY == (_prop.Gpt.attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY)) : false;
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->is_readonly())
                return true;
        }
        return false;
    }
    virtual bool         is_offline(){ 
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->is_offline())
                return true;
        }
        return false;
    }
    virtual bool         is_system(){ return VDS_PTF_SYSTEM & _prop.ulFlags; }
    virtual bool         is_boot(){ 
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->is_boot())
                return true;
        }
        return false;
    }

    virtual bool         is_active(){
        /*
        If TRUE, the partition is active and can be used to start the system.     
        This property is only valid when the disk's PartitionStyle property is MBR and will be NULL for all other partition styles.
        */
        if (_prop.PartitionStyle == VDS_PST_MBR)
            return (TRUE == _prop.Mbr.bootIndicator);
        return false;
    }
    
    virtual bool         is_hidden(){
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->is_hidden())
                return true;
        }
        return false;
    }
    
    virtual bool         is_shadow_copy(){       
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->is_shadow_copy())
                return true;
        }
        return false;
    }
    
    virtual bool         no_default_drive_letter(){
        /*
        If TRUE, the operating system does not assign a drive letter automatically when the partition is discovered. 
        */
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (pv->no_default_drive_letter())
                return true;
        }
        return false;
    }

    virtual uint32_t set_attributes(bool _is_read_only, bool _no_default_drive_letter, bool _is_active, bool _is_hidden){
        HRESULT hr = S_OK;
        if (_is_active != is_active()){
            LOG(LOG_LEVEL_ERROR, L"set_attributes for vds_partition is not implemented.");
            return 1; // Not Supported (1)
        }
        for (storage::volume::vtr::iterator v = _volumes.begin(); v != _volumes.end(); v++){
            vds_volume* pv = dynamic_cast<vds_volume*>((*v).get());
            if (!SUCCEEDED(hr = pv->set_attributes(_is_read_only, _no_default_drive_letter, _is_hidden))){
                LOG(LOG_LEVEL_ERROR, L"Failed to set_attributes for volume(%s). error:(0x%08X)", pv->path().c_str(), hr);
                break;
            }
        }
        return hr;
    }

private:
    int                     _disk_number;
    VDS_PARTITION_PROP      _prop;
    storage::volume::vtr    _volumes;
};

class vds_disk : virtual public storage::disk{
public:
    vds_disk(vds_storage& vds, ATL::CComPtr<IVdsDisk>& vdsdisk) : _vds(vds), _vds_disk(vdsdisk), _number(-1){
        ZeroMemory(&_luninfo, sizeof(VDS_LUN_INFORMATION));
        refresh();
    }
    virtual ~vds_disk(){ cleanup(); }
    // Only for vds_disk
    virtual uint32_t     device_type() { return _vds_disk3 ? _disk_prop2_ptr->dwDeviceType : _disk_prop_ptr->dwDeviceType; }
    
    //General functions
    virtual std::wstring unique_id() {  
        std::wstring _id;
        if (_luninfo.m_deviceIdDescriptor.m_cIdentifiers > 0){
            if (_luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_CodeSet == VDSStorageIdCodeSetAscii){
                std::auto_ptr<char> ascii = std::auto_ptr<char>( new char[_luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_cbIdentifier + 1]);
                memcpy_s(ascii.get(), _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_cbIdentifier + 1, _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_rgbIdentifier, _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_cbIdentifier);
                ascii.get()[_luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_cbIdentifier] = 0;
                _id = macho::stringutils::convert_ansi_to_unicode(std::string(ascii.get()));
            }
            else if(_luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_CodeSet == VDSStorageIdCodeSetBinary){
                for (int i = 0; i < _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_cbIdentifier; i++){
                    _id.append(boost::str(boost::wformat(L"%x") % _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_rgbIdentifier[i]));
                }
            }
        }
        return _id;
    }
    virtual uint16_t     unique_id_format() { 
        if (_luninfo.m_deviceIdDescriptor.m_cIdentifiers < 1){
            return 0;
        }
        return _luninfo.m_deviceIdDescriptor.m_rgIdentifiers[0].m_Type;
    }
    virtual std::wstring path() { return  _vds_disk3 ? CheckWstr(_disk_prop2_ptr->pwszDevicePath) : CheckWstr(_disk_prop_ptr->pwszDevicePath); }
    virtual std::wstring location() { return  _vds_disk3 ? CheckWstr(_disk_prop2_ptr->pwszLocationPath) : L""; }
    virtual std::wstring friendly_name() { return _vds_disk3 ? CheckWstr(_disk_prop2_ptr->pwszFriendlyName) : CheckWstr(_disk_prop_ptr->pwszFriendlyName); }
    virtual uint32_t     number() { return _number; }
    virtual std::wstring serial_number() { return _luninfo.m_szSerialNumber ? stringutils::convert_ansi_to_unicode(_luninfo.m_szSerialNumber) : L""; }
    virtual std::wstring firmware_version() { return _luninfo.m_szProductRevision ? stringutils::convert_ansi_to_unicode(_luninfo.m_szProductRevision) : L""; }
    virtual std::wstring manufacturer() { return _luninfo.m_szVendorId ? stringutils::convert_ansi_to_unicode(_luninfo.m_szVendorId) : L""; }
    virtual std::wstring model() { return _luninfo.m_szProductId ? stringutils::convert_ansi_to_unicode(_luninfo.m_szProductId) : L""; }
    virtual uint64_t     size() { return  _vds_disk3 ? _disk_prop2_ptr->ullSize : _disk_prop_ptr->ullSize; }
    virtual uint64_t     allocated_size() { 
        VDS_DISK_FREE_EXTENT *pFreeExtentArray = NULL;
        LONG lNumberOfFreeExtents = 0;
        if (_vds_disk3){
            uint64_t _allocated_size = size();
            HRESULT hr = _vds_disk3->QueryFreeExtents(0, &pFreeExtentArray, &lNumberOfFreeExtents);
            if (SUCCEEDED(hr)){
                VDS_DISK_FREE_EXTENT * tmp = pFreeExtentArray;
                for (int i = 0; i < lNumberOfFreeExtents; i++){
                    _allocated_size -= tmp->ullSize;
                }
                CoTaskMemFree(pFreeExtentArray);
                return _allocated_size;
            }
        }
        return size();
    }
    virtual uint32_t     logical_sector_size()  { return _vds_disk3 ? _disk_prop2_ptr->ulBytesPerSector : _disk_prop_ptr->ulBytesPerSector; }
    // need to find out how to get the real physical_sector_size by vds
    virtual uint32_t     physical_sector_size() { return _vds_disk3 ? _disk_prop2_ptr->ulBytesPerSector : _disk_prop_ptr->ulBytesPerSector; }
    virtual uint64_t     largest_free_extent()  { 
        uint64_t _largest_free_extent = 0;
        VDS_DISK_FREE_EXTENT *pFreeExtentArray = NULL;
        LONG lNumberOfFreeExtents = 0;
        if (_vds_disk3){
            HRESULT hr = _vds_disk3->QueryFreeExtents(0, &pFreeExtentArray, &lNumberOfFreeExtents);
            if (SUCCEEDED(hr)){
                VDS_DISK_FREE_EXTENT * tmp = pFreeExtentArray;
                for (int i = 0; i < lNumberOfFreeExtents; i++){
                    if (_largest_free_extent < tmp->ullSize)
                        _largest_free_extent = tmp->ullSize;
                }
                CoTaskMemFree(pFreeExtentArray);
            }
        }
        return _largest_free_extent;
    }
    virtual uint32_t     number_of_partitions() { 
        LONG lNumberOfPartitions = 0;
        ATL::CComQIPtr<IVdsAdvancedDisk> advanceDisk = _vds_disk;
        if (advanceDisk){
            _vds.set_auth(advanceDisk);
            VDS_PARTITION_PROP *pPartitionPropArray = NULL;         
            HRESULT hr = advanceDisk->QueryPartitions(&pPartitionPropArray, &lNumberOfPartitions);
            if (SUCCEEDED(hr)){
                CoTaskMemFree(pPartitionPropArray);
            }
        }
        return lNumberOfPartitions;
    }
    virtual uint16_t            health_status() { return _vds_disk3 ? _disk_prop2_ptr->health : _disk_prop_ptr->health; }
    virtual uint16_t            bus_type() { return _vds_disk3 ? _disk_prop2_ptr->BusType : _disk_prop_ptr->BusType; }
    virtual storage::ST_PARTITION_STYLE     partition_style() { return (storage::ST_PARTITION_STYLE)(uint16_t)(_vds_disk3 ? _disk_prop2_ptr->PartitionStyle : _disk_prop_ptr->PartitionStyle); }
    virtual uint32_t            signature() { return _vds_disk3 ? _disk_prop2_ptr->dwSignature : _disk_prop_ptr->dwSignature; }
    virtual std::wstring guid() { 
        GUID guid = _vds_disk3 ? _disk_prop2_ptr->DiskGuid : _disk_prop_ptr->DiskGuid;
        if ( guid == GUID_NULL )
            return L"";
        return  boost::str(boost::wformat(L"{%s}") % macho::guid_(guid).wstring());
    }
    virtual bool         is_offline(){ return VDS_DS_OFFLINE == ((_vds_disk3 ? _disk_prop2_ptr->status : _disk_prop_ptr->status) & VDS_DS_OFFLINE); }
    virtual uint16_t     offline_reason() { return _vds_disk3 ? _disk_prop2_ptr->OfflineReason : VDSDiskOfflineReasonNone; }
    virtual bool         is_read_only(){ 
        ULONG ulFlags = _vds_disk3 ? _disk_prop2_ptr->ulFlags : _disk_prop_ptr->ulFlags;
        return (ulFlags & VDS_DF_READ_ONLY) || (ulFlags & VDS_DF_CURRENT_READ_ONLY);
    }
#ifdef _WIN2K3
    virtual bool         is_system(){ return _vds._boot_disk == _number; }
    virtual bool         is_boot(){ return _vds._boot_disk == _number; }
    virtual bool         boot_from_disk(){ return _vds._boot_disk == _number; }
#else
    virtual bool         is_system(){ return ((_vds_disk3 ? _disk_prop2_ptr->ulFlags : _disk_prop_ptr->ulFlags) & VDS_DF_SYSTEM_DISK) == VDS_DF_SYSTEM_DISK; }
    virtual bool         is_boot(){ return ((_vds_disk3 ? _disk_prop2_ptr->ulFlags : _disk_prop_ptr->ulFlags) & VDS_DF_BOOT_DISK) == VDS_DF_BOOT_DISK; }
    virtual bool         boot_from_disk(){ return ((_vds_disk3 ? _disk_prop2_ptr->ulFlags : _disk_prop_ptr->ulFlags) & VDS_DF_BOOT_FROM_DISK) == VDS_DF_BOOT_FROM_DISK; }
#endif
    virtual bool         is_clustered(){ return ((_vds_disk3 ? _disk_prop2_ptr->ulFlags : _disk_prop_ptr->ulFlags) & VDS_DF_CLUSTERED) == VDS_DF_CLUSTERED; }
    virtual uint32_t     sectors_per_track() { return _vds_disk3 ? _disk_prop2_ptr->ulSectorsPerTrack : _disk_prop_ptr->ulSectorsPerTrack; }
    virtual uint32_t     tracks_per_cylinder() { return _vds_disk3 ? _disk_prop2_ptr->ulTracksPerCylinder : _disk_prop_ptr->ulTracksPerCylinder; }
    virtual uint64_t     total_cylinders() {
        VDS_DISK_STATUS status;
        uint64_t ullSize;
        if (_vds_disk3){
            status = _disk_prop2_ptr->status;
            ullSize = _disk_prop2_ptr->ullSize;
        }
        else{
            status = _disk_prop_ptr->status;
            ullSize = _disk_prop_ptr->ullSize;
        }
        if (status == VDS_DS_ONLINE && ullSize > 0){
            uint64_t ullSize_Pre_Cylinder = sectors_per_track() * tracks_per_cylinder() * physical_sector_size();
            return ullSize / ullSize_Pre_Cylinder;
        }
        else{
            return 0;
        }
    }

    virtual uint32_t            scsi_bus() { return _win32_disk ? _win32_disk->scsi_bus() : -1; }
    virtual uint16_t            scsi_logical_unit() { return _win32_disk ? _win32_disk->scsi_logical_unit() : -1; }
    virtual uint16_t            scsi_port() { return _win32_disk ? _win32_disk->scsi_port() : -1; }
    virtual uint16_t            scsi_target_id() { return _win32_disk ? _win32_disk->scsi_target_id() : -1; }

    virtual storage::partition::vtr get_partitions(){
        storage::partition::vtr partitions;
        ATL::CComQIPtr<IVdsAdvancedDisk> advanceDisk = _vds_disk;
        if (advanceDisk){       
            _vds.set_auth(advanceDisk);
            VDS_PARTITION_PROP *pPartitionPropArray = NULL;
            LONG lNumberOfPartitions = 0;
            HRESULT hr = advanceDisk->QueryPartitions(&pPartitionPropArray, &lNumberOfPartitions);
            if (SUCCEEDED(hr)){
                storage::volume::vtr allVolumes = _vds.get_volumes();
                vds_extent::vtr      extents = get_extents();
                std::map<std::wstring, storage::volume::ptr> volumes_map;
                for (storage::volume::vtr::iterator ppVol = allVolumes.begin(); ppVol != allVolumes.end(); ppVol++){
                    for (vds_extent::vtr::iterator ppExt = extents.begin(); ppExt != extents.end(); ppExt++){
                        vds_volume *pVdsVol = dynamic_cast<vds_volume *>((*ppVol).get());
                        if (macho::guid_(pVdsVol->volume_id()) == (*ppExt)->_extent.volumeId)
                            volumes_map[pVdsVol->volume_id()] = (*ppVol);
                    }
                }
                VDS_PARTITION_PROP * tmp = pPartitionPropArray;
                for (int i = 0; i < lNumberOfPartitions; i++){
                    storage::volume::vtr _volumes;
                    for (vds_extent::vtr::iterator ppExt = extents.begin(); ppExt != extents.end(); ppExt++){
                        if ((*ppExt)->_extent.ullOffset >= tmp->ullOffset && 
                            (*ppExt)->_extent.ullOffset < (tmp->ullOffset + tmp->ullSize)){
                            std::wstring volume_id = macho::guid_((*ppExt)->_extent.volumeId).wstring();
                            if (volumes_map.count(volume_id))
                                _volumes.push_back(volumes_map[volume_id]);
                        }
                    }
                    partitions.push_back(storage::partition::ptr(new vds_partition(_number, *tmp, _volumes)));
                    ++tmp;
                }
                CoTaskMemFree(pPartitionPropArray);
            }
        }
        return partitions;
    }

    virtual storage::volume::vtr get_volumes(){
        storage::volume::vtr volumes;
        storage::volume::vtr allVolumes = _vds.get_volumes();
        vds_extent::vtr      extents = get_extents();
        for (storage::volume::vtr::iterator ppVol = allVolumes.begin(); ppVol != allVolumes.end(); ppVol++){
            for (vds_extent::vtr::iterator ppExt = extents.begin(); ppExt != extents.end(); ppExt++){
                vds_volume *pVdsVol = dynamic_cast<vds_volume *>((*ppVol).get());
                if (macho::guid_(pVdsVol->volume_id()) == (*ppExt)->_extent.volumeId)
                    volumes.push_back((*ppVol));
            }
        }
        return volumes;
    }

    virtual bool online(){
        bool result = false;
        if (!is_offline())
            result = true;
        else{
            ATL::CComQIPtr<IVdsDiskOnline> vdsDiskOnline = _vds_disk;
            if (vdsDiskOnline){
                _vds.set_auth(vdsDiskOnline);
                result = SUCCEEDED(vdsDiskOnline->Online());
                refresh();
            }
        }     
        return result;
    }
    virtual bool offline(){
        bool result = false;
        if (is_offline())
            result = true;
        else{
            ATL::CComQIPtr<IVdsDiskOnline> vdsDiskOnline = _vds_disk;
            if (vdsDiskOnline){
                _vds.set_auth(vdsDiskOnline);
                result = SUCCEEDED(vdsDiskOnline->Offline());
                refresh();
            }
        }
        return result;
    }
    virtual bool clear_read_only_flag(){
        bool result = false;
        if (!is_read_only())
            result = true;
        else{
            result = SUCCEEDED(_vds_disk->ClearFlags(VDS_DF_READ_ONLY));
            refresh();
        }
        return result;
    }
    virtual bool set_read_only_flag(){
        bool result = false;
        if (is_read_only())
            result = true;
        else{
            result = SUCCEEDED(_vds_disk->SetFlags(VDS_DF_READ_ONLY));
            refresh();
        }
        return result;
    }

    virtual bool initialize(macho::windows::storage::ST_PARTITION_STYLE partition_style) { 
        LOG(LOG_LEVEL_ERROR, L"This function is not implemented.");
        return false; 
    }

    virtual macho::windows::storage::partition::ptr create_partition(uint64_t size,
        bool use_maximum_size,
        uint64_t offset,
        uint32_t alignment,
        std::wstring drive_letter,
        bool assign_drive_letter,
        macho::windows::storage::ST_MBR_PARTITION_TYPE mbr_type,
        std::wstring  gpt_type,
        bool is_hidden,
        bool is_active
        ) {
        LOG(LOG_LEVEL_ERROR, L"This function is not implemented.");
        return false;
    };
    friend class vds_storage;
protected:
    
    virtual vds_extent::vtr get_extents(){
        vds_extent::vtr extents;
        VDS_DISK_EXTENT *pExtentArray = NULL;
        LONG            lNumberOfExtents = 0;
        HRESULT hr = _vds_disk->QueryExtents(&pExtentArray, &lNumberOfExtents);
        if (SUCCEEDED(hr)){
            VDS_DISK_EXTENT * tmp = pExtentArray;
            for (int i = 0; i < lNumberOfExtents; i++){
                extents.push_back(vds_extent::ptr(new vds_extent(_number, *tmp)));
                ++tmp;
            }
            CoTaskMemFree(pExtentArray);
        }
        return extents;
    }

    win32_disk_drive::ptr               _win32_disk;
private:
    void                    refresh(){
        HRESULT hr = S_OK;
        cleanup();
        if (NULL == _vds_disk3){
            _vds_disk3 = _vds_disk;
            if (_vds_disk3 != NULL) 
                _vds.set_auth(_vds_disk3);
        }
        if (_vds_disk3 != NULL){
            if (!_disk_prop2_ptr)
                _disk_prop2_ptr = boost::shared_ptr<VDS_DISK_PROP2>(new VDS_DISK_PROP2());
            ZeroMemory(_disk_prop2_ptr.get(), sizeof(VDS_DISK_PROP2));
            hr = _vds_disk3->GetProperties2(_disk_prop2_ptr.get());
            if (NULL != wcsstr(CheckWstr(_disk_prop2_ptr->pwszName), L"\\\\?\\PhysicalDrive"))
                _number = _wtol(_disk_prop2_ptr->pwszName + 17);
        }
        else{
            if (!_disk_prop_ptr)
                _disk_prop_ptr = boost::shared_ptr<VDS_DISK_PROP>(new VDS_DISK_PROP());
            ZeroMemory(_disk_prop_ptr.get(), sizeof(VDS_DISK_PROP));
            hr = _vds_disk->GetProperties(_disk_prop_ptr.get());
            if (NULL != wcsstr(CheckWstr(_disk_prop_ptr->pwszName), L"\\\\?\\PhysicalDrive"))
                _number = _wtol(_disk_prop_ptr->pwszName + 17);
        }
        hr = _vds_disk->GetIdentificationData(&_luninfo);
    }
    void                    cleanup(){
        //VDS_DISK_PROP
        if (_disk_prop_ptr){
            CoTaskMemFree(_disk_prop_ptr->pwszDiskAddress);
            CoTaskMemFree(_disk_prop_ptr->pwszName);
            CoTaskMemFree(_disk_prop_ptr->pwszFriendlyName);
            CoTaskMemFree(_disk_prop_ptr->pwszAdaptorName);
            CoTaskMemFree(_disk_prop_ptr->pwszDevicePath);
        }
        //VDS_DISK_PROP2
        if (_disk_prop2_ptr){
            CoTaskMemFree(_disk_prop2_ptr->pwszDiskAddress);
            CoTaskMemFree(_disk_prop2_ptr->pwszName);
            CoTaskMemFree(_disk_prop2_ptr->pwszFriendlyName);
            CoTaskMemFree(_disk_prop2_ptr->pwszAdaptorName);
            CoTaskMemFree(_disk_prop2_ptr->pwszDevicePath);
            CoTaskMemFree(_disk_prop2_ptr->pwszLocationPath);
        }
        //VDS_LUN_INFORMATION
        CoTaskMemFree(_luninfo.m_szProductId);
        CoTaskMemFree(_luninfo.m_szProductRevision);
        CoTaskMemFree(_luninfo.m_szSerialNumber);
        CoTaskMemFree(_luninfo.m_szVendorId);
        for (ULONG i = 0; i < _luninfo.m_cInterconnects; i++) {
            CoTaskMemFree(_luninfo.m_rgInterconnects[i].m_pbAddress);
            CoTaskMemFree(_luninfo.m_rgInterconnects[i].m_pbPort);
        }
        CoTaskMemFree(_luninfo.m_rgInterconnects);
    }
    ATL::CComPtr<IVdsDisk>              _vds_disk;
    ATL::CComQIPtr<IVdsDisk3>           _vds_disk3;
    int                                 _number;
    vds_storage                         _vds;
    boost::shared_ptr<VDS_DISK_PROP>    _disk_prop_ptr;
    boost::shared_ptr<VDS_DISK_PROP2>   _disk_prop2_ptr;
    VDS_LUN_INFORMATION                 _luninfo;
};

storage::disk::vtr vds_storage::get_disks() {
    win32_disk_drive::vtr win32_disks = _win32_storage->get_win32_disk_drives();
    std::map<int, win32_disk_drive::ptr> win32_disks_map;
    foreach(win32_disk_drive::ptr &wd, win32_disks)
        win32_disks_map[wd->index()] = wd;
    storage::disk::vtr unallocated_disks = get_unallocated_disks();
    storage::disk::vtr pack_disks = get_disks_from_enum_sw_provider();
    pack_disks.insert(pack_disks.end(), unallocated_disks.begin(), unallocated_disks.end());

    for (storage::disk::vtr::iterator d = pack_disks.begin(); d != pack_disks.end(); d++){
        vds_disk* vd = dynamic_cast<vds_disk*>((*d).get());
        vd->_win32_disk = win32_disks_map.count(vd->number()) ? win32_disks_map[vd->number()] : __noop;
    }
    return pack_disks;
}

storage::partition::vtr vds_storage::get_partitions(){
    storage::partition::vtr partitions;
    storage::disk::vtr _disks = get_disks_from_enum_sw_provider();
    for (storage::disk::vtr::iterator p = _disks.begin(); p != _disks.end(); p++){
        vds_disk* vDisk = dynamic_cast<vds_disk*>((*p).get());
        if (vDisk){
            storage::partition::vtr disk_partitions = vDisk->get_partitions();
            partitions.insert(partitions.end(), disk_partitions.begin(), disk_partitions.end());
        }
    }
    return partitions;
}

storage::volume::vtr vds_storage::get_volumes(){
    storage::volume::vtr volumes;
    ATL::CComPtr<IEnumVdsObject> pEnumProvider = NULL;
    HRESULT                      hr = S_OK;
    ULONG                        ulFetched = 0;
    hr = _vds->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS, &pEnumProvider);
    if (FAILED(hr) || (!pEnumProvider))
        BOOST_THROW_STORAGE_EXCEPTION(hr, L"QueryProviders failed.");
    set_auth(pEnumProvider);
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    enumerate_objects(pEnumProvider, tbUnknowns);
    _win32_storage->enable_cache_mode();
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsSwProvider> pSwProvider = (*ppUnknown);
        if (!pSwProvider)
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsSwProvider) E_UNEXPECTED.");
        set_auth(pSwProvider);
        std::vector<ATL::CComPtr<IVdsPack>> tbPacks;
        hr = query_packs(pSwProvider, tbPacks);
        for (std::vector<ATL::CComPtr<IVdsPack> >::iterator ppPack = tbPacks.begin(); ppPack != tbPacks.end(); ppPack++){
            storage::volume::vtr           pack_volumes = get_volumes_from_vds_pack((*ppPack));
            volumes.insert(volumes.end(), pack_volumes.begin(), pack_volumes.end());
        }
    }
    _win32_storage->disable_cache_mode();
    return volumes;
}

HRESULT vds_storage::enumerate_objects(ATL::CComPtr<IEnumVdsObject> &pEnumeration, std::vector<ATL::CComPtr<IUnknown> > &vpUnknown){
    HRESULT hr = S_OK;
    ULONG   nFetched = 0;

    while (true){
        ATL::CComPtr<IUnknown> pUnknown;
        nFetched = 0;
        hr = pEnumeration->Next(1, &pUnknown, &nFetched);
        if (FAILED(hr)){
            BOOST_THROW_STORAGE_EXCEPTION(hr, L"pEnumeration->Next failed.");
        }
        if (0 == nFetched)
            break;
        set_auth(pUnknown);
        vpUnknown.push_back(pUnknown);
    }

    hr = S_OK;
    return hr;
}

storage::disk::vtr vds_storage::get_unallocated_disks(){
    storage::disk::vtr                   disks;
    ATL::CComPtr<IEnumVdsObject>         enumDisk = NULL;
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    HRESULT                              hr = S_OK;
    hr = _vds->QueryUnallocatedDisks(&enumDisk);
    if (FAILED(hr) || !enumDisk)
        BOOST_THROW_STORAGE_EXCEPTION(hr, L"QueryUnallocatedDisks failed.");
    set_auth(enumDisk);  
    enumerate_objects(enumDisk, tbUnknowns);
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsDisk>              vdsDisk = (*ppUnknown);
        if ( !vdsDisk )
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsDisk) E_UNEXPECTED.");
        set_auth(vdsDisk);
        storage::disk::ptr d = storage::disk::ptr(new vds_disk(*this, vdsDisk));
        vds_disk* vd = dynamic_cast<vds_disk*>(d.get());
        if (FILE_DEVICE_DISK == (vd->device_type() & FILE_DEVICE_DISK))
            disks.push_back(d);
    }
    return disks;
}

storage::disk::vtr vds_storage::get_disks_from_enum_sw_provider(){
    storage::disk::vtr           disks;
    ATL::CComPtr<IEnumVdsObject> pEnumProvider = NULL;
    HRESULT                      hr = S_OK;
    ULONG                        ulFetched = 0;
    hr = _vds->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS, &pEnumProvider);
    if (FAILED(hr) ||(!pEnumProvider))
        BOOST_THROW_STORAGE_EXCEPTION(hr, L"QueryProviders failed.");
    set_auth(pEnumProvider);
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    enumerate_objects(pEnumProvider, tbUnknowns);
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsSwProvider> pSwProvider = (*ppUnknown);
        if (!pSwProvider)
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsSwProvider) E_UNEXPECTED.");
        set_auth(pSwProvider);
        std::vector<ATL::CComPtr<IVdsPack>> tbPacks;
        hr = query_packs(pSwProvider, tbPacks);
        for (std::vector<ATL::CComPtr<IVdsPack> >::iterator ppPack = tbPacks.begin(); ppPack != tbPacks.end(); ppPack++){
            storage::disk::vtr           pack_disks = get_disks_from_vds_pack((*ppPack));
            disks.insert(disks.end(), pack_disks.begin(), pack_disks.end());
        }
    }
    return disks;
}

HRESULT vds_storage::query_packs(ATL::CComPtr<IVdsSwProvider> &pSwProvider, std::vector<ATL::CComPtr<IVdsPack> > &vPacks){
    HRESULT hr = S_OK;
    CComPtr<IEnumVdsObject>     pEnumPack;
    hr = pSwProvider->QueryPacks(&pEnumPack);
    set_auth(pEnumPack);
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    enumerate_objects(pEnumPack, tbUnknowns);
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsPack> pVdsPack = (*ppUnknown);
        if (!pVdsPack)
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsPack) E_UNEXPECTED.");
        set_auth(pVdsPack);
        vPacks.push_back(pVdsPack);
    }
    return hr;
}

storage::disk::vtr vds_storage::get_disks_from_vds_pack(ATL::CComPtr<IVdsPack>& vds_pack){
    storage::disk::vtr                   disks;
    ATL::CComPtr<IEnumVdsObject>         enumDisks = NULL;
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    HRESULT                              hr = S_OK;
    hr = vds_pack->QueryDisks(&enumDisks);
    if (FAILED(hr) || !enumDisks)
        BOOST_THROW_STORAGE_EXCEPTION(hr, L"QueryDisks failed.");
    set_auth(enumDisks);
    enumerate_objects(enumDisks, tbUnknowns);
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsDisk>              vdsDisk = (*ppUnknown);
        if (!vdsDisk)
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsDisk) E_UNEXPECTED.");
        set_auth(vdsDisk);
        storage::disk::ptr d = storage::disk::ptr(new vds_disk(*this, vdsDisk));
        vds_disk* vd = dynamic_cast<vds_disk*>(d.get());
        if (FILE_DEVICE_DISK == (vd->device_type() & FILE_DEVICE_DISK))
            disks.push_back(d);
    }
    return disks;
}

storage::volume::vtr vds_storage::get_volumes_from_vds_pack(ATL::CComPtr<IVdsPack>& vds_pack){
    storage::volume::vtr                 volumes;
    ATL::CComPtr<IEnumVdsObject>         enumVolumes = NULL;
    std::vector<ATL::CComPtr<IUnknown> > tbUnknowns;
    HRESULT                              hr = S_OK;
    hr = vds_pack->QueryVolumes(&enumVolumes);
    if (FAILED(hr) || !enumVolumes)
        BOOST_THROW_STORAGE_EXCEPTION(hr, L"QueryVolumes failed.");
    set_auth(enumVolumes);
    enumerate_objects(enumVolumes, tbUnknowns);
    for (std::vector<ATL::CComPtr<IUnknown> >::iterator ppUnknown = tbUnknowns.begin(); ppUnknown != tbUnknowns.end(); ppUnknown++){
        ATL::CComQIPtr<IVdsVolume>              vdsVolume = (*ppUnknown);
        if (!vdsVolume)
            BOOST_THROW_STORAGE_EXCEPTION_STRING(L"QueryInterface(IVdsVolume) E_UNEXPECTED.");
        set_auth(vdsVolume);
        volumes.push_back(storage::volume::ptr(new vds_volume(*this, vdsVolume)));
    }
    return volumes;
}

class msft_storage : virtual public storage {
public:
    msft_storage(std::wstring host, std::wstring domain, std::wstring user, std::wstring password){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        macho::windows::wmi_services root;
        if (host.length())
            hr = root.connect(L"", host, domain, user, password);
        else
            hr = root.connect(L"");
        hr = root.open_namespace(L"Microsoft\\Windows\\Storage", _smapi);
        try{
            _vds_storage = vds_storage::ptr(new vds_storage(host, domain, user, password));
        }
        catch(...){
            _win32_storage = win32_storage::ptr(new win32_storage(root));
        }
    }
    virtual ~msft_storage(){}
    virtual void rescan(){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        hr = _smapi.exec_method(L"MSFT_StorageSetting", L"UpdateHostStorageCache", in, out, ret);
    }
    virtual storage::disk::vtr get_disks();
    virtual storage::volume::vtr get_volumes();
    virtual storage::partition::vtr get_partitions();
    virtual storage::disk::ptr get_disk(uint32_t disk_number);
    virtual storage::volume::vtr get_volumes(uint32_t disk_number);
    virtual storage::partition::vtr get_partitions(uint32_t disk_number);
    static std::wstring get_error_message(int code);
private:
    macho::windows::wmi_services _smapi;
    vds_storage::ptr             _vds_storage;
    win32_storage::ptr           _win32_storage;
};

class msft_partition : virtual public storage::partition{
public:
    msft_partition( wmi_object &obj) : _obj(obj){}
    virtual ~msft_partition(){}
    virtual std::wstring disk_id() { return _obj[L"DiskId"]; }
    virtual uint32_t     disk_number() { return _obj[L"DiskNumber"]; }
    virtual uint32_t     partition_number() { return _obj[L"PartitionNumber"]; }
    virtual std::wstring   drive_letter() { 
        wchar_t _letter[2] = { 0 };
        _letter[0] = (int)_obj[L"DriveLetter"];
        return _letter;
    }
    virtual string_array_w access_paths() { return _obj[L"AccessPaths"]; }
    virtual uint16_t     operational_status() { return _obj[L"OperationalStatus"]; }
    virtual uint16_t     transition_state() { return _obj[L"TransitionState"]; }
    virtual uint64_t     offset() { return _obj[L"Offset"]; }
    virtual uint64_t     size() { return _obj[L"Size"]; }
    virtual uint16_t     mbr_type() { return _obj[L"MbrType"]; }
    virtual std::wstring gpt_type() { return _obj[L"GptType"]; }
    virtual std::wstring guid() { return _obj[L"Guid"]; }
    virtual bool         is_read_only(){ return _obj[L"IsReadOnly"]; }
    virtual bool         is_offline(){ return _obj[L"IsOffline"]; }
    virtual bool         is_system(){ return _obj[L"IsSystem"]; }
    virtual bool         is_boot(){ return _obj[L"IsBoot"]; }
    virtual bool         is_active(){ return _obj[L"IsActive"]; }
    virtual bool         is_hidden(){ return _obj[L"IsHidden"]; }
    virtual bool         is_shadow_copy(){ return _obj[L"IsShadowCopy"]; }
    virtual bool         no_default_drive_letter(){ return _obj[L"NoDefaultDriveLetter"]; }
    virtual uint32_t     set_attributes(bool _is_read_only, bool _no_default_drive_letter, bool _is_active, bool _is_hidden){
        wmi_object in, out;
        DWORD ret = 0;
        HRESULT hr = _obj.get_input_parameters(L"SetAttributes", in);
        if (!SUCCEEDED(hr)){
            LOG(LOG_LEVEL_ERROR, L"_obj.get_input_parameters(SetAttributes) error : (0x%08X)", hr);
            ret = 1; //Not Supported (1)
        }
        else{
            in[L"IsReadOnly"] = _is_read_only;
            in[L"NoDefaultDriveLetter"] = _no_default_drive_letter;
            in[L"IsActive"] = _is_active;
            in[L"IsHidden"] = _is_hidden;
            hr = _obj.exec_method(L"SetAttributes", in, out, ret);
            if (!(SUCCEEDED(hr) && 0 == ret))
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        }
        return ret;
    }
private:
    wmi_object              _obj;
};

class msft_volume : virtual public storage::volume{
public:
    msft_volume(wmi_object &obj, vds_volume::ptr& volume) : _obj(obj), _vds_volume(volume){}
    msft_volume(wmi_object &obj, win32_volume::ptr& volume) : _obj(obj), _win32_volume(volume){}
    virtual ~msft_volume(){}
    virtual std::wstring id() {
        std::wstring _id = _obj[L"Path"];
        std::wstring::size_type first = _id.find_first_of(L"{");
        std::wstring::size_type last = _id.find_last_of(L"}");
        if (first != std::wstring::npos && last != std::wstring::npos)
            return _id.substr(first + 1, last - first - 1);
        else
            return L"";
    }
    virtual std::wstring   object_id() { return _obj[L"ObjectId"]; }
    virtual std::wstring   drive_letter() {
        wchar_t _letter[2] = {0};
        _letter[0] = (int)_obj[L"DriveLetter"];
        return _letter;
    }

    virtual storage::ST_VOLUME_TYPE type(){
        return _vds_volume ? _vds_volume->type() : storage::ST_VOLUME_TYPE::ST_VT_SIMPLE;
    }

    virtual std::wstring   path() { return _obj[L"Path"]; }
    virtual uint16_t       health_status() { return _obj[L"HealthStatus"]; }
    virtual std::wstring   file_system() { return _obj[L"FileSystem"];  }
    virtual std::wstring   file_system_label() { return _obj[L"FileSystemLabel"]; }
    virtual uint64_t       size() { return _obj[L"Size"]; }
    virtual uint64_t       size_remaining() { return _obj[L"SizeRemaining"]; }
    virtual storage::ST_DRIVE_TYPE       drive_type() { return (storage::ST_DRIVE_TYPE)(uint32_t)_obj[L"DriveType"]; }
    virtual string_array_w access_paths() { 
        string_array_w _access_paths;
        wmi_object_table objs = _obj.get_relateds(L"MSFT_Partition", L"MSFT_PartitionToVolume");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            string_array_w _paths = (*obj)[L"AccessPaths"];
            _access_paths.insert(_access_paths.end(), _paths.begin(), _paths.end());
        }
        return _access_paths;
    }

    virtual bool           mount(){
        return _vds_volume ? _vds_volume->mount() : _win32_volume ? _win32_volume->mount() : false;
    }

    virtual bool           dismount(bool force = false, bool permanent = false){
        return _vds_volume ? _vds_volume->dismount(force, permanent) : _win32_volume ? _win32_volume->dismount(force, permanent) : false;
    }

    virtual bool           format(
        std::wstring file_system,
        std::wstring file_system_label,
        uint32_t allocation_unit_size,
        bool full,
        bool force,
        bool compress,
        bool short_file_name_support,
        bool set_integrity_streams,
        bool use_large_frs,
        bool disable_heat_gathering){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        in = _obj.get_input_parameters(L"Format");
        in.set_parameter(L"FileSystem", file_system);
        in.set_parameter(L"FileSystemLabel", file_system_label);
        in.set_parameter(L"AllocationUnitSize", allocation_unit_size);
        in.set_parameter(L"Full", full);
        in.set_parameter(L"Force", force);
        if (!_wcsicmp(file_system.c_str(), L"ReFS")){       
            in.set_parameter(L"SetIntegrityStreams", set_integrity_streams);
        }
        else{
            in.set_parameter(L"Compress", compress);
            in.set_parameter(L"ShortFileNameSupport", short_file_name_support);
            in.set_parameter(L"UseLargeFRS", use_large_frs);
        }

        in.set_parameter(L"DisableHeatGathering", disable_heat_gathering);
        hr = _obj.exec_method(L"Format", in, out, ret);
        if (SUCCEEDED(hr) && 0 == ret){
            out.get_parameter(L"FormattedVolume", _obj);
            result = true;
        }
        else
            LOG(LOG_LEVEL_ERROR, L"Error (%s) - %s (0x%08X)", msft_storage::get_error_message(ret).c_str(), ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        return result;
    }

private:
    wmi_object              _obj;
    vds_volume::ptr         _vds_volume;
    win32_volume::ptr       _win32_volume;
};

class msft_disk : virtual public storage::disk{
public:
    msft_disk(msft_storage &storage, wmi_object &obj, vds_disk::ptr& _disk) : _storage(storage), _obj(obj), _vds_disk(_disk){}
    msft_disk(msft_storage &storage, wmi_object &obj, win32_disk_drive::ptr& win32_disk) : _storage(storage), _obj(obj), _win32_disk(win32_disk){}
    virtual ~msft_disk(){}
    // Only for msft_disk
    virtual std::wstring object_id() { return _obj[L"ObjectId"]; }
    virtual uint16_t     provisioning_type() { return _obj[L"ProvisioningType"]; }
    virtual uint16_t     operational_status() { return _obj[L"OperationalStatus"]; }

    //General functions
    virtual std::wstring unique_id() { return _obj[L"UniqueId"]; }
    virtual uint16_t     unique_id_format() { return _obj[L"UniqueIdFormat"]; }
    virtual std::wstring path() { return _obj[L"Path"]; }
    virtual std::wstring location() { return _obj[L"Location"]; }
    virtual std::wstring friendly_name() { return _obj[L"FriendlyName"]; }
    virtual uint32_t     number() { return _obj[L"Number"]; }
    virtual std::wstring serial_number() { return _obj[L"SerialNumber"]; }
    virtual std::wstring firmware_version() { return _obj[L"FirmwareVersion"]; }
    virtual std::wstring manufacturer() { return _obj[L"Manufacturer"]; }
    virtual std::wstring model() { return _obj[L"Model"]; }
    virtual uint64_t     size() { return _obj[L"Size"]; }
    virtual uint64_t     allocated_size() { return _obj[L"AllocatedSize"]; }
    virtual uint32_t     logical_sector_size() { return _obj[L"LogicalSectorSize"]; }
    virtual uint32_t     physical_sector_size() { return _obj[L"PhysicalSectorSize"]; }
    virtual uint64_t     largest_free_extent() { return _obj[L"LargestFreeExtent"]; }
    virtual uint32_t     number_of_partitions() { return _obj[L"NumberOfPartitions"]; }
    virtual uint16_t     health_status() { return (_obj[L"HealthStatus"]); }
    virtual uint16_t     bus_type() { return _obj[L"BusType"]; }
    virtual storage::ST_PARTITION_STYLE     partition_style() { return (storage::ST_PARTITION_STYLE)(uint16_t)_obj[L"PartitionStyle"]; }
    virtual uint32_t     signature() { return _obj[L"Signature"]; }
    virtual std::wstring guid() { return _obj[L"Guid"]; }
    virtual bool         is_offline(){ return _obj[L"IsOffline"]; }
    virtual uint16_t     offline_reason() { return _obj[L"OfflineReason"]; }
    virtual bool         is_read_only(){ return _obj[L"IsReadOnly"]; }
    virtual bool         is_system(){ return _obj[L"IsSystem"]; }
    virtual bool         is_clustered(){ return _obj[L"IsClustered"]; }
    virtual bool         is_boot(){ return _obj[L"IsBoot"]; }
    virtual bool         boot_from_disk(){ return _obj[L"BootFromDisk"]; }

    virtual uint32_t sectors_per_track() { return _vds_disk ? _vds_disk->sectors_per_track() : _win32_disk ? _win32_disk->sectors_per_track() : 0; ; }
    virtual uint32_t tracks_per_cylinder() { return _vds_disk ? _vds_disk->tracks_per_cylinder() : _win32_disk ? _win32_disk->tracks_per_cylinder() : 0; }
    virtual uint64_t total_cylinders() { return _vds_disk ? _vds_disk->total_cylinders() : 0; }

    virtual uint32_t            scsi_bus() { return _vds_disk ? _vds_disk->scsi_bus() : _win32_disk ? _win32_disk->scsi_bus() : -1; }
    virtual uint16_t            scsi_logical_unit() { return _vds_disk ? _vds_disk->scsi_logical_unit() : _win32_disk ? _win32_disk->scsi_logical_unit() : -1; }
    virtual uint16_t            scsi_port() { return _vds_disk ? _vds_disk->scsi_port() :  _win32_disk ? _win32_disk->scsi_port() : -1; }
    virtual uint16_t            scsi_target_id() { return _vds_disk ? _vds_disk->scsi_target_id() : _win32_disk ? _win32_disk->scsi_target_id() : -1; }

    virtual storage::volume::vtr get_volumes(){
        return _storage.get_volumes(number());
    }
    virtual storage::partition::vtr get_partitions(){
        return _storage.get_partitions(number());
    }

    virtual bool online(){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        if (!is_offline())
            result = true;
        else{
            wmi_object in, out;
            DWORD ret = 0;
            hr = _obj.exec_method(L"Online", in, out, ret);
            if (SUCCEEDED(hr) && 0 == ret){
                *this = *(dynamic_cast<msft_disk*>(_storage.get_disk(number()).get()));
                result = true;
            }
            else
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        }
        return result;
    }
    virtual bool offline(){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        if (is_offline())
            result = true;
        else{
            wmi_object in, out;
            DWORD ret = 0;
            hr = _obj.exec_method(L"Offline", in, out, ret);
            if (SUCCEEDED(hr) && 0 == ret){
                *this = *(dynamic_cast<msft_disk*>(_storage.get_disk(number()).get()));
                result = true;
            }
            else
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        }
        return result;
    }
    virtual bool clear_read_only_flag(){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        if (!is_read_only())
            result = true;
        else{
            wmi_object in, out;
            DWORD ret = 0;
            in = _obj.get_input_parameters(L"SetAttributes");
            in.set_parameter(L"IsReadOnly", false);
            hr = _obj.exec_method(L"SetAttributes", in, out, ret);
            if (SUCCEEDED(hr) && 0 == ret){
                *this = *(dynamic_cast<msft_disk*>(_storage.get_disk(number()).get()));
                result = true;
            }
            else
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        }
        return result;
    }
    virtual bool set_read_only_flag(){
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        if (is_read_only())
            result = true;
        else{
            wmi_object in, out;
            DWORD ret = 0;
            in = _obj.get_input_parameters(L"SetAttributes");
            in.set_parameter(L"IsReadOnly", true);
            hr = _obj.exec_method(L"SetAttributes", in, out, ret);
            if (SUCCEEDED(hr) && 0 == ret){
                *this = *(dynamic_cast<msft_disk*>(_storage.get_disk(number()).get()));
                result = true;
            }
            else
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        }
        return result;
    }

    virtual bool initialize(macho::windows::storage::ST_PARTITION_STYLE partition_style) {
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        in = _obj.get_input_parameters(L"Initialize");
        in.set_parameter(L"PartitionStyle", (uint16_t)partition_style);
        hr = _obj.exec_method(L"Initialize", in, out, ret);
        if (SUCCEEDED(hr) && 0 == ret){
            *this = *(dynamic_cast<msft_disk*>(_storage.get_disk(number()).get()));
            result = true;
        }
        else
            LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        return result;
    }

    virtual macho::windows::storage::partition::ptr create_partition(uint64_t size,
        bool use_maximum_size,
        uint64_t offset,
        uint32_t alignment,
        std::wstring drive_letter,
        bool assign_drive_letter,
        macho::windows::storage::ST_MBR_PARTITION_TYPE mbr_type,
        std::wstring  gpt_type,
        bool is_hidden,
        bool is_active
        ) {
        macho::windows::storage::partition::ptr new_partition;
        bool result = false;
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_object in, out;
        DWORD ret = 0;
        in = _obj.get_input_parameters(L"CreatePartition");
        if (!use_maximum_size)
            in.set_parameter(L"Size", size);
        else
            in.set_parameter(L"UseMaximumSize", true);
        if (offset)
            in.set_parameter(L"Offset", offset);
        if (alignment)
            in.set_parameter(L"Alignment", alignment);
        if (!assign_drive_letter && drive_letter.size())
            in.set_parameter(L"DriveLetter", drive_letter[0]);       
        else
            in.set_parameter(L"AssignDriveLetter", true);
        if (partition_style() == macho::windows::storage::ST_PST_MBR)
            in.set_parameter(L"MbrType", (uint16_t)mbr_type);
        else if (partition_style() == macho::windows::storage::ST_PST_GPT){
            if (!gpt_type.length()) gpt_type = L"{ebd0a0a2-b9e5-4433-87c0-68b6b72699c7}";
            in.set_parameter(L"GptType", gpt_type);
        }
        in.set_parameter(L"IsHidden", is_hidden);
        in.set_parameter(L"IsActive", is_active);

        hr = _obj.exec_method(L"CreatePartition", in, out, ret);
        if (SUCCEEDED(hr)){
            if (42002 == ret || 0 == ret){
                if (42002 == ret)
                    LOG(LOG_LEVEL_WARNING, L"Warning (%d) : The requested access path is already in use.", ret);                    
                wmi_object partition;
                hr = out.get_parameter(L"CreatedPartition", partition);
                if (SUCCEEDED(hr))
                    new_partition = macho::windows::storage::partition::ptr(new msft_partition(partition));
            }
            else
                LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str());
        }
        else
            LOG(LOG_LEVEL_ERROR, L"Error (%d) - %s (0x%08X)", ret, ((std::wstring)out[L"ExtendedStatus"]).c_str(), hr);
        return new_partition;
    };

private:
    wmi_object              _obj;
    vds_disk::ptr           _vds_disk;
    win32_disk_drive::ptr   _win32_disk;
    msft_storage            _storage;
};

storage::disk::vtr msft_storage::get_disks(){
    storage::disk::vtr disks;
    wmi_object_table objs = _smapi.query_wmi_objects(L"MSFT_Disk");
    if (_vds_storage){
        storage::disk::vtr _disks = _vds_storage->get_disks();
        foreach(storage::disk::ptr d, _disks){
            bool found = false;
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                if (found = (d->number() == (uint32_t)((*obj)[L"Number"]))){
                    disks.push_back(storage::disk::ptr(new msft_disk(*this, *obj, d)));
                    break;
                }
            }
            if (!found)
                disks.push_back(d);
        }
    }
    else{
        win32_disk_drive::vtr win32_disks = _win32_storage->get_win32_disk_drives();
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            win32_disk_drive::ptr win32_disk;
            for (win32_disk_drive::vtr::iterator wd = win32_disks.begin(); wd != win32_disks.end(); wd++){
                if ((*wd)->index() == (uint32_t)((*obj)[L"Number"])){
                    win32_disk = (*wd);
                    break;
                }
            }
            disks.push_back(storage::disk::ptr(new msft_disk(*this, *obj, win32_disk)));
        }
    }
    return disks;
}

storage::partition::vtr msft_storage::get_partitions(){
    storage::partition::vtr partitions;
    wmi_object_table objs = _smapi.query_wmi_objects(L"MSFT_Partition");
    for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
        partitions.push_back(storage::partition::ptr(new msft_partition(*obj)));
    }
    return partitions;
}

storage::volume::vtr   msft_storage::get_volumes(){
    storage::volume::vtr volumes;
    wmi_object_table objs = _smapi.query_wmi_objects(L"MSFT_Volume");
    if (_vds_storage){
        storage::volume::vtr _volumes = _vds_storage->get_volumes();
        foreach(storage::volume::ptr _v, _volumes){
            bool found = false;
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                if (found = (_v->path() == (std::wstring)(*obj)[L"Path"])){
                    volumes.push_back(storage::volume::ptr(new msft_volume(*obj, _v)));
                    break;
                }
            }
            if (!found)
                volumes.push_back(_v);
        }
    }
    else{
        wmi_object_table objs = _smapi.query_wmi_objects(L"MSFT_Volume");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            volumes.push_back(storage::volume::ptr(new msft_volume(*obj, _win32_storage->get_win32_volume((*obj)[L"Path"]))));
        }
    }
    return volumes;
}

storage::disk::ptr msft_storage::get_disk(uint32_t disk_number){
    storage::disk::ptr disk;
    std::wstring query = boost::str(boost::wformat(L"Select * From MSFT_Disk Where Number=%d") % disk_number);
    wmi_object_table objs = _smapi.exec_query(query);
    if (_vds_storage){
        vds_disk::ptr _vds_disk = _vds_storage->get_disk(disk_number);
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            disk = storage::disk::ptr(new msft_disk(*this, *obj, _vds_disk));
        }
        if (disk)
            return disk;
        else
            return _vds_disk;
    }
    else{
        win32_disk_drive::ptr win32_disk = _win32_storage->get_win32_disk_drive(disk_number);
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            disk = storage::disk::ptr(new msft_disk(*this, *obj, win32_disk));
        }
    }
    return disk;
}

storage::volume::vtr msft_storage::get_volumes(uint32_t disk_number){
    storage::volume::vtr volumes;
    std::wstring query = boost::str(boost::wformat(L"Select * From MSFT_Partition Where DiskNumber=%d") % disk_number);
    //std::wstring query = boost::str(boost::wformat(L"Select * From MSFT_Partition Where DiskId=\"%s\"") % stringutils::replace_one_slash_with_two(_obj[L"ObjectId"]));
    wmi_object_table objs = _smapi.exec_query(query);
    if (_vds_storage){
        storage::volume::vtr _volumes = _vds_storage->get_volumes(disk_number);
        foreach(storage::volume::ptr _v, _volumes){
            bool found = false;
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                wmi_object_table vols = (*obj).get_relateds(L"MSFT_Volume", L"MSFT_PartitionToVolume");
                for (wmi_object_table::iterator v = vols.begin(); v != vols.end(); v++){
                    if (found = (_v->path() == (std::wstring)(*v)[L"Path"])){
                        volumes.push_back(storage::volume::ptr(new msft_volume(*v, _v)));
                        break;
                    }
                }
                if (found)
                    break;
            }
            if (!found)
                volumes.push_back(_v);
        }
    }
    else{
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            wmi_object_table vols = (*obj).get_relateds(L"MSFT_Volume", L"MSFT_PartitionToVolume");
            for (wmi_object_table::iterator v = vols.begin(); v != vols.end(); v++){
                volumes.push_back(storage::volume::ptr(new msft_volume(*v, _win32_storage->get_win32_volume((*v)[L"Path"]))));
            }
        }
    }
    return volumes;
}

storage::partition::vtr msft_storage::get_partitions(uint32_t disk_number){
    storage::partition::vtr partitions;
    std::wstring query = boost::str(boost::wformat(L"Select * From MSFT_Partition Where DiskNumber=%d") % disk_number);
    //std::wstring query = boost::str(boost::wformat(L"Select * From MSFT_Partition Where DiskId=\"%s\"") % stringutils::replace_one_slash_with_two(_obj[L"ObjectId"]));
    wmi_object_table objs = _smapi.exec_query(query);
    for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
        partitions.push_back(storage::partition::ptr(new msft_partition(*obj)));
    }
    return partitions;
}

class local_disk;
class local_volume;
class local_storage : virtual public storage {
public:
    local_storage(storage::ptr stg) : _stg(stg), _boot_disk(-1), _system_partition(-1){
        get_system_device_number(_boot_disk, _system_partition);
        rescan();
    }
    virtual ~local_storage(){}
    virtual void rescan();
    virtual storage::disk::vtr get_disks();
    virtual storage::volume::vtr get_volumes();
    virtual storage::partition::vtr get_partitions();
    virtual storage::disk::ptr get_disk(uint32_t disk_number);
    virtual storage::volume::vtr get_volumes(uint32_t disk_number);
    virtual storage::partition::vtr get_partitions(uint32_t disk_number);
    static bool get_system_device_number(uint32_t& device_number, uint32_t& partition_number);

private:
    friend class local_disk;
    void enumerate_disks();
    void enumerate_volumes();
    void enumerate_partitions();
    //Disks
    static bool get_disk(HANDLE handle, local_disk& _disk);
    static bool get_registry_property(HDEVINFO DevInfo, DWORD Index, stdstring &deviceId);
    static bool get_device_property(HDEVINFO IntDevInfo, DWORD Index, stdstring &devicePath);
    static bool get_disk_number(HANDLE handle, uint32_t &number);
    static bool get_device_number(HANDLE handle, uint32_t& device_number, uint32_t& partition_number);
    static bool get_disk_geometry(HANDLE handle, DISK_GEOMETRY_EX &disk_geometry);
    static bool get_storage_query_property(HANDLE handle, DWORD &device_type, STORAGE_BUS_TYPE &bus_type, bool &is_remove_media, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &serial_number);
    static bool get_scsi_pass_through_data(HANDLE handle, DWORD &device_type, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &verdor_string);
    static bool get_drive_layout(HANDLE handle, boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex);
    static bool get_disk_length(HANDLE handle, ULONGLONG &length);
    static bool get_disk_scsi_address(HANDLE handle, UCHAR &port_number, UCHAR &path_id, UCHAR &target_id, UCHAR &lun);
    static void get_disk_id(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex, uint32_t& signature, stdstring& disk_uuid);
    static bool is_clustered(HANDLE handle);
    static bool is_writable(HANDLE handle);
    static bool is_dynamic_disk(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex);
    static bool is_bootable_disk(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex);
    static bool detect_sector_size(HANDLE handle, uint32_t &logical_sector_size, uint32_t& physical_sector_size);
    //Volumes
    static bool get_disk_extents(stdstring device_id, local_volume&  volume);
    static bool get_mount_points(stdstring device_id, string_table& mount_points);
    static bool get_volume(stdstring device_id, local_volume& volume);

    macho::windows::critical_section                _cs;
    uint32_t                                        _boot_disk;
    uint32_t                                        _system_partition;
    storage::disk::vtr                              _disks;
    storage::partition::vtr                         _partitions;
    storage::volume::vtr                            _volumes;
    storage::ptr                                    _stg;
    const static TCHAR*                             _bus_types[BusTypeMax + 1];
    const static TCHAR*                             _device_types[16];
};

class local_partition : virtual public storage::partition{
public:
    local_partition(uint32_t disk_number, const PARTITION_INFORMATION_EX &entry); 
    virtual ~local_partition(){}
    virtual uint32_t     disk_number() { return _disk_number; }
    virtual uint32_t     partition_number() { return _partition_number; }
    virtual std::wstring   drive_letter() { return _drive_letter; }
    virtual string_array_w access_paths() { return _access_paths; }
    virtual uint64_t     offset() { return _offset; }
    virtual uint64_t     size() { return _size; }
    virtual uint16_t     mbr_type() { return _mbr_type; }
    virtual std::wstring gpt_type() { return _gpt_type; }
    virtual std::wstring guid() { return _guid; }
    virtual bool         is_read_only(){ return _is_read_only; }
    virtual bool         is_offline(){ return _is_offline; }
    virtual bool         is_system(){ return _is_system; }
    virtual bool         is_boot(){ return _is_boot; }
    virtual bool         is_active(){ return _is_active; }
    virtual bool         is_hidden(){ return _is_hidden; }
    virtual bool         is_shadow_copy(){ return _is_shadow_copy; }
    virtual bool         no_default_drive_letter(){ return _no_default_drive_letter; }
    virtual uint32_t     set_attributes(bool _is_read_only, bool _no_default_drive_letter, bool _is_active, bool _is_hidden){
        return _partition ? _partition->set_attributes(_is_read_only, _no_default_drive_letter, _is_active, _is_hidden) : 1; //Not Supported (1)
    }
private:
    friend class local_storage;
    uint32_t                        _disk_number;
    uint32_t                        _partition_number;
    std::wstring                    _drive_letter;
    string_array_w                  _access_paths;
    uint64_t                        _offset;
    uint64_t                        _size;
    uint16_t                        _mbr_type;
    std::wstring                    _gpt_type;
    std::wstring                    _guid;
    bool                            _is_read_only;
    bool                            _is_offline;
    bool                            _is_system;
    bool                            _is_boot;
    bool                            _is_active;
    bool                            _is_hidden;
    bool                            _is_shadow_copy;
    bool                            _no_default_drive_letter;
    storage::partition::ptr         _partition;
};

class local_volume : virtual public storage::volume{
public:
    struct extent{
        typedef std::vector<extent> vtr;
        extent() : disk_number(-1), offset(0), length(0) {}
        extent(DWORD number, ULONGLONG starting_offset, ULONGLONG extent_length)
            : disk_number(number), offset(starting_offset), length(extent_length) {}
        extent(const extent& ext){
            copy(ext);
        }
        void copy(const extent& ext){
            disk_number = ext.disk_number;
            offset = ext.offset;
            length = ext.length;
        }
        const extent &operator =(const extent& ext){
            if (this != &ext)
                copy(ext);
            return(*this);
        }
        DWORD      disk_number;
        ULONGLONG  offset;
        ULONGLONG  length;
    };

    local_volume(storage::volume::ptr volume) : _volume(volume),
        _health_status(0), 
        _size(0ULL), 
        _size_remaining(0ULL), 
        _drive_type(storage::ST_DRIVE_TYPE::ST_UNKNOWN){}
    virtual ~local_volume(){}
    virtual std::wstring id() {
        std::wstring _id = _path;
        std::wstring::size_type first = _id.find_first_of(L"{");
        std::wstring::size_type last = _id.find_last_of(L"}");
        if (first != std::wstring::npos && last != std::wstring::npos)
            return _id.substr(first + 1, last - first - 1);
        else
            return L"";
    }
    virtual std::wstring   drive_letter() { return _drive_letter; }
    virtual storage::ST_VOLUME_TYPE type(){
        return _volume ? _volume->type() : storage::ST_VOLUME_TYPE::ST_VT_SIMPLE;
    }
    virtual std::wstring   path() { return _path; }
    virtual uint16_t       health_status() { return _volume ? _volume -> health_status() : _health_status; }
    virtual std::wstring   file_system() { return _file_system; }
    virtual std::wstring   file_system_label() { return _file_system_label; }
    virtual uint64_t       size() { return _size; }
    virtual uint64_t       size_remaining() { return _size_remaining; }
    virtual storage::ST_DRIVE_TYPE       drive_type() { return _drive_type; }
    virtual string_array_w access_paths() { return _access_paths;  }

    virtual bool           mount(){
        return _volume ? _volume->mount() : false;
    }

    virtual bool           dismount(bool force = false, bool permanent = false){
        return _volume ? _volume->dismount() : false;
    }

    virtual bool           format(
        std::wstring file_system,
        std::wstring file_system_label,
        uint32_t allocation_unit_size,
        bool full,
        bool force,
        bool compress,
        bool short_file_name_support,
        bool set_integrity_streams,
        bool use_large_frs,
        bool disable_heat_gathering){
        return _volume ? _volume->format(file_system, file_system_label, allocation_unit_size, full, force, compress, short_file_name_support, set_integrity_streams, use_large_frs, disable_heat_gathering) : false;
    }

private:
    friend class local_storage;
    std::wstring            _drive_letter;
    std::wstring            _path;
    uint16_t                _health_status;
    std::wstring            _file_system;
    std::wstring            _file_system_label;
    uint64_t                _size;
    uint64_t                _size_remaining;
    string_array_w          _access_paths;
    storage::ST_DRIVE_TYPE  _drive_type;
    storage::volume::ptr    _volume;
    extent::vtr             _extents;
};

class local_disk : virtual public storage::disk{
public:
    local_disk(storage::disk::ptr disk) :
        _disk(disk),
        _unique_id_format(0),
        _number(0),
        _size(0ULL),
        _allocated_size(0ULL),
        _logical_sector_size(0),
        _physical_sector_size(0),
        _largest_free_extent(0),
        _number_of_partitions(0),
        _health_status(0),
        _bus_type(0),
        _is_offline(false),
        _offline_reason(0),
        _is_read_only(false),
        _is_system(false),
        _is_clustered(false),
        _is_boot(false),
        _boot_from_disk(false),
        _sectors_per_track(0),
        _tracks_per_cylinder(0),
        _total_cylinders(0),
        _scsi_bus(0),
        _scsi_logical_unit(0),
        _scsi_port(0),
        _scsi_target_id(0)
    {}
    virtual ~local_disk(){}

    //General functions
    virtual std::wstring unique_id() { return _disk ? _disk->unique_id() : _unique_id; }
    virtual uint16_t     unique_id_format() { return _disk ? _disk->unique_id_format() : _unique_id_format; }
    virtual std::wstring path() { return _path; }
    virtual std::wstring location() { return _disk ? _disk->location() : _location; }
    virtual std::wstring friendly_name() { return _friendly_name; }
    virtual uint32_t     number() { return _number; }
    virtual std::wstring serial_number() { return _serial_number; }
    virtual std::wstring firmware_version() { return _firmware_version; }
    virtual std::wstring manufacturer() { return _manufacturer; }
    virtual std::wstring model() { return _model; }
    virtual uint64_t     size() { return _size; }
    virtual uint64_t     allocated_size() { return _allocated_size; }
    virtual uint32_t     logical_sector_size() { return _logical_sector_size; }
    virtual uint32_t     physical_sector_size() { return _physical_sector_size; }
    virtual uint64_t     largest_free_extent() { return _largest_free_extent; }
    virtual uint32_t     number_of_partitions() { return _number_of_partitions; }
    virtual uint16_t     health_status() { return _disk ? _disk->health_status() : _health_status; }
    virtual uint16_t     bus_type() { return _bus_type; }
    virtual storage::ST_PARTITION_STYLE     partition_style() { 
        if (_pdliex){
            if (_pdliex->PartitionStyle == PARTITION_STYLE_MBR){
                if ( _pdliex->Mbr.Signature == 0 && _pdliex->PartitionCount == 0)
                    return storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN;
                else
                    return storage::ST_PARTITION_STYLE::ST_PST_MBR;
            }
            else if (_pdliex->PartitionStyle == PARTITION_STYLE_GPT)
                return storage::ST_PARTITION_STYLE::ST_PST_GPT;
        }
        return storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN;
    }
    virtual bool         is_offline(){ return _disk ? _disk->is_offline() : _is_offline; }
    virtual uint16_t     offline_reason() { return _disk ? _disk->offline_reason() : _offline_reason; }
    virtual bool         is_read_only(){ return _is_read_only; }
    virtual bool         is_system(){ return _is_system; }
    virtual bool         is_clustered(){ return _is_clustered; }
    virtual bool         is_boot(){ return _is_boot; }
    virtual bool         boot_from_disk(){ return _boot_from_disk; }

    virtual uint32_t     sectors_per_track() { return _sectors_per_track; }
    virtual uint32_t     tracks_per_cylinder() { return _tracks_per_cylinder; }
    virtual uint64_t     total_cylinders() { return _total_cylinders; }

    virtual uint32_t     scsi_bus() { return _scsi_bus; }
    virtual uint16_t     scsi_logical_unit() { return _scsi_logical_unit; }
    virtual uint16_t     scsi_port() { return _scsi_port; }
    virtual uint16_t     scsi_target_id() { return _scsi_target_id; }

    virtual uint32_t     signature() { 
        if (_pdliex && _pdliex->PartitionStyle == PARTITION_STYLE_MBR)
            return _pdliex->Mbr.Signature;
        return 0; 
    }

    virtual std::wstring guid() { 
        if (_pdliex && _pdliex->PartitionStyle == PARTITION_STYLE_GPT)
            return macho::guid_(_pdliex->Gpt.DiskId).wstring();
        return L""; }

    virtual storage::volume::vtr get_volumes(){
        return _volumes;
    }
    virtual storage::partition::vtr get_partitions(){
        return _partitions;
    }
    virtual bool online(){
        return _disk ? _disk->online() : false;
    }
    virtual bool offline(){
        return _disk ? _disk->offline() : false;
    }
    virtual bool clear_read_only_flag(){       
        return _disk ? _disk->clear_read_only_flag() : false;
    }
    virtual bool set_read_only_flag(){        
        return _disk ? _disk->set_read_only_flag() : false;
    }

    virtual bool initialize(macho::windows::storage::ST_PARTITION_STYLE partition_style) {
        return _disk ? _disk->initialize(partition_style) : false;
    }

    virtual macho::windows::storage::partition::ptr create_partition(uint64_t size,
        bool use_maximum_size,
        uint64_t offset,
        uint32_t alignment,
        std::wstring drive_letter,
        bool assign_drive_letter,
        macho::windows::storage::ST_MBR_PARTITION_TYPE mbr_type,
        std::wstring  gpt_type,
        bool is_hidden,
        bool is_active
        ) {       
        return _disk ? _disk->create_partition(size, use_maximum_size, offset, alignment, drive_letter, assign_drive_letter, mbr_type, gpt_type, is_hidden, is_active) : NULL;
    };

private:
    friend class local_storage;
    //General functions
    std::wstring                    _unique_id;
    uint16_t                        _unique_id_format;
    std::wstring                    _path;
    std::wstring                    _location;
    std::wstring                    _friendly_name;
    uint32_t                        _number;
    std::wstring                    _serial_number;
    std::wstring                    _firmware_version;
    std::wstring                    _manufacturer;
    std::wstring                    _model;
    uint64_t                        _size;
    uint64_t                        _allocated_size;
    uint32_t                        _logical_sector_size;
    uint32_t                        _physical_sector_size;
    uint64_t                        _largest_free_extent;
    uint32_t                        _number_of_partitions;
    uint16_t                        _health_status;
    uint16_t                        _bus_type;
    bool                            _is_offline;
    uint16_t                        _offline_reason;
    bool                            _is_read_only;
    bool                            _is_system;
    bool                            _is_clustered;
    bool                            _is_boot;
    bool                            _boot_from_disk;
    uint32_t                        _sectors_per_track;
    uint32_t                        _tracks_per_cylinder;
    uint64_t                        _total_cylinders;
    uint32_t                        _scsi_bus;
    uint16_t                        _scsi_logical_unit;
    uint16_t                        _scsi_port;
    uint16_t                        _scsi_target_id;
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > _pdliex;
    storage::partition::vtr         _partitions;
    storage::volume::vtr            _volumes;
    storage::disk::ptr              _disk;
};

storage::ptr storage::get(std::wstring host, std::wstring user, std::wstring password){
    HRESULT hr = S_OK;
    std::wstring domain, account;
    if (host.length()){
        string_array arr = stringutils::tokenize2(user, L"\\", 2, false);
        if (arr.size()){
            if (arr.size() == 1)
                account = arr[0];
            else{
                domain = arr[0];
                account = arr[1];
            }
        }
    }
    int env_vds_storage = 0;
    std::wstring env_vds_storage_str = environment::get_environment_variable(L"VDS_STORAGE");
    if (env_vds_storage_str.length())
        env_vds_storage = std::stoi(env_vds_storage_str);
    if (env_vds_storage > 0)
        return storage::ptr(new vds_storage(host, domain, account, password));
    else{
        wmi::win32_operating_system os = wmi::win32_operating_system::get(host, user, password);
        if (os.is_win2012_or_greater())
            return storage::ptr(new msft_storage(host, domain, account, password));
        else
            return storage::ptr(new vds_storage(host, domain, account, password));
    }
}

bool storage::disk::is_dynmaic(){
    storage::partition::vtr parts = get_partitions();
    switch (partition_style()){
    case ST_PARTITION_STYLE::ST_PST_GPT:
        foreach(storage::partition::ptr &p, parts){
            if ((macho::guid_(p->gpt_type()) == macho::guid_("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")) || // PARTITION_LDM_METADATA_GUID
                (macho::guid_(p->gpt_type()) == macho::guid_("af9b60a0-1431-4f62-bc68-3311714a69ad"))) //PARTITION_LDM_DATA_GUID
                return true;
        }
        break;
    case ST_PARTITION_STYLE::ST_PST_MBR:
        foreach(storage::partition::ptr &p, parts){
            if (p->mbr_type() == MS_PARTITION_LDM)
                return true;
        }
        break;
    }
    return false;
}

storage::ptr storage::local(){
    storage::ptr stg;
    try{
        stg = get();
    }
    catch (macho::exception_base& ex){
        LOG(LOG_LEVEL_WARNING, L"%s", macho::get_diagnostic_information(ex).c_str());
    }
    catch (...){
    }
    return storage::ptr(new local_storage(stg));
}

std::wstring msft_storage::get_error_message(int code){
    std::wstring err = L"Unknown";
    switch (code){
    case 0 :
        err = L"Success";
        break;
    case 1:
        err = L"Not Supported";
        break;
    case 2:
        err = L"Unspecified Error";
        break;
    case 3:
        err = L"Timeout";
        break;
    case 4:
        err = L"Failed";
        break;
    case 5:
        err = L"Invalid Parameter";
        break;   
    case 6:
        err = L"Disk is in use";
        break;
    case 7:
        err = L"This command is not supported on x86 running in x64 environment.";
        break; 
    case 4097:
        err = L"Size Not Supported";
        break;
    case 40001:
        err = L"Access Denied";
        break;    
    case 40004:
        err = L"An unexpected I/O error has occured";
        break;
    case 40018:
        err = L"The specified object is managed by the Microsoft Failover Clustering component. The disk must be in cluster maintenance mode and the cluster resource status must be online to perform this operation.";
        break;
    case 42010:
        err = L"The operation is not allowed on a system or critical partition.";
        break;
    case 43000:
        err = L"The specified cluster size is invalid";
        break;
    case 43001:
        err = L"The specified file system is not supported";
        break;
    case 43002:
        err = L"The volume cannot be quick formatted";
        break;
    case 43003:
        err = L"The number of clusters exceeds 32 bits";
        break;
    case 43004:
        err = L"The specified UDF version is not supported";
        break;
    case 43005:
        err = L"The cluster size must be a multiple of the disk's physical sector size";
        break;
    case 43006:
        err = L"Cannot perform the requested operation when the drive is read only";
        break;
    }
    return boost::str(boost::wformat(L"%s(%d)") % err %code);
}

storage::disk::vtr local_storage::get_disks(){
    return _disks;
}

storage::volume::vtr local_storage::get_volumes(){
    macho::windows::auto_lock lock(_cs);
    return _volumes;
}

storage::partition::vtr local_storage::get_partitions(){
    macho::windows::auto_lock lock(_cs);
    return _partitions;
}

storage::disk::ptr local_storage::get_disk(uint32_t disk_number){
    macho::windows::auto_lock lock(_cs);
    foreach(storage::disk::ptr& d, _disks){
        if (d->number() == disk_number){
            if (_stg){
                try{
                    local_disk* _d = dynamic_cast<local_disk*>(d.get());
                    _d->_disk = _stg->get_disk(d->number());
                }
                catch (macho::exception_base& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                }
            }
            return d;
        }
    }
    return NULL;
}

storage::volume::vtr local_storage::get_volumes(uint32_t disk_number){
    storage::volume::vtr volumes;
    macho::windows::auto_lock lock(_cs);
    foreach(auto v, _volumes){
        local_volume* vol = dynamic_cast<local_volume*>(v.get());
        foreach(local_volume::extent& ext, vol->_extents){
            if (ext.disk_number == disk_number){
                volumes.push_back(v);
                break;
            }
        }
    }
    return volumes;
}

storage::partition::vtr local_storage::get_partitions(uint32_t disk_number){
    storage::partition::vtr partitions;
    macho::windows::auto_lock lock(_cs);
    foreach(auto p, _partitions){
        if (p->disk_number() == disk_number)
            partitions.push_back(p);
    }
    return partitions;
}

bool local_storage::get_system_device_number(uint32_t& device_number, uint32_t& partition_number){
    bool result = false;
    std::wstring win_dir = environment::get_windows_directory();
    //system_drive.append(L"\\");
    std::wstring path = boost::str(boost::wformat(L"\\\\?\\%1%:") % win_dir[0]);
    auto_file_handle h = CreateFile(path.c_str(),    // device interface name
        GENERIC_READ ,       // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // dwShareMode
        NULL,                               // lpSecurityAttributes
        OPEN_EXISTING,                      // dwCreationDistribution
        0,                                  // dwFlagsAndAttributes
        NULL                                // hTemplateFile
        );
    if (h.is_invalid()){
        LOG(LOG_LEVEL_ERROR, _T("CreateFile failed. error( 0x%08X )"), GetLastError());
    }
    else{
        result = get_device_number(h, device_number, partition_number);
    }
    return result;
}

void local_storage::get_disk_id(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex, uint32_t& signature, stdstring& disk_uuid){
    TCHAR         szGUID[40];
    switch (pdliex->PartitionStyle){
    case PARTITION_STYLE_MBR:
        signature = pdliex->Mbr.Signature;
        break;
    case PARTITION_STYLE_GPT:
        StringFromGUID2(pdliex->Gpt.DiskId, szGUID, 40);
        disk_uuid = szGUID;
        break;
    }
}

bool local_storage::is_dynamic_disk(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex){
    switch (pdliex.get()->PartitionStyle){
    case PARTITION_STYLE_GPT:
        for (DWORD i = 0; i < pdliex.get()->PartitionCount; ++i){
            if ((pdliex.get()->PartitionEntry[i].Gpt.PartitionType == macho::guid_("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")) || // PARTITION_LDM_METADATA_GUID
                (pdliex.get()->PartitionEntry[i].Gpt.PartitionType == macho::guid_("af9b60a0-1431-4f62-bc68-3311714a69ad"))) //PARTITION_LDM_DATA_GUID
                return true;
        }
        break;
    case PARTITION_STYLE_MBR:
        for (DWORD i = 0; i < pdliex.get()->PartitionCount; ++i){
            if (pdliex.get()->PartitionEntry[i].Mbr.PartitionType == MS_PARTITION_LDM)
                return true;
        }
        break;
    }
    return false;
}

bool local_storage::is_bootable_disk(boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex){
    switch (pdliex.get()->PartitionStyle){
    case PARTITION_STYLE_GPT:
        for (DWORD i = 0; i < pdliex.get()->PartitionCount; ++i){
            if (pdliex.get()->PartitionEntry[i].Gpt.PartitionType == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")) //PARTITION_SYSTEM_GUID
                return true;
        }
        break;
    case PARTITION_STYLE_MBR:
        for (DWORD i = 0; i < pdliex.get()->PartitionCount; ++i){
            if (pdliex.get()->PartitionEntry[i].Mbr.RecognizedPartition &&
                pdliex.get()->PartitionEntry[i].Mbr.BootIndicator)
                return true;
        }
        break;
    }
    return false;
}

bool local_storage::get_registry_property(HDEVINFO DevInfo, DWORD Index, stdstring &deviceId)
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

    if (status == FALSE) {
        errorCode = GetLastError();
        if (errorCode == ERROR_NO_MORE_ITEMS) {
            LOG(LOG_LEVEL_TRACE, _T("No more devices.\n"));
        }
        else {
            LOG(LOG_LEVEL_ERROR, _T("SetupDiEnumDeviceInfo failed with error: %d\n"), errorCode);
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

    if (status == FALSE) {
        errorCode = GetLastError();
        if (errorCode != ERROR_INSUFFICIENT_BUFFER) {
            if (errorCode == ERROR_INVALID_DATA) {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                return true;
            }
            else {
                LOG(LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode);
                return false;
            }
        }
    }

    //
    // We need to change the buffer size.
    //
    pbuffer = std::auto_ptr<TCHAR>(new TCHAR[bufferSize / sizeof(TCHAR)]);
    memset(pbuffer.get(), 0, bufferSize);
    status = SetupDiGetDeviceRegistryProperty(
        DevInfo,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        &dataType,
        (PBYTE)pbuffer.get(),
        bufferSize,
        &bufferSize);

    if (status == FALSE) {
        errorCode = GetLastError();
        if (errorCode == ERROR_INVALID_DATA) {
            //
            // May be a Legacy Device with no HardwareID. Continue.
            //
            return true;
        }
        else {
            LOG(LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode);
            return false;
        }
    }

    LOG(LOG_LEVEL_TRACE, _T("\n\nDevice ID: %s\n"), pbuffer.get());

    deviceId = pbuffer.get();

    return true;
}

bool local_storage::get_device_property(HDEVINFO IntDevInfo, DWORD Index, stdstring &devicePath)
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

    interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

    status = SetupDiEnumDeviceInterfaces(
        IntDevInfo,             // Interface Device Info handle
        0,                      // Device Info data
        (LPGUID)&DiskClassGuid, // Interface registered by driver
        Index,                  // Member
        &interfaceData          // Device Interface Data
        );

    if (status == FALSE) {
        errorCode = GetLastError();
        if (errorCode == ERROR_NO_MORE_ITEMS) {
            LOG(LOG_LEVEL_TRACE, _T("No more interfaces\n"));
        }
        else {
            LOG(LOG_LEVEL_ERROR, _T("SetupDiEnumDeviceInterfaces failed with error: %d\n"), errorCode);
        }
        return false;
    }

    //
    // Find out required buffer size, so pass NULL 
    //

    status = SetupDiGetDeviceInterfaceDetail(
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

    if (status == FALSE) {
        errorCode = GetLastError();
        if (errorCode != ERROR_INSUFFICIENT_BUFFER) {
            LOG(LOG_LEVEL_ERROR, _T("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), errorCode);
            return false;
        }
    }

    //
    // Allocate memory to get the interface detail data
    // This contains the devicepath we need to open the device
    //

    interfaceDetailDataSize = reqSize;
    p_interfaceDetailData = std::auto_ptr< SP_DEVICE_INTERFACE_DETAIL_DATA >((PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[interfaceDetailDataSize]);
    memset(p_interfaceDetailData.get(), 0, interfaceDetailDataSize);
    if (NULL == p_interfaceDetailData.get()) {
        LOG(LOG_LEVEL_ERROR, _T("Unable to allocate memory to get the interface detail data.\n"));
        return false;
    }
    p_interfaceDetailData.get()->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
    status = SetupDiGetDeviceInterfaceDetail(
        IntDevInfo,               // Interface Device info handle
        &interfaceData,           // Interface data for the event class
        p_interfaceDetailData.get(),      // Interface detail data
        interfaceDetailDataSize,  // Interface detail data size
        &reqSize,                 // Buffer size required to get the detail data
        NULL);                    // Interface device info

    if (status == FALSE) {
        LOG(LOG_LEVEL_ERROR, _T("Error in SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), GetLastError());
        return false;
    }

    devicePath = p_interfaceDetailData.get()->DevicePath;
    LOG(LOG_LEVEL_TRACE, _T("Interface: %s\n"), p_interfaceDetailData.get()->DevicePath);

    return true;
}

bool local_storage::get_disk_number(HANDLE handle, uint32_t &number){
    STORAGE_DEVICE_NUMBER      sdn;
    DWORD               cbReturned;
    BOOL               state;
    if (state = DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &cbReturned, NULL))
        number = sdn.DeviceNumber;
    return (TRUE == state);
}

bool local_storage::get_device_number(HANDLE handle, uint32_t& device_number, uint32_t& partition_number){
    STORAGE_DEVICE_NUMBER      sdn;
    DWORD               cbReturned;
    BOOL               state;
    if (state = DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &cbReturned, NULL)){
        device_number = sdn.DeviceNumber;
        partition_number = sdn.PartitionNumber;
    }
    return (TRUE == state);
}

bool local_storage::get_disk_geometry(HANDLE handle, DISK_GEOMETRY_EX &disk_geometry){
    DWORD               cbReturned;
    return (TRUE == DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &disk_geometry, sizeof(disk_geometry), &cbReturned, NULL));
}

bool local_storage::get_storage_query_property(HANDLE handle, DWORD &device_type, STORAGE_BUS_TYPE &bus_type, bool &is_remove_media, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &serial_number){
    bool ret = false;
    STORAGE_PROPERTY_QUERY              query;
    PSTORAGE_DEVICE_DESCRIPTOR          devDesc;
    BOOL                state;
    PUCHAR                              p;
    UCHAR                               outBuf[512];
    ULONG                               length = 0,
        returned = 0,
        returnedLength;
    DWORD                               i;

    memset(outBuf, 0, sizeof(outBuf));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    state = DeviceIoControl(handle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(STORAGE_PROPERTY_QUERY),
        &outBuf,
        512,
        &returnedLength,
        NULL);
    if (!state) {
        LOG(LOG_LEVEL_ERROR, _T("IOCTL failed with error code%d.\n\n"), GetLastError());
    }
    else {
        LOG(LOG_LEVEL_TRACE, _T("\nDevice Properties\n"));
        LOG(LOG_LEVEL_TRACE, _T("-----------------\n"));
        devDesc = (PSTORAGE_DEVICE_DESCRIPTOR)outBuf;
        //
        // Our device table can handle only 16 devices.
        //
        LOG(LOG_LEVEL_TRACE, _T("Device Type     : %s (0x%X)\n"),
            _device_types[devDesc->DeviceType > 0x0F ? 0x0F : devDesc->DeviceType], devDesc->DeviceType);
        if (devDesc->DeviceTypeModifier) {
            LOG(LOG_LEVEL_TRACE, _T("Device Modifier : 0x%x\n"), devDesc->DeviceTypeModifier);
        }
        device_type = devDesc->DeviceType;

        LOG(LOG_LEVEL_TRACE, _T("Bus Type     : %s (0x%X)\n"),
            _bus_types[devDesc->BusType > BusTypeMax ? BusTypeMax : devDesc->BusType], devDesc->BusType);
        bus_type = devDesc->BusType;

        is_remove_media = (devDesc->RemovableMedia == TRUE);
        LOG(LOG_LEVEL_TRACE, _T("Removable Media : %s\n"), devDesc->RemovableMedia ? _T("Yes") : _T("No"));
        p = (PUCHAR)outBuf;

        if (devDesc->VendorIdOffset && (devDesc->VendorIdOffset < returnedLength) && p[devDesc->VendorIdOffset]) {
            std::string szVendorId;
            for (i = devDesc->VendorIdOffset; p[i] != (UCHAR)NULL && i < returnedLength; i++) {
                szVendorId.push_back(p[i]);
            }
#if _UNICODE   
            vendor_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szVendorId));
#else
            vendor_id = stringutils::erase_trailing_whitespaces(szVendorId);
#endif
            LOG(LOG_LEVEL_TRACE, _T("Vendor ID       : %s\n"), vendor_id.c_str());
        }
        if (devDesc->ProductIdOffset && (devDesc->ProductIdOffset < returnedLength) && p[devDesc->ProductIdOffset]) {
            std::string szProductId;
            for (i = devDesc->ProductIdOffset; p[i] != (UCHAR)NULL && i < returnedLength; i++) {
                szProductId.push_back(p[i]);
            }
#if _UNICODE   
            product_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szProductId));
#else
            product_id = stringutils::erase_trailing_whitespaces(szProductId);
#endif
            LOG(LOG_LEVEL_TRACE, _T("Product ID      : %s\n"), product_id.c_str());
        }

        if (devDesc->ProductRevisionOffset && (devDesc->ProductRevisionOffset < returnedLength) && p[devDesc->ProductRevisionOffset]) {
            std::string szProductRevision;
            for (i = devDesc->ProductRevisionOffset; p[i] != (UCHAR)NULL && i < returnedLength; i++) {
                szProductRevision.push_back(p[i]);
            }
#if _UNICODE   
            product_revision = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szProductRevision));
#else
            product_revision = stringutils::erase_trailing_whitespaces(szProductRevision);
#endif
            LOG(LOG_LEVEL_TRACE, _T("Product Revision: %s\n"), product_revision.c_str());
        }

        if (devDesc->SerialNumberOffset && (devDesc->SerialNumberOffset < returnedLength) && p[devDesc->SerialNumberOffset]) {
            std::string szSerialNumber;
            for (i = devDesc->SerialNumberOffset; p[i] != (UCHAR)NULL && i < returnedLength; i++) {
                szSerialNumber.push_back(p[i]);
            }
#if _UNICODE   
            serial_number = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(szSerialNumber));
#else
            serial_number = stringutils::erase_trailing_whitespaces(szSerialNumber);
#endif
            LOG(LOG_LEVEL_TRACE, _T("Serial Number   : %s"), serial_number.c_str());
        }
    }
    return (TRUE == state);
}

bool local_storage::get_scsi_pass_through_data(HANDLE handle, DWORD &device_type, stdstring &vendor_id, stdstring &product_id, stdstring &product_revision, stdstring &verdor_string){
    SCSI_PASS_THROUGH_WITH_BUFFERS      sptwb;
    BOOL                                status;
    ULONG                               length = 0,
        returned = 0;
    DWORD                               i,
        errorCode;
    ZeroMemory(&sptwb, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

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
        offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf);
    sptwb.Spt.SenseInfoOffset =
        offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, SenseBuf);
    sptwb.Spt.Cdb[0] = SCSIOP_INQUIRY;
    sptwb.Spt.Cdb[4] = 192;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf) +
        sptwb.Spt.DataTransferLength;

    status = DeviceIoControl(handle,
        IOCTL_SCSI_PASS_THROUGH,
        &sptwb,
        sizeof(SCSI_PASS_THROUGH),
        &sptwb,
        length,
        &returned,
        FALSE);
    LOG(LOG_LEVEL_TRACE, _T("Inquiry Data from Pass Through\n"));
    LOG(LOG_LEVEL_TRACE, _T("------------------------------\n"));

    if (!status) {
        LOG(LOG_LEVEL_ERROR, _T("Error: %d "), errorCode = GetLastError());
        return FALSE;
    }
    if (sptwb.Spt.ScsiStatus) {
        LOG(LOG_LEVEL_ERROR, _T("Scsi status: %02Xh\n"), sptwb.Spt.ScsiStatus);
        return FALSE;
    }
    else {

        device_type = sptwb.DataBuf[0] & 0x1f;
        //
        // Our Device Table can handle only 16 devices.
        //
        LOG(LOG_LEVEL_TRACE, _T("Device Type: %s (0x%X)\n"), _device_types[device_type > 0x0F ? 0x0F : device_type], device_type);
        std::string tempstr;
        for (i = 8; i <= 15; i++) {
            tempstr.push_back(sptwb.DataBuf[i]);
        }
#if _UNICODE   
        vendor_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        vendor_id = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG(LOG_LEVEL_TRACE, _T("Vendor ID  : %s"), vendor_id.c_str());
        tempstr.clear();
        for (i = 16; i <= 31; i++) {
            tempstr.push_back(sptwb.DataBuf[i]);
        }
#if _UNICODE   
        product_id = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        product_id = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG(LOG_LEVEL_TRACE, _T("Product ID : %s"), product_id.c_str());
        tempstr.clear();
        for (i = 32; i <= 35; i++) {
            tempstr.push_back(sptwb.DataBuf[i]);
        }
#if _UNICODE   
        product_revision = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        product_revision = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG(LOG_LEVEL_TRACE, _T("Product Rev: %s"), product_revision.c_str());
        tempstr.clear();
        for (i = 36; i <= 55 && sptwb.DataBuf[i] != NULL; i++) {
            tempstr.push_back(sptwb.DataBuf[i]);
        }
#if _UNICODE   
        verdor_string = stringutils::erase_trailing_whitespaces(stringutils::convert_ansi_to_unicode(tempstr));
#else
        verdor_string = stringutils::erase_trailing_whitespaces(tempstr);
#endif
        LOG(LOG_LEVEL_TRACE, _T("Vendor Str : %s"), verdor_string.c_str());
    }
    return (TRUE == status);
}

bool local_storage::get_drive_layout(HANDLE handle, boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > &pdliex){
    DWORD                            bufsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 3;
    boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > pdli((DRIVE_LAYOUT_INFORMATION_EX *) new BYTE[bufsize]);
    BOOL                            fResult;
    DWORD                            dwBytesReturned;

    while (!(fResult = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
        NULL, 0, pdli.get(), bufsize, &dwBytesReturned, NULL))){
        if ((GetLastError() == ERROR_MORE_DATA) ||
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER)){
            // Create enough space for four more partition table entries.
            bufsize += sizeof(PARTITION_INFORMATION_EX) * 4;
            pdli = boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX >((DRIVE_LAYOUT_INFORMATION_EX *) new BYTE[bufsize]);
        }
        else{
            LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed"));
            break;
        }
    }
    if (fResult) pdliex = pdli;
    return (TRUE == fResult);
}

bool local_storage::get_disk_length(HANDLE handle, ULONGLONG &length){
    GET_LENGTH_INFORMATION      gLength;
    BOOL                        state;
    DWORD                       cbReturned;
    if (state = DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gLength, sizeof(gLength), &cbReturned, NULL))
        length = gLength.Length.QuadPart;
    return (TRUE == state);
}

bool local_storage::detect_sector_size(HANDLE handle, uint32_t &logical_sector_size, uint32_t& physical_sector_size){
    DWORD                  Bytes = 0;
    DWORD                  Error = NO_ERROR;
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment = { 0 };
    STORAGE_PROPERTY_QUERY Query;
    ZeroMemory(&Query, sizeof(Query));
    Query.QueryType = PropertyStandardQuery;
    Query.PropertyId = StorageAccessAlignmentProperty;
    BOOL bReturn = DeviceIoControl(handle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &Query,
        sizeof(STORAGE_PROPERTY_QUERY),
        &alignment,
        sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR),
        &Bytes,
        NULL);
    if (bReturn == FALSE) {
        LOG(LOG_LEVEL_DEBUG, L"bReturn==FALSE. GetLastError() returns %lu.", Error = GetLastError());
        return false;
    }
    logical_sector_size = alignment.BytesPerLogicalSector;
    physical_sector_size = alignment.BytesPerPhysicalSector;
    return true;
}

bool local_storage::is_clustered(HANDLE handle){
    BOOL                        clustered;
    BOOL                        state;
    DWORD                       cbReturned;
    state = DeviceIoControl(handle, IOCTL_DISK_IS_CLUSTERED, NULL, 0, &clustered, sizeof(clustered), &cbReturned, NULL);
    return (TRUE == state) && (TRUE == clustered );
}

bool local_storage::is_writable(HANDLE handle){
    DWORD                       cbReturned;
    return (TRUE == DeviceIoControl(handle, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &cbReturned, NULL));
}

bool local_storage::get_disk_scsi_address(HANDLE handle, UCHAR &port_number, UCHAR &path_id, UCHAR &target_id, UCHAR &lun)
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

bool local_storage::get_disk(HANDLE handle, local_disk& _disk){

    bool status = false;
    if (INVALID_HANDLE_VALUE != handle){
        if (!(status = get_disk_number(handle, _disk._number))){
            LOG(LOG_LEVEL_ERROR, _T("Can't get disk number (%s). error( 0x%08X )"), _disk._path.c_str(), GetLastError());
        }
        else{
            DISK_GEOMETRY_EX geometry;
            memset(&geometry, 0, sizeof(DISK_GEOMETRY_EX));
            if (!(status = get_disk_geometry(handle, geometry))){
                LOG(LOG_LEVEL_ERROR, _T("Can't get disk geometry (%s). error( 0x%08X )"), _disk._path.c_str(), GetLastError());
            }
            else{
                _disk._sectors_per_track = geometry.Geometry.SectorsPerTrack;
                _disk._tracks_per_cylinder = geometry.Geometry.TracksPerCylinder;
                _disk._total_cylinders = geometry.Geometry.Cylinders.QuadPart;
                _disk._logical_sector_size = _disk._physical_sector_size = geometry.Geometry.BytesPerSector;
                _disk._size = geometry.DiskSize.QuadPart;
                DWORD device_type = 0;
                bool  is_remove_media = false;
                STORAGE_BUS_TYPE bus_type = STORAGE_BUS_TYPE::BusTypeUnknown;
                stdstring vendor_string;
                if (!(status = get_storage_query_property(handle, device_type, bus_type, is_remove_media,
                    _disk._manufacturer, _disk._model, _disk._firmware_version, _disk._serial_number))){
                    LOG(LOG_LEVEL_ERROR, _T("Can't get storage query property information (%s). error( 0x%08X )"), _disk._path.c_str(), GetLastError());
                    if (!(status = get_scsi_pass_through_data(handle, device_type, _disk._manufacturer, _disk._model, _disk._firmware_version, vendor_string))){
                        LOG(LOG_LEVEL_ERROR, _T("Can't get query scsi pass through data (%s). error( 0x%08X )"), _disk._path.c_str(), GetLastError());
                    }
                    else if (stdstring::npos != _disk._manufacturer.find(_T("FALCON"))){ // If the disk is IPStor disk. the vendor string is equal as the serial number.
                        _disk._serial_number = vendor_string;
                    }
                }
                else{
                    _disk._bus_type = bus_type;
                    /*DWORD type;
                    stdstring vender_id, product_id, product_revision;
                    if (!get_scsi_pass_through_data(handle, type, vender_id, product_id, product_revision, vendor_string)){
                        LOG(LOG_LEVEL_ERROR, _T("Can't get query scsi pass through data (%s). error( 0x%08X )"), _disk._path.c_str(), GetLastError());
                    }*/
                }
            }

            if (status){
                if (_disk._manufacturer.empty())
                    _disk._friendly_name = _disk._model;
                else
                    _disk._friendly_name = boost::str(boost::wformat(L"%s %s") % _disk._manufacturer %_disk._model);
                boost::shared_ptr< DRIVE_LAYOUT_INFORMATION_EX > pdliex;
                if (!(status = get_drive_layout(handle, pdliex))){
                    LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX (%s) failed. error( 0x%08X )"), _disk._path.c_str(), GetLastError());
                }
                else{
                    get_disk_length(handle, _disk._size);               
                    _disk._pdliex = pdliex;
                   // _disk._is_readable = true;
                    //_disk._is_dynamic_disk = is_dynamic_disk(pdliex);
                    //_disk._is_boot = is_bootable_disk(pdliex);
                    _disk._is_clustered = is_clustered(handle);
                    _disk._is_read_only = !is_writable(handle);
                    detect_sector_size(handle, _disk._logical_sector_size, _disk._physical_sector_size);
                    UCHAR scsi_bus = 0, scsi_logical_unit = 0, scsi_target_id = 0, scsi_port = 0;
                    if (status = get_disk_scsi_address(handle, scsi_port, scsi_bus, scsi_target_id, scsi_logical_unit)){
                        _disk._scsi_bus = scsi_bus;
                        _disk._scsi_port = scsi_port;
                        _disk._scsi_target_id = scsi_target_id;
                        _disk._scsi_logical_unit = scsi_logical_unit;
                    }
                }
            }
        }
    }
    return status;
}

void local_storage::enumerate_disks(){
    auto_setupdi_handle   dev_info, int_dev_info;
    DWORD                  index;
    BOOL                  status = FALSE;
    BOOL                  result = FALSE;

    dev_info = SetupDiGetClassDevs(
        (LPGUID)&GUID_DEVCLASS_DISKDRIVE,
        NULL,
        NULL,
        DIGCF_PRESENT); // All devices present on system
    if (dev_info.is_invalid()){
        LOG(LOG_LEVEL_ERROR, _T("SetupDiGetClassDevs failed with error: %d\n"), GetLastError());
    }
    else{
        //
        // Open the device using device interface registered by the driver
        //
        //
        // Get the interface device information set that contains all devices of event class.
        //
        int_dev_info = SetupDiGetClassDevs(
            (LPGUID)&DiskClassGuid,
            NULL,                                   // Enumerator
            NULL,                                   // Parent Window
            (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE  // Only Devices present & Interface class
            ));

        if (int_dev_info.is_invalid()) {
            LOG(LOG_LEVEL_ERROR, _T("SetupDiGetClassDevs failed with error: %d\n"), GetLastError());
        }
        else{
            //
            //  Enumerate all the disk devices
            //
            index = 0;
            while (TRUE) {
                stdstring            devicePath, deviceId;
                auto_file_handle    device_handle;
                LOG(LOG_LEVEL_TRACE, _T("Properties for Device %d"), index + 1);
                status = get_registry_property(dev_info, index, deviceId);
                if (status == FALSE) {
                    break;
                }

                status = get_device_property(int_dev_info, index, devicePath);
                if (status == FALSE) {
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
                if (device_handle.is_invalid()){
                    LOG(LOG_LEVEL_ERROR, _T("CreateFile(%s) failed. error( 0x%08X )"), devicePath.c_str(), GetLastError());
                }
                else{
                    uint32_t number = -1;
                    if (get_disk_number(device_handle, number)){
                        storage::disk::ptr disk;
                        try{
                            disk = _stg ? _stg->get_disk(number) : NULL;
                        }
                        catch (macho::exception_base& ex){
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                        }
                        local_disk* _disk = new local_disk(disk);
                        if (_disk){
                            storage::disk::ptr disk(_disk);
                            _disk->_path = devicePath;
                            _disk->_number = number;
                            if (get_disk(device_handle, *_disk))
                                _disks.push_back(disk);
                        }
                    }
                }
            }
        }
    }
}

void local_storage::enumerate_volumes(){
    TCHAR volume_name[MAX_PATH];
    memset(volume_name, 0, sizeof(volume_name));
    auto_find_volume_handle handle = FindFirstVolume(volume_name, ARRAYSIZE(volume_name));
    if (handle.is_invalid()){
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to find first volume info [Error: %d]"), GetLastError());
    }
    else{
        std::map<std::wstring, storage::volume::ptr, stringutils::no_case_string_less_w> volumes;
        if (_stg){
            try{
                storage::volume::vtr volumes_vtr = _stg->get_volumes();
                foreach(storage::volume::ptr &v, volumes_vtr)
                    volumes[v->path()] = v;
            }
            catch (macho::exception_base& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
            }
        }
        for (;;){
            size_t index = _tcsclen(volume_name) - 1;
            if (volume_name[0] != TEXT('\\') ||
                volume_name[1] != TEXT('\\') ||
                volume_name[2] != TEXT('?') ||
                volume_name[3] != TEXT('\\') ||
                volume_name[index] != TEXT('\\')) {
                LOG(LOG_LEVEL_ERROR, TEXT("Invalid path of FindFirstVolume()/FindNextVolume() returned: %s"), volume_name);
            }
            else{
                local_volume* v = new local_volume(volumes.count(volume_name) ? volumes[volume_name] : NULL); 
                if (v){
                    storage::volume::ptr volume(v);
                    if (get_volume(volume_name, *v))
                        _volumes.push_back(volume);
                }
            }
            // Move on to the next volume
            if (!FindNextVolume(handle, volume_name, ARRAYSIZE(volume_name))){
                DWORD error = GetLastError();
                if (error == ERROR_NO_MORE_FILES)
                    break;
                LOG(LOG_LEVEL_ERROR, TEXT("Failed to find next volume [Error: %d]"), error);
                break;
            }
        }
    }
}

bool local_storage::get_volume(stdstring device_id, local_volume& volume){
    bool  result = false;
    TCHAR file_system[MAX_PATH + 1];
    TCHAR volume_label[MAX_PATH + 1];
    ULARGE_INTEGER capacity, free_space;
    memset(file_system, 0, sizeof(file_system));
    memset(volume_label, 0, sizeof(volume_label));
    volume._drive_type = (storage::ST_DRIVE_TYPE) GetDriveType(device_id.c_str());
    volume._path = device_id;
    if (result = get_mount_points(device_id, volume._access_paths)){
        foreach(stdstring mount_point, volume._access_paths){
            if ((0 < mount_point.length()) &&
                (3 >= mount_point.length()) &&
                (IsCharAlpha(mount_point[0])) &&
                (_T(':') == mount_point[1]) &&
                ((_T('\0') == mount_point[2]) || ((_T('\\') == mount_point[2])))){
                volume._drive_letter = mount_point;
                volume._drive_letter.erase(2);
            }
        }
    }
    volume._access_paths.push_back(device_id);
    if (3 == volume._drive_type || 2 == volume._drive_type)
        result = get_disk_extents(device_id, volume);
    if (!(result = (TRUE == GetVolumeInformation(device_id.c_str(), volume_label, MAX_PATH, NULL, NULL, NULL, file_system, MAX_PATH)))){
        LOG(LOG_LEVEL_ERROR, TEXT("Failed to GetVolumeInformation (%s) [Error: %d]"), device_id.c_str(), GetLastError());
    }
    else{
        volume._file_system = file_system;
        volume._file_system_label = volume_label;
        if (!(result = (TRUE == GetDiskFreeSpaceEx(device_id.c_str(), NULL, &capacity, &free_space)))){
            LOG(LOG_LEVEL_ERROR, TEXT("Failed to GetDiskFreeSpaceEx (%s) [Error: %d]"), device_id.c_str(), GetLastError());
        }
        else{
            volume._size = capacity.QuadPart;
            volume._size_remaining = free_space.QuadPart;
        }
    }
    return true;
}

bool local_storage::get_mount_points(stdstring device_id, string_table& mount_points){

    DWORD                    CharCount = MAX_PATH + 1;
    std::auto_ptr<TCHAR>    pNames;
    TCHAR*                    NameIdx = NULL;
    BOOL                    Success = FALSE;
    mount_points.clear();
    if (device_id[device_id.length() - 1] != _T('\\'))
        device_id.append(_T("\\"));

    for (;;) {
        //
        //  Allocate a buffer to hold the paths.
        pNames = std::auto_ptr<TCHAR>(new TCHAR[CharCount]);

        if (!pNames.get()) //  If memory can't be allocated, return.
            return false;
        //  Obtain all of the paths for this volume.
        if (Success = GetVolumePathNamesForVolumeName(
            device_id.c_str(), pNames.get(), CharCount, &CharCount))
            break;
        else if (GetLastError() != ERROR_MORE_DATA)
            break;
    }

    if (Success){
        //  Get the various paths.
        for (NameIdx = pNames.get();
            NameIdx[0] != _T('\0');
            NameIdx += _tcslen(NameIdx) + 1){
            mount_points.push_back(stdstring(NameIdx));
        }
    }
    return (TRUE == Success);
}

bool local_storage::get_disk_extents(stdstring device_id, local_volume& volume){
    BOOL result = TRUE;
    volume._extents.clear();
    std::auto_ptr<VOLUME_DISK_EXTENTS> pDiskExtents((VOLUME_DISK_EXTENTS *) new BYTE[sizeof(VOLUME_DISK_EXTENTS)]);
    memset(pDiskExtents.get(), 0, sizeof(VOLUME_DISK_EXTENTS));
    if ((5 > device_id.length()) ||
        (device_id[0] != _T('\\') ||
        device_id[1] != _T('\\') ||
        device_id[2] != _T('?') ||
        device_id[3] != _T('\\') ||
        device_id[device_id.length() - 1] != _T('\\'))) {
        result = FALSE;
    }
    else{
        device_id.erase(device_id.length() - 1);
        auto_file_handle device_handle = CreateFile(device_id.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (device_handle.is_invalid()){
            LOG(LOG_LEVEL_ERROR, _T("Can't access volume (%s). error( 0x%08X )"), device_id.c_str(), GetLastError());
            result = FALSE;
        }
        else{
            DWORD bufferSize;
            if (!(result = DeviceIoControl(device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), sizeof(VOLUME_DISK_EXTENTS), &bufferSize, 0))){
                DWORD error = GetLastError();
                if (((error == ERROR_INSUFFICIENT_BUFFER) || (error == ERROR_MORE_DATA)) && (pDiskExtents->NumberOfDiskExtents > 1)){
                    bufferSize = sizeof(VOLUME_DISK_EXTENTS) + (pDiskExtents->NumberOfDiskExtents*sizeof(DISK_EXTENT));
                    pDiskExtents = std::auto_ptr<VOLUME_DISK_EXTENTS>((VOLUME_DISK_EXTENTS *) new BYTE[bufferSize]);
                    if (!(result = DeviceIoControl(device_handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents.get(), bufferSize, &bufferSize, 0))){
                        LOG(LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), device_id.c_str(), GetLastError());
                    }
                }
                else{
                    LOG(LOG_LEVEL_ERROR, _T("Can't get volume (%s) disk extents. error( 0x%08X )"), device_id.c_str(), error);
                }
            }
        }
    }

    for (DWORD i = 0; i < pDiskExtents->NumberOfDiskExtents; i++){
        volume._extents.push_back(local_volume::extent(pDiskExtents->Extents[i].DiskNumber, pDiskExtents->Extents[i].StartingOffset.QuadPart, pDiskExtents->Extents[i].ExtentLength.QuadPart));
    }
    return (TRUE == result);
}

void local_storage::enumerate_partitions(){
    _partitions.clear();
    storage::partition::vtr partitions;
    if (_stg){
        try{
            partitions = _stg->get_partitions();
        }
        catch (macho::exception_base& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
        }
    }
    foreach(storage::disk::ptr d, _disks){
        local_disk* disk = dynamic_cast<local_disk*>(d.get());
        if (_boot_disk == disk->number())
            disk->_boot_from_disk = disk->_is_boot = disk->_is_system = true;
        if (disk->_pdliex){
            disk->_partitions.clear();
            disk->_volumes.clear();
            disk->_number_of_partitions = 0;
            uint64_t latest_end_of_partition = 0;
            uint64_t real_end_of_partition = 0;
            for (DWORD i = 0; i < disk->_pdliex->PartitionCount; ++i){        
                local_partition *p = new local_partition(disk->number(), disk->_pdliex->PartitionEntry[i]);
                if (p){
                    storage::partition::ptr _p(p);
                    if ( p->offset() && p->size()){
                        if (1 == p->partition_number()){
                            disk->_allocated_size += p->offset();
                            latest_end_of_partition = real_end_of_partition = p->offset();
                        }
                        if (latest_end_of_partition < real_end_of_partition){
                            uint64_t free_extent = real_end_of_partition - latest_end_of_partition;
                            if (1048576ULL > free_extent)
                                disk->_allocated_size += free_extent;
                            else if (free_extent > disk->_largest_free_extent)
                                disk->_largest_free_extent = free_extent;
                            latest_end_of_partition = real_end_of_partition;
                        }
                        if (latest_end_of_partition < p->offset()){
                            uint64_t free_extent = p->offset() - latest_end_of_partition;
                            if (1048576ULL > free_extent )
                                disk->_allocated_size += free_extent;
                            else if (free_extent >disk->_largest_free_extent)
                                disk->_largest_free_extent = free_extent;
                        }
                        uint64_t end_of_partition = 0;
                        uint64_t _real_end_of_partition = 0;
                        if (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)
                            end_of_partition = p->offset() + disk->logical_sector_size();
                        else
                            end_of_partition = p->offset() + p->size();
                        _real_end_of_partition = p->offset() + p->size();
                        if (real_end_of_partition < _real_end_of_partition)
                            real_end_of_partition = _real_end_of_partition;
                        if (latest_end_of_partition < end_of_partition){
                            latest_end_of_partition = end_of_partition;
                            if (!(p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED))
                                disk->_allocated_size += p->size();
                            else
                                disk->_allocated_size += disk->logical_sector_size();
                        }

                        disk->_number_of_partitions++;
                        if (_boot_disk == p->disk_number() && _system_partition == p->partition_number())
                            p->_is_boot = true;
                        if (_boot_disk == p->disk_number() && p->is_active())
                            p->_is_system = true;
                        foreach(storage::volume::ptr v, _volumes){
                            local_volume * _v = dynamic_cast<local_volume*>(v.get());
                            foreach(local_volume::extent& e, _v->_extents){
                                if ((e.disk_number == p->disk_number()) &&
                                    (e.offset >= p->offset()) &&
                                    (e.offset < (p->offset() + p->size())) &&
                                    (e.length <= p->size())){
                                    p->_access_paths = _v->_access_paths;
                                    p->_drive_letter = _v->_drive_letter;
                                    disk->_volumes.push_back(v);
                                    break;
                                }
                            }
                        }
                        foreach(storage::partition::ptr &obj, partitions){
                            if (p->disk_number() == obj->disk_number() && p->partition_number() == obj->partition_number()){
                                p->_partition = obj;
                                break;
                            }
                        }
                        disk->_partitions.push_back(_p);
                        _partitions.push_back(_p);
                    }
                }
            }
            
            if (latest_end_of_partition < disk->size()) {
                uint64_t end_extent = disk->size() - latest_end_of_partition;
                if (1048576ULL > end_extent)
                    disk->_allocated_size += end_extent;
                else if ((end_extent - 1048576ULL) > disk->_largest_free_extent)
                    disk->_largest_free_extent = (end_extent - 1048576ULL);
            }
        }
    }
}

void local_storage::rescan(){
    macho::windows::auto_lock lock(_cs);
    _disks.clear();
    _volumes.clear();
    enumerate_disks();
    enumerate_volumes();
    enumerate_partitions();
}

local_partition::local_partition(uint32_t disk_number, const PARTITION_INFORMATION_EX &entry) :
_disk_number(disk_number),
_partition_number(entry.PartitionNumber),
_offset(entry.StartingOffset.QuadPart),
_size(entry.PartitionLength.QuadPart),
_mbr_type(0),
_is_read_only(false),
_is_offline(false),
_is_system(false),
_is_boot(false),
_is_active(false),
_is_hidden(false),
_is_shadow_copy(false),
_no_default_drive_letter(false){
    switch (entry.PartitionStyle){
    case PARTITION_STYLE_MBR:
        if (entry.Mbr.RecognizedPartition && entry.Mbr.BootIndicator)
           _is_active = true;
        _mbr_type = entry.Mbr.PartitionType;
        //_is_hidden = !entry.Mbr.RecognizedPartition;
        break;
    case PARTITION_STYLE_GPT:
        if (entry.Gpt.PartitionType == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")) //PARTITION_SYSTEM_GUID
            _is_active = true;
        _gpt_type = macho::guid_(entry.Gpt.PartitionType).wstring();
        _guid = macho::guid_(entry.Gpt.PartitionId).wstring();
        if (60 == entry.Gpt.Attributes)
            _is_read_only = true;
        else if (62 == entry.Gpt.Attributes)
            _is_hidden = true;
        else if (63 == entry.Gpt.Attributes)
            _no_default_drive_letter = true;
        break;
    }
}

const TCHAR* local_storage::_bus_types[BusTypeMax + 1] = {
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

const TCHAR* local_storage::_device_types[16] = {
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

#ifdef _WIN2K3
void  vds_storage::get_system_device_number(){
    local_storage::get_system_device_number(_boot_disk, _system_partition);
}
#endif
#endif // MACHO_HEADER_ONLY

#endif // __MACHO_WINDOWS_STORAGE__

