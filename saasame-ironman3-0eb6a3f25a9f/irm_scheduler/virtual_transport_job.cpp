#include "transport_job.h"
#include "common_service_handler.h"
#include "carrier_rw.h"
#include <codecvt>

#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

#define PACKER_SERVICE_TIMEOUT          300 // seconds
#define PACKER_SERVICE_CHECK_INTERVAL   5 // seconds

bool virtual_transport_job::virtual_packer_job_op::initialize()
{
    FUN_TRACE;
    foreach(std::string addr, _addrs)
    {
        try
        {
            if (addr.empty())
                continue;
            macho::windows::registry reg;
            boost::filesystem::path p(macho::windows::environment::get_working_directory());
            std::shared_ptr<TTransport>        transport;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                    p = reg[L"KeyPath"].wstring();
            }
            if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
            {
                try
                {
                    _factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                    _factory->authenticate(false);
                    _factory->loadCertificate((p / "server.crt").string().c_str());
                    _factory->loadPrivateKey((p / "server.key").string().c_str());
                    _factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                    std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
                    _factory->access(accessManager);
                    _ssl_socket = std::shared_ptr<TSSLSocket>(_factory->createSocket(addr, saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT));
                    _ssl_socket->setConnTimeout(30 * 1000);
                    _ssl_socket->setSendTimeout(10 * 60 * 1000);
                    _ssl_socket->setRecvTimeout(10 * 60 * 1000);
                    _transport = std::shared_ptr<TTransport>(new TBufferedTransport(_ssl_socket));
                }
                catch (TException& ex) {
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
            }
            else{
                _socket = std::shared_ptr<TSocket>(new TSocket(addr, saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT));
                _socket->setConnTimeout(30 * 1000);
                _socket->setSendTimeout(10 * 60 * 1000);
                _socket->setRecvTimeout(10 * 60 * 1000);
                _transport = std::shared_ptr<TTransport>(new TBufferedTransport(_socket));
            }
            if (_transport){
                _protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));
                _client = std::shared_ptr<virtual_packer_serviceClient>(new virtual_packer_serviceClient(_protocol));
                _transport->open();
                _transport->close();
                _host = addr;
                break;
            }
        }
        catch (saasame::transport::invalid_operation& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (TException &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            _host.clear();
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        if (_transport && _transport->isOpen())
            _transport->close();
    }

    return (_host.length() > 0);
}

