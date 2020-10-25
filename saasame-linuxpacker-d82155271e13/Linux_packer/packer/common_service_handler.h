#pragma once 
#ifndef common_service_handler_H
#define common_service_handler_H

#include "include.h"
#include "../gen-cpp/common_service.h"
#include "../tools/system_tools.h"
#include "../tools/storage.h"
using namespace linux_storage;

using namespace ::saasame::transport;
using namespace std;

#define INIT_BUS_TYPE_MAP								\
do{														\
	bus_type_index["scsi"] = bus_type::type::SCSI;		\
}while(0)


/*struct*/
typedef struct _host_context
{
    string host_working_path;
    string host_name;
    string host_crt_file_path;
    string host_key_gile_path;
    string host_uuid;
    string host_version;
}host_context;

class common_service_handler : virtual public common_serviceIf {
public:
    common_service_handler() :is_skip_read_error(false), merge_size(65536){ init_config(); /*take_xray_local();*/ }
    void ping(service_info& _return) = 0;
    void get_host_detail(physical_machine_info& _return, const std::string& session_id, const machine_detail_filter::type filter);
    void get_service_list(std::set<service_info> & _return, const std::string& session_id);
    void enumerate_disks(std::set<disk_info> & _return, const enumerate_disk_filter_style::type filter);
    bool verify_carrier(const std::string& carrier, const bool is_ssl);
    void take_xray(std::string& _return);
    void take_xrays(std::string& _return);
    bool create_mutex(const std::string& session, const int16_t timeout);
    bool delete_mutex(const std::string& session);
    void take_xray_local();

private:
    std::string get_xray(int port, const std::string& host = std::string("127.0.0.1"));
    virtual void init_config();
    void get_host_general_info(physical_machine_info& _info);
    disk_info fill_disk_info(disk::ptr & dc);
    partition_info fill_partition_info(partitionA::ptr &pc);
    void get_host_general_info_fake();
    service_info get_service(int port);
    host_context _hc;
    system_tools _st;
protected:
    disk::vtr                     dcs;
    storage::ptr                  str;
    bool           is_skip_read_error;
    std::string     version;
    std::map<std::string, int>           snapshots_cow_space_user_config;
    uint64_t                      merge_size;
};

#endif