// reg_cluster_edit.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_MS_CLUSTER__
#define __MACHO_WINDOWS_MS_CLUSTER__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include <vector>
#include "windows\wmi.hpp"

namespace macho{
namespace windows{
class cluster{
private:
    class logical_element{
    public:
        logical_element(wmi_object &obj) : _obj(obj){}
        virtual ~logical_element(){}
        virtual std::wstring    caption() { return _obj[L"Caption"]; }
        virtual std::wstring    description() { return _obj[L"Description"]; }
        virtual std::wstring    name() { return _obj[L"Name"]; }
        virtual std::wstring    status() { return _obj[L"Status"]; }
        virtual uint32_t        flags() { return _obj[L"Flags"]; }
        virtual uint32_t        characteristics() { return _obj[L"Characteristics"]; }
        /*
        virtual datetime      install_date() { return _obj[L"InstallDate"]; }
        */
    protected:
        macho::windows::wmi_object _obj;
    };
public:
    typedef boost::shared_ptr<cluster> ptr;
    class resource_group;
    typedef boost::shared_ptr<resource_group> resource_group_ptr;
    class resource;
    typedef boost::shared_ptr<resource> resource_ptr;
    class resource_type;
    typedef boost::shared_ptr<resource_type> resource_type_ptr;
    typedef std::vector<resource_type_ptr> resource_type_vtr;
    class node;
    typedef boost::shared_ptr<node> node_ptr;
    typedef std::vector<node_ptr> node_vtr;
    class disk;
    typedef boost::shared_ptr<disk> disk_ptr;
    class network;
    typedef boost::shared_ptr<network> network_ptr;
    class available_disk;
    typedef boost::shared_ptr<available_disk> available_disk_ptr;

    class disk_partition : virtual public logical_element{
    public:
        typedef boost::shared_ptr<disk_partition> ptr;
        typedef std::vector<ptr> vtr;
        disk_partition(wmi_object &obj) : logical_element(obj){}
        virtual ~disk_partition(){}

        virtual std::wstring    path() { return _obj[L"Path"]; }
        virtual std::wstring    volume_label() { return _obj[L"VolumeLabel"]; }
        virtual uint32_t        serial_number() { return _obj[L"SerialNumber"]; }
        virtual uint32_t        maximum_component_length() { return _obj[L"MaximumComponentLength"]; }
        virtual uint32_t        file_system_flags() { return _obj[L"FileSystemFlags"]; }
        virtual std::wstring    file_system() { return _obj[L"FileSystem"]; }
        virtual uint64_t        total_size() { return _obj[L"TotalSize"]; }
        virtual uint64_t        free_space() { return _obj[L"FreeSpace"]; }
        virtual uint32_t        partition_number() { return _obj[L"PartitionNumber"]; }
        virtual std::wstring    volume_guid() { return _obj[L"VolumeGuid"]; }
        virtual string_array_w  mount_points() { return _obj[L"MountPoints"]; }

        resource_ptr get_resource(){
            wmi_object obj = _obj.get_related(L"MSCluster_Resource", L"MSCluster_ResourceToDiskPartition");
            return resource::ptr(new resource(obj));
        }
        disk_ptr get_disk(){
            wmi_object obj = _obj.get_related(L"MSCluster_Disk", L"MSCluster_DiskToDiskPartition");
            return disk::ptr(new disk(obj));
        }
    };

    class disk : virtual public logical_element{
    public:
        typedef boost::shared_ptr<disk> ptr;
        typedef std::vector<ptr> vtr;
        disk(wmi_object &obj) : logical_element(obj){}
        virtual std::wstring    id() { return _obj[L"Id"]; }
        virtual uint32_t        signature() { return _obj[L"Signature"]; }
        virtual std::wstring    gpt_guid() { return _obj[L"GptGuid"]; }
        virtual uint32_t        scsi_port() { return _obj[L"ScsiPort"]; }
        virtual uint32_t        scsi_bus() { return _obj[L"ScsiBus"]; }
        virtual uint32_t        scsi_target_id() { return _obj[L"ScsiTargetID"]; }
        virtual uint32_t        scsi_lun() { return _obj[L"ScsiLUN"]; }
        virtual uint64_t        size() { return _obj[L"Size"]; }
        virtual uint32_t        number() { return _obj[L"Number"]; }
        virtual std::wstring    virtual_disk_id() { return _obj[L"VirtualDiskId"]; }
        virtual std::wstring    storage_pool_id() { return _obj[L"StoragePoolId"]; }

        virtual ~disk(){}
        resource_ptr get_resource(){
            wmi_object obj = _obj.get_related(L"MSCluster_Resource", L"MSCluster_ResourceToDisk");
            return resource::ptr(new resource(obj));
        }
        disk_partition::vtr get_partitions(){
            disk_partition::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_DiskPartition", L"MSCluster_DiskToDiskPartition");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(disk_partition::ptr(new disk_partition(*obj)));
            }
            return results;
        }
    };

