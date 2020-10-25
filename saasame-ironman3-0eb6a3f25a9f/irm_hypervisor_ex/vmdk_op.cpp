#include "stdafx.h"
#include "vmware_ex.h"

using namespace macho;
using namespace boost;
using namespace mwdc::ironman::hypervisor_ex;

#ifdef VIXMNTAPI
BOOL vmware_vmdk_mgmt::create_vmdk_file(vmware_portal_ex::ptr portal_ex, std::wstring image_file, disk_convert_settings create_flags, uint64_t number_of_sectors_size)
{
    FUN_TRACE;

    if (portal_ex)
    {
        BOOL bResult = portal_ex->create_vmdk_file_local(image_file, create_flags, number_of_sectors_size);
        return bResult;
    }
    else
        return FALSE;
}

BOOL vmware_vmdk_mgmt::create_vmdk_file(vmware_portal_ex *portal_ex, std::wstring image_file, disk_convert_settings create_flags, uint64_t number_of_sectors_size)
{
    FUN_TRACE;

    if (portal_ex)
        return portal_ex->create_vmdk_file_local(image_file, create_flags, number_of_sectors_size);
    else
        return FALSE;
}

universal_disk_rw::ptr vmware_vmdk_mgmt::open_vmdk_for_rw(vmware_portal_ex::ptr portal_ex, std::wstring image_file, bool read_only)
{
    FUN_TRACE;

    if (portal_ex)
        return portal_ex->open_vmdk_for_rw(image_file, read_only);
    return NULL;
}

universal_disk_rw::ptr vmware_vmdk_mgmt::open_vmdk_for_rw(vmware_portal_ex *portal_ex, std::wstring image_file, bool read_only)
{
    FUN_TRACE;
    if (portal_ex)
        return portal_ex->open_vmdk_for_rw(image_file, read_only);;

    return NULL;
}

#endif

VOID vmware_vmdk_mgmt::get_vm_vmdk_list(vmware_portal_ex::ptr portal_ex, std::wstring host, std::wstring username, std::wstring passwd, std::wstring machine_key, std::map<std::wstring, uint64>& vmdk_list)
{
    FUN_TRACE;

    if (portal_ex)
    {
        portal_ex->get_vmdk_list(host, username, passwd, machine_key, vmdk_list);
    }
}

universal_disk_rw::ptr vmware_vmdk_mgmt::open_vmdk_for_rw(vmware_portal_ex::ptr portal_ex, std::wstring host, std::wstring username, std::wstring passwd, std::wstring machine_key, std::wstring image_file, std::wstring snapshot_name, bool read_only)
{
    FUN_TRACE;

    if (portal_ex)
    {
        vmware_vixdisk_connection::ptr connect = portal_ex->get_vixdisk_connection(host, username, passwd, machine_key, snapshot_name, read_only);
        if (connect)
            return portal_ex->open_vmdk_for_rw(connect, image_file, read_only);;
    }
    return NULL;
}



universal_disk_rw::ptr vmware_vmdk_mgmt::open_vmdk_for_rw(vmware_portal_ex *portal_ex, std::wstring host, std::wstring username, std::wstring passwd, std::wstring machine_key, std::wstring image_file, std::wstring snapshot_name, bool read_only)
{
    FUN_TRACE;

    if (portal_ex)
    {
        vmware_vixdisk_connection::ptr connect = portal_ex->get_vixdisk_connection(host, username, passwd, machine_key, snapshot_name, read_only);
        if (connect)
            return portal_ex->open_vmdk_for_rw(connect, image_file, read_only);;
    }

    return NULL;
}

vmware_vmdk_file_rw::vmware_vmdk_file_rw() : _readonly(false)
{
}

vmware_vmdk_file_rw::~vmware_vmdk_file_rw()
{
    uint32 num_cleanedup = 0, num_remaining = 0;
   
    VixError vixerror = VixDiskLib_Close(_handle);

    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to close (%s) handle.", _virtual_disk_path.c_str());
    }
}

