#include "macho.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <initguid.h>
#include <virtdisk.h>
#include <rpc.h>
#include <sddl.h>
#include "Strsafe.h"
#include "vhdx.h"

using namespace macho;

bool is_windows_version_or_greater(WORD wMajorVersion, WORD wMinorVersion)
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
    DWORDLONG        const dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(
        0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = wMajorVersion;
    osvi.dwMinorVersion = wMinorVersion;

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION |
        VER_MINORVERSION, dwlConditionMask) != FALSE;
}

//bool win_vhdx_file_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
//
//    std::auto_ptr<BYTE> buf = std::auto_ptr<BYTE>(new BYTE[number_of_bytes_to_read]);
//    uint32_t number_of_bytes_read = 0;
//    bool result = false;
//    if (result = read(start, number_of_bytes_to_read, buf.get(), number_of_bytes_read)){
//        output = std::string(reinterpret_cast<char const*>(buf.get()), number_of_bytes_read);
//    }
//    return result;
//}

win_vhdx_file_rw::win_vhdx_file_rw(HANDLE &handle, std::wstring& virtual_disk_path, bool read_only) : _handle(handle), _virtual_disk_path(virtual_disk_path), _read_only(read_only){
    win_vhdx_mgmt::get_virtual_disk_information(_virtual_disk_path, _info);
}


bool win_vhdx_file_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read){
    
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    DWORD opStatus = ERROR_SUCCESS;
    LARGE_INTEGER offset;
    offset.QuadPart = start;
    overlapped.Offset = offset.LowPart;
    overlapped.OffsetHigh = offset.HighPart;
    macho::windows::auto_lock lock(_cs);
    if (!ReadFile(
        _handle,
        buffer,
        number_of_bytes_to_read,
        (LPDWORD)&number_of_bytes_read,
        &overlapped)){
        if (ERROR_IO_PENDING == (opStatus = GetLastError())){
            if (!GetOverlappedResult(_handle, &overlapped, (LPDWORD)&number_of_bytes_read, TRUE)){
                opStatus = GetLastError();
            }
        }

        if (opStatus != ERROR_SUCCESS){
            LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
            return false;
        }
    }
    return true;
}

//bool win_vhdx_file_rw::write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
//    return write(start, buf.c_str(), (uint32_t)buf.size(), number_of_bytes_written);
//}

bool win_vhdx_file_rw::write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
    
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    DWORD opStatus = ERROR_SUCCESS;
    LARGE_INTEGER offset;
    offset.QuadPart = start;
    overlapped.Offset = offset.LowPart;
    overlapped.OffsetHigh = offset.HighPart;
    macho::windows::auto_lock lock(_cs);
    if (!WriteFile(
        _handle,
        buffer,
        number_of_bytes_to_write,
        (LPDWORD)&number_of_bytes_written,
        &overlapped)){
        if (ERROR_IO_PENDING == ( opStatus = GetLastError() ) ){
            opStatus = ERROR_SUCCESS;
            if (!GetOverlappedResult(_handle, &overlapped, (LPDWORD)&number_of_bytes_written, TRUE)){
                opStatus = GetLastError();
            }
        }

        if (opStatus != ERROR_SUCCESS){
            LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
            return false;
        }

        if (number_of_bytes_written != number_of_bytes_to_write){
            opStatus = ERROR_HANDLE_EOF;
            LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
            return false;
        }
    }
    return true;
}

bool win_vhdx_file_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    uint32_t number_of_bytes_read = 0;
    number_of_sectors_read = 0;
    if (read(start_sector *_info.logical_sector_size, number_of_sectors_to_read *_info.logical_sector_size, buffer, number_of_bytes_read)){
        number_of_sectors_read = number_of_bytes_read / _info.logical_sector_size;
        return true;
    }
    return false;
}

bool win_vhdx_file_rw::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
    uint32_t number_of_bytes_written = 0;
    number_of_sectors_written = 0;
    if (write(start_sector *_info.logical_sector_size, buffer, number_of_sectors_to_write*_info.logical_sector_size, number_of_bytes_written)){
        number_of_sectors_written = number_of_bytes_written / _info.logical_sector_size;
        return true;
    }
    return false;
}

universal_disk_rw::ptr win_vhdx_file_rw::clone(){
    return win_vhdx_mgmt::open_virtual_disk_for_rw(_virtual_disk_path, _read_only);
}