    class available_disk_partition : virtual public disk_partition{
    public:
        typedef boost::shared_ptr<available_disk_partition> ptr;
        typedef std::vector<ptr> vtr;
        available_disk_partition(wmi_object &obj) : disk_partition(obj), logical_element(obj){}
        virtual ~available_disk_partition(){}

        available_disk_ptr get_disk(){
            wmi_object obj = _obj.get_related(L"MSCluster_AvailableDisk", L"MSCluster_AvailableDiskToPartition");
            return available_disk::ptr(new available_disk(obj));
        }
    };

    class available_disk : virtual public disk{
    public:
        typedef boost::shared_ptr<available_disk> ptr;
        typedef std::vector<ptr> vtr;
        available_disk(wmi_object &obj) : disk(obj), logical_element(obj){}
        virtual ~available_disk(){}

        virtual std::wstring    node() { return _obj[L"Node"]; }
        virtual std::wstring    resource_name() { return _obj[L"ResourceName"]; }
        virtual string_array_w  connected_nodes() { return _obj[L"ConnectedNodes"]; }

        available_disk_partition::vtr get_partitions(){
            available_disk_partition::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_AvailableDiskPartition", L"MSCluster_AvailableDiskToPartition");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(available_disk_partition::ptr(new available_disk_partition(*obj)));
            }
            return results;
        }
    };

    class shared_volume : virtual public logical_element{
    public:
        typedef boost::shared_ptr<shared_volume> ptr;
        typedef std::vector<ptr> vtr;
        shared_volume(wmi_object &obj) : logical_element(obj){}
        virtual ~shared_volume(){}
        virtual std::wstring    volume_name() { return _obj[L"VolumeName"]; }
        virtual uint32_t        fault_state() { return _obj[L"FaultState"]; }
        virtual uint64_t        volume_offset() { return _obj[L"VolumeOffset"]; }
        virtual uint32_t        backup_state() { return _obj[L"BackupState"]; }
    };

