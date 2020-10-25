// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IRM_HYPERVIOSR_EX_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IRM_HYPERVIOSR_EX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#ifndef __IRONMAN_HYPERVISOR_EX__
#define __IRONMAN_HYPERVISOR_EX__

#include "boost\shared_ptr.hpp"
#include <map>
#include <set>
#include "macho.h"
#include <algorithm>

#include "vixDiskLib.h"
#ifdef VIXMNTAPI
#include "vixMntapi.h"
#endif
#include "vmware.h"
#include "universal_disk_rw.h"

#pragma warning( disable : 4251 )

#ifdef IRM_HYPERVIOSR_EX_EXPORTS
#define IRM_HYPERVIOSR_EX_API __declspec(dllexport)
#else
#define IRM_HYPERVIOSR_EX_API __declspec(dllimport)
#endif

using namespace mwdc::ironman::hypervisor;

namespace mwdc { namespace ironman { namespace hypervisor_ex {

enum
{
    IMAGE_TYPE_VMDK = 0,
    IMAGE_TYPE_VHDX = 1
};

typedef struct 
{
    uint32 cylinders;
    uint32 heads;
    uint32 sectors;
} disk_geometry;

typedef struct 
{
    std::wstring family;        // OS Family
    uint32 major_version;       // On Windows, 4=NT, 5=2000 and above
    uint32 minor_version;       // On Windows, 0=2000, 1=XP, 2=2003
    Bool  is_64Bit;             // True if the OS is 64-bit
    std::wstring vendor;        // e.g. Microsoft, RedHat, etc ...
    std::wstring edition;       // e.g. Desktop, Enterprise, etc ...
    std::wstring os_folder;     // Location where the default OS is installed
} os_info;

#ifdef VIXMNTAPI
struct volume_info
{
    typedef boost::shared_ptr<volume_info> ptr;
    typedef std::vector<ptr> vtr;
    volume_info() : 
        symbolic_link(L""),
        type(L""){}
    std::wstring type;                                  // Type of the volume
    Bool         isMounted;                             // True if the volume is mounted on the proxy.
    std::wstring symbolic_link;                         // Path to the volume mount point, NULL if the volume is not mounted on the proxy.
    size_t  num_guest_mount_points;                     // Number of mount points for the volume in the guest, 0 if the volume is not mounted on the proxy.
    std::vector<std::wstring> in_guest_mount_points;    // Mount points for the volume in the guest
};

struct vol_mounted_info
{
    typedef boost::shared_ptr<vol_mounted_info> ptr;
    typedef std::vector<ptr> vtr;
    vol_mounted_info() : mount_point(L"") {}
    std::wstring mount_point;
    volume_info::ptr vol_info;
};

struct vol_handle_info
{
    typedef boost::shared_ptr<vol_handle_info> ptr;
    typedef std::vector<ptr> vtr;
    VixVolumeHandle vol_handle;
    vol_mounted_info::ptr vol_mounted_info;
};

struct mounted_memory_repo
{
    typedef boost::shared_ptr<mounted_memory_repo> ptr;
    typedef std::map<std::wstring, ptr> map;
    mounted_memory_repo() : 
        image_name(L""),
        vol_handles(NULL){}
    virtual ~mounted_memory_repo();
    VixDiskLibHandle disk_handle;
    VixDiskSetHandle diskset_handle;
    VixVolumeHandle *vol_handles;
    std::wstring image_name;
    vol_handle_info::vtr mounted_info;
};
#endif
struct image_info
{
    typedef boost::shared_ptr<image_info> ptr;
    typedef std::vector<ptr> vtr;
    image_info() : 
        disk_signature(L""),
        parent_filename_hint(L"") {}
    disk_geometry bios;                             // BIOS geometry for booting and partitioning
    disk_geometry physical;                         // physical geometry
    os_info os;
    uint64 total_sectors;                           // capacity in sectors
    int num_links;                                  // number of links (i.e. base disk + redo logs)
    std::wstring adapter_type;
    std::wstring parent_filename_hint;              // parent file for a redo log
    std::wstring disk_signature;                    // UUID
#ifdef VIXMNTAPI
    volume_info::vtr volumes;
#endif
    std::vector<char> metakeys;
};

struct connect_params
{
    typedef boost::shared_ptr<connect_params> ptr;
    connect_params() : 
        portal_port(902),
        portal_ssl_port(443) {}
    std::wstring ip;
    int portal_port;
    int portal_ssl_port;
    std::wstring uname;
    std::wstring passwd;
    std::wstring vmx_spec;
    std::string thumbprint;
};

#ifdef VIXMNTAPI

typedef struct _login_cred_data
{
    _login_cred_data() :
        portal_port(902),
        portal_ssl_port(443) {}
    std::wstring ip;
    int portal_port;
    int portal_ssl_port;
    std::wstring uname;
    std::wstring passwd;
}login_cred;

typedef struct _disk_convert_settings
{
    _disk_convert_settings() :
        adapter_type(VIXDISKLIB_ADAPTER_IDE),
        hw_version(VIXDISKLIB_HWVERSION_ESX30),
        disk_type(VIXDISKLIB_DISK_MONOLITHIC_SPARSE){}
    VixDiskLibAdapterType adapter_type;
    uint16 hw_version;
    VixDiskLibDiskType disk_type;
} disk_convert_settings;

struct image_clone_params
{
    typedef boost::shared_ptr<connect_params> ptr;
    std::wstring src_machine_key;
    std::wstring src_image_path_file;
    login_cred src;
    login_cred dst;
    std::wstring dst_image_path_file;
    disk_convert_settings convert;
};
#endif

struct snapshot_internal_info
{
    typedef boost::shared_ptr<snapshot_internal_info> ptr;
    snapshot_internal_info() : id(-1) {}
    std::wstring ref_item;
    std::wstring name;
    int id;
};

class IRM_HYPERVIOSR_EX_API vmware_portal_ : public boost::noncopyable
{
public:   
    virtual ~vmware_portal_(); 
    typedef boost::signals2::signal<void(const std::wstring&, const int&)> operation_progress;
    typedef boost::shared_ptr<vmware_portal_> ptr;
    virtual vmware_host::vtr get_hosts(const std::wstring &host_key = std::wstring());
    virtual vmware_host::ptr get_host(const std::wstring &host_key);
    virtual vmware_datacenter::vtr get_datacenters(const std::wstring &name = std::wstring());
    virtual vmware_datacenter::ptr get_datacenter(const std::wstring &name);
    virtual vmware_cluster::vtr get_clusters(const std::wstring &cluster_key = std::wstring());
    virtual vmware_cluster::ptr get_cluster(const std::wstring &cluster_key);
    virtual key_map get_all_virtual_machines();
    virtual vmware_virtual_machine::vtr get_virtual_machines(const std::wstring &host_key = std::wstring(), const std::wstring &machine_key = std::wstring());
    virtual vmware_virtual_machine::ptr get_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual vmware_vm_task_info::vtr get_virtual_machine_tasks(const std::wstring &machine_key);

