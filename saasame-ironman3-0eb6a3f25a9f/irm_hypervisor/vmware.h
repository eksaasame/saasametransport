#pragma once

#ifndef __IRONMAN_HYPERVISOR__
#define __IRONMAN_HYPERVISOR__

#include "boost\shared_ptr.hpp"
#include <map>
#include <set>
#include "macho.h"
#include <algorithm>

#pragma warning( push )
#pragma warning( disable : 4251 )
#include "soapVimBindingProxy.h"
#pragma warning( pop )
#include "vmware_def.h"

namespace mwdc { namespace ironman { namespace hypervisor {

typedef std::map< std::wstring, std::wstring > mor_name_map;
typedef std::map< std::wstring, Vim25Api1__ManagedObjectReference*> mor_map;
struct key_name{
    typedef std::map< std::wstring, key_name > map;
    std::wstring key;
    std::wstring name;
};
typedef std::set< std::wstring, macho::stringutils::no_case_string_less> string_set;
typedef std::map<std::wstring, std::wstring, macho::stringutils::no_case_string_less> key_map;
typedef std::map<std::wstring, std::vector<std::wstring>> license_map;

enum DEV_KEY{
    UNKNOWN_DEV     =        -1,
    SCSICTRL        =        1000,
    DISK            =        2000,
    CDROM           =        3000,
    NIC             =        4000,
    FLOPPY          =        8000,
    SERIAL_PORT     =        9000
};

enum DEV_CONFIGSPEC_OP{
    OP_ADD = 0,
    OP_REMOVE = 1,
    OP_EDIT = 2,
    OP_UNSPECIFIED = -1
};

enum DEV_CONFIGSPEC_FILEOP{
    FILEOP_CREATE = 0,
    FILEOP_DESTROY = 1,
    FILEOP_REPLACE = 2,
    FILEOP_UNSPECIFIED = -1
};

enum TASK_STATE{
    STATE_UNKNOWN       = -1,
    STATE_QUEUED        =  0,
    STATE_RUNNING       =  1,
    STATE_SUCCESS       =  2,
    STATE_ERROR         =  3,
    STATE_TIMEOUT       =  4,
    STATE_CANCELLED     =  5
};

class journal_transaction;

struct vmware_folder{
    typedef boost::shared_ptr<vmware_folder> ptr;
    typedef std::vector<ptr> vtr;
    bool get_vm_folder_path(const std::wstring vm_key, std::wstring & path){
        std::wstring newPath = boost::str(boost::wformat( L"%s/%s") %path %name);
        key_map::const_iterator iVm = vms.find(vm_key);
        if (iVm != vms.end()){
            path = newPath;
            return true;
        }

        for (size_t i = 0; i < folders.size(); i++){
            if (folders.at(i)->get_vm_folder_path(vm_key, newPath)){
                path = newPath;
                return true;
            }
        }
        return false;
    }
    
