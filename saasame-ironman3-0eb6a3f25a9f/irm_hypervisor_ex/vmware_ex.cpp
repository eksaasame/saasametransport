// irm_hyperviosr_ex.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//#include "delayimp.h"
#include "vmware.h"
#include "vmware_ex.h"
#include "..\irm_host_mgmt\vhdx.h"
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/err.h>

#ifdef _WIN32
# include <windows.h>
#endif
#ifdef PTHREADS
# include <pthread.h>
#endif
#ifdef _WIN32
#define MUTEX_TYPE            HANDLE
#define MUTEX_SETUP(x)        (x) = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x)      CloseHandle(x)
#define MUTEX_LOCK(x)         WaitForSingleObject((x), INFINITE)
#define MUTEX_UNLOCK(x)       ReleaseMutex(x)
#define THREAD_ID             GetCurrentThreadId()
#else
#define MUTEX_TYPE            pthread_mutex_t
#define MUTEX_SETUP(x)        pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x)      pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)         pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)       pthread_mutex_unlock(&(x))
#define THREAD_ID             pthread_self()
#endif

/* This array will store all of the mutexes available to OpenSSL. */
static MUTEX_TYPE *mutex_buf = NULL;

static void locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
    return ((unsigned long)THREAD_ID);
}

int ssl_thread_setup(void)
{
    int i;
    mutex_buf = (MUTEX_TYPE*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
    if (!mutex_buf)
        return 0;
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_SETUP(mutex_buf[i]);
    //    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    return 1;
}

int ssl_thread_cleanup(void)
{
    int i;
    if (!mutex_buf)
        return 0;
    //    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_CLEANUP(mutex_buf[i]);
    OPENSSL_free(mutex_buf);
    mutex_buf = NULL;
    return 1;
}

using namespace macho;
using namespace boost;
using namespace mwdc::ironman::hypervisor;
using namespace mwdc::ironman::hypervisor_ex;

#define BOOST_THROW_VMWARE_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( vmware_portal_ex::exception, no, message )
#define BOOST_THROW_VMWARE_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING(vmware_portal_ex::exception, message )

#define VIXDISKLIB_VERSION_MAJOR    6
#define VIXDISKLIB_VERSION_MINOR    0
#define CONFIG_FILE                 ".\\irm_vddk.conf"
//#define TRANSPORT_MODES             "file:nbdssl:san:hotadd:nbd"
#define TRANSPORT_MODES             "hotadd:san"
#define DATA_BUFFER_SIZE            4096
#define WAIT_INTERVAL_SECONDS       2 * 1000

macho::windows::mutex mutex_obj(L"irm_hv_ex");

void log_info(const char *fmt, va_list args)
{
    char i_msg[1024] = { 0 };

    vsprintf(i_msg, fmt, args);
    std::string msg(i_msg);

    LOG(LOG_LEVEL_INFO, std::wstring(msg.begin(), msg.end()).c_str());
}

void log_warn(const char *fmt, va_list args)
{
    char w_msg[1024] = { 0 };

    vsprintf(w_msg, fmt, args);
    std::string msg(w_msg);

    LOG(LOG_LEVEL_WARNING, std::wstring(msg.begin(), msg.end()).c_str());
}

void log_error(const char *fmt, va_list args)
{
    char e_msg[1024] = { 0 };

    vsprintf(e_msg, fmt, args);
    std::string msg(e_msg);

    LOG(LOG_LEVEL_ERROR, std::wstring(msg.begin(), msg.end()).c_str());
}
//
//static Bool clone_progress_func(void *progress_data, int percent_completed) 
//{
//    if (percent_completed % 10 == 0)
//    {
//        LOG(LOG_LEVEL_DEBUG, L"[%u]Cloning: %d.", GetCurrentThreadId(), percent_completed);
//
//        if (_isatty(_fileno(stdout)))
//        {
//            std::wcout << L"Progress percentage: " + std::to_wstring(percent_completed) << L"% Done." << "\r";
//            if (percent_completed == 100)
//                std::wcout << std::endl;
//        }
//    }
//
//    return TRUE;
//}

#ifdef VIXMNTAPI
mounted_memory_repo::~mounted_memory_repo()
{
    FUN_TRACE;

    if (vol_handles != NULL)
    {
        VixMntapi_FreeVolumeHandles(vol_handles);
        VixMntapi_CloseDiskSet(diskset_handle);
        VixDiskLib_Close(disk_handle);
    }
}
#endif

vmware_portal_::vmware_portal_(){}

vmware_portal_::~vmware_portal_(){}

vmware_host::vtr vmware_portal_::get_hosts(const std::wstring &host_key){
    if (portal)
        return portal->get_hosts(host_key);
    return vmware_host::vtr();
}

vmware_host::ptr vmware_portal_::get_host(const std::wstring &host_key){
    if (portal)
        return  portal->get_host(host_key);
    return vmware_host::ptr();
}

vmware_datacenter::vtr vmware_portal_::get_datacenters(const std::wstring &name){
    if (portal)
        return portal->get_datacenters(name);
    return vmware_datacenter::vtr();
}

vmware_datacenter::ptr vmware_portal_::get_datacenter(const std::wstring &name){
    if (portal)
        return  portal->get_datacenter(name);
    return vmware_datacenter::ptr();
}

vmware_cluster::vtr vmware_portal_::get_clusters(const std::wstring &cluster_key){
    if (portal)
        return portal->get_clusters(cluster_key);
    return vmware_cluster::vtr();
}

vmware_cluster::ptr vmware_portal_::get_cluster(const std::wstring &cluster_key){
    if (portal)
        return portal->get_cluster(cluster_key);
    return vmware_cluster::ptr();
}

vmware_virtual_machine::vtr vmware_portal_::get_virtual_machines(const std::wstring &host_key, const std::wstring &machine_key){
    if (portal)
        return portal->get_virtual_machines(host_key, machine_key);
    return vmware_virtual_machine::vtr();
}

vmware_virtual_machine::ptr vmware_portal_::get_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    if (portal)
        return portal->get_virtual_machine(machine_key, host_key);
    return vmware_virtual_machine::ptr();
}

vmware_vm_task_info::vtr vmware_portal_::get_virtual_machine_tasks(const std::wstring &machine_key){
    if (portal)
        return portal->get_virtual_machine_tasks(machine_key);
    return vmware_vm_task_info::vtr();
}

vmware_virtual_machine::ptr vmware_portal_::create_virtual_machine(const vmware_virtual_machine_config_spec &config_spec){
    if (portal)
        return portal->create_virtual_machine(config_spec);
    return NULL;
}

vmware_virtual_machine::ptr vmware_portal_::modify_virtual_machine(const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec){
    if (portal)
        return portal->modify_virtual_machine(machine_key, config_spec);
    return NULL;
}

vmware_virtual_machine::ptr vmware_portal_::clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec){
    if (portal)
        return portal->clone_virtual_machine(machine_key, clone_spec);
    return NULL;
}

vmware_virtual_machine::ptr vmware_portal_::clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot){
    if (portal)
        return portal->clone_virtual_machine(machine_key, clone_spec, slot);
    return NULL;
}

bool vmware_portal_::delete_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    if (portal)
        return portal->delete_virtual_machine(machine_key, host_key);
    return false;
}

bool vmware_portal_::power_on_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    if (portal)
        return portal->power_on_virtual_machine(machine_key, host_key);
    return false;
}

bool vmware_portal_::unregister_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    if (portal)
        return portal->unregister_virtual_machine(machine_key, host_key);
    return false;
}

vmware_virtual_machine::ptr vmware_portal_::add_existing_virtual_machine(const vmware_add_existing_virtual_machine_spec &spec){
    if (portal)
        return portal->add_existing_virtual_machine(spec);
    return NULL;
}

bool vmware_portal_::power_off_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key){
    if (portal)
        return portal->power_off_virtual_machine(machine_key, host_key);
    return false;
}

int vmware_portal_::create_virtual_machine_snapshot(const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot){
    if (portal)
        return portal->create_virtual_machine_snapshot(machine_key, snapshot_request_parm, slot);
    return ERROR_NOT_READY;
}

int vmware_portal_::remove_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot){
    if (portal)
        return portal->remove_virtual_machine_snapshot(machine_key, snapshot_request_parm, slot);
    return ERROR_NOT_READY;
}

int vmware_portal_::revert_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot){
    if (portal)
        return portal->revert_virtual_machine_snapshot(machine_key, snapshot_request_parm, slot);
    return ERROR_NOT_READY;
}

bool vmware_portal_::enable_change_block_tracking(const std::wstring &machine_key){
    if (portal)
        return portal->enable_change_block_tracking(machine_key);
    return false;
}

bool vmware_portal_::disable_change_block_tracking(const std::wstring &machine_key){
    if (portal)
        return portal->disable_change_block_tracking(machine_key);
    return false;
}

bool vmware_portal_::mount_vm_tools(const std::wstring &machine_key){
    if (portal)
        return portal->mount_vm_tools(machine_key);
    return false;
}

void vmware_portal_::reset_flags(){
    if (portal)
        portal->reset_flags();
}
 
int	vmware_portal_::get_last_error_code(){
    if (portal)
        return portal->get_last_error_code();
    return 0;
}

std::wstring vmware_portal_::get_last_error_message(){
    if (portal)
        return portal->get_last_error_message();
    return L"";
}

vmdk_changed_areas vmware_portal_::get_vmdk_changed_areas(const std::wstring &vm_mor_item, const std::wstring &snapshot_mor_item, const std::wstring &device_key, const std::wstring &changeid, const LONG64 start_offset, const LONG64 disk_size){
    if (portal){       
        return portal->get_vmdk_changed_areas(vm_mor_item, snapshot_mor_item, device_key, changeid, start_offset, disk_size);
    }
    return vmdk_changed_areas();
}

vmware_portal_::ptr vmware_portal_::connect(std::wstring uri, std::wstring user, std::wstring password){
    vmware_portal_::ptr p = vmware_portal_::ptr(new vmware_portal_());
    p->portal = vmware_portal::connect(uri, user, password);
    if (p->portal)
        return p;
    return NULL;
}