DWORD win_vhdx_mgmt::create_vhdx_file(__in std::wstring virtual_disk_path, __in CREATE_VIRTUAL_DISK_FLAG flags, __in ULONGLONG file_size, __in DWORD block_size, __in DWORD logical_sector_size, __in DWORD physical_sector_size, std::wstring parent_path){

    VIRTUAL_STORAGE_TYPE storageType;
    CREATE_VIRTUAL_DISK_PARAMETERS parameters;
    VIRTUAL_DISK_ACCESS_MASK accessMask;
    HANDLE vhdHandle = INVALID_HANDLE_VALUE;
    DWORD opStatus;
    GUID uniqueId;
    if (RPC_S_OK != UuidCreate((UUID*)&uniqueId)){
        opStatus = ERROR_NOT_ENOUGH_MEMORY;
    }
    else{
        //
        // Specify UNKNOWN for both device and vendor so the system will use the
        // file extension to determine the correct VHD format.
        //
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

        memset(&parameters, 0, sizeof(parameters));
        //
        // CREATE_VIRTUAL_DISK_VERSION_2 allows specifying a richer set a values and returns
        // a V2 handle.
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        //
        // Valid BlockSize values are as follows (use 0 to indicate default value):
        //      Fixed VHD: 0
        //      Dynamic VHD: 512kb, 2mb (default)
        //      Differencing VHD: 512kb, 2mb (if parent is fixed, default is 2mb; if parent is dynamic or differencing, default is parent blocksize)
        //      Fixed VHDX: 0
        //      Dynamic VHDX: 1mb, 2mb, 4mb, 8mb, 16mb, 32mb (default), 64mb, 128mb, 256mb
        //      Differencing VHDX: 1mb, 2mb (default), 4mb, 8mb, 16mb, 32mb, 64mb, 128mb, 256mb
        //
        // Valid LogicalSectorSize values are as follows (use 0 to indicate default value):
        //      VHD: 512 (default)
        //      VHDX: 512 (for fixed or dynamic, default is 512; for differencing, default is parent logicalsectorsize), 4096
        //
        // Valid PhysicalSectorSize values are as follows (use 0 to indicate default value):
        //      VHD: 512 (default)
        //      VHDX: 512, 4096 (for fixed or dynamic, default is 4096; for differencing, default is parent physicalsectorsize)
        //
        
        if (is_windows_version_or_greater(6, 2)){
            parameters.Version = CREATE_VIRTUAL_DISK_VERSION_2;
            parameters.Version2.UniqueId = uniqueId;
            parameters.Version2.MaximumSize = file_size;
            parameters.Version2.BlockSizeInBytes = block_size;
            parameters.Version2.SectorSizeInBytes = logical_sector_size;
            parameters.Version2.PhysicalSectorSizeInBytes = physical_sector_size;
            parameters.Version2.ParentPath = parent_path.length() ? parent_path.c_str() : NULL;
            accessMask = VIRTUAL_DISK_ACCESS_NONE;
            if (parent_path.length()){
                LPCTSTR extension = ::PathFindExtension(parent_path.c_str());
                if (extension != NULL && _wcsicmp(extension, L".vhd") == 0){
                    parameters.Version2.MaximumSize = 0;
                    parameters.Version2.BlockSizeInBytes = 0;
                    parameters.Version2.SectorSizeInBytes = 0;
                    parameters.Version2.PhysicalSectorSizeInBytes = 0;
                }
            }
        }
        else{
            parameters.Version = CREATE_VIRTUAL_DISK_VERSION_1;
            parameters.Version1.UniqueId = uniqueId;
            parameters.Version1.MaximumSize = file_size;
            parameters.Version1.BlockSizeInBytes = block_size;
            parameters.Version1.SectorSizeInBytes = logical_sector_size;
            parameters.Version1.ParentPath = parent_path.length() ? parent_path.c_str() : NULL;
            accessMask = VIRTUAL_DISK_ACCESS_ALL;
            storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
            storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;
        }

        if (ERROR_SUCCESS == (opStatus = CreateVirtualDisk(
            &storageType,
            virtual_disk_path.c_str(),
            accessMask,
            NULL,
            flags,
            0,
            &parameters,
            NULL,
            &vhdHandle))){

            if (vhdHandle != INVALID_HANDLE_VALUE){
                CloseHandle(vhdHandle);
            }
        }
    }
    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }
    return opStatus;
}