    class network_interface : virtual public logical_element{
    public:
        typedef boost::shared_ptr<network_interface> ptr;
        typedef std::vector<ptr> vtr;
        network_interface(wmi_object &obj) : logical_element(obj){}
        virtual ~network_interface(){}
        virtual std::wstring    system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
        virtual std::wstring    system_name() { return _obj[L"SystemName"]; }
        virtual std::wstring    creation_class_name() { return _obj[L"CreationClassName"]; }
        virtual std::wstring    device_id() { return _obj[L"DeviceID"]; }
        virtual bool            power_management_supported() { return _obj[L"PowerManagementSupported"]; }
        virtual uint16_table    power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
        virtual uint16_t        availability() { return _obj[L"Availability"]; }
        virtual uint32_t        config_manager_error_code() { return _obj[L"ConfigManagerErrorCode"]; }
        virtual bool            config_manager_user_config() { return _obj[L"ConfigManagerUserConfig"]; }
        virtual std::wstring    pnp_device_id() { return _obj[L"PNPDeviceID"]; }
        virtual uint16_t        status_info() { return _obj[L"StatusInfo"]; }
        virtual uint32_t        last_error_code() { return _obj[L"LastErrorCode"]; }
        virtual std::wstring    error_description() { return _obj[L"ErrorDescription"]; }
        virtual bool            error_cleared() { return _obj[L"ErrorCleared"]; }
        virtual string_array_w  other_identifying_info() { return _obj[L"OtherIdentifyingInfo"]; }
        virtual uint64_t        power_on_hours() { return _obj[L"PowerOnHours"]; }
        virtual uint64_t        total_power_on_hours() { return _obj[L"TotalPowerOnHours"]; }
        virtual string_array_w  identifying_descriptions() { return _obj[L"IdentifyingDescriptions"]; }
        virtual std::wstring    adapter() { return _obj[L"Adapter"]; }
        virtual std::wstring    adapter_id() { return _obj[L"AdapterId"]; }
        virtual std::wstring    node() { return _obj[L"Node"]; }
        virtual std::wstring    address() { return _obj[L"Address"]; }
        virtual std::wstring    network() { return _obj[L"Network"]; }
        virtual uint32_t        state() { return _obj[L"State"]; }
        virtual string_array_w  ipv6_addresses() { return _obj[L"IPv6Addresses"]; }
        virtual string_array_w  ipv4_addresses() { return _obj[L"IPv4Addresses"]; }
        virtual bool            dhcp_enabled() { return _obj[L"DhcpEnabled"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        network_ptr get_network(){
            wmi_object obj = _obj.get_related(L"MSCluster_Network", L"MSCluster_NetworkToNetworkInterface");
            return network::ptr(new macho::windows::cluster::network(obj));
        }
    };

    class network : virtual public logical_element{
    public:
        typedef boost::shared_ptr<network> ptr;
        typedef std::vector<ptr> vtr;
        network(wmi_object &obj) : logical_element(obj){}

        virtual ~network(){}
        virtual std::wstring    id() { return _obj[L"ID"]; }
        virtual std::wstring    address() { return _obj[L"Address"]; }
        virtual std::wstring    address_mask() { return _obj[L"AddressMask"]; }
        virtual uint32_t        role() { return _obj[L"Role"]; }
        virtual uint32_t        state() { return _obj[L"State"]; }
        virtual string_array_w  ipv6_addresses() { return _obj[L"IPv6Addresses"]; }
        virtual string_array_w  ipv6_prefix_lengths() { return _obj[L"IPv6PrefixLengths"]; }
        virtual string_array_w  ipv4_addresses() { return _obj[L"IPv4Addresses"]; }
        virtual string_array_w  ipv4_prefix_lengths() { return _obj[L"IPv4PrefixLengths"]; }
        virtual uint32_t        metric() { return _obj[L"Metric"]; }
        virtual bool            auto_metric() { return _obj[L"AutoMetric"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        network_interface::vtr get_network_interfaces(){
            network_interface::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_NetworkInterface", L"MSCluster_NetworkToNetworkInterface");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(network_interface::ptr(new network_interface(*obj)));
            }
            return results;
        }
    };

    class resource : virtual public logical_element{
    public:
        typedef boost::shared_ptr<resource> ptr;
        typedef std::vector<ptr> vtr;
        resource(wmi_object &obj) : logical_element(obj){}
        virtual ~resource(){}
        virtual std::wstring    id() { return _obj[L"Id"]; }
        virtual std::wstring    debug_prefix() { return _obj[L"DebugPrefix"]; }
        virtual uint32_t        is_alive_poll_interval() { return _obj[L"IsAlivePollInterval"]; }
        virtual uint32_t        looks_alive_poll_interval() { return _obj[L"LooksAlivePollInterval"]; }
        virtual uint32_t        pending_timeout() { return _obj[L"PendingTimeout"]; }
        virtual uint32_t        monitor_process_id() { return _obj[L"MonitorProcessId"]; }
        virtual bool            persistent_state() { return _obj[L"PersistentState"]; }
        virtual uint32_t        restart_action() { return _obj[L"RestartAction"]; }
        virtual uint32_t        restart_period() { return _obj[L"RestartPeriod"]; }
        virtual uint32_t        restart_threshold() { return _obj[L"RestartThreshold"]; }
        virtual uint32_t        embedded_failure_action() { return _obj[L"EmbeddedFailureAction"]; }
        virtual uint32_t        retry_period_on_failure() { return _obj[L"RetryPeriodOnFailure"]; }
        virtual bool            separate_monitor() { return _obj[L"SeparateMonitor"]; }
        virtual std::wstring    type() { return _obj[L"Type"]; }
        virtual uint32_t        state() { return _obj[L"State"]; }
        virtual std::wstring    internal_state() { return _obj[L"InternalState"]; }
        virtual uint32_t        resource_class() { return _obj[L"ResourceClass"]; }
        virtual uint32_t        subclass() { return _obj[L"Subclass"]; }
        virtual string_array_w  crypto_checkpoints() { return _obj[L"CryptoCheckpoints"]; }
        virtual string_array_w  registry_checkpoints() { return _obj[L"RegistryCheckpoints"]; }
        virtual bool            quorum_capable() { return _obj[L"QuorumCapable"]; }
        virtual bool            local_quorum_capable() { return _obj[L"LocalQuorumCapable"]; }
        virtual bool            delete_requires_all_nodes() { return _obj[L"DeleteRequiresAllNodes"]; }
        virtual bool            core_resource() { return _obj[L"CoreResource"]; }
        virtual uint32_t        deadlock_timeout() { return _obj[L"DeadlockTimeout"]; }
        virtual uint64_t        status_information() { return _obj[L"StatusInformation"]; }
        virtual uint64_t        last_operation_status_code() { return _obj[L"LastOperationStatusCode"]; }
        virtual uint64_t        resource_specific_data1() { return _obj[L"ResourceSpecificData1"]; }
        virtual uint64_t        resource_specific_data2() { return _obj[L"ResourceSpecificData2"]; }
        virtual std::wstring    resource_specific_status() { return _obj[L"ResourceSpecificStatus"]; }
        virtual uint32_t        restart_delay() { return _obj[L"RestartDelay"]; }
        virtual bool            is_cluster_shared_volume() { return _obj[L"IsClusterSharedVolume"]; }
        virtual string_array_w  required_dependency_types() { return _obj[L"RequiredDependencyTypes"]; }
        virtual uint32_table    required_dependency_classes() { return _obj[L"RequiredDependencyClasses"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        resource_group_ptr get_resource_group(){
            wmi_object obj = _obj.get_related(L"MSCluster_ResourceGroup", L"MSCluster_ResourceGroupToResource");
            return resource_group::ptr(new resource_group(obj));
        }
        
        resource::vtr get_dependent_resources(){
            resource::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_ResourceToDependentResource");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource::ptr(new resource(*obj)));
            }
            return results;
        }
        
        disk::vtr get_disks(){
            disk::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Disk", L"MSCluster_ResourceToDisk");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(disk::ptr(new disk(*obj)));
            }
            return results;
        }
        
        disk_partition::vtr get_disk_partitions(){
            disk_partition::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_DiskPartition", L"MSCluster_ResourceToDiskPartition");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(disk_partition::ptr(new disk_partition(*obj)));
            }
            return results;
        }

        node_vtr get_possible_owner(){
            node::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Node", L"MSCluster_ResourceToPossibleOwner");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(node::ptr(new node(*obj)));
            }
            return results;
        }
        /*
        resource_type_vtr get_resource_types(){
            resource_type::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_ResourceType", L"MSCluster_ResourceTypeToResource");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource_type::ptr(new resource_type(*obj)));
            }
            return results;
        }
        */
        resource_type_ptr get_resource_type(){
            wmi_object obj = _obj.get_related(L"MSCluster_ResourceType", L"MSCluster_ResourceTypeToResource");
            return resource_type::ptr(new resource_type(obj));
        }
    };

