#pragma once

#ifndef physical_packer_service_handler_H
#define physical_packer_service_handler_H

#include "boost/filesystem.hpp"
#include "physical_packer_service.h"
#include "common_service_handler.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "physical_packer_job.h"
#include "vss_snapshot.h"
#include "..\buildenv.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace macho::windows;
using namespace macho;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class physical_packer_service_handler : virtual public common_service_handler, virtual public physical_packer_serviceIf {
public:
    physical_packer_service_handler(std::wstring session_id) {
        _session = macho::guid_(session_id);
        _scheduler.start();
        load_jobs();
    }

    virtual ~physical_packer_service_handler(){ suspend_jobs(); }

    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }

    virtual bool verify_carrier(const std::string& carrier, const bool is_ssl);

    void take_snapshots(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks);
    void take_snapshots_ex(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script);
    void take_snapshots2(std::vector<snapshot> & _return, const std::string& session_id, const take_snapshots_parameters& parameters);

    void delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_id);

    void delete_snapshot_set(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_set_id);

    void get_all_snapshots(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id);

    void create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job){
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        physical_packer_job::ptr new_job;
        try{
            new_job = physical_packer_job::create(job_id, create_job);
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
                new_job->register_job_to_be_executed_callback_function(boost::bind(&physical_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&physical_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));

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
        physical_packer_job::ptr new_job = physical_packer_job::create(create_job);
        if (!new_job){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
            error.why = "Create job failure";
            throw error;
        }
        else{
            if (!_scheduled.count(new_job->id())){
                new_job->register_job_to_be_executed_callback_function(boost::bind(&physical_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&physical_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
                
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
            _scheduled[id]->resume();
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
        foreach(physical_packer_job::map::value_type& j, _scheduled){
            _return.push_back(j.second->get_job_detail());
        }
    }

    void terminate(const std::string& session_id) {
        // Your implementation goes here
        printf("exit\n");
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
    
    bool unregister(const std::string& session){
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MgmtAddr"].exists()){
                reg[L"SessionId"].delete_value();
                reg[L"MgmtAddr"].delete_value();
                reg[L"AllowMultiple"].delete_value();
                process::exec_console_application_without_wait(boost::str(boost::wformat(L"\"%s\" --restart") % macho::windows::environment::get_execution_full_path()));
                return true;
            }
        }
        return false;
    }
    virtual void take_xray(std::string& _return);

private:

    void send_vcbt_command(DWORD ioctl, std::vector<macho::guid_> &volumes, std::vector<macho::guid_>& snapshot_results = std::vector<macho::guid_>());

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
                        physical_packer_job::ptr job = physical_packer_job::load(dir_iter->path(), dir_iter->path().wstring() + L".status");
                        if (!_scheduled.count(job->id())){
                            job->register_job_to_be_executed_callback_function(boost::bind(&physical_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                            job->register_job_was_executed_callback_function(boost::bind(&physical_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
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
        foreach(physical_packer_job::map::value_type& j, _scheduled){
            j.second->interrupt();
        }
    }

    void job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec){
        macho::windows::auto_lock lock(_lock);

    }
    void job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex){
        macho::windows::auto_lock lock(_lock);

    }

    macho::guid_                                _session;
    macho::scheduler                            _scheduler;
    physical_packer_job::map                    _scheduled;
    macho::windows::critical_section            _lock;
    irm_vss_snapshot                            _req;
};

#endif