bool vmware_vmdk_file_rw::sector_read(uint64_t start, uint32_t number_of_sectors_to_read, LPVOID buffer, uint32_t& number_of_sectors_read)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    VixError vixerror = VIX_OK;
    number_of_sectors_read = 0;
    memset(buffer, 0, number_of_sectors_to_read * VIXDISKLIB_SECTOR_SIZE);
    vixerror = VixDiskLib_Read(_handle, start, number_of_sectors_to_read, (uint8 *)buffer);
    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Read image file %s failure, start sector: %lld, length sectors = %ld, error = %d", 
            _virtual_disk_path.c_str(), start, number_of_sectors_to_read, VIX_ERROR_CODE(vixerror));
        return false;
    }

    number_of_sectors_read = number_of_sectors_to_read;
    return true;
}

bool vmware_vmdk_file_rw::sector_write(uint64_t start, LPCVOID buffer, uint32_t number_of_sectors_to_write, uint32_t& number_of_sectors_written)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    VixError vixerror = VIX_OK;
    number_of_sectors_written = 0;
    vixerror = VixDiskLib_Write(_handle, start, number_of_sectors_to_write, (uint8 *)buffer);
    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Write image file %s failure, start sector: %lld, length sectors = %ld, error = %d",
            _virtual_disk_path.c_str(), start, number_of_sectors_to_write, VIX_ERROR_CODE(vixerror));
        return false;
    }

    number_of_sectors_written = number_of_sectors_to_write;
    return true;
}

bool vmware_vmdk_file_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read)
{
    bool result = true;
    macho::windows::auto_lock lock(_cs);
    VixError vixerror;

    if (buffer)
    {
        memset(buffer, 0, number_of_bytes_to_read);
        vixerror = VixDiskLib_Read(_handle, start / VIXDISKLIB_SECTOR_SIZE, number_of_bytes_to_read / VIXDISKLIB_SECTOR_SIZE, (uint8 *)buffer);
        if (VIX_SUCCEEDED(vixerror))
            number_of_bytes_read = number_of_bytes_to_read;
        else
        {
            number_of_bytes_read = 0;
            result = false;
        }
    }
    else
        result = false;

    return result;
}

bool vmware_vmdk_file_rw::write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written)
{
    uint32_t number_of_sectors_written = 0;
    bool result = false;
    if (result = sector_write(start / VIXDISKLIB_SECTOR_SIZE, buffer, number_of_bytes_to_write / VIXDISKLIB_SECTOR_SIZE, number_of_sectors_written)){
        number_of_bytes_written = number_of_sectors_written * VIXDISKLIB_SECTOR_SIZE;
    }
    return result;
}

bool vmware_vmdk_file_rw::write_meta_data(LPVOID key, LPCVOID value)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    VixError vixerror = VIX_OK;
    vixerror = VixDiskLib_WriteMetadata(_handle, (char *)key, (char *)value);
    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Write image meta %s information failure, error = %d.", 
            macho::stringutils::convert_utf8_to_unicode(std::string((char *)key)).c_str(), VIX_ERROR_CODE(vixerror));
        return false;
    }

    return true;
}

bool vmware_vmdk_file_rw::read_meta_data(LPVOID key, void **value)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    size_t required_len;
    VixError vixerror = VIX_OK;

    vixerror = VixDiskLib_ReadMetadata(_handle, (char *)key, NULL, 0, &required_len);
    if (VIX_ERROR_CODE(vixerror) != VIX_OK && VIX_ERROR_CODE(vixerror) != VIX_E_BUFFER_TOOSMALL)
    {
        LOG(LOG_LEVEL_ERROR, L"Calculate required buffer size for read remote image meta information, error = %d.", VIX_ERROR_CODE(vixerror));
        return false;
    }

    *value = realloc(NULL, required_len);
    if (*value == NULL)
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to allocate buffer for store image metadata information.", VIX_ERROR_CODE(vixerror));
        return false;
    }
    vixerror = VixDiskLib_ReadMetadata(_handle, (char *)key, (char *)*value, required_len, NULL);
    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Get remote image meta information failure, error = %d.", VIX_ERROR_CODE(vixerror));
        return false;
    }
    else
    {
        std::string keyval = std::string((char *)key) + "=" + std::string((char *)*value);
        LOG(LOG_LEVEL_DEBUG, L"%s", macho::stringutils::convert_utf8_to_unicode(keyval).c_str());
    }

    return true;
}

