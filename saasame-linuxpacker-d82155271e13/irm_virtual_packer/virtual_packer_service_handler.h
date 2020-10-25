
#include "virtual_packer_service.h"
#include "common_service_handler.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "virtual_packer_job.h"
#include "vmware_ex.h"
#include "..\buildenv.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace mwdc::ironman::hypervisor_ex;
using boost::shared_ptr;

using namespace  ::saasame::transport;

class virtual_packer_service_handler : virtual public common_service_handler, virtual public virtual_packer_serviceIf {
public:
    virtual_packer_service_handler(std::wstring session_id) {
        _session = macho::guid_(session_id);
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"ConcurrentVirtualPackerJobNumber"].exists() && (DWORD)reg[L"ConcurrentVirtualPackerJobNumber"] > 0){
                _scheduler.initial((DWORD)reg[L"ConcurrentVirtualPackerJobNumber"]);
            }
        }
        _scheduler.start();
        load_jobs();
    }
    virtual ~virtual_packer_service_handler(){ suspend_jobs(); }

    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }

    void get_virtual_host_info(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password)
    {
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

            for (key_map::iterator vm = h->vms.begin(); vm != h->vms.end(); vm++)
            {
                _return.vms[macho::stringutils::convert_unicode_to_utf8(vm->first)] = macho::stringutils::convert_unicode_to_utf8(vm->second);
            }
            _return.__set_vms(_return.vms);

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
    }

    void get_virtual_machine_detail(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id)
    {
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

            foreach(vmware_disk_info::map::value_type& disk, vm->disks)
            {
                saasame::transport::virtual_disk_info _disk;
                _disk.key = macho::stringutils::convert_unicode_to_utf8(disk.second->key);
                _disk.name = macho::stringutils::convert_unicode_to_utf8(disk.second->name);
                _disk.id = macho::stringutils::convert_unicode_to_utf8(disk.second->uuid);
                _disk.size_kb = disk.second->size / 1024L;
                _disk.size = disk.second->size;
                _return.disks.push_back(_disk);
            }
            std::sort(_return.disks.begin(), _return.disks.end(), [](saasame::transport::virtual_disk_info const& lhs, saasame::transport::virtual_disk_info const& rhs){ return std::stoi(lhs.key) < std::stoi(rhs.key); });
            _return.__set_disks(_return.disks);

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
            throw error;
        }
    }
    void create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job){
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        virtual_packer_job::ptr new_job;

        try{
            new_job = virtual_packer_job::create(job_id, create_job);
        }
        catch ( macho::guid_::exception &e){
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

    void create_job(packer_job_detail& _return, const std::string& session_id, const create_packer_job_detail& create_job) {
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

    void get_job(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& previous_updated_time) {
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

    bool interrupt_job(const std::string& session_id, const std::string& job_id) {
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

    bool resume_job(const std::string& session_id, const std::string& job_id) {
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

    bool remove_job(const std::string& session_id, const std::string& job_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
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

    void list_jobs(std::vector<packer_job_detail> & _return, const std::string& session_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
        foreach(virtual_packer_job::map::value_type& j, _scheduled){
            _return.push_back(j.second->get_job_detail());
        }
    }

    void terminate(const std::string& session_id) {
        // Your implementation goes here
        printf("exit\n");

        suspend_jobs();
        wait_jobs_completion();
    }

    bool running_job(const std::string& session_id, const std::string& job_id)
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
private:
    virtual_machine_snapshots get_virtual_machine_snapshot_from_object(vmware_virtual_machine_snapshots::ptr child)
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

    void load_jobs(){
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

    void suspend_jobs(){
        macho::windows::auto_lock lock(_lock);
        foreach(virtual_packer_job::map::value_type& j, _scheduled){
            j.second->interrupt();
        }
    }

    void job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec){
        macho::windows::auto_lock lock(_lock);

    }

    void job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex){
        macho::windows::auto_lock lock(_lock);

        std::vector<std::wstring>::iterator it;
    }

    void wait_jobs_completion()
    {
        foreach(virtual_packer_job::map::value_type& j, _scheduled)
        {
            while (_scheduler.is_running(j.second->id()))
                boost::this_thread::sleep(boost::posix_time::seconds(2));
        }
    }

    macho::guid_                                _session;
    macho::scheduler                            _scheduler;
    virtual_packer_job::map                     _scheduled;
    macho::windows::critical_section            _lock;
};