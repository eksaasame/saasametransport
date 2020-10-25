#include "windows.h"
#include "loader_service_handler.h"
#include "..\gen-cpp\universal_disk_rw.h"
#include "..\ntfs_parser\azure_blob_op.h"
#include "..\gen-cpp\mgmt_op.h"

loader_service_handler::loader_service_handler(std::wstring session_id, int workers) : _scheduler(workers){
    _session = macho::guid_(session_id);
    _scheduler.start();
    load_jobs();
}

void loader_service_handler::ping(service_info& _return){
    _return.id = saasame::transport::g_saasame_constants.LOADER_SERVICE;
    _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
}

void loader_service_handler::create_job_ex(loader_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job){
    VALIDATE;
    FUN_TRACE;
    macho::windows::auto_lock lock(_lock);
    loader_job::ptr new_job;
    try{
        new_job = loader_job::create(*this, job_id, create_job);
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
            new_job->register_job_to_be_executed_callback_function(boost::bind(&loader_service_handler::job_to_be_executed_callback, this, _1, _2));
            new_job->register_job_was_executed_callback_function(boost::bind(&loader_service_handler::job_was_executed_callback, this, _1, _2, _3));
            _scheduler.schedule_job(new_job, new_job->get_triggers());
            _scheduled[new_job->id()] = new_job;
            new_job->save_config();
            _return = new_job->get_job_detail(false);
        }
        else{
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_ID_DUPLICATED;
            error.why = boost::str(boost::format("Job '%1%' is duplicated") % macho::stringutils::convert_unicode_to_ansi(new_job->id()));
            throw error;
        }
    }
}

void loader_service_handler::create_job(loader_job_detail& _return, const std::string& session_id, const create_job_detail& create_job) {
    VALIDATE;
    FUN_TRACE;
    macho::windows::auto_lock lock(_lock);
    if (!create_job.triggers.size()){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Doesn't have any scheduler trigger for job.";
        throw error;
    }

    loader_job::ptr new_job = loader_job::create(*this, create_job);
    if (!new_job){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
        error.why = "Create job failure";
        throw error;
    }
    else{
        if (!_scheduled.count(new_job->id())){
            new_job->register_job_to_be_executed_callback_function(boost::bind(&loader_service_handler::job_to_be_executed_callback, this, _1, _2));
            new_job->register_job_was_executed_callback_function(boost::bind(&loader_service_handler::job_was_executed_callback, this, _1, _2, _3));
            _scheduler.schedule_job(new_job, new_job->get_triggers());
            _scheduled[new_job->id()] = new_job;
            new_job->save_config();
            _return = new_job->get_job_detail(false);
        }
        else{
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_ID_DUPLICATED;
            error.why = boost::str(boost::format("Job '%1%' is duplicated") % macho::stringutils::convert_unicode_to_ansi(new_job->id()));
            throw error;
        }
    }
}

void loader_service_handler::get_job(loader_job_detail& _return, const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _return = _scheduled[id]->get_job_detail(true);
}

bool loader_service_handler::interrupt_job(const std::string& session_id, const std::string& job_id) {
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

bool loader_service_handler::resume_job(const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    _scheduled[id]->resume();
    if (!_scheduler.is_scheduled(id)){
        _scheduler.schedule_job(_scheduled[id]);
    }
    else if (!_scheduler.is_running(id)){
        trigger::vtr triggers = _scheduled[id]->get_triggers();
        triggers.push_back(trigger::ptr(new run_once_trigger()));
        _scheduler.update_job_triggers(id, triggers);
    }
    return true;
}

bool loader_service_handler::remove_job(const std::string& session_id, const std::string& job_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (_scheduled.count(id)){
        _scheduler.interrupt_job(id);
        _scheduled[id]->cancel();
        _scheduled[id]->remove();
        _scheduler.remove_job(id);
        _scheduler.interrupt_group_jobs(id);
        _scheduled.erase(id);
    }
    return true;
}

void loader_service_handler::list_jobs(std::vector<loader_job_detail> & _return, const std::string& session_id) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    foreach(loader_job::map::value_type& j, _scheduled){
        _return.push_back(j.second->get_job_detail(false));
    }
}

