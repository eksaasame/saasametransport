#include "virtual_packer_service_handler.h"
#include "vmware_ex.h"
using namespace mwdc::ironman::hypervisor_ex;

void get_folder_list(vmware_folder::vtr &folder, std::vector<std::string> &_return){
    foreach(vmware_folder::ptr f, folder){
        _return.push_back(macho::stringutils::convert_unicode_to_utf8(f->path()));
        get_folder_list(f->folders, _return);
    }
}

virtual_machine_snapshots get_virtual_machine_snapshot_from_object(vmware_virtual_machine_snapshots::ptr child){
    boost::shared_ptr<saasame::transport::virtual_machine_snapshots> _snapshot(new saasame::transport::virtual_machine_snapshots());
    _snapshot->name = macho::stringutils::convert_unicode_to_utf8(child->name);
    _snapshot->create_time = std::to_string(child->create_time);
    _snapshot->description = macho::stringutils::convert_unicode_to_utf8(child->description);
    _snapshot->quiesced = child->quiesced;
    _snapshot->id = child->id;
    _snapshot->backup_manifest = macho::stringutils::convert_unicode_to_utf8(child->backup_manifest);
    _snapshot->replay_supported = child->replay_supported;

    foreach(vmware_virtual_machine_snapshots::ptr snapshot_list, child->child_snapshot_list)
    {
        _snapshot->child_snapshot_list.push_back(get_virtual_machine_snapshot_from_object(snapshot_list));
    }
    _snapshot->__set_child_snapshot_list(_snapshot->child_snapshot_list);

    return *_snapshot.get();
}

std::vector<vmware_snapshot>  _get_virtual_machine_snapshots(vmware_virtual_machine_snapshots::ptr child){
    std::vector<vmware_snapshot> ret;
    vmware_snapshot s;
    s.name = s.id = macho::stringutils::convert_unicode_to_utf8(child->name);
    s.datetime = std::to_string(child->create_time);
    ret.push_back(s);
    foreach(vmware_virtual_machine_snapshots::ptr snapshot_list, child->child_snapshot_list){
        std::vector<vmware_snapshot>  r = _get_virtual_machine_snapshots(snapshot_list);
        ret.insert(ret.end(), r.begin(), r.end());
    }
    return ret;
}

virtual_packer_service_handler::virtual_packer_service_handler(std::wstring session_id){
    _session = macho::guid_(session_id);
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"ConcurrentVirtualPackerJobNumber"].exists() && (DWORD)reg[L"ConcurrentVirtualPackerJobNumber"] > 0){
            _scheduler.initial((DWORD)reg[L"ConcurrentVirtualPackerJobNumber"]);
        }
    }
    _scheduler.start();
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(boost::str(boost::format("%s\\snapshots management") % saasame::transport::g_saasame_constants.CONFIG_PATH)))){
        reg.refresh_subkeys();
        if (reg.subkeys_count())
            _mgmt.trigger();
    }
    load_jobs();
}