image_info::ptr vmware_vmdk_file_rw::get_image_info()
{
    FUN_TRACE;
    vmware_portal_ex::ptr portal_ex = vmware_portal_ex::ptr(new vmware_portal_ex());
    if (portal_ex)
    {
        image_info::ptr img_info = portal_ex->get_image_info_internal(_handle); 
        return img_info;
    }
    else
        return NULL;
}

vmware_vmdk_file_rw* vmware_vmdk_file_rw::connect(VixDiskLibConnection& connection, const std::wstring& virtual_disk_path, bool readonly)
{
    FUN_TRACE;
    VixError vixerror;
    if (virtual_disk_path.empty()){
        LOG(LOG_LEVEL_ERROR, L"Open image file failure => virtual_disk_path is empty");
    }
    else{
        vmware_vmdk_file_rw* rw(new vmware_vmdk_file_rw());
        if (rw){
            try
            {
                rw->_virtual_disk_path = virtual_disk_path;
                rw->_readonly = readonly;
                vixerror = VixDiskLib_Open(connection, macho::stringutils::convert_unicode_to_utf8(virtual_disk_path).c_str(), readonly ? VIXDISKLIB_FLAG_OPEN_READ_ONLY : VIXDISKLIB_FLAG_OPEN_UNBUFFERED, &rw->_handle);
            }
            catch (...)
            {
                vixerror = VIX_E_FAIL;
            }

            if (VIX_FAILED(vixerror))
            {
                delete rw;
                LOG(LOG_LEVEL_ERROR, L"Open image file failure => %s, error = %d.", vmware_portal_ex::get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
                return NULL;
            }
        }
        return rw;
    }
    return NULL;
}

vmware_vmdk_file_rw* vmware_vmdk_file_rw::connect(vmware_vixdisk_connection::ptr& connection, const std::wstring& virtual_disk_path, bool readonly)
{
    FUN_TRACE;
    vmware_vmdk_file_rw* rw = connect(connection->_connection, virtual_disk_path, readonly);
    if (rw)
        rw->_connection = connection;
    return rw;
}

vmware_vmdk_file_rw* vmware_vmdk_file_rw::rw_clone()
{
    if (_connection)
        return connect(_connection, _virtual_disk_path, _readonly);
    return NULL;
}

vmware_vixdisk_connection::~vmware_vixdisk_connection()
{
    uint32 num_cleanedup = 0, num_remaining = 0;
    if (_is_prepare_for_access)
        end_access();
    if (_connection != NULL ){
        VixDiskLib_Disconnect(_connection);
    }
    VixDiskLib_Cleanup(&_connect_params, &num_cleanedup, &num_remaining);
    free(_connect_params.vmxSpec);
    free(_connect_params.creds.uid.userName);
    free(_connect_params.creds.uid.password);
    free(_connect_params.serverName);
    free(_connect_params.thumbPrint);
}

void vmware_vixdisk_connection::prepare_for_access()
{
    VixError vixerror = VixDiskLib_PrepareForAccess(&_connect_params, "vmware_vixdisk_connection");
    if (VIX_SUCCEEDED(vixerror))
        _is_prepare_for_access = true;
    else
        LOG(LOG_LEVEL_WARNING, L"VixDiskLib_PrepareForAccess Failed (%d).", vixerror);
}

void vmware_vixdisk_connection::end_access()
{
    VixError vixerror = VixDiskLib_EndAccess(&_connect_params, "vmware_vixdisk_connection");
    if (!VIX_SUCCEEDED(vixerror))
        LOG(LOG_LEVEL_WARNING, L"VixDiskLib_EndAccess Failed (%d).", vixerror);
}
