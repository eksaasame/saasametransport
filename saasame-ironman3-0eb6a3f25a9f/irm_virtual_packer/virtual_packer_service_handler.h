
#include "virtual_packer_service.h"
#include "common_service_handler.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "virtual_packer_job.h"

#include "..\buildenv.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using boost::shared_ptr;
using namespace  ::saasame::transport;

struct snapshots_management{
    snapshots_management() : tasks(20), terminated(false), timeout(60), initialized(false){
    }
    int													      timeout;
    macho::windows::critical_section                          cs;
    boost::mutex                                              lock;
    boost::condition_variable                                 cond;
    macho::scheduler                                          tasks;
    bool                                                      terminated;
    bool													  initialized;
    void													  trigger();
    void													  close(){
        if (initialized){
            terminated = true;
            cond.notify_all();
            tasks.stop();
        }
    }
};

class virtual_packer_service_handler : virtual public common_service_handler, virtual public virtual_packer_serviceIf {
public:
    virtual_packer_service_handler(std::wstring session_id);
    virtual ~virtual_packer_service_handler(){ suspend_jobs(); _mgmt.close(); }
    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }
    void get_virtual_host_info(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password);
    void get_virtual_machine_detail(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    void get_virtual_hosts(std::vector<virtual_host> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password);
    bool power_off_virtual_machine(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    bool remove_virtual_machine(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    void get_virtual_machine_snapshots(std::vector<vmware_snapshot> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    bool remove_virtual_machine_snapshot(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id, const std::string& snapshot_id);
    void get_datacenter_folder_list(std::vector<std::string> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& datacenter);
    void create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job);
    void create_job(packer_job_detail& _return, const std::string& session_id, const create_packer_job_detail& create_job);
    void get_job(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& previous_updated_time);
    bool interrupt_job(const std::string& session_id, const std::string& job_id);
    bool resume_job(const std::string& session_id, const std::string& job_id);
    bool remove_job(const std::string& session_id, const std::string& job_id);
    void list_jobs(std::vector<packer_job_detail> & _return, const std::string& session_id);
    void terminate(const std::string& session_id);
    bool running_job(const std::string& session_id, const std::string& job_id);
private:

    void load_jobs();
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
    snapshots_management						_mgmt;
};