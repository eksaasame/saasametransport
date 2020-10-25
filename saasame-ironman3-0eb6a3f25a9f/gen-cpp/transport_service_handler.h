#pragma once

#ifndef transport_service_handler_H
#define transport_service_handler_H

#include "transport_service.h"
#include "reverse_transport.h"
#include "common_service_handler.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "license_db.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class long_running_task : public macho::removeable_job {
public:
    typedef boost::shared_ptr<long_running_task> ptr;
    typedef std::map<std::string, ptr>           map;
    long_running_task(running_task task);
    static long_running_task::ptr create(running_task task);
    static long_running_task::ptr load(boost::filesystem::path config_file);
    virtual void execute();
    virtual void remove();
    trigger::vtr virtual get_trigger();
    virtual void interrupt(){}
    void save();
    void update(running_task task){
        _task = task;
        save();
    }
private:
    boost::filesystem::path _config_file;
    running_task            _task;
};

class reverse_transport_handler : virtual public saasame::transport::reverse_transportIf {
public:

    reverse_transport_handler();

    virtual ~reverse_transport_handler(){
        _is_Open = false;
    }
    void ping(service_info& _return);
    virtual void generate_session(std::string& _return, const std::string& addr);
    virtual bool is_valid_session(const std::string& session, const std::string& addr);
    void receive(transport_message& _return, const std::string& session, const std::string& addr, const std::string& name);
    bool response(const std::string& session, const std::string& addr, const transport_message& response);
    void release();
    bool is_session_alive(std::string addr);
    std::string flush(const std::string& addr, const std::string& command, int timeout = 300);
    void set_machine_id(std::string id) { _machine_id = id; }
private:
    std::string encode64(const std::string &val);
    struct session_object{
        typedef boost::shared_ptr<session_object>       ptr;
        typedef std::map<std::string, ptr>              map;
        struct response{
            typedef boost::shared_ptr<response>             ptr;
            typedef std::map<int64_t, ptr>                  map;
            boost::mutex                                    cs;
            std::string                                     data;
            boost::condition_variable                       cond;
        };

        boost::mutex                                    cs;
        std::string                                     name;
        boost::posix_time::ptime                        timeout;
        std::deque<transport_message>                   commands;
        response::map                                   responses;
        boost::condition_variable                       cond;
    };
    session_object::map                                                   _sessions;
    uint16_t                                                              _id;
    boost::mutex                                                          _cs;
    bool                                                                  _is_Open;
    int                                                                   _long_polling_interval;
    std::string                                                           _machine_id;
};

class transport_service_handler : virtual public physical_packer_service_proxyIf, virtual public common_service_handler, virtual public transport_serviceIf {
private:
    class sync_job : public macho::job{
    public:
        sync_job() : job(L"sync_job", L""){}
        virtual void execute(){}
    };
    bool is_uuid(std::string uuid){
        try{
            guid_ u = uuid;
            return true;
        }
        catch (...){
        }
        return false;
    }
public:
    transport_service_handler(reverse_transport_handler& reverse) : _sync(1), _reverse(reverse){
        initialize();
        macho::job::ptr j(new sync_job());
        j->register_job_to_be_executed_callback_function(boost::bind(&transport_service_handler::timer_sync_db, this, _1, _2));
        _sync.schedule_job(j, macho::interval_trigger(boost::posix_time::minutes(5)));
        load_tasks();
        _sync.start();
    }
    virtual ~transport_service_handler(){ }

    virtual void generate_session(std::string& _return, const std::string& addr);

    void get_package_info(std::string& _return, const std::string& email, const std::string& name, const std::string& key){
        std::string _key = key;
        _key = macho::stringutils::toupper(_key);
        _get_package_info(_return, email, name, _key);
    }
    bool active_license(const std::string& email, const std::string& name, const std::string& key);
    bool add_license(const std::string& license){
        return _add_license(license, false, "");
    }
    bool add_license_with_key(const std::string& key, const std::string& license){
        std::string _key = key;
        return _add_license(license, false, stringutils::remove_begining_whitespaces(stringutils::erase_trailing_whitespaces(stringutils::toupper(_key))));
    }
    void get_licenses(license_infos& _return);
    bool check_license_expiration(const int8_t days);

    bool is_license_valid(const std::string& job_id);
    bool is_license_valid_ex(const std::string& job_id, const bool is_recovery);
    bool remove_license(const std::string& key);
    void query_package_info(std::string& _return, const std::string& key);
    void ping(service_info& _return);
    bool create_task(const running_task& task);
    bool remove_task(const std::string& task_id);

    //common_service
    void ping_p(service_info& _return, const std::string& addr);
    void get_host_detail_p(physical_machine_info& _return, const std::string& addr, const machine_detail_filter::type filter);
    void get_service_list_p(std::set<service_info> & _return, const std::string& addr);
    void enumerate_disks_p(std::set<disk_info> & _return, const std::string& addr, const enumerate_disk_filter_style::type filter);
    bool verify_carrier_p(const std::string& addr, const std::string& carrier, const bool is_ssl);
    void take_xray_p(std::string& _return, const std::string& addr);
    void take_xrays_p(std::string& _return, const std::string& addr);
    bool create_mutex_p(const std::string& addr, const std::string& session, const int16_t timeout);
    bool delete_mutex_p(const std::string& addr, const std::string& session);