DWORD win_vhdx_mgmt::attach_virtual_disk(__in std::wstring virtual_disk_path, __in bool read_only){

    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    VIRTUAL_DISK_ACCESS_MASK accessMask;
    ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
    PSECURITY_DESCRIPTOR sd;
    VIRTUAL_STORAGE_TYPE storageType;
    LPCTSTR extension;
    HANDLE vhdHandle;
    ATTACH_VIRTUAL_DISK_FLAG attachFlags;
    DWORD opStatus;

    vhdHandle = INVALID_HANDLE_VALUE;
    sd = NULL;

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    memset(&openParameters, 0, sizeof(openParameters));

    extension = ::PathFindExtension(virtual_disk_path.c_str());

    if (extension != NULL && _wcsicmp(extension, L".iso") == 0){
        //
        // ISO files can only be mounted read-only and using the V1 API.
        //

        if (read_only != TRUE)
        {
            opStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        accessMask = VIRTUAL_DISK_ACCESS_READ;
    }
    else if (is_windows_version_or_greater(6,2))
    {
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;
        accessMask = VIRTUAL_DISK_ACCESS_NONE;
    }
    else{
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        openParameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;
        accessMask = VIRTUAL_DISK_ACCESS_ALL;;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;
    }

    //
    // Open the VHD or ISO.
    //
    // OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
    //

    opStatus = OpenVirtualDisk(
        &storageType,
        virtual_disk_path.c_str(),
        accessMask,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Create the world-RW SD.
    //

    if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(
        L"O:BAG:BAD:(A;;GA;;;WD)",
        SDDL_REVISION_1,
        &sd,
        NULL))
    {
        opStatus = ::GetLastError();
        goto Cleanup;
    }

    //
    // Attach the VHD/VHDX or ISO.
    //

    memset(&attachParameters, 0, sizeof(attachParameters));
    attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    //
    // A "Permanent" surface persists even when the handle is closed.
    //

    attachFlags = ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;

    if (read_only)
    {
        // ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY specifies a read-only mount.
        attachFlags |= ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY;
    }

    opStatus = AttachVirtualDisk(
        vhdHandle,
        sd,
        attachFlags,
        0,
        &attachParameters,
        NULL);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
Cleanup:

    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (sd != NULL)
    {
        LocalFree(sd);
        sd = NULL;
    }

    if (vhdHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vhdHandle);
    }
    return opStatus ;
}

DWORD win_vhdx_mgmt::detach_virtual_disk(__in std::wstring virtual_disk_path){

    VIRTUAL_STORAGE_TYPE storageType;
    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    VIRTUAL_DISK_ACCESS_MASK accessMask;
    LPCTSTR extension;
    HANDLE vhdHandle;
    DWORD opStatus;

    vhdHandle = INVALID_HANDLE_VALUE;

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    memset(&openParameters, 0, sizeof(openParameters));

    extension = ::PathFindExtension(virtual_disk_path.c_str());

    if (extension != NULL && _wcsicmp(extension, L".iso") == 0)
    {
        //
        // ISO files can only be opened using the V1 API.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        accessMask = VIRTUAL_DISK_ACCESS_READ;
    }
    else if (is_windows_version_or_greater(6, 2))
    {
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        // OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;
        accessMask = VIRTUAL_DISK_ACCESS_NONE;
    }
    else
    {
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        openParameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;
        accessMask = VIRTUAL_DISK_ACCESS_ALL;;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;
    }

    //
    // Open the VHD/VHDX or ISO.
    //
    //

    opStatus = OpenVirtualDisk(
        &storageType,
        virtual_disk_path.c_str(),
        accessMask,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Detach the VHD/VHDX/ISO.
    //
    // DETACH_VIRTUAL_DISK_FLAG_NONE is the only flag currently supported for detach.
    //

    opStatus = DetachVirtualDisk(
        vhdHandle,
        DETACH_VIRTUAL_DISK_FLAG_NONE,
        0);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

Cleanup:

    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (vhdHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vhdHandle);
    }

    return opStatus;
}

