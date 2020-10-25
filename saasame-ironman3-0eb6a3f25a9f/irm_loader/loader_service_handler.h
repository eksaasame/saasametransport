
#include "loader_service.h"
#include "common_service_handler.h"
#include "common_connection_service_handler.h"
#include "management_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "loader_job.h"
#include "..\irm_imagex\irm_imagex.h"
#include "irm_imagex_op.h"
#include "..\buildenv.h"

using boost::shared_ptr;

using namespace  ::saasame::transport;
using namespace  ::saasame::ironman;

class loader_service_handler : virtual public common_connection_service_handler, virtual public loader_serviceIf {
public:
    loader_service_handler(std::wstring session_id, int workers = 0);
    virtual ~loader_service_handler(){ suspend_jobs(); }
    void ping(service_info& _return);
    void create_job_ex(loader_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job);
    void create_job(loader_job_detail& _return, const std::string& session_id, const create_job_detail& create_job);
    void get_job(loader_job_detail& _return, const std::string& session_id, const std::string& job_id);
    bool interrupt_job(const std::string& session_id, const std::string& job_id);
    bool resume_job(const std::string& session_id, const std::string& job_id);
    bool remove_job(const std::string& session_id, const std::string& job_id);
    void list_jobs(std::vector<loader_job_detail> & _return, const std::string& session_id);
    bool update_job(const std::string& session_id, const std::string& job_id, const create_job_detail& job);
    void terminate(const std::string& session_id);
    bool remove_snapshot_image(const std::string& session_id, const std::map<std::string, image_map_info> & images);
    bool running_job(const std::string& session_id, const std::string& job_id);
    bool verify_management(const std::string& management, const int32_t port, const bool is_ssl);
    bool set_customized_id(const std::string& session_id, const std::string& disk_addr, const std::string& disk_id);
    void create_vhd_disk_from_snapshot(std::string& _return, const std::string& connection_string, const std::string& container, const std::string& original_disk_name, const std::string& target_disk_name, const std::string& snapshot);
    bool is_snapshot_vhd_disk_ready(const std::string& task_id);
    bool delete_vhd_disk(const std::string& connection_string, const std::string& container, const std::string& disk_name);
    bool delete_vhd_disk_snapshot(const std::string& connection_string, const std::string& container, const std::string& disk_name, const std::string& snapshot);
    void get_vhd_disk_snapshots(std::vector<vhd_snapshot> & _return, const std::string& connection_string, const std::string& container, const std::string& disk_name);
    bool verify_connection_string(const std::string& connection_string);
    void create_cascading_job(std::string id, std::string machine_id, std::string loader_job_id, saasame::transport::create_job_detail detail);
    macho::job::vtr get_cascading_jobs(std::wstring id);

private:

    class create_vhd_blob_from_snapshot_task : public macho::job {
    public:
        typedef boost::shared_ptr<create_vhd_blob_from_snapshot_task> ptr;
        typedef std::map<std::string, ptr>           map;
        create_vhd_blob_from_snapshot_task(
            const std::string& connection_string,
            const std::string& container,
            const std::string& original_disk_name,
            const std::string& target_disk_name,
            const std::string& snapshot
            ):
            _connection_string(connection_string),
            _container(container),
            _original_disk_name(original_disk_name),
            _target_disk_name(target_disk_name),
            _snapshot(snapshot),
            _is_error(false),
            macho::job(macho::guid_::create(), macho::guid_(GUID_NULL))
            {
        }
        virtual void execute();
        virtual void interrupt(){}
        bool is_error() const { return _is_error; }
        std::string result() const { return _result;  }
    private:
        std::string _connection_string;
        std::string _container;
        std::string _original_disk_name;
        std::string _target_disk_name;
        std::string _snapshot;
        bool        _is_error;
        std::string _result;
    };

    void load_jobs();
    void suspend_jobs();

    void job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec){
        macho::windows::auto_lock lock(_lock);

    }
    void job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex){
        macho::windows::auto_lock lock(_lock);

    }
    macho::guid_                                _session;
    macho::scheduler                            _tasker;
    create_vhd_blob_from_snapshot_task::map     _create_vhd_blob_from_snapshot_tasks;
    macho::scheduler                            _scheduler;
    loader_job::map                             _scheduled;
    macho::windows::critical_section            _lock;
    macho::windows::critical_section            _f_lock;
    macho::windows::critical_section            _verify_lock;
};