    std::wstring       name;
    vmware_folder::vtr folders;
    key_map            vms;
    vmware_folder::ptr parent;
    std::wstring path() {
        std::wstring _parent;
        if (parent){
            _parent = parent->path();
        }
        return boost::str(boost::wformat(L"%s/%s")%_parent%name);
    }
};

struct vmware_datacenter{
    typedef boost::shared_ptr<vmware_datacenter> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring       name;
    vmware_folder::vtr folders;
};

struct vmware_virtual_network{
    typedef boost::shared_ptr<vmware_virtual_network> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring key;
    std::wstring name;
    std::wstring distributed_port_group_key;
    std::wstring distributed_switch_uuid;
    macho::string_array hosts;
    macho::string_array vms;
    bool is_distributed() const { return !(distributed_port_group_key.empty() || distributed_switch_uuid.empty()); }
};

struct vmware_virtual_network_adapter{
    vmware_virtual_network_adapter() :
        is_connected(false)
        , is_start_connected(false)
        , key(-1)
        , is_allow_guest_control(false){}
    typedef boost::shared_ptr<vmware_virtual_network_adapter> ptr;
    typedef std::vector<ptr> vtr;
    int                     key;                 // Unique identifier
    std::wstring            name;                // Device label for VMware 
    std::wstring            mac_address;         // Will be generated if not specified
    std::wstring            network;             // Network NIC is connected to
    std::wstring            port_group;          // VMware distributed network only
    std::wstring            type;                // Type of network card - dafaults to E1000 for VMware 
    bool                    is_connected;        // Is currently connected
    bool                    is_start_connected;  // Connect when VM starts
    bool                    is_allow_guest_control;
    std::wstring            address_type;
};

struct vmware_virtual_scsi_controller {
    typedef boost::shared_ptr<vmware_virtual_scsi_controller> ptr;
    typedef std::vector<ptr> vtr;
    vmware_virtual_scsi_controller() : type(HV_CTRL_ANY), sharing(HV_BUS_SHARING_NONE), bus_number(-1), key(-1){
        slots.resize(16);
        slots[7] = true;
    }
    hv_controller_type type;
    hv_bus_sharing     sharing;
    int                key;                 // Unique identifier
    int                bus_number;
    std::vector<bool>  slots;
};  

struct vmware_vm_remove_snapshot_parm
{
    vmware_vm_remove_snapshot_parm() :
        id(-1),
        remove_children(false),
        consolidate(false){}
    typedef boost::shared_ptr<vmware_vm_remove_snapshot_parm> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring name;          /* required element of type xsd:string */
    std::wstring mor_ref;       /* required element of type xsd:string */
    int id;                     /* required element of type xsd:int */
    bool remove_children;       /* required element of type xsd:boolean */
    bool consolidate;           /* optional element of type xsd:boolean */
};

struct vmware_vm_create_snapshot_parm
{
    vmware_vm_create_snapshot_parm() :
        description(L""),
        include_memory(false),
        quiesce(true){}
    typedef boost::shared_ptr<vmware_vm_create_snapshot_parm> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring name;          /* In, required element of type xsd:string */
    std::wstring description;   /* In, optional element of type xsd:string */
    bool include_memory;        /* In, required element of type xsd:boolean */
    bool quiesce;               /* In, required element of type xsd:boolean */
    std::wstring snapshot_moref;/* Out */
};

struct vmware_vm_revert_to_snapshot_parm
{
    typedef boost::shared_ptr<vmware_vm_revert_to_snapshot_parm> ptr;
    vmware_vm_revert_to_snapshot_parm() :
        suppress_power_on(false), id(-1){}
    std::wstring mor_ref;     
    int id;                     
    bool suppress_power_on;
};

struct vmware_vm_task_info
{
    vmware_vm_task_info() :
        state(TASK_STATE::STATE_UNKNOWN),
        progress(0),
        cancelled(false),
        cancelable(false) {
    }
    typedef boost::shared_ptr<vmware_vm_task_info> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring                        name;              /* InOUT */
    std::wstring                        entity_name;       /* Out */
    std::wstring                        vm_uuid;           /* Out */
    std::wstring                        state_name;        /* Out */
    TASK_STATE                          state;             /* Out */
    bool                                cancelable;        /* Out */
    bool                                cancelled;         /* Out */
    int                                 progress;          /* Out */
    std::wstring                        moref;             /* Out */
    std::wstring                        moref_type;
    std::wstring                        error;             
    boost::posix_time::ptime            start;
    boost::posix_time::ptime            complete;
    bool                                is_running(){ return state == TASK_STATE::STATE_RUNNING; }
    bool                                is_queued(){ return state == TASK_STATE::STATE_QUEUED; }
    bool                                is_success(){ return state == TASK_STATE::STATE_SUCCESS; }
    bool                                is_error(){ return state == TASK_STATE::STATE_ERROR; }
    bool                                is_completed() { return complete != boost::posix_time::not_a_date_time; }
    bool                                is_create_snapshot_task() { return name == L"CreateSnapshot_Task"; }
    bool                                is_running_snapshot_task() { 
        return name == L"CreateSnapshot_Task" || 
        name == L"RevertToCurrentSnapshot_Task" || 
        name == L"RemoveSnapshot_Task" ||
        name == L"RevertToSnapshot_Task"||
        name == L"RemoveAllSnapshots_Task";
    }
};

struct vmware_virtual_machine_snapshots
{
    vmware_virtual_machine_snapshots() :
        quiesced(false),
        id(-1),
        replay_supported(false){}
    typedef boost::shared_ptr<vmware_virtual_machine_snapshots> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring snapshot_mor_item;                 /* a reference to VirtualMachineSnapshot managed object, required element of type xsd:string */
    std::wstring name;                              /* required element of type xsd:string */
    std::wstring description;                       /* required element of type xsd:string */
    time_t create_time;                             /* required element of type xsd:dateTime */
    bool quiesced;                                  /* required element of type xsd:boolean */
    int id;                                         /* optional element of type xsd:int */
    std::wstring backup_manifest;                   /* optional element of type xsd:string */
    std::vector<ptr> child_snapshot_list;           /* optional element of type vmware_virtual_machine_snapshots */
    bool replay_supported;                          /* optional element of type xsd:boolean */
};

struct vmware_disk_info
{
    typedef boost::shared_ptr<vmware_disk_info> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr, macho::stringutils::no_case_string_less_w> map;
    vmware_disk_info() : size(0), unit_number(-1), key(-1), thin_provisioned(false){}
    std::wstring                           wsz_key() { return boost::str(boost::wformat(L"%d") % key); }
    std::wstring                           name;
    std::wstring                           uuid;
    uint64_t                               size;
    int                                    key;
    int                                    unit_number;
    bool                                   thin_provisioned;
    vmware_virtual_scsi_controller::ptr    controller;
};

struct vmware_snapshot_disk_info : public vmware_disk_info
{
    typedef boost::shared_ptr<vmware_snapshot_disk_info> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr, macho::stringutils::no_case_string_less_w> map;
    vmware_snapshot_disk_info() : vmware_disk_info(){}
    std::wstring                   change_id;
    //vmware_snapshot_disk_info::ptr parent;
};

struct guest_nic_info
{
    std::wstring mac_address;
    std::vector<std::wstring> ip_config;
};

struct vmware_device{
    typedef boost::shared_ptr<vmware_device> ptr;
    typedef std::vector<ptr> vtr;
    vmware_device() : controller_key(0), key(0) {}
    int   controller_key;
    int   key;
};

struct vmware_virtual_machine{
    typedef boost::shared_ptr<vmware_virtual_machine> ptr;
    typedef std::vector<ptr> vtr;
    vmware_virtual_machine() : 
        is_cpu_hot_add(false),
        is_cpu_hot_remove(false),
        memory_mb(0),
        number_of_cpu(0),
        is_template(false),
        version(0),
        power_state(HV_VMPOWER_UNKNOWN),
        connection_state(HV_VMCONNECT_UNKNOWN),
        tools_status(HV_VMTOOLS_UNKNOWN),
        firmware(HV_VM_FIRMWARE_BIOS),
        guest_os_type(HV_OS_UNKNOWN),
        is_disk_uuid_enabled(false),
        is_cbt_enabled(false),
        has_cdrom(false),
        has_serial_port(false),
        connection_type(HV_CONNECTION_TYPE_UNKNOWN)
    {}
    virtual ~vmware_virtual_machine(){}
    std::wstring            key;           //VM GUID
    std::wstring            uuid;
    std::wstring            name;          //*
    std::wstring            host_key;
    std::wstring            host_ip;
    std::wstring            host_name;
    std::wstring            cluster_key;
    std::wstring            cluster_name;
    std::wstring            annotation; // Comments
    bool                    is_cpu_hot_add;
    bool                    is_cpu_hot_remove;
    long                    memory_mb;
    long                    number_of_cpu;
    bool                    is_template;
    std::wstring            config_path;
    std::wstring            config_path_file;
    std::wstring            vmxspec;
    long                    version;
    hv_vm_power_state       power_state;
    hv_vm_connection_state  connection_state;
    hv_vm_tools_status      tools_status;
    hv_vm_firmware          firmware;
    hv_guest_os_type        guest_os_type;
    std::wstring            guest_id;
    std::wstring            guest_full_name;
    std::wstring            guest_host_name;
    std::wstring            guest_primary_ip;
    std::vector<guest_nic_info> guest_net;
    bool                    is_disk_uuid_enabled;
    std::wstring            folder_path;
    std::wstring            resource_pool_path;
#if 0
    key_map                 disks;
    key_map                 disks_size;
    key_map                 disks_changeid;
#endif
    key_map                 networks;
    std::wstring            vm_mor_item;
    std::wstring            current_snapshot_mor_item;
    std::wstring            datacenter_name;
    std::wstring            datastore_browser_mor_item;
    std::wstring            parent_mor_item;
    bool                    is_cbt_enabled;
    vmware_disk_info::map   disks_map;
    vmware_disk_info::vtr   disks;
    vmware_virtual_network_adapter::vtr     network_adapters;
    vmware_virtual_machine_snapshots::vtr   root_snapshot_list;
    vmware_virtual_scsi_controller::vtr     scsi_controllers;
    hv_connection_type                      connection_type;
    bool                                    has_cdrom;
    bool                                    has_serial_port;
    vmware_device::ptr						cdrom;
};

struct vmware_virtual_machine_config_spec{
    vmware_virtual_machine_config_spec(): 
        number_of_cpu(1),
        memory_mb(128),
        firmware(HV_VM_FIRMWARE_BIOS),
        has_cdrom(false),
        has_serial_port(false)
        {
    }
    vmware_virtual_machine_config_spec(const vmware_virtual_machine& vm) : vmware_virtual_machine_config_spec() {
        memory_mb = vm.memory_mb;
        number_of_cpu = vm.number_of_cpu;
        name = vm.name;
        uuid = vm.uuid;
        host_key = vm.host_key;
        config_path = vm.config_path;
        resource_pool_path = vm.resource_pool_path;
        folder_path = vm.folder_path;
        annotation = vm.annotation;
        firmware = vm.firmware;
        annotation = vm.annotation;
        has_cdrom = vm.has_cdrom;
        has_serial_port = vm.has_serial_port;
        guest_id = vm.guest_id;
        foreach(const vmware_virtual_network_adapter::ptr &n, vm.network_adapters){
            nics.push_back(vmware_virtual_network_adapter::ptr(new vmware_virtual_network_adapter(*n.get())));
        }
        foreach(const vmware_disk_info::ptr &v, vm.disks){
            disks.push_back(vmware_disk_info::ptr(new vmware_disk_info(*v)));
        }
    }
    virtual ~vmware_virtual_machine_config_spec(){}
    long                             memory_mb;
    long                             number_of_cpu;
    std::wstring                     name;
    std::wstring                     uuid;
    std::wstring                     host_key;
    std::wstring                     config_path;
    std::wstring                     resource_pool_path;
    std::wstring                     folder_path;
    std::wstring                     annotation;
    std::wstring                     guest_id;
    hv_vm_firmware                   firmware;
    vmware_virtual_network_adapter::vtr nics;
    vmware_disk_info::vtr               disks;
    bool                                has_cdrom;
    bool                                has_serial_port;
    static std::wstring guest_os_id(hv_guest_os::type guest_os_type = hv_guest_os::type::HV_OS_UNKNOWN, hv_guest_os::architecture guest_os_architecture = hv_guest_os::architecture::X86) {
        switch (guest_os_type){
        case hv_guest_os::type::HV_OS_WINDOWS_VISTA:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"winVistaGuest" : L"winVista64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_7:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"windows7Guest" : L"windows7_64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_8:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"windows8Guest" : L"windows8_64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_8_1:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"windows8Guest" : L"windows8_64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_10:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"windows9Guest" : L"windows9_64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2003_WEB:
            return L"winNetWebGuest";
        case hv_guest_os::type::HV_OS_WINDOWS_2003_STANDARD:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"winNetStandardGuest" : L"winNetStandard64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2003_ENTERPRISE:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"winNetEnterpriseGuest" : L"winNetEnterprise64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2003_DATACENTER:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"winNetDatacenterGuest" : L"winNetDatacenter64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2003_BUSINESS:
            return L"winNetBusinessGuest";
        case hv_guest_os::type::HV_OS_WINDOWS_2008:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"winLonghornGuest" : L"winLonghorn64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2008R2:
            return L"windows7Server64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2012:
            return L"windows8Server64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2012R2:
            return L"windows8Server64Guest";
        case hv_guest_os::type::HV_OS_WINDOWS_2016:
            return L"windows9Server64Guest";
        case hv_guest_os::type::HV_OS_REDHAT_4:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"rhel4Guest" : L"rhel4_64Guest";
        case hv_guest_os::type::HV_OS_REDHAT_5:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"rhel5Guest" : L"rhel5_64Guest";
        case hv_guest_os::type::HV_OS_REDHAT_6:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"rhel6Guest" : L"rhel6_64Guest";
        case hv_guest_os::type::HV_OS_REDHAT_7:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"rhel7Guest" : L"rhel7_64Guest";
        case hv_guest_os::type::HV_OS_SUSE_10:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"sles10Guest" : L"sles10_64Guest";
        case hv_guest_os::type::HV_OS_SUSE_11:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"sles11Guest" : L"sles11_64Guest";
        case hv_guest_os::type::HV_OS_SUSE_12:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"sles12Guest" : L"sles12_64Guest";
        case hv_guest_os::type::HV_OS_CENTOS:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"centosGuest" : L"centos64Guest";
        case hv_guest_os::type::HV_OS_UBUNTU:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"ubuntuGuest" : L"ubuntu64Guest";
        case hv_guest_os::type::HV_OS_OPENSUSE:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"opensuseGuest" : L"opensuse64Guest";
        case hv_guest_os::type::HV_OS_DEBIAN_6:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"debian6Guest" : L"debian6_64Guest";
        case hv_guest_os::type::HV_OS_DEBIAN_7:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"debian7Guest" : L"debian7_64Guest";
        case hv_guest_os::type::HV_OS_DEBIAN_8:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"debian8Guest" : L"debian8_64Guest";
        case hv_guest_os::type::FREE_BSD:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"freebsdGuest" : L"freebsd64Guest";
        case hv_guest_os::type::ORACLE_LINUX:
            return guest_os_architecture == hv_guest_os::architecture::X86 ? L"oracleLinuxGuest" : L"oracleLinux64Guest";
        }
        return L"";
    }
};

struct vmware_clone_virtual_machine_config_spec{
    enum DISK_MOVE_TYPE{
        createNewChildDiskBacking = 0,
        moveAllDiskBackingsAndAllowSharing = 1,
        moveAllDiskBackingsAndConsolidate = 2,
        moveAllDiskBackingsAndDisallowSharing = 3,
        moveChildMostDiskBacking  = 4
    };
    enum RELOCATE_TRANSFORMATION_TYPE{
        Transformation__flat = 0,
        Transformation__sparse = 1
    };
    std::wstring   snapshot_mor_item;
    std::wstring   vm_name;
    std::wstring   datastore;
    std::wstring   uuid;
    std::wstring   folder_path;
    DISK_MOVE_TYPE disk_move_type;
    RELOCATE_TRANSFORMATION_TYPE transformation_type;
    vmware_clone_virtual_machine_config_spec() : disk_move_type(DISK_MOVE_TYPE::createNewChildDiskBacking), transformation_type(Transformation__sparse){}
};

struct vmware_add_existing_virtual_machine_spec{
    std::wstring    host_key;
    std::wstring    config_file_path;
    std::wstring    resource_pool_path;
    std::wstring    folder_path;
    std::wstring    uuid;
};

struct vmware_virtual_center{
    typedef boost::shared_ptr<vmware_virtual_center> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring product_name;
    std::wstring version;
};

struct vmware_cluster{
    typedef boost::shared_ptr<vmware_cluster> ptr;
    typedef std::vector<ptr> vtr;
    vmware_cluster() :
        is_ha(false), 
        is_drs(false), 
        number_of_hosts(0), 
        number_of_cpu_cores(0), 
        total_cpu(0), 
        total_memory(0), 
        current_evc_mode_key(false){}
    std::wstring key;
    std::wstring name;
    bool         is_ha;
    bool         is_drs;
    key_map      hosts;
    key_map      datastores;
    long         number_of_hosts;
    long         number_of_cpu_cores;
    long         total_cpu;
    int64_t      total_memory;
    bool         current_evc_mode_key;
    std::wstring datacenter;
    key_map      networks;
};

struct vmware_datastore{
    typedef boost::shared_ptr<vmware_datastore> ptr;
    typedef std::vector<ptr> vtr;
    vmware_datastore() :version_major(0), version_minor(0), is_accessible(false){}
    std::wstring name;
    std::wstring key;
    std::wstring uuid;
    hv_datastore_type type;
    macho::string_array lun_unique_names;
    int             version_major;
    int             version_minor;
    bool            is_accessible;
    key_map         hosts;
    key_map         vms;
};

struct vmware_host{
    vmware_host() :in_maintenance_mode(false), power_state(HV_HOSTPOWER_UNKNOWN), number_of_cpu_cores(0), number_of_cpu_packages(0), size_of_memory(0), number_of_cpu_threads(0){}
    typedef boost::shared_ptr<vmware_host> ptr;
    typedef std::vector<ptr> vtr;
    std::wstring        name_ref;
    std::wstring        uuid;
    macho::string_array name;
    macho::string_array key;
    macho::string_array ip_addresses;
    std::wstring        ip_address;
    std::wstring        product_name;
    std::wstring        version;
    hv_host_power_state power_state;
    std::wstring        state;
    bool                in_maintenance_mode;
    key_map             vms;
    key_map             datastores;
    key_map             networks;
    std::wstring        datacenter_name;
    macho::string_array domain_name;
    std::wstring        cluster_key;
    std::wstring        full_name(){
        if (domain_name.size())
            return boost::str(boost::wformat(L"%s.%s")%name.begin()->c_str()%domain_name.begin()->c_str());//TODO, need verify under cluster env
        return boost::str(boost::wformat(L"%s") % name.begin()->c_str()); //TODO, need verify under cluster env
    }
    license_map            features;
    hv_connection_type     connection_type;
    vmware_virtual_center  virtual_center;
    short				   number_of_cpu_cores;
    short				   number_of_cpu_packages;
    short                  number_of_cpu_threads;
    uint64_t			   size_of_memory;
};

struct changed_disk_extent
{
    typedef std::vector<changed_disk_extent> vtr;
    typedef std::map<std::wstring, changed_disk_extent::vtr> map;
    changed_disk_extent() : start(0), length(0){}
    changed_disk_extent(uint64_t _start, uint64_t _length) : start(_start), length(_length){}
    struct compare {
        bool operator() (changed_disk_extent const & lhs, changed_disk_extent const & rhs) const {
            return lhs.start < rhs.start;
        }
    };
    LONG64  start;
    LONG64  length;
};

class vmware_portal;
class service_connect{
public:
    typedef boost::shared_ptr<service_connect> ptr;
    explicit service_connect(vmware_portal *portal);
    virtual ~service_connect();
    operator Vim25Api1__ServiceContent*();
private:
    vmware_portal *_portal;
    LPVOID         _service_content;
};

struct vmdk_changed_areas
{
    vmdk_changed_areas() : last_error_code(0){}
    changed_disk_extent::vtr            changed_list;
    std::wstring                        error_description;
    int                                 last_error_code;
};

class vmware_portal : public boost::noncopyable{
public:
    typedef boost::signals2::signal<void(const std::wstring&, const int&)> operation_progress;
    typedef boost::shared_ptr<vmware_portal> ptr;
    struct  exception : virtual public macho::exception_base {};
    virtual ~vmware_portal(){ _connect = NULL; soap_destroy(&_vim_binding); soap_end(&_vim_binding); soap_done(&_vim_binding); }
    vmware_portal(std::wstring uri, std::wstring username, std::wstring password);
    virtual vmware_host::vtr get_hosts(const std::wstring &host_key = std::wstring());
    virtual vmware_host::ptr get_host(const std::wstring &host_key);
    virtual vmware_cluster::vtr get_clusters(const std::wstring &cluster_key = std::wstring());
    virtual vmware_cluster::ptr get_cluster(const std::wstring &cluster_key);
    virtual key_map get_all_virtual_machines();
    virtual vmware_virtual_machine::vtr get_virtual_machines(const std::wstring &host_key = std::wstring(), const std::wstring &machine_key = std::wstring());
    virtual vmware_virtual_machine::ptr get_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual vmware_virtual_machine::ptr create_virtual_machine(const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr modify_virtual_machine(const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec);
    virtual vmware_virtual_machine::ptr clone_virtual_machine(const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot);
    virtual bool delete_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_on_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_off_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool unregister_virtual_machine(const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual vmware_virtual_machine::ptr add_existing_virtual_machine(const vmware_add_existing_virtual_machine_spec &spec);

    virtual vmware_vm_task_info::vtr get_virtual_machine_tasks(const std::wstring &machine_key);

    virtual int create_virtual_machine_snapshot(const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int remove_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int revert_virtual_machine_snapshot(const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual bool enable_change_block_tracking(const std::wstring &machine_key);
    virtual bool disable_change_block_tracking(const std::wstring &machine_key);
    virtual bool mount_vm_tools(const std::wstring &machine_key);
    virtual vmdk_changed_areas get_vmdk_changed_areas(const std::wstring &vm_mor_item, const std::wstring &snapshot_mor_item, const std::wstring &device_key, const std::wstring &changeid, const LONG64 start_offset, const LONG64 disk_size);
    static  vmware_portal::ptr connect(std::wstring uri, std::wstring user, std::wstring password);
    static  bool login_verify(std::wstring uri, std::wstring user, std::wstring password);
    virtual vmware_snapshot_disk_info::map  get_snapshot_info(const std::wstring& snapshot_mof, vmware_snapshot_disk_info::vtr& snapshot_disk_infos = vmware_snapshot_disk_info::vtr());
    bool    interrupt(bool is_cancel = false);
    void    reset_flags() {
        _is_interrupted = false; _is_cancel = false; _last_error_code = 0; _last_error_message.clear();
    }
    virtual vmware_datacenter::vtr get_datacenters(const std::wstring &name = std::wstring());
    virtual vmware_datacenter::ptr get_datacenter(const std::wstring &name);
    virtual hv_connection_type     get_connection_type(vmware_virtual_center &virtual_center = vmware_virtual_center());
    virtual int		     get_last_error_code() const { return _last_error_code; }
    virtual std::wstring get_last_error_message() const { return _last_error_message;  }
private:
    friend class journal_transaction;
    friend class service_connect;
    vmware_portal() : _is_interrupted(false), _is_cancel(false), _last_error_code(0){};

    virtual float get_connection_info(Vim25Api1__ServiceContent& service_content, hv_connection_type &type, vmware_virtual_center &virtual_center = vmware_virtual_center());
    virtual vmware_host::vtr    get_hosts_internal(Vim25Api1__ServiceContent& service_content, std::wstring host_key = L"");
    virtual vmware_cluster::vtr get_clusters_internal(Vim25Api1__ServiceContent& service_content, std::wstring cluster_key = L"", bool host_only = false);
    virtual vmware_virtual_machine::vtr get_virtual_machines_internal(Vim25Api1__ServiceContent& service_content, std::wstring machine_key = L"", std::wstring host_key = L"", std::wstring cluster_key = L"");
    virtual key_name::map get_virtual_machines_key_name_map(Vim25Api1__ServiceContent& service_content);

    virtual vmware_vm_task_info::vtr get_virtual_machine_tasks_internal(Vim25Api1__ServiceContent& service_content, std::wstring machine_key = L"");
    virtual vmware_datacenter::vtr get_datacenters_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &datacenter);

    virtual vmware_virtual_network::vtr get_virtual_networks_internal(Vim25Api1__ServiceContent& service_content, std::vector<Vim25Api1__ManagedObjectReference*> hosts, const std::wstring& network_key);

    virtual vmware_folder::vtr          get_vm_folder_internal(Vim25Api1__ServiceContent& service_content, const std::wstring& datacenter, const key_name::map& vm_mor_key_name_map);
    virtual vmware_folder::vtr          get_vm_folder_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& folder, const key_name::map& vm_mor_key_name_map);
    virtual Vim25Api1__ManagedObjectReference* get_vm_folder_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& datacenter, const std::wstring& folder_path, bool can_be_create);
    virtual Vim25Api1__ManagedObjectReference* create_vm_folder_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& parentFolder, const std::wstring& name);
    virtual std::vector<Vim25Api1__ManagedObjectReference*> get_datacenters_mor(Vim25Api1__ServiceContent& service_content);
    virtual Vim25Api1__ManagedObjectReference* get_resource_pool_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& computeResource, const std::wstring& resource_pool_path);