    class resource_type : virtual public logical_element{
    public:
        typedef boost::shared_ptr<resource_type> ptr;
        typedef std::vector<ptr> vtr;
        resource_type(wmi_object &obj) : logical_element(obj){}
        virtual ~resource_type(){}
        virtual std::wstring    display_name() { return _obj[L"DisplayName"]; }
        virtual string_array_w  admin_extensions() { return _obj[L"AdminExtensions"]; }
        virtual std::wstring    dll_name() { return _obj[L"DllName"]; }
        virtual uint32_t        is_alive_poll_interval() { return _obj[L"IsAlivePollInterval"]; }
        virtual uint32_t        looks_alive_poll_interval() { return _obj[L"LooksAlivePollInterval"]; }
        virtual bool            quorum_capable() { return _obj[L"QuorumCapable"]; }
        virtual bool            local_quorum_capable() { return _obj[L"LocalQuorumCapable"]; }
        virtual bool            delete_requires_all_nodes() { return _obj[L"DeleteRequiresAllNodes"]; }
        virtual uint32_t        deadlock_timeout() { return _obj[L"DeadlockTimeout"]; }
        virtual uint32_t        pending_timeout() { return _obj[L"PendingTimeout"]; }
        virtual uint64_t        dump_policy() { return _obj[L"DumpPolicy"]; }
        virtual string_array_w  dump_log_query() { return _obj[L"DumpLogQuery"]; }
        virtual string_array_w  enabled_event_logs() { return _obj[L"EnabledEventLogs"]; }
        virtual string_array_w  required_dependency_types() { return _obj[L"RequiredDependencyTypes"]; }
        virtual uint32_t        resource_class() { return _obj[L"ResourceClass"]; }
        virtual uint32_table    required_dependency_classes() { return _obj[L"RequiredDependencyClasses"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        resource::vtr get_resources(){
            resource::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_ResourceTypeToResource");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource::ptr(new resource(*obj)));
            }
            return results;
        }
        
        node_vtr get_possible_owner(){
            node::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Node", L"MSCluster_ResourceTypeToPossibleOwner");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(node::ptr(new node(*obj)));
            }
            return results;
        }
    };

    class resource_group : virtual public logical_element{
    public:
        typedef boost::shared_ptr<resource_group> ptr;
        typedef std::vector<ptr> vtr;
        resource_group(wmi_object &obj) : logical_element(obj){}
        virtual ~resource_group(){}
        virtual std::wstring    name() { return L""; }
        virtual uint32_t        state() { return _obj[L"State"]; }
        virtual uint32_t        auto_failback_type() { return _obj[L"AutoFailbackType"]; }
        virtual uint32_t        failback_window_end() { return _obj[L"FailbackWindowEnd"]; }
        virtual uint32_t        failback_window_start() { return _obj[L"FailbackWindowStart"]; }
        virtual uint32_t        failover_period() { return _obj[L"FailoverPeriod"]; }
        virtual uint32_t        failover_threshold() { return _obj[L"FailoverThreshold"]; }
        virtual std::wstring    id() { return _obj[L"Id"]; }
        virtual bool            persistent_state() { return _obj[L"PersistentState"]; }
        virtual string_array_w  anti_affinity_class_names() { return _obj[L"AntiAffinityClassNames"]; }
        virtual std::wstring    internal_state() { return _obj[L"InternalState"]; }
        virtual uint32_t        priority() { return _obj[L"Priority"]; }
        virtual uint32_t        default_owner() { return _obj[L"DefaultOwner"]; }
        virtual uint64_t        ccfepoch() { return _obj[L"CCFEpoch"]; }
        virtual uint32_t        resiliency_period() { return _obj[L"ResiliencyPeriod"]; }
        virtual uint32_t        group_type() { return _obj[L"GroupType"]; }
        virtual std::wstring    owner_node() { return _obj[L"OwnerNode"]; }
        virtual bool            is_core() { return _obj[L"IsCore"]; }
        virtual uint64_t        status_information() { return _obj[L"StatusInformation"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        resource::vtr get_resources(){
            resource::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_ResourceGroupToResource");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource::ptr(new resource(*obj)));
            }
            return results;
        }

        node_vtr get_preferred_nodes(){
            node::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Node", L"MSCluster_ResourceGroupToPreferredNode");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(node::ptr(new node(*obj)));
            }
            return results;
        }
    };

