
#pragma once

#ifndef scheduler_service_handler_H
#define scheduler_service_handler_H

#include "macho.h"
#include "scheduler_service.h"
#include "common_service_handler.h"
#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "transport_job.h"
#include "..\buildenv.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace macho::windows;
using namespace macho;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class scheduler_service_handler : virtual public common_service_handler, virtual public scheduler_serviceIf {
private:
    class scheduler_job : public macho::job{
    public:
        scheduler_job() : job(L"scheduler_job", L""){}
        virtual void execute(){}
    };
    class removing_job : public macho::job{
    public:
        removing_job() : job(L"removing_job", L""){}
        virtual void execute(){}
    };

    class time_sync_job : public macho::job{
    public:
        time_sync_job() : job(L"time_sync_job", L""){}
        virtual void execute();
    private:
        boost::posix_time::ptime  _latest_updated_time;
    };
public:
    void job_to_be_executed_callback2(const macho::trigger::ptr& job, const macho::job_execution_context& ec);
    void removing_job_to_be_executed_callback(const macho::trigger::ptr& job, const macho::job_execution_context& ec);

    scheduler_service_handler(std::wstring session_id, int workers = 0) : _scheduler(workers){
        _session = macho::guid_(session_id);
#ifndef IRM_TRANSPORTER
        macho::job::ptr time_sync(new time_sync_job());
        macho::trigger::vtr triggers;
        triggers.push_back(macho::trigger::ptr(new macho::interval_trigger(boost::posix_time::minutes(1), 10, boost::posix_time::microsec_clock::universal_time() + boost::posix_time::minutes(1))));
        triggers.push_back(macho::trigger::ptr(new macho::interval_trigger(boost::posix_time::hours(24), 0, boost::posix_time::microsec_clock::universal_time() + boost::posix_time::minutes(1))));
        _scheduler.schedule_job(time_sync, triggers);
#endif
        macho::job::ptr j(new scheduler_job());
        j->register_job_to_be_executed_callback_function(boost::bind(&scheduler_service_handler::job_to_be_executed_callback2, this, _1, _2));
        _scheduler.schedule_job(j, macho::interval_trigger(boost::posix_time::minutes(1)));
        macho::job::ptr removing_j(new removing_job());
        removing_j->register_job_to_be_executed_callback_function(boost::bind(&scheduler_service_handler::removing_job_to_be_executed_callback, this, _1, _2));
        _scheduler.schedule_job(removing_j, macho::interval_trigger(boost::posix_time::minutes(1)));
        load_jobs();
        _scheduler.start();  
    }
    virtual ~scheduler_service_handler(){ }

    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.SCHEDULER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }
    
    void get_physical_machine_detail(physical_machine_info& _return, const std::string& session_id, const std::string& host, const machine_detail_filter::type filter);

    void get_physical_machine_detail2(physical_machine_info& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const machine_detail_filter::type filter) {
        // Your implementation goes here
        FUN_TRACE;
    }

    void get_packer_service_info(service_info& _return, const std::string& session_id, const std::string& host);

    void get_virtual_host_info(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password);

    void get_virtual_machine_detail(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    
    void create_job_ex(replica_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job){
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        if (!create_job.triggers.size()){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
            error.why = "Doesn't have any scheduler trigger for job.";
            throw error;
        }
        transport_job::ptr new_job;
        try{
            new_job = transport_job::create(job_id, create_job);
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
                new_job->register_job_to_be_executed_callback_function(boost::bind(&scheduler_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&scheduler_service_handler::job_was_executed_callback, this, _1, _2, _3));
                _scheduler.schedule_job(new_job, new_job->get_triggers());
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

    void create_job(replica_job_detail& _return, const std::string& session_id, const create_job_detail& create_job) {
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        if (!create_job.triggers.size()){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
            error.why = "Doesn't have any scheduler trigger for job.";
            throw error;
        }

        transport_job::ptr new_job = transport_job::create(create_job);
        if (!new_job){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
            error.why = "Create job failure";
            throw error;
        }
        else{
            if (!_scheduled.count(new_job->id())){
                new_job->register_job_to_be_executed_callback_function(boost::bind(&scheduler_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&scheduler_service_handler::job_was_executed_callback, this, _1, _2, _3));
                _scheduler.schedule_job(new_job, new_job->get_triggers());
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

    void get_job(replica_job_detail& _return, const std::string& session_id, const std::string& job_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
        std::wstring id = macho::guid_(job_id);
        if (!_scheduled.count(id)){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND;
            error.why = boost::str(boost::format("Job '%1%' is not found.") % job_id);
            throw error;
        }
        _return = _scheduled[id]->get_job_detail(boost::posix_time::time_from_string("2009-02-23 15:30:0.000"));
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
        if (_scheduled[id]->is_canceled())
            return false;
        else if (!_scheduler.is_scheduled(id)){
            _scheduled[id]->set_pending_rerun();
            _scheduler.schedule_job(_scheduled[id], _scheduled[id]->get_triggers());
            return true;
        }
        else{
            if (!_scheduler.is_running(id)){
                _scheduled[id]->set_pending_rerun();
                trigger::vtr triggers = _scheduled[id]->get_triggers();
                triggers.push_back(trigger::ptr(new run_once_trigger()));
                _scheduler.update_job_triggers(id, triggers);
                return true;
            }
            else if (_scheduled[id]->is_cdr_trigger()){
                _scheduled[id]->set_pending_rerun();
                trigger::vtr triggers = _scheduled[id]->get_triggers();
                triggers.push_back(trigger::ptr(new run_once_trigger()));
                _scheduler.update_job_triggers(id, triggers);
                return true;
            }
        }
        return false;
    }

    bool remove_job(const std::string& session_id, const std::string& job_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
        interrupt_job(session_id, job_id);
        std::wstring id = macho::guid_(job_id);
        if (_scheduled.count(id)){
            _removing[id] = _scheduled[id];
            _scheduled.erase(id);
            save_removing_jobs();
        }
        return true;
    }

    void list_jobs(std::vector<replica_job_detail> & _return, const std::string& session_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
        foreach(transport_job::map::value_type& j, _scheduled){
            _return.push_back(j.second->get_job_detail());
        }
    }

    bool update_job(const std::string& session_id, const std::string& job_id, const create_job_detail& job) {
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
    bool verify_management(const std::string& management, const int32_t port, const bool is_ssl);
    bool verify_packer_to_carrier(const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl);
    void take_packer_xray(std::string& _return_data, const std::string& session_id, const std::string& host){
        _return_data = common_service_handler::get_xray(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
    }

private:
    void load_jobs();
    void suspend_jobs();
#ifndef IRM_TRANSPORTER
    virtual_machine_snapshots get_virtual_machine_snapshot_from_object(vmware_virtual_machine_snapshots::ptr child);
#endif
    void job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec){
        macho::windows::auto_lock lock(_lock);

    }
    void job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex){
        macho::windows::auto_lock lock(_lock);

    }
    void wait_jobs_completion()
    {
        foreach(transport_job::map::value_type& j, _scheduled)
        {
            while (_scheduler.is_running(j.second->id()))
                boost::this_thread::sleep(boost::posix_time::seconds(2));
        }
    }
    void save_removing_jobs();
    void load_removing_jobs();
    macho::guid_                                _session;
    macho::scheduler                            _scheduler;
    transport_job::map                          _scheduled;
    transport_job::map                          _removing;
    macho::windows::critical_section            _lock;
};

#endif