    virtual Vim25Api1__ManagedObjectReference* get_datacenter_mor(Vim25Api1__ServiceContent& service_content, const std::wstring& datacenter);
    virtual Vim25Api1__ManagedObjectReference* get_host_mor(Vim25Api1__ServiceContent& service_content, const std::wstring& host_key);
    virtual VOID get_vm_mor_key_name_map(Vim25Api1__ServiceContent& service_content, key_name::map& more_key_name_map);
    virtual VOID get_host_mor_name_map(Vim25Api1__ServiceContent& service_content, mor_name_map& _mor_name_map);
    virtual VOID get_datastore_mor_key_name_map(Vim25Api1__ServiceContent& service_content, key_name::map& more_key_name_map);
    virtual VOID get_virtual_network_mor_key_name_map(Vim25Api1__ServiceContent& service_content, mor_name_map& _mor_name_map);
    virtual vmware_virtual_network::ptr get_virtual_network_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& network);
    virtual vmware_virtual_network_adapter::ptr get_virtual_network_adapter_from_object(Vim25Api1__ServiceContent& service_content, mor_name_map& network_mor_name_map, Vim25Api1__VirtualEthernetCard& virtual_ethernet_card);
    virtual vmware_datastore::ptr get_datastore_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& datastore, const mor_name_map& hosts = mor_name_map(), const key_name::map& vms = key_name::map());
    virtual vmware_datastore::ptr get_datastore_internal(Vim25Api1__ObjectContent& object_content, const mor_name_map& hosts = mor_name_map(), const key_name::map& vms = key_name::map());
    virtual vmware_virtual_machine_snapshots::ptr get_virtual_machine_snapshot_from_object(Vim25Api1__VirtualMachineSnapshotTree& snapshot_tree);