bool loader_service_handler::update_job(const std::string& session_id, const std::string& job_id, const create_job_detail& job) {
    macho::windows::auto_lock lock(_lock);
    FUN_TRACE;
    std::wstring id = macho::guid_(job_id);
    if (!_scheduled.count(id)){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
        throw error;
    }
    else{
        if (!job.triggers.size()){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
            error.why = "Doesn't have any scheduler trigger for job.";
            throw error;
        }
        _scheduled[id]->update_create_detail(job);
        trigger::vtr triggers = _scheduled[id]->get_triggers();
        if (_scheduler.is_scheduled(id))
            _scheduler.update_job_triggers(id, triggers, _scheduled[id]->get_latest_leave_time());
        else
            _scheduler.schedule_job(_scheduled[id], triggers);
    }
    return true;
}

void loader_service_handler::terminate(const std::string& session_id) {
    // Your implementation goes here
    suspend_jobs();
    _scheduler.stop();
    printf("exit\n");
}

bool loader_service_handler::remove_snapshot_image(const std::string& session_id, const std::map<std::string, image_map_info> & images){
    FUN_TRACE;
    bool result = true;
    invalid_operation        err_ex;
    macho::windows::auto_lock lock(_f_lock);
    typedef std::map<std::string, image_map_info> map_type;

    if (images.size())
    {
        boost::filesystem::directory_iterator end_itr;

        try
        {
            foreach(const map_type::value_type &image_map, images)
            {
                if (image_map.second.image.empty())
                    continue;

                std::set<connection>    conns;

                foreach(std::string connection_id, image_map.second.connection_ids)
                {
                    if (connections_map.count(connection_id))
                    {
                        connection conn = connections_map[connection_id];
                        if (conn.type != connection_type::type::WEBDAV_WITH_SSL && conn.type != connection_type::type::WEBDAV)
                            conns.insert(conn);
                    }
                    else
                    {
                        err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
                        err_ex.why = boost::str(boost::format("Unknown connection id(%1%).") % connection_id);
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                        throw err_ex;
                    }
                }
                if (!conns.empty()){
                    imagex::irm_imagex_op::ptr op = irm_imagex_carrier_op::get(conns);
                    if (op)
                    {
                        std::wstring lck_file = macho::stringutils::convert_utf8_to_unicode(image_map.second.image) + L".lck";
                        if (!op->is_file_existing(lck_file))
                            result = op->remove_image(macho::stringutils::convert_utf8_to_unicode(image_map.second.image));
                        else
                        {
                            result = false;
                            break;
                        }
                    }
                    else
                        result = false;
                }
            }
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Encountered an exception.");
            result = false;
        }
    }
    else
    {
        result = false;
        LOG(LOG_LEVEL_ERROR, L"the snapshot image list is empty.");
    }

    return result;
}

bool loader_service_handler::running_job(const std::string& session_id, const std::string& job_id)
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

bool loader_service_handler::verify_management(const std::string& management, const int32_t port, const bool is_ssl){
    bool result = false;
    FUN_TRACE; 
    mgmt_op op({ management }, std::string(), port, is_ssl);
    result = op.open();
    return result;
}

bool loader_service_handler::set_customized_id(const std::string& session_id, const std::string& disk_addr, const std::string& disk_id){
    com_init com;
    FUN_TRACE;
    storage::ptr stg = storage::get();
    storage::disk::vtr disks = stg->get_disks();
    storage::disk::ptr _d;
    LOG(LOG_LEVEL_RECORD, L"disk addr : %s, disk id : %s "
        ,macho::stringutils::convert_ansi_to_unicode(disk_addr).c_str()
        ,macho::stringutils::convert_ansi_to_unicode(disk_id).c_str());
    DWORD scsi_port = 0, scsi_bus = 0, scsi_target_id = 0, scsi_lun = 0;
    if (4 == sscanf_s(disk_addr.c_str(), "%d:%d:%d:%d", &scsi_port, &scsi_bus, &scsi_target_id, &scsi_lun)){
        foreach(storage::disk::ptr d, disks){
            if (d->scsi_port() == scsi_port &&
                d->scsi_bus() == scsi_bus &&
                d->scsi_target_id() == scsi_target_id &&
                d->scsi_logical_unit() == scsi_lun){
                _d = d;
                break;
            }
        }
    }
    else{
        foreach(storage::disk::ptr d, disks){
            general_io_rw::ptr rd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % d->number()));
            if (rd){
                uint32_t r = 0;
                std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                memset(data.get(), 0, 512);
                if (rd->sector_read((d->size() >> 9) - 40, 1, data.get(), r)){
                    GUID guid;
                    memcpy(&guid, &(data.get()[512 - 16]), 16);
                    if (macho::guid_(guid) == macho::guid_(disk_addr)){
                        _d = d;
                        break;
                    }
                }
            }
        }
    }
    if (_d && disk_id.length() == 36){
        if (_d->is_read_only()){
            if (!_d->clear_read_only_flag()){
                LOG(LOG_LEVEL_ERROR, _T("Cannot clear the read only flag of disk (%s)."), macho::stringutils::convert_ansi_to_unicode(disk_addr).c_str());
            }
        }
        bool result = false;
        int count = 10;
        std::wstring scsi_addr = boost::str(boost::wformat(L"%d:%d:%d:%d") % _d->scsi_port() % _d->scsi_bus() % _d->scsi_target_id() % _d->scsi_logical_unit());
        while ( count > 0 ){
            general_io_rw::ptr wd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % _d->number()), false);
            if (wd){
                uint32_t r = 0, w = 0;
                uint64_t sec = (_d->size() >> 9) - 40;
                std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                memset(data.get(), 0, 512);
                if (wd->sector_read(sec, 1, data.get(), r)){
                    GUID guid = macho::guid_(disk_id);
                    memcpy(&(data.get()[512 - 16]), &guid, 16);
                    result = wd->sector_write(sec, data.get(), 1, w);
                }
                count--;
                if (result)
                    break;
                else
                    boost::this_thread::sleep(boost::posix_time::seconds(5));
            }
        }
        if (result){
            LOG(LOG_LEVEL_RECORD, L"Succeeded to set disk id : %s (%s)"
                , macho::stringutils::convert_ansi_to_unicode(disk_id).c_str(), scsi_addr.c_str());
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Failded to set disk id : %s (%s)"
                , macho::stringutils::convert_ansi_to_unicode(disk_id).c_str(), scsi_addr.c_str());
        }
        return result;
    }
    else if (!_d){
        LOG(LOG_LEVEL_ERROR, L"Cannot find the disk : %s "
            , macho::stringutils::convert_ansi_to_unicode(disk_addr).c_str());
    }
    return false;
}