    class service{
    public:
        typedef boost::shared_ptr<service> ptr;
        service(wmi_object &obj) : _obj(obj){}
        virtual ~service(){}
        virtual std::wstring    caption() { return _obj[L"Caption"]; }
        virtual std::wstring    creation_class_name() { return _obj[L"CreationClassName"]; }
        virtual std::wstring    description() { return _obj[L"Description"]; }
        virtual std::wstring    name() { return _obj[L"Name"]; }
        virtual bool            started() { return _obj[L"Started"]; }
        virtual std::wstring    start_mode() { return _obj[L"StartMode"]; }
        virtual std::wstring    status() { return _obj[L"Status"]; }
        virtual std::wstring    system_creation_class_name() { return _obj[L"SystemCreationClassName"]; }
        virtual std::wstring    system_name() { return _obj[L"SystemName"]; }
        virtual uint32_t        node_highest_version() { return _obj[L"NodeHighestVersion"]; }
        virtual uint32_t        node_lowest_version() { return _obj[L"NodeLowestVersion"]; }
        virtual uint32_t        enable_event_log_replication() { return _obj[L"EnableEventLogReplication"]; }
        virtual std::wstring    state() { return _obj[L"State"]; }
        //virtual datetime      install_date() { return _obj[L"InstallDate"]; }

    private:
        macho::windows::wmi_object _obj;
    };

    class node : virtual public logical_element{
    public:
        typedef boost::shared_ptr<node> ptr;
        typedef std::vector<ptr> vtr;
        node(wmi_object &obj) : logical_element(obj){}
        virtual ~node(){}

        virtual std::wstring    creation_class_name() { return _obj[L"CreationClassName"]; }
        virtual string_array_w  initial_load_info() { return _obj[L"InitialLoadInfo"]; }
        virtual std::wstring    last_load_info() { return _obj[L"LastLoadInfo"]; }
        virtual std::wstring    name_format() { return _obj[L"NameFormat"]; }
        virtual string_array_w  other_identifying_info() { return _obj[L"OtherIdentifyingInfo"]; }
        virtual string_array_w  identifying_descriptions() { return _obj[L"IdentifyingDescriptions"]; }
        virtual uint16_table    dedicated() { return _obj[L"Dedicated"]; }
        virtual uint16_table    power_management_capabilities() { return _obj[L"PowerManagementCapabilities"]; }
        virtual bool            power_management_supported() { return _obj[L"PowerManagementSupported"]; }
        virtual uint16_t        power_state() { return _obj[L"PowerState"]; }
        virtual std::wstring    primary_owner_contact() { return _obj[L"PrimaryOwnerContact"]; }
        virtual std::wstring    primary_owner_name() { return _obj[L"PrimaryOwnerName"]; }
        virtual uint16_t        reset_capability() { return _obj[L"ResetCapability"]; }
        virtual string_array_w  roles() { return _obj[L"Roles"]; }
        virtual uint32_t        node_weight() { return _obj[L"NodeWeight"]; }
        virtual uint32_t        dynamic_weight() { return _obj[L"DynamicWeight"]; }
        virtual uint32_t        node_highest_version() { return _obj[L"NodeHighestVersion"]; }
        virtual uint32_t        node_lowest_version() { return _obj[L"NodeLowestVersion"]; }
        virtual uint32_t        major_version() { return _obj[L"MajorVersion"]; }
        virtual uint32_t        minor_version() { return _obj[L"MinorVersion"]; }
        virtual uint32_t        build_number() { return _obj[L"BuildNumber"]; }
        virtual uint32_t        csd_version() { return _obj[L"CSDVersion"]; }
        virtual std::wstring    id() { return _obj[L"Id"]; }
        virtual std::wstring    node_instance_id() { return _obj[L"NodeInstanceID"]; }
        virtual uint32_t        node_drain_status() { return _obj[L"NodeDrainStatus"]; }
        virtual std::wstring    node_drain_target() { return _obj[L"NodeDrainTarget"]; }
        virtual uint32_t        enable_event_log_replication() { return _obj[L"EnableEventLogReplication"]; }
        virtual uint32_t        state() { return _obj[L"State"]; }
        virtual uint32_t        needs_prevent_quorum() { return _obj[L"NeedsPreventQuorum"]; }
        virtual std::wstring    fault_domain_id() { return _obj[L"FaultDomainId"]; }
        virtual uint32_t        status_information() { return _obj[L"StatusInformation"]; }
        virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

        resource_group::vtr get_active_groups(){
            resource_group::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_ResourceGroup", L"MSCluster_NodeToActiveGroup");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource_group::ptr(new resource_group(*obj)));
            }
            return results;
        }
        resource::vtr get_active_resources(){
            resource::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_NodeToActiveResource");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(resource::ptr(new resource(*obj)));
            }
            return results;
        }