void virtual_packer_service_handler::get_virtual_host_info(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password){
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;

    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host '%1%'.") % host);
        error.format = "Cannot connect to the host '%1%'.";
        error.arguments.push_back(host);
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to the host %s", macho::stringutils::convert_utf8_to_unicode(host).c_str());
        throw error;
    }
    vmware_virtual_center virtual_center;
    mwdc::ironman::hypervisor::hv_connection_type connection_type = portal->get_connection_type(virtual_center);
    if (mwdc::ironman::hypervisor::hv_connection_type::HV_CONNECTION_TYPE_VCENTER == connection_type){
        _return.__set_connection_type((saasame::transport::hv_connection_type::type)connection_type);
        _return.__set_virtual_center_name(macho::stringutils::convert_unicode_to_utf8(virtual_center.product_name));
        _return.__set_virtual_center_version(macho::stringutils::convert_unicode_to_utf8(virtual_center.version));
        key_map vms = portal->get_all_virtual_machines();
        foreach(key_map::value_type& vm, vms)
            _return.vms[macho::stringutils::convert_unicode_to_utf8(vm.first)] = macho::stringutils::convert_unicode_to_utf8(vm.second);
    }
    else{
        vmware_host::vtr hosts = portal->get_hosts();
        portal = NULL;
        foreach(vmware_host::ptr h, hosts)
        {
            if (h->name.size() > 0)
                _return.__set_name(macho::stringutils::convert_unicode_to_utf8(*h->name.begin()));
            if (h->domain_name.size() > 0)
                _return.__set_domain_name(macho::stringutils::convert_unicode_to_utf8(*h->domain_name.begin()));
            _return.__set_name_ref(macho::stringutils::convert_unicode_to_utf8(h->name_ref));
            _return.__set_full_name(macho::stringutils::convert_unicode_to_utf8(h->full_name()));
            _return.__set_cluster_key(macho::stringutils::convert_unicode_to_utf8(h->cluster_key));
            _return.__set_datacenter_name(macho::stringutils::convert_unicode_to_utf8(h->datacenter_name));
            _return.__set_product_name(macho::stringutils::convert_unicode_to_utf8(h->product_name));
            _return.__set_version(macho::stringutils::convert_unicode_to_utf8(h->version));
            _return.__set_in_maintenance_mode(h->in_maintenance_mode);
            _return.__set_power_state((saasame::transport::hv_host_power_state::type)h->power_state);
            _return.__set_state(macho::stringutils::convert_unicode_to_utf8(h->state));
            _return.__set_ip_address(macho::stringutils::convert_unicode_to_utf8(h->ip_address));
            _return.__set_uuid(macho::stringutils::convert_unicode_to_utf8(h->uuid));
            _return.__set_number_of_cpu_cores(h->number_of_cpu_cores);
            _return.__set_number_of_cpu_packages(h->number_of_cpu_packages);
            _return.__set_number_of_cpu_threads(h->number_of_cpu_threads);
            _return.__set_size_of_memory(h->size_of_memory);

            foreach(std::wstring ip, h->ip_addresses)
            {
                _return.ip_addresses.push_back(macho::stringutils::convert_unicode_to_utf8(ip));
            }
            _return.__set_ip_addresses(_return.ip_addresses);

            for (key_map::iterator ds = h->datastores.begin(); ds != h->datastores.end(); ds++)
            {
                _return.datastores[macho::stringutils::convert_unicode_to_utf8(ds->first)] = macho::stringutils::convert_unicode_to_utf8(ds->second);
            }
            _return.__set_datastores(_return.datastores);

            for (key_map::iterator net = h->networks.begin(); net != h->networks.end(); net++)
            {
                _return.networks[macho::stringutils::convert_unicode_to_utf8(net->first)] = macho::stringutils::convert_unicode_to_utf8(net->second);
            }
            _return.__set_networks(_return.networks);

            foreach(std::wstring n, h->name)
            {
                _return.name_list.push_back(macho::stringutils::convert_unicode_to_utf8(n));
            }
            _return.__set_name_list(_return.name_list);

            foreach(std::wstring dn, h->domain_name)
            {
                _return.domain_name_list.push_back(macho::stringutils::convert_unicode_to_utf8(dn));
            }
            _return.__set_domain_name_list(_return.domain_name_list);

            for (license_map::iterator p = h->features.begin(); p != h->features.end(); p++)
            {
                std::vector<std::string> props;

                _return.lic_features[macho::stringutils::convert_unicode_to_utf8(p->first)] = props;
                foreach(std::wstring prop, p->second)
                    _return.lic_features[macho::stringutils::convert_unicode_to_utf8(p->first)].push_back(macho::stringutils::convert_unicode_to_utf8(prop));
            }
            _return.__set_lic_features(_return.lic_features);
            _return.__set_connection_type((saasame::transport::hv_connection_type::type)h->connection_type);
            _return.__set_virtual_center_name(macho::stringutils::convert_unicode_to_utf8(h->virtual_center.product_name));
            _return.__set_virtual_center_version(macho::stringutils::convert_unicode_to_utf8(h->virtual_center.version));
            break;
        }

        foreach(vmware_host::ptr h, hosts)
        {
            for (key_map::iterator vm = h->vms.begin(); vm != h->vms.end(); vm++)
            {
                _return.vms[macho::stringutils::convert_unicode_to_utf8(vm->first)] = macho::stringutils::convert_unicode_to_utf8(vm->second);
            }
        }
    }
    _return.__set_vms(_return.vms);
}

