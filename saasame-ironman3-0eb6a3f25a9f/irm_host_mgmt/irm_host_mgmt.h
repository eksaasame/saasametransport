#pragma once
#ifndef __IRM_HOST_MGMT__
#define __IRM_HOST_MGMT__

#include <macho.h>
#include "../gen-cpp/saasame_constants.h"
#include "../gen-cpp/saasame_types.h"
#include "../gen-cpp/host_agent_service.h"
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <VersionHelpers.h>
#include <initguid.h>
#include <virtdisk.h>
#include <rpc.h>
#include <sddl.h>
#include "vhdx.h"

using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace saasame::transport;

#pragma comment(lib, "libevent.lib")
#pragma comment(lib, "libevent_core.lib")
#pragma comment(lib, "libevent_extras.lib")
#pragma comment(lib, "libthrift.lib")
#pragma comment(lib, "libthriftnb.lib")


enum replication_state{
    initial = 0,
    replicating,
    finished,
    error,
};

class irm_host_mgmt{

public:

    typedef boost::signals2::signal<void(const uint32_t&, const replication_state state, const std::wstring&, const uint64_t&, const uint64_t&)> replicate_disk_progress;
    
    irm_host_mgmt( std::wstring host, std::wstring username, std::wstring password, std::wstring session = L"");
    virtual ~irm_host_mgmt(){}
    bool                                        config(std::wstring config_path = macho::windows::environment::get_working_directory(), bool is_force_reconfig = false );
    bool                                        remove();
    saasame::transport::client_info                  get_host_info();
    saasame::transport::snapshot_result              take_snapshots(std::set<int32_t>& disks);
    saasame::transport::snapshot_result              get_latest_snapshots_info();
    void       inline                           set_replication_snapshots(saasame::transport::snapshot_result &snapshots){
        _snapshots = snapshots;
    }
    saasame::transport::volume_bit_map               get_snapshot_bit_map(std::string snapshot_id, uint64_t start, bool compressed);
    saasame::transport::volume_bit_map               get_snapshot_bit_map(std::wstring snapshot_id, uint64_t start, bool compressed);
    saasame::transport::delete_snapshot_result       delete_snapshot_set(std::string snapshot_set_id);
    saasame::transport::delete_snapshot_result       delete_snapshot(std::string snapshot_id);
    saasame::transport::delete_snapshot_result       delete_snapshot_set(std::wstring snapshot_set_id);
    saasame::transport::delete_snapshot_result       delete_snapshot(std::wstring snapshot_id);
    bool                                        replicate_disk(int32_t disk, std::wstring virtual_disk_path, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot);
    bool                                        replicate_beginning_of_disk(saasame::transport::disk_info& disk, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot);
    bool                                        replicate_end_of_disk(saasame::transport::disk_info& disk, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot);
    bool                                        replicate_partitions_of_disk(saasame::transport::disk_info& disk, uint64_t& start_offset, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot);
    bool                                        terminate();
    bool                                        interrupt();
    std::set<saasame::transport::partition_info> get_partiton_info(uint32_t disk);
    bool get_volume_info(std::set<std::string> access_paths, saasame::transport::volume_info& info);
    bool get_disk_info(uint32_t disk, saasame::transport::disk_info& info);
    void set_log(std::wstring log_file, macho::TRACE_LOG_LEVEL level = macho::TRACE_LOG_LEVEL::LOG_LEVEL_WARNING);
    uint64_t estimated_output_size(uint32_t disk);
private:

    bool get_snapshot_info(std::string volume_path, saasame::transport::snapshot& info);
    bool write_vhd(const saasame::transport::replication_result& res, universal_disk_rw::ptr& file, uint64_t& start, uint32_t length);
    bool                                        _is_interrupted;
    macho::guid_                                _session;
    std::wstring                                _host;
    std::string                                 _target;
#ifdef _DEBUG
    std::wstring                                _username;
    std::wstring                                _password;
#else
    macho::windows::protected_data              _username;
    macho::windows::protected_data              _password;
#endif
    int                                         _port ;

    saasame::transport::client_info     _info;
    saasame::transport::snapshot_result _snapshots;
    boost::shared_ptr<TSocket>                  _socket;
    boost::shared_ptr<TTransport>               _transport;
    boost::shared_ptr<TProtocol>                _protocol;
    boost::shared_ptr<host_agent_serviceClient> _client;

};


#endif