bool vmware_portal_::login_verify(std::wstring uri, std::wstring user, std::wstring password){
    return vmware_portal::login_verify(uri, user, password);
}

vmware_snapshot_disk_info::map  vmware_portal_::get_snapshot_info(std::wstring& snapshot_mof, vmware_snapshot_disk_info::vtr& snapshot_disk_infos)
{
    if (portal)
        return portal->get_snapshot_info(snapshot_mof, snapshot_disk_infos);
    return vmware_snapshot_disk_info::map();
}

bool vmware_portal_::interrupt(bool is_cancel){
    if (portal)
        return portal->interrupt(is_cancel);
    return false;
}

hv_connection_type vmware_portal_::get_connection_type(vmware_virtual_center &virtual_center){
    if (portal)
        return portal->get_connection_type(virtual_center);
    return hv_connection_type::HV_CONNECTION_TYPE_UNKNOWN;
}

key_map vmware_portal_::get_all_virtual_machines(){
    if (portal)
        return portal->get_all_virtual_machines();
    return key_map();
}

// This is the constructor of a class that has been exported.
// see irm_hyperviosr_ex.h for the class definition
bool vmware_portal_ex::_init_vixlib = false;
int32 vmware_portal_ex::_vixlib_ref_count = 0;

bool vmware_portal_ex::vixdisk_init()
{
    VixError vixerror = VIX_OK;

    macho::windows::auto_lock lock(mutex_obj);

    LOG(LOG_LEVEL_RECORD, L"Thread id: %ld => init portal_ex.", GetCurrentThreadId());
    ssl_thread_setup();
    if (!_init_vixlib)
    {
        std::string lib_dir = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
        std::string conf_path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory() + _T(CONFIG_FILE));

        vixerror = VixDiskLib_InitEx(VIXDISKLIB_VERSION_MAJOR, VIXDISKLIB_VERSION_MINOR, &log_info, &log_warn, &log_error, lib_dir.c_str(), conf_path.c_str());
        if (VIX_SUCCEEDED(vixerror))
        {
#ifdef VIXMNTAPI
            vixerror = VixMntapi_Init(VIXMNTAPI_MAJOR_VERSION, VIXMNTAPI_MINOR_VERSION, &log_info, &log_warn, &log_error, lib_dir.c_str(), conf_path.c_str());
            if (VIX_FAILED(vixerror))
            {
                VixDiskLib_Exit();
            }
            else
#endif
            {
                _init_vixlib = true;
            }
        }
    }

    if (VIX_SUCCEEDED(vixerror))
    {
        _vixlib_ref_count++;
        return true;
    }
    else
        return false;
}

void vmware_portal_ex::vixdisk_exit()
{
    macho::windows::auto_lock lock(mutex_obj);
    ssl_thread_cleanup();
    if (_vixlib_ref_count > 0)
        _vixlib_ref_count--;

    if (_init_vixlib && _vixlib_ref_count == 0)
    {
        try
        {
#ifdef VIXMNTAPI
            VixMntapi_Exit();
#endif
            VixDiskLib_Exit();

#if 0
            BOOL test_return = __FUnloadDelayLoadedDLL2("vixMntapi.dll");
            if (test_return)
                LOG(LOG_LEVEL_RECORD, L"vixMntapi.dll was unloaded");
            test_return = __FUnloadDelayLoadedDLL2("vixDiskLib.dll");
            if (test_return)
                LOG(LOG_LEVEL_RECORD, L"vixDiskLib.dll was unloaded")
            LOG(LOG_LEVEL_RECORD, L"VMware portal_ex/VIX released.");
#endif
        }
        catch (...){}

        _init_vixlib = false;
    }
}
 
//vmware_portal_ex::vmware_portal_ex() : _mounted_repo(NULL), _is_interrupted(false), _portal(NULL)
vmware_portal_ex::vmware_portal_ex() : _is_interrupted(false)
{
    FUN_TRACE;
#ifdef VIXMNTAPI
    VixError vixerror = VIX_OK;

    _connect = { 0 };
    _connect_params = { 0 };

    //pre-create a local connection for local use purpose and also verify vixdisklib and vixmntapi are initiated without problem
    vixerror = VixDiskLib_ConnectEx(&_connect_params, false, NULL, NULL, &_connect);
        
    if (VIX_SUCCEEDED(vixerror))
    {
        _init_status = true;
        //_mounted_repo = new std::map<std::wstring, mounted_memory_repo::ptr>();
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"%s, error = %d.", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
        BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
    }
#else
    _init_status = true;
#endif
}

vmware_portal_ex::~vmware_portal_ex()
{
    FUN_TRACE;
#ifdef VIXMNTAPI
    uint32 num_cleanedup;
    uint32 num_remaining;

    if (_init_status)
    {
        _init_status = false;
        //delete _mounted_repo;
        foreach(mounted_memory_repo::map::value_type &item, _mounted_repo)
        {
            item.second = NULL;
        }

        VixDiskLib_Disconnect(_connect);
        VixDiskLib_Cleanup(&_connect_params, &num_cleanedup, &num_remaining);
    }
#endif
    _portal.clear();
}

void vmware_portal_ex::set_log(std::wstring log_file, macho::TRACE_LOG_LEVEL level)
{
    macho::set_log_level(level);
    macho::set_log_file(log_file);
}

std::wstring vmware_portal_ex::get_native_error(VixError& vixerror)
{
    char *err_msg = NULL;

    err_msg = VixDiskLib_GetErrorText(vixerror, NULL);
    std::string str(err_msg);
    VixDiskLib_FreeErrorText(err_msg);

    return macho::stringutils::convert_utf8_to_unicode(str);
}

std::wstring vmware_portal_ex::get_free_drive_letter()
{
    DWORD bitmask = GetLogicalDrives();
    char drv_letter[3] = { 0 };

    for (int i = 3; i < 26; i++)
    {
        if ((bitmask & (1 << i)) == 0)
        {
            drv_letter[0] = 'A' + i;
            drv_letter[1] = ':';
            std::string drive_letter(drv_letter);

            return std::wstring(drive_letter.begin(), drive_letter.end());
        }
    }

    return std::wstring(L"");
}

std::wstring vmware_portal_ex::get_last_error_string_win()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::wstring(L"No error message has been recorded");

    LPSTR message_buffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message_buffer, 0, NULL);

    std::string message(message_buffer, size);

    //Free the buffer.
    LocalFree(message_buffer);

    return std::wstring(message.begin(), message.end());
}

void vmware_portal_ex::prepare_connection_param(connect_params *connect_params_req, VixDiskLibConnectParams *connect_params)
{
    FUN_TRACE;

    if (connect_params_req->ip.empty() || connect_params_req->uname.empty() || connect_params_req->passwd.empty() || connect_params_req->vmx_spec.empty())
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid arguments");
        BOOST_THROW_VMWARE_EXCEPTION(ERROR_INVALID_PARAMETER, L"Invalid arguments");
    }
    else
    {
        get_ssl_thumbprint(connect_params_req->ip, connect_params_req->portal_ssl_port, connect_params_req->thumbprint);
        
        if (!connect_params_req->thumbprint.empty())
        {
            connect_params->credType = VIXDISKLIB_CRED_UID;
            connect_params->serverName = _strdup(macho::stringutils::convert_unicode_to_utf8(connect_params_req->ip).c_str());
            //connect_params->port = connect_params_req->portal_port;
            connect_params->creds.uid.userName = _strdup(macho::stringutils::convert_unicode_to_utf8(connect_params_req->uname).c_str());
            connect_params->creds.uid.password = _strdup(macho::stringutils::convert_unicode_to_utf8(connect_params_req->passwd).c_str());
            connect_params->vmxSpec = _strdup(macho::stringutils::convert_unicode_to_utf8(connect_params_req->vmx_spec).c_str());
            connect_params->thumbPrint = _strdup(connect_params_req->thumbprint.c_str());
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"Invalid thumbprint data returned.");
            BOOST_THROW_VMWARE_EXCEPTION(ERROR_INVALID_DATA, L"Invalid thumbprint data returned.");
        }
    }
}

void vmware_portal_ex::get_snapshot_internal_from_list(vmware_virtual_machine_snapshots::vtr& snapshot_list, const std::wstring& snapshot_name, snapshot_internal_info& snapshot_internal)
{
    FUN_TRACE;

    if (snapshot_list.size() > 0 && !snapshot_name.empty())
    {
        for (vmware_virtual_machine_snapshots::vtr::iterator it = snapshot_list.begin(); it != snapshot_list.end(); ++it)
        {
            vmware_virtual_machine_snapshots::ptr obj = (vmware_virtual_machine_snapshots::ptr)(*it);

            if (obj->name == snapshot_name)
            {
                snapshot_internal.ref_item = obj->snapshot_mor_item;
                snapshot_internal.id = obj->id;
                return;
            }

            if (obj->child_snapshot_list.size() > 0)
            {
                get_snapshot_internal_from_list(obj->child_snapshot_list, snapshot_name, snapshot_internal);
            }
        }
    }
    else
    {
        std::wstring msg = L"Query snapshot managed object reference item failure";
        LOG(LOG_LEVEL_ERROR, L"%s, error %d", msg.c_str(), ERROR_INVALID_PARAMETER);
    }

    return;
}

DWORD vmware_portal_ex::get_vmxspec(const std::wstring& host, const std::wstring& uname, const std::wstring& passwd, const std::wstring& machine_key, std::wstring& vmxspec)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_portal::ptr portal = NULL;
    vmware_virtual_machine::ptr vm = NULL;

    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
    portal = vmware_portal::connect(uri, uname, passwd);

    if (portal)
    {
        vm = portal->get_virtual_machine(machine_key);
        if (vm != NULL)
        {
            if (!boost::starts_with(vm->vm_mor_item, L"vm-"))
                vmxspec = vm->vmxspec; // direct to esx server
            else
                vmxspec = L"moref=" + vm->vm_mor_item; // via vcenter server
        }
        else
        {
            LOG(LOG_LEVEL_WARNING, L"The specific VM doesn't existing, failed to get vm spec information.");
            dwRtn = S_FALSE;
        }
    }
    else
        dwRtn = S_FALSE;

    return dwRtn;
}

