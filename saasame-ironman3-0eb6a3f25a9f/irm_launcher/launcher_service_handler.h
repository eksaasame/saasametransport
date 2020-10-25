
#include "launcher_service.h"
#include "service_proxy_handler.h"
#include "management_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "launcher_job.h"
#include "..\buildenv.h"
#include "service_proxy_handler.h"
#include "..\gen-cpp\mgmt_op.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class launcher_service_handler : virtual public service_proxy_handler, virtual public common_service_handler, virtual public launcher_serviceIf {
public:

    launcher_service_handler(std::wstring session_id) {
        _session = macho::guid_(session_id);
        _scheduler.start();
        load_jobs();
    }
    virtual ~launcher_service_handler(){ suspend_jobs(); }

    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.LAUNCHER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }

    void create_job_ex(launcher_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job){
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        launcher_job::ptr new_job;
        try{
            new_job = launcher_job::create(job_id, create_job);
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
                new_job->register_job_to_be_executed_callback_function(boost::bind(&launcher_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&launcher_service_handler::job_was_executed_callback, this, _1, _2, _3));
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

    void create_job(launcher_job_detail& _return, const std::string& session_id, const create_job_detail& create_job) {
        VALIDATE;
        FUN_TRACE;
        macho::windows::auto_lock lock(_lock);
        if (!create_job.triggers.size()){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
            error.why = "Doesn't have any scheduler trigger for job.";
            throw error;
        }

        launcher_job::ptr new_job = launcher_job::create(create_job);
        if (!new_job){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_JOB_CREATE_FAIL;
            error.why = "Create job failure";
            throw error;
        }
        else{
            if (!_scheduled.count(new_job->id())){
                new_job->register_job_to_be_executed_callback_function(boost::bind(&launcher_service_handler::job_to_be_executed_callback, this, _1, _2));
                new_job->register_job_was_executed_callback_function(boost::bind(&launcher_service_handler::job_was_executed_callback, this, _1, _2, _3));        
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

    void get_job(launcher_job_detail& _return, const std::string& session_id, const std::string& job_id) {
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
                _scheduler.update_job_triggers(id, triggers);
            else
                _scheduler.schedule_job(_scheduled[id], triggers);
        }
        return true;
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

    void list_jobs(std::vector<launcher_job_detail> & _return, const std::string& session_id) {
        macho::windows::auto_lock lock(_lock);
        FUN_TRACE;
        foreach(launcher_job::map::value_type& j, _scheduled){
            _return.push_back(j.second->get_job_detail(false));
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

    bool verify_management(const std::string& management, const int32_t port, const bool is_ssl){
        bool result = false;
        FUN_TRACE;
        mgmt_op op({ management }, std::string(), port, is_ssl);
        result = op.open();
        return result;
    }

    bool unregister(const std::string& session){
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MgmtAddr"].exists()){
                reg[L"SessionId"].delete_value();
                reg[L"MgmtAddr"].delete_value();
                reg[L"AllowMultiple"].delete_value();           
                if (macho::windows::environment::is_winpe()){
                    std::wstring cfgfile = macho::windows::environment::get_execution_full_path() + L".cfg";
                    if (boost::filesystem::exists(cfgfile))
                        boost::filesystem::remove(cfgfile);
                    process::exec_console_application_without_wait(L"wpeutil reboot");
                }
                else{
                    process::exec_console_application_without_wait(boost::str(boost::wformat(L"\"%s\" --restart") % macho::windows::environment::get_execution_full_path()));
                }
                return true;
            }
        }
        return false;
    }

private:
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
                        launcher_job::ptr job = launcher_job::load(dir_iter->path(), dir_iter->path().wstring() + L".status");
                        if (!_scheduled.count(job->id())){
                            job->register_job_to_be_executed_callback_function(boost::bind(&launcher_service_handler::job_to_be_executed_callback, this, _1, _2));
                            job->register_job_was_executed_callback_function(boost::bind(&launcher_service_handler::job_was_executed_callback, this, _1, _2, _3));
                            if (job->is_uploading())
                                _scheduler.schedule_job(job, job->get_triggers());
                            _scheduled[job->id()] = job;
                        }
                    }
                }
            }
        }
    }

    void suspend_jobs(){
        macho::windows::auto_lock lock(_lock);
        foreach(launcher_job::map::value_type& j, _scheduled){
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
    launcher_job::map                           _scheduled;
    macho::windows::critical_section            _lock;
};