bool virtual_transport_job::virtual_packer_job_op::get_packer_job_detail(const std::string job_id, const boost::posix_time::ptime previous_updated_time, saasame::transport::packer_job_detail &detail)
{
    bool result = false;
    std::string errmsg;
    int32_t error;
    FUN_TRACE;
    try
    {
        _transport->open();
        _client->get_job(detail, _session, job_id, boost::posix_time::to_simple_string(previous_updated_time));
        result = true;
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        errmsg = ex.why;
        error = ex.what_op;
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        errmsg = std::string(ex.what());
        error = error_codes::SAASAME_E_INTERNAL_FAIL;
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();

    return result;
}

bool virtual_transport_job::virtual_packer_job_op::create_packer_job(
    const std::set<std::string> connections,
    const replica_job_create_detail& _replica_job_create_detail,
    const virtual_create_packer_job_detail& detail,
    std::string& job_id, 
    boost::posix_time::ptime& created_time)
{
    bool result = false;
    FUN_TRACE;
    saasame::transport::create_packer_job_detail create_detail;
    saasame::transport::packer_job_detail _return;
    create_detail.__set_type(saasame::transport::job_type::virtual_packer_job_type);
    create_detail.__set_connection_ids(connections);
    create_detail.__set_carriers(_replica_job_create_detail.carriers);
    create_detail.__set_priority_carrier(_replica_job_create_detail.priority_carrier);
    create_detail.detail.v.__set_disks(detail.disks);
    create_detail.detail.v.__set_addr(detail.addr);
    create_detail.detail.v.__set_host(detail.host);
    create_detail.detail.v.__set_images(detail.images);
    create_detail.detail.v.__set_password(detail.password);
    create_detail.detail.v.__set_snapshot(detail.snapshot);
    create_detail.detail.v.__set_username(detail.username);
    create_detail.detail.v.__set_virtual_machine_id(detail.virtual_machine_id);
    create_detail.detail.v.__set_previous_change_ids(detail.previous_change_ids);
    create_detail.detail.v.__set_backup_image_offset(detail.backup_image_offset);
    create_detail.detail.v.__set_backup_progress(detail.backup_progress);
    create_detail.detail.v.__set_backup_size(detail.backup_size);
    create_detail.detail.v.__set_completed_blocks(detail.completed_blocks);
    create_detail.detail.__set_v(create_detail.detail.v);
    create_detail.__set_timeout(_replica_job_create_detail.timeout);
    create_detail.__set_checksum_verify(_replica_job_create_detail.checksum_verify);
    create_detail.__set_is_encrypted(_replica_job_create_detail.is_encrypted);
    create_detail.__set_worker_thread_number(_replica_job_create_detail.worker_thread_number);
    create_detail.__set_file_system_filter_enable(_replica_job_create_detail.file_system_filter_enable);
    create_detail.__set_min_transport_size(_replica_job_create_detail.min_transport_size);
    create_detail.__set_full_min_transport_size(_replica_job_create_detail.full_min_transport_size);
    create_detail.__set_is_checksum(_replica_job_create_detail.is_checksum);
    create_detail.__set_is_compressed(_replica_job_create_detail.is_compressed);
    create_detail.__set_is_only_single_system_disk(_replica_job_create_detail.is_only_single_system_disk);
    //create_detail.__set_priority_carrier(_replica_job_create_detail.priority_carrier);
    create_detail.__set_is_compressed_by_packer(_replica_job_create_detail.is_compressed_by_packer);
    create_detail.__set_checksum_target(_replica_job_create_detail.checksum_target);
    try
    {
        _transport->open();
        _client->create_job_ex(_return, _session, job_id, create_detail);
        if (_return.id.length())
        {
            job_id = _return.id;
            created_time = boost::posix_time::time_from_string(_return.created_time);
            result = true;
        }
        else{
            job_id.erase();
        }
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();
    return result;
}

bool virtual_transport_job::virtual_packer_job_op::remove_packer_job(const std::string job_id)
{
    bool result = false;
    FUN_TRACE;
    try
    {
        _transport->open();
        result = _client->remove_job(_session, job_id);
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();
    return result;
}

bool virtual_transport_job::virtual_packer_job_op::resume_packer_job(const std::string job_id)
{
    bool result = false;
    FUN_TRACE;
    try
    {
        _transport->open();
        result = _client->resume_job(_session, job_id);
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();
    return result;
}

bool virtual_transport_job::virtual_packer_job_op::interrupt_packer_job(const std::string job_id)
{
    bool result = false;
    FUN_TRACE;
    try
    {
        _transport->open();
        result = _client->interrupt_job(_session, job_id);
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();

    return result;
}

bool virtual_transport_job::virtual_packer_job_op::is_packer_job_running(const std::string job_id)
{
    bool result = false;
    FUN_TRACE;
    for (uint32_t i = 0; i < 5; i++)
    {
        try
        {
            if (!_transport->isOpen())
                _transport->open();
            result = _client->running_job(_session, job_id);
            _transport->close();
            break;
        }
        catch (saasame::transport::invalid_operation& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (TException &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }

        if (_transport && _transport->isOpen())
            _transport->close();

        boost::this_thread::sleep(boost::posix_time::seconds(PACKER_SERVICE_CHECK_INTERVAL));
    }

    return result;
}

bool virtual_transport_job::virtual_packer_job_op::is_packer_job_existing(const std::string job_id){
    bool result = true;
    FUN_TRACE;
    try{
        if (!_transport->isOpen())
            _transport->open();
        _client->running_job(_session, job_id);
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex){
        if (ex.what_op == saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND)
            result = false;
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }

    if (_transport && _transport->isOpen())
        _transport->close();

    return result;
}

std::string virtual_transport_job::virtual_packer_job_op::get_machine_id()
{
    std::string machine_id;
    FUN_TRACE;
    try
    {
        _transport->open();
        physical_machine_info machine_info;
        _client->get_host_detail(machine_info, _session, machine_detail_filter::SIMPLE); 
        if (machine_info.machine_id.length()){
            machine_id = machine_info.machine_id;
        }
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();
    return machine_id;
}

bool virtual_transport_job::virtual_packer_job_op::can_connect_to_carriers(std::map<std::string, std::set<std::string> >& carriers, bool is_ssl){
    bool result = false;
    FUN_TRACE;
#ifndef string_set_map
    typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif
    try
    {
        _transport->open();
        foreach(string_set_map::value_type &c, carriers){
            foreach(std::string _c, c.second){
                try{
                    if (result = _client->verify_carrier(_c, is_ssl))
                        break;
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                }
            }
            if (result)
                break;
        }
        _transport->close();
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        if (std::string(ex.what()).find("Invalid method name:") != std::string::npos)
        {
            result = true;
            LOG(LOG_LEVEL_WARNING, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_transport && _transport->isOpen())
        _transport->close();
    return result;
}

void virtual_transport_job::save_status()
{
    FUN_TRACE;
    try
    {
        mObject job_status;
        macho::windows::auto_lock lock(_cs);
        mArray progress;
        foreach(virtual_transport_job_progress::map::value_type &p, _progress){
            mObject o;
            o["device_uuid"] = p.second->device_uuid;
            o["device_name"] = p.second->device_name;
            o["device_key"] = p.second->device_key;
            o["output"] = p.second->output;
            o["base"] = p.second->base;
            o["parent"] = p.second->parent;
            o["discard"] = p.second->discard;
            o["total_size"] = p.second->total_size;
            o["backup_progress"] = p.second->backup_progress;
            o["backup_size"] = p.second->backup_size;
            o["real_backup_size"] = p.second->real_backup_size;
            o["backup_image_offset"] = p.second->backup_image_offset;
            o["error_what"] = p.second->error_what;
            o["error_why"] = p.second->error_why;
            mArray completeds;
            foreach(io_changed_range& r, p.second->completed_blocks){
                mObject completed;
                completed["o"] = r.start;
                completed["l"] = r.length;
                completeds.push_back(completed);
            }
            o["completed_blocks"] = completeds;
            progress.push_back(o);
        }
        job_status["progress"] = progress;
        job_status["hv_type"] = _conn_type;
        job_status["full_replica"] = _full_replica;
        job_status["snapshot_mor_ref"] = _snapshot_mor_ref;

        job_status["self_enable_cbt"] = _self_enable_cbt;
        job_status["cbt_config_enabled"] = _cbt_config_enabled;
        job_status["is_windows"] = _is_windows;
        job_status["has_changed"] = _has_changed;

        mArray previous_change_ids;
        foreach(string_map::value_type &i, _previous_change_ids){
            mObject d;
            d["disk"] = i.first;
            d["changeid"] = i.second;
            previous_change_ids.push_back(d);
        }
        job_status["previous_change_ids"] = previous_change_ids;
       
        transport_job::save_status(job_status);
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output virtual_transport_job status info.")));
    }
    catch (...){
    }
}

bool virtual_transport_job::load_status(std::wstring status_file)
{
    FUN_TRACE;
    mValue job_status;
    if (transport_job::load_status(status_file, job_status))
    {
        mArray progress = find_value(job_status.get_obj(), "progress").get_array();
        foreach(mValue p, progress)
        {
            virtual_transport_job_progress::ptr ptr = virtual_transport_job_progress::ptr(new virtual_transport_job_progress());
            ptr->device_uuid = find_value_string(p.get_obj(), "device_uuid");
            ptr->device_name = find_value_string(p.get_obj(), "device_name");
            ptr->device_key = find_value_string(p.get_obj(), "device_key");
            ptr->output = find_value_string(p.get_obj(), "output");
            ptr->base = find_value_string(p.get_obj(), "base");
            ptr->parent = find_value_string(p.get_obj(), "parent");
            ptr->discard = find_value_string(p.get_obj(), "discard");
            ptr->total_size = find_value(p.get_obj(), "total_size").get_int64();
            ptr->backup_progress = find_value(p.get_obj(), "backup_progress").get_int64();
            ptr->backup_size = find_value(p.get_obj(), "backup_size").get_int64();
            ptr->real_backup_size = find_value(p.get_obj(), "real_backup_size").get_int64();
            ptr->backup_image_offset = find_value(p.get_obj(), "backup_image_offset").get_int64();
            ptr->error_what = find_value(p.get_obj(), "error_what").get_int(); 
            ptr->error_why = find_value_string(p.get_obj(), "error_why");
            mArray  completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
            foreach(mValue c, completed_blocks){
                io_changed_range completed;
                completed.start = find_value(c.get_obj(), "o").get_int64();
                completed.length = find_value(c.get_obj(), "l").get_int64();
                ptr->completed_blocks.push_back(completed);
            }
            _progress[ptr->device_uuid] = ptr;
        }
        _conn_type = (mwdc::ironman::hypervisor::hv_connection_type)find_value(job_status.get_obj(), "hv_type").get_int();
        _full_replica = find_value_bool(job_status.get_obj(), "full_replica");
        _self_enable_cbt = find_value_bool(job_status.get_obj(), "self_enable_cbt");
        _cbt_config_enabled = find_value_bool(job_status.get_obj(), "cbt_config_enabled");
        _is_windows = find_value_bool(job_status.get_obj(), "is_windows");
        _has_changed = find_value_bool(job_status.get_obj(), "has_changed");

        mArray  previous_change_ids = find_value(job_status.get_obj(), "previous_change_ids").get_array();
        foreach(mValue t, previous_change_ids){
            _previous_change_ids[find_value_string(t.get_obj(), "disk")] = find_value_string(t.get_obj(), "changeid");
        }
 
        _snapshot_mor_ref = find_value_string(job_status.get_obj(), "snapshot_mor_ref");
        if (!_snapshot_info.empty())
            _wsz_current_snapshot_name = macho::stringutils::convert_utf8_to_unicode(_snapshot_info);
        return true;
    }
    return false;
}

saasame::transport::replica_job_detail virtual_transport_job::get_job_detail(const boost::posix_time::ptime previous_updated_time, const std::string connection_id)
{
    FUN_TRACE;
    saasame::transport::replica_job_detail job;
    macho::windows::auto_lock lock(_cs);
    job.id = macho::stringutils::convert_unicode_to_utf8(_id);
    job.__set_type(_create_job_detail.type);
    job.created_time = boost::posix_time::to_simple_string(_created_time);
    job.host = _replica_job_create_detail.host;
    job.disks = _replica_job_create_detail.disks;
    job.state = (job_state::type)(_state & ~mgmt_job_state::job_state_error);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    job.updated_time = boost::posix_time::to_simple_string(update_time);
    job.__set_is_pending_rerun(_is_pending_rerun);
    history_record::vtr histories;
    if (_is_initialized){
        std::string job_id;
        virtual_packer_job_op op(_replica_job_create_detail.addr);
        if (_packer_jobs.size() && !op.initialize()){
            if (_packer_jobs.count(connection_id)){
                job_id = _packer_jobs[connection_id];
                if (_packers_update_time.count(job_id)){
                    boost::posix_time::time_duration duration(update_time - _packers_update_time[job_id]);
                    if (duration.total_seconds() >= 30){
                        _jobs_state[job_id].is_error = true;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Connection retry timeout to Packer for VMware."));
                    }
                }
            }
        }
        else{
            packer_job_detail packer_detail;
            if (_packer_jobs.size() && connection_id.length()){
                if (_packer_jobs.count(connection_id)){
                    job_id = _packer_jobs[connection_id];
                    boost::posix_time::ptime query_time;
                    boost::posix_time::time_duration diff = boost::posix_time::seconds(0);
                    if (_packer_previous_update_time.count(job_id)){
                        //diff = _packer_time_diffs[job_id];
                        query_time = boost::posix_time::time_from_string(_packer_previous_update_time[job_id]);
                    }
                    else if (_packer_time_diffs.count(job_id)){
                        diff = _packer_time_diffs[job_id];
                        query_time = previous_updated_time - _packer_time_diffs[job_id];
                    }
                    else{
                        query_time = previous_updated_time;
                    }

                    if (!op.get_packer_job_detail(job_id, query_time, packer_detail)){
                        if (_packers_update_time.count(job_id)){
                            boost::posix_time::time_duration duration(update_time - _packers_update_time[job_id]);
                            if (duration.total_seconds() >= 180){
                                _jobs_state[job_id].disconnected = true;
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Response timeout from Packer for VMware."));
                            }
                        }
                        if (!op.is_packer_job_existing(job_id)){
                            job.is_error = true;
                            LOG(LOG_LEVEL_ERROR, L"Packer Job (%s) was missing.", job_id.c_str());
                        }
                    }
                    else{
                        if (_jobs_state[job_id].disconnected){
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Resume connection to Packer for VMware."));
                        }
                        _jobs_state[job_id].disconnected = false;
                        _packers_update_time[job_id] = update_time;
                        _packer_previous_update_time[job_id] = packer_detail.updated_time;
                        _jobs_state[job_id].state = packer_detail.state;
                        _jobs_state[job_id].is_error = packer_detail.is_error;
                        job.is_error = packer_detail.is_error;
                        _boot_disk = job.boot_disk = packer_detail.boot_disk;
                        _system_disks = job.system_disks = packer_detail.system_disks;
                        
                        if (!packer_detail.detail.v.disk_infos.empty())
                            job.__set_virtual_disk_infos(packer_detail.detail.v.disk_infos);
                        
                        typedef std::map<std::string, int64_t> str_int64_map;

                        foreach(str_int64_map::value_type p, packer_detail.detail.v.original_size){
                            _progress[p.first]->total_size = p.second;
                        }
                        foreach(str_int64_map::value_type p, packer_detail.detail.v.backup_progress){
                            _progress[p.first]->backup_progress = p.second;
                        }
                        _need_update = false;
                        foreach(str_int64_map::value_type p, packer_detail.detail.v.backup_image_offset){
                            if (_progress[p.first]->backup_image_offset != p.second)
                                _has_changed = _need_update = true;
                            _progress[p.first]->backup_image_offset = p.second;
                        }
                        foreach(str_int64_map::value_type p, packer_detail.detail.v.backup_size){
                            _progress[p.first]->backup_size = p.second;
                        }

                        typedef std::map<std::string, std::vector<io_changed_range>> io_changed_range_map;
                        foreach(io_changed_range_map::value_type c, packer_detail.detail.v.completed_blocks){
                            _progress[c.first]->completed_blocks = c.second;
                        }
                        
                        foreach(job_history h, packer_detail.histories){
                            history_record::ptr _h = history_record::ptr(new history_record(h.time, h.state, h.error, h.description));
                            _h->time += diff;
                            _h->format = h.format;
                            _h->is_display = h.is_display;
                            _h->args = h.arguments;
                            _h->is_display = h.is_display;
                            histories.push_back(_h);
                        }
                        merge_histories(packer_detail.histories, diff);
                    }
                    if (!_replica_job_create_detail.targets.empty())
                        job.replica_id = _replica_job_create_detail.targets.begin()->second;

                    job.connection_id = connection_id;

                    if (!_snapshot_info.empty() && 
                        (_state & mgmt_job_state::job_state_replicating || 
                        _state & mgmt_job_state::job_state_replicated || 
                        _state & mgmt_job_state::job_state_sche_completed || 
                        _state & mgmt_job_state::job_state_finished)){
                        job.snapshot_time = _snapshot_time_stamp;
                        job.snapshot_info = _snapshot_info;
                    }

                    foreach(virtual_transport_job_progress::map::value_type p, _progress){
                        std::string image_snapshot_name = p.second->output;
                        if (!p.second->discard.empty() && (p.second->parent == p.second->output))
                            image_snapshot_name = p.second->discard;
                        if (_replica_job_create_detail.disk_ids.size()){
                            job.snapshot_mapping[_replica_job_create_detail.disk_ids[p.first]] = image_snapshot_name;
                        }
                        else{
                            job.snapshot_mapping[p.first] = image_snapshot_name;
                        }
                    }
                    if (_previous_change_ids.size()){
                        mObject obj;
                        mArray previous_change_ids;
                        foreach(string_map::value_type &i, _previous_change_ids){
                            mObject o;
                            o["disk"] = i.first;
                            o["changeid"] = i.second;
                            previous_change_ids.push_back(o);
                        }
                        obj["previous_change_ids"] = previous_change_ids;
                        job.cbt_info = write(obj, json_spirit::raw_utf8);
                    }
                    foreach(virtual_transport_job_progress::map::value_type p, _progress){
                        if (_replica_job_create_detail.disk_ids.count(p.first)){
                            job.original_size[_replica_job_create_detail.disk_ids[p.first]] = p.second->total_size;
                            job.backup_size[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_size;
                            job.backup_progress[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_progress;
                            job.backup_image_offset[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_image_offset;
                        }
                        else{
                            job.original_size[p.first] = p.second->total_size;
                            job.backup_size[p.first] = p.second->backup_size;
                            job.backup_progress[p.first] = p.second->backup_progress;
                            job.backup_image_offset[p.first] = p.second->backup_image_offset;
                        }
                    }
                }
            }
        }
    }
    foreach(history_record::ptr &h, _histories){
        if (h->time >= previous_updated_time){
            histories.push_back(h);
        }
    }
    foreach(history_record::ptr &h, histories){
        saasame::transport::job_history _h;
        _h.state = h->state;
        _h.error = h->error;
        _h.description = h->description;
        _h.time = boost::posix_time::to_simple_string(h->time);
        _h.format = h->format;
        _h.is_display = h->is_display;
        _h.arguments = h->args;
        _h.__set_arguments(_h.arguments);
        bool exist = false;
        BOOST_REVERSE_FOREACH(job_history p, job.histories){
            if (_h.time == p.time && 0 == strcmp(_h.description.c_str(), p.description.c_str())){
                exist = true;
                break;
            }
        }
        if (!exist){
            job.histories.push_back(_h);
        }
    }
    if ((_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error)
        job.is_error = true;

    save_status();
    return job;
}

void virtual_transport_job::cancel()
{
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"(%s)Job suspend event captured.", _id.c_str());

    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){
    }
    else
    {
        virtual_packer_job_op op(_replica_job_create_detail.addr);
        if (!op.initialize()){
        }
        else
        {
            bool result;
            typedef std::map<std::string, std::string> string_map;
            for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++)
            {
                result = false;
                if (!(result = op.interrupt_packer_job(job->second)))
                    break;
            }
        }
    }
    _is_interrupted = true;
    _is_canceled = true;
    save_status();
}

void virtual_transport_job::interrupt()
{
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"(%s)Job interrupt event captured.", _id.c_str());
    _portal_ex.interrupt(_is_canceled);
    _is_interrupted = true;
    save_status();
}

void virtual_transport_job::remove()
{
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"(%s)Job remove event captured.", _id.c_str());
    if (_packer_jobs.size())
    {
        virtual_packer_job_op op(_replica_job_create_detail.addr);
        if (op.initialize()){
            remove_packer_jobs(op, true);
        }
    }

    __super::remove();
}

bool virtual_transport_job::packer_jobs_exist(virtual_packer_job_op &op)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    bool result = _packer_jobs.size() > 0;
    typedef std::map<std::string, std::string> string_map;
    for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++)
    {
        LOG(LOG_LEVEL_RECORD, L"(%s)checking  packer job...", _id.c_str());
        if (result = op.is_packer_job_existing(job->second))
        {
            LOG(LOG_LEVEL_RECORD, L"(%s)packer job exists.", _id.c_str());
            break;
        }
    }
    return result;
}

bool virtual_transport_job::is_packer_job_running(virtual_packer_job_op &op)
{
    FUN_TRACE;
    bool result = false;
    macho::windows::auto_lock lock(_cs);
    typedef std::map<std::string, std::string> string_map;
    for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++)
    {
        LOG(LOG_LEVEL_RECORD, L"(%s)checking  packer job...", _id.c_str());
        if (result = op.is_packer_job_running(job->second))
        {
            LOG(LOG_LEVEL_RECORD, L"(%s)packer job is running.", _id.c_str());
            break;
        }
    }
    return result;
}

bool virtual_transport_job::resume_packer_jobs(virtual_packer_job_op &op){
    FUN_TRACE;
    bool result = false;
    bool _updated = false;
    macho::windows::auto_lock lock(_cs);
    if (packer_jobs_exist(op)){
        typedef std::map<std::string, std::string> string_map;
        for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
            if (!_updated){
                packer_job_detail detail;
                if (result = op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                    if (detail.is_error || detail.state != job_state::type::job_state_finished){
                        if (!(result = op.resume_packer_job(job->second))){
                            break;
                        }
                    }
                    else if (detail.state & job_state::type::job_state_finished){
                        result = false;
                        _updated = true;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

bool virtual_transport_job::remove_packer_jobs(virtual_packer_job_op &op, bool erase_all)
{
    FUN_TRACE;
    bool result = true;
    bool previous_changid_updated = false;

    if (packer_jobs_exist(op) && (result = _running.try_lock()))
    {
        LOG(LOG_LEVEL_RECORD, L"(%s)removing packer job...", _id.c_str());
        string_map packer_jobs;
        {
            macho::windows::auto_lock lock(_cs);
            packer_jobs = _packer_jobs;
        }

        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++)
        {
            if (!previous_changid_updated)
            {
                packer_job_detail detail;
                if (result = op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail))
                {
                    if (detail.is_error || detail.state != job_state::type::job_state_finished)
                    {
                        //discard images
                        if (erase_all)
                        {
                            std::map<std::string, std::set<std::string> > carriers = _replica_job_create_detail.carriers;
                            foreach(string_set_map::value_type &c, carriers){
                                std::string carrier_id = carrier_rw::get_machine_id(c.second);
                                if (carrier_id.empty())
                                {
                                    result = false;
                                    break;
                                }
                            }

                            if (result){
                                bool has_synced_data = false;
                                foreach(virtual_transport_job_progress::map::value_type p, _progress)
                                {
                                    try
                                    {
                                        if (detail.detail.v.backup_image_offset[p.first] != 0)
                                            has_synced_data = true;

                                        if (p.second->discard.empty()){
                                            result = false;
                                            universal_disk_rw::vtr outputs = carrier_rw::open(carriers,
                                                p.second->base,
                                                p.second->output,
                                                _snapshot_info,
                                                _replica_job_create_detail.timeout,
                                                _replica_job_create_detail.is_encrypted);
                                            foreach(universal_disk_rw::ptr o, outputs)
                                            {
                                                carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                                                if (rw)
                                                {
                                                    result = rw->close(true);
                                                }
                                            }

                                            if (result){
                                                p.second->discard = p.second->output;
                                                p.second->output = p.second->parent;
                                            }
                                        }
                                    }
                                    catch (...)
                                    {
                                        result = detail.detail.v.backup_image_offset[p.first] == 0;
                                        if (result){
                                            p.second->discard = p.second->output;
                                            p.second->output = p.second->parent;
                                        }
                                    }
                                    if (!result)
                                        break;
                                }
                                if (result){
                                    if (!has_synced_data)
                                        result = discard_snapshot(_snapshot_info);
                                    if (result)
                                    {
                                        _state = job_state::type::job_state_discard;
                                        if (_replica_job_create_detail.block_mode_enable && has_synced_data){
                                            result = false;
                                            int count = 100;
                                            while (!_is_interrupted && count > 0 && !(result = is_snapshot_ready(_snapshot_info))){
                                                boost::this_thread::sleep(boost::posix_time::seconds(5));
                                                count--;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (detail.state & job_state::type::job_state_finished)
                    {
                        result = true;
                        previous_changid_updated = true;
                        _previous_change_ids = detail.detail.v.change_ids;
                        _state = mgmt_job_state::job_state_replicated;
                        save_status();
                    }
                    update(false);
                }
            }
            if (result)
            {
                if (!(result = op.remove_packer_job(job->second)))
                {
                    LOG(LOG_LEVEL_ERROR, L"(%s)Cannot remove virtual packer job %s.", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(job->second).c_str());
                    break;
                }
                else
                {
                    _packer_time_diffs.erase(job->second);
                    _packer_previous_update_time.erase(job->second);
                    _jobs_state.erase(job->second);
                    _packer_jobs.erase(job->first);
                    if (_state & mgmt_job_state::job_state_replicated)
                        _state = mgmt_job_state::job_state_sche_completed;
                }
            }
        }

        save_status();
        _running.unlock();
    }

    return result;
}

bool virtual_transport_job::find_snapshot_in_tree(const std::vector<vmware_virtual_machine_snapshots::ptr>& child, const std::wstring& snapshot_name){
    foreach(const vmware_virtual_machine_snapshots::ptr &s, child){
        if (s->name == snapshot_name)
            return true;
        if (s->child_snapshot_list.size() && find_snapshot_in_tree(s->child_snapshot_list, snapshot_name))
            return true;
    }
    return false;
}

bool virtual_transport_job::remove_snapshot(vmware_portal_::ptr portal, std::wstring& snapshot_name)
{
    FUN_TRACE;
    bool result = true;

    if (portal && !snapshot_name.empty())
    {
        try{
            carrier_rw::set_session_buffer(_replica_job_create_detail.carriers, macho::stringutils::convert_unicode_to_utf8(snapshot_name), 0, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
            vmware_virtual_machine::ptr vm = portal->get_virtual_machine(_wsz_virtual_machine_id.empty() ? macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.virtual_machine_id) : _wsz_virtual_machine_id);

            if (vm)
            {
                _portal_ex.set_portal(portal, (_wsz_virtual_machine_id.empty() ? macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.virtual_machine_id) : _wsz_virtual_machine_id));
                if (!(result = (_portal_ex.remove_temp_snapshot(vm, snapshot_name) == S_OK)))
                {
                    LOG(LOG_LEVEL_WARNING, L"(%s)Cannot remove temp snapshot %s.", _id.c_str(), snapshot_name.c_str());
                }
                else{
                    snapshot_name.clear();
                }
                _portal_ex.erase_portal((_wsz_virtual_machine_id.empty() ? macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.virtual_machine_id) : _wsz_virtual_machine_id));              
                vm = NULL;
            }
        }
        catch (const boost::exception &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
            result = false;
        }
        catch (const std::exception& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            result = false;
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"(%s)Unknown exception", _id.c_str());
            result = false;
        }
    }
    return result;
}

void virtual_transport_job::get_previous_change_ids(const std::string &cbt_info){
    if (cbt_info.length()){
        std::stringstream is;
        is << cbt_info;
        mValue cbt_config;
        read(is, cbt_config);
        mArray  previous_change_ids = find_value(cbt_config.get_obj(), "previous_change_ids").get_array();
        foreach(mValue t, previous_change_ids){
            _previous_change_ids[find_value_string(t.get_obj(), "disk")] = find_value_string(t.get_obj(), "changeid");
        }
    }
}

void virtual_transport_job::execute()
{
    VALIDATE;
    FUN_TRACE;
#ifdef DEBUG
    boost::this_thread::sleep(boost::posix_time::seconds(10));
#endif
    if (_is_canceled || _is_removing)
        return;

    bool is_get_replica_job_create_detail = false;
    if (!_is_initialized)
        is_get_replica_job_create_detail = _is_initialized = get_replica_job_create_detail(_replica_job_create_detail);
    else
        is_get_replica_job_create_detail = get_replica_job_create_detail(_replica_job_create_detail);
   
    save_config();
    save_status();
    _will_be_resumed = false;
    if (!is_get_replica_job_create_detail)
    {
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot get the replica job detail."));
        _state |= mgmt_job_state::job_state_error;
        save_status();
    }
    else if (!_replica_job_create_detail.is_paused)
    {
        virtual_packer_job_op op(_replica_job_create_detail.addr);
        vmware_portal_::ptr portal = NULL;
        vmware_virtual_machine::ptr vm = NULL;
        try{
            //std::string thumbprint;
            _portal_ex.set_log(get_log_file(), get_log_level());
            _terminated = false;
            _thread = boost::thread(&transport_job::update, this, true);
            _state &= ~mgmt_job_state::job_state_error;
            _wsz_virtual_machine_id = macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.virtual_machine_id);
            std::time_t utc = std::time(nullptr);
            std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host));
            portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
            if (!portal){
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                    error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST,
                    (record_format("Cannot connect to VMware host %1%.") % _replica_job_create_detail.host));
            }
            /*else if (S_OK != mwdc::ironman::hypervisor_ex::vmware_portal_ex::get_ssl_thumbprint(macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host), 443, thumbprint)){
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST,
                (record_format("Cannot get thumbprint data of VMware host %1%.") % _replica_job_create_detail.host));
                portal = NULL;
                }*/
            else if (!op.initialize()){
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Cannot connect to the Packer for VMware."));
            }
            else if (!op.can_connect_to_carriers(_replica_job_create_detail.carriers, _replica_job_create_detail.is_encrypted)){
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("The Packer cannot connect to the Carrier."));
            }
            else if (NULL == (vm = portal->get_virtual_machine(_wsz_virtual_machine_id))){
                record((job_state::type)(_state & mgmt_job_state::job_state_none),
                    error_codes::SAASAME_E_VIRTUAL_VM_NOTFOUND,
                    (record_format("Virtual machine %1% not found.") % _replica_job_create_detail.virtual_machine_id));
                _state |= mgmt_job_state::job_state_error;
            }
            else{
                _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
                _latest_enter_time = boost::posix_time::microsec_clock::universal_time();
                _will_be_resumed = false;
                _is_interrupted = false;
                _is_canceled = false;
                _is_source_ready = true;
                if (_state & mgmt_job_state::job_state_none){
                    if (_previous_change_ids.empty()){
                        get_previous_change_ids(_replica_job_create_detail.cbt_info);
                    }
                }
                save_status();

                if (_state & mgmt_job_state::job_state_replicating){ // Error handling for the re-enter job
                    //Checking snapshoting and job ....
                    if (!is_packer_job_running(op)){                       
                        bool is_source_snapshot_ready = find_snapshot_in_tree(vm->root_snapshot_list, macho::stringutils::convert_utf8_to_unicode(_snapshot_info));
                        if (is_source_snapshot_ready){
                            if (remove_packer_jobs(op, false)){
                                if (_state & mgmt_job_state::job_state_sche_completed){
                                    remove_snapshot(portal, _wsz_current_snapshot_name);
                                }
                                else{
                                    macho::windows::auto_lock lock(_cs);
                                    _state = mgmt_job_state::job_state_initialed;
                                }
                            }
                        }
                        else{
                            if (remove_packer_jobs(op, true)){
                                if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                                    if (!(_state & mgmt_job_state::job_state_sche_completed)){
                                        macho::windows::auto_lock lock(_cs);
                                        _state = mgmt_job_state::job_state_none;
                                        foreach(virtual_transport_job_progress::map::value_type p, _progress){
                                            p.second->backup_image_offset = 0;
                                            p.second->backup_progress = 0;
                                            p.second->backup_size = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (_state & mgmt_job_state::job_state_sche_completed){
                    if (_update(true)){
                        if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                            if (!_replica_job_create_detail.block_mode_enable || is_snapshot_ready(_snapshot_info)){
                                macho::windows::auto_lock lock(_cs);
                                _state = mgmt_job_state::job_state_none;
                                foreach(virtual_transport_job_progress::map::value_type p, _progress){
                                    p.second->backup_image_offset = 0;
                                    p.second->backup_progress = 0;
                                    p.second->backup_size = 0;
                                }
                            }
                            else {
                                LOG(LOG_LEVEL_WARNING, L"(%s)Snapshot %s is not ready.", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(_snapshot_info).c_str());
                            }
                        }
                    }
                    else{
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot connect to the management server."));
                    }
                }
                if (_state & mgmt_job_state::job_state_none){
                    _is_pending_rerun = false;
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Carrier job started."));
                    remove_snapshot(portal, _wsz_current_snapshot_name);
                    _conn_type = vm->connection_type;
                    if (!vm->is_cbt_enabled)
                    {
                        if (portal->enable_change_block_tracking(_wsz_virtual_machine_id))
                            _cbt_config_enabled = _self_enable_cbt = true;
                        else
                        {
                            record((job_state::type)(_state & mgmt_job_state::job_state_none),
                                error_codes::SAASAME_E_INTERNAL_FAIL,
                                (record_format("Cannot enable CBT.")));
                            _state |= mgmt_job_state::job_state_error;
                        }
                    }
                    else
                        _cbt_config_enabled = true;
                    //Initial checking 
                    if ((!(_state & mgmt_job_state::job_state_error)) && vm->disks.size()){
                        bool found = false;
                        foreach(std::string uri, _replica_job_create_detail.disks){
                            macho::guid_ disk_uuid(uri);
                            found = false;
                            foreach(vmware_disk_info::ptr d, vm->disks){
                                if (found = (!d->uuid.empty()) && (disk_uuid == macho::guid_(d->uuid))){
                                    virtual_transport_job_progress::ptr progress;
                                    if (!_progress.count(uri)){
                                        progress = virtual_transport_job_progress::ptr(new virtual_transport_job_progress());
                                        _progress[uri] = progress;
                                    }
                                    else{
                                        progress = _progress[uri];
                                    }
                                    progress->device_uuid = uri;
                                    progress->total_size = d->size;
                                    progress->device_key = macho::stringutils::convert_unicode_to_utf8(d->wsz_key());
                                    progress->device_name = macho::stringutils::convert_unicode_to_utf8(d->name);
                                    break;
                                }
                            }
                            if (!found){
                                _state |= mgmt_job_state::job_state_error;
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_PHYSICAL_CONFIG_FAILED, (record_format("Cannot find disks from virtual machine %1%.") % _replica_job_create_detail.virtual_machine_id));
                                break;
                            }
                        }
                        if (found){
                            _portal_ex.set_portal(portal, _wsz_virtual_machine_id);
                            if (S_OK == _portal_ex.create_temp_snapshot(vm, _wsz_current_snapshot_name)){
                                LOG(LOG_LEVEL_RECORD, L"(%s)Snapshot %s has created successfully.", _id.c_str(), _wsz_current_snapshot_name.c_str());
                                _snapshot_time_stamp = boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time());
                                std::wstring wsz_snapshot_mor_ref;
                                _portal_ex.get_snapshot_ref_item(_wsz_virtual_machine_id, _wsz_current_snapshot_name, wsz_snapshot_mor_ref);
                                _snapshot_mor_ref = macho::stringutils::convert_unicode_to_utf8(wsz_snapshot_mor_ref);
                                _snapshot_info = macho::stringutils::convert_unicode_to_utf8(_wsz_current_snapshot_name);
                                _state = mgmt_job_state::job_state_initialed;
                            }
                            else{
                                std::wstring snapshot_name = _wsz_current_snapshot_name;
                                if (remove_snapshot(portal, _wsz_current_snapshot_name))
                                    LOG(LOG_LEVEL_ERROR, L"(%s)The temp snapshot %s was forced to remove by job due to unknown error.", _id.c_str(), snapshot_name.c_str());
                                else
                                    LOG(LOG_LEVEL_ERROR, L"(%s)Cannot remove temporary snapshot %s.", _id.c_str(), snapshot_name.c_str());

                                record((job_state::type)(_state & mgmt_job_state::job_state_initialed),
                                    error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL,
                                    (record_format("Cannot create temporary snapshot %1%.") % macho::stringutils::convert_unicode_to_utf8(snapshot_name)));
                                _state |= mgmt_job_state::job_state_error;
                            }
                        }
                        save_status();
                    }
                }

                if (_state & mgmt_job_state::job_state_initialed){
                    std::string job_id = macho::stringutils::convert_unicode_to_utf8(_id);
                    std::set<std::string> connections;
                    foreach(string_map::value_type t, _replica_job_create_detail.targets)
                        connections.insert(t.first);
                    virtual_create_packer_job_detail packer_job_detail;
                    packer_job_detail.host = _replica_job_create_detail.host;
                    packer_job_detail.username = _replica_job_create_detail.username;
                    packer_job_detail.password = _replica_job_create_detail.password;
                    packer_job_detail.virtual_machine_id = _replica_job_create_detail.virtual_machine_id;
                    packer_job_detail.snapshot = _snapshot_info;
                    {
                        macho::windows::auto_lock lock(_cs);

                        packer_job_detail.__set_disks(_replica_job_create_detail.disks);

                        foreach(virtual_transport_job_progress::map::value_type p, _progress){
                            p.second->discard.clear();
                            if (p.second->base.length())
                                packer_job_detail.images[p.first].base = p.second->base;
                            else{
                                if (_replica_job_create_detail.disk_ids.count(p.first))
                                    packer_job_detail.images[p.first].base = p.second->base = _replica_job_create_detail.disk_ids[p.first];
                                else
                                    packer_job_detail.images[p.first].base = p.second->base = p.first;
                            }
                            if (p.second->backup_size == 0 && p.second->output != boost::str(boost::format("%s_%s") % p.second->base % _snapshot_info)){
                                packer_job_detail.images[p.first].parent = p.second->parent = p.second->output;
                                packer_job_detail.images[p.first].name = p.second->output = boost::str(boost::format("%s_%s") % p.second->base % _snapshot_info);
                            }
                            else{
                                packer_job_detail.images[p.first].parent = p.second->parent;
                                packer_job_detail.images[p.first].name = p.second->output;
                            }
                            if (p.second->backup_size != 0)
                                packer_job_detail.backup_size[p.first] = p.second->backup_size;
                            if (p.second->backup_progress != 0)
                                packer_job_detail.backup_progress[p.first] = p.second->backup_progress;
                            if (p.second->backup_image_offset != 0)
                                packer_job_detail.backup_image_offset[p.first] = p.second->backup_image_offset;
                            if (p.second->completed_blocks.size() != 0)
                                packer_job_detail.completed_blocks[p.first] = p.second->completed_blocks;
                        }
                    }
                    if (_replica_job_create_detail.is_full_replica)
                        _previous_change_ids.clear();
                    packer_job_detail.previous_change_ids = _previous_change_ids;

                    if (_replica_job_create_detail.buffer_size)
                        carrier_rw::set_session_buffer(_replica_job_create_detail.carriers, _snapshot_info, _replica_job_create_detail.buffer_size, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                    boost::posix_time::ptime created_time;
                    if (op.create_packer_job(connections, _replica_job_create_detail, packer_job_detail, job_id, created_time)){
                        foreach(string_map::value_type t, _replica_job_create_detail.targets){
                            macho::windows::auto_lock lock(_cs);
                            _packer_jobs[t.first] = job_id;
                            //_packer_time_diffs[job_id] = boost::posix_time::microsec_clock::universal_time() - created_time;
                            _jobs_state[job_id].state = job_state::type::job_state_none;
                        }
                        save_status();
                    }
                    else{
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_CREATE_FAIL, (record_format("Cannot create a Packer job for host %1%.") % _replica_job_create_detail.host));
                        if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                            _state = mgmt_job_state::job_state_none;
                        }
                        _state |= mgmt_job_state::job_state_error;
                        save_status();
                    }
                }
                if (!(_state & mgmt_job_state::job_state_error)){
                    if ((_state & mgmt_job_state::job_state_initialed) || (_state & mgmt_job_state::job_state_replicating)){
                        //checking replication status
                        _state = mgmt_job_state::job_state_replicating;
                        save_status();
                        bool is_error = false;
                        _has_changed = false;
                        uint32_t timeout = 60;
                        {
                            registry reg;
                            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                                if (reg[L"VMwareJobTimeOut"].exists() && (DWORD)reg[L"VMwareJobTimeOut"] >= 30){
                                    timeout = (DWORD)reg[L"VMwareJobTimeOut"];
                                }
                            }
                        }
                        boost::posix_time::ptime running_time = boost::posix_time::second_clock::universal_time();
                        while ((!is_error) && (!_is_interrupted) && (!_is_canceled)){
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                            {
                                macho::windows::auto_lock lock(_cs);
                                bool finished = false;
                                typedef std::map<std::string, running_job_state>  job_state_map;
                                foreach(job_state_map::value_type s, _jobs_state){
                                    finished = false;
                                    if (s.second.is_error){
                                        _state |= mgmt_job_state::job_state_error;
                                        is_error = true;
                                        break;
                                    }
                                    else if (s.second.disconnected){
                                        is_error = true;
                                        break;
                                    }
                                    else if (!(finished = (job_state::type::job_state_finished == (s.second.state & job_state::type::job_state_finished))))
                                        break;
                                }
                                if (finished && _jobs_state.size() == _packer_jobs.size()){
                                    _state = mgmt_job_state::job_state_replicated;
                                    save_status();
                                    break;
                                }
                                else if (!_has_changed){
                                    boost::posix_time::time_duration diff = boost::posix_time::second_clock::universal_time() - running_time;
                                    if (diff.total_seconds() > timeout){
                                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_QUEUE_FULL,
                                            record_format("Pending for available job resource."));
                                        LOG(LOG_LEVEL_RECORD, L"(%s)VM '%s' job is in queue. Release resource for another scheduled task.", _id.c_str(), vm->name.c_str());
                                        save_status();
                                        _will_be_resumed = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (_state & mgmt_job_state::job_state_replicated) {
                    //Remove snapshots and job when replication was finished.                
                    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
                    if (remove_snapshot(portal, _wsz_current_snapshot_name) && remove_packer_jobs(op, true)){
                        _state = mgmt_job_state::job_state_sche_completed;
                    }
                    save_status();
                }
                else if (_is_canceled) {
                    //Remove snapshots and job when replication was finished.                
                    if (remove_packer_jobs(op, true)){
                        portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
                        if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                            _state = mgmt_job_state::job_state_sche_completed;
                        }
                    }
                    save_status();
                }
                else if (_state & mgmt_job_state::job_state_error){
                    boost::this_thread::sleep(boost::posix_time::seconds(15));
                    if (packer_jobs_exist(op) && !is_packer_job_running(op)){
                        if (!(_replica_job_create_detail.always_retry || 0 == _previous_change_ids.size())){
                            if (remove_packer_jobs(op, true)){
                                portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
                                if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                                    _state = mgmt_job_state::job_state_none;
                                    foreach(virtual_transport_job_progress::map::value_type p, _progress){
                                        p.second->backup_image_offset = 0;
                                        p.second->backup_progress = 0;
                                        p.second->backup_size = 0;
                                    }
                                }
                            }
                            save_status();
                        }
                    }
                }
            }
        }
        catch (macho::exception_base& ex)
        {
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::filesystem::filesystem_error& ex)
        {
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::exception &ex)
        {
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const std::exception& ex)
        {
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (...)
        {
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Unknown exception"));
            _state |= mgmt_job_state::job_state_error;
        }
        _portal_ex.erase_portal((_wsz_virtual_machine_id.empty() ? macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.virtual_machine_id) : _wsz_virtual_machine_id));
        _latest_leave_time = boost::posix_time::microsec_clock::universal_time();
        save_status();
        _terminated = true;
        if (_thread.joinable())
            _thread.join();
    }
}

bool virtual_transport_job::need_to_be_resumed(){
    FUN_TRACE;
    bool result = false;
    if (_running.try_lock()){
        try{
            is_replica_job_alive();
            if (_state & mgmt_job_state::job_state_replicating){
                virtual_packer_job_op op(_replica_job_create_detail.addr);
                if (op.initialize()){
                    if (!(result = is_packer_job_running(op))){
                        string_map packer_jobs;
                        {
                            macho::windows::auto_lock lock(_cs);
                            packer_jobs = _packer_jobs;
                        }
                        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                            result = false;
                            packer_job_detail detail;
                            if (op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                                update(false);
                                if ( (!detail.is_error) && detail.state == job_state::type::job_state_finished){                                   
                                    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host));
                                    vmware_portal_::ptr portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
                                    if (remove_snapshot(portal, _wsz_current_snapshot_name)){
                                        if (remove_packer_jobs(op, true)){
                                            _state = mgmt_job_state::job_state_sche_completed;
                                            update(false);
                                        }
                                    }
                                }
                                else{
                                    result = _replica_job_create_detail.always_retry;
                                }
                                break;
                            }
                        }
                    }
                    else{
                        string_map packer_jobs;
                        {
                            macho::windows::auto_lock lock(_cs);
                            packer_jobs = _packer_jobs;
                        }
                        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                            result = false;
                            packer_job_detail detail;
                            if (op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                                update(false);
                                break;
                            }
                        }
                    }
                }
            }
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"(%s)Unkonwn exception.", _id.c_str());
        }
        _running.unlock();
    }
    return result;
}

bool virtual_transport_job::prepare_for_job_removing(){
    FUN_TRACE;
    bool result = false;
    if ((result = _running.try_lock()) && _is_source_ready && (!_replica_job_create_detail.addr.empty())){
        try{
            std::map<std::string, std::set<std::string> > carriers = _replica_job_create_detail.carriers;
            if (_packer_jobs.size()){
                virtual_packer_job_op op(_replica_job_create_detail.addr);
                if (result = op.initialize()){
                    bool _is_packer_job_running = false;
                    {
                        macho::windows::auto_lock lock(_cs);
                        for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
                            if (op.is_packer_job_running(job->second)){
                                _is_packer_job_running = true;
                                op.interrupt_packer_job(job->second);
                            }
                        }
                    }

                    if (result && (result = (!_is_packer_job_running))){

                        string_map packer_jobs;
                        {
                            macho::windows::auto_lock lock(_cs);
                            packer_jobs = _packer_jobs;
                        }

                        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                            if (op.is_packer_job_existing(job->second))
                            {
                                while (op.is_packer_job_running(job->second)){
                                    result = op.interrupt_packer_job(job->second);
                                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                                }

                                if (!(result = op.remove_packer_job(job->second)))
                                    break;
                                else{
                                    macho::windows::auto_lock lock(_cs);
                                    _packer_time_diffs.erase(job->second);
                                    _packer_previous_update_time.erase(job->second);
                                    _jobs_state.erase(job->second);
                                    _packer_jobs.erase(job->first);
                                    save_status();
                                }
                            }
                            else{
                                macho::windows::auto_lock lock(_cs);
                                _packer_time_diffs.erase(job->second);
                                _packer_previous_update_time.erase(job->second);
                                _jobs_state.erase(job->second);
                                _packer_jobs.erase(job->first);
                                save_status();
                            }
                        }
                    }
                }
            }
            
            if (result){
                if (!_wsz_current_snapshot_name.empty()){
                    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host));
                    vmware_portal_::ptr portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.username), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.password));
                    if (portal)
                    {
                        std::string thumbprint;
                        if (mwdc::ironman::hypervisor_ex::vmware_portal_ex::get_ssl_thumbprint(macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host), 443, thumbprint) != S_OK)
                        {
                            LOG(LOG_LEVEL_ERROR, L"(%s)Failed to get VMware host agent's(%s) thumbprint data.", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(_replica_job_create_detail.host).c_str());
                            portal = NULL;
                        }
                    }
                    result = remove_snapshot(portal, _wsz_current_snapshot_name);
                }
            }

            if (result){
                foreach(string_set_map::value_type &c, carriers){
                    std::string carrier_id = carrier_rw::get_machine_id(c.second);
                    if (carrier_id.empty())
                    {
                        result = false;
                        break;
                    }
                }
            }
            std::map<std::string, std::set<std::string>> missing_connections;
            if (result && carrier_rw::are_connections_ready(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted, missing_connections)){
                foreach(string_set_map::value_type &c, missing_connections){
                    foreach(std::string addr, c.second)
                        carriers[c.first].erase(addr);
                    if (carriers[c.first].empty())
                        carriers.erase(c.first);
                }
                foreach(virtual_transport_job_progress::map::value_type p, _progress){
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->discard, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->output, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted))){
                        foreach(virtual_transport_job_progress::map::value_type p, _progress){
                            try{
                                if (p.second->discard.empty()){
                                    universal_disk_rw::vtr outputs = carrier_rw::open(carriers, p.second->base, p.second->output, _snapshot_info, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                                    foreach(universal_disk_rw::ptr o, outputs){
                                        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                                        if (rw){
                                            rw->close(true);
                                        }
                                    }
                                }
                            }
                            catch (...){
                            }
                        }
                        break;
                    }
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->parent, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                }
                if (result){
                    carrier_rw::remove_connections(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                }
            }
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"(%s)Unkonwn exception.", _id.c_str());
        }
        save_status();
        _running.unlock();
    }
    return result;
}