    virtual vmware_virtual_machine::ptr create_virtual_machine(const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr modify_virtual_machine(const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec);
    virtual vmware_virtual_machine::ptr clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot);
    virtual bool delete_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_on_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_off_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool unregister_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual vmware_virtual_machine::ptr add_existing_virtual_machine(const vmware_add_existing_virtual_machine_spec &spec);

    virtual int create_virtual_machine_snapshot(const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int remove_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int revert_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual bool enable_change_block_tracking(const std::wstring &machine_key);
    virtual bool disable_change_block_tracking(const std::wstring &machine_key);
    virtual bool mount_vm_tools(const std::wstring &machine_key);
    virtual vmdk_changed_areas get_vmdk_changed_areas(const std::wstring &vm_mor_item, const std::wstring &snapshot_mor_item, const std::wstring &device_key, const std::wstring &changeid, const LONG64 start_offset, const LONG64 disk_size);
    static  vmware_portal_::ptr connect(std::wstring uri, std::wstring user, std::wstring password);
    static  bool login_verify(std::wstring uri, std::wstring user, std::wstring password);
    virtual vmware_snapshot_disk_info::map  get_snapshot_info(std::wstring& snapshot_mof, vmware_snapshot_disk_info::vtr& snapshot_disk_infos = vmware_snapshot_disk_info::vtr());
    virtual mwdc::ironman::hypervisor::hv_connection_type get_connection_type(vmware_virtual_center &virtual_center = vmware_virtual_center());
    bool    interrupt(bool is_cancel = false);
    void    reset_flags();
    virtual int		     get_last_error_code();
    virtual std::wstring get_last_error_message();

private:
    friend class vmware_portal_ex;
    vmware_portal_();
    vmware_portal::ptr portal;
};

class vmware_vixdisk_connection;
typedef boost::shared_ptr<vmware_vixdisk_connection> vmware_vixdisk_connection_ptr;

// This class is exported from the irm_hyperviosr_ex.dll
class IRM_HYPERVIOSR_EX_API vmware_portal_ex : public boost::noncopyable
{
public:
    typedef boost::signals2::signal<void(const std::wstring&, const std::wstring&, const std::wstring&, const uint64&, const uint64&, const uint64&)> clone_image_progress;
    typedef boost::shared_ptr<vmware_portal_ex> ptr;
    struct  exception : virtual public macho::exception_base {};
    virtual ~vmware_portal_ex();
    vmware_portal_ex();
    vmware_portal_ex(vmware_portal::ptr portal, std::wstring& machine_key) : vmware_portal_ex() { if (portal != NULL && !machine_key.empty()) _portal[machine_key] = portal; }
    vmware_portal_ex(vmware_portal_::ptr portal, std::wstring& machine_key) : vmware_portal_ex() { if (portal != NULL && !machine_key.empty()) _portal[machine_key] = portal->portal; }
    