        network_interface::vtr get_network_interfaces(){
            network_interface::vtr results;
            wmi_object_table objs = _obj.get_relateds(L"MSCluster_NetworkInterface", L"MSCluster_NodeToNetworkInterface");
            for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
                results.push_back(network_interface::ptr(new network_interface(*obj)));
            }
            return results;
        }

        service::ptr get_hosted_service(){
            wmi_object obj = _obj.get_related(L"MSCluster_Service", L"MSCluster_NodeToHostedService");
            return service::ptr(new service(obj));
        }
    };


    node::vtr get_nodes(){
        node::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_Node", L"MSCluster_ClusterToNode");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(node::ptr(new node(*obj)));
        }
        return results;
    }

    resource::vtr get_resources(){
        resource::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_ClusterToResource");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(resource::ptr(new resource(*obj)));
        }
        return results;
    }

    resource_group::vtr get_resource_groups(){
        resource_group::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_ResourceGroup", L"MSCluster_ClusterToResourceGroup");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(resource_group::ptr(new resource_group(*obj)));
        }
        return results;
    }

    resource_type::vtr get_resource_types(){
        resource_type::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_ResourceType", L"MSCluster_ClusterToResourceType");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(resource_type::ptr(new resource_type(*obj)));
        }
        return results;
    }

    available_disk::vtr get_available_disks(){
        available_disk::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_AvailableDisk", L"MSCluster_ClusterToAvailableDisk");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(available_disk::ptr(new available_disk(*obj)));
        }
        return results;
    }

    shared_volume::vtr get_shared_volumes(){
        shared_volume::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_ClusterSharedVolume", L"MSCluster_ClusterToClusterSharedVolume");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(shared_volume::ptr(new shared_volume(*obj)));
        }
        return results;
    }

    network::vtr get_networks(){
        network::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_Network", L"MSCluster_ClusterToNetwork");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(network::ptr(new network(*obj)));
        }
        return results;
    }

    network_interface::vtr get_network_interfaces(){
        network_interface::vtr results;
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_NetworkInterface", L"MSCluster_ClusterToNetworkInterface");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(network_interface::ptr(new network_interface(*obj)));
        }
        return results;
    }

    resource::ptr get_quorum_resource(){
        wmi_object_table objs = _obj.get_relateds(L"MSCluster_Resource", L"MSCluster_ClusterToQuorumResource");
        if (objs.size())
            return resource::ptr(new resource(objs[0]));
        return NULL;
    }
    
    disk::vtr get_disks(){
        disk::vtr results;
        wmi_object_table objs = _mscluster.query_wmi_objects(L"MSCluster_Disk");
        for (wmi_object_table::iterator obj = objs.begin(); obj != objs.end(); obj++){
            results.push_back(disk::ptr(new disk(*obj)));
        }
        return results;
    }

private:
    cluster(macho::windows::wmi_services &mscluster, wmi_object &obj) : _mscluster(mscluster), _obj(obj){}
    macho::windows::wmi_object   _obj;
    macho::windows::wmi_services _mscluster;