DWORD win_vhdx_mgmt::merge_virtual_disk(__in std::wstring leaf_path){

    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    MERGE_VIRTUAL_DISK_PARAMETERS mergeParameters;
    VIRTUAL_STORAGE_TYPE storageType;
    HANDLE vhdHandle;
    DWORD opStatus;

    vhdHandle = INVALID_HANDLE_VALUE;

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    //
    // Open the VHD.
    //
    // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
    // OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
    //

    memset(&openParameters, 0, sizeof(openParameters));
    openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;

    opStatus = OpenVirtualDisk(
        &storageType,
        leaf_path.c_str(),
        VIRTUAL_DISK_ACCESS_NONE,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }

    //
    // Perform the merge.
    //
    // MERGE_VIRTUAL_DISK_VERSION_2 allows merging of VHDs/VHDXs in use.
    // MERGE_VIRTUAL_DISK_FLAG_NONE is currently the only merge flag supported.
    //
    // DO NOT attempt to perform a live merge of a leaf (a)VHD or (a)VHDX of a VM as the
    // operation will not update the virtual machine configuration file.
    //

    memset(&mergeParameters, 0, sizeof(mergeParameters));
    mergeParameters.Version = MERGE_VIRTUAL_DISK_VERSION_2;

    // In this sample, the leaf is being merged so the source depth is 1.
    mergeParameters.Version2.MergeSourceDepth = 1;

    // In this sample, the leaf is being merged only to it's parent so the target depth is 2
    mergeParameters.Version2.MergeTargetDepth = 2;

    opStatus = MergeVirtualDisk(
        vhdHandle,
        MERGE_VIRTUAL_DISK_FLAG_NONE,
        &mergeParameters,
        NULL);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }

Cleanup:

    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (vhdHandle != INVALID_HANDLE_VALUE){
        CloseHandle(vhdHandle);
    }

    return opStatus;
}