    void set_portal(vmware_portal::ptr portal, std::wstring& machine_key) { if (portal != NULL && !machine_key.empty()) _portal[machine_key] = portal; }
    void set_portal(vmware_portal_::ptr portal, std::wstring& machine_key) { if (portal != NULL && !machine_key.empty()) _portal[machine_key] = portal->portal; }
    void erase_portal(std::wstring& machine_key) { _portal.erase(machine_key); }

#ifdef VIXMNTAPI
    virtual vol_mounted_info::vtr mount_image_local(_In_ const std::wstring& image_name, _In_ BOOL read_only = false);
    virtual BOOL unmount_image_local(_In_ const std::wstring& image_name = L"", _In_ BOOL force = true);
    virtual image_info::ptr get_image_info_local(_In_ const std::wstring& image_name);
    virtual DWORD clone_image_to_local(_In_ const image_clone_params& clone_req, _In_ clone_image_progress::slot_type slot, _In_ int image_type = IMAGE_TYPE_VMDK);
    virtual DWORD clone_image_to_local_type(_In_ universal_disk_rw::ptr in_file, _In_ universal_disk_rw::ptr out_file, std::wstring& snapshot_name, _Inout_ uint64& offset_read_bytes, _Inout_ uint64& offset_written_bytes, _In_ clone_image_progress::slot_type slot, _In_ int image_type = IMAGE_TYPE_VMDK);
    virtual BOOL create_vmdk_file_local(_In_ const std::wstring& image_file, _In_ const disk_convert_settings& create_flags, _In_ const uint64_t& number_of_sectors_size);
    virtual universal_disk_rw::ptr open_vmdk_for_rw(_In_ const std::wstring& image_file, _In_ bool read_only = false);
#endif

    virtual image_info::ptr get_image_info_remote(_In_ const std::wstring& host, _In_ const std::wstring& uname, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring& image_name);
    virtual DWORD create_temp_snapshot(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _Inout_ std::wstring& snapshot_name);
    virtual DWORD remove_temp_snapshot(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name);
    virtual DWORD revert_temp_snapshot(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name);

