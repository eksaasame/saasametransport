// hv_cli.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vmware.h"
#include "vmware_ex.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\date_time\local_time_adjustor.hpp"
#include "boost\date_time\c_local_time_adjustor.hpp"
#include "boost\thread.hpp"
#include "boost\date_time.hpp"
#include "macho.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"

using namespace macho;
using namespace macho::windows;
using namespace mwdc::ironman::hypervisor;
using namespace mwdc::ironman::hypervisor_ex;
namespace po = boost::program_options;
#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_MINIPORT_PROCESS_SERVICE_IRP CTL_CODE(IOCTL_SCSI_BASE,  0x040e, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;

void print_cmd_examples()
{
    std::wcout << std::endl << L"Examples:" << std::endl << std::endl;
    std::wcout << L"Browsing ESX server -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password>" << std::endl << std::endl;
    std::wcout << L"Browsing the registered VM on ESX server -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid>" << std::endl << std::endl;
    std::wcout << L"Browsing the registered VM with the specific changeid information on ESX server if CBT enabled -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> --sm <snapshot mor ref string>" << std::endl << std::endl;
#ifdef VIXMNTAPI  
    std::wcout << L"Browsing the remote vmdk file of the POWER OFF registered VM -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -f <[datastore] vm_name/vmdk file name>" << std::endl << std::endl;
    std::wcout << L"Browsing local vmdk file -" << std::endl;
    std::wcout << L"  -f <vmdk file name>" << std::endl << std::endl;
#endif    
    std::wcout << L"Create registerd VM snapshot -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -e <snapshot name> -d <snapshot description>" << std::endl << std::endl;
    std::wcout << L"Remove registered VM snapshot -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -r <snapshot id>" << std::endl << std::endl;
#ifdef VIXMNTAPI 
    std::wcout << L"Mount local vmdk file read only -" << std::endl;
    std::wcout << L"  -f <vmdk file name> -m ro" << std::endl << std::endl;
    std::wcout << L"Mount local vmdk file writable -" << std::endl;
    std::wcout << L"  -f <vmdk file name> -m rw" << std::endl << std::endl;
    std::wcout << L"Clone all vmdk files of the registered VM to local -" << std::endl;

    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -b <save folder>" << std::endl << std::endl;
    std::wcout << L"Clone the vmdk file of the registered VM to local -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -f <[datastore] vm_name/vmdk file name>> -b <cloned vmdk file name>" << std::endl << std::endl;
#endif   
    std::wcout << L"Enable/Disable CBT of the registered VM on ESX server -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -x <true|false>" << std::endl << std::endl;
    std::wcout << L"List all vmdks' data blocks address of the registered VM on ESX server -" << std::endl;
    std::wcout << L"  -t <ESX server IP> -u <login user name> -p <login password> -v <virtual machine uuid> -k <*|\"52 f5 dd 00 35 a9 d9 9b-cf 97 28 ad 69 8c 20 c8/80\">" << std::endl << std::endl;
}

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    po::options_description target("Target");
    target.add_options()
        ("host,t", po::wvalue<std::wstring>(), "VMware ESX server host IP/Name")
        ("user,u", po::wvalue<std::wstring>(), "User name")
        ("password,p", po::wvalue<std::wstring>(), "Password")
        ("task", "List Tasks")
        ;
    po::options_description command("Commands");
    command.add_options()
        ("all,a", "List all virtual Machines")
        ("vm,v", po::wvalue<std::wstring>(), "Get virtual machine detail")
#ifdef VIXMNTAPI
        ("vmdk,f", po::wvalue<std::wstring>(), "Get VMDK file detail")
#endif
        ("cluster,c", po::wvalue<std::wstring>(), "Get cluster detail")
#if _DEBUG
        ("new", po::wvalue<std::wstring>(), "Create new VM")
        ("modify", po::wvalue<std::wstring>(), "Modify VM")
        ("mount", po::wvalue<std::wstring>(), "Mount VM disks")
        ("clone", po::wvalue<std::wstring>(), "Clone VM")
        ("copy", po::wvalue<std::wstring>(), "Copy VM")
        ("revert", po::value<int>(), "Revert snapshot by snapshot id")
        ("datacenter", po::wvalue<std::wstring>(), "Print DataCenter VM Folders")
        ("snapshot", po::wvalue<std::wstring>(), "Clone VM snapshot")
        ("delete", "Delete VM")
        ("poweron", "Power On VM")
        ("poweroff", "Power Off VM")
        ("reregister", "Re-register VM")
#endif
        ("thumbprint,i", po::value<int>(), "The SSL port to the thumbprint data of VMware ESX server")
        ;

    po::options_description create_options("Create Snapshot Options");
    create_options.add_options()
        ("create,e", po::wvalue<std::wstring>(), "Create Snapshot")
        ("description,d", po::wvalue<std::wstring>(), "Snapshot description")
        ("memory,o", po::value<bool>()->default_value(false, "false"), "Include memory")
        ("quiescent,q", po::value<bool>()->default_value(true, "true"), "Quiescent")    
        ("count", po::value<int>()->default_value(0, "0"), "Max snapshot number (0 is unlimited)")
        ;

    po::options_description remove_options("Remove Snapshot Options");
    remove_options.add_options()
        ("remove,r", po::value<int>(), "Remove snapshot by snapshot id")
        ("children,n", po::value<bool>()->default_value(true, "true"), "Remove children")
        ("consolidate,s", po::value<bool>()->default_value(true, "true"), "Consolidate")
        ;
#ifdef VIXMNTAPI
    po::options_description vmdk_options("VMDK Options");
    vmdk_options.add_options()
        ("backup,b", po::wvalue<std::wstring>(), "Specify the path if without -f option or file name to store VMDK backup.")
        ("image_type,g", po::value<int>()->default_value(0, "0"), "0: to vmdk, 1: to vhdx")
        ("mount,m", po::wvalue<std::wstring>(), "Mount VMDK file w/ ro or rw mode.")
        //("unmount,n", po::wvalue<std::wstring>(), "Unmount local VMDK file.")
        ;
#endif
    po::options_description cbt_options("Change Block Tracking Options");
    cbt_options.add_options()
        ("enable_cbt,x", po::value<bool>(), "Enable/Disable change block tracking")
        ("list_changed_areas,k", po::wvalue<std::wstring>(), "List vmdk changed areas via previous changed id, \"*\" for all data blocks.")
        ("sm", po::wvalue<std::wstring>(), "Specify snapshot mor string to query changed blocks List.")
        ;

    po::options_description general("General");
    general.add_options()
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 10 )")
        ("help,h", "produce help message (option)");
    ;
    po::options_description all("Allowed options");
    all.add(general).add(target).add(command).add(create_options).add(remove_options)
#ifdef VIXMNTAPI       
        .add(vmdk_options)
