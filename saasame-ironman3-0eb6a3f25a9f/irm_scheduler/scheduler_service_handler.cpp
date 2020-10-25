#include "stdafx.h"
#include "scheduler_service_handler.h"
#include "management_service.h"
#include <codecvt>
#include "common\ntp_client.hpp"
#include "mgmt_op.h"
#include "service_op.h"

#ifndef IRM_TRANSPORTER

virtual_machine_snapshots scheduler_service_handler::get_virtual_machine_snapshot_from_object(vmware_virtual_machine_snapshots::ptr child)
{
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

#endif

void scheduler_service_handler::get_virtual_host_info(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password) 
{
    // Your implementation goes here
    FUN_TRACE;
#ifndef IRM_TRANSPORTER
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

#endif
}

void scheduler_service_handler::get_virtual_machine_detail(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id) 
{
    // Your implementation goes here
    FUN_TRACE;
#ifndef IRM_TRANSPORTER
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
            foreach( std::wstring ip, nic.ip_config)
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
#endif
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

void scheduler_service_handler::load_jobs(){
    macho::windows::auto_lock lock(_lock);
    load_removing_jobs();
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
                    transport_job::ptr job = transport_job::load(dir_iter->path(), dir_iter->path().wstring() + L".status");
                    if (job){
                        if (_removing.count(job->id()))
                            _removing[job->id()] = job;
                        else if (!_scheduled.count(job->id())){
                            job->register_job_to_be_executed_callback_function(boost::bind(&scheduler_service_handler::job_to_be_executed_callback, this, _1, _2));
                            job->register_job_was_executed_callback_function(boost::bind(&scheduler_service_handler::job_was_executed_callback, this, _1, _2, _3));
                            trigger::vtr triggers = job->get_triggers();
                            _scheduler.schedule_job(job, triggers);
                            _scheduler.update_job_triggers(job->id(), triggers, job->get_latest_leave_time());
                            _scheduled[job->id()] = job;
                        }
                    }
                }
            }
        }
    }
}

void scheduler_service_handler::suspend_jobs(){
    macho::windows::auto_lock lock(_lock);
    foreach(transport_job::map::value_type& j, _scheduled){
        j.second->interrupt();
    }
}

bool scheduler_service_handler::verify_management(const std::string& management, const int32_t port, const bool is_ssl){
    bool result = false;
    FUN_TRACE;
    mgmt_op op({ management }, std::string(), port, is_ssl);
    result = op.open();
    return result;
}

void scheduler_service_handler::job_to_be_executed_callback2(const macho::trigger::ptr& job, const macho::job_execution_context& ec)
{
    transport_job::map  scheduled;
    {
        macho::windows::auto_lock lock(_lock);
        scheduled = _scheduled;
    }
    foreach(transport_job::map::value_type& j, scheduled){
        if (!_scheduler.is_running(j.first)){
            physical_transport_job* _job = dynamic_cast<physical_transport_job*>(j.second.get());
            if (_job != NULL){
                if (_job->need_to_be_resumed()){
                    trigger::vtr triggers = j.second->get_triggers();
                    triggers.push_back(trigger::ptr(new run_once_trigger()));
                    _scheduler.update_job_triggers(j.first, triggers);
                }
                continue;
            }
#ifndef IRM_TRANSPORTER
            virtual_transport_job* _vjob = dynamic_cast<virtual_transport_job*>(j.second.get());
            if (_vjob != NULL){
                if (_vjob->need_to_be_resumed()){
                    trigger::vtr triggers = j.second->get_triggers();
                    triggers.push_back(trigger::ptr(new run_once_trigger()));
                    _scheduler.update_job_triggers(j.first, triggers);
                }
                continue;
            }
#endif
        }
    }
}

void scheduler_service_handler::removing_job_to_be_executed_callback(const macho::trigger::ptr& job, const macho::job_execution_context& ec){
    transport_job::map  removing;
    {
        macho::windows::auto_lock lock(_lock);
        removing = _removing;
    }
    foreach(transport_job::map::value_type &v, removing){
        bool can_be_erase = false;
        if (v.second){
            _scheduler.interrupt_job(v.first);
            v.second->cancel();
            if (can_be_erase = v.second->prepare_for_job_removing()){
                v.second->remove();
                _scheduler.remove_job(v.first);
            }
        }
        else{
            can_be_erase = true;
        }

        if (can_be_erase){
            macho::windows::auto_lock lock(_lock);
            _removing.erase(v.first);
            save_removing_jobs();
        }
    }
}

void scheduler_service_handler::save_removing_jobs(){
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path removing_config = working_dir / "jobs" / "removing";
    try
    {
        macho::windows::auto_lock lock(_lock);
        using namespace json_spirit;
        mObject config;
        mArray removing_jobs;
        foreach(transport_job::map::value_type &v, _removing){
            removing_jobs.push_back((std::string)macho::guid_(v.first));
        }
        config["removing_jobs"] = removing_jobs;
        boost::filesystem::path temp = removing_config.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), removing_config.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), removing_config.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output config info.")).c_str());
    }
    catch (...){
    }
}

