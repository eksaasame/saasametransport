#include "transport_job.h"
#include "common_service.h"
#include "scheduler_service.h"
#include "management_service.h"
#include <codecvt>
#include "mgmt_op.h"
#include "carrier_rw.h"

#define RetryCount     3

#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

class scheduler_trigger : virtual public trigger{
public:
    scheduler_trigger(trigger::vtr _triggers){
        foreach(trigger::ptr t, _triggers){
            triggers.push_back(t->clone());
        }
    }

    scheduler_trigger(boost::posix_time::ptime start_time, boost::posix_time::ptime latest_leave_time, boost::posix_time::ptime latest_enter_time, std::vector<saasame::transport::job_trigger> _triggers){
        _name = _T("scheduler_trigger");
        _start_time = start_time;
        _latest_finished_time = boost::date_time::not_a_date_time;
        _priority = 100;
        foreach(saasame::transport::job_trigger t, _triggers){
            trigger::ptr _t;
            if (saasame::transport::job_trigger_type::interval_trigger == t.type){
                if (t.start.length() && t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), latest_leave_time == boost::date_time::not_a_date_time ? latest_enter_time : latest_leave_time));
                }
                else if (t.start.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, latest_leave_time == boost::date_time::not_a_date_time ? latest_enter_time : latest_leave_time));
                }
                else if (t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), latest_leave_time == boost::date_time::not_a_date_time ? latest_enter_time : latest_leave_time));
                }
                else{
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, latest_leave_time == boost::date_time::not_a_date_time ? latest_enter_time : latest_leave_time));
                }
                triggers.push_back(_t);
            }    
        }
    }

    virtual ~scheduler_trigger(){}
    
    virtual trigger::ptr             clone(){
        trigger::ptr t = trigger::ptr(new scheduler_trigger(triggers));
        trigger::clone_values((*t));
        return t;
    }
    
    virtual boost::posix_time::ptime const set_latest_finished_time(boost::posix_time::ptime finished_time = boost::posix_time::microsec_clock::universal_time()){
        foreach(trigger::ptr _t, triggers){
            if (finished_time != boost::date_time::not_a_date_time && finished_time > _t->get_start_time()){
                _t->set_fire_time(finished_time);
                _t->set_latest_finished_time(finished_time);
            }
        }
        return _latest_finished_time = finished_time;
    }

    virtual void                     set_fire_time(boost::posix_time::ptime fire_time = boost::posix_time::microsec_clock::universal_time()){
        _final_fire_time = fire_time;
    }

    virtual bool                     may_fire_again() const {
        foreach(trigger::ptr _t, triggers){
            if (_t->may_fire_again())
                return true;
        }
        return false;
    }

    virtual boost::posix_time::ptime get_next_fire_time(const boost::posix_time::ptime previous_fire_time) const {
        std::vector<boost::posix_time::ptime> next_fire_times;
        foreach(trigger::ptr _t, triggers){
            if (_t->may_fire_again())
                next_fire_times.push_back(_t->get_next_fire_time(_final_fire_time));
        }
        if (next_fire_times.size()){
            std::sort(next_fire_times.begin(), next_fire_times.end(), [](boost::posix_time::ptime const& lhs, boost::posix_time::ptime const& rhs){ return lhs < rhs; });
            return next_fire_times[0];
        }
        return _start_time;
    }
private:
    trigger::vtr triggers;
};

class continues_data_replication_trigger : virtual public interval_trigger{
public:
    continues_data_replication_trigger(boost::posix_time::time_duration repeat_interval = boost::posix_time::seconds(15)) : interval_trigger(repeat_interval){
        _name = _T("continues_data_replication_trigger");
        _priority = -2;
    }
};

transport_job::transport_job(std::wstring id, saasame::transport::create_job_detail detail) : _is_source_ready(false), _is_pending_rerun(false), _enable_cdr_trigger(false), _is_continuous_data_replication(false), _will_be_resumed(false), _is_need_full_rescan(false), _need_update(false), _terminated(false), _state(job_state_none), _create_job_detail(detail), _is_initialized(false), _is_interrupted(false), _is_canceled(false), removeable_job(id, macho::guid_(GUID_NULL)){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"Machine"].exists() && reg[L"Machine"].is_string())
            _machine_id = reg[L"Machine"].string();
    }
}

transport_job::transport_job(saasame::transport::create_job_detail detail) : _is_source_ready(false), _is_pending_rerun(false), _enable_cdr_trigger(false), _is_continuous_data_replication(false), _will_be_resumed(false), _is_need_full_rescan(false), _need_update(false), _terminated(false), _state(job_state_none), _create_job_detail(detail), _is_initialized(false), _is_interrupted(false), _is_canceled(false), removeable_job(macho::guid_::create(), macho::guid_(GUID_NULL)) {
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"Machine"].exists() && reg[L"Machine"].is_string())
            _machine_id = reg[L"Machine"].string();
    }
}

void transport_job::update_create_detail(const saasame::transport::create_job_detail& detail){
    FUN_TRACE;
    {
        macho::windows::auto_lock lock(_config_lock);
        _create_job_detail = detail;
    }
    save_config();
}