DWORD win_vhdx_mgmt::get_virtual_disk_physical_path(__in std::wstring virtual_disk_path, __inout std::wstring& physical_disk_path)
{
    VIRTUAL_STORAGE_TYPE storageType;
    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    VIRTUAL_DISK_ACCESS_MASK accessMask;
    LPCTSTR extension;
    HANDLE vhdHandle;
    DWORD opStatus;

    vhdHandle = INVALID_HANDLE_VALUE;

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    memset(&openParameters, 0, sizeof(openParameters));

    extension = ::PathFindExtension(virtual_disk_path.c_str());

    if (extension != NULL && _wcsicmp(extension, L".iso") == 0)
    {
        //
        // ISO files can only be opened using the V1 API.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        accessMask = VIRTUAL_DISK_ACCESS_READ;
    }
    else if(is_windows_version_or_greater(6, 2))
    {
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        //
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;
        accessMask = VIRTUAL_DISK_ACCESS_NONE;
    }
    else
    {
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        openParameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;
        accessMask = VIRTUAL_DISK_ACCESS_ALL;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;
    }

    //
    // Open the VHD/VHDX or ISO.
    //
    //

    opStatus = OpenVirtualDisk(
        &storageType,
        virtual_disk_path.c_str(),
        accessMask,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    ULONG diskPathSizeInBytes = 0;
    PWSTR diskPath = NULL;

    opStatus = GetVirtualDiskPhysicalPath(
        vhdHandle,
        &diskPathSizeInBytes,
        diskPath);

    if (opStatus != ERROR_SUCCESS)
    {
        if (opStatus != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Cleanup;
        }

        diskPath = (PWSTR)malloc(diskPathSizeInBytes);
        if (diskPath == NULL)
        {
            opStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        opStatus = GetVirtualDiskPhysicalPath(
            vhdHandle,
            &diskPathSizeInBytes,
            diskPath);

        if (opStatus != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
        physical_disk_path = diskPath;
        free(diskPath);
    }

Cleanup:
    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (vhdHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vhdHandle);
    }

    return opStatus;
}

DWORD win_vhdx_mgmt::get_virtual_disk_information(__in std::wstring virtual_disk_path, __inout vhd_disk_info &disk){

    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    VIRTUAL_STORAGE_TYPE storageType;
    PGET_VIRTUAL_DISK_INFO diskInfo;
    VIRTUAL_DISK_ACCESS_MASK accessMask;
    LPCTSTR extension;
    ULONG diskInfoSize;
    DWORD opStatus;
    HANDLE vhdHandle;
    HRESULT stringLengthResult;
    LPCWSTR parentPath;
    size_t parentPathSize;
    size_t parentPathSizeRemaining;

    vhdHandle = INVALID_HANDLE_VALUE;
    diskInfo = NULL;
    diskInfoSize = sizeof(GET_VIRTUAL_DISK_INFO);

    diskInfo = (PGET_VIRTUAL_DISK_INFO)malloc(diskInfoSize);
    if (diskInfo == NULL)
    {
        opStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    memset(&openParameters, 0, sizeof(openParameters));
    //
    // Open the VHD for query access.
    //
    // A "GetInfoOnly" handle is a handle that can only be used to query properties or
    // metadata.
    //
    // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
    // OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS indicates the parent chain should not be opened.
    //

    extension = ::PathFindExtension(virtual_disk_path.c_str());

    if (extension != NULL && _wcsicmp(extension, L".iso") == 0)
    {
        //
        // ISO files can only be opened using the V1 API.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        accessMask = VIRTUAL_DISK_ACCESS_READ;
    }
    else if (is_windows_version_or_greater(6, 2))
    {
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        //

        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;
        accessMask = VIRTUAL_DISK_ACCESS_NONE;
    }
    else
    {
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
        openParameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;
        accessMask = VIRTUAL_DISK_ACCESS_ALL;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;
    }

    //
    // Open the VHD/VHDX or ISO.
    //
    //

    opStatus = OpenVirtualDisk(
        &storageType,
        virtual_disk_path.c_str(),
        accessMask,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }

    //
    // Get the VHD/VHDX type.
    //

    diskInfo->Version = GET_VIRTUAL_DISK_INFO_PROVIDER_SUBTYPE;

    opStatus = GetVirtualDiskInformation(
        vhdHandle,
        &diskInfoSize,
        diskInfo,
        NULL);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }
    disk.vhd_file = virtual_disk_path;
    disk.drive_type = diskInfo->ProviderSubtype;

    //
    // Get the VHD/VHDX format.
    //

    diskInfo->Version = GET_VIRTUAL_DISK_INFO_VIRTUAL_STORAGE_TYPE;

    opStatus = GetVirtualDiskInformation(
        vhdHandle,
        &diskInfoSize,
        diskInfo,
        NULL);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }

    disk.drive_format = diskInfo->VirtualStorageType.DeviceId;

    //
    // Get the VHD/VHDX virtual disk size.
    //

    diskInfo->Version = GET_VIRTUAL_DISK_INFO_SIZE;

    opStatus = GetVirtualDiskInformation(
        vhdHandle,
        &diskInfoSize,
        diskInfo,
        NULL);

    if (opStatus != ERROR_SUCCESS){
        goto Cleanup;
    }

    disk.physical_size = diskInfo->Size.PhysicalSize;
    disk.virtual_size = diskInfo->Size.VirtualSize;
    disk.logical_sector_size = diskInfo->Size.SectorSize;
    disk.block_size = diskInfo->Size.BlockSize;


    if (is_windows_version_or_greater(6, 2)){

        diskInfo->Version = GET_VIRTUAL_DISK_INFO_VHD_PHYSICAL_SECTOR_SIZE;

        opStatus = GetVirtualDiskInformation(
            vhdHandle,
            &diskInfoSize,
            diskInfo,
            NULL);

        if (opStatus != ERROR_SUCCESS){
            goto Cleanup;
        }
        disk.physical_sector_size = diskInfo->VhdPhysicalSectorSize;
    }
    else{
        disk.physical_sector_size = disk.logical_sector_size;
    }
    //
    // Get the virtual disk ID.
    //

    diskInfo->Version = GET_VIRTUAL_DISK_INFO_IDENTIFIER;

    opStatus = GetVirtualDiskInformation(
        vhdHandle,
        &diskInfoSize,
        diskInfo,
        NULL);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    disk.identifier = diskInfo->Identifier;

    //
    // Get the VHD parent path.
    //

    if (disk.drive_type == 0x4)
    {
        diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_LOCATION;

        opStatus = GetVirtualDiskInformation(
            vhdHandle,
            &diskInfoSize,
            diskInfo,
            NULL);

        if (opStatus != ERROR_SUCCESS)
        {
            if (opStatus != ERROR_INSUFFICIENT_BUFFER)
            {
                goto Cleanup;
            }

            free(diskInfo);

            diskInfo = (PGET_VIRTUAL_DISK_INFO)malloc(diskInfoSize);
            if (diskInfo == NULL)
            {
                opStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_LOCATION;

            opStatus = GetVirtualDiskInformation(
                vhdHandle,
                &diskInfoSize,
                diskInfo,
                NULL);

            if (opStatus != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }

        parentPath = diskInfo->ParentLocation.ParentLocationBuffer;
        parentPathSizeRemaining = diskInfoSize - FIELD_OFFSET(GET_VIRTUAL_DISK_INFO,
            ParentLocation.ParentLocationBuffer);

        if (diskInfo->ParentLocation.ParentResolved)
        {
            disk.parents.push_back(parentPath);
        }
        else
        {
            //
            // If the parent is not resolved, the buffer is a MULTI_SZ
            //

            wprintf(L"parentPath:\n");

            while ((parentPathSizeRemaining >= sizeof(parentPath[0])) && (*parentPath != 0))
            {
                stringLengthResult = StringCbLengthW(
                    parentPath,
                    parentPathSizeRemaining,
                    &parentPathSize);

                if (FAILED(stringLengthResult))
                {
                    goto Cleanup;
                }
                disk.parents.push_back(parentPath);
                parentPathSize += sizeof(parentPath[0]);
                parentPath = parentPath + (parentPathSize / sizeof(parentPath[0]));
                parentPathSizeRemaining -= parentPathSize;
            }
        }

        //
        // Get parent ID.
        //

        diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_IDENTIFIER;

        opStatus = GetVirtualDiskInformation(
            vhdHandle,
            &diskInfoSize,
            diskInfo,
            NULL);

        if (opStatus != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
        disk.parent_identifier = diskInfo->ParentIdentifier;
    }

Cleanup:
    
    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (vhdHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vhdHandle);
    }

    if (diskInfo != NULL)
    {
        free(diskInfo);
    }

    return opStatus;
}

DWORD win_vhdx_mgmt::get_attached_physical_disk_number(__in std::wstring virtual_disk_path, __inout DWORD& disk_number){

    DWORD opStatus = ERROR_SUCCESS;
    std::wstring physical_path;
    if (ERROR_SUCCESS == (opStatus = get_virtual_disk_physical_path(virtual_disk_path, physical_path))){
        transform(physical_path.begin(), physical_path.end(), physical_path.begin(), towupper);
        _stscanf(physical_path.c_str(), L"\\\\.\\PHYSICALDRIVE%d", &disk_number);
    }
    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }
    return opStatus;
}

universal_disk_rw::ptr win_vhdx_mgmt::open_virtual_disk_for_rw(__in std::wstring virtual_disk_path, __in bool read_only){

    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
    ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
    VIRTUAL_STORAGE_TYPE storageType;
    LPCTSTR extension;
    HANDLE vhdHandle;
    ATTACH_VIRTUAL_DISK_FLAG attachFlags;
    DWORD opStatus;
   
    vhdHandle = INVALID_HANDLE_VALUE;

    //
    // Specify UNKNOWN for both device and vendor so the system will use the
    // file extension to determine the correct VHD format.
    //

    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    memset(&openParameters, 0, sizeof(openParameters));

    extension = ::PathFindExtension(virtual_disk_path.c_str());

    if ((extension != NULL && _wcsicmp(extension, L".iso") == 0) || !is_windows_version_or_greater(6, 2)){
        return NULL;
    }
    else{
        //
        // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
        //
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;
    }

    //
    // Open the VHD or ISO.
    //
    // OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
    //

    opStatus = OpenVirtualDisk(
        &storageType,
        virtual_disk_path.c_str(),
        VIRTUAL_DISK_ACCESS_NONE,
        OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters,
        &vhdHandle);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    memset(&attachParameters, 0, sizeof(attachParameters));
    attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    //
    // A "Permanent" surface persists even when the handle is closed.
    //

    attachFlags = ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST;
   
    if (read_only)
    {
        // ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY specifies a read-only mount.
        attachFlags |= ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY;
    }

    opStatus = AttachVirtualDisk(
        vhdHandle,
        NULL,
        attachFlags,
        0,
        &attachParameters,
        NULL);

    if (opStatus != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    return universal_disk_rw::ptr(new win_vhdx_file_rw(vhdHandle, virtual_disk_path));

Cleanup:

    if (opStatus == ERROR_SUCCESS)
    {
        LOG(LOG_LEVEL_INFO, _T("success"));
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
    }

    if (vhdHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(vhdHandle);       
    }
    return NULL;
}

typedef LONG(WINAPI *LPFN_GetAllAttachedVirtualDiskPhysicalPaths)(
    __inout                                     PULONG PathsBufferSizeInBytes,
    __out_bcount(*PathsBufferSizeInBytes)		PWSTR  PathsBuffer
    );

DWORD win_vhdx_mgmt::get_all_attached_virtual_disk_physical_paths(std::vector<std::wstring> &paths){

    LPWSTR  pathList;
    LPWSTR  pathListBuffer;
    size_t  nextPathListSize;
    DWORD   opStatus;
    ULONG   pathListSizeInBytes;
    size_t  pathListSizeRemaining;
    HRESULT stringLengthResult;

    pathListBuffer = NULL;
    pathListSizeInBytes = 0;
    LPFN_GetAllAttachedVirtualDiskPhysicalPaths fnGetAllAttachedVirtualDiskPhysicalPaths = NULL;
    fnGetAllAttachedVirtualDiskPhysicalPaths = (LPFN_GetAllAttachedVirtualDiskPhysicalPaths)GetProcAddress(
        GetModuleHandle(TEXT("VirtDisk")), "GetAllAttachedVirtualDiskPhysicalPaths");
    if (NULL == fnGetAllAttachedVirtualDiskPhysicalPaths){
        opStatus = ERROR_FUNCTION_NOT_CALLED;
    }
    else{

        do
        {
            //
            // Determine the size actually required.
            //
            opStatus = fnGetAllAttachedVirtualDiskPhysicalPaths(&pathListSizeInBytes,
                pathListBuffer);
            if (opStatus == ERROR_SUCCESS)
            {
                break;
            }

            if (opStatus != ERROR_INSUFFICIENT_BUFFER)
            {
                goto Cleanup;
            }

            if (pathListBuffer != NULL)
            {
                free(pathListBuffer);
            }

            //
            // Allocate a large enough buffer.
            //

            pathListBuffer = (LPWSTR)malloc(pathListSizeInBytes);
            if (pathListBuffer == NULL)
            {
                opStatus = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }

        } while (opStatus == ERROR_INSUFFICIENT_BUFFER);

        if (pathListBuffer == NULL || pathListBuffer[0] == NULL)
        {
            // There are no loopback mounted virtual disks.
            LOG(LOG_LEVEL_INFO, L"There are no loopback mounted virtual disks.");
            goto Cleanup;
        }

        //
        // The pathList is a MULTI_SZ.
        //

        pathList = pathListBuffer;
        pathListSizeRemaining = (size_t)pathListSizeInBytes;

        while ((pathListSizeRemaining >= sizeof(pathList[0])) && (*pathList != 0))
        {
            stringLengthResult = StringCbLengthW(pathList,
                pathListSizeRemaining,
                &nextPathListSize);

            if (FAILED(stringLengthResult))
            {
                goto Cleanup;
            }

            paths.push_back(pathList);

            nextPathListSize += sizeof(pathList[0]);
            pathList = pathList + (nextPathListSize / sizeof(pathList[0]));
            pathListSizeRemaining -= nextPathListSize;
        }

    Cleanup:

        if (opStatus == ERROR_SUCCESS)
        {
            LOG(LOG_LEVEL_INFO, _T("success"));
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, _T("error = %u"), opStatus);
        }

        if (pathListBuffer != NULL)
        {
            free(pathListBuffer);
        }
    }
    return opStatus;
}

DWORD win_vhdx_mgmt::get_all_attached_virtual_disk_physical_paths(std::vector<DWORD> &disks){
    std::vector<std::wstring> paths;
    DWORD   opStatus = get_all_attached_virtual_disk_physical_paths(paths);
    if (opStatus == ERROR_SUCCESS)
    {
        foreach(std::wstring &path, paths)
        {
            DWORD disk_number;
            if (ERROR_SUCCESS == get_attached_physical_disk_number(path, disk_number) )
                disks.push_back(disk_number);
        }
    }
    return opStatus;
}