    //service_proxy
    void create_job_ex_p(job_detail& _return, const std::string& addr, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type);
    void get_job_p(job_detail& _return, const std::string& addr, const std::string& job_id, const std::string& service_type);
    bool interrupt_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type);
    bool resume_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type);
    bool remove_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type);
    bool running_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type);
    bool update_job_p(const std::string& addr, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type);
    bool remove_snapshot_image_p(const std::string& addr, const std::map<std::string, image_map_info> & images, const std::string& service_type);
    bool test_connection_p(const std::string& addr, const connection& conn, const std::string& service_type);
    bool add_connection_p(const std::string& addr, const connection& conn, const std::string& service_type);
    bool remove_connection_p(const std::string& addr, const std::string& connection_id, const std::string& service_type);
    bool modify_connection_p(const std::string& addr, const connection& conn, const std::string& service_type);
    void enumerate_connections_p(std::vector<connection> & _return, const std::string& addr, const std::string& service_type);
    void get_connection_p(connection& _return, const std::string& addr, const std::string& connection_id, const std::string& service_type);
    void get_virtual_host_info_p(virtual_host& _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password);
    void get_virtual_machine_detail_p(virtual_machine& _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    
    void get_virtual_hosts_p(std::vector<virtual_host> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password);
    bool power_off_virtual_machine_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    bool remove_virtual_machine_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    void get_virtual_machine_snapshots_p(std::vector<vmware_snapshot> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id);
    bool remove_virtual_machine_snapshot_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id, const std::string& snapshot_id);
    void get_datacenter_folder_list_p(std::vector<std::string> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& datacenter);

    void get_physical_machine_detail_p(physical_machine_info& _return, const std::string& addr, const std::string& host, const machine_detail_filter::type filter);
    void take_packer_xray_p(std::string& _return, const std::string& addr, const std::string& host);
    void get_packer_service_info_p(service_info& _return, const std::string& addr, const std::string& host);
    bool verify_management_p(const std::string& addr, const std::string& management, const int32_t port, const bool is_ssl);
    bool verify_packer_to_carrier_p(const std::string& addr, const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl);
    void get_replica_job_create_detail(replica_job_create_detail& _return, const std::string& session_id, const std::string& job_id);
    void get_loader_job_create_detail(loader_job_create_detail& _return, const std::string& session_id, const std::string& job_id);
    void get_launcher_job_create_detail(launcher_job_create_detail& _return, const std::string& session_id, const std::string& job_id);
    void terminate(const std::string& session_id);
    bool set_customized_id_p(const std::string& addr, const std::string& disk_addr, const std::string& disk_id);

    // physical packer service proxy
    void packer_ping_p(service_info& _return, const std::string& session_id, const std::string& addr);
    void take_snapshots_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks);
    void take_snapshots_ex_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script);
    void take_snapshots2_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const take_snapshots_parameters& parameters);
    void delete_snapshot_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_id);
    void delete_snapshot_set_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_set_id);
    void get_all_snapshots_p(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id, const std::string& addr);
    void create_packer_job_ex_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const create_packer_job_detail& create_job);
    void get_packer_job_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const std::string& previous_updated_time);
    bool interrupt_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id);
    bool resume_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id);
    bool remove_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id);
    bool running_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id);
    void enumerate_packer_disks_p(std::set<disk_info> & _return, const std::string& session_id, const std::string& addr, const enumerate_disk_filter_style::type filter);
    bool verify_packer_carrier_p(const std::string& session_id, const std::string& addr, const std::string& carrier, const bool is_ssl);
    void get_packer_host_detail_p(physical_machine_info& _return, const std::string& session_id, const std::string& addr, const machine_detail_filter::type filter);
    bool unregister_packer_p(const std::string& addr);
    bool unregister_server_p(const std::string& addr);
    void create_vhd_disk_from_snapshot(std::string& _return, const std::string& connection_string, const std::string& container, const std::string& original_disk_name, const std::string& target_disk_name, const std::string& snapshot);
    bool is_snapshot_vhd_disk_ready(const std::string& task_id);
    bool delete_vhd_disk(const std::string& connection_string, const std::string& container, const std::string& disk_name);
    bool delete_vhd_disk_snapshot(const std::string& connection_string, const std::string& container, const std::string& disk_name, const std::string& snapshot);
    void get_vhd_disk_snapshots(std::vector<vhd_snapshot> & _return, const std::string& connection_string, const std::string& container, const std::string& disk_name);
    bool verify_connection_string(const std::string& connection_string);

private:
    void _get_package_info(std::string& _return, const std::string& email, const std::string& name, const std::string& key);
    bool _add_license(const std::string& license, bool online,const std::string& key);
    typedef std::map<uint32_t, uint32_t> consumed_map_type;
    typedef std::map<std::string, consumed_map_type> workload_consumed_map_type;
    inline consumed_map_type calculated_consumed(workload::vtr& workloads);
    inline workload_consumed_map_type calculated_consumed(workload::map& workloads);
    inline workload_consumed_map_type calculated_license_consumed(std::vector<license_info> & _return, const std::string& machine_id, bool &can_be_run, workload_consumed_map_type& consumed, bool is_recovery, boost::posix_time::ptime datetime = boost::posix_time::second_clock::universal_time());
    void initialize();
    inline bool sync_db(const std::string& job_id = std::string(), std::string& machine_id = std::string(), bool is_recovery = false);
    inline void check_license(const std::string& job_id, bool is_recovery);
    std::string extract_file_resource(std::string custom_resource_name, int resource_id);
    std::string public_encrypt(const std::string& data, const std::string& key);
    std::string private_decrypt(const std::string& info, const std::string& key);
    std::string decode64(const std::string &val);
    std::string encode64(const std::string &val);

    void timer_sync_db(const trigger::ptr& job, const job_execution_context& ec);
    void load_tasks();
    macho::windows::protected_data   _ps;
    macho::windows::protected_data   _mac;
    macho::windows::critical_section _cs;
    license_db::ptr                  _db;
    macho::windows::critical_section _sync_cs;
    macho::scheduler                 _sync;
    std::string                      _id;
    long_running_task::map           _running_tasks;
    reverse_transport_handler&       _reverse;
};


#endif