    virtual DWORD create_temp_snapshot(_In_ const vmware_virtual_machine::ptr& vm, _Inout_ std::wstring& snapshot_name);
    virtual DWORD remove_temp_snapshot(_In_ const vmware_virtual_machine::ptr& vm, _In_ const std::wstring& snapshot_name);
    virtual DWORD revert_temp_snapshot(_In_ const vmware_virtual_machine::ptr& vm, _In_ const std::wstring& snapshot_name);

    virtual DWORD get_vmdk_list(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _Inout_ std::map<std::wstring, uint64>& vmdk_list);
    virtual vmware_vixdisk_connection_ptr get_vixdisk_connection(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring snapshot_name = L"", _In_ const bool read_only = true);
    virtual universal_disk_rw::ptr open_vmdk_for_rw(vmware_vixdisk_connection_ptr& connect, _In_ const std::wstring& image_file, _In_ bool read_only = true);
    virtual VOID get_snapshot_ref_item( _In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name, _Out_ std::wstring& snapshot_ref_item);
    virtual VOID get_snapshot_ref_item(_In_ const std::wstring& host, _In_ const std::wstring& username, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _In_ const std::wstring& snapshot_name, _Out_ std::wstring& snapshot_ref_item);    
    static void set_log(std::wstring log_file, macho::TRACE_LOG_LEVEL level = macho::TRACE_LOG_LEVEL::LOG_LEVEL_WARNING);
    bool interrupt(bool is_cancel = false);

    static bool vixdisk_init();
    static void vixdisk_exit();
    static std::wstring get_native_error(_In_ VixError& vixerror);
    static  DWORD get_ssl_thumbprint(const std::wstring& host, const int port, std::string& thumbprint);

private:
    // TODO: add your methods here.
    friend class vmware_vmdk_file_rw;

    bool                                        _init_status;
    bool                                        _is_interrupted;
    static bool                                 _init_vixlib;
    static int32_t                              _vixlib_ref_count;
#ifdef VIXMNTAPI
    mounted_memory_repo::map                    _mounted_repo;
    VixDiskLibConnection                        _connect;
    VixDiskLibConnectParams                     _connect_params;
#endif
    std::map<std::wstring, vmware_portal::ptr>  _portal;