void loader_service_handler::load_jobs(){
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
                    loader_job::ptr job = loader_job::load(*this, dir_iter->path(), dir_iter->path().wstring() + L".status");
                    if (!_scheduled.count(job->id())){
                        job->register_job_to_be_executed_callback_function(boost::bind(&loader_service_handler::job_to_be_executed_callback, this, _1, _2));
                        job->register_job_was_executed_callback_function(boost::bind(&loader_service_handler::job_was_executed_callback, this, _1, _2, _3));
                        _scheduler.schedule_job(job, job->get_triggers());
                        _scheduled[job->id()] = job;
                    }
                }
                else if (dir_iter->path().extension() == CASCADING_JOB_EXTENSION){
                    cascading_job::ptr job = cascading_job::load(*this, dir_iter->path(), dir_iter->path().wstring() + L".status");
                    job->register_job_to_be_executed_callback_function(boost::bind(&loader_service_handler::job_to_be_executed_callback, this, _1, _2));
                    job->register_job_was_executed_callback_function(boost::bind(&loader_service_handler::job_was_executed_callback, this, _1, _2, _3));
                    _scheduler.schedule_job(job, job->get_triggers());
                }
            }
        }
    }
}

void loader_service_handler::suspend_jobs(){
    macho::windows::auto_lock lock(_lock);
    foreach(loader_job::map::value_type& j, _scheduled){
        j.second->interrupt();
    }
}

void loader_service_handler::create_vhd_disk_from_snapshot(std::string& _return, const std::string& connection_string, const std::string& container, const std::string& original_disk_name, const std::string& target_disk_name, const std::string& snapshot){
    create_vhd_blob_from_snapshot_task::ptr  task(new create_vhd_blob_from_snapshot_task(connection_string, container, original_disk_name, target_disk_name, snapshot));
    _return = macho::stringutils::convert_unicode_to_utf8(task->id());
    _create_vhd_blob_from_snapshot_tasks[_return] = task;
    _tasker.schedule_job(task, run_once_trigger());
    _tasker.start();
}

bool loader_service_handler::is_snapshot_vhd_disk_ready(const std::string& task_id){
    bool result = false;
#ifdef _AZURE_BLOB
    if (_create_vhd_blob_from_snapshot_tasks.count(task_id)){
        if (!_tasker.is_running(macho::stringutils::convert_utf8_to_unicode(task_id))){
            result = !_create_vhd_blob_from_snapshot_tasks[task_id]->is_error();
            std::string rest = _create_vhd_blob_from_snapshot_tasks[task_id]->result();
            _create_vhd_blob_from_snapshot_tasks.erase(task_id);
            if (!result){
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(rest).c_str());
                saasame::transport::invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL;
                error.why = boost::str(boost::format("Cannot create vhd disk : '%1%'.") % rest);
            }
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot find the task : %s "
            , macho::stringutils::convert_ansi_to_unicode(task_id).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
        error.why = boost::str(boost::format("Task '%1%' is not found.") % task_id);
        throw error;
    }
#endif
    return result;
}