image_info::ptr vmware_portal_ex::get_image_info_remote(const std::wstring& host, const std::wstring& uname, const std::wstring& passwd, const std::wstring& machine_key, const std::wstring& image_name)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    VixDiskLibConnection remote_conn = { 0 };
    VixDiskLibConnectParams conn_params = { 0 };
    VixDiskLibHandle disk_handle = { 0 };
    image_info::ptr img_info = NULL;
    connect_params conn_params_req;
    std::wstring vmxspec(L"");

    if (host.empty() || uname.empty() || passwd.empty() || machine_key.empty() || image_name.empty())
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid parameters input.");
        return NULL;
    }

    if (get_vmxspec(host, uname, passwd, machine_key, vmxspec) != S_OK)
    {
        return NULL;
    }

    conn_params_req.ip = host;
    conn_params_req.passwd = passwd;
    conn_params_req.uname = uname;
    conn_params_req.vmx_spec = vmxspec;

    prepare_connection_param(&conn_params_req, &conn_params);

    vixerror = VixDiskLib_ConnectEx(&conn_params, true, NULL, "hotadd:san", &remote_conn); // bug, need snapshot moref
    if (VIX_FAILED(vixerror))
    {
        uint32 num_cleanedup = 0;
        uint32 num_remaining = 0;

        LOG(LOG_LEVEL_ERROR, L"Connect to %s failure, reason: %s, error = %d.", host.c_str(), get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
        VixDiskLib_Cleanup(&conn_params, &num_cleanedup, &num_remaining);
        return NULL;
    }

    vixerror = VixDiskLib_Open(remote_conn, macho::stringutils::convert_unicode_to_utf8(image_name).c_str(), VIXDISKLIB_FLAG_OPEN_READ_ONLY, &disk_handle);
    if (VIX_SUCCEEDED(vixerror))
    {
        img_info = get_image_info_internal(disk_handle);
        VixDiskLib_Close(disk_handle);
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Get disk handle failure: %s.", get_native_error(vixerror).c_str());
    }

    VixDiskLib_Disconnect(remote_conn);

    return img_info;
}

#ifdef VIXMNTAPI
image_info::ptr vmware_portal_ex::get_image_info_local(const std::wstring& image_name)
{
    FUN_TRACE;

    return get_image_info_internal(_connect, image_name);
}
#endif

image_info::ptr vmware_portal_ex::get_image_info_internal(const VixDiskLibConnection connect, const std::wstring& image_name)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    image_info::ptr img_info(new image_info());

    if (!image_name.empty())
    {
        if (_init_status)
        {
            VixDiskLibHandle disk_handle = { 0 };

            vixerror = VixDiskLib_Open(connect, macho::stringutils::convert_unicode_to_utf8(image_name.c_str()).c_str(), VIXDISKLIB_FLAG_OPEN_READ_ONLY, &disk_handle);
            if (VIX_SUCCEEDED(vixerror))
            {
                img_info = get_image_info_internal(disk_handle);
                VixDiskLib_Close(disk_handle);
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"Get disk handle failure: %s.", get_native_error(vixerror).c_str());
            } // end open if
        } // end _init_status if
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid image name arguement.");
        img_info = NULL;
    }

    if (VIX_FAILED(vixerror))
    {
        img_info = NULL;
        //LOG(LOG_LEVEL_ERROR, L"%s, VixDiskLib error = %d", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
    }

    return img_info;
}

image_info::ptr vmware_portal_ex::get_image_info_internal(const VixDiskLibHandle dsk_handle)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    image_info::ptr img_info(new image_info());

    if (_init_status)
    {
        VixDiskLibInfo *disk_info = { 0 };

        vixerror = VixDiskLib_GetInfo(dsk_handle, &disk_info);
        if (VIX_SUCCEEDED(vixerror))
        {
            //get image disk information
            img_info->bios.cylinders = disk_info->biosGeo.cylinders;
            img_info->bios.heads = disk_info->biosGeo.heads;
            img_info->bios.sectors = disk_info->biosGeo.sectors;
            img_info->physical.cylinders = disk_info->physGeo.cylinders;
            img_info->physical.heads = disk_info->physGeo.heads;
            img_info->physical.sectors = disk_info->physGeo.sectors;
            img_info->total_sectors = disk_info->capacity;
            img_info->num_links = disk_info->numLinks;
            switch (disk_info->adapterType)
            {
                case VIXDISKLIB_ADAPTER_IDE:
                    img_info->adapter_type = std::wstring(L"IDE");
                    break;

                case VIXDISKLIB_ADAPTER_SCSI_BUSLOGIC:
                    img_info->adapter_type = std::wstring(L"SCSI_BUSLOGIC");
                    break;

                case VIXDISKLIB_ADAPTER_SCSI_LSILOGIC:
                    img_info->adapter_type = std::wstring(L"SCSI_LSILOGIC");
                    break;

                case VIXDISKLIB_ADAPTER_UNKNOWN:
                    img_info->adapter_type = std::wstring(L"UNKNOWN");
                    break;
            }
            if (disk_info->parentFileNameHint != NULL)
            {
                std::string pfh(disk_info->parentFileNameHint);
                img_info->parent_filename_hint = std::wstring(pfh.begin(), pfh.end());
            }
            if (disk_info->uuid != NULL)
            {
                std::string uuid(disk_info->uuid);
                img_info->disk_signature = std::wstring(uuid.begin(), uuid.end());
            }
            VixDiskLib_FreeInfo(disk_info);
#ifdef VIXMNTAPI
            VixDiskSetHandle disk_set_handle = { 0 };
            VixDiskLibHandle disk_handles[1] = { 0 };
            size_t num_of_disks = 1;

            disk_handles[0] = dsk_handle;
            vixerror = VixMntapi_OpenDiskSet(disk_handles, num_of_disks, VIXDISKLIB_FLAG_OPEN_READ_ONLY, &disk_set_handle);
            if (VIX_SUCCEEDED(vixerror))
            {
                size_t num_of_vols = 0;
                VixVolumeHandle *vol_handles = { 0 };
                VixOsInfo *os_info = { 0 };

                vixerror = VixMntapi_GetOsInfo(disk_set_handle, &os_info);
                if (VIX_SUCCEEDED(vixerror))
                {
                    if (os_info->edition != NULL)
                    {
                        std::string edition(os_info->edition);
                        img_info->os.edition = std::wstring(edition.begin(), edition.end());
                    }
                    else
                        img_info->os.edition = L"";

                    switch (os_info->family)
                    {
                        case VIXMNTAPI_NO_OS:
                            img_info->os.family = std::wstring(L"NO_OS");
                            break;

                        case VIXMNTAPI_WINDOWS:
                            img_info->os.family = std::wstring(L"Windows");
                            break;

                        case VIXMNTAPI_OTHER:
                            img_info->os.family = std::wstring(L"Other");
                            break;
                    }

                    img_info->os.is_64Bit = os_info->osIs64Bit;

                    if (os_info->vendor != NULL)
                    {
                        std::string vendor(os_info->vendor);
                        img_info->os.vendor = std::wstring(vendor.begin(), vendor.end());
                    }
                    else
                        img_info->os.vendor = L"";

                    img_info->os.major_version = os_info->majorVersion;
                    img_info->os.minor_version = os_info->minorVersion;

                    if (os_info->osFolder != NULL)
                    {
                        std::string folder(os_info->osFolder);
                        img_info->os.os_folder = std::wstring(folder.begin(), folder.end());
                    }
                    else
                        img_info->os.os_folder = L"";

                    VixMntapi_FreeOsInfo(os_info);
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, L"Get OS information failure: %s.", get_native_error(vixerror).c_str());
                }

                vixerror = VixMntapi_GetVolumeHandles(disk_set_handle, &num_of_vols, &vol_handles);
                if (VIX_SUCCEEDED(vixerror))
                {
                    VixVolumeInfo *vol_info = { 0 };

                    //get volume information
                    for (int i = 0; i < num_of_vols; i++)
                    {
                        vixerror = VixMntapi_GetVolumeInfo(vol_handles[i], &vol_info);
                        if (VIX_SUCCEEDED(vixerror))
                        {
                            volume_info::ptr vol(new volume_info());

                            vol->isMounted = vol_info->isMounted;
                            if (vol_info->symbolicLink != NULL)
                            {
                                std::string vol_sym_bolic_link(vol_info->symbolicLink);
                                vol->symbolic_link = std::wstring(vol_sym_bolic_link.begin(), vol_sym_bolic_link.end());
                            }
                            vol->num_guest_mount_points = vol_info->numGuestMountPoints;
                            switch (vol_info->type)
                            {
                                case VIXMNTAPI_BASIC_PARTITION:
                                    vol->type = std::wstring(L"BASIC_PARTITION");
                                    break;

                                case VIXMNTAPI_GPT_PARTITION:
                                    vol->type = std::wstring(L"GPT_PARTITION");
                                    break;

                                case VIXMNTAPI_DYNAMIC_VOLUME:
                                    vol->type = std::wstring(L"DYNAMIC_VOLUME");
                                    break;

                                case VIXMNTAPI_LVM_VOLUME:
                                    vol->type = std::wstring(L"LVM_VOLUME");
                                    break;

                                case VIXMNTAPI_UNKNOWN_VOLTYPE:
                                    vol->type = std::wstring(L"UNKNOWN");
                                    break;
                            }
                            for (int j = 0; j < vol_info->numGuestMountPoints; j++)
                            {
                                std::string mount_point(vol_info->inGuestMountPoints[j]);
                                vol->in_guest_mount_points.push_back(std::wstring(mount_point.begin(), mount_point.end()));
                            }

                            img_info->volumes.push_back(vol);
                            VixMntapi_FreeVolumeInfo(vol_info);
                        }
                    }
                    VixMntapi_FreeVolumeHandles(vol_handles);
                } // end get volumes if
                VixMntapi_CloseDiskSet(disk_set_handle);
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"Open disk set failure: %s.", get_native_error(vixerror).c_str());
            } // end open disk set if
