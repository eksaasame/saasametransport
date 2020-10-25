#pragma once
#ifndef loader_job_H
#define loader_job_H

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"
#include "job.h"
#include "universal_disk_rw.h"
#include "..\gen-cpp\webdav.h"
#include <queue>
#include <limits.h>
#include "..\irm_imagex\irm_imagex.h"
#include "json_spirit.h"

#define JOB_EXTENSION L".ljob"
#define CASCADING_JOB_EXTENSION L".xjob"

#define _MAX_HISTORY_RECORDS 50

class loader_service_handler;
class block_file {
public:
    typedef boost::shared_ptr<block_file> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::deque<ptr> queue;
    struct compare {
        bool operator() (block_file::ptr const & lhs, block_file::ptr const & rhs) const {
            return (*lhs).index < (*rhs).index;
        }
    };
    block_file() : index(0), data(0), data_end(0), duplicated(false), transport_size(0){}
    std::wstring                   name;
    uint64_t                       index;
    uint64_t                       data_end;
    uint32_t                       data;
    macho::windows::lock_able::ptr lock;
    bool                           duplicated;
    uint32_t                       transport_size;
};

enum connection_op_type{
    local,
    winnet,
    remote
};

class connection_op{
public:
    typedef boost::shared_ptr<connection_op> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    static connection_op::ptr               get(const std::string connection_id, int workthread = 1);
    static connection_op::ptr               get(const saasame::transport::connection &conn, int workthread = 1);
    static connection_op::ptr               get_cascading(const saasame::transport::connection &conn, int workthread = 1);
    static bool                             remove(const std::string connection_id);
    virtual bool                            remove_file(std::wstring filename) = 0;
    virtual bool                            is_file_ready(std::wstring filename) = 0;
    virtual bool                            read_file(std::wstring filename, std::ifstream& ifs) = 0;
    //virtual bool                            read_file(std::wstring filename, std::stringstream& data) = 0;
    virtual bool                            read_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data, uint32_t& size, connection_op::ptr& cascading = connection_op::ptr()) = 0;
    virtual bool                            write_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data) = 0;
    virtual bool                            write_file(std::wstring filename, std::istream& data) = 0;
    virtual connection_op_type              type() = 0;
    virtual std::wstring                    path() const = 0;
    virtual macho::windows::lock_able::ptr  get_lock_object(std::wstring filename, std::string flags = "l") = 0;
    virtual connection_op::ptr              clone() = 0;
    virtual bool is_canceled() { return _is_canceled(); }
    typedef boost::signals2::signal<bool(), macho::_or<bool>> be_canceled;
    inline void register_is_cancelled_function(be_canceled::slot_type slot){
        _is_canceled.connect(slot);
    }
protected:
    be_canceled                             _is_canceled;
    saasame::transport::connection          _conn;
    connection_op(const saasame::transport::connection& conn) : _conn(conn){}
};

struct loader_disk{
    typedef boost::shared_ptr<loader_disk> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    loader_disk(std::string _key) : key(_key),progress(0), data(0), journal_index(-1), completed(false), duplicated_data(0), transport_data(0), canceled(false), cdr(false){}
    macho::windows::critical_section cs;
    uint64_t                         progress;
    uint64_t                         data;
    uint64_t                         duplicated_data;
    uint64_t                         transport_data;
    uint64_t                         journal_index;
    block_file::queue                completed_blocks;
    bool                             completed;
    bool                             canceled;
    block_file::vtr                  purgging;
    bool                             cdr;
    std::string                      key;
};

class loader_job : public macho::removeable_job{
public:
    struct exception : public exception_base {};

#define BOOST_THROW_LOADER_EXCEPTION(message) BOOST_THROW_EXCEPTION_BASE_STRING( exception, message)    

    loader_job(loader_service_handler& handler, std::wstring id, saasame::transport::create_job_detail detail);
    loader_job(loader_service_handler& handler, saasame::transport::create_job_detail detail);
    typedef boost::shared_ptr<loader_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;