void virtual_packer_service_handler::get_virtual_machine_detail(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;

    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host '%1%'.") % host);
        error.format = "Cannot connect to the host '%1%'.";
        error.arguments.push_back(host);
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to the host %s", macho::stringutils::convert_utf8_to_unicode(host).c_str());
        throw error;
    }

    vmware_virtual_machine::ptr vm = portal->get_virtual_machine(macho::stringutils::convert_utf8_to_unicode(machine_id));
    portal = NULL;
    if (vm)
    {
        _return.__set_uuid(macho::stringutils::convert_unicode_to_utf8(vm->uuid));
        _return.__set_name(macho::stringutils::convert_unicode_to_utf8(vm->name));
        _return.__set_host_ip(macho::stringutils::convert_unicode_to_utf8(vm->host_ip));
        _return.__set_host_key(macho::stringutils::convert_unicode_to_utf8(vm->host_key));
        _return.__set_host_name(macho::stringutils::convert_unicode_to_utf8(vm->host_name));
        _return.__set_annotation(macho::stringutils::convert_unicode_to_utf8(vm->annotation));
        _return.__set_datacenter_name(macho::stringutils::convert_unicode_to_utf8(vm->datacenter_name));
        _return.__set_config_path(macho::stringutils::convert_unicode_to_utf8(vm->config_path));
        _return.__set_config_path_file(macho::stringutils::convert_unicode_to_utf8(vm->config_path_file));
        _return.__set_memory_mb(vm->memory_mb);
        _return.__set_number_of_cpu(vm->number_of_cpu);
        _return.__set_cluster_name(macho::stringutils::convert_unicode_to_utf8(vm->cluster_name));
        _return.__set_cluster_key(macho::stringutils::convert_unicode_to_utf8(vm->cluster_key));
        _return.__set_guest_os_name(macho::stringutils::convert_unicode_to_utf8(vm->guest_full_name));
        _return.__set_guest_id(macho::stringutils::convert_unicode_to_utf8(vm->guest_id));
        _return.__set_guest_os_type((saasame::transport::hv_guest_os_type::type)vm->guest_os_type);
        _return.__set_guest_host_name(macho::stringutils::convert_unicode_to_utf8(vm->guest_host_name));
        _return.__set_guest_ip(macho::stringutils::convert_unicode_to_utf8(vm->guest_primary_ip));
        _return.__set_is_cpu_hot_add(vm->is_cpu_hot_add);
        _return.__set_is_cpu_hot_remove(vm->is_cpu_hot_remove);
        _return.__set_is_disk_uuid_enabled(vm->is_disk_uuid_enabled);
        _return.__set_is_template(vm->is_template);
        _return.__set_folder_path(macho::stringutils::convert_unicode_to_utf8(vm->folder_path));
        _return.__set_tools_status((saasame::transport::hv_vm_tools_status::type)vm->tools_status);
        _return.__set_power_state((saasame::transport::hv_vm_power_state::type)vm->power_state);
        _return.__set_connection_state((saasame::transport::hv_vm_connection_state::type)vm->connection_state);
        _return.__set_firmware((saasame::transport::hv_vm_firmware::type)vm->firmware);

        for (key_map::iterator network = vm->networks.begin(); network != vm->networks.end(); network++)
        {
            _return.networks[macho::stringutils::convert_unicode_to_utf8(network->first)] = macho::stringutils::convert_unicode_to_utf8(network->second);
        }
        _return.__set_networks(_return.networks);

        foreach(vmware_disk_info::ptr& disk, vm->disks)
        {
            saasame::transport::virtual_disk_info _disk;
            _disk.key = macho::stringutils::convert_unicode_to_utf8(disk->wsz_key());
            _disk.name = macho::stringutils::convert_unicode_to_utf8(disk->name);
            _disk.id = macho::stringutils::convert_unicode_to_utf8(disk->uuid);
            _disk.size_kb = disk->size / 1024L;
            _disk.size = disk->size;
            _disk.controller_type = (saasame::transport::hv_controller_type::type)(disk->controller ? disk->controller->type : (saasame::transport::hv_controller_type::type::HV_CTRL_ANY));
            _return.disks.push_back(_disk);
        }
        _return.__set_disks(_return.disks);

        std::map<std::string, std::vector<std::string>> net_cfg_map;
        foreach(guest_nic_info nic, vm->guest_net){
            std::string mac = macho::stringutils::convert_unicode_to_utf8(nic.mac_address);
            foreach(std::wstring ip, nic.ip_config)
                net_cfg_map[mac].push_back(macho::stringutils::convert_unicode_to_utf8(ip));
        }

        foreach(vmware_virtual_network_adapter::ptr net_adapter, vm->network_adapters)
        {
            saasame::transport::virtual_network_adapter _adapter;

            _adapter.address_type = macho::stringutils::convert_unicode_to_utf8(net_adapter->address_type);
            _adapter.is_allow_guest_control = net_adapter->is_allow_guest_control;
            _adapter.is_connected = net_adapter->is_connected;
            _adapter.is_start_connected = net_adapter->is_start_connected;
            _adapter.key = net_adapter->key;
            _adapter.mac_address = macho::stringutils::convert_unicode_to_utf8(net_adapter->mac_address);
            _adapter.name = macho::stringutils::convert_unicode_to_utf8(net_adapter->name);
            _adapter.network = macho::stringutils::convert_unicode_to_utf8(net_adapter->network);
            _adapter.port_group = macho::stringutils::convert_unicode_to_utf8(net_adapter->port_group);
            _adapter.type = macho::stringutils::convert_unicode_to_utf8(net_adapter->type);
            if (net_cfg_map.count(_adapter.mac_address))
                _adapter.__set_ip_addresses(net_cfg_map[_adapter.mac_address]);
            _return.network_adapters.push_back(_adapter);
        }
        _return.__set_network_adapters(_return.network_adapters);

        if (vm->root_snapshot_list.size() > 0)
        {
            foreach(vmware_virtual_machine_snapshots::ptr snapshot_list, vm->root_snapshot_list)
            {
                _return.root_snapshot_list.push_back(get_virtual_machine_snapshot_from_object(snapshot_list));
            }

            _return.__set_root_snapshot_list(_return.root_snapshot_list);
        }
    }
    else
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_VIRTUAL_VM_NOTFOUND;
        error.why = boost::str(boost::format("The specified virtual machine '%1' doesn't exist.") % machine_id);
        error.format = "The specified virtual machine '%1' doesn't exist.";
        error.arguments.push_back(machine_id);
        LOG(LOG_LEVEL_ERROR, L"The specified virtual machine '%s' doesn't exist.", macho::stringutils::convert_utf8_to_unicode(machine_id).c_str());
        throw error;
    }
}