#endif
            size_t required_len = 0;

            vixerror = VixDiskLib_GetMetadataKeys(dsk_handle, NULL, required_len, &required_len);
            if (VIX_ERROR_CODE(vixerror) == VIX_E_BUFFER_TOOSMALL)
            {
                img_info->metakeys.resize(required_len);
                vixerror = VixDiskLib_GetMetadataKeys(dsk_handle, &img_info->metakeys[0], required_len, NULL);
            }
        } //end get info if
    } // end _init_status if

    if (VIX_FAILED(vixerror))
    {
        img_info = NULL;
        LOG(LOG_LEVEL_ERROR, L"%s, VixDiskLib error = %d", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
    }

    return img_info;
}

#ifdef VIXMNTAPI
vol_mounted_info::vtr vmware_portal_ex::mount_image_local(const std::wstring& image_name, BOOL read_only)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    vol_mounted_info::vtr vols_mounted_info;
    mounted_memory_repo::ptr mounted_repo_ptr(new mounted_memory_repo());

    if (!image_name.empty())
    {
        std::map<std::wstring, mounted_memory_repo::ptr>::iterator it;

        if ((it = _mounted_repo.find(image_name)) != _mounted_repo.end())
        {
            vol_handle_info::vtr vol_mounted_records = it->second->mounted_info;
            for (std::vector<vol_handle_info::ptr>::iterator v = vol_mounted_records.begin(); v != vol_mounted_records.end(); ++v)
            {
                vols_mounted_info.push_back((*v)->vol_mounted_info);
            }

            return vols_mounted_info;
        }

        if (_init_status)
        {
            VixDiskLibHandle disk_handle;

            if (read_only)
                vixerror = VixDiskLib_Open(_connect, macho::stringutils::convert_unicode_to_utf8(image_name.c_str()).c_str(), VIXDISKLIB_FLAG_OPEN_READ_ONLY, &disk_handle);
            else
                vixerror = VixDiskLib_Open(_connect, macho::stringutils::convert_unicode_to_utf8(image_name.c_str()).c_str(), VIXDISKLIB_FLAG_OPEN_SINGLE_LINK, &disk_handle);
            if (VIX_SUCCEEDED(vixerror))
            {
                VixDiskSetHandle disk_set_handle = { 0 };
                VixDiskLibHandle disk_handles[1] = { 0 };
                size_t num_of_disks = 1;

                mounted_repo_ptr->image_name = image_name;
                mounted_repo_ptr->disk_handle = disk_handle;
                //mounted_repo_ptr->connect_handle = _connect;

                disk_handles[0] = disk_handle;
                if (read_only)
                    vixerror = VixMntapi_OpenDiskSet(disk_handles, num_of_disks, VIXDISKLIB_FLAG_OPEN_READ_ONLY, &disk_set_handle);
                else
                    vixerror = VixMntapi_OpenDiskSet(disk_handles, num_of_disks, VIXDISKLIB_FLAG_OPEN_SINGLE_LINK, &disk_set_handle);
                if (VIX_SUCCEEDED(vixerror))
                {
                    size_t num_of_vols = 0;
                    VixVolumeHandle *vol_handles = { 0 };

                    mounted_repo_ptr->diskset_handle = disk_set_handle;

                    vixerror = VixMntapi_GetVolumeHandles(disk_set_handle, &num_of_vols, &vol_handles);
                    if (VIX_SUCCEEDED(vixerror))
                    {
                        VixVolumeInfo *vol_info = { 0 };

                        mounted_repo_ptr->vol_handles = vol_handles;

                        //get volume information
                        for (int i = 0; i < num_of_vols; i++)
                        {
                            vixerror = VixMntapi_MountVolume(vol_handles[i], read_only);
                            if (VIX_SUCCEEDED(vixerror))
                            {
                                vixerror = VixMntapi_GetVolumeInfo(vol_handles[i], &vol_info);
                                if (VIX_SUCCEEDED(vixerror))
                                {
                                    vol_mounted_info::ptr vol_mounted_info(new vol_mounted_info());
                                    vol_handle_info::ptr vol_handle_info(new vol_handle_info());
                                    volume_info::ptr vol(new volume_info());

                                    vol->isMounted = vol_info->isMounted;
                                    vol->num_guest_mount_points = vol_info->numGuestMountPoints;
                                    switch (vol_info->type)
                                    {
                                        case VIXMNTAPI_BASIC_PARTITION:
                                            vol->type = L"BASIC_PARTITION";
                                            break;

                                        case VIXMNTAPI_GPT_PARTITION:
                                            vol->type = L"GPT_PARTITION";
                                            break;

                                        case VIXMNTAPI_DYNAMIC_VOLUME:
                                            vol->type = L"DYNAMIC_VOLUME";
                                            break;

                                        case VIXMNTAPI_LVM_VOLUME:
                                            vol->type = L"LVM_VOLUME";
                                            break;

                                        case VIXMNTAPI_UNKNOWN_VOLTYPE:
                                            vol->type = L"UNKNOWN";
                                            break;
                                    }
                                    for (int j = 0; j < vol_info->numGuestMountPoints; j++)
                                    {
                                        std::string mount_point(vol_info->inGuestMountPoints[j]);
                                        vol->in_guest_mount_points.push_back(std::wstring(mount_point.begin(), mount_point.end()));
                                    }

                                    if (vol_info->symbolicLink != NULL)
                                    {
                                        std::string vol_sym_bolic_link(vol_info->symbolicLink);
                                        vol->symbolic_link = std::wstring(vol_sym_bolic_link.begin(), vol_sym_bolic_link.end());
 
                                        std::wstring drv_letter = get_free_drive_letter();
                                        if (!drv_letter.empty() && DefineDosDevice(0, drv_letter.c_str(), vol->symbolic_link.c_str()))
                                        {
                                            vol_mounted_info->mount_point = drv_letter;
                                            vol_handle_info->vol_handle = vol_handles[i];
                                        }
                                        else
                                            VixMntapi_DismountVolume(vol_handles[i], true);
                                    }

                                    vol_mounted_info->vol_info = vol;
                                    vols_mounted_info.push_back(vol_mounted_info);
                                    vol_handle_info->vol_mounted_info = vol_mounted_info;
                                    mounted_repo_ptr->mounted_info.push_back(vol_handle_info);
                                    VixMntapi_FreeVolumeInfo(vol_info);
                                }
                                else
                                {
                                    VixMntapi_DismountVolume(vol_handles[i], true);
                                    break;
                                }
                            }
                            else
                                break;
                        }
                    } // end get volumes if
                } // end open disk set if
            } // end open if
        } // end _init_status if
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid image name arguement.");
    }

    if (VIX_FAILED(vixerror))
    {
#if 0
        LOG(LOG_LEVEL_ERROR, L"%s, VixDiskLib error = %d", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
        for (vol_mounted_info::vtr::iterator it = vols_mounted_info.begin(); it != vols_mounted_info.end(); ++it)
        {
            *it = NULL;
        }
        vols_mounted_info.clear();
        mounted_repo_ptr = NULL;
#endif
        unmount_image_local(image_name);
    }
    else
    {
        _mounted_repo.insert(std::pair<std::wstring, mounted_memory_repo::ptr>(mounted_repo_ptr->image_name, mounted_repo_ptr));
    }

    return vols_mounted_info;
}

BOOL vmware_portal_ex::unmount_image_local(const std::wstring& image_name, BOOL force)
{
    FUN_TRACE;

    BOOL bRtn = 1;
    BOOL bOnce = false;
    //std::map<std::wstring, mounted_memory_repo::ptr>::iterator it;

    if (_init_status && _mounted_repo.size() > 0)
    {
        mounted_memory_repo::map::iterator p = _mounted_repo.begin();

        for (;;)
        {
            std::wstring key = p->first;
            if (!image_name.empty() && image_name == key)
                bOnce = true;
            else if (!image_name.empty() && image_name != key)
            {
                if (p != _mounted_repo.end())
                {
                    ++p;
                    continue;
                }
                else
                {
                    bRtn = 0;
                    LOG(LOG_LEVEL_ERROR, L"No mounted records for this image \"%s\".", image_name);
                    break;
                }
            }

            vol_handle_info::vtr vols_mounted_info = p->second->mounted_info;

            for (std::vector<vol_handle_info::ptr>::iterator v = vols_mounted_info.begin(); v != vols_mounted_info.end(); ++v)
            {
                vol_handle_info::ptr vol_mounted_info_ptr = *v;

                BOOL bRtn = DefineDosDevice(DDD_RAW_TARGET_PATH, vol_mounted_info_ptr->vol_mounted_info->mount_point.c_str(), vol_mounted_info_ptr->vol_mounted_info->vol_info->symbolic_link.c_str());
                if (bRtn != 0)
                {
                    vol_mounted_info_ptr->vol_mounted_info->mount_point = L"";
                    VixMntapi_DismountVolume(vol_mounted_info_ptr->vol_handle, true);
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, L"%s, Umount error = %d", get_last_error_string_win(), GetLastError());
                    break;
                }
            }

            if (bRtn != 0)
            {
                for (std::vector<vol_handle_info::ptr>::iterator v = vols_mounted_info.begin(); v != vols_mounted_info.end(); ++v)
                    *v = NULL;
                vols_mounted_info.clear();
                p->second = NULL;
                _mounted_repo.erase(p);

                if (_mounted_repo.size() > 0)
                    p = _mounted_repo.begin();
                else
                    break;
            }
            else
                break;

            if (bOnce)
            {
                break;
            }
        }
    }
    else
    {
        bRtn = 0;
        LOG(LOG_LEVEL_ERROR, L"No mounted records.");
    }

