#pragma once
#ifndef transport_job_H
#define transport_job_H

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"
#include "job.h"
#include "physical_packer_service.h"
#include "virtual_packer_service.h"
#include "service_op.h"
#include "physical_packer_service_proxy.h"

#define JOB_EXTENSION L".job"
#define _MAX_HISTORY_RECORDS 50

struct physical_transport_job_progress{
    typedef boost::shared_ptr<physical_transport_job_progress> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    physical_transport_job_progress() : number(0U), total_size(0ULL), backup_progress(0ULL), backup_size(0ULL), backup_image_offset(0ULL){}
    std::string                         disk_uri;
    std::string                         output;
    std::string                         base;
    std::string                         parent;
    std::string                         discard;
    uint64_t                            total_size;
    uint64_t                            backup_progress;
    uint64_t                            backup_size;
    uint64_t                            backup_image_offset;
    uint32_t                            number;
    std::string                         friendly_name;
    std::vector<io_changed_range>       cdr_changed_areas;
    std::vector<io_changed_range>       completed_blocks;
    macho::windows::critical_section    lock;
};

struct transport_jog_trigger{
    typedef boost::shared_ptr<transport_jog_trigger> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    transport_jog_trigger() : final_fire_time(boost::posix_time::microsec_clock::universal_time()), latest_finished_time(boost::posix_time::microsec_clock::universal_time()){}
    std::string               id;
    macho::trigger::ptr       t;
    boost::posix_time::ptime  final_fire_time;
    boost::posix_time::ptime  latest_finished_time;
};

class transport_job : public macho::removeable_job{
public:
    transport_job(std::wstring id, saasame::transport::create_job_detail detail);
    transport_job(saasame::transport::create_job_detail detail);
    typedef boost::shared_ptr<transport_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;
    static transport_job::ptr create(std::string id, saasame::transport::create_job_detail detail);
    static transport_job::ptr create(saasame::transport::create_job_detail detail);
    static transport_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file);
    static bool is_uuid(std::string uuid){
        try{
            guid_ u = uuid;
            return true;
        }
        catch (...){
        }
        return false;
    }
    bool virtual is_canceled() { return _is_interrupted || _is_canceled; }
    void virtual cancel() = 0;
    void virtual remove();
    trigger::vtr virtual get_triggers();
    saasame::transport::replica_job_detail virtual get_job_detail(const boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time(), const std::string connection_id = "") = 0;
    void virtual save_config();
    void virtual update_create_detail(const saasame::transport::create_job_detail& detail);
    void virtual update(bool loop);
    bool virtual prepare_for_job_removing() = 0;
    boost::posix_time::ptime get_latest_enter_time() const { return _latest_enter_time; }
    boost::posix_time::ptime get_latest_leave_time() const { return _latest_leave_time;  }
    bool is_cdr_trigger();
    bool is_runonce_trigger();
    void set_pending_rerun(){ _is_pending_rerun = true; save_status(); }
protected:
    struct running_job_state{
        running_job_state() : state(saasame::transport::job_state::type::job_state_none), is_error(false), disconnected(false){}
        saasame::transport::job_state::type state;
        bool                                is_error;
        bool                                disconnected;
    };
    bool virtual _update(bool force);
    bool virtual __update(bool force);
    bool virtual get_replica_job_create_detail(saasame::transport::replica_job_create_detail &detail);
    bool virtual is_snapshot_ready(const std::string& snapshot_id);
    bool virtual is_replica_job_alive();
    bool virtual discard_snapshot(const std::string& snapshot_id);
    bool virtual update_state(const saasame::transport::replica_job_detail &detail );
    //void virtual record(saasame::transport::job_state::type state, int error, std::string description);
    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);

    void virtual save_status() = 0;
    static saasame::transport::create_job_detail  load_config(std::wstring config_file, std::wstring &job_id);
    void virtual load_replica_create_job_detail(std::wstring config_file);
    bool virtual load_status(std::wstring status_file) = 0;   
    void virtual merge_histories(const std::vector<job_history> &histories, boost::posix_time::time_duration diff = boost::posix_time::seconds(0));

    bool save_status(mObject &job_status);
    bool load_status(const std::wstring status_file, mValue &job_status);
    std::string scheduler_machine_id();
    bool virtual update_job_triggers();
    bool _is_canceled;
    bool _is_interrupted;
    bool _is_initialized;
    bool _is_need_full_rescan;
    bool _is_continuous_data_replication;
    bool _is_pending_rerun;
    bool _enable_cdr_trigger;
    bool _is_source_ready;
    bool _need_update;
    int  _state;
    boost::filesystem::path                         _config_file;
    boost::filesystem::path                         _status_file;
    boost::posix_time::ptime                        _created_time;
    std::map<std::string,boost::posix_time::ptime>  _latest_update_times;
    saasame::transport::create_job_detail           _create_job_detail;
    saasame::transport::replica_job_create_detail   _replica_job_create_detail;
    history_record::vtr                             _histories;
    macho::windows::critical_section                _cs;
    macho::windows::critical_section                _lck; // for status update using
    macho::windows::critical_section                _config_lock; // for status update using
    bool                                                         _will_be_resumed;
    bool                                                         _terminated;
    boost::thread                                                _thread;
    boost::posix_time::ptime                                     _latest_enter_time;
    boost::posix_time::ptime                                     _latest_leave_time;
    std::map<std::string, boost::posix_time::time_duration>      _packer_time_diffs;
    std::map<std::string, std::string>                           _packer_previous_update_time;
    std::map<std::string, std::string>                           _packer_jobs;
    std::string                                                  _snapshot_info;
    std::string                                                  _boot_disk;
    std::vector<std::string>                                     _system_disks;
