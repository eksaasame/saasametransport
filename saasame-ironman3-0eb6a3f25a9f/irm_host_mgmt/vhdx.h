#pragma once
#ifndef __IRM_HOST_MGMT_VHDX__
#define __IRM_HOST_MGMT_VHDX__

#include "macho.h"
#include <virtdisk.h>
#include "universal_disk_rw.h"
#pragma comment (lib, "VirtDisk.lib")

struct vhd_disk_info
{
    vhd_disk_info() : drive_type(0)
        , drive_format(0)
        , identifier(GUID_NULL)
        , physical_size(0)
        , virtual_size(0)
        , min_internal_size(0)
        , block_size(0)
        , logical_sector_size(0)
        , physical_sector_size(0)
        , parent_identifier(GUID_NULL) {}

    std::wstring              vhd_file;
    std::vector<std::wstring> parents;
    int                       drive_type;
    int                       drive_format;
    GUID                      identifier;
    ULONGLONG                 physical_size;
    ULONGLONG                 virtual_size;
    ULONGLONG                 min_internal_size;
    ULONG                     block_size;
    ULONG                     logical_sector_size;
    ULONG                     physical_sector_size;
    GUID                      parent_identifier;
};

class win_vhdx_file_rw : public virtual universal_disk_rw{
public:
    //typedef boost::shared_ptr<win_vhdx_file_rw> ptr;
    win_vhdx_file_rw(HANDLE &handle, std::wstring& virtual_disk_path, bool read_only = true);

    //virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read);
    //virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written);
    virtual bool write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual std::wstring path() const { return _virtual_disk_path; }
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone();

private:
    macho::windows::critical_section _cs;
    std::wstring                     _virtual_disk_path;
    macho::windows::auto_handle      _handle;
    vhd_disk_info                    _info;
    bool                             _read_only;
};

struct win_vhdx_mgmt{
    static DWORD create_vhdx_file(__in std::wstring virtual_disk_path, __in CREATE_VIRTUAL_DISK_FLAG flags, __in ULONGLONG file_size, __in DWORD block_size, __in DWORD logical_sector_size, __in DWORD physical_sector_size, __in std::wstring parent_path = L"");
    static DWORD attach_virtual_disk(__in std::wstring virtual_disk_path, __in bool read_only = false);
    static DWORD detach_virtual_disk(__in std::wstring virtual_disk_path);
    static DWORD merge_virtual_disk(__in std::wstring leaf_path);
    static DWORD get_virtual_disk_physical_path(__in std::wstring virtual_disk_path, __inout std::wstring& physical_disk_path);
    static DWORD get_attached_physical_disk_number(__in std::wstring virtual_disk_path, __inout DWORD& disk_number);
    static DWORD get_virtual_disk_information(__in std::wstring virtual_disk_path, __inout vhd_disk_info &disk);
    static universal_disk_rw::ptr open_virtual_disk_for_rw(__in std::wstring virtual_disk_path, __in bool read_only = false);
    static DWORD get_all_attached_virtual_disk_physical_paths(std::vector<std::wstring> &paths);
    static DWORD get_all_attached_virtual_disk_physical_paths(std::vector<DWORD> &disks);
};


#endif