void virtual_packer_service_handler::get_virtual_hosts(std::vector<virtual_host> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password){
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;

    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }

    vmware_host::vtr hosts = portal->get_hosts();
    portal = NULL;
    foreach(vmware_host::ptr h, hosts)
    {
        virtual_host ret;
        if (h->name.size() > 0)
            ret.__set_name(macho::stringutils::convert_unicode_to_utf8(*h->name.begin()));
        if (h->domain_name.size() > 0)
            ret.__set_domain_name(macho::stringutils::convert_unicode_to_utf8(*h->domain_name.begin()));
        ret.__set_name_ref(macho::stringutils::convert_unicode_to_utf8(h->name_ref));
        ret.__set_full_name(macho::stringutils::convert_unicode_to_utf8(h->full_name()));
        ret.__set_cluster_key(macho::stringutils::convert_unicode_to_utf8(h->cluster_key));
        ret.__set_datacenter_name(macho::stringutils::convert_unicode_to_utf8(h->datacenter_name));
        ret.__set_product_name(macho::stringutils::convert_unicode_to_utf8(h->product_name));
        ret.__set_version(macho::stringutils::convert_unicode_to_utf8(h->version));
        ret.__set_in_maintenance_mode(h->in_maintenance_mode);
        ret.__set_power_state((saasame::transport::hv_host_power_state::type)h->power_state);
        ret.__set_state(macho::stringutils::convert_unicode_to_utf8(h->state));
        ret.__set_ip_address(macho::stringutils::convert_unicode_to_utf8(h->ip_address));
        ret.__set_uuid(macho::stringutils::convert_unicode_to_utf8(h->uuid));
        ret.__set_number_of_cpu_cores(h->number_of_cpu_cores);
        ret.__set_number_of_cpu_packages(h->number_of_cpu_packages);
        ret.__set_number_of_cpu_threads(h->number_of_cpu_threads);
        ret.__set_size_of_memory(h->size_of_memory);

        foreach(std::wstring ip, h->ip_addresses)
        {
            ret.ip_addresses.push_back(macho::stringutils::convert_unicode_to_utf8(ip));
        }
        ret.__set_ip_addresses(ret.ip_addresses);

        for (key_map::iterator ds = h->datastores.begin(); ds != h->datastores.end(); ds++)
        {
            ret.datastores[macho::stringutils::convert_unicode_to_utf8(ds->first)] = macho::stringutils::convert_unicode_to_utf8(ds->second);
        }
        ret.__set_datastores(ret.datastores);

        for (key_map::iterator net = h->networks.begin(); net != h->networks.end(); net++)
        {
            ret.networks[macho::stringutils::convert_unicode_to_utf8(net->first)] = macho::stringutils::convert_unicode_to_utf8(net->second);
        }
        ret.__set_networks(ret.networks);

        foreach(std::wstring n, h->name)
        {
            ret.name_list.push_back(macho::stringutils::convert_unicode_to_utf8(n));
        }
        ret.__set_name_list(ret.name_list);

        foreach(std::wstring dn, h->domain_name)
        {
            ret.domain_name_list.push_back(macho::stringutils::convert_unicode_to_utf8(dn));
        }
        ret.__set_domain_name_list(ret.domain_name_list);

        for (license_map::iterator p = h->features.begin(); p != h->features.end(); p++)
        {
            std::vector<std::string> props;

            ret.lic_features[macho::stringutils::convert_unicode_to_utf8(p->first)] = props;
            foreach(std::wstring prop, p->second)
                ret.lic_features[macho::stringutils::convert_unicode_to_utf8(p->first)].push_back(macho::stringutils::convert_unicode_to_utf8(prop));
        }
        ret.__set_lic_features(ret.lic_features);
        ret.__set_connection_type((saasame::transport::hv_connection_type::type)h->connection_type);
        ret.__set_virtual_center_name(macho::stringutils::convert_unicode_to_utf8(h->virtual_center.product_name));
        ret.__set_virtual_center_version(macho::stringutils::convert_unicode_to_utf8(h->virtual_center.version));
        for (key_map::iterator vm = h->vms.begin(); vm != h->vms.end(); vm++)
        {
            ret.vms[macho::stringutils::convert_unicode_to_utf8(vm->first)] = macho::stringutils::convert_unicode_to_utf8(vm->second);
        }
        ret.__set_vms(ret.vms);
        _return.push_back(ret);
    }
}