bool loader_service_handler::delete_vhd_disk(const std::string& connection_string, const std::string& container, const std::string& disk_name){
#ifdef _AZURE_BLOB
    return azure_page_blob_op::delete_vhd_page_blob(connection_string, container, disk_name);
#else
    return false;
#endif
}

bool loader_service_handler::delete_vhd_disk_snapshot(const std::string& connection_string, const std::string& container, const std::string& disk_name, const std::string& snapshot){
#ifdef _AZURE_BLOB
    return azure_page_blob_op::delete_vhd_page_blob_snapshot(connection_string, container, disk_name, snapshot);
#else
    return false;
#endif
}

void loader_service_handler::get_vhd_disk_snapshots(std::vector<vhd_snapshot> & _return, const std::string& connection_string, const std::string& container, const std::string& disk_name){
#ifdef _AZURE_BLOB
    azure_page_blob_op::blob_snapshot::vtr snapshots = azure_page_blob_op::get_vhd_page_blob_snapshots(connection_string, container, disk_name);
    foreach(azure_page_blob_op::blob_snapshot &s, snapshots){
        vhd_snapshot vhd_snap;
        vhd_snap.id = s.id;
        vhd_snap.name = s.name;
        vhd_snap.datetime = s.date_time;
        _return.push_back(vhd_snap);
    }
#endif
}

void loader_service_handler::create_vhd_blob_from_snapshot_task::execute(){
#ifdef _AZURE_BLOB
    azure_page_blob_op::ptr op_ptr = azure_page_blob_op::open(_connection_string);
    LOG(LOG_LEVEL_RECORD, macho::stringutils::convert_utf8_to_unicode(boost::str(boost::format("Start to create %1%/%2% from %1%/%3%(%4%).") % _container %_target_disk_name %_original_disk_name %_snapshot)));
    if (op_ptr){
        _is_error = !op_ptr->create_vhd_page_blob_from_snapshot(_container, _original_disk_name, _snapshot, _target_disk_name);
        if (_is_error){
            _result = boost::str(boost::format("Cannot create %1%/%2% from %1%/%3%(%4%).") %_container %_target_disk_name %_original_disk_name %_snapshot);
        }
    }
    else{
        _is_error = true;
        _result = "Cannot open the connection string.";
    }
#endif
}

bool loader_service_handler::verify_connection_string(const std::string& connection_string){
#ifdef _AZURE_BLOB
    macho::windows::auto_lock lock(_verify_lock);
    azure_page_blob_op::ptr op_ptr = azure_page_blob_op::open(connection_string);
    if (op_ptr){
        macho::guid_ container = macho::guid_::create();
        bool result = op_ptr->create_vhd_page_blob(container, "verify.vhd", 1024 * 1024);
        op_ptr->delete_vhd_page_blob(container, "verify.vhd");
        return result;
    }
#endif
    return false;
}

void loader_service_handler::create_cascading_job(std::string id, std::string machine_id, std::string loader_job_id, saasame::transport::create_job_detail detail){
    macho::windows::auto_lock lock(_lock);
    std::wstring _id = macho::stringutils::convert_utf8_to_unicode(id);
    if (_scheduler.is_scheduled(_id)){
        if (!_scheduler.is_running(_id)){
            trigger::vtr triggers = _scheduler.get_job_triggers(_id);
            bool found = false;
            foreach(trigger::ptr t, triggers){
                run_once_trigger* run_once = dynamic_cast<run_once_trigger*>(t.get());
                if (run_once)
                    found = true;
            }
            if (!found){
                triggers.push_back(trigger::ptr(new run_once_trigger()));
                _scheduler.update_job_triggers(_id, triggers);
            }
        }
    }
    else{
        cascading_job::ptr job = cascading_job::create(*this, id, machine_id, loader_job_id, detail);
        job->save_config();
        job->register_job_to_be_executed_callback_function(boost::bind(&loader_service_handler::job_to_be_executed_callback, this, _1, _2));
        job->register_job_was_executed_callback_function(boost::bind(&loader_service_handler::job_was_executed_callback, this, _1, _2, _3));
        _scheduler.schedule_job(job, job->get_triggers());
    }
}

macho::job::vtr loader_service_handler::get_cascading_jobs(std::wstring id){
    return _scheduler.get_group_jobs(id);
}