transport_job::ptr transport_job::create(std::string id, saasame::transport::create_job_detail detail){
    FUN_TRACE;
    transport_job::ptr job;
    if (detail.type == saasame::transport::job_type::physical_transport_type){
        // Physical tranpsort type
        job = transport_job::ptr(new physical_transport_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
#ifndef IRM_TRANSPORTER
    else if (detail.type == saasame::transport::job_type::virtual_transport_type){
        // virtual tranpsort type;
        job = transport_job::ptr(new virtual_transport_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    else if (detail.type == saasame::transport::job_type::winpe_transport_job_type){
        // winpe tranpsort type
        job = transport_job::ptr(new winpe_transport_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
#endif
    return job;
}

transport_job::ptr transport_job::create(saasame::transport::create_job_detail detail){
    FUN_TRACE;
    transport_job::ptr job;
    if (detail.type == saasame::transport::job_type::physical_transport_type){
        // Physical tranpsort type
        job = transport_job::ptr(new physical_transport_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();     
    }
#ifndef IRM_TRANSPORTER
    else if (detail.type == saasame::transport::job_type::virtual_transport_type){
        // virtual tranpsort type;
        job = transport_job::ptr(new virtual_transport_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    else if (detail.type == saasame::transport::job_type::winpe_transport_job_type){
        // winpe tranpsort type;
        job = transport_job::ptr(new winpe_transport_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
#endif
    return job;
}

transport_job::ptr transport_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file){
    FUN_TRACE;
    transport_job::ptr job;
    std::wstring id;
    saasame::transport::create_job_detail _create_job_detail = load_config(config_file.wstring(), id);
    if (_create_job_detail.type == saasame::transport::job_type::physical_transport_type){
        // Physical tranpsort type
        job = transport_job::ptr(new physical_transport_job(id, _create_job_detail));
        job->load_replica_create_job_detail(config_file.wstring());
        job->load_status(status_file.wstring());
        
    }
#ifndef IRM_TRANSPORTER
    else if (_create_job_detail.type == saasame::transport::job_type::virtual_transport_type){
        // virtual tranpsort type;
        job = transport_job::ptr(new virtual_transport_job(id, _create_job_detail));
        job->load_replica_create_job_detail(config_file.wstring());
        job->load_status(status_file.wstring());
    }
    else if (_create_job_detail.type == saasame::transport::job_type::winpe_transport_job_type){
        // virtual tranpsort type;
        job = transport_job::ptr(new winpe_transport_job(id, _create_job_detail));
        job->load_replica_create_job_detail(config_file.wstring());
        job->load_status(status_file.wstring());
    }
#endif
    return job;
}

void transport_job::remove(){
    FUN_TRACE;
    _is_removing = true;
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        _running.unlock();
    }
}

saasame::transport::create_job_detail transport_job::load_config(std::wstring config_file, std::wstring &job_id){
    FUN_TRACE;
    saasame::transport::create_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "id").get_str());
        detail.management_id = find_value(_create_job_detail.get_obj(), "management_id").get_str();
        detail.mgmt_port = find_value_int32(_create_job_detail.get_obj(), "mgmt_port", 80);
        detail.is_ssl = find_value_bool(_create_job_detail.get_obj(), "is_ssl");
        mArray addr = find_value(_create_job_detail.get_obj(), "mgmt_addr").get_array();
        foreach(mValue a, addr){
            detail.mgmt_addr.insert(a.get_str());
        }
        detail.type = (saasame::transport::job_type::type)find_value(_create_job_detail.get_obj(), "type").get_int();
        mArray triggers = find_value(_create_job_detail.get_obj(), "triggers").get_array();
        foreach(mValue t, triggers){
            saasame::transport::job_trigger trigger;
            trigger.type = (saasame::transport::job_trigger_type::type)find_value(t.get_obj(), "type").get_int();
            trigger.start = find_value(t.get_obj(), "start").get_str();
            trigger.finish = find_value(t.get_obj(), "finish").get_str();
            trigger.interval = find_value(t.get_obj(), "interval").get_int();
            trigger.duration = find_value(t.get_obj(), "duration").get_int();
            trigger.id = find_value_string(t.get_obj(), "id");
            detail.triggers.push_back(trigger);
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")));
    }
    catch (...){
    }
    return detail;
}

void transport_job::load_replica_create_job_detail(std::wstring config_file){
    FUN_TRACE;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        const mObject job_config_obj = job_config.get_obj();
        mObject::const_iterator i = job_config_obj.find("replica_job_create_detail");
        if (i == job_config_obj.end()){
            _is_initialized = false;
        }
        else{
            _is_initialized = true;
            mValue _replica_job_detail = i->second;
            mObject _replica_job_detail_obj = _replica_job_detail.get_obj();
            _replica_job_create_detail = replica_job_create_detail();
            _replica_job_create_detail.host = find_value(_replica_job_detail_obj, "host").get_str();
            mArray  addr = find_value(_replica_job_detail_obj, "addr").get_array();
            foreach(mValue a, addr){
                _replica_job_create_detail.addr.insert(a.get_str());
            }
            macho::bytes user, password;
            user.set(macho::stringutils::convert_utf8_to_unicode(find_value(_replica_job_detail_obj, "u1").get_str()));
            password.set(macho::stringutils::convert_utf8_to_unicode(find_value(_replica_job_detail_obj, "u2").get_str()));
            macho::bytes u1 = macho::windows::protected_data::unprotect(user, true);
            macho::bytes u2 = macho::windows::protected_data::unprotect(password, true);
            
            if (u1.length() && u1.ptr()){
                _replica_job_create_detail.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
            }
            else{
                _replica_job_create_detail.username = "";
            }
            if (u2.length() && u2.ptr()){
                _replica_job_create_detail.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
            }
            else{
                _replica_job_create_detail.password = "";
            }

            _replica_job_create_detail.type = (saasame::transport::job_type::type)find_value(_replica_job_detail_obj, "type").get_int();
            _replica_job_create_detail.virtual_machine_id = find_value(_replica_job_detail_obj, "virtual_machine_id").get_str();
            mArray  disks = find_value(_replica_job_detail_obj, "disks").get_array();
            foreach(mValue d, disks){
                _replica_job_create_detail.disks.insert(d.get_str());
            }

            mArray  targets = find_value(_replica_job_detail_obj, "targets").get_array();
            foreach(mValue t, targets){
                _replica_job_create_detail.targets[find_value_string(t.get_obj(), "connection_id")] = find_value_string(t.get_obj(), "replica_id");
            }

            mArray  carriers = find_value(_replica_job_detail_obj, "carriers").get_array();
            foreach(mValue c, carriers){
                std::string connection_id = find_value_string(c.get_obj(), "connection_id");
                mArray  carrier_addr = find_value(c.get_obj(), "carrier_addr").get_array();
                std::set<std::string> _addr;
                foreach(mValue a, carrier_addr){
                    _addr.insert(a.get_str());
                }
                _replica_job_create_detail.carriers[connection_id] = _addr;
            }
            mArray full_replicas = find_value(_replica_job_detail_obj, "full_replicas").get_array();

            mArray  disk_ids = find_value(_replica_job_detail_obj, "disk_ids").get_array();
            foreach(mValue t, disk_ids){
                _replica_job_create_detail.disk_ids[find_value_string(t.get_obj(), "disk")] = find_value_string(t.get_obj(), "id");
            }
            _replica_job_create_detail.cbt_info = find_value_string(_replica_job_detail_obj, "cbt_info");
            _replica_job_create_detail.snapshot_info = find_value_string(_replica_job_detail_obj, "snapshot_info");
            _replica_job_create_detail.checksum_verify = find_value_bool(_replica_job_detail_obj, "checksum_verify");
            _replica_job_create_detail.timeout = find_value_int32(_replica_job_detail_obj, "timeout", 600);
            _replica_job_create_detail.is_encrypted = find_value_bool(_replica_job_detail_obj, "is_encrypted");
            _replica_job_create_detail.is_paused = find_value_bool(_replica_job_detail_obj, "is_paused");
            _replica_job_create_detail.worker_thread_number = find_value_int32(_replica_job_detail_obj, "worker_thread_number");
            _replica_job_create_detail.block_mode_enable = find_value_bool(_replica_job_detail_obj, "block_mode_enable");
            _replica_job_create_detail.file_system_filter_enable = find_value_bool(_replica_job_detail_obj, "file_system_filter_enable");
            _replica_job_create_detail.min_transport_size = find_value_int32(_replica_job_detail_obj, "min_transport_size");
            _replica_job_create_detail.full_min_transport_size = find_value_int32(_replica_job_detail_obj, "full_min_transport_size");
            _replica_job_create_detail.is_full_replica = find_value_bool(_replica_job_detail_obj, "is_full_replica");
            _replica_job_create_detail.buffer_size = find_value_int32(_replica_job_detail_obj, "buffer_size");
            _replica_job_create_detail.is_compressed = find_value_bool(_replica_job_detail_obj, "is_compressed");
            _replica_job_create_detail.is_checksum = find_value_bool(_replica_job_detail_obj, "is_checksum");
            _replica_job_create_detail.is_only_single_system_disk = find_value_bool(_replica_job_detail_obj, "is_only_single_system_disk", false);
            _replica_job_create_detail.is_continuous_data_replication = find_value_bool(_replica_job_detail_obj, "is_continuous_data_replication", false);
            _replica_job_create_detail.always_retry = find_value_bool(_replica_job_detail_obj, "always_retry", true);
            _replica_job_create_detail.is_compressed_by_packer = find_value_bool(_replica_job_detail_obj, "is_compressed_by_packer", false);
            _replica_job_create_detail.pre_snapshot_script = find_value_string(_replica_job_detail_obj, "pre_snapshot_script");
            _replica_job_create_detail.post_snapshot_script = find_value_string(_replica_job_detail_obj, "post_snapshot_script");
            if (_replica_job_detail_obj.end() != _replica_job_detail_obj.find("priority_carrier")){
                mArray  priority_carrier = find_value_array(_replica_job_detail_obj, "priority_carrier");
                foreach(mValue c, carriers){
                    std::string connection_id = find_value_string(c.get_obj(), "connection_id");
                    std::string addr = find_value_string(c.get_obj(), "addr");
                    _replica_job_create_detail.priority_carrier[connection_id] = addr;
                }
            }
            if (_replica_job_detail_obj.end() != _replica_job_detail_obj.find("excluded_paths")){
                mArray  excluded_paths = find_value(_replica_job_detail_obj, "excluded_paths").get_array();
                foreach(mValue p, excluded_paths){
                    _replica_job_create_detail.excluded_paths.insert(p.get_str());
                }
            }
            if (_replica_job_detail_obj.end() != _replica_job_detail_obj.find("previous_excluded_paths")){
                mArray  previous_excluded_paths = find_value(_replica_job_detail_obj, "previous_excluded_paths").get_array();
                foreach(mValue p, previous_excluded_paths){
                    _replica_job_create_detail.previous_excluded_paths.insert(p.get_str());
                }
            }
            if (_replica_job_detail_obj.end() != _replica_job_detail_obj.find("checksum_target")){
                mArray  checksum_target = find_value(_replica_job_detail_obj, "checksum_target").get_array();
                foreach(mValue t, checksum_target){
                    _replica_job_create_detail.checksum_target[find_value_string(t.get_obj(), "connection_id")] = find_value_string(t.get_obj(), "machine_id");
                }
            }
            if (_machine_id.empty()){
                mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
                _machine_id = find_value_string(_create_job_detail.get_obj(), "machine_id");
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")));
    }
    catch (...){
    }
}

void transport_job::save_config(){
    FUN_TRACE;
    try{
        using namespace json_spirit;
        mObject job_config;
        mObject job_detail;
        job_detail["id"] = macho::stringutils::convert_unicode_to_utf8(_id);    
        job_detail["type"] = _create_job_detail.type;
        job_detail["management_id"] = _create_job_detail.management_id;
        job_detail["mgmt_port"] = _create_job_detail.mgmt_port;
        job_detail["is_ssl"] = _create_job_detail.is_ssl;
        job_detail["machine_id"] = _machine_id;
        mArray  mgmt_addr(_create_job_detail.mgmt_addr.begin(), _create_job_detail.mgmt_addr.end());
        job_detail["mgmt_addr"] = mgmt_addr;
        mArray triggers;
        foreach(saasame::transport::job_trigger &t, _create_job_detail.triggers){
            mObject trigger;
            if (0 == t.start.length()) 
                t.start = boost::posix_time::to_simple_string(_created_time);
            trigger["type"] = (int)t.type;
            trigger["start"] = t.start;
            trigger["finish"] = t.finish;
            trigger["interval"] = t.interval;
            trigger["duration"] = t.duration;
            trigger["id"] = t.id;
            triggers.push_back(trigger);
        }
        job_detail["triggers"] = triggers;
        job_config["create_job_detail"] = job_detail;
        
        if (_is_initialized){
            mObject replica_job_detail;
            replica_job_detail["host"] = _replica_job_create_detail.host;
            mArray  addr(_replica_job_create_detail.addr.begin(), _replica_job_create_detail.addr.end());
            replica_job_detail["addr"] = addr;
            macho::bytes user, password;
            user.set((LPBYTE)_replica_job_create_detail.username.c_str(), _replica_job_create_detail.username.length());
            password.set((LPBYTE)_replica_job_create_detail.password.c_str(), _replica_job_create_detail.password.length());
            macho::bytes u1 = macho::windows::protected_data::protect(user, true);
            macho::bytes u2 = macho::windows::protected_data::protect(password, true);
            replica_job_detail["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
            replica_job_detail["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
            replica_job_detail["type"] = (int)_replica_job_create_detail.type;
            replica_job_detail["virtual_machine_id"] = _replica_job_create_detail.virtual_machine_id;
            mArray  disks(_replica_job_create_detail.disks.begin(), _replica_job_create_detail.disks.end());
            replica_job_detail["disks"] = disks;
            mArray targets;
            foreach(string_map::value_type &t, _replica_job_create_detail.targets){
                mObject target;
                target["connection_id"] = t.first;
                target["replica_id"] = t.second;
                targets.push_back(target);
            }
            replica_job_detail["targets"] = targets;
            mArray carriers;
            foreach(string_set_map::value_type &t, _replica_job_create_detail.carriers){
                mObject carrier;
                carrier["connection_id"] = t.first;
                mArray carrier_addr( t.second.begin(), t.second.end());
                carrier["carrier_addr"] = carrier_addr;
                carriers.push_back(carrier);
            }
            replica_job_detail["carriers"] = carriers;
            mArray  full_replicas(_replica_job_create_detail.full_replicas.begin(), _replica_job_create_detail.full_replicas.end());
            replica_job_detail["full_replicas"] = full_replicas;

            mArray disk_ids;
            foreach(string_map::value_type &i, _replica_job_create_detail.disk_ids){
                mObject id;
                id["disk"] = i.first;
                id["id"] = i.second;
                disk_ids.push_back(id);
            }
            replica_job_detail["disk_ids"] = disk_ids;
            replica_job_detail["cbt_info"] = _replica_job_create_detail.cbt_info;
            replica_job_detail["snapshot_info"] = _replica_job_create_detail.snapshot_info;
            replica_job_detail["checksum_verify"] = _replica_job_create_detail.checksum_verify;
            replica_job_detail["timeout"] = _replica_job_create_detail.timeout;
            replica_job_detail["is_encrypted"] = _replica_job_create_detail.is_encrypted;
            replica_job_detail["is_paused"] = _replica_job_create_detail.is_paused;
            replica_job_detail["worker_thread_number"] = _replica_job_create_detail.worker_thread_number;
            replica_job_detail["block_mode_enable"] = _replica_job_create_detail.block_mode_enable;
            replica_job_detail["file_system_filter_enable"] = _replica_job_create_detail.file_system_filter_enable;
            replica_job_detail["min_transport_size"] = _replica_job_create_detail.min_transport_size;
            replica_job_detail["full_min_transport_size"] = _replica_job_create_detail.full_min_transport_size;
            replica_job_detail["is_full_replica"] = _replica_job_create_detail.is_full_replica;
            replica_job_detail["buffer_size"] = _replica_job_create_detail.buffer_size;
            replica_job_detail["is_compressed"] = _replica_job_create_detail.is_compressed;
            replica_job_detail["is_checksum"] = _replica_job_create_detail.is_checksum;
            replica_job_detail["is_only_single_system_disk"] = _replica_job_create_detail.is_only_single_system_disk;
            replica_job_detail["is_continuous_data_replication"] = _replica_job_create_detail.is_continuous_data_replication;
            replica_job_detail["always_retry"] = _replica_job_create_detail.always_retry;
            replica_job_detail["is_compressed_by_packer"] = _replica_job_create_detail.is_compressed_by_packer;
            replica_job_detail["pre_snapshot_script"] = _replica_job_create_detail.pre_snapshot_script;
            replica_job_detail["post_snapshot_script"] = _replica_job_create_detail.post_snapshot_script;

            mArray priority_carrier;
            foreach(string_map::value_type &i, _replica_job_create_detail.priority_carrier){
                mObject target;
                target["connection_id"] = i.first;
                target["addr"] = i.second;
                priority_carrier.push_back(target);
            }
            replica_job_detail["priority_carrier"] = priority_carrier;
            if (!_replica_job_create_detail.excluded_paths.empty()){
                mArray  excluded_paths(_replica_job_create_detail.excluded_paths.begin(), _replica_job_create_detail.excluded_paths.end());
                replica_job_detail["excluded_paths"] = excluded_paths;
            }
            if (!_replica_job_create_detail.previous_excluded_paths.empty()){
                mArray  previous_excluded_paths(_replica_job_create_detail.previous_excluded_paths.begin(), _replica_job_create_detail.previous_excluded_paths.end());
                replica_job_detail["previous_excluded_paths"] = previous_excluded_paths;
            }
            mArray checksum_target;
            foreach(string_map::value_type &t, _replica_job_create_detail.checksum_target){
                mObject _target;
                _target["connection_id"] = t.first;
                _target["machine_id"] = t.second;
                checksum_target.push_back(_target);
            }
            replica_job_detail["checksum_target"] = checksum_target;

            job_config["replica_job_create_detail"] = replica_job_detail;
        }

        boost::filesystem::path temp = _config_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _config_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _config_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output config info.")).c_str());
    }
    catch (...){
    }
}

void transport_job::merge_histories(const std::vector<job_history> &histories, boost::posix_time::time_duration diff){
    macho::windows::auto_lock lock(_cs);
    foreach(job_history h, histories){
        LOG(LOG_LEVEL_DEBUG, L"Packer Message: (%s) - %s", macho::stringutils::convert_utf8_to_unicode(h.time).c_str(), macho::stringutils::convert_utf8_to_unicode(h.description).c_str());
        history_record::ptr _h = history_record::ptr(new history_record(h.time, h.state, h.error, h.description));
        _h->time += diff;
        _h->format = h.format;
        _h->is_display = h.is_display;
        _h->args = h.arguments;
        _h->is_display = h.is_display;
        bool exist = false;
        BOOST_REVERSE_FOREACH(history_record::ptr &p, _histories){
            if (p->time == _h->time && 0 == strcmp(p->description.c_str(), _h->description.c_str())){
                exist = true;
                break;
            }
        }
        if (!exist){
            LOG(LOG_LEVEL_DEBUG, L"Merged Message: (%s) - %s", macho::stringutils::convert_utf8_to_unicode(h.time).c_str(), macho::stringutils::convert_utf8_to_unicode(h.description).c_str());
            _histories.push_back(_h);
        }
    }
    std::sort(_histories.begin(), _histories.end(), history_record::compare());
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

void transport_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    macho::windows::auto_lock lock(_cs);
    _histories.push_back(history_record::ptr(new history_record(state, error, format, error ? true : !is_cdr_trigger())));
    LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(format.str()).c_str());
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

//void transport_job::record(saasame::transport::job_state::type state, int error, std::string description){
//    macho::windows::auto_lock lock(_cs);
//    _histories.push_back(history_record::ptr(new history_record(state, error, description)));
//    while (_histories.size() > _MAX_HISTORY_RECORDS)
//        _histories.erase(_histories.begin());
//}

bool transport_job::get_replica_job_create_detail(saasame::transport::replica_job_create_detail &detail){
    FUN_TRACE;
    bool result = false;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    int retry = 3;
    while (!result){
        mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                FUN_TRACE_MSG(boost::str(boost::wformat(L"get_replica_job_create_detail(%s)") % _id));
                saasame::transport::replica_job_create_detail _detail;
                op.client()->get_replica_job_create_detail(_detail, _machine_id, macho::stringutils::convert_unicode_to_ansi(_id));
                if (_detail.host.length() && _detail.disk_ids.size() > 0 && _detail.disks.size() == _detail.disk_ids.size()){
                    std::map<std::string, std::set<std::string>> missing_connections;
                    foreach(string_set_map::value_type &t, detail.carriers){
                        if (!_detail.carriers.count(t.first))
                            missing_connections[t.first] = t.second;
                    }
                    if (!missing_connections.empty()){
                        carrier_rw::remove_connections(missing_connections, _detail.timeout, _detail.is_encrypted);
                    }
                    detail = _detail;
                    registry reg;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"BufferSize"].exists() && (DWORD)reg[L"BufferSize"] > 0){
                            detail.buffer_size = (DWORD)reg[L"BufferSize"];
                        }
                    }
                    result = true;
                }
            }
            catch (saasame::transport::invalid_operation& op){
                LOG(LOG_LEVEL_ERROR, L"Invalid operation (0x%08x): %s", op.what_op, macho::stringutils::convert_ansi_to_unicode(op.why).c_str());
                if (op.what_op == saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND){
                    LOG(LOG_LEVEL_RECORD, L"Replica job %s will be removed.", _id.c_str());
                    thrift_connect<scheduler_serviceClient> connect(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (connect.open()){
                        connect.client()->remove_job("", macho::stringutils::convert_unicode_to_utf8(_id));
                        connect.close();
                    }
                    break;
                }
            }
            catch (TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
        retry--;
        if (result || 0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return _is_initialized ? true : result;
}

bool transport_job::is_replica_job_alive(){
    bool result = false;
    int port;
    FUN_TRACE;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (op.open()){
        try{
            if (!(result = op.client()->is_replica_job_alive(_machine_id, macho::stringutils::convert_unicode_to_ansi(_id)))){
                LOG(LOG_LEVEL_RECORD, L"Replica job %s will be removed.", _id.c_str());
                thrift_connect<scheduler_serviceClient> connect(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                if (connect.open()){
                    connect.client()->remove_job("", macho::stringutils::convert_unicode_to_utf8(_id));
                    connect.close();
                }
            }
        }
        catch (TException & tx){
            LOG(LOG_LEVEL_WARNING, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    return result;
}

bool transport_job::discard_snapshot(const std::string& snapshot_id){
    bool result = false;
    int port;
    FUN_TRACE;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if ( op.open()){
        try{
            if (result = op.client()->discard_snapshots(_machine_id, snapshot_id)){
                LOG(LOG_LEVEL_RECORD, L"Discard snapshots %s succeeded.", macho::stringutils::convert_ansi_to_unicode(snapshot_id).c_str());
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Discard snapshots %s failed.", macho::stringutils::convert_ansi_to_unicode(snapshot_id).c_str());
            }

        }
        catch (TException & tx){
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    return result;
}

bool transport_job::is_snapshot_ready(const std::string& snapshot_id){
    FUN_TRACE;
    bool result = false;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (op.open()){
        try{
            result = op.client()->check_snapshots(_machine_id, snapshot_id);
        }
        catch (TException & tx){
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }   
    return result;
}

bool transport_job::update_state(const saasame::transport::replica_job_detail &detail){
    FUN_TRACE;
    bool result = false;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    int retry = 3;
    while (!result && !_is_canceled){
        mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                FUN_TRACE_MSG(boost::str(boost::wformat(L"update_replica_job_state(%s)") % _id));
                op.client()->update_replica_job_state(_machine_id, detail);
                result = true;
                break;
            }
            catch (TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
        }
        retry--;
        if (0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return result;
}

trigger::vtr transport_job::get_triggers(){
    trigger::vtr triggers;
    triggers.push_back(trigger::ptr(new scheduler_trigger(boost::posix_time::microsec_clock::universal_time(), _latest_leave_time, _latest_enter_time, _create_job_detail.triggers)));
    foreach(saasame::transport::job_trigger t, _create_job_detail.triggers){
        trigger::ptr _t;
        switch (t.type){
            /*
            case saasame::transport::job_trigger_type::interval_trigger:{
                if (t.start.length() && t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else if (t.start.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else if (t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else{
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                break;
            }
            */
            case saasame::transport::job_trigger_type::runonce_trigger:{
                if (t.start.length()){
                    _t = trigger::ptr(new run_once_trigger(boost::posix_time::time_from_string(t.start)));
                }           
                else{
                    _t = trigger::ptr(new run_once_trigger());
                }
                break;
            }
        }
        if (_t)
            triggers.push_back(_t);
    }

    if (_is_continuous_data_replication){
        _enable_cdr_trigger = true;
        triggers.push_back(trigger::ptr(new continues_data_replication_trigger()));
    }
    else{
        _enable_cdr_trigger = false;
    }
    
    return triggers;
}

bool transport_job::_update(bool force){
    int  retry_count = RetryCount;
    bool result = true;
    while (retry_count > 0){
        if (result = __update(force))
            break;
        else if (_terminated)
            break;
        else
            boost::this_thread::sleep(boost::posix_time::seconds(10));
        retry_count--;
    }
    return result;
}

bool transport_job::__update(bool force){
    bool result = true;
    macho::windows::auto_lock lock(_lck);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    typedef std::map<std::string, std::string> string_map;
    foreach(string_map::value_type t, _replica_job_create_detail.targets){
        std::string job_id;
        if (!_latest_update_times.count(t.first))
            _latest_update_times[t.first] = _created_time;
        try{
            replica_job_detail update_detail = get_job_detail(_latest_update_times[t.first], t.first);
            update_detail.replica_id = t.second;
            update_detail.__set_backup_progress(update_detail.backup_progress);
            update_detail.__set_connection_id(update_detail.connection_id);
            update_detail.__set_created_time(update_detail.created_time);
            update_detail.__set_disks(update_detail.disks);
            update_detail.__set_host(update_detail.host);
            update_detail.__set_id(update_detail.id);
            update_detail.__set_is_error(update_detail.is_error);
            update_detail.__set_original_size(update_detail.original_size);
            update_detail.__set_replica_id(update_detail.replica_id);
            update_detail.__set_snapshot_mapping(update_detail.snapshot_mapping);
            update_detail.__set_state(update_detail.state);
            update_detail.__set_type(update_detail.type);
            update_detail.__set_virtual_machine_id(update_detail.virtual_machine_id);
            update_detail.__set_backup_image_offset(update_detail.backup_image_offset);
            update_detail.__set_backup_size(update_detail.backup_size);
            update_detail.__set_cbt_info(update_detail.cbt_info);
            update_detail.__set_snapshot_time(update_detail.snapshot_time);
            update_detail.__set_snapshot_info(update_detail.snapshot_info);
            boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
            update_detail.updated_time = boost::posix_time::to_simple_string(update_time);
            update_detail.__set_updated_time(update_detail.updated_time);
            update_detail.__set_histories(update_detail.histories);
            update_detail.__set_boot_disk(update_detail.boot_disk);
            update_detail.__set_is_cdr(update_detail.is_cdr);
            update_detail.__set_system_disks(update_detail.system_disks);
            update_detail.__set_excluded_paths(update_detail.excluded_paths);
            if (update_detail.histories.size() || force){
                if (!(result = update_state(update_detail)))
                    break;
                else{
                    _latest_update_times[t.first] = update_time;
                }
            }
        }
        catch (...){
            result = false;
        }
    }
    return result;
}

void transport_job::update(bool loop){
    FUN_TRACE;
    int count = 1;
    do {
        if (!_terminated && loop) 
            boost::this_thread::sleep(boost::posix_time::seconds(15));
        _update(_need_update);    
        if (_terminated){
            if (count){
                count--;
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
    } while (loop);
}

bool transport_job::save_status(mObject &job_status){
    FUN_TRACE;
    try{
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["is_initialized"] = _is_initialized;
        job_status["is_need_full_rescan"] = _is_need_full_rescan;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);
        job_status["latest_enter_time"] = boost::posix_time::to_simple_string(_latest_enter_time);
        job_status["latest_leave_time"] = boost::posix_time::to_simple_string(_latest_leave_time);
        job_status["mgmt_addr"] = _mgmt_addr;
        mArray latest_update_times;
        typedef std::map<std::string, boost::posix_time::ptime> string_ptime_map;
        foreach(string_ptime_map::value_type t, _latest_update_times){
            mObject o;
            o["key"] = t.first;
            o["value"] = boost::posix_time::to_simple_string(t.second);
            latest_update_times.push_back(o);
        }
        job_status["latest_update_times"] = latest_update_times;
        mArray histories;
        foreach(history_record::ptr &h, _histories){
            mObject o;
            o["time"] = boost::posix_time::to_simple_string(h->time);
            o["original_time"] = boost::posix_time::to_simple_string(h->original_time);
            o["state"] = (int)h->state;
            o["error"] = h->error;
            o["description"] = h->description;
            o["format"] = h->format;
            o["is_display"] = h->is_display;
            mArray  args(h->args.begin(), h->args.end());
            o["arguments"] = args;
            histories.push_back(o);
        }
        job_status["histories"] = histories;
        if (!_snapshot_info.empty())
            job_status["snapshot_info"] = _snapshot_info;
        job_status["boot_disk"] = _boot_disk;
        job_status["will_be_resumed"] = _will_be_resumed;
        mArray  system_disks(_system_disks.begin(), _system_disks.end());
        job_status["system_disks"] = system_disks;
        job_status["is_continuous_data_replication"] = _is_continuous_data_replication;
        job_status["is_pending_rerun"] = _is_pending_rerun;
        job_status["is_source_ready"] =  _is_source_ready;
        mArray packer_jobs;
        foreach(string_map::value_type p, _packer_jobs){
            mObject o;
            o["key"] = p.first;
            o["value"] = p.second;
            packer_jobs.push_back(o);
        }
        job_status["packer_jobs"] = packer_jobs;
        typedef std::map<std::string, boost::posix_time::time_duration> time_duration_map;
        mArray packer_time_diffs;
        foreach(time_duration_map::value_type p, _packer_time_diffs){
            mObject o;
            o["key"] = p.first;
            o["value"] = boost::posix_time::to_simple_string(p.second);
            packer_time_diffs.push_back(o);
        }
        job_status["packer_time_diffs"] = packer_time_diffs;

        mArray packer_previous_update_time;
        foreach(string_map::value_type p, _packer_previous_update_time){
            mObject o;
            o["key"] = p.first;
            o["value"] = p.second;
            packer_previous_update_time.push_back(o);
        }
        job_status["packer_previous_update_time"] = packer_previous_update_time;

        boost::filesystem::path temp = _status_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_status, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _status_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _status_file.wstring().c_str(), GetLastError());
            return false;
        }
        return true;
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output transport_job status info.")).c_str());
    }
    catch (...){
    }
    return false;
}

bool transport_job::load_status(const std::wstring status_file, mValue &job_status){
    FUN_TRACE;
    try{
        if (boost::filesystem::exists(status_file)){
            std::ifstream is(status_file);
            read(is, job_status);
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            _is_initialized = find_value(job_status.get_obj(), "is_initialized").get_bool();
            _is_need_full_rescan = find_value_bool(job_status.get_obj(), "is_need_full_rescan");
            _state = find_value(job_status.get_obj(), "state").get_int();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _latest_enter_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "latest_enter_time").get_str());  
            std::string latest_leave_time_str = find_value(job_status.get_obj(), "latest_leave_time").get_str();
            if ( 0 == latest_leave_time_str.length() || latest_leave_time_str == "not-a-date-time")
                _latest_leave_time = boost::date_time::not_a_date_time;
            else
                _latest_leave_time = boost::posix_time::time_from_string(latest_leave_time_str);
            mArray latest_update_times = find_value(job_status.get_obj(), "latest_update_times").get_array();
            foreach(mValue &t, latest_update_times){
                _latest_update_times[find_value_string(t.get_obj(), "key")] = boost::posix_time::time_from_string(find_value_string(t.get_obj(), "value"));
            }
            mArray histories = find_value(job_status.get_obj(), "histories").get_array();
            foreach(mValue &h, histories){
                std::vector<std::string> arguments;
                mArray args = find_value(h.get_obj(), "arguments").get_array();
                foreach(mValue a, args){
                    arguments.push_back(a.get_str());
                }
                _histories.push_back(history_record::ptr(new history_record(
                    find_value_string(h.get_obj(), "time"), 
                    find_value_string(h.get_obj(), "original_time"), 
                    (saasame::transport::job_state::type)find_value_int32(h.get_obj(), "state"),
                    find_value_int32(h.get_obj(), "error"), 
                    find_value_string(h.get_obj(), "description"),
                    find_value_string(h.get_obj(), "format"),
                    arguments,
                    find_value_bool(h.get_obj(), "is_display"))));
            }
            _snapshot_info = find_value_string(job_status.get_obj(), "snapshot_info");
            _boot_disk = find_value_string(job_status.get_obj(), "boot_disk");
            //_will_be_resumed = find_value_bool(job_status.get_obj(), "will_be_resumed");
            mArray  system_disks = find_value(job_status.get_obj(), "system_disks").get_array();
            foreach(mValue i, system_disks){
                _system_disks.push_back(i.get_str());
            }
            _is_continuous_data_replication = find_value_bool(job_status.get_obj(), "is_continuous_data_replication", false);
            _is_source_ready = find_value_bool(job_status.get_obj(), "is_source_ready", true);
            _mgmt_addr = find_value_string(job_status.get_obj(), "mgmt_addr");
            const mObject job_status_obj = job_status.get_obj();
            if (job_status_obj.find("packer_time_diffs") != job_status_obj.end()){
                mArray packer_time_diffs = find_value(job_status.get_obj(), "packer_time_diffs").get_array();
                foreach(mValue &t, packer_time_diffs){
                    _packer_time_diffs[find_value_string(t.get_obj(), "key")] = boost::posix_time::duration_from_string(find_value(t.get_obj(), "value").get_str());
                }
            }
            if (job_status_obj.find("packer_previous_update_time") != job_status_obj.end()){
                mArray packer_previous_update_time = find_value(job_status.get_obj(), "packer_previous_update_time").get_array();
                foreach(mValue &t, packer_previous_update_time){
                    _packer_previous_update_time[find_value_string(t.get_obj(), "key")] = find_value(t.get_obj(), "value").get_str();
                }
            }
            mArray packer_jobs = find_value(job_status.get_obj(), "packer_jobs").get_array();
            foreach(mValue &t, packer_jobs){
                _packer_jobs[find_value_string(t.get_obj(), "key")] = find_value(t.get_obj(), "value").get_str();
            }
            return true;
        }
        else{
            _created_time = boost::posix_time::microsec_clock::universal_time();
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read status info.")));
    }
    catch (...)
    {
    }
    return false;
}

std::string transport_job::scheduler_machine_id(){
    FUN_TRACE;
    std::string machine_id;
    thrift_connect<scheduler_serviceClient>  thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    try{
        if (thrift.open()){
            physical_machine_info machine_info;
            thrift.client()->get_host_detail(machine_info, "", machine_detail_filter::SIMPLE);
            if (machine_info.machine_id.length()){
                machine_id = machine_info.machine_id;
            }
        } 
    }
    catch (TException& tx) {
    }
    return machine_id;
}

bool transport_job::update_job_triggers(){
    FUN_TRACE;
    thrift_connect<scheduler_serviceClient>  thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    try{
        if (thrift.open()){
            return thrift.client()->update_job("", macho::stringutils::convert_unicode_to_utf8(_id), _create_job_detail);
        }
    }
    catch (TException& tx) {
    }
    return false;
}

bool transport_job::is_cdr_trigger(){
   return _trigger ? _trigger->name() == L"continues_data_replication_trigger" : false;
} 

bool transport_job::is_runonce_trigger(){
    return _trigger ? _trigger->name() == L"run_once_trigger" : false;
}