#if 0
    if (!image_name.empty() && _init_status)
    {
        if ((it = _mounted_repo->find(image_name)) != _mounted_repo->end())
        {
            vol_handle_info::vtr vols_mounted_info = it->second->mounted_info;

            for (std::vector<vol_handle_info::ptr>::iterator v = vols_mounted_info.begin(); v != vols_mounted_info.end(); ++v)
            {
                vol_handle_info::ptr vol_mounted_info_ptr = *v;

                BOOL bRtn = DefineDosDevice(DDD_RAW_TARGET_PATH, vol_mounted_info_ptr->vol_mounted_info->mount_point.c_str(), vol_mounted_info_ptr->vol_mounted_info->vol_info->symbolic_link.c_str());
                if (bRtn != 0)
                {
                    vol_mounted_info_ptr->vol_mounted_info->mount_point = L"";
                    VixMntapi_DismountVolume(vol_mounted_info_ptr->vol_handle, true);
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, L"%s, Umount error = %d", get_last_error_string_win(), GetLastError());
                    break;
                }
            }
        }
        else
        {
            bRtn = 0;
            LOG(LOG_LEVEL_ERROR, L"No mounted records for this image \"%s\".", image_name);
        }
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid image name arguement.");
        bRtn = 0;
    }

    if (bRtn != 0)
    {
        vol_handle_info::vtr vols_mounted_info = it->second->mounted_info;

        for (std::vector<vol_handle_info::ptr>::iterator v = vols_mounted_info.begin(); v != vols_mounted_info.end(); ++v)
            *v = NULL;
        vols_mounted_info.clear();
        it->second = NULL;
        _mounted_repo->erase(it);
    }
#endif

    return bRtn;
}

#endif

VOID vmware_portal_ex::snapshot_progress_callback(const std::wstring& snapshot_name, const int& progess)
{
    LOG(LOG_LEVEL_DEBUG, L"Snapshot: %s, progress: %d%%.", snapshot_name.c_str(), progess);
    if (_isatty(_fileno(stdout)))
    {
        std::wcout << L"Snapshot: " << snapshot_name << L", progress: " << std::to_wstring(progess) << L"%\r";
        std::cout.flush();
    }
}

#ifdef VIXMNTAPI

DWORD vmware_portal_ex::clone_image_to_local(const image_clone_params& clone_req, clone_image_progress::slot_type slot, int image_type)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_virtual_machine::ptr vm = NULL;
    int retry_count = 3;
    std::vector<std::wstring> outfiles;

    if (clone_req.src.ip.empty() || clone_req.src.uname.empty() || 
        clone_req.src.passwd.empty() || clone_req.src_machine_key.empty() ||
        clone_req.dst_image_path_file.empty() ||
        (image_type < IMAGE_TYPE_VMDK || image_type > IMAGE_TYPE_VHDX))
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid parameters for clone task.");
        return S_FALSE;
    }

    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % clone_req.src.ip);
    vmware_portal::ptr portal = vmware_portal::connect(uri, clone_req.src.uname, clone_req.src.passwd);
    
    if (!portal)
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to login portal.");
        return S_FALSE;
    }

    //get vmx spec
    vm = portal->get_virtual_machine(clone_req.src_machine_key);
    if (vm == NULL)
    {
        LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing, abort clone task.");
        return S_FALSE;
    }

    //take a temp snapshot, synchronous mode
    std::wstring snapshot_name = L"";
    dwRtn = create_temp_snapshot(clone_req.src.ip, clone_req.src.uname, clone_req.src.passwd, clone_req.src_machine_key, snapshot_name);
    if (dwRtn != S_OK)
        return dwRtn;

    if (!snapshot_name.empty())
    {
        std::wstring snapshot_ref_item = L"";

        get_snapshot_ref_item(clone_req.src.ip, clone_req.src.uname, clone_req.src.passwd, clone_req.src_machine_key, snapshot_name, snapshot_ref_item);

        LOG(LOG_LEVEL_DEBUG, L"Thread ID: %u, start for cloning %s.", GetCurrentThreadId(), clone_req.src_image_path_file.c_str());

        if (_isatty(_fileno(stdout)))
            std::wcout << L"\nPrepare cloning..." << std::endl;

        std::wstring dst_image_file;
        std::wstring src_image_file;
        universal_disk_rw::ptr infile, outfile;
        uint64 next_to_read_pos_byte;
        uint64 next_to_write_pos_byte;

        foreach (vmware_disk_info::map::value_type& disk, vm->disks)
        {
            if (!clone_req.src_image_path_file.empty())
            {
                if (!boost::equals(clone_req.src_image_path_file, disk.first))
                    continue;
                else
                {
                    src_image_file = clone_req.src_image_path_file;
                    dst_image_file = clone_req.dst_image_path_file;
                    if (image_type == IMAGE_TYPE_VHDX)
                        boost::replace_all(dst_image_file, L".vmdk", L".vhdx");
                    outfiles.push_back(dst_image_file);
                }
            }
            else
            {
                std::string dst_folder = macho::stringutils::convert_unicode_to_utf8(clone_req.dst_image_path_file); //destination folder
                
                //create subfolder under the destination folder, if subfolder doesn't existing
                if (!(boost::filesystem::exists(str(boost::format("%s/%s") % dst_folder % macho::stringutils::convert_unicode_to_utf8(vm->name)))))
                {
                    boost::filesystem::create_directory(str(boost::format("%s/%s") % dst_folder % macho::stringutils::convert_unicode_to_utf8(vm->name)));
                }

                //extract vmdk name
                std::wstring image_name = L"";
                size_t pos = disk.first.find_last_of(L"/");
                image_name = disk.first.substr(pos + 1);

                dst_image_file = clone_req.dst_image_path_file + L"/" + vm->name + L"/" + image_name;
                src_image_file = disk.first;
                if (image_type == IMAGE_TYPE_VHDX)
                    boost::replace_all(dst_image_file, L".vmdk", L".vhdx");
                outfiles.push_back(dst_image_file);
            }

            if (boost::filesystem::exists(dst_image_file))
                boost::filesystem::remove(dst_image_file);

            next_to_read_pos_byte = next_to_write_pos_byte = 0;

            infile = vmware_vmdk_mgmt::open_vmdk_for_rw(
                this,
                clone_req.src.ip,
                clone_req.src.uname,
                clone_req.src.passwd,
                clone_req.src_machine_key,
                src_image_file,
                snapshot_name,
                true);

            if (infile)
            {
                image_info::ptr dsk_info = dynamic_cast<vmware_vmdk_file_rw*>(infile.get())->get_image_info();

                if (dsk_info)
                {
                    if (image_type == IMAGE_TYPE_VMDK)
                    {                
                        disk_convert_settings create_flags;
                        if (vmware_vmdk_mgmt::create_vmdk_file(this, dst_image_file, create_flags, dsk_info->total_sectors))
                        {
                            outfile = vmware_vmdk_mgmt::open_vmdk_for_rw(this, dst_image_file, false);
                            if (outfile == NULL)
                            {
                                boost::filesystem::remove(dst_image_file);
                                infile = NULL;
                                dwRtn = S_FALSE;
                            }
                        }
                        else
                        {
                            LOG(LOG_LEVEL_ERROR, L"Can't create the virtual disk file \"%s\".", dst_image_file.c_str());
                            dwRtn = S_FALSE;
                        }
                    }
                    else if (image_type == IMAGE_TYPE_VHDX)
                    {
                        DWORD err = 0;

                        if (ERROR_SUCCESS != (err = win_vhdx_mgmt::create_vhdx_file(dst_image_file, 
                            CREATE_VIRTUAL_DISK_FLAG_NONE, 
                            dsk_info->total_sectors * VIXDISKLIB_SECTOR_SIZE, 
                            CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE, 
                            VIXDISKLIB_SECTOR_SIZE, 
                            VIXDISKLIB_SECTOR_SIZE)))
                        {
                            LOG(LOG_LEVEL_ERROR, L"Can't create the virtual disk file \"%s\". ( error : %d )", dst_image_file.c_str(), err);
                            dwRtn = S_FALSE;
                        }
                        else
                        {
                            outfile = win_vhdx_mgmt::open_virtual_disk_for_rw(dst_image_file);
                            if (outfile == NULL)
                            {
                                boost::filesystem::remove(dst_image_file);
                                infile = NULL;
                                dwRtn = S_FALSE;
                            }
                        }
                    }
                    else
                    {
                        LOG(LOG_LEVEL_ERROR, L"Unknown image type: %d.", image_type);
                        dwRtn = S_FALSE;
                    }
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, L"Failed to retrieve image: %s information.", src_image_file.c_str());
                    infile = NULL;
                    dwRtn = S_FALSE;
                }
            }

            if(dwRtn != S_OK)
            {
                LOG(LOG_LEVEL_ERROR, L"Failed to initial read/write objects.");
                break;
            }

            do
            {
                if ((dwRtn = clone_image_to_local_type(infile, outfile, snapshot_name, next_to_read_pos_byte, next_to_write_pos_byte, slot, image_type)) == S_OK)
                    retry_count = 0;
                else
                {
                    if (retry_count > 0)
                    {
                        Sleep(WAIT_INTERVAL_SECONDS);
                        infile = outfile = NULL;

                        LOG(LOG_LEVEL_WARNING, L"Reconnect to %s.", clone_req.src.ip.c_str());

                        infile = vmware_vmdk_mgmt::open_vmdk_for_rw(
                            this,
                            clone_req.src.ip,
                            clone_req.src.uname,
                            clone_req.src.passwd,
                            clone_req.src_machine_key,
                            src_image_file,
                            snapshot_name,
                            true);

                        if (image_type == IMAGE_TYPE_VMDK)
                            outfile = vmware_vmdk_mgmt::open_vmdk_for_rw(this, dst_image_file, false);
                        else if (image_type == IMAGE_TYPE_VHDX)
                            outfile = win_vhdx_mgmt::open_virtual_disk_for_rw(dst_image_file);

                        if (infile == NULL || outfile == NULL)
                        {
                            LOG(LOG_LEVEL_ERROR, L"Failed to initial read/write objects for retry.");
                            retry_count = 1;
                        }

                        dynamic_cast<vmware_vmdk_file_rw*>(infile.get())->get_image_info();
                        retry_count--;

                        if (retry_count == 0)
                            dwRtn = S_FALSE;
                        //continue;
                    }
                    else
                    {
                        dwRtn = S_FALSE;
                    }
                }
            } while (retry_count > 0 && !_is_interrupted);

            infile = NULL;
            outfile = NULL;

#if 0
            if (boost::filesystem::exists(dst_path))
            {
                std::string vmdk_lck(dst_path);
                vmdk_lck += ".lck";

                if (boost::filesystem::exists(vmdk_lck.c_str()))
                    VixDiskLib_Unlink(_connect, vmdk_lck.c_str());
                VixDiskLib_Unlink(_connect, dst_path);
            }

            vixerror = VixDiskLib_Clone(_connect,
                                        dst_path,
                                        src_conn,
                                        src_path,
                                        &create_params,
                                        &clone_progress_func,
                                        NULL, true);

            if (VIX_FAILED(vixerror))
            {
                LOG(LOG_LEVEL_ERROR, L"Clone failed => %s, error = %d.", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
                if (_isatty(_fileno(stdout)))
                    std::wcout << "Error message: " << get_native_error(vixerror) << std::endl;
                dwRtn = S_FALSE;

                if (clone_req.src_image_path_file.empty())
                {
                    boost::filesystem::remove_all(clone_req.dst_image_path_file);
                }
                else
                {
                    boost::filesystem::remove(clone_req.dst_image_path_file);
                }

                break;
            }
            else
            {
                LOG(LOG_LEVEL_DEBUG, L"Thread ID: %u, clone %s completed.", GetCurrentThreadId(), clone_req.src_image_path_file.c_str());
                free(src_path);
                free(dst_path);
                src_path = dst_path = NULL;
                dwRtn = S_OK;
            }
#endif
            if (dwRtn != S_OK)
                break;
        }// end images loop

        //delete the temp snapshot
        if (!_is_interrupted)
        {
            remove_temp_snapshot(clone_req.src.ip, clone_req.src.uname, clone_req.src.passwd, clone_req.src_machine_key, snapshot_name);
        }
            
        if (dwRtn != S_OK)
        {
            foreach(std::wstring outfile, outfiles)
                VixDiskLib_Unlink(_connect, macho::stringutils::convert_unicode_to_utf8(outfile).c_str());

            dwRtn = S_FALSE;
        }
    }
    else
    {
        dwRtn = S_FALSE;
        LOG(LOG_LEVEL_ERROR, "Failed to clone the image.");
    }

    return dwRtn;
}

