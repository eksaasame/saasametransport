#pragma once

#ifndef physical_packer_service_handler_H
#define physical_packer_service_handler_H

#include "../gen-cpp/saasame_constants.h"
#include "../gen-cpp/physical_packer_service.h"

#include "../gen-cpp/saasame_types.h"

#include <mutex>
#include <chrono>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include "compile_config.h"
#include "../tools/system_tools.h"
#include "../tools/snapshot.h"
#include "../tools/jobs_scheduler.h"
#include "../tools/storage.h"
using namespace linux_storage;
//#include "carrier.h"
#include "common_service_handler.h"
#include "linux_packer_job.h"

#include <boost/thread/mutex.hpp>

using boost::shared_ptr;

using namespace  ::saasame::transport;

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::saasame::transport;
using namespace std;

/*define*/
#define PK_MAX_LOGS_COUNT 9
class physical_packer_service_handler : virtual public common_service_handler, virtual public physical_packer_serviceIf {
public:
    struct snapshot_set
    {
        std::string uuid;
        std::set<std::string> disks;
        snapshot_instance::vtr sets;
        btrfs_snapshot_instance::vtr btrfs_sets;
        void clear() { uuid.clear(); disks.clear(); sets.clear(); btrfs_sets.clear(); }
    };

    typedef boost::shared_ptr<physical_packer_service_handler> ptr;
	physical_packer_service_handler() : common_service_handler(), cg_retry_count(2){
        default_init();
        _scheduler.start();
        str = storage::get_storage();
        sh = snapshot_manager::ptr(new snapshot_manager(snapshots_cow_space_user_config));
        snapshot_init(sh);
        sh->set_merge_size(merge_size);
        load_jobs(); //unload jobs for test;
    };
	physical_packer_service_handler(std::string session_id) : common_service_handler() { default_init(); }

	virtual ~physical_packer_service_handler() { }

	void ping(service_info& _return);

	void take_snapshots(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks);

    void take_snapshots_ex(std::vector<snapshot> & _return, const std::string& session_id, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script);

    void take_snapshots2(std::vector<snapshot> & _return, const std::string& session_id, const take_snapshots_parameters& parameters);

	void delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_id);

	void delete_snapshot_set(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_set_id);

	void get_all_snapshots(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id);

	void create_job_ex(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_packer_job_detail& create_job); 

	void create_job(packer_job_detail& _return, const std::string& session_id, const create_packer_job_detail& create_job);

	void get_job(packer_job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& previous_updated_time);

	bool interrupt_job(const std::string& session_id, const std::string& job_id);

	bool resume_job(const std::string& session_id, const std::string& job_id);

	bool remove_job(const std::string& session_id, const std::string& job_id);

	void list_jobs(std::vector<packer_job_detail> & _return, const std::string& session_id);

	void terminate(const std::string& session_id);

	bool running_job(const std::string& session_id, const std::string& job_id);

    bool create_mutex(const std::string& session, const int16_t timeout);

    bool delete_mutex(const std::string& session);

    void random_mutli_threads_datto_read(std::string input);

    void compress_log_file();

    bool unregister(const std::string& session);

    void set_next_job_force_full() { b_force_full = true; }

    int                                   cg_retry_count;
    disk::map                                 modify_disks;

private:
    void _delete_snapshot(delete_snapshot_result& _return, const std::string& session_id, const std::string& snapshot_id, bool is_set);
    void _fill_snapshot_info(snapshot &sn, snapshot_instance::ptr &si);
    void snapshot_init(snapshot_manager::ptr sh);

    void job_to_be_executed_callback(const trigger::ptr& job, const job_execution_context& ec);
    void job_was_executed_callback(const trigger::ptr& job, const job_execution_context& ec, const job::exception& ex);
    void default_init() { b_force_full = false; need_reset_snapshot = false; }
    void load_jobs() {
        FUN_TRACE;
        boost::unique_lock<boost::mutex> lock(_cs);
        boost::filesystem::path working_dir = boost::filesystem::path(system_tools::get_execution_file_path_linux());
        namespace fs = boost::filesystem;
        fs::path job_dir = working_dir.parent_path() / "jobs";
        fs::directory_iterator end_iter;
        typedef std::multimap<std::time_t, fs::path> result_set_t;
        result_set_t result_set;
        if (fs::exists(job_dir) && fs::is_directory(job_dir)) {
            for (fs::directory_iterator dir_iter(job_dir); dir_iter != end_iter; ++dir_iter) {
                if (fs::is_regular_file(dir_iter->status())) {
                    if (dir_iter->path().extension() == JOB_EXTENSION) {
                        linux_packer_job::ptr job = linux_packer_job::load(dir_iter->path(), dir_iter->path().string() + ".status",sh, b_force_full);
                        job->set_packer(this);// set packer
                        job->set_modify_disks(modify_disks);
                        if (!_scheduled.count(job->id())) {
                            job->register_job_to_be_executed_callback_function(boost::bind(&physical_packer_service_handler::job_to_be_executed_callback, this, _1, _2));
                            job->register_job_was_executed_callback_function(boost::bind(&physical_packer_service_handler::job_was_executed_callback, this, _1, _2, _3));
                            run_once_trigger rot;
                            _scheduler.schedule_job(job,rot);
                            _scheduled[job->id()] = job;
                        }
                    }
                }
            }
        }
    }


    std::timed_mutex                                   _tcs;
    boost::mutex                                        _cs;
    boost::recursive_mutex                   _lock;
    linux_packer_job::map                    _scheduled;
    scheduler                                   _scheduler;
    snapshot_manager::ptr                     sh;
    bool                                      b_force_full;
    bool                                      need_reset_snapshot;
    snapshot_set                              snset;
    storage::ptr                              str;
};
#endif