public:
    virtual std::wstring    caption() { return _obj[L"Caption"]; }
    virtual std::wstring    status() { return _obj[L"Status"]; }
    virtual std::wstring    creation_class_name() { return _obj[L"CreationClassName"]; }
    virtual std::wstring    primary_owner_contact() { return _obj[L"PrimaryOwnerContact"]; }
    virtual std::wstring    primary_owner_name() { return _obj[L"PrimaryOwnerName"]; }
    virtual string_array_w  roles() { return _obj[L"Roles"]; }
    virtual std::wstring    name_format() { return _obj[L"NameFormat"]; }
    virtual string_array_w  other_identifying_info() { return _obj[L"OtherIdentifyingInfo"]; }
    virtual string_array_w  identifying_descriptions() { return _obj[L"IdentifyingDescriptions"]; }
    virtual uint16_table    dedicated() { return _obj[L"Dedicated"]; }
    virtual uint32_t        max_number_of_nodes() { return _obj[L"MaxNumberOfNodes"]; }
    virtual std::wstring    name() { return _obj[L"Name"]; }
    virtual std::wstring    fqdn() { return _obj[L"Fqdn"]; }
    virtual std::wstring    description() { return _obj[L"Description"]; }
    virtual std::wstring    maintenance_file() { return _obj[L"MaintenanceFile"]; }
    virtual string_array_w  admin_extensions() { return _obj[L"AdminExtensions"]; }
    virtual string_array_w  group_admin_extensions() { return _obj[L"GroupAdminExtensions"]; }
    virtual string_array_w  node_admin_extensions() { return _obj[L"NodeAdminExtensions"]; }
    virtual string_array_w  resource_admin_extensions() { return _obj[L"ResourceAdminExtensions"]; }
    virtual string_array_w  resource_type_admin_extensions() { return _obj[L"ResourceTypeAdminExtensions"]; }
    virtual string_array_w  network_admin_extensions() { return _obj[L"NetworkAdminExtensions"]; }
    virtual string_array_w  network_interface_admin_extensions() { return _obj[L"NetworkInterfaceAdminExtensions"]; }
    virtual uint32_t        quorum_log_file_size() { return _obj[L"QuorumLogFileSize"]; }
    virtual std::wstring    quorum_type() { return _obj[L"QuorumType"]; }
    virtual std::wstring    quorum_path() { return _obj[L"QuorumPath"]; }
    virtual uint16_t        quorum_type_value() { return _obj[L"QuorumTypeValue"]; } //uint8
    virtual string_array_w  network_priorities() { return _obj[L"NetworkPriorities"]; }
    virtual uint32_t        default_network_role() { return _obj[L"DefaultNetworkRole"]; }
    virtual uint32_t        enable_event_log_replication() { return _obj[L"EnableEventLogReplication"]; }
    virtual std::wstring    shared_volumes_root() { return _obj[L"SharedVolumesRoot"]; }
    virtual std::wstring    use_client_access_networks_for_shared_volumes() { return _obj[L"UseClientAccessNetworksForSharedVolumes"]; }
    virtual std::wstring    recent_events_reset_time() { return _obj[L"RecentEventsResetTime"]; }
    virtual string_array_w  shared_volume_compatible_filters() { return _obj[L"SharedVolumeCompatibleFilters"]; }
    virtual string_array_w  shared_volume_incompatible_filters() { return _obj[L"SharedVolumeIncompatibleFilters"]; }
    virtual uint64_t        dump_policy() { return _obj[L"DumpPolicy"]; }
    virtual uint32_t        quorum_arbitration_time_max() { return _obj[L"QuorumArbitrationTimeMax"]; }
    virtual uint32_t        quorum_arbitration_time_min() { return _obj[L"QuorumArbitrationTimeMin"]; }
    virtual uint32_t        disable_group_preferred_owner_randomization() { return _obj[L"DisableGroupPreferredOwnerRandomization"]; }
    virtual uint32_t        resource_dll_deadlock_period() { return _obj[L"ResourceDllDeadlockPeriod"]; }
    virtual uint32_t        clus_svc_hang_timeout() { return _obj[L"ClusSvcHangTimeout"]; }
    virtual uint32_t        clus_svc_regroup_opening_timeout() { return _obj[L"ClusSvcRegroupOpeningTimeout"]; }
    virtual uint32_t        plumb_all_cross_subnet_routes() { return _obj[L"PlumbAllCrossSubnetRoutes"]; }
    virtual uint32_t        clus_svc_regroup_stage_timeout() { return _obj[L"ClusSvcRegroupStageTimeout"]; }
    virtual uint32_t        clus_svc_regroup_pruning_timeout() { return _obj[L"ClusSvcRegroupPruningTimeout"]; }
    virtual uint32_t        clus_svc_regroup_tick_in_milliseconds() { return _obj[L"ClusSvcRegroupTickInMilliseconds"]; }
    virtual uint32_t        cluster_log_size() { return _obj[L"ClusterLogSize"]; }
    virtual uint32_t        cluster_log_level() { return _obj[L"ClusterLogLevel"]; }
    virtual uint32_t        log_resource_controls() { return _obj[L"LogResourceControls"]; }
    virtual uint32_t        hang_recovery_action() { return _obj[L"HangRecoveryAction"]; }
    virtual uint32_t        same_subnet_delay() { return _obj[L"SameSubnetDelay"]; }
    virtual uint32_t        cross_subnet_delay() { return _obj[L"CrossSubnetDelay"]; }
    virtual uint32_t        same_subnet_threshold() { return _obj[L"SameSubnetThreshold"]; }
    virtual uint32_t        cross_subnet_threshold() { return _obj[L"CrossSubnetThreshold"]; }
    virtual uint32_t        backup_in_progress() { return _obj[L"BackupInProgress"]; }
    virtual uint32_t        request_reply_timeout() { return _obj[L"RequestReplyTimeout"]; }
    virtual uint32_t        witness_restart_interval() { return _obj[L"WitnessRestartInterval"]; }
    virtual uint32_t        security_level() { return _obj[L"SecurityLevel"]; }
    virtual uint32_t        witness_database_write_timeout() { return _obj[L"WitnessDatabaseWriteTimeout"]; }
    virtual uint32_t        add_evict_delay() { return _obj[L"AddEvictDelay"]; }
    virtual uint32_t        fix_quorum() { return _obj[L"FixQuorum"]; }
    virtual uint32_t        prevent_quorum() { return _obj[L"PreventQuorum"]; }
    virtual uint32_t        ignore_persistent_state_on_startup() { return _obj[L"IgnorePersistentStateOnStartup"]; }
    virtual uint32_t        witness_dynamic_weight() { return _obj[L"WitnessDynamicWeight"]; }
    virtual uint32_t        admin_access_point() { return _obj[L"AdminAccessPoint"]; }
    virtual uint32_t        cluster_functional_level() { return _obj[L"ClusterFunctionalLevel"]; }
    virtual uint32_t        resiliency_level() { return _obj[L"ResiliencyLevel"]; }
    virtual uint32_t        resiliency_default_period() { return _obj[L"ResiliencyDefaultPeriod"]; }
    virtual uint32_t        grace_period_enabled() { return _obj[L"GracePeriodEnabled"]; }
    virtual uint32_t        grace_period_timeout() { return _obj[L"GracePeriodTimeout"]; }
    virtual uint32_t        quarantine_duration() { return _obj[L"QuarantineDuration"]; }
    virtual uint32_t        enable_shared_volumes() { return _obj[L"EnableSharedVolumes"]; }
    virtual uint32_t        block_cache_size() { return _obj[L"BlockCacheSize"]; }
    virtual uint32_t        cluster_enforced_anti_affinity() { return _obj[L"ClusterEnforcedAntiAffinity"]; }
    virtual uint32_t        shared_volume_vss_writer_operation_timeout() { return _obj[L"SharedVolumeVssWriterOperationTimeout"]; }
    virtual uint32_t        cluster_group_wait_delay() { return _obj[L"ClusterGroupWaitDelay"]; }
    virtual uint32_t        minimum_preemptor_priority() { return _obj[L"MinimumPreemptorPriority"]; }
    virtual uint32_t        minimum_never_preempt_priority() { return _obj[L"MinimumNeverPreemptPriority"]; }
    virtual uint32_t        shutdown_timeout_in_minutes() { return _obj[L"ShutdownTimeoutInMinutes"]; }
    virtual uint32_t        root_memory_reserved() { return _obj[L"RootMemoryReserved"]; }
    virtual uint32_t        route_history_length() { return _obj[L"RouteHistoryLength"]; }
    virtual uint32_t        dynamic_quorum_enabled() { return _obj[L"DynamicQuorumEnabled"]; }
    virtual uint32_t        drain_on_shutdown() { return _obj[L"DrainOnShutdown"]; }
    virtual uint32_t        database_read_write_mode() { return _obj[L"DatabaseReadWriteMode"]; }
    virtual uint32_t        netft_ipsec_enabled() { return _obj[L"NetftIPSecEnabled"]; }
    virtual uint32_t        csv_balancer() { return _obj[L"CsvBalancer"]; }
    virtual uint32_t        message_buffer_length() { return _obj[L"MessageBufferLength"]; }
    virtual uint32_t        virtual_storage_array_notification_interval() { return _obj[L"VirtualStorageArrayNotificationInterval"]; }
    virtual uint32_t        virtual_storage_array_io_latency_threshold() { return _obj[L"VirtualStorageArrayIOLatencyThreshold"]; }
    virtual uint32_t        virtual_storage_array_storage_opt_flags() { return _obj[L"VirtualStorageArrayStorageOptFlags"]; }
    virtual uint32_t        virtual_storage_array_storage_bus_types() { return _obj[L"VirtualStorageArrayStorageBusTypes"]; }
    virtual uint32_t        virtual_storage_array_enabled() { return _obj[L"VirtualStorageArrayEnabled"]; }
    virtual uint32_t        das_mode_enabled() { return _obj[L"DASModeEnabled"]; }
    virtual uint32_t        das_mode_bus_types() { return _obj[L"DASModeBusTypes"]; }
    virtual uint32_t        das_mode_optimizations() { return _obj[L"DASModeOptimizations"]; }
    virtual uint32_t        das_mode_io_latency_threshold() { return _obj[L"DASModeIOLatencyThreshold"]; }
    virtual uint32_t        lower_quorum_priority_node_id() { return _obj[L"LowerQuorumPriorityNodeId"]; }
    virtual wmi_object      private_properties() { return _obj[L"PrivateProperties"].get_wmi_object(); }

    /*
    //virtual datetime      installDate() { return _obj[L"InstallDate"]; }
    uint8              Security[];
    uint8              Security_Descriptor[];
    uint8              SharedVolumeSecurityDescriptor[];  
    */