    static loader_job::ptr create(loader_service_handler& handler, std::string id, saasame::transport::create_job_detail detail);
    static loader_job::ptr create(loader_service_handler& handler, saasame::transport::create_job_detail detail);
    static loader_job::ptr load(loader_service_handler& handler, boost::filesystem::path config_file, boost::filesystem::path status_file);
    void virtual cancel(){ _is_canceled = true; interrupt(); }
    void virtual resume();
    void virtual remove();
    void virtual update_create_detail(const saasame::transport::create_job_detail& detail);
    saasame::transport::loader_job_detail virtual get_job_detail(bool complete = false);
    void virtual save_config();
    virtual void interrupt();
    virtual void execute();
    trigger::vtr virtual get_triggers();
    boost::posix_time::ptime get_latest_leave_time() const { return _latest_leave_time; }

protected:
    saasame::transport::cascading load_cascadings(const json_spirit::mObject& cascadings);
    json_spirit::mObject save_cascadings(const saasame::transport::cascading& cascadings);
    bool virtual get_loader_job_create_detail(saasame::transport::loader_job_create_detail &detail);
    virtual bool is_canceled() { return _is_canceled || _is_interrupted; }
    bool check_snapshot(std::string snapshot_id);
    bool discard_snapshot(std::string snapshot_id);
    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);
    void virtual save_status();
    void virtual load_create_job_detail(std::wstring config_file);
    bool virtual load_status(std::wstring status_file);
    bool virtual load_progress(std::wstring progress_file);
    void virtual save_progress(loader_disk& progress);
    virtual void modify_job_scheduler_interval(uint32_t minutes = INT_MAX);
    void virtual save_config(json_spirit::mObject& job_config);

    boost::filesystem::path                         _config_file;
    boost::filesystem::path                         _status_file;
    boost::filesystem::path                         _progress_file;
    boost::posix_time::ptime                        _created_time;
    boost::posix_time::ptime                        _latest_update_time;
    saasame::transport::create_job_detail           _create_job_detail;
    saasame::transport::loader_job_create_detail    _loader_job_create_detail;
    loader_disk::map                                _progress_map;
    std::string                                     _current_snapshot;
    std::string                                     _machine_id;
    bool                                            _is_initialized;
    int                                             _get_loader_job_create_detail_error_count;
    int                                             _state;
    bool                                            _terminated;
    bool                                            _is_replicating;
    bool                                            _is_cdr;
    boost::posix_time::ptime                        _latest_enter_time;
    boost::posix_time::ptime                        _latest_leave_time;
    std::string                                     _mgmt_addr;
    boost::mutex                                    _wait_lock;
    boost::condition_variable                       _wait_cond;
    bool                                            _is_continuous_data_replication;
    bool                                            _enable_cdr_trigger;
    loader_service_handler&                         _handler;
    macho::windows::critical_section                _config_lock; // for status update using
    history_record::vtr                             _histories;
    macho::windows::critical_section                _cs;
    bool                                            _is_canceled;
    bool                                            _is_interrupted;
    std::map<std::string, bool>                     _sent_messages;
    
    template <class JOB, class TASK>
    void _execute(
        std::string name,
        connection_op::ptr conn_ptr,
        loader_disk::ptr disk_progress,
        universal_disk_rw::ptr rw,
        bool is_canceled_snapshot,
        uint64_t size,
        connection_op::ptr cascading_ptr,
        std::vector<std::wstring>& parallels,
        std::vector<std::wstring>& branches, 
        bool is_cascading_job);

private:
    void get_cascadings_info(saasame::transport::cascading& cascadings, connection_op::ptr& cascading_ptr, std::vector<std::wstring>& parallels, std::vector<std::wstring>& branches, std::map<std::wstring,std::wstring>& jobs);
    void execute_snapshot_post_script();
    void intrrupt_unused_cascading_jobs();
    void intrrupt_unused_cascading_jobs(std::map<std::wstring, std::wstring>& jobs);
    bool virtual update_state(const saasame::transport::loader_job_detail &detail);
    bool take_snapshot( std::string snapshot_id );
    bool mount_devices();
    bool dismount_devices();
    bool is_devices_ready();
    void disable_auto_mount_disk_devices();
    static saasame::transport::create_job_detail  load_config(std::wstring config_file, std::wstring &job_id);

    void                                          update();
    bool                                          _update( bool force );
    bool                                          __update(bool force);
};

class cascading_job : virtual public loader_job{
public:
    typedef boost::shared_ptr<cascading_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;

    cascading_job(loader_service_handler& handler, std::wstring id, std::wstring machine_id, std::wstring loader_job_id, saasame::transport::create_job_detail detail);
    static cascading_job::ptr create(loader_service_handler& handler, std::string id, std::string machine_id, std::string loader_job_id, saasame::transport::create_job_detail detail);
    static cascading_job::ptr load(loader_service_handler& handler, boost::filesystem::path config_file, boost::filesystem::path status_file);
    virtual void execute();
    void virtual remove();
private:
    void earse_data();
    void get_cascadings_info(saasame::transport::cascading& cascadings, connection_op::ptr& conn_ptr, connection_op::ptr& cascading_ptr, std::vector<std::wstring>& parallels, int worker_thread_number = 1);
    static saasame::transport::create_job_detail load_config(std::wstring config_file, std::wstring &job_id, std::wstring &machine_id, std::wstring &loader_job_id);
};
#endif