DWORD vmware_portal_ex::clone_image_to_local_type(universal_disk_rw::ptr in_file, universal_disk_rw::ptr out_file, std::wstring& snapshot_name, uint64& offset_read_byte, uint64& offset_write_byte, clone_image_progress::slot_type slot, int image_type)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;

    try
    {
        if (image_type == IMAGE_TYPE_VMDK)
            clone_image_to_vmdk(in_file, out_file, snapshot_name, offset_read_byte, offset_write_byte, slot);
        else if (image_type == IMAGE_TYPE_VHDX)
            clone_image_to_vhdx(in_file, out_file, snapshot_name, offset_read_byte, offset_write_byte, slot);
        else
            dwRtn = S_FALSE;
    }
    catch (boost::exception const& e)
    {
        std::string msg = boost::diagnostic_information(e);
        LOG(LOG_LEVEL_WARNING, L"%s.", macho::stringutils::convert_utf8_to_unicode(msg).c_str());
        dwRtn = S_FALSE;
    }

    return dwRtn;
}

#endif

void vmware_portal_ex::clone_image_to_vmdk(universal_disk_rw::ptr in_file, universal_disk_rw::ptr out_file, std::wstring& snapshot_name, uint64& offset_read_byte, uint64& offset_write_byte, clone_image_progress::slot_type slot)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    VixDiskLibSectorType capacity = 0U;
    VixDiskLibSectorType total_sectors = 0U;
    clone_image_progress progress;
    int retry_count = 5;

    if (in_file == NULL || out_file == NULL || snapshot_name.empty())
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid parametes.");
        BOOST_THROW_VMWARE_EXCEPTION(ERROR_INVALID_PARAMETER, L"The parameter is incorrect.");
    }

    vmware_vmdk_file_rw *in = dynamic_cast<vmware_vmdk_file_rw*>(in_file.get());
    vmware_vmdk_file_rw *out = dynamic_cast<vmware_vmdk_file_rw*>(out_file.get());
    image_info::ptr image = in->get_image_info();
    if ( !image || image->total_sectors == 0U)
    {
        LOG(LOG_LEVEL_ERROR, L"Get image: %s size information failure.", in->path().c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_VDISK_GET_FAILED, L"Get image file information error.");
    }
    
    total_sectors = capacity = image->total_sectors;

#if 0
    if (offset == 0L)
    {
        if (boost::filesystem::exists(macho::stringutils::convert_unicode_to_utf8(out->path()).c_str()))
        {
            std::string vmdk_lck = macho::stringutils::convert_unicode_to_utf8(out->path());
            vmdk_lck += ".lck";

            if (boost::filesystem::exists(vmdk_lck.c_str()))
                vixerror = VixDiskLib_Unlink(_connect, vmdk_lck.c_str());

            if (!out->delete_image_file())
            {
                LOG(LOG_LEVEL_ERROR, L"Remove existing local image file failure => %s, error = %d.", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
                BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
            }
        }

        disk_convert_settings create_flags;

        if (!vmware_vmdk_mgmt::create_vmdk_file(out->path(), create_flags, total_sectors))
        {
            BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
        }
    }
#endif

    char *key = NULL;
    char *val = NULL;
    std::vector<char> metakeys = image->metakeys;
    key = &metakeys[0];

    while (key[0] != '\0' && offset_read_byte == 0L)
    {
        if (in->read_meta_data(key, (void **)&val))
        {
            if (!out->write_meta_data(key, val))
            {
                vixerror = VIX_E_DISK_NOKEYOVERRIDE;
                BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
            }
        }
        else
        {
            vixerror = VIX_E_DISK_KEY_NOTFOUND;
            BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
        }

        if (val)
        {
            free(val);
            val = NULL;
        }
        key += (1 + strlen(key));
    }

    uint64 start_sector = offset_read_byte / VIXDISKLIB_SECTOR_SIZE;
    uint32_t num_sectors_to_read = 0;
    uint32_t num_sectors_read = 0;
    uint32_t num_sectors_written = 0;
    uint8 *data_buf = new uint8[DATA_BUFFER_SIZE * VIXDISKLIB_SECTOR_SIZE];
    progress.connect(slot);

    if (start_sector != 0U)
        capacity -= start_sector;

    do
    {
        if (capacity >= DATA_BUFFER_SIZE)
        {
            num_sectors_to_read = DATA_BUFFER_SIZE;
        }
        else
            num_sectors_to_read = capacity;

        if (in->sector_read(start_sector, num_sectors_to_read, data_buf, num_sectors_read))
        {
            if (!out->sector_write(start_sector, data_buf, num_sectors_read, num_sectors_written))
            {
                delete[] data_buf;
                vixerror = VIX_E_FILE_ACCESS_ERROR;
                BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
            }
        }
        else
        {
            delete[] data_buf;
            vixerror = VIX_E_FILE_ACCESS_ERROR;
            BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
        }

        start_sector += num_sectors_read;
        offset_write_byte = offset_read_byte = start_sector * VIXDISKLIB_SECTOR_SIZE;
        capacity -= num_sectors_read;
        
        //clone_progress_func(NULL, (offset * 100) / total_sectors);
        progress(snapshot_name, 
            in->path(), 
            out->path(), 
            total_sectors * VIXDISKLIB_SECTOR_SIZE, 
            offset_read_byte,  //number of bytes read 
            offset_write_byte); //number of bytes written
    } while (capacity > 0 && !_is_interrupted);

    delete[] data_buf;
}

void vmware_portal_ex::clone_image_to_vhdx(universal_disk_rw::ptr in_file, universal_disk_rw::ptr out_file, std::wstring& snapshot_name, uint64& offset_read_byte, uint64& offset_write_byte, clone_image_progress::slot_type slot)
{
    FUN_TRACE;

    VixError vixerror = VIX_OK;
    VixDiskLibSectorType capacity = 0U;
    VixDiskLibSectorType total_sectors = 0U;
    clone_image_progress progress;
    int retry_count = 5;

    if (in_file == NULL || out_file == NULL || snapshot_name.empty())
    {
        LOG(LOG_LEVEL_ERROR, L"Invalid parametes.");
        BOOST_THROW_VMWARE_EXCEPTION(ERROR_INVALID_PARAMETER, L"The parameter is incorrect.");
    }

    vmware_vmdk_file_rw *in = dynamic_cast<vmware_vmdk_file_rw*>(in_file.get());

    image_info::ptr image = in->get_image_info();
    if (!image || image->total_sectors == 0U)
    {
        LOG(LOG_LEVEL_ERROR, L"Get image: %s size information failure.", in->path().c_str());
        BOOST_THROW_VMWARE_EXCEPTION(HV_ERROR_VDISK_GET_FAILED, L"Get image file information error.");
    }

    total_sectors = capacity = image->total_sectors;

    uint64 start_sector = offset_read_byte / VIXDISKLIB_SECTOR_SIZE;
    uint64 vhdx_start_byte = offset_write_byte;
    uint32_t num_sectors_to_read = 0;
    uint32_t num_sectors_read = 0;
    uint32_t num_bytes_written = 0;
    uint8 *data_buf = new uint8[DATA_BUFFER_SIZE * VIXDISKLIB_SECTOR_SIZE];
    progress.connect(slot);

    if (start_sector != 0U)
        capacity -= start_sector;

    do
    {
        if (capacity >= DATA_BUFFER_SIZE)
        {
            num_sectors_to_read = DATA_BUFFER_SIZE;
        }
        else
            num_sectors_to_read = capacity;

        if (in->sector_read(start_sector, num_sectors_to_read, data_buf, num_sectors_read))
        {
            if (!out_file->write(vhdx_start_byte, data_buf, num_sectors_read * VIXDISKLIB_SECTOR_SIZE, num_bytes_written))
            {
                delete[] data_buf;
                vixerror = VIX_E_FILE_ACCESS_ERROR;
                BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
            }
        }
        else
        {
            delete[] data_buf;
            vixerror = VIX_E_FILE_ACCESS_ERROR;
            BOOST_THROW_VMWARE_EXCEPTION(VIX_ERROR_CODE(vixerror), get_native_error(vixerror));
        }

        start_sector += num_sectors_read;
        offset_read_byte = start_sector * VIXDISKLIB_SECTOR_SIZE;
        offset_write_byte = vhdx_start_byte += num_bytes_written;
        capacity -= num_sectors_read;

        //clone_progress_func(NULL, (offset * 100) / total_sectors);
        progress(snapshot_name, 
            in->path(), 
            out_file->path(), 
            total_sectors * VIXDISKLIB_SECTOR_SIZE, 
            offset_read_byte,
            offset_write_byte);
    } while (capacity > 0 && !_is_interrupted);

    delete[] data_buf;
}