#endif       
        ;

    all.add(cbt_options);

    try{

#if _UNICODE
        po::store(po::wcommand_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#else
        po::store(po::command_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#endif
        po::notify(vm);
        if (vm.count("help") || (vm["level"].as<int>() > 10) || (vm["level"].as<int>() < 0)){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("host") && vm.count("user") && vm.count("password")){

                //if (vm.count("vmdk") && vm.count("vm") && !vm.count("backup"))
                if (vm.count("vmdk") && vm.count("vm") && vm.count("mount"))
                    std::cout << title << all << std::endl;
                else if (vm.count("vmdk") && vm.count("cluster"))
                    std::cout << title << all << std::endl;
                else if (vm.count("vmdk") && vm.count("all"))
                    std::cout << title << all << std::endl;
                else if (vm.count("vm") && vm.count("cluster"))
                    std::cout << title << all << std::endl;
                else if (vm.count("vm") && vm.count("all"))
                    std::cout << title << all << std::endl;
                else if (vm.count("cluster") && vm.count("all"))
                    std::cout << title << all << std::endl;
                else if (vm.count("thumbprint") && vm.count("vm"))
                    std::cout << title << all << std::endl;
                else if (vm.count("thumbprint") && vm.count("vmdk"))
                    std::cout << title << all << std::endl;
                else if (vm.count("thumbprint") && vm.count("cluster"))
                    std::cout << title << all << std::endl;
                else if (vm.count("thumbprint") && vm.count("all"))
                    std::cout << title << all << std::endl;
                else if (vm.count("thumbprint"))
                    std::cout << title << all << std::endl;
                else
                    result = true;
            }
            else if (vm.count("host") && vm.count("thumbprint"))
            {
                if (vm.count("vm") || vm.count("cluster") || vm.count("all") || vm.count("vmdk") || vm.count("user") || vm.count("password"))
                    std::cout << title << all << std::endl;
                else
                    result = true;
            }
            else if (vm.count("vmdk") && vm.count("mount")){
                result = true;
            }
            else if (vm.count("vmdk")){
                result = true;
            }
            else
                std::cout << title << all << std::endl; 
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << all << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << all << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

void traverse_snapshot_in_tree(std::vector<vmware_virtual_machine_snapshots::ptr> child, std::wstring parent_id = L"")
{
    for (vmware_virtual_machine_snapshots::vtr::iterator s = child.begin(); s != child.end(); s++)
    {
        vmware_virtual_machine_snapshots::ptr obj = (vmware_virtual_machine_snapshots::ptr)(*s);
        std::wcout << L"         " << obj->name << std::endl;
        std::wcout << L"             Created Time      : " << ctime(&obj->create_time);
        if (obj->description.length() > 0)
            std::wcout << L"             Description       : " << obj->description << std::endl;
        std::wcout << L"             Moref             : " << obj->snapshot_mor_item << std::endl;
        std::wcout << L"             Quiescent         : " << (obj->quiesced ? L"Yes" : L"No") << std::endl;
        std::wcout << L"             ID                : " << std::to_wstring(obj->id) << std::endl;
        if (!parent_id.empty())
            std::wcout << L"             Parent ID         : " << parent_id << std::endl;

        if (obj->child_snapshot_list.size() > 0)
        {
            traverse_snapshot_in_tree(obj->child_snapshot_list, std::to_wstring(obj->id));
        }
    }
}

std::vector<vmware_virtual_machine_snapshots::ptr> get_total_snapshots_in_tree(std::vector<vmware_virtual_machine_snapshots::ptr> child, std::wstring parent_id = L"")
{
    std::vector<vmware_virtual_machine_snapshots::ptr> total = child;
    
    for (vmware_virtual_machine_snapshots::vtr::iterator s = child.begin(); s != child.end(); s++)
    {
        vmware_virtual_machine_snapshots::ptr obj = (vmware_virtual_machine_snapshots::ptr)(*s);
        std::wcout << L"         " << obj->name << std::endl;
        std::wcout << L"             Created Time      : " << ctime(&obj->create_time);
        if (obj->description.length() > 0)
            std::wcout << L"             Description       : " << obj->description << std::endl;
        std::wcout << L"             Moref             : " << obj->snapshot_mor_item << std::endl;
        std::wcout << L"             Quiescent         : " << (obj->quiesced ? L"Yes" : L"No") << std::endl;
        std::wcout << L"             ID                : " << std::to_wstring(obj->id) << std::endl;
        if (!parent_id.empty())
            std::wcout << L"             Parent ID         : " << parent_id << std::endl;

        if (obj->child_snapshot_list.size() > 0)
        {
            std::vector<vmware_virtual_machine_snapshots::ptr> sub  = get_total_snapshots_in_tree(obj->child_snapshot_list, std::to_wstring(obj->id));
            total.insert(total.end(), sub.begin(), sub.end());
        }
    }
    return std::move(total);
}

void print_vm(vmware_virtual_machine::ptr& v){

    std::wcout << L"Virtual Machine UUID : " << v->uuid << std::endl;
    std::wcout << L"     --Basic--" << std::endl;
    std::wcout << L"         Name                  : " << v->name << std::endl;
    if (v->annotation.length() > 0)
        std::cout << "         Description           : " << macho::stringutils::convert_unicode_to_utf8(v->annotation) << std::endl;
    std::wcout << L"         DataCenter            : " << v->datacenter_name << std::endl;
    std::wcout << L"         VM Config Path        : " << v->config_path_file << std::endl;
    std::wcout << L"         VM moref              : " << v->vm_mor_item << std::endl;
    switch (v->connection_type)
    {
        case HV_CONNECTION_TYPE_VCENTER:
            std::wcout << L"         Connection Type       : " << L"via vCenter server" << std::endl;
            break;
        case HV_CONNECTION_TYPE_HOST:
            std::wcout << L"         Connection Type       : " << L"ESXi direct" << std::endl;
            break;
        default:
            std::wcout << L"         Connection Type       : " << L"Unknown" << std::endl;
            break;
    }
    std::wcout << L"     --Host Agent--" << std::endl;
    std::wcout << L"         Name                  : " << v->host_name << std::endl;
    std::wcout << L"     --Hardware--" << std::endl;
    std::wcout << L"         Families              : " << boost::wformat(L"vmx-%02d") % v->version << std::endl;
    std::wcout << L"         Memory(MB)            : " << v->memory_mb << std::endl;
    std::wcout << L"         CPUs                  : " << v->number_of_cpu << std::endl;
    if (v->cluster_name.length() > 0)
    {
        std::wcout << L"     --Cluster--" << std::endl;
        std::wcout << L"         Name                  : " << v->cluster_name << std::endl;
    }
    std::wcout << L"     --Network Adapters--" << std::endl;
    for (vmware_virtual_network_adapter::vtr::iterator p = v->network_adapters.begin(); p != v->network_adapters.end(); p++)
    {
        vmware_virtual_network_adapter::ptr obj = (vmware_virtual_network_adapter::ptr)(*p);
        std::wcout << L"         " << obj->name << std::endl;
        std::wcout << L"             Network           : " << obj->network << std::endl;
        std::wcout << L"             Address Type      : " << obj->address_type << std::endl;
        std::wcout << L"             Adapter Type      : " << obj->type << std::endl;
        std::wcout << L"             MAC               : " << obj->mac_address << std::endl;
        std::wcout << L"             Key               : " << obj->key << std::endl;
    }
    std::wcout << L"     --Disks--" << std::endl;

    foreach(vmware_disk_info::ptr& disk, v->disks)
    {
        std::wcout << L"             UUID              : " << disk->uuid << std::endl;
        std::wcout << L"             Key               : " << disk->wsz_key() << std::endl;
        std::wcout << L"             Size              : " << std::to_wstring(disk->size) << L" Bytes" << std::endl;
        std::wcout << L"             Thin Provisioned  : " << (disk->thin_provisioned ? L"true" : L"false") << std::endl;
    }
    std::wcout << L"     --Guest--" << std::endl;
    std::wcout << L"         OS Version            : " << v->guest_full_name << std::endl;
    std::wcout << L"         OS Id                 : " << v->guest_id << std::endl;
    if (!v->guest_host_name.empty())
        std::wcout << L"         Host Name             : " << v->guest_host_name << std::endl;
    if (v->guest_net.size())
    {
        std::wcout << L"         Network -" << std::endl;
        foreach(auto nic, v->guest_net)
        {
            std::wcout << L"                 MAC           : " << nic.mac_address << std::endl;
            if (nic.ip_config.size())
            {
                std::wcout << L"                 IP Addresses  :" << std::endl;
                foreach(auto ip, nic.ip_config)
                {
                    if (ip == v->guest_primary_ip)
                        ip.append(L"(*)");
                    std::wcout << L"                                 " << ip << std::endl;
                }
                std::wcout << std::endl;
            }
        }
    }
    std::wcout << L"         Tool Status           : ";
    switch (v->tools_status)
    {
        case HV_VMTOOLS_NOTINSTALLED:
            std::wcout << L"Not install" << std::endl;
            break;

        case HV_VMTOOLS_NOTRUNNING:
            std::wcout << L"Not running" << std::endl;
            break;

        case HV_VMTOOLS_OLD:
            std::wcout << L"Too old" << std::endl;
            break;
        
        case HV_VMTOOLS_NEW:
            std::wcout << L"Too new" << std::endl;
            break;
        
        case HV_VMTOOLS_NEEDUPGRADE:
            std::wcout << L"Need upgrade" << std::endl;
            break;

        case HV_VMTOOLS_UNMANAGED:
            std::wcout << L"Unmanaged" << std::endl;
            break;

        case HV_VMTOOLS_BLACKLISTED:
            std::wcout << L"In black listed" << std::endl;
            break;

        case HV_VMTOOLS_OK:
            std::wcout << L"In supported status" << std::endl;
            break;

        case HV_VMTOOLS_UNKNOWN:
            std::wcout << L"Unknown" << std::endl;
    }

    std::wcout << L"     --State--" << std::endl;
    std::wcout << L"         Power                 : ";
    switch (v->power_state)
    {
        case HV_VMPOWER_OFF:
            std::wcout << L"Off" << std::endl;
            break;

        case HV_VMPOWER_ON:
            std::wcout << L"On" << std::endl;
            break;

        case HV_VMPOWER_SUSPENDED:
            std::wcout << L"Suspended" << std::endl;
            break;

        case HV_VMPOWER_UNKNOWN:
            std::wcout << L"Unknown" << std::endl;
    }

    std::wcout << L"         Connection            : ";
    switch (v->connection_state)
    {
        case HV_VMCONNECT_CONNECTED:
            std::wcout << L"Connected" << std::endl;
            break;

        case HV_VMCONNECT_DISCONNECTED:
            std::wcout << L"Disconnected" << std::endl;
            break;

        case HV_VMCONNECT_ORPHANED:
            std::wcout << L"Orphaned" << std::endl;
            break;

        case HV_VMCONNECT_INACCESSIBLE:
            std::wcout << L"Inaccessible" << std::endl;
            break;

        case HV_VMCONNECT_INVALID:
            std::wcout << L"Invalid" << std::endl;
            break;

        case HV_VMCONNECT_UNKNOWN:
            std::wcout << L"Unknown" << std::endl;
    }

    if (v->root_snapshot_list.size() > 0)
    {
        std::wcout << L"     --Snapshots--" << std::endl;
        traverse_snapshot_in_tree(v->root_snapshot_list);
    }
    std::wcout << L"     --Others--" << std::endl;
    std::wcout << L"         Disk UUID Enabled     : " << (v->is_disk_uuid_enabled ? L"Yes" : L"No") << std::endl;
    std::wcout << L"         CBT Enabled           : " << (v->is_cbt_enabled ? L"Yes" : L"No") << std::endl;

}

void print_vm_snapshot(vmware_snapshot_disk_info::map& snapshot){

    foreach(vmware_snapshot_disk_info::map::value_type& disk, snapshot)
    {
        std::wcout << L"             UUID              : " << disk.second->uuid << std::endl;
        std::wcout << L"             Key               : " << disk.second->wsz_key() << std::endl;
        std::wcout << L"             FileName              : " << disk.second->name << std::endl;
        std::wcout << L"             Size              : " << std::to_wstring(disk.second->size) << L" Bytes" << std::endl;
        std::wcout << L"             ChangeId              : " << disk.second->change_id << std::endl;
    }
}

void snapshot_progress_callback(const std::wstring& snapshot_name, const int& progess)
{
    std::wcout << L"Snapshot: " << snapshot_name << L", progress: " << std::to_wstring(progess) << L"%\r";
    std::cout.flush();
}

void clone_progress_callback(const std::wstring& snapshot_name, const std::wstring& image_name, const std::wstring& output_file, const uint64& total_size_bytes, const uint64& completed_read_bytes, const uint64& completed_written_bytes)
{
    int completed_ratio = (completed_read_bytes * 100) / total_size_bytes;
    if (completed_ratio % 5 == 0)
    {
        std::wcout << L"Image: " << image_name << L", progress: " << std::to_wstring(completed_ratio) << L"%\r";
        std::cout.flush();
    }

    if (completed_ratio == 100)
        std::wcout << std::endl;
}

struct device_mount_repository{
    device_mount_repository() : terminated(false){
        quit_event = CreateEvent(
            NULL,   // lpEventAttributes
            TRUE,   // bManualReset
            FALSE,  // bInitialState
            NULL    // lpName
            );
    }
    bool                        terminated;
    std::wstring				device_path;
    HANDLE			            quit_event;
    bool						is_canceled(){ return terminated; }
};

struct device_mount_task{
    typedef boost::shared_ptr<device_mount_task> ptr;
    typedef std::vector<ptr> vtr;
    typedef boost::signals2::signal<bool(), macho::_or<bool>> task_is_canceled;

    device_mount_task(universal_disk_rw::ptr _rw, unsigned long _disk_size_in_mb, std::wstring _name, device_mount_repository& _repository) 
        : rw(_rw), name(_name), disk_size_in_mb(_disk_size_in_mb), repository(_repository){}
    device_mount_repository&                                     repository;
    universal_disk_rw::ptr                                       rw;
    std::wstring												 name;
    task_is_canceled                                             is_canceled;
    macho::windows::auto_handle									 handle;
    unsigned long										    	 disk_size_in_mb;
    inline void register_task_is_canceled_function(task_is_canceled::slot_type slot){
        is_canceled.connect(slot);
    }
    void                                                         mount(){
        handle = CreateFile(repository.device_path.c_str(),  // Name of the NT "device" to open 
            GENERIC_READ | GENERIC_WRITE,  // Access rights requested
            0,                           // Share access - NONE
            0,                           // Security attributes - not used!
            OPEN_EXISTING,               // Device must exist to open it.
            FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
            0);
        if (handle.is_valid()){
            CONNECT_IN						 connectIn;
            CONNECT_IN_RESULT                connectInRet;
            DWORD							 bytesReturned;
            BOOL							 devStatus;
            memset(&connectIn, 0, sizeof(CONNECT_IN));
            memset(&connectInRet, 0, sizeof(CONNECT_IN_RESULT));
            wmemcpy_s(connectIn.DiskId, sizeof(connectIn.DiskId), name.c_str(), name.length());
            std::wcout << L"Try to mount disk with serial number : " << connectIn.DiskId << std::endl;
            connectIn.DiskSizeMB = disk_size_in_mb;
            connectIn.MergeIOs = TRUE;
            std::wcout << L"Disk Size : " << connectIn.DiskSizeMB << L" MB" << std::endl;
            connectIn.Command.IoControlCode = IOCTL_VMPORT_CONNECT;
            connectIn.NotifyEvent = CreateEvent(
                NULL,   // lpEventAttributes
                TRUE,   // bManualReset
                FALSE,  // bInitialState
                NULL    // lpName
                );
            if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, &connectIn, sizeof(CONNECT_IN),
                &connectInRet, sizeof(CONNECT_IN_RESULT), &bytesReturned, NULL)){
                bool request_only = true;
                DWORD  dwEvent;
#define BUFF_SIZE  4 * 1024 * 1024
#define RETRY_COUNT 3
                std::wcout << L"TargetId : " << connectInRet.TargetId << std::endl;
                std::wcout << L"PathId : " << connectInRet.PathId << std::endl;
                std::wcout << L"Lun : " << connectInRet.Lun << std::endl;
                std::wcout << L"MaxTransferLength : " << connectInRet.MaxTransferLength << std::endl;
                size_t in_size = sizeof(VMPORT_CONTROL_IN) + (connectInRet.MaxTransferLength ? connectInRet.MaxTransferLength : BUFF_SIZE);
                size_t out_size = sizeof(VMPORT_CONTROL_OUT) + (connectInRet.MaxTransferLength ? connectInRet.MaxTransferLength : BUFF_SIZE);
                std::auto_ptr<VMPORT_CONTROL_IN> in((PVMPORT_CONTROL_IN)new BYTE[in_size]);
                std::auto_ptr<VMPORT_CONTROL_OUT> out((PVMPORT_CONTROL_OUT)new BYTE[out_size]);
                memset(in.get(), 0, in_size);
                memset(out.get(), 0, out_size);
                in->Device = connectInRet.Device;
                uint32_t number_of_sectors_read, number_of_sectors_written;
                HANDLE ghEvents[2];
                ghEvents[0] = connectIn.NotifyEvent;
                ghEvents[1] = repository.quit_event;
                int retry = RETRY_COUNT;
                while (true){
                    dwEvent = WaitForMultipleObjects(
                        2,           // number of objects in array
                        ghEvents,     // array of objects
                        FALSE,       // wait for any object
                        10000);       // ten-second wait
                    switch (dwEvent)
                    {
                    case WAIT_OBJECT_0 + 0:
                    //	std::wcout << L"Event triggered." << std::endl;
                        break;
                    case WAIT_OBJECT_0 + 1:
                        std::wcout << L"Quit triggered." << std::endl;
                        break;
                    case WAIT_TIMEOUT:
                        std::wcout << L"TIMEOUT triggered." << std::endl;
                        break;
                    default:
                        printf("Wait error: %d\n", GetLastError());
                    }
                    if (repository.terminated || is_canceled())
                        break;
                    std::wcout << L"Request only :" << request_only << std::endl;
                    in->Command.IoControlCode = request_only ? IOCTL_VMPORT_GET_REQUEST : IOCTL_VMPORT_SEND_AND_GET;
                    out->RequestBufferLength = (connectInRet.MaxTransferLength ? connectInRet.MaxTransferLength : BUFF_SIZE);
                    if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, in.get(), in_size,
                        out.get(), out_size, &bytesReturned, NULL)){
                        //std::wcout << L"RequestType :" << out->RequestType << std::endl;
                        //if (out->RequestType){
                        //	std::wcout << L"Request Id :" << out->RequestID << std::endl;
                        //	std::wcout << L"Start Sector :" << out->StartSector << std::endl;
                        //	std::wcout << L"Length :" << out->RequestBufferLength << std::endl;
                        //}
                        switch (out->RequestType) {
                        case VMPORT_READ_REQUEST: {
                            request_only = false;
                            in->RequestID = out->RequestID;
                            std::wcout << L"Read : Start Sector(" << out->StartSector << L"), Length : " << out->RequestBufferLength << std::endl;
                            if (rw->sector_read(out->StartSector, out->RequestBufferLength / 512, in->ResponseBuffer, number_of_sectors_read)){
                                in->ResponseBufferLength = out->RequestBufferLength;
                                in->ErrorCode = 0;
                        //		std::wcout << L"Read Succeeded." << std::endl;
                                retry = RETRY_COUNT;
                            }
                            else{
                                retry--;
                                std::wcout << L"Read Failed." << std::endl;
                                if (retry > 0){
                                    std::wcout << L"Retry : " << retry << std::endl;
                                    request_only = true;
                                }
                                else{
                                    in->ErrorCode = 0x1;
                                    repository.terminated = true;
                                }
                            }
                        }
                        break;
                        case VMPORT_WRITE_REQUEST: {
                            request_only = false;
                            in->RequestID = out->RequestID;
                            std::wcout << L"Write : Start Sector(" << out->StartSector << L"), Length : " << out->RequestBufferLength << std::endl;
                            if (rw->sector_write(out->StartSector, out->RequestBuffer, out->RequestBufferLength / 512, number_of_sectors_written)){
                                in->ResponseBufferLength = out->RequestBufferLength;
                                in->ErrorCode = 0;
                        //		std::wcout << L"Write Succeeded." << std::endl;
                                retry = RETRY_COUNT;
                            }
                            else{
                                retry--;
                                std::wcout << L"Write Failed." << std::endl;
                                if (retry > 0){
                                    std::wcout << L"Retry : " << retry << std::endl;
                                    request_only = true;
                                }
                                else{
                                    in->ErrorCode = 0x1;
                                    repository.terminated = true;
                                }
                            }
                        }
                                                   break;
                        case VMPORT_NONE_REQUEST:
                            std::wcout << L"NONE_REQUEST." << std::endl;
                        default:
                            request_only = true;
                        }
                    }
                    else{
                        printf("DeviceIoControl error: %d\n", GetLastError());
                        repository.terminated = true;
                    }
                }
                std::wcout << L"Dismount device" << std::endl;
                connectIn.Command.IoControlCode = IOCTL_VMPORT_DISCONNECT;
                devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, &connectIn, sizeof(CONNECT_IN),
                    NULL, 0, &bytesReturned, NULL);
            }
            else{
                std::wcout << L"Cannot mount device" << std::endl;
            }
        }
    }
};