    virtual VOID get_virtual_network_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& network, std::wstring& name);
    virtual VOID get_cluster_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& name);
    virtual VOID get_host_name_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& name);
    virtual VOID get_virtual_machine_uuid_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, std::wstring& uuid);
    virtual VOID get_vm_mor_from_key_name_map(const std::wstring& machine_key, key_name::map& more_key_name_map, Vim25Api1__ManagedObjectReference& mor);
    bool        login(LPVOID service_content, std::wstring username, std::wstring password);
    bool        logout(LPVOID service_content);

    virtual VOID vmware_portal::get_snapshot_mor_from_snapshot_list(vmware_virtual_machine_snapshots::vtr& snapshot_list, const int& snapshot_id, Vim25Api1__ManagedObjectReference &snapshot_mor);
    virtual int create_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, vmware_vm_create_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int remove_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_vm_remove_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual int revert_virtual_machine_snapshot_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_vm_revert_to_snapshot_parm& snapshot_request_parm, operation_progress::slot_type slot);
    virtual bool changeblocktracking_setting_internal(Vim25Api1__ServiceContent& service_content, const std::wstring& vm_mor_item, const bool& value);
    virtual license_map get_supported_features(Vim25Api1__ServiceContent& service_content);

    virtual vmware_virtual_machine::ptr create_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr modify_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_virtual_machine_config_spec &config_spec);
    virtual vmware_virtual_machine::ptr clone_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const vmware_clone_virtual_machine_config_spec &clone_spec, operation_progress::slot_type slot);
    virtual bool delete_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_on_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual bool power_off_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key = std::wstring());

    virtual bool unregister_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key, const std::wstring &host_key = std::wstring());
    virtual vmware_virtual_machine::ptr add_existing_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, const vmware_add_existing_virtual_machine_spec &spec);

    Vim25Api1__VirtualDeviceConfigSpec* create_serial_port_spec(Vim25Api1__ServiceContent& service_content);
    Vim25Api1__VirtualDeviceConfigSpec* create_floppy_spec(Vim25Api1__ServiceContent& service_content);
    Vim25Api1__VirtualDeviceConfigSpec* create_cd_rom_spec(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* host_mor);
    Vim25Api1__VirtualDeviceConfigSpec* create_nic_spec(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* host_mor, DEV_CONFIGSPEC_OP op, vmware_virtual_network_adapter::ptr nic);
    Vim25Api1__VirtualDeviceConfigSpec* create_scsi_ctrl_spec(Vim25Api1__ServiceContent& service_content, const vmware_virtual_scsi_controller& ctrl, DEV_CONFIGSPEC_OP op = DEV_CONFIGSPEC_OP::OP_ADD);
    Vim25Api1__VirtualDeviceConfigSpec* create_virtual_disk_spec(Vim25Api1__ServiceContent& service_content, vmware_disk_info::ptr disk, DEV_CONFIGSPEC_OP op, DEV_CONFIGSPEC_FILEOP file_op);
    std::vector<Vim25Api1__VirtualDeviceConfigSpec*> create_vmdks_spec(Vim25Api1__ServiceContent& service_content, vmware_virtual_machine::ptr vm, const vmware_disk_info::vtr disks);
    vmware_virtual_machine::ptr add_virtual_disks_vmdk(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor, vmware_disk_info::vtr disks);
    bool                        reconfig_virtual_machine(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor, Vim25Api1__VirtualMachineConfigSpec& spec);
    bool                        delete_virtual_machine_internal(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor);
    bool                        mount_vm_tools_internal(Vim25Api1__ServiceContent& service_content, const std::wstring &machine_key);
    vmware_virtual_machine::ptr get_virtual_machine_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& vm_mor);
    vmware_virtual_machine::ptr fetch_virtual_machine_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ObjectContent& obj, mor_name_map &mapHostMorName, mor_name_map &mapNetworkMorName, key_name::map &mapVmMorKeyName, mor_name_map &mapHostMorClusterName, hv_connection_type &type, const std::wstring &host_key = std::wstring(), const std::wstring &cluster_key = std::wstring());
    
    //MISC
    Vim25Api1__ServiceContent*         get_service_content(std::wstring uri);
    Vim25Api1__ManagedObjectReference* get_ancestor_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* mor, const std::wstring& ancestor_type, mor_map& ancestor_mor_cache = mor_map());
    Vim25Api1__ManagedObjectReference* get_ancestor_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference* mor, const macho::string_array& types);
    LPVOID      get_property_from_mor(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& mor, const std::wstring& property_name);
    void        get_native_error(int &error, std::wstring &message);

    TASK_STATE  wait_task_completion_callback(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info, std::wstring task_display_name, operation_progress::slot_type slot, const DWORD wait_time = INFINITE);
    TASK_STATE  wait_task_completion(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info, std::wstring task_display_name, const DWORD wait_time = INFINITE);
#if 0
    VOID        fetch_vm_vmdks_size(_In_ Vim25Api1__ServiceContent& service_content, vmware_virtual_machine::vtr vms);
#endif
    VOID                            fetch_task_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& task_mor, vmware_vm_task_info& task_info);
    vmware_snapshot_disk_info::vtr  _get_snapshot_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ManagedObjectReference& snapshot_mor);

    VOID        _fetch_task_info(Vim25Api1__ServiceContent& service_content, Vim25Api1__ObjectContent* objectContent, vmware_vm_task_info& task_info);
    VimBindingProxy                     _vim_binding;
    std::string                         _endpoint;
    service_connect::ptr                _connect;
    std::wstring                        _uri;
    bool                                _is_interrupted;
    bool                                _is_cancel;

#ifdef _DEBUG
    std::wstring                        _username;
    std::wstring                        _password;
#else
    macho::windows::protected_data      _username;
    macho::windows::protected_data      _password;
#endif
    int                                 _last_error_code;
    std::wstring                        _last_error_message;
};

}}}

#endif