static cluster::ptr get(std::wstring host = L"", std::wstring user = L"", std::wstring password = L"") {
    std::wstring domain, account;
    string_array arr = stringutils::tokenize2(user, L"\\", 2, false);
    if (arr.size()){
        if (arr.size() == 1)
            account = arr[0];
        else{
            domain = arr[0];
            account = arr[1];
        }
    }
    return get(host, domain,account, password);
}

static cluster::ptr get(std::wstring host, std::wstring domain, std::wstring user, std::wstring password ) {
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    try{
        macho::windows::wmi_services mscluster;
        if (host.length())
            hr = mscluster.connect(L"MSCluster", host, domain, user, password, RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_LEVEL_PKT_PRIVACY);
        else
            hr = mscluster.connect(L"MSCluster");
        if (SUCCEEDED(hr)){
            return cluster::ptr(new cluster(mscluster, mscluster.query_wmi_object(L"MSCluster_Cluster")));
        }
    }
    catch (...){
    }
    return NULL;
}

static cluster::ptr get(macho::windows::wmi_services& root) {
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    try{
        macho::windows::wmi_services mscluster;
        hr = root.open_namespace(L"MSCluster", mscluster);
        if (SUCCEEDED(hr)){
            return cluster::ptr(new cluster(mscluster, mscluster.query_wmi_object(L"MSCluster_Cluster")));
        }
    }
    catch (...){}
    return NULL;
}
};
};
};




#endif