private:
    std::string                                                  _mgmt_addr;
    std::string                                                  _machine_id;
};

class physical_transport_job : public transport_job{

public:
    physical_transport_job(std::wstring id, saasame::transport::create_job_detail detail) : transport_job(id, detail){}
    physical_transport_job(saasame::transport::create_job_detail detail) : transport_job(detail){}
    virtual ~physical_transport_job(){}
    void virtual save_status();
    bool virtual load_status(std::wstring status_file);
    virtual void interrupt();
    void virtual remove();
    void virtual cancel();
    virtual void execute();
    virtual bool need_to_be_resumed();
    bool virtual prepare_for_job_removing();

    saasame::transport::replica_job_detail virtual get_job_detail(const boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time(), const std::string connection_id = "");
protected:
    class physical_packer_job_op{
    public:
        physical_packer_job_op(const std::set<std::string> addrs, const std::set<std::string> mgmt_addrs) :_addrs(addrs), _mgmt_addrs(mgmt_addrs){
        }
        bool initialize(std::string &priority_packer_addr);
        bool enumerate_disks(std::set<saasame::transport::disk_info>& disks);
        bool take_snapshots(std::vector<snapshot>& snapshots, const saasame::transport::take_snapshots_parameters& parameter);
        bool delete_snapshot_set(const std::string snapshot_set_id, delete_snapshot_result& _delete_snapshot_result);
        bool get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots);
        bool get_packer_job_detail(const std::string job_id, const boost::posix_time::ptime previous_updated_time, saasame::transport::packer_job_detail &detail);

        bool create_packer_job(const std::set<std::string> connections,
            const replica_job_create_detail&,
            const physical_create_packer_job_detail&,
            const std::set<std::string>&,
            const std::set<std::string>&,
            std::string& job_id,
            boost::posix_time::ptime& created_time,
            saasame::transport::job_type::type type);

        bool remove_packer_job(const std::string job_id);
        bool resume_packer_job(const std::string job_id);
        bool interrupt_packer_job(const std::string job_id);
        bool is_packer_job_running(const std::string job_id);
        bool is_packer_job_existing(const std::string job_id);
        bool can_connect_to_carriers(std::map<std::string, std::set<std::string> >& carriers, bool is_ssl);
        bool can_connect_to_carriers(std::map<std::string, std::string>& carriers, bool is_ssl);
        std::string get_machine_id();
        bool is_cdr_support();
    private:
        ULONGLONG get_version(std::string version_string);
        std::set<std::string>                                       _addrs;
        std::set<std::string>                                       _mgmt_addrs;
        thrift_connect<physical_packer_serviceClient>::ptr          _packer;
        thrift_connect<physical_packer_service_proxyClient>::ptr    _proxy;
        std::string                                                 _proxy_addr;
        std::string                                                 _session;
        saasame::transport::service_info                            _svc_info;
    };

    // Internal functions for operations
    void                                                         get_previous_journals(const std::string &cbt_info);
    bool                                                         remove_packer_jobs(physical_packer_job_op &op, bool erase_all );
    bool                                                         resume_packer_jobs(physical_packer_job_op &op);
    bool                                                         remove_snapshots(physical_packer_job_op &op);
    bool                                                         is_packer_job_running(physical_packer_job_op &op);
    bool                                                         packer_jobs_exist(physical_packer_job_op &op);

    bool                                                         update_packer_job_progress(packer_job_detail &packer_detail);
    //
    physical_transport_job_progress::map                         _progress;
    std::vector<saasame::transport::snapshot>                    _snapshots;
    std::map<std::string, running_job_state>                     _jobs_state;
    std::map<std::string, boost::posix_time::ptime>              _packers_update_time;
    std::map<int64_t, saasame::transport::physical_vcbt_journal> _previous_journals;
    std::map<int64_t, saasame::transport::physical_vcbt_journal> _cdr_journals;
    std::string                                                  _optimize_file_name( const std::string& name );
    std::string                                                  _priority_packer_addr;
    boost::posix_time::ptime                                     _latest_connect_packer_time;
    std::set<std::string>                                        _excluded_paths;
    std::set<std::string>                                        _resync_paths;
};