DWORD vmware_portal_ex::create_temp_snapshot(_In_ const vmware_virtual_machine::ptr& vm, _Inout_ std::wstring& snapshot_name)
{
    DWORD dwRtn = S_OK;

    if (!vm)
    {
        LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
        dwRtn = S_FALSE;
    }
    else if (!_portal.count(vm->uuid))
    {
        LOG(LOG_LEVEL_ERROR, L"Doesn't specify the portal for the VM.");
        dwRtn = S_FALSE;
    }
    else{
        vmware_vm_create_snapshot_parm::ptr snapshot_parm(new vmware_vm_create_snapshot_parm());
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        std::stringstream uuid_str;

        if (snapshot_name.empty())
        {
            uuid_str << uuid;
            std::string uuid_snapshot_name = uuid_str.str();
            snapshot_name = snapshot_parm->name = macho::stringutils::convert_utf8_to_unicode(uuid_snapshot_name);
        }
        else
            snapshot_parm->name = snapshot_name;

        macho::windows::registry reg;
        if (reg.open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\SaaSaMe")))
        {
            if (reg[_T("IncludeMemory")].exists() && reg[_T("IncludeMemory")].is_dword())
            {
                LOG(LOG_LEVEL_RECORD, L"IncludeMemory option: %d", (DWORD)reg[_T("IncludeMemory")]);

                if ((DWORD)reg[_T("IncludeMemory")] > 0)
                    snapshot_parm->include_memory = true;
            }
        }
        if (vm->power_state != HV_VMPOWER_ON ){
            LOG(LOG_LEVEL_WARNING, L"Taking snapshot (%s) without quiesce when the VM power state is OFF or SUSPENDED.", snapshot_parm->name.c_str());
            snapshot_parm->quiesce = false;
        }
        else if (vm->tools_status == HV_VMTOOLS_NOTINSTALLED ||
            vm->tools_status == HV_VMTOOLS_NOTRUNNING ||
            vm->tools_status == HV_VMTOOLS_UNKNOWN ||
            vm->tools_status == HV_VMTOOLS_UNMANAGED){
            LOG(LOG_LEVEL_WARNING, L"Taking snapshot (%s) without quiesce when the VM tool is not ready. VM tool status is (%d).", snapshot_parm->name.c_str(), vm->tools_status);
            snapshot_parm->quiesce = false;
        }
        //using default
        // => snapshot_parm->include_memory = false;
        // => snapshot_parm->quiesce = true;
        snapshot_parm->description = boost::str(boost::wformat(L"irm_vp: %s, %s") 
            % snapshot_parm->name 
            % macho::stringutils::convert_utf8_to_unicode(boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time())));

        LOG(LOG_LEVEL_DEBUG, L"Start taking snapshot %s for clone.", snapshot_parm->name.c_str());

        if (_isatty(_fileno(stdout)))
            std::wcout << L"Prepare taking a temp snapshot for clone, it may take a while, please wait..." << std::endl;

        try
        {
            std::wstring machine_key = vm->uuid;
            int nReturn = _portal[machine_key]->create_virtual_machine_snapshot(machine_key, *snapshot_parm, boost::bind(&vmware_portal_ex::snapshot_progress_callback, this, _1, _2));
            if (nReturn == TASK_STATE::STATE_SUCCESS)
            {
                vmware_virtual_machine::ptr vm = _portal[machine_key]->get_virtual_machine(machine_key);
                if (vm != NULL)
                {
                    snapshot_internal_info snapshot_internal;

                    get_snapshot_internal_from_list(vm->root_snapshot_list, snapshot_parm->name, snapshot_internal);
                    if (snapshot_internal.ref_item.empty())
                    {
                        LOG(LOG_LEVEL_ERROR, L"Snapshot \"%s\" not found.", snapshot_parm->name.c_str());
                        dwRtn = S_FALSE;
                    }
                    else
                    {
                        snapshot_internal.name = snapshot_name;
                    }
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
                    dwRtn = S_FALSE;
                }
            }
            else if (nReturn == TASK_STATE::STATE_CANCELLED)
            {
                LOG(LOG_LEVEL_WARNING, L"Create temp snapshot cancelled, state = %d.", nReturn);
                dwRtn = S_FALSE;
            }
            else if (nReturn == TASK_STATE::STATE_RUNNING || nReturn == TASK_STATE::STATE_QUEUED || nReturn == TASK_STATE::STATE_TIMEOUT)
            {
                LOG(LOG_LEVEL_WARNING, L"Create temp snapshot is on going, state = %d.", nReturn);
                dwRtn = S_OK;
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"Create temp snapshot failed, state = %d.", nReturn);
                dwRtn = S_FALSE;
            }
        }
        catch (macho::exception_base& e){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
            dwRtn = S_FALSE;
        }
        catch (boost::exception &e)
        {
            LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
            dwRtn = S_FALSE;
        }
    }
    return dwRtn;
}

DWORD vmware_portal_ex::create_temp_snapshot(const std::wstring& host, const std::wstring& username, const std::wstring& passwd, const std::wstring& machine_key, std::wstring& snapshot_name)
{
    FUN_TRACE;
    if (!_portal.count(machine_key))
    {
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        vmware_portal::ptr portal = vmware_portal::connect(uri, username, passwd);
        if (portal)
            _portal[machine_key] = portal;
        else
        {
            LOG(LOG_LEVEL_ERROR, L"Failed to connect to portal \"%s\".", host.c_str());
            return S_FALSE;
        }
    }
    vmware_virtual_machine::ptr vm = _portal[machine_key]->get_virtual_machine(machine_key);
    return create_temp_snapshot(vm, snapshot_name);
}

DWORD vmware_portal_ex::remove_temp_snapshot(const std::wstring& host, const std::wstring& username, const std::wstring& passwd, const std::wstring& machine_key, const std::wstring& snapshot_name)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_virtual_machine::ptr vm = NULL;

    auto search = _portal.find(machine_key);
    if (search == _portal.end())
    {
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        vmware_portal::ptr portal = vmware_portal::connect(uri, username, passwd);
        if (portal)
            _portal[machine_key] = portal;
    }

    try
    {
        if (_portal.count(machine_key))
        {
            vm = _portal[machine_key]->get_virtual_machine(machine_key);
            if (vm != NULL)
            {
                dwRtn = remove_temp_snapshot(vm, snapshot_name);
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
                dwRtn = S_FALSE;
            }
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"Can't connect the portal.");
            dwRtn = S_FALSE;
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        dwRtn = S_FALSE;
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        dwRtn = S_FALSE;
    }
 
    return dwRtn;
}

DWORD vmware_portal_ex::remove_temp_snapshot(const vmware_virtual_machine::ptr& vm, const std::wstring& snapshot_name)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_vm_remove_snapshot_parm::ptr remove_snapshot_req(new vmware_vm_remove_snapshot_parm());

    remove_snapshot_req->consolidate = true;
    remove_snapshot_req->remove_children = false;

    try
    {
        if (vm == NULL)
        {
            LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
            dwRtn = S_FALSE;
        }
        else if (0 == _portal.count(vm->uuid))
        {
            LOG(LOG_LEVEL_ERROR, L"Doesn't specify the portal for the VM.");
            dwRtn = S_FALSE;
        } 
        else
        {
            bool has_running_create_snapshot_task = false;
            vmware_vm_task_info::vtr tasks = _portal[vm->uuid]->get_virtual_machine_tasks(vm->uuid);
            foreach(vmware_vm_task_info::ptr t, tasks){
                if (has_running_create_snapshot_task = ( t->is_create_snapshot_task() && (!t->is_completed())) )
                    break;
            }
            if (has_running_create_snapshot_task){
                LOG(LOG_LEVEL_ERROR, L"Has running create snapshot task for vm (%s).", vm->name.c_str());
                dwRtn = S_FALSE;
            }
            else{
                snapshot_internal_info snapshot_internal;
                get_snapshot_internal_from_list(vm->root_snapshot_list, snapshot_name, snapshot_internal);
                if (snapshot_internal.ref_item.empty())
                {
                    LOG(LOG_LEVEL_WARNING, L"Snapshot \"%s\" not found.", snapshot_name.c_str());
                    dwRtn = S_OK;
                }
                else
                {
                    remove_snapshot_req->id = snapshot_internal.id;
                    remove_snapshot_req->mor_ref = snapshot_internal.ref_item;

                    int nReturn = _portal[vm->uuid]->remove_virtual_machine_snapshot(vm->uuid, *remove_snapshot_req, boost::bind(&vmware_portal_ex::snapshot_progress_callback, this, _1, _2));
                    if (nReturn != TASK_STATE::STATE_SUCCESS)
                    {
                        LOG(LOG_LEVEL_ERROR, L"Failed to remove the temp snapshot %s:%d.", snapshot_name.c_str(), remove_snapshot_req->id);
                        dwRtn = S_FALSE;
                    }
                }
            }
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        dwRtn = S_FALSE;
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        dwRtn = S_FALSE;
    }

    return dwRtn;
}

DWORD vmware_portal_ex::revert_temp_snapshot(const std::wstring& host, const std::wstring& username, const std::wstring& passwd, const std::wstring& machine_key, const std::wstring& snapshot_name)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_virtual_machine::ptr vm = NULL;

    auto search = _portal.find(machine_key);
    if (search == _portal.end())
    {
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        vmware_portal::ptr portal = vmware_portal::connect(uri, username, passwd);
        if (portal)
            _portal[machine_key] = portal;
    }

    try
    {
        if (_portal.count(machine_key))
        {
            vm = _portal[machine_key]->get_virtual_machine(machine_key);
            if (vm != NULL)
            {
                dwRtn = revert_temp_snapshot(vm, snapshot_name);
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
                dwRtn = S_FALSE;
            }
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"Can't connect the portal.");
            dwRtn = S_FALSE;
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        dwRtn = S_FALSE;
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        dwRtn = S_FALSE;
    }

    return dwRtn;
}