bool virtual_packer_service_handler::power_off_virtual_machine(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    VALIDATE;
    FUN_TRACE;

    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }

    return portal->power_off_virtual_machine(macho::stringutils::convert_utf8_to_unicode(machine_id));
}

bool virtual_packer_service_handler::remove_virtual_machine(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    VALIDATE;
    FUN_TRACE;

    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }
    return portal->delete_virtual_machine(macho::stringutils::convert_utf8_to_unicode(machine_id));
}

void virtual_packer_service_handler::get_virtual_machine_snapshots(std::vector<vmware_snapshot> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    VALIDATE;
    FUN_TRACE;
    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }
    std::vector<std::string> removing_snapshots;
    {
        macho::windows::auto_lock lock(_mgmt.cs);
        registry reg;
        std::string p = boost::str(boost::format("%s\\snapshots management\\%s\\%s") % saasame::transport::g_saasame_constants.CONFIG_PATH % host % machine_id);
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(p))){
            for (size_t index = 0; index < reg[L"snapshots"].get_multi_count(); index++){
                removing_snapshots.push_back(macho::stringutils::convert_unicode_to_utf8(reg[L"snapshots"].get_multi_at(index)));
            }
        }
    }
    vmware_virtual_machine::ptr vm = portal->get_virtual_machine(macho::stringutils::convert_utf8_to_unicode(machine_id));
    portal = NULL;
    if (vm){
        typedef std::vector<vmware_snapshot> vmware_snapshot_vtr;
        if (vm->root_snapshot_list.size() > 0){
            foreach(vmware_virtual_machine_snapshots::ptr snapshot_list, vm->root_snapshot_list){
                std::vector<vmware_snapshot> r = _get_virtual_machine_snapshots(snapshot_list);
                _return.insert(_return.end(), r.begin(), r.end());
            }
            foreach(std::string r, removing_snapshots){
                vmware_snapshot_vtr::iterator s = _return.begin();
                while (s != _return.end()){
                    if (s->name == r)
                        s = _return.erase(s);
                    else
                        s++;
                }
            }
        }
    }
}