void scheduler_service_handler::load_removing_jobs(){
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path removing_config = working_dir / "jobs" / "removing";
    if (boost::filesystem::exists(removing_config)){
        try{
            macho::windows::auto_lock lock(_lock);
            std::ifstream is(removing_config.string());
            mValue config;
            read(is, config);
            mArray removing_jobs = find_value(config.get_obj(), "removing_jobs").get_array();
            foreach(mValue j, removing_jobs){
                _removing[macho::guid_(j.get_str())] = nullptr;
            }
        }
        catch (boost::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")));
        }
        catch (...){
        }
    }
}

bool scheduler_service_handler::verify_packer_to_carrier(const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl){
    bool result = false;
    thrift_connect<common_serviceClient> thrift(port, packer);
    if (thrift.open()){
        try{
            result = thrift.client()->verify_carrier(carrier, is_ssl);
        }
        catch (TException& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    return result;
}

void scheduler_service_handler::get_physical_machine_detail(physical_machine_info& _return, const std::string& session_id, const std::string& host, const machine_detail_filter::type filter) {
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;
    bool result = false;
    thrift_connect<common_serviceClient> thrift(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
    if (thrift.open()){
        try{
            thrift.client()->get_host_detail(_return, session_id, filter);
            result = true;
        }
        catch (TException& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    if (!result){
        invalid_operation err;
        err.what_op = error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        err.why = boost::str(boost::format("Failed to connect the host packer on machine '%1%'.") % host);
        throw err;
    }
}

void scheduler_service_handler::get_packer_service_info(service_info& _return, const std::string& session_id, const std::string& host){
    VALIDATE;
    FUN_TRACE;
    bool result = false;
    thrift_connect<common_serviceClient> thrift(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
    if (thrift.open()){
        try{
            thrift.client()->ping(_return);
            result = true;
        }
        catch (TException& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    if (!result){
        invalid_operation err;
        err.what_op = error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        err.why = boost::str(boost::format("Failed to connect the host packer on machine '%1%'.") % host);
        throw err;
    }
}

void scheduler_service_handler::time_sync_job::execute(){
    boost::posix_time::time_duration diff = boost::posix_time::minutes(60);
    if (_latest_updated_time != boost::date_time::not_a_date_time){
        diff = boost::posix_time::microsec_clock::universal_time() - _latest_updated_time;
    }
    macho::windows::registry reg;
    bool disable_time_sync = false;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"DisableTimeSync"].exists() && reg[L"DisableTimeSync"].is_dword())
            disable_time_sync = (DWORD)reg[L"DisableTimeSync"] > 0;
    }
    if (disable_time_sync)
        return;
    if (diff.total_seconds() >= 3600 || diff.is_negative()){
        std::vector<std::string> time_servers;
        time_servers.push_back("0.cn.pool.ntp.org");
        time_servers.push_back("time.google.com");
        time_servers.push_back("time1.google.com");
        time_servers.push_back("time2.google.com");
        time_servers.push_back("time3.google.com");
        time_servers.push_back("time4.google.com");
        time_servers.push_back("time.windows.com");
        time_servers.push_back("time.nist.gov");
        time_servers.push_back("time-nw.nist.gov");
        time_servers.push_back("time-a.nist.gov");
        time_servers.push_back("time-b.nist.gov");
        time_servers.push_back("pool.ntp.org");
        time_servers.push_back("1.cn.pool.ntp.org");
        time_servers.push_back("2.cn.pool.ntp.org");
        time_servers.push_back("3.cn.pool.ntp.org");

        foreach(std::string var, time_servers){
            boost::posix_time::ptime pt;
            if (macho::ntp_client::get_time(var, pt)){
                SYSTEMTIME st;
                boost::gregorian::date::ymd_type ymd = pt.date().year_month_day();
                st.wYear = ymd.year;
                st.wMonth = ymd.month;
                st.wDay = ymd.day;
                st.wDayOfWeek = pt.date().day_of_week();

                // Now extract the hour/min/second field from time_duration
                boost::posix_time::time_duration td = pt.time_of_day();
                st.wHour = static_cast<WORD>(td.hours());
                st.wMinute = static_cast<WORD>(td.minutes());
                st.wSecond = static_cast<WORD>(td.seconds());

                // Although ptime has a fractional second field, SYSTEMTIME millisecond
                // field is 16 bit, and will not store microsecond. We will treat this
                // field separately later.
                st.wMilliseconds = 0;
                if (SetSystemTime(&st)){
                    _latest_updated_time = boost::posix_time::microsec_clock::universal_time();
                    LOG(LOG_LEVEL_RECORD, L"Succeed to set time from time server(%s). UTC time (%s)", macho::stringutils::convert_ansi_to_unicode(var).c_str(), macho::stringutils::convert_ansi_to_unicode(boost::posix_time::to_simple_string(pt)).c_str());
                    break;
                }
            }
            else{
                LOG(LOG_LEVEL_WARNING, L"Failed to get time from time server(%s).", macho::stringutils::convert_ansi_to_unicode(var).c_str());
            }
        }
    }
}