void print_vmware_folders( vmware_folder::vtr& folders ){
    foreach(vmware_folder::ptr& f, folders){
        std::wcout << L" " << f->path() << std::endl;
        print_vmware_folders(f->folders);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    po::variables_map vm;
    boost::filesystem::path logfile;
    vmware_portal_::ptr portal = NULL;
    vmware_portal_ex::ptr portal_ex = NULL;
    std::wstring app = macho::windows::environment::get_execution_full_path();
    SetDllDirectory(boost::filesystem::path(app).parent_path().parent_path().wstring().c_str());

    if (macho::windows::environment::is_running_as_local_system())
        logfile = boost::filesystem::path(macho::windows::environment::get_windows_directory()) / (boost::filesystem::path(app).filename().wstring() + L".log");
    else
        logfile = macho::windows::environment::get_execution_full_path() + L".log";
    macho::set_log_file(logfile.wstring());

    if (command_line_parser(vm)){
        vmware_portal_ex::set_log(logfile.wstring(), (macho::TRACE_LOG_LEVEL)vm["level"].as<int>());
        macho::set_log_level((macho::TRACE_LOG_LEVEL)vm["level"].as<int>());
        if (vm.count("host")
            && vm.count("user")
            && vm.count("password")
#ifdef VIXMNTAPI
            && !vm.count("backup") 
            && !vm.count("mount") 
            && !vm.count("vmdk")
#endif
            )
        {
            std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % vm["host"].as<std::wstring>());
            //vmware_portal_::ptr portal = vmware_portal_::connect(L"https://10.100.100.11/sdk", L"root", L"falconstor");
            portal = vmware_portal_::connect(uri, vm["user"].as<std::wstring>(), vm["password"].as<std::wstring>());
            if (!portal)
            {
                std::wcout << L"Login failure." << std::endl;
                return 1;
            }
        }
        else if (vm.count("vmdk") || (vm.count("vmdk") && vm.count("mount")) || (vm.count("vmdk") && vm.count("backup")) || (vm.count("vm") && vm.count("backup")))
        {
            if (vmware_portal_ex::vixdisk_init())
                portal_ex = vmware_portal_ex::ptr(new vmware_portal_ex());
            if (!portal_ex)
            {
                std::wcout << L"Initial failure." << std::endl;
                return 1;
            }
            portal_ex->set_log(logfile.wstring(), (macho::TRACE_LOG_LEVEL)vm["level"].as<int>());
        }
#if _DEBUG
        if (vm.count("mount")){
            std::wstring  device_path;
            macho::windows::auto_handle handle;
            macho::windows::device_manager  dev_mgmt;
            hardware_device::vtr class_devices;
            class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
            foreach(hardware_device::ptr &d, class_devices){
                std::wcout << L"GUID               : " << d->sz_class_guid << std::endl;
                std::wcout << L"Class           : " << d->sz_class << std::endl;
                std::wcout << L"Driver           : " << d->driver << std::endl;
                std::wcout << L"Service           : " << d->service << std::endl;
                std::wcout << L"Location           : " << d->location << std::endl;
                std::wcout << L"Location Path      : " << d->location_path << std::endl;
                std::wcout << L"Device Description : " << d->device_description << std::endl;
                std::wcout << L"Device Instance Id : " << d->device_instance_id << std::endl;				
                if (d->service == L"vmp"){
                    device_path = dev_mgmt.get_device_path(d->device_instance_id, (LPGUID)&GUID_DEVINTERFACE_STORAGEPORT);
                    std::wcout << L"Device Path : " << device_path << std::endl;
                    break;
                }
                std::wcout << std::endl;
            }

            if (!device_path.empty()) {
                DWORD							 bytesReturned;
                COMMAND_IN						 command;
                BOOL							 devStatus;	
                device_mount_repository      	 repository;
                repository.device_path = device_path;
                handle = CreateFile(device_path.c_str(),  // Name of the NT "device" to open 
                    GENERIC_READ | GENERIC_WRITE,  // Access rights requested
                    0,                           // Share access - NONE
                    0,                           // Security attributes - not used!
                    OPEN_EXISTING,               // Device must exist to open it.
                    FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
                    0);                          // extended attributes - not used!
                if (handle.is_valid()){
                    command.IoControlCode = IOCTL_VMPORT_SCSIPORT;
                    if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP,
                        &command, sizeof(command), &command, sizeof(command), &bytesReturned, NULL))
                    {
                        std::wcout << L"Open device" << std::endl;
                        vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["mount"].as<std::wstring>());
                        if (v && vmware_portal_ex::vixdisk_init()){
                            std::wcout << L"Vixdisk init" << std::endl;
                            vmware_portal_ex portal_ex;
                            boost::thread_group thread_pool;
                            portal_ex.set_log(get_log_file(), get_log_level());
                            portal_ex.set_portal(portal, v->key);
                            std::wstring host = vm["host"].as<std::wstring>();
                            std::wstring user = vm["user"].as<std::wstring>();
                            std::wstring password = vm["password"].as<std::wstring>();
                            vmware_vixdisk_connection::ptr conn;
                            conn = portal_ex.get_vixdisk_connection(host, user, password, v->key, L"", false);
                            if (!conn){
                                std::wcout << L"Failed to open vixdisk connection." << std::endl;
                            }
                            else{
                                std::wcout << L"Open vixdisk connection." << std::endl;
                                GUID guid = macho::guid_(v->uuid);
                                device_mount_task::vtr tasks;
                                foreach(vmware_disk_info::map::value_type& _d, v->disks_map){
                                    WCHAR disk_id[40];
                                    std::wcout << L"Disk key :" << _d.second->key << std::endl;
                                    memset(disk_id, 0, sizeof(disk_id));
                                    _snwprintf_s(disk_id, sizeof(disk_id),
                                        L"%04x%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", _d.second->key,
                                        guid.Data1, guid.Data2, guid.Data3,
                                        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                                        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
                                    std::wcout << L"Disk Id :" << disk_id << std::endl;
                                    universal_disk_rw::ptr out = portal_ex.open_vmdk_for_rw(conn, _d.second->name, false);
                                    if (out){
                                        device_mount_task::ptr task(new device_mount_task(out, _d.second->size / (1024 * 1024), disk_id, repository));
                                        task->register_task_is_canceled_function(boost::bind(&device_mount_repository::is_canceled, &repository));
                                        tasks.push_back(task);
                                    }
                                    else{
                                        std::wcout << L"Failed to open for rw." << std::endl;
                                        break;
                                    }
                                }
                                if (tasks.size() == v->disks.size()){
                                    for (int i = 0; i < tasks.size(); i++)
                                        thread_pool.create_thread(boost::bind(&device_mount_task::mount, &(*tasks[i])));
                                    std::wcout << L"Press Enter to Continue" << std::endl;
                                    std::cin.ignore();
                                    repository.terminated = true;
                                    SetEvent(repository.quit_event);
                                    thread_pool.join_all();
                                }
                            }					
                        }
                    }
                }
                else{
                    std::wcout << L"Cannot open device" << std::endl;
                }
            }
        }
        else if (vm.count("new")){
            vmware_virtual_machine_config_spec config_spec;
            config_spec.name = vm["new"].as<std::wstring>();
            vmware_host::vtr hosts = portal->get_hosts();
            foreach(key_map::value_type v, hosts[0]->datastores){
                config_spec.config_path = v.second;
            }
            config_spec.host_key = hosts[0]->key[0];
            config_spec.guest_id = vmware_virtual_machine_config_spec::guest_os_id(mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_7);
            config_spec.memory_mb = 1024;
            config_spec.number_of_cpu = 1;
            config_spec.folder_path = L"/vm";
            config_spec.has_cdrom = true;
            config_spec.has_serial_port = true;
            config_spec.uuid = macho::guid_(L"564def83-27ff-a442-f3d7-574c60b36f55");
            config_spec.nics.push_back(vmware_virtual_network_adapter::ptr(new vmware_virtual_network_adapter()));
            config_spec.nics[0]->network = hosts[0]->networks.begin()->second;
            config_spec.nics[0]->type = L"E1000";
            config_spec.disks.push_back(vmware_disk_info::ptr(new vmware_disk_info()));
            config_spec.disks[0]->controller = vmware_virtual_scsi_controller::ptr(new vmware_virtual_scsi_controller());
            config_spec.disks[0]->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC_SAS;
            config_spec.disks[0]->name = L"disk01.vmdk";
            config_spec.disks[0]->uuid = L"3AE876F6-3A86-4844-A1B0-6F9D1377DDFA";
            config_spec.disks[0]->size = (uint64_t)10 * 1024 * 1024 * 1024;
            config_spec.disks[0]->thin_provisioned = true;
            vmware_virtual_machine::ptr v = portal->create_virtual_machine(config_spec);
            if (v){
                bool result = portal->mount_vm_tools(v->uuid);
                v->disks.size();
            }
        }
        else if (vm.count("modify")){
            vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["modify"].as<std::wstring>());
            if (v){
                vmware_virtual_machine_config_spec config_spec(*v);
                config_spec.uuid = vm["modify"].as<std::wstring>();
                vmware_host::vtr hosts = portal->get_hosts();
                config_spec.guest_id = vmware_virtual_machine_config_spec::guest_os_id(mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_8);
                config_spec.memory_mb = 2048;
                config_spec.number_of_cpu = 2;
                /*
                config_spec.nics[0]->network = hosts[0]->networks.rbegin()->second;
                config_spec.nics[0]->is_allow_guest_control = true;
                config_spec.nics[0]->is_connected = true;
                config_spec.nics[0]->is_start_connected = true;
                config_spec.nics.push_back(vmware_virtual_network_adapter::ptr(new vmware_virtual_network_adapter()));
                config_spec.nics[1]->network = hosts[0]->networks.begin()->second;
                config_spec.nics[1]->type = L"E1000";
                config_spec.disks[0]->size = (uint64_t)20 * 1024 * 1024 * 1024;
                config_spec.disks.push_back(vmware_disk_info::ptr(new vmware_disk_info()));
                config_spec.disks[config_spec.disks.size() - 1]->controller = vmware_virtual_scsi_controller::ptr(new vmware_virtual_scsi_controller());
                config_spec.disks[config_spec.disks.size() - 1]->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC_SAS;
                config_spec.disks[config_spec.disks.size() - 1]->name = L"new_disk.vmdk";
                config_spec.disks[config_spec.disks.size() - 1]->size = (uint64_t)10 * 1024 * 1024 * 1024;
                */
                //config_spec.nics.erase(config_spec.nics.begin());
                //config_spec.disks.erase(config_spec.disks.begin());
                foreach(vmware_disk_info::ptr d, config_spec.disks){
                    d->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                }
                vmware_virtual_machine::ptr v = portal->modify_virtual_machine(vm["modify"].as<std::wstring>(), config_spec);
                if (v){
                    v->disks.size();
                }
            }
        }
        else if (vm.count("datacenter")){
            vmware_datacenter::ptr datacenter = portal->get_datacenter(vm["datacenter"].as<std::wstring>());
            if (datacenter){
                print_vmware_folders(datacenter->folders);
            }
        }
        else if (vm.count("vm") && vm.count("delete")){
            portal->delete_virtual_machine(vm["vm"].as<std::wstring>());
        }
        else if (vm.count("vm") && vm.count("poweron")){
            portal->power_on_virtual_machine(vm["vm"].as<std::wstring>());
        }
        else if (vm.count("vm") && vm.count("poweroff")){
            portal->power_off_virtual_machine(vm["vm"].as<std::wstring>());
        }
        else if (vm.count("vm") && vm.count("reregister")){
            vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["vm"].as<std::wstring>());
            if (v){
                portal->unregister_virtual_machine(vm["vm"].as<std::wstring>());
                vmware_add_existing_virtual_machine_spec spec;
                spec.host_key = v->host_key;
                spec.config_file_path = v->config_path_file;
                spec.resource_pool_path = v->resource_pool_path;
                spec.folder_path = v->folder_path;
                spec.uuid;
                vmware_virtual_machine::ptr new_vm =  portal->add_existing_virtual_machine(spec);
            }	
        }
        else if (vm.count("vm") && vm.count("clone") && vm.count("snapshot")){
            vmware_clone_virtual_machine_config_spec clone_spec;
            clone_spec.snapshot_mor_item = vm["snapshot"].as<std::wstring>();
            clone_spec.vm_name = vm["clone"].as<std::wstring>();
            clone_spec.folder_path = L"/vm/SaaSaMeVM/New";
            clone_spec.uuid = macho::guid_(L"564def83-27ff-a442-f3d7-574c60b36f52");
            portal->clone_virtual_machine(vm["vm"].as<std::wstring>(), clone_spec);
        }
        else if (vm.count("vm") && vm.count("revert")){
            vmware_vm_revert_to_snapshot_parm revert_parm;
            revert_parm.id = vm["revert"].as<int>();
            portal->revert_virtual_machine_snapshot(vm["vm"].as<std::wstring>(), revert_parm, boost::bind(&snapshot_progress_callback, _1, _2));
        }
        else if (vm.count("vm") && vm.count("copy") && vm.count("snapshot")){
            vmware_host::vtr hosts = portal->get_hosts();
            foreach(vmware_host::ptr h, hosts){
                for (key_map::iterator p = h->vms.begin(); p != h->vms.end(); p++){
                    if (p->second == vm["copy"].as<std::wstring>()){
                        portal->delete_virtual_machine(p->first);
                        break;
                    }
                }
            }
        
            vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["vm"].as<std::wstring>());
            if (v && vmware_portal_ex::vixdisk_init()){
                vmware_virtual_machine_config_spec config_spec(*v);
                config_spec.name = vm["copy"].as<std::wstring>();
                config_spec.uuid = macho::guid_::create();
                foreach(vmware_disk_info::ptr d, config_spec.disks){
                    d->name = boost::filesystem::path(d->name).filename().wstring();
                }
                foreach(vmware_virtual_network_adapter::ptr& n, config_spec.nics){
                    n->mac_address.clear();
                }
                vmware_virtual_machine::ptr _v = portal->create_virtual_machine(config_spec);
                if (_v){
                    bool result = false;
                    vmware_portal_ex portal_ex;
                    portal_ex.set_log(get_log_file(), get_log_level());
                    portal_ex.set_portal(portal, v->key);
                    portal_ex.set_portal(portal, _v->key);
                    std::wstring snapshot = vm["snapshot"].as<std::wstring>();
                    std::wstring snapshot_ref;
                    std::wstring host = vm["host"].as<std::wstring>();
                    std::wstring user = vm["user"].as<std::wstring>();
                    std::wstring password = vm["password"].as<std::wstring>();
                    {
                        vmware_vixdisk_connection::ptr source_conn, target_conn;
                        portal_ex.get_snapshot_ref_item(v->key, snapshot, snapshot_ref);
                        vmware_snapshot_disk_info::map snapshot_disks = portal->get_snapshot_info(snapshot_ref);
                        source_conn = portal_ex.get_vixdisk_connection(host, user, password, v->key, snapshot);
                        target_conn = portal_ex.get_vixdisk_connection(host, user, password, _v->key, L"", false);
                        foreach(vmware_snapshot_disk_info::map::value_type &snap, snapshot_disks){
                            std::wstring name;
                            foreach(vmware_disk_info::map::value_type& _d, _v->disks_map){
                                if (_d.second->uuid == snap.second->uuid){
                                    name = _d.second->name;
                                    break;
                                }
                            }
                            universal_disk_rw::ptr out = portal_ex.open_vmdk_for_rw(target_conn, name, false);
                            universal_disk_rw::ptr in = portal_ex.open_vmdk_for_rw(source_conn, snap.second->name);
                            if (in && out){
                                vmdk_changed_areas changed = portal->get_vmdk_changed_areas(v->vm_mor_item, snapshot_ref, snap.second->wsz_key(), L"*", 0LL, snap.second->size);
                                changed_disk_extent::vtr _chunks;
#define MAX_BLOCK_SIZE          8388608UL
                                LONG64 _block_size = MAX_BLOCK_SIZE;
                                foreach(auto s, changed.changed_list){
                                    if (s.length <= _block_size)
                                        _chunks.push_back(s);
                                    else{
                                        UINT64 length = s.length;
                                        UINT64 next_start = s.start;
                                        while (length > _block_size){
                                            _chunks.push_back(changed_disk_extent(next_start, _block_size));
                                            length -= _block_size;
                                            next_start += _block_size;
                                        }
                                        _chunks.push_back(changed_disk_extent(next_start, length));
                                    }
                                }
                                std::string buf;
                                foreach(changed_disk_extent &ext, _chunks){
                                    if (result = in->read(ext.start, ext.length, buf)){
                                        uint32_t number_of_bytes_written = 0;
                                        if (!(result = out->write(ext.start, buf, number_of_bytes_written))){
                                            break;
                                        }
                                    }
                                    else{
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (!result){
                        portal->delete_virtual_machine(_v->key);
                    }
                }
            }
        }
        else if (vm.count("task") && !vm.count("cluster") && !vm.count("all") && !vm.count("vmdk") && !vm.count("mount") && !vm.count("thumbprint")){
#else
        if (vm.count("task") && !vm.count("cluster") && !vm.count("all") && !vm.count("vmdk") && !vm.count("mount") && !vm.count("thumbprint")){
#endif
            vmware_vm_task_info::vtr tasks = portal->get_virtual_machine_tasks(vm.count("vm") ? vm["vm"].as<std::wstring>() : L"");
            std::wcout << L"[Current Tasks] " << std::endl;
            foreach(vmware_vm_task_info::ptr t, tasks){
                std::wcout << L"    Task Name     : " << t->name << std::endl;
                std::wcout << L"    Entity Name   : " << t->entity_name << std::endl;
                std::wcout << L"    VM UUID       : " << t->vm_uuid << std::endl;
                std::wcout << L"    Progress      : " << t->progress << std::endl;
                std::wcout << L"    Status        : " << t->state_name << std::endl;
                std::wcout << L"    Start Time    : " << ((t->start != boost::posix_time::not_a_date_time) ? macho::stringutils::convert_ansi_to_unicode(boost::posix_time::to_simple_string(local_adj::utc_to_local(t->start))) : L"" ) << std::endl;
                std::wcout << L"    Complete Time : " << ((t->complete != boost::posix_time::not_a_date_time) ? macho::stringutils::convert_ansi_to_unicode(boost::posix_time::to_simple_string(local_adj::utc_to_local(t->complete))) : L"") << std::endl;
                std::wcout << L"    Cancelable    : " << t->cancelable << std::endl;
                std::wcout << L"    Cancelled     : " << t->cancelled << std::endl << std::endl;
            }
        }
        else if(!vm.count("vm") && !vm.count("cluster") && !vm.count("all") && !vm.count("vmdk") && !vm.count("mount") && !vm.count("thumbprint")){
            vmware_host::vtr hosts = portal->get_hosts();
            std::wcout << L"Number of Hosts : " << hosts.size() << std::endl;
            foreach(vmware_host::ptr h, hosts){
                if (h){
                    try{
                        std::wcout << L"Host key             : " << h->key.begin()->c_str() << std::endl;

                        if (h->key.size() > 1)
                        {
                            for (macho::string_array::iterator i = ++h->key.begin(); i != h->key.end(); i++)
                                std::wcout << L"                     : " << (*i) << std::endl;
                        }
                        std::wcout << L"     Name            : " << h->name.begin()->c_str() << std::endl;
                        if (h->name.size() > 1)
                        {
                            for (macho::string_array::iterator i = ++h->name.begin(); i != h->name.end(); i++)
                                std::wcout << L"                     : " << (*i) << std::endl;
                        }

                        std::wcout << L"     Name Ref        : " << h->name_ref << std::endl;
                        std::wcout << L"     Domain Name     : " << h->domain_name.begin()->c_str() << std::endl;
                        if (h->domain_name.size() > 1)
                        {
                            for (macho::string_array::iterator i = ++h->domain_name.begin(); i != h->domain_name.end(); i++)
                                std::wcout << L"                     : " << (*i) << std::endl;
                        }
                        std::wcout << L"     UUID            : " << h->uuid << std::endl;
                        std::wcout << L"     Full Name       : " << h->full_name() << std::endl;
                        std::wcout << L"     Cluster Key     : " << h->cluster_key << std::endl;
                        std::wcout << L"     Datacenter Name : " << h->datacenter_name << std::endl;
                        std::wcout << L"     Product Name    : " << h->product_name << std::endl;
                        std::wcout << L"     Version         : " << h->version << std::endl;
                        std::wcout << L"     CPU Cores       : " << h->number_of_cpu_cores << std::endl;
                        std::wcout << L"     CPU Packages    : " << h->number_of_cpu_packages << std::endl;
                        std::wcout << L"     CPU Threads     : " << h->number_of_cpu_threads << std::endl;
                        std::wcout << L"     Memory(MB)      : " << (h->size_of_memory >> 20) << std::endl;
                        std::wcout << L"     Maintenance Mode: " << (h->in_maintenance_mode ? L"true" : L"false") << std::endl;
                        std::wcout << L"     Power State     : " << h->state << std::endl;
                        std::wcout << L"     IP Addresses    : " << h->ip_address << std::endl;
                        foreach(std::wstring ip, h->ip_addresses){
                            std::wcout << L"        " << ip << std::endl;
                        }
                        std::wcout << L"     DataStores      : " << std::endl;
                        for (key_map::iterator p = h->datastores.begin(); p != h->datastores.end(); p++){
                            std::wcout << L"        " << p->first << L" (" << p->second << L")" << std::endl;
                        }
                        std::wcout << L"     Networks        : " << std::endl;
                        for (key_map::iterator p = h->networks.begin(); p != h->networks.end(); p++){
                            std::wcout << L"        " << p->first << L" (" << p->second << L")" << std::endl;
                        }
                        std::wcout << L"     Virtual Machines: " << std::endl;
                        for (key_map::iterator p = h->vms.begin(); p != h->vms.end(); p++){
                            std::wcout << L"        " << p->second << L" (" << p->first << L")" << std::endl;
                        }
                        if (!h->datacenter_name.empty()){
                            std::wcout << L"     Folders     : " << std::endl;
                            vmware_datacenter::ptr datacenter = portal->get_datacenter(h->datacenter_name);
                            if (datacenter){
                                print_vmware_folders(datacenter->folders);
                            }
                        }

                        std::wcout << L"     Licensed Features: " << std::endl;
                        for (license_map::iterator p = h->features.begin(); p != h->features.end(); p++){
                            std::wcout << L"        " << p->first << std::endl;
                            if (HV_CONNECTION_TYPE_HOST == h->connection_type){
                                foreach(std::wstring prop, p->second){
                                    std::wcout << L"           " << prop << std::endl;
                                }
                            }
                        }
                    }
                    catch (...){
                    }
                    std::wcout << std::endl << std::endl;
                }
            }   
        }
        else if (vm.count("all")){
            vmware_virtual_machine::vtr vms = portal->get_virtual_machines();
            foreach(vmware_virtual_machine::ptr v, vms){
                print_vm(v);
            }
        }
        else if (vm.count("vm") && !vm.count("vmdk") && !vm.count("list_changed_areas"))
        {
            if (vm.count("create") || vm.count("remove"))
            {
                if (vm.count("create"))
                {
                    vmware_vm_create_snapshot_parm::ptr snapshot_parm(new vmware_vm_create_snapshot_parm());
                    snapshot_parm->name = vm["create"].as<std::wstring>();
                    if (vm.count("count") && vm["count"].as<int>() > 0){
                        vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["vm"].as<std::wstring>());
                        if (v != NULL && (v->root_snapshot_list.size() > 0))
                        {
                            std::vector<vmware_virtual_machine_snapshots::ptr> total_snapshots = get_total_snapshots_in_tree(v->root_snapshot_list);
                            if ((total_snapshots.size() + 1) > vm["count"].as<int>()){
                                std::sort(total_snapshots.begin(), total_snapshots.end(), [](vmware_virtual_machine_snapshots::ptr const& lhs, vmware_virtual_machine_snapshots::ptr const& rhs){ return lhs->create_time < rhs->create_time; });
                                for(int index = 0; index < total_snapshots.size() + 1 - vm["count"].as<int>(); index++){                                  
                                    vmware_vm_remove_snapshot_parm::ptr snapshot_parm(new vmware_vm_remove_snapshot_parm());
                                    snapshot_parm->id = total_snapshots[index]->id;
                                    snapshot_parm->remove_children = false;
                                    snapshot_parm->consolidate = true;

                                    int nReturn = portal->remove_virtual_machine_snapshot(vm["vm"].as<std::wstring>(), *snapshot_parm, boost::bind(&snapshot_progress_callback, _1, _2));

                                    switch (nReturn)
                                    {
                                    case TASK_STATE::STATE_SUCCESS:
                                        std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed successfully." << std::endl;
                                        break;

                                    case TASK_STATE::STATE_ERROR:
                                        std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed failure." << std::endl;
                                        break;

                                    case TASK_STATE::STATE_QUEUED:
                                        std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" was queued to wait for remove." << std::endl;
                                        break;

                                    case TASK_STATE::STATE_TIMEOUT:
                                        std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed timeout." << std::endl;
                                        break;

                                    default:
                                        std::wcout << L"\nSnapshot id " << snapshot_parm->id << L", remove error " << nReturn << L"." << std::endl;
                                    }
                                }
                            }
                        }
                    }

                    if (vm.count("memory"))
                        snapshot_parm->include_memory = vm["memory"].as<bool>();

                    if (vm.count("quiescent"))
                        snapshot_parm->quiesce = vm["quiescent"].as<bool>();

                    if (vm.count("description"))
                        snapshot_parm->description = vm["description"].as<std::wstring>();

                    int nReturn = portal->create_virtual_machine_snapshot(vm["vm"].as<std::wstring>(), *snapshot_parm, boost::bind(&snapshot_progress_callback, _1, _2));
                    
                    switch (nReturn)
                    {
                    case TASK_STATE::STATE_SUCCESS:
                            std::wcout << L"\nSnapshot " << snapshot_parm->name << L"(" << snapshot_parm->snapshot_moref << L") created successfully." << std::endl;
                            break;

                    case TASK_STATE::STATE_ERROR:
                            std::wcout << L"\nSnapshot " << snapshot_parm->name << L" created failure." << std::endl;
                            break;

                    case TASK_STATE::STATE_QUEUED:
                            std::wcout << L"\nSnapshot " << snapshot_parm->name << L" was queued to wait for create." << std::endl;
                            break;

                    case TASK_STATE::STATE_TIMEOUT:
                            std::wcout << L"\nSnapshot " << snapshot_parm->name << L" created timeout." << std::endl;
                            break;

                    default:
                            std::wcout << L"\nSnapshot " << snapshot_parm->name << L", create error " << nReturn << L"." << std::endl;
                    }
                }
                else if (vm.count("remove"))
                {
                    vmware_vm_remove_snapshot_parm::ptr snapshot_parm(new vmware_vm_remove_snapshot_parm());

                    snapshot_parm->id = vm["remove"].as<int>();

                    if (vm.count("children"))
                        snapshot_parm->remove_children = vm["children"].as<bool>();

                    if (vm.count("consolidate"))
                        snapshot_parm->consolidate = vm["consolidate"].as<bool>();

                    int nReturn = portal->remove_virtual_machine_snapshot(vm["vm"].as<std::wstring>(), *snapshot_parm, boost::bind(&snapshot_progress_callback, _1, _2));

                    switch (nReturn)
                    {
                    case TASK_STATE::STATE_SUCCESS:
                            std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed successfully." << std::endl;
                            break;

                    case TASK_STATE::STATE_ERROR:
                            std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed failure." << std::endl;
                            break;

                    case TASK_STATE::STATE_QUEUED:
                            std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" was queued to wait for remove." << std::endl;
                            break;

                    case TASK_STATE::STATE_TIMEOUT:
                            std::wcout << L"\nSnapshot id " << snapshot_parm->id << L" removed timeout." << std::endl;
                            break;

                    default:
                            std::wcout << L"\nSnapshot id " << snapshot_parm->id << L", remove error " << nReturn << L"." << std::endl;
                    }
                }
            }
#ifdef VIXMNTAPI
            else if (vm.count("backup"))
            {
                image_clone_params clone_req;

                clone_req.src.ip = vm["host"].as<std::wstring>();
                clone_req.src.uname = vm["user"].as<std::wstring>();
                clone_req.src.passwd = vm["password"].as<std::wstring>();
                clone_req.dst_image_path_file = vm["backup"].as<std::wstring>();
                clone_req.src_machine_key = vm["vm"].as<std::wstring>();
                clone_req.src_image_path_file = L"";


                if (!boost::filesystem::is_directory(clone_req.dst_image_path_file))
                {
                    std::wcout << L"The destination location is not a folder." << std::endl;
                }
                else
                {
                    DWORD dwRtn = portal_ex->clone_image_to_local(clone_req, boost::bind(&clone_progress_callback, _1, _2, _3, _4, _5, _6), vm["image_type"].as<int>());
                    if (dwRtn != S_OK)
                        std::wcout << L"\nImage backup(clone) failure." << std::endl;
                    else
                        std::wcout << L"\nImage backup(clone) completed." << std::endl;
                }
            }
#endif
            else if (vm.count("enable_cbt"))
            {
                if (vm["enable_cbt"].as<bool>() == true)
                {
                    bool result = portal->enable_change_block_tracking(vm["vm"].as<std::wstring>());
                    if (result)
                        std::wcout << L"\nChange block tracking is enabled." << std::endl;
                    else
                        std::wcout << L"\nFailed to enable change block tracking." << std::endl;
                }
                else if (vm["enable_cbt"].as<bool>() == false)
                {
                    bool result = portal->disable_change_block_tracking(vm["vm"].as<std::wstring>());
                    if (result)
                        std::wcout << L"\nChange block tracking is disabled." << std::endl;
                    else
                        std::wcout << L"\nFailed to disable change block tracking." << std::endl;
                }
            }
            else
            {
                vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["vm"].as<std::wstring>());
                if (v != NULL)
                {
                    vmware_snapshot_disk_info::map snapshot;
                    if (v->is_cbt_enabled && vm.count("sm"))
                    {
                        std::wstring s_mor_item = vm["sm"].as<std::wstring>();
                        snapshot = portal->get_snapshot_info(s_mor_item);
                    }
                    print_vm(v);
                    print_vm_snapshot(snapshot);
                }
                else
                {
                    std::wcout << L"The specified virtual machine doesn't existing." << std::endl;
                }
            }
        }
        else if (vm.count("vm") && vm.count("list_changed_areas"))
        {
            vmware_virtual_machine::ptr v = portal->get_virtual_machine(vm["vm"].as<std::wstring>());
            if (v != NULL && v->is_cbt_enabled)
            {
                LONG64 start_offset = 0L;

                if (vm.count("sm"))
                {
                    if (v->root_snapshot_list.size() > 0)
                    {
                        std::wstring s_mor_item = vm["sm"].as<std::wstring>();
                        vmware_snapshot_disk_info::map snapshot_disks = portal->get_snapshot_info(s_mor_item);
                        foreach(vmware_snapshot_disk_info::map::value_type& p, snapshot_disks)
                        {
                            LONG64 disk_size = p.second->size;

                            //std::wcout << L"Disk: " << p->second << std::endl << L"Data range report(Sector)" << std::endl;
                            std::wcout << L"Disk: " << p.second->name << std::endl;
                            std::wcout << L"Change Id: " << p.second->change_id << std::endl << L"Data range report(Sector)" << std::endl;;

                            vmdk_changed_areas areas = portal->get_vmdk_changed_areas(v->vm_mor_item, s_mor_item, (std::wstring)p.second->wsz_key(), (std::wstring)vm["list_changed_areas"].as<std::wstring>(), start_offset, disk_size);
                            if (!areas.changed_list.empty())
                            {
                                foreach(changed_disk_extent& p, areas.changed_list)
                                {
                                    std::wcout << L"\tStart: " << p.start / VIXDISKLIB_SECTOR_SIZE << ", Length: " << p.length / VIXDISKLIB_SECTOR_SIZE << std::endl;
                                }

                                std::wcout << std::endl;
                            }
                            else
                            {
                                std::wcout << areas.error_description << L"(" << areas.last_error_code << L")" << std::endl;
                            }
                        }
                    }
                    else
                    {
                        std::wcout << L"No shapshot(s) created on this VM." << std::endl;
                    }
                }
                else
                {
                    std::wcout << L"Please specify snapshot name by --sm option." << std::endl;
                }
            }
            else
            {
                std::wcout << L"The specified virtual machine doesn't existing OR CBT is not enabled." << std::endl;
            }
        }
        else if (vm.count("cluster")){
            vmware_cluster::ptr cluster = portal->get_cluster(vm["cluster"].as<std::wstring>());
            //TODO
        }
        else if (vm.count("host") && vm.count("thumbprint"))
        {
            std::string thumbprint("");
            std::wstring host = vm["host"].as<std::wstring>();
            DWORD dwRtn = vmware_portal_ex::get_ssl_thumbprint(host, vm["thumbprint"].as<int>(), thumbprint);

            if (dwRtn == S_OK)
                std::wcout << L"Ths SSL thunmprint data of VMware ESX server " << host << L" on port " << vm["thumbprint"].as<int>() << L" is " << std::wstring(thumbprint.begin(), thumbprint.end()) << std::endl;
            else
                std::wcout << L"Failed to get SSL thunmprint data for the specific host " << host << std::endl;
        }
#ifdef VIXMNTAPI
        else if (vm.count("vmdk"))
        {
            std::wstring image_name = std::wstring(vm["vmdk"].as<std::wstring>().c_str());

            if (vm.count("mount"))
            {
                vol_mounted_info::vtr mounted_info;

                if (vm["mount"].as<std::wstring>() == L"ro")
                {
                    //read-only mount
                    mounted_info = portal_ex->mount_image_local(image_name, true);
                }
                else if (vm["mount"].as<std::wstring>() == L"rw")
                {
                    //read-write mount
                    mounted_info = portal_ex->mount_image_local(image_name);
                }
                else
                {
                    std::wcout << L"-m option only allows ro or rw value" << std::endl;
                    return 0;
                }

                if (mounted_info.size() > 0)
                {
                    std::wcout << L"Mount Information" << std::endl;

                    for (vol_mounted_info::vtr::iterator p = mounted_info.begin(); p != mounted_info.end(); p++)           
                    {
                        vol_mounted_info::ptr vol_mounted_info = (*p);
                        
                        std::wcout << L"    Volume     : " << vol_mounted_info->vol_info->symbolic_link << std::endl;
                        std::wcout << L"    Mount Point: " << vol_mounted_info->mount_point << std::endl;
                        std::wcout << L"    Type       : " << vol_mounted_info->vol_info->type << std::endl;

                        if (p != mounted_info.end())
                            std::wcout << std::endl;
                    }

                    std::wcout << std::endl << L"Press Enter key to unmount image(s)...";
                    while (std::cin.get() != '\n');

                    //BOOL result = portal_ex->unmount_image_local(image_name); //unmount specific image
                    BOOL result = portal_ex->unmount_image_local(); //unmount all

                    if (result)
                        std::wcout << L"Image(s) unmount successfully." << std::endl;
                    else
                        std::wcout << L"Image(s) unmount failure." << std::endl;
                }
                else
                    std::wcout << L"Failed to mount the image." << std::endl;
            }
            else if (vm.count("backup"))
            {
                image_clone_params clone_req;

                clone_req.src.ip = vm["host"].as<std::wstring>();
                clone_req.src.uname = vm["user"].as<std::wstring>();
                clone_req.src.passwd = vm["password"].as<std::wstring>();
                clone_req.dst_image_path_file = vm["backup"].as<std::wstring>();
                clone_req.src_machine_key = vm["vm"].as<std::wstring>();
                clone_req.src_image_path_file = image_name;
                
                DWORD dwRtn = portal_ex->clone_image_to_local(clone_req, boost::bind(&clone_progress_callback, _1, _2, _3, _4, _5, _6), vm["image_type"].as<int>());
                if (dwRtn != S_OK)
                    std::wcout << L"\nImage backup(clone) failure." << std::endl;
                else
                    std::wcout << L"\nImage backup(clone) completed." << std::endl;
            }
            else
            {
                image_info::ptr img_info = NULL;

                if (!vm.count("host"))
                    img_info = portal_ex->get_image_info_local(image_name);
                else
                    img_info = portal_ex->get_image_info_remote(vm["host"].as<std::wstring>(),
                                                                vm["user"].as<std::wstring>(),
                                                                vm["password"].as<std::wstring>(),
                                                                vm["vm"].as<std::wstring>(),
                                                                image_name);

                if (img_info != NULL)
                {
                    std::wcout << L"Image file: " << vm["vmdk"].as<std::wstring>() << std::endl;
                    std::wcout << L"    Adapter Type            : " << img_info->adapter_type << std::endl;
                    std::wcout << L"    Capacity in Sectors     : " << img_info->total_sectors << std::endl;
                    std::wcout << L"    Disk Signature          : " << img_info->disk_signature << std::endl;
                    std::wcout << L"    Number of Links         : " << img_info->num_links << std::endl;
                    std::wcout << L"    Physical Cylinders      : " << img_info->physical.cylinders << std::endl;
                    std::wcout << L"    Physical Heads          : " << img_info->physical.heads << std::endl;
                    std::wcout << L"    Physical Sectors        : " << img_info->physical.sectors << std::endl;
                    std::wcout << L"    Guest OS                : " << std::endl;
                    std::wcout << L"        Vendor              : " << img_info->os.vendor << std::endl;
                    std::wcout << L"        Family              : " << img_info->os.family << std::endl;
                    std::wcout << L"    Number of Volumes       : " << img_info->volumes.size() << std::endl;

                    for (volume_info::vtr::iterator p = img_info->volumes.begin(); p != img_info->volumes.end(); p++)
                    {
                        volume_info::ptr vol_info_ptr = *p;

                        std::wcout << L"        Volume Type         : " << vol_info_ptr->type << std::endl;
                        std::wcout << L"        Volume Mounted      : " << (vol_info_ptr->isMounted ? L"Yes" : L"No") << std::endl;
                        std::wcout << L"        Volume Symbolic Link: " << vol_info_ptr->symbolic_link << std::endl;

                        if (p != img_info->volumes.end())
                            std::wcout << std::endl;
                    }
                }
                else
                {
                    std::wcout << L"Failed to get image disk information." << std::endl;
                }
            }
        }
#endif
    }
    else
        print_cmd_examples();

    if (portal_ex)
    {
        portal_ex = NULL;
        vmware_portal_ex::vixdisk_exit();
    }

    if (portal)
        portal = NULL;

    return 0;
}