DWORD vmware_portal_ex::revert_temp_snapshot(const vmware_virtual_machine::ptr& vm, const std::wstring& snapshot_name)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    vmware_vm_revert_to_snapshot_parm::ptr revert_snapshot_req(new vmware_vm_revert_to_snapshot_parm());

    revert_snapshot_req->suppress_power_on = false;

    try
    {
        if (vm == NULL)
        {
            LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
            dwRtn = S_FALSE;
        }
        else if (0 == _portal.count(vm->uuid))
        {
            LOG(LOG_LEVEL_ERROR, L"Doesn't specify the portal for the VM.");
            dwRtn = S_FALSE;
        }
        else
        {
            bool has_running_snapshot_task = false;
            vmware_vm_task_info::vtr tasks = _portal[vm->uuid]->get_virtual_machine_tasks(vm->uuid);
            foreach(vmware_vm_task_info::ptr t, tasks){
                if (has_running_snapshot_task = (t->is_running_snapshot_task() && (!t->is_completed())))
                    break;
            }
            if (has_running_snapshot_task){
                LOG(LOG_LEVEL_ERROR, L"Has running snapshot task for vm (%s).", vm->name.c_str());
                dwRtn = S_FALSE;
            }
            else{
                snapshot_internal_info snapshot_internal;
                get_snapshot_internal_from_list(vm->root_snapshot_list, snapshot_name, snapshot_internal);
                if (snapshot_internal.ref_item.empty())
                {
                    LOG(LOG_LEVEL_WARNING, L"Snapshot \"%s\" not found.", snapshot_name.c_str());
                    dwRtn = S_OK;
                }
                else
                {
                    revert_snapshot_req->id = snapshot_internal.id;
                    revert_snapshot_req->mor_ref = snapshot_internal.ref_item;

                    int nReturn = _portal[vm->uuid]->revert_virtual_machine_snapshot(vm->uuid, *revert_snapshot_req, boost::bind(&vmware_portal_ex::snapshot_progress_callback, this, _1, _2));
                    if (nReturn != TASK_STATE::STATE_SUCCESS)
                    {
                        LOG(LOG_LEVEL_ERROR, L"Failed to revert the temp snapshot %s:%d.", snapshot_name.c_str(), revert_snapshot_req->id);
                        dwRtn = S_FALSE;
                    }
                }
            }
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        dwRtn = S_FALSE;
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        dwRtn = S_FALSE;
    }

    return dwRtn;
}

VOID vmware_portal_ex::get_snapshot_ref_item(_In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name, _Out_ std::wstring& snapshot_ref_item){
    try
    {
        vmware_virtual_machine::ptr vm = _portal[machine_key]->get_virtual_machine(machine_key);
        if (vm != NULL)
        {
            snapshot_internal_info snapshot_internal;
            get_snapshot_internal_from_list(vm->root_snapshot_list, snapshot_name, snapshot_internal);
            if (snapshot_internal.ref_item.empty())
            {
                LOG(LOG_LEVEL_ERROR, L"Snapshot \"%s\" not found.", snapshot_name.c_str());
            }
            else
                snapshot_ref_item = snapshot_internal.ref_item;
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
    }
}

VOID vmware_portal_ex::get_snapshot_ref_item(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name, _Out_ std::wstring& snapshot_ref_item)
{
    FUN_TRACE;

    vmware_portal::ptr portal = NULL;

    auto search = _portal.find(machine_key);
    if (search == _portal.end())
    {
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        portal = vmware_portal::connect(uri, username, passwd);
        _portal[machine_key] = portal;
    }
    else
        portal = _portal[machine_key];

    if (portal)
        get_snapshot_ref_item(machine_key, snapshot_name, snapshot_ref_item);
    else
        LOG(LOG_LEVEL_ERROR, L"The specific HOST failed to connect.");
}
#ifdef VIXMNTAPI
BOOL vmware_portal_ex::create_vmdk_file_local(const std::wstring& image_file, const disk_convert_settings& create_flags, const uint64_t& number_of_sectors_size)
{
    FUN_TRACE;

    VixDiskLibCreateParams create_params;

    create_params.adapterType = create_flags.adapter_type;
    create_params.diskType = create_flags.disk_type;
    create_params.hwVersion = create_flags.hw_version;
    create_params.capacity = number_of_sectors_size;

    VixError vixerror = VixDiskLib_Create(_connect, macho::stringutils::convert_unicode_to_utf8(image_file).c_str(), &create_params, NULL, NULL);
    if (VIX_FAILED(vixerror))
    {
        LOG(LOG_LEVEL_ERROR, L"Create local image file failure => %s, error = %d.", get_native_error(vixerror).c_str(), VIX_ERROR_CODE(vixerror));
        return FALSE;
    }

    return TRUE;
}
#endif
DWORD vmware_portal_ex::get_vmdk_list(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _Inout_ std::map<std::wstring, uint64>& vmdk_list)
{
    FUN_TRACE;

    DWORD dwRtn = S_OK;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
    vmware_portal::ptr portal = vmware_portal::connect(uri, username, passwd);

    if (portal)
    {
        try
        {
            vmware_virtual_machine::ptr vm = portal->get_virtual_machine(machine_key);
            if (vm != NULL)
            {
                foreach(vmware_disk_info::map::value_type& disk, vm->disks_map){
                    vmdk_list[disk.first] = disk.second->size;
                }
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing.");
                dwRtn = S_FALSE;
            }
        }
        catch (macho::exception_base& e){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        }
        catch (boost::exception const& e)
        {
            LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        }

        portal = NULL;
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"The specific HOST failed to connect.");
        dwRtn = S_FALSE;
    }

    return dwRtn;
}

vmware_vixdisk_connection_ptr vmware_portal_ex::get_vixdisk_connection(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring snapshot_name, _In_ const bool read_only){
    connect_params conn_params_req;
    vmware_vixdisk_connection::ptr conn = vmware_vixdisk_connection_ptr(new vmware_vixdisk_connection());
    conn_params_req.ip = host;
    conn_params_req.uname = username;
    conn_params_req.passwd = passwd;
    conn_params_req.vmx_spec = L"";
    vmware_portal::ptr portal = NULL;
    vmware_virtual_machine::ptr vm = NULL;
    hv_connection_type connection_type = hv_connection_type::HV_CONNECTION_TYPE_UNKNOWN;
    auto search = _portal.find(machine_key);
    if (search == _portal.end())
    {
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        portal = vmware_portal::connect(uri, username, passwd);
        _portal[machine_key] = portal;
    }
    else
        portal = _portal[machine_key];

    //get vmx spec
    if (portal)
    {
        vm = portal->get_virtual_machine(machine_key);
        portal = NULL;

        if (vm != NULL)
        {
            if (vm->connection_type == HV_CONNECTION_TYPE_VCENTER || boost::starts_with(vm->vm_mor_item, L"vm-")){
                connection_type = vm->connection_type;
                conn_params_req.vmx_spec = L"moref=" + vm->vm_mor_item;
            }
            else
                conn_params_req.vmx_spec = vm->config_path_file;
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"The specific VM doesn't existing, abort clone task.");
            return NULL;
        }
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to login portal.");
        return NULL;
    }

    try
    {
        prepare_connection_param(&conn_params_req, &conn->_connect_params);

        if (conn->_connect_params.thumbPrint == NULL)
        {
            LOG(LOG_LEVEL_ERROR, L"Failed to get server thumbprint data, abort clone task.");
            return NULL;
        }
    }
    catch (macho::exception_base& e){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(e).c_str());
        return NULL;
    }
    catch (boost::exception const& e)
    {
        LOG(LOG_LEVEL_ERROR, L"%s.", macho::stringutils::convert_utf8_to_unicode(boost::diagnostic_information(e)).c_str());
        return NULL;
    }
    VixError vixerror;
    std::wstring snapshot_ref_item = L"";
    if (!snapshot_name.empty())
        get_snapshot_ref_item(machine_key, snapshot_name, snapshot_ref_item);

    if (!boost::starts_with(conn_params_req.vmx_spec, "moref=vm-"))
        vixerror = VixDiskLib_ConnectEx(&conn->_connect_params, read_only, macho::stringutils::convert_unicode_to_utf8(snapshot_ref_item).c_str(), TRANSPORT_MODES, &conn->_connection); // direct to esx server
    else
        vixerror = VixDiskLib_Connect(&conn->_connect_params, &conn->_connection); // via vcenter server

    if (VIX_SUCCEEDED(vixerror))
    {
        if (hv_connection_type::HV_CONNECTION_TYPE_VCENTER == connection_type )
            conn->prepare_for_access();
        return conn;
    }
    return NULL;
}

universal_disk_rw::ptr vmware_portal_ex::open_vmdk_for_rw(vmware_vixdisk_connection_ptr& connect, const std::wstring& image_file, bool read_only)
{
    vmware_vmdk_file_rw* img_obj = vmware_vmdk_file_rw::connect(connect, image_file, read_only);
    if (img_obj){
        return universal_disk_rw::ptr(img_obj);
    }
    return NULL;
}

#ifdef VIXMNTAPI
universal_disk_rw::ptr vmware_portal_ex::open_vmdk_for_rw(const std::wstring& image_file, bool read_only)
{
    bool                 is_remote = false;
    vmware_vmdk_file_rw *vmdk = vmware_vmdk_file_rw::connect(_connect, image_file, is_remote);
    if (vmdk)
        return universal_disk_rw::ptr(vmdk);
    return NULL;
}
#endif

bool vmware_portal_ex::interrupt(bool is_cancel)
{
    FUN_TRACE;

    LOG(LOG_LEVEL_INFO, L"Interrupt captured.");

    _is_interrupted = true;
    for (std::map<std::wstring, vmware_portal::ptr>::iterator it = _portal.begin(); it != _portal.end(); it++)
    {
        it->second->interrupt(is_cancel);
    }

    return _is_interrupted;
}