struct virtual_transport_job_progress
{
    typedef boost::shared_ptr<virtual_transport_job_progress> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    virtual_transport_job_progress() :
        total_size(0ULL),
        backup_progress(0ULL),
        backup_size(0ULL),
        real_backup_size(0ULL),
        backup_image_offset(0ULL),
        error_what(0){}
    std::string                         device_uuid;
    std::string                         device_name;
    std::string                         device_key;
    std::string                         output;
    std::string                         base;
    std::string                         parent;
    std::string                         discard;
    uint64_t                            total_size;
    uint64_t                            backup_progress;
    uint64_t                            backup_size;
    uint64_t                            real_backup_size;
    uint64_t                            backup_image_offset;
    std::vector<io_changed_range>       completed_blocks;
    macho::windows::critical_section    lock;
    int32_t                             error_what;
    std::string                         error_why;
};

#ifndef IRM_TRANSPORTER

class virtual_transport_job : public transport_job
{
public:
    virtual_transport_job(std::wstring id, saasame::transport::create_job_detail detail) : transport_job(id, detail), _full_replica(false), _self_enable_cbt(false), _cbt_config_enabled(false), _is_windows(false), _has_changed(false){}
    virtual_transport_job(saasame::transport::create_job_detail detail) : transport_job(detail), _full_replica(false), _self_enable_cbt(false), _cbt_config_enabled(false), _is_windows(false), _has_changed(false){}
    virtual ~virtual_transport_job(){}
    void virtual save_status();
    bool virtual load_status(std::wstring status_file);
    virtual void interrupt();
    void virtual cancel();
    virtual void execute();
    virtual void remove();
    virtual bool need_to_be_resumed();
    bool virtual prepare_for_job_removing();

    virtual saasame::transport::replica_job_detail get_job_detail(const boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time(), const std::string connection_id = "");

private:
    bool find_snapshot_in_tree(const std::vector<vmware_virtual_machine_snapshots::ptr>& child, const std::wstring& snapshot_name);
    class virtual_packer_job_op
    {
    public:
        virtual_packer_job_op(const std::set<std::string> addrs) :_addrs(addrs) {}

        bool initialize();
        bool get_packer_job_detail(const std::string job_id, const boost::posix_time::ptime previous_updated_time, saasame::transport::packer_job_detail &detail);
        bool create_packer_job(const std::set<std::string> connections,
            const replica_job_create_detail&,
            const virtual_create_packer_job_detail&,
            std::string& job_id, 
            boost::posix_time::ptime& created_time);
        bool remove_packer_job(const std::string job_id);
        bool resume_packer_job(const std::string job_id);
        bool interrupt_packer_job(const std::string job_id);
        bool is_packer_job_running(const std::string job_id);
        bool is_packer_job_existing(const std::string job_id);
        bool can_connect_to_carriers(std::map<std::string, std::set<std::string> >& carriers, bool is_ssl);
        std::string get_machine_id();
    private:
        std::set<std::string>                            _addrs;
        std::string                                      _host;
        std::shared_ptr<TSocket>                         _socket;
        std::shared_ptr<TSSLSocket>                      _ssl_socket;
        std::shared_ptr<TTransport>                      _transport;
        std::shared_ptr<TProtocol>                       _protocol;
        std::shared_ptr<virtual_packer_serviceClient>    _client;
        std::string                                      _session;
        std::shared_ptr<TSSLSocketFactory>               _factory;
    };
    bool    packer_jobs_exist(virtual_packer_job_op &op);
    bool    is_packer_job_running(virtual_packer_job_op &op);
    bool    remove_snapshot(vmware_portal_::ptr portal, std::wstring& snapshot_name);
    bool    resume_packer_jobs(virtual_packer_job_op &op);
    bool    remove_packer_jobs(virtual_packer_job_op &op, bool erase_all);
    void                                                         get_previous_change_ids(const std::string &cbt_info);

    std::map<std::string, std::string>                          _previous_change_ids;
    virtual_transport_job_progress::map                         _progress;
    std::map<std::string, running_job_state>  _jobs_state;
    bool                                                        _self_enable_cbt;
    bool                                                        _cbt_config_enabled;
    bool                                                        _full_replica;
    bool                                                        _is_windows;
    bool                                                        _has_changed;
    std::wstring                                                _wsz_virtual_machine_id;
    std::wstring                                                _wsz_current_snapshot_name;
    std::string                                                 _snapshot_time_stamp;
    std::string                                                 _snapshot_mor_ref;
    std::map<std::string, boost::posix_time::ptime>             _packers_update_time;
    mwdc::ironman::hypervisor_ex::vmware_portal_ex              _portal_ex;
    macho::windows::critical_section                            _op_lock;
    mwdc::ironman::hypervisor::hv_connection_type               _conn_type;
};

class winpe_transport_job : public physical_transport_job{
public:
    winpe_transport_job(std::wstring id, saasame::transport::create_job_detail detail) : physical_transport_job(id, detail){}
    winpe_transport_job(saasame::transport::create_job_detail detail) : physical_transport_job(detail){}
    virtual ~winpe_transport_job(){}
    virtual void execute();
private:

};

#endif

#endif