bool virtual_packer_service_handler::remove_virtual_machine_snapshot(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id, const std::string& snapshot_id) {
    VALIDATE;
    FUN_TRACE;
    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }
    vmware_virtual_machine::ptr vm = portal->get_virtual_machine(macho::stringutils::convert_utf8_to_unicode(machine_id));
    if (vm){
#if 0
        mwdc::ironman::hypervisor_ex::vmware_portal_ex  portal_ex;
        portal_ex.set_portal(portal, macho::stringutils::convert_utf8_to_unicode(machine_id));
        return (portal_ex.remove_temp_snapshot(vm, macho::stringutils::convert_utf8_to_unicode(snapshot_id)) == S_OK);
#else
        macho::windows::auto_lock lock(_mgmt.cs);
        registry reg(REGISTRY_FLAGS_ENUM::REGISTRY_CREATE);
        std::string p = boost::str(boost::format("%s\\snapshots management\\%s\\%s") % saasame::transport::g_saasame_constants.CONFIG_PATH % host % machine_id);
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(p))){
            reg[L"snapshots"].add_multi(macho::stringutils::convert_ansi_to_unicode(snapshot_id));
            if (!(reg[L"u1"].exists() && reg[L"u2"].exists())){
                macho::bytes _user, _password;
                _user.set((LPBYTE)username.c_str(), username.length());
                _password.set((LPBYTE)password.c_str(), password.length());
                macho::bytes u1 = macho::windows::protected_data::protect(_user, true);
                macho::bytes u2 = macho::windows::protected_data::protect(_password, true);
                reg[L"u1"] = u1;
                reg[L"u2"] = u2;
            }
        }
        _mgmt.trigger();
        return true;
#endif
    }
    return false;
}

void virtual_packer_service_handler::get_datacenter_folder_list(std::vector<std::string> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& datacenter){
    VALIDATE;
    FUN_TRACE;
    vmware_portal_::ptr portal = NULL;
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(username), macho::stringutils::convert_utf8_to_unicode(password));

    if (!portal)
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = boost::str(boost::format("Cannot connect to the host agent '%1%'.") % host);
        throw error;
    }
    vmware_datacenter::ptr _datacenter = portal->get_datacenter(macho::stringutils::convert_utf8_to_unicode(datacenter));
    if (_datacenter){
        get_folder_list(_datacenter->folders, _return);
    }
}

void virtual_packer_service_handler::create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job){
    VALIDATE;
    FUN_TRACE;
    macho::windows::auto_lock lock(_lock);
    virtual_packer_job::ptr new_job;

    try{
        new_job = virtual_packer_job::create(job_id, create_job);
    }
    catch (macho::guid_::exception &e){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
        error.why = "Create job failure (Invalid job id).";
        throw error;
    }

    if (!new_job){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
        error.why = "Create job failure";
        throw error;
    }
    else{
        if (!_scheduled.count(new_job->id())){
            new_job->register_job_to_be_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
            new_job->register_job_was_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
            _scheduler.schedule_job(new_job);
            _scheduled[new_job->id()] = new_job;
            new_job->save_config();
            _return = new_job->get_job_detail();
        }
        else{
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_ID_DUPLICATED;
            error.why = boost::str(boost::format("Job '%1%' is duplicated") % macho::stringutils::convert_unicode_to_ansi(new_job->id()));
            throw error;
        }
    }
}