    virtual std::wstring get_free_drive_letter();
    virtual std::wstring get_last_error_string_win();
    virtual void prepare_connection_param(_Inout_ connect_params *connect_params_req, _Inout_ VixDiskLibConnectParams *connect_params);
    virtual void get_snapshot_internal_from_list(_In_ vmware_virtual_machine_snapshots::vtr& snapshot_list, _In_ const std::wstring& snapshot_name, _Inout_ snapshot_internal_info& snapshot_internal);
    virtual void clone_image_to_vmdk(_In_ universal_disk_rw::ptr in_file, _In_ universal_disk_rw::ptr out_file, std::wstring& snapshot_name, _Inout_ uint64& offset_read_bytes, _Inout_ uint64& offset_written_bytes, _In_ clone_image_progress::slot_type slot);
    virtual void clone_image_to_vhdx(_In_ universal_disk_rw::ptr in_file, _In_ universal_disk_rw::ptr out_file, std::wstring& snapshot_name, _Inout_ uint64& offset_read_bytes, _Inout_ uint64& offset_written_bytes, _In_ clone_image_progress::slot_type slot);
    virtual image_info::ptr get_image_info_internal(_In_ const VixDiskLibConnection connect, _In_ const std::wstring& image_name);
    virtual image_info::ptr get_image_info_internal(_In_ const VixDiskLibHandle dsk_handle);
    DWORD get_vmxspec(_In_ const std::wstring& host, _In_ const std::wstring& uname, _In_ const std::wstring& passwd, _In_ const std::wstring& machine_key, _Out_ std::wstring& vmxspec);
    VOID snapshot_progress_callback(_In_ const std::wstring& snapshot_name, _In_ const int& progess);
};
class vmware_vmdk_file_rw;
class vmware_vixdisk_connection{
public:
    typedef boost::shared_ptr<vmware_vixdisk_connection> ptr;
    ~vmware_vixdisk_connection();
private:
    friend class vmware_vmdk_file_rw;
    friend class vmware_portal_ex;
    vmware_vixdisk_connection() : _is_prepare_for_access(false){
        _connection = { 0 };
        _connect_params = { 0 };
    }
    void                        prepare_for_access();
    void                        end_access();
    VixDiskLibConnection        _connection;
    VixDiskLibConnectParams     _connect_params;
    bool                        _is_prepare_for_access;
};

class vmware_vmdk_file_rw : public virtual universal_disk_rw
{
public:
    ~vmware_vmdk_file_rw();
    static vmware_vmdk_file_rw* connect(VixDiskLibConnection& connection, const std::wstring& virtual_disk_path, bool readonly);
    static vmware_vmdk_file_rw* connect(vmware_vixdisk_connection::ptr& connection, const std::wstring& virtual_disk_path, bool readonly);
    virtual bool read(_In_ uint64_t start, _In_ uint32_t number_of_sectors_to_read, _Inout_ LPVOID buffer, _Inout_ uint32_t& number_of_sectors_read);
    virtual bool write(_In_ uint64_t start, _In_ LPCVOID buffer, _In_ uint32_t number_of_sectors_to_write, _Inout_ uint32_t& number_of_sectors_written);
    virtual bool write_meta_data(_In_ LPVOID key, _In_ LPCVOID value);
    virtual bool read_meta_data(_In_ LPVOID key, _Inout_ void **value);
    virtual image_info::ptr get_image_info();
    virtual std::wstring path() const { return _virtual_disk_path; }
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone() {
        return universal_disk_rw::ptr(rw_clone());
    }
    virtual vmware_vmdk_file_rw* rw_clone();
private:
    friend class vmware_portal_ex;
    vmware_vmdk_file_rw();
    vmware_vixdisk_connection::ptr   _connection;
    VixDiskLibHandle                 _handle;
    std::wstring                     _virtual_disk_path;
    bool                             _readonly;
    macho::windows::critical_section _cs;
};

struct IRM_HYPERVIOSR_EX_API vmware_vmdk_mgmt
{
#ifdef VIXMNTAPI
    static BOOL create_vmdk_file(_In_ vmware_portal_ex::ptr portal_ex, std::wstring image_file, _In_ disk_convert_settings create_flags, _In_ uint64_t number_of_sectors_size);
    static BOOL create_vmdk_file(_In_ vmware_portal_ex *portal_ex, _In_ std::wstring image_file, _In_ disk_convert_settings create_flags, _In_ uint64_t number_of_sectors_size);
    static universal_disk_rw::ptr open_vmdk_for_rw(_In_ vmware_portal_ex *portal_ex, _In_ std::wstring image_file, _In_ bool read_only);
    static universal_disk_rw::ptr open_vmdk_for_rw(_In_ vmware_portal_ex::ptr portal_ex, _In_ std::wstring image_file, _In_ bool read_only);
#endif
    static VOID get_vm_vmdk_list(_In_ vmware_portal_ex::ptr portal_ex, _In_ std::wstring host, _In_ std::wstring username, _In_ std::wstring passwd, _In_ std::wstring machine_key, _Inout_ std::map<std::wstring, uint64>& vmdk_list);
    static universal_disk_rw::ptr open_vmdk_for_rw(_In_ vmware_portal_ex::ptr portal_ex, _In_ std::wstring host, _In_ std::wstring username, _In_ std::wstring passwd, _In_ std::wstring machine_key, _In_ std::wstring image_file, _In_ std::wstring snapshot_name, _In_ bool read_only);
    static universal_disk_rw::ptr open_vmdk_for_rw(_In_ vmware_portal_ex *portal_ex, _In_ std::wstring host, _In_ std::wstring username, _In_ std::wstring passwd, _In_ std::wstring machine_key, _In_ std::wstring image_file, _In_ std::wstring snapshot_name, _In_ bool read_only);
};

}}}

#endif