void virtual_packer_service_handler::create_job(packer_job_detail& _return, const std::string& session_id, const create_packer_job_detail& create_job) {
    VALIDATE;
    FUN_TRACE;
    macho::windows::auto_lock lock(_lock);
    virtual_packer_job::ptr new_job = virtual_packer_job::create(create_job);
    if (!new_job){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
        error.why = "Create job failure";
        throw error;
    }
    else{
        if (!_scheduled.count(new_job->id())){
            new_job->register_job_to_be_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
            new_job->register_job_was_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
            _scheduler.schedule_job(new_job);
            _scheduled[new_job->id()] = new_job;
            new_job->save_config();
            _return = new_job->get_job_detail();
        }
        else{
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_ID_DUPLICATED;
            error.why = boost::str(boost::format("Job '%1%' is duplicated") % macho::stringutils::convert_unicode_to_ansi(new_job->id()));
            throw error;
        }
    }
}

void virtual_packer_service_handler::get_job(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& previous_updated_time) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _return = _scheduled[id]->get_job_detail(boost::posix_time::time_from_string(previous_updated_time));
}

bool virtual_packer_service_handler::interrupt_job(const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _scheduler.interrupt_job(id);
    _scheduled[id]->cancel();
    return true;
}

bool virtual_packer_service_handler::resume_job(const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    if (!_scheduler.is_scheduled(id)){
        _scheduler.schedule_job(_scheduled[id]);
    }
    return true;
}

bool virtual_packer_service_handler::remove_job(const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    interrupt_job(session_id, job_id);
    std::wstring id = macho::guid_(job_id);
    if (_scheduled.count(id)){
        _scheduler.interrupt_job(id);
        _scheduled[id]->cancel();
        _scheduled[id]->remove();
        _scheduler.remove_job(id);
        _scheduled.erase(id);
    }
    return true;
}

void virtual_packer_service_handler::list_jobs(std::vector<packer_job_detail> & _return, const std::string& session_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    foreach(virtual_packer_job::map::value_type& j, _scheduled){
        _return.push_back(j.second->get_job_detail());
    }
}

void virtual_packer_service_handler::terminate(const std::string& session_id) {
    // Your implementation goes here
    printf("exit\n");

    suspend_jobs();
    wait_jobs_completion();
}

bool virtual_packer_service_handler::running_job(const std::string& session_id, const std::string& job_id)
{
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id))
    {
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    else
    {
        return _scheduler.is_running(id);
    }
}

void virtual_packer_service_handler::load_jobs(){
    macho::windows::auto_lock lock(_lock);
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    namespace fs = boost::filesystem;
    fs::path job_dir = working_dir / "jobs";
    fs::directory_iterator end_iter;
    typedef std::multimap<std::time_t, fs::path> result_set_t;
    result_set_t result_set;
    if (fs::exists(job_dir) && fs::is_directory(job_dir)){
        for (fs::directory_iterator dir_iter(job_dir); dir_iter != end_iter; ++dir_iter){
            if (fs::is_regular_file(dir_iter->status())){
                if (dir_iter->path().extension() == JOB_EXTENSION){
                    virtual_packer_job::ptr job = virtual_packer_job::load(dir_iter->path(), dir_iter->path().wstring() + L".status");
                    if (!_scheduled.count(job->id())){
                        job->register_job_to_be_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                        job->register_job_was_executed_callback_function(boost::bind(&virtual_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
                        _scheduler.schedule_job(job);
                        _scheduled[job->id()] = job;
                    }
                }
            }
        }
    }
}

class removing_snapshots_job : public job{
public:
    removing_snapshots_job(std::wstring _vm, std::wstring _host, snapshots_management& _mgmt) : vm(_vm), host(_host), job(_vm + _host, L""), mgmt(_mgmt){
    }

    virtual void execute(){
        std::wstring snapshot_id;
        registry reg;
        std::wstring p = boost::str(boost::wformat(L"%s\\snapshots management\\%s\\%s") % macho::stringutils::convert_utf8_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH) % host % vm);
        {
            macho::windows::auto_lock lock(mgmt.cs);
            if (reg.open(p)){
                macho::bytes u1 = reg[_T("u1")];
                macho::bytes u2 = reg[_T("u2")];
                if (u1.length() && u2.length()){
                    macho::bytes user = macho::windows::protected_data::unprotect(u1, true);
                    if (user.length()){
                        username = macho::stringutils::convert_utf8_to_unicode(std::string(reinterpret_cast<char const*>(user.ptr()), user.length()));
                    }
                    macho::bytes pwd = macho::windows::protected_data::unprotect(u2, true);
                    if (pwd.length()){
                        password = macho::stringutils::convert_utf8_to_unicode(std::string(reinterpret_cast<char const*>(pwd.ptr()), pwd.length()));
                    }
                }
                snapshot_id = reg[_T("snapshots")].get_multi_count() ? reg[_T("snapshots")].get_multi_at(0) : L"";
                reg.close();
            }
        }		
        vmware_portal_::ptr portal = NULL;
        std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % host);
        portal = vmware_portal_::connect(uri, username, password);
        vmware_virtual_machine::ptr _vm = portal->get_virtual_machine(vm);
        if (_vm){
            mwdc::ironman::hypervisor_ex::vmware_portal_ex  portal_ex;
            portal_ex.set_portal(portal, vm);
            while (!snapshot_id.empty()){
                if (portal_ex.remove_temp_snapshot(_vm, snapshot_id) == S_OK){
                    macho::windows::auto_lock lock(mgmt.cs);
                    bool completed = false;
                    if (reg.open(p)){
                        reg[_T("snapshots")].remove_multi(snapshot_id);
                        if (reg[_T("snapshots")].get_multi_count()){
                            snapshot_id = reg[_T("snapshots")].get_multi_at(0);
                        }
                        else{
                            snapshot_id.clear();
                            reg.delete_key();
                            break;
                        }
                    }
                }
                else{
                    break;
                }
            }
        }
    }
private:
    snapshots_management&			   mgmt;
    std::wstring					   vm; 
    std::wstring                       host;
    protected_data					   username;
    protected_data					   password;
};

class mgmt_job : public job{
public:
    mgmt_job(snapshots_management& _mgmt) : job(L"mgmt_job", L"base"), mgmt(_mgmt){
    }
    virtual void execute(){
        while (!mgmt.terminated){
            {
                macho::windows::auto_lock lock(mgmt.cs);
                registry reg;
                std::string p = boost::str(boost::format("%s\\snapshots management") % saasame::transport::g_saasame_constants.CONFIG_PATH);
                if (reg.open(macho::stringutils::convert_ansi_to_unicode(p))){
                    reg.refresh_subkeys();
                    for (int i = 0; i < reg.subkeys_count(); i++){
                        registry& host = reg.subkey(i);
                        host.refresh_subkeys();
                        if (host.subkeys_count()){
                            for (int j = 0; j < host.subkeys_count(); j++){
                                registry& vm = host.subkey(j);
                                if (!mgmt.tasks.is_scheduled(vm.key_name() + host.key_name())){
                                    mgmt.tasks.schedule_job(job::ptr(new removing_snapshots_job(vm.key_name(), host.key_name(), mgmt)));
                                }
                            }
                        }
                        else{
                            host.delete_key();
                        }
                    }
                }
            }
            {
                boost::unique_lock<boost::mutex> _lock(mgmt.lock);
                mgmt.cond.timed_wait(_lock, boost::posix_time::seconds(mgmt.timeout));
            }
        }
    }
private:
    snapshots_management&			   mgmt;
};

void snapshots_management::trigger(){
    macho::windows::auto_lock lock(cs);
    if (initialized){
        cond.notify_all();
    }
    else{
        initialized = true;
        tasks.schedule_job(job::ptr(new mgmt_job(*this)));
        tasks.start();
    }
}