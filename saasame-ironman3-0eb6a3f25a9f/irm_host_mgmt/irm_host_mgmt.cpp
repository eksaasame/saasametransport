
#include "irm_host_mgmt.h"
#include "vhdx.h"
#include "..\update\miniz.c"
#pragma comment(lib, "mpr.lib")
using namespace macho::windows;
using namespace macho;

void irm_host_mgmt::set_log(std::wstring log_file, macho::TRACE_LOG_LEVEL level ){
    macho::set_log_level(level);
    macho::set_log_file(log_file);
}

irm_host_mgmt::irm_host_mgmt(std::wstring host, std::wstring username, std::wstring password, std::wstring session) : _host(host), _username(username), _password(password), _port(18889), _is_interrupted(false){
    _target = macho::stringutils::convert_unicode_to_utf8(_host);
    _session = session;
    _socket = boost::shared_ptr<TSocket>(new TSocket(_target, _port));
    _transport = boost::shared_ptr<TTransport>(new TBufferedTransport(_socket));
    _protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));
    _client = boost::shared_ptr<host_agent_serviceClient>(new host_agent_serviceClient(_protocol));
}

bool irm_host_mgmt::interrupt(){
    _is_interrupted = true;
    return _is_interrupted;
}

bool irm_host_mgmt::config(std::wstring config_path, bool is_force_reconfig){
    
    auto_handle h;
    bool        result = false;
    bool        need_to_copy = true;
    try{
        com_init com;
        wmi_services wmi_service;
        wmi_service.connect(L"cimv2", _host, _username, _password);
        wmi_object processor = wmi_service.query_wmi_object(L"Win32_Processor");
        boost::filesystem::path binary_path = config_path;
        if (32 == (uint16_t)processor[L"AddressWidth"]){
            binary_path /= L"x86\\irm_host_agent.exe";
        }
        else{
            binary_path /= L"x64\\irm_host_agent.exe";
        }
        std::wstring remote = L"\\\\" + _host + L"\\Admin$";

        if (macho::windows::environment::is_running_as_local_system()){
            HANDLE hToken;
            LogonUser(L"NETWORK SERVICE", L"NT AUTHORITY", NULL, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken);
            h = hToken;
            ImpersonateLoggedOnUser(hToken);
        }
        NETRESOURCE nr;
        // Zero out the NETRESOURCE struct
        memset(&nr, 0, sizeof(NETRESOURCE));
        nr.dwScope = RESOURCE_GLOBALNET;
        nr.dwType = RESOURCETYPE_DISK;
        nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
        nr.lpRemoteName = (LPWSTR)remote.c_str();
        nr.lpProvider = NULL;

        // Assign a value to the connection options
        DWORD dwFlags = CONNECT_TEMPORARY;
        //
        // Call the WNetAddConnection2 function to assign
        //   a drive letter to the share.
        //
        DWORD dwRetVal = WNetAddConnection2(&nr, ((std::wstring)_password).c_str(), ((std::wstring)_username).c_str(), dwFlags);
        //
        // If the call succeeds, inform the user; otherwise,
        //  print the error.
        //
        if (dwRetVal != NO_ERROR){
            LOG(LOG_LEVEL_ERROR, L"WNetAddConnection2 Error: %d ", dwRetVal);
        }
        else{
            wmi_object_table objs = wmi_service.exec_query(L"SELECT * FROM Win32_Process WHERE Name = 'irm_host_agent.exe'");
            if (objs.size() ){
                if (!is_force_reconfig){
                    // Can add a code to parse the command line parameter to compare the session before return.
                    std::wstring command_line = objs[0][L"CommandLine"];
                    std::wstring executable_path = objs[0][L"ExecutablePath"];
                    std::wstring id = command_line.find(L"-s") != std::wstring::npos ? command_line.substr(command_line.find(L"-s") + 3, -1) : __noop;
                    if (id.length() && macho::guid_(id) == _session){
                        result = true;
                    }
                    need_to_copy = false;
                }
                else{
                    terminate();
                    Sleep(1000);
                    objs = wmi_service.exec_query(L"SELECT * FROM Win32_Process WHERE Name = 'irm_host_agent.exe'");
                    if (objs.size()){
                        wmi_object in = objs[0].get_input_parameters(L"Terminate");
                        //in[L"Reason"] = 0;
                        objs[0].exec_method(L"Terminate", in);
                    }
                }                    
            }
            if (need_to_copy){
                boost::filesystem::copy_file(binary_path, remote + L"\\irm_host_agent.exe", boost::filesystem::copy_option::overwrite_if_exists);
                wmi_object process;
                HRESULT hr = wmi_service.get_wmi_object(L"Win32_Process", process);
                if (SUCCEEDED(hr)){
                    wmi_object in = process.get_input_parameters(L"Create");
                    in[L"CommandLine"] = boost::str(boost::wformat(L"irm_host_agent.exe -s %s") % _session.wstring());
                    wmi_object out = process.exec_method(L"Create", in);
                    int32_t processid = out[L"ProcessId"];
                    result = true;
                }
            }
            WNetCancelConnection((LPWSTR)remote.c_str(), TRUE);
        }
    }
    catch (macho::exception_base &e){
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
    }
    if (h.is_valid()) RevertToSelf();
    return result;
}

bool irm_host_mgmt::remove(){

    auto_handle h;
    bool        result = false;
    try{
        bool    can_be_removed = true;
        com_init com;
        wmi_services wmi_service;
        wmi_service.connect(L"cimv2", _host, _username, _password);
        wmi_object_table objs = wmi_service.exec_query(L"SELECT * FROM Win32_Process WHERE Name = 'irm_host_agent.exe'");
        if (objs.size()){
            std::wstring command_line = objs[0][L"CommandLine"];
            std::wstring executable_path = objs[0][L"ExecutablePath"];
            std::wstring id = command_line.find(L"-s") != std::wstring::npos ? command_line.substr(command_line.find(L"-s") + 3, -1) : __noop;
            if (id.length() && macho::guid_(id) == _session){
                terminate();
                Sleep(1000);
                objs = wmi_service.exec_query(L"SELECT * FROM Win32_Process WHERE Name = 'irm_host_agent.exe'");
                if (objs.size()){
                    wmi_object in = objs[0].get_input_parameters(L"Terminate");
                    //in[L"Reason"] = 0;
                    objs[0].exec_method(L"Terminate", in);
                }
            }
            else
                can_be_removed = false;
        }

        if (!can_be_removed){
            result = true;
        }
        else{
            std::wstring remote = L"\\\\" + _host + L"\\Admin$";

            if (macho::windows::environment::is_running_as_local_system()){
                HANDLE hToken;
                LogonUser(L"NETWORK SERVICE", L"NT AUTHORITY", NULL, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken);
                h = hToken;
                ImpersonateLoggedOnUser(hToken);
            }
            NETRESOURCE nr;
            // Zero out the NETRESOURCE struct
            memset(&nr, 0, sizeof(NETRESOURCE));
            nr.dwScope = RESOURCE_GLOBALNET;
            nr.dwType = RESOURCETYPE_DISK;
            nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
            nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            nr.lpRemoteName = (LPWSTR)remote.c_str();
            nr.lpProvider = NULL;

            // Assign a value to the connection options
            DWORD dwFlags = CONNECT_TEMPORARY;
            //
            // Call the WNetAddConnection2 function to assign
            //   a drive letter to the share.
            //
            DWORD dwRetVal = WNetAddConnection2(&nr, ((std::wstring)_password).c_str(), ((std::wstring)_username).c_str(), dwFlags);
            //
            // If the call succeeds, inform the user; otherwise,
            //  print the error.
            //
            if (dwRetVal != NO_ERROR){
                LOG(LOG_LEVEL_ERROR, L"WNetAddConnection2 Error: %d ", dwRetVal);
            }
            else{
                boost::filesystem::remove(remote + L"\\irm_host_agent.exe");
                WNetCancelConnection((LPWSTR)remote.c_str(), TRUE);
                result = true;
            }
        }
    }
    catch (macho::exception_base &e){
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){

    }
    if (h.is_valid()) RevertToSelf();
    return result;
}

saasame::transport::client_info irm_host_mgmt::get_host_info(){
    try{
        _transport->open();
        _client->get_client_info(_info, _session);
        _transport->close();
    }
    catch (...){
        _transport->close();
    }
    return _info;
}

saasame::transport::snapshot_result irm_host_mgmt::take_snapshots(std::set<int32_t>& disks){
    try{
        _transport->open();
        _client->take_snapshots(_snapshots, _session, disks);
        _transport->close();
    }
    catch (...){
        _transport->close();
    }
    return _snapshots;
}

saasame::transport::snapshot_result irm_host_mgmt::get_latest_snapshots_info(){
    try{
        _transport->open();
        _client->get_latest_snapshots_info(_snapshots, _session);
        _transport->close();
    }
    catch (...){
        _transport->close();
    }
    return _snapshots;
}

saasame::transport::volume_bit_map  irm_host_mgmt::get_snapshot_bit_map(std::wstring snapshot_id, uint64_t start, bool compressed){
    return get_snapshot_bit_map(macho::stringutils::convert_unicode_to_ansi(snapshot_id), start, compressed);
}

saasame::transport::volume_bit_map  irm_host_mgmt::get_snapshot_bit_map(std::string snapshot_id, uint64_t start, bool compressed){
    saasame::transport::volume_bit_map _return;
    try{
        _transport->open();
        _client->get_snapshot_bit_map(_return, _session, snapshot_id, start, compressed);
        _transport->close();
    }
    catch (...){
        _transport->close();
    }
    return _return;
}

bool                                        irm_host_mgmt::terminate(){
    try{
        _transport->open();
        _client->exit(_session);
        _transport->close();
    }
    catch (...){
        _transport->close(); 
    }
    return true;
}

saasame::transport::delete_snapshot_result  irm_host_mgmt::delete_snapshot_set(std::wstring snapshot_set_id){
    return delete_snapshot_set(macho::stringutils::convert_unicode_to_ansi(snapshot_set_id));
}

saasame::transport::delete_snapshot_result irm_host_mgmt::delete_snapshot_set(std::string snapshot_set_id){
    saasame::transport::delete_snapshot_result _result;
    try{
        _transport->open();
        _client->delete_snapshot_set(_result, _session, snapshot_set_id);
        _transport->close();
    }
    catch (...){  
        _result.code = -1;
        _transport->close();
    }
    return _result;
}

saasame::transport::delete_snapshot_result  irm_host_mgmt::delete_snapshot(std::wstring snapshot_id){
    return delete_snapshot(macho::stringutils::convert_unicode_to_ansi(snapshot_id));
}

saasame::transport::delete_snapshot_result irm_host_mgmt::delete_snapshot(std::string snapshot_id){
    saasame::transport::delete_snapshot_result _result;
    try{
        _transport->open();
        _client->delete_snapshot_set(_result, _session, snapshot_id);
        _transport->close();
    }
    catch (...){
        _result.code = -1;
        _transport->close();
    }
    return _result;
}

bool irm_host_mgmt::write_vhd(const saasame::transport::replication_result& res, universal_disk_rw::ptr& file, uint64_t& start, uint32_t length){
    bool is_sync = true;
    uint32_t number_of_bytes_written = 0;
    if (res.result.size()){
        if (res.compressed){
            std::auto_ptr<BYTE> uncmp = std::auto_ptr<BYTE>(new BYTE[length]);
            if (Z_OK == uncompress(uncmp.get(), (uLong*)&length, (const unsigned char*)res.result.c_str(), (uLong)res.result.size())){
                if (file->write(start, uncmp.get(), length, number_of_bytes_written))
                    start += number_of_bytes_written;
                else{
                    is_sync = false;
                }
            }
        }
        else{
            if (file->write(start, res.result, number_of_bytes_written))
                start += number_of_bytes_written;
            else{
                is_sync = false;
            }
        }
    }
    else{
        is_sync = false;
    }
    return is_sync;
}

bool irm_host_mgmt::replicate_disk(int32_t disk, std::wstring virtual_disk_path, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot){

    bool result = false;
    if (boost::filesystem::exists(virtual_disk_path)) return result;
    replicate_disk_progress progress;
    DWORD err;
    progress.connect(slot);
    try{
        saasame::transport::disk_info d;       
        if (!get_disk_info(disk, d)){
            LOG(LOG_LEVEL_ERROR, L"Can't find disk %d", disk);
        }
        else if (ERROR_SUCCESS != ( err = win_vhdx_mgmt::create_vhdx_file(virtual_disk_path, CREATE_VIRTUAL_DISK_FLAG_NONE, d.size, CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE, d.logical_sector_size, d.physical_sector_size))){
            LOG(LOG_LEVEL_ERROR, L"Can't create the virtual disk file \"%s\". ( error : %d )", virtual_disk_path.c_str(), err);
        }
        else{           
            if (!block_size){
                vhd_disk_info _vhd_disk_info;
                if (ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_information(virtual_disk_path, _vhd_disk_info))
                    block_size = _vhd_disk_info.block_size;
                else
                    block_size = 2 * 1024 * 1024;
            }
            universal_disk_rw::ptr file = win_vhdx_mgmt::open_virtual_disk_for_rw(virtual_disk_path);
            if (file){
                int32_t retry_count = 3;
                uint64_t start = 0;
                do{
                    if (start == 0 && !(result = replicate_beginning_of_disk(d, file, block_size, is_compress, slot))){
                    }
                    else if (!(result = replicate_partitions_of_disk(d, start, file, block_size, is_compress, slot))){
                    }
                    else if (!(result = replicate_end_of_disk(d, file, block_size, is_compress, slot))){
                    }
                    else{
                        break;
                    }
                    Sleep(1000 * 10);
                } while ( ( retry_count-- ) > 0 );

                /*
                std::set<saasame::transport::partition_info> parts = get_partiton_info(disk);
                _transport->open();
                _client->get_latest_snapshots_info(_snapshots, _session);
                uint32_t length = block_size;
                uint64_t offset = 0;
                bool is_replicate = true;
                foreach(saasame::transport::partition_info p, parts){
                    if (1 == p.partition_number){
                        if (p.offset < block_size)
                            length = p.offset;
                        while (offset < p.offset){                     
                            saasame::transport::replication_result res;
                            _client->replicate_disk(res, _session, disk, offset, length, is_compress);
                            if (!(is_replicate = write_vhd(res, file, offset, length)))
                                break;
                            else
                                progress(disk, replication_state::replicating, virtual_disk_path, d.size, offset);
                        }
                    }
                    if (is_replicate && p.access_paths.size()){
                        saasame::transport::volume_info v;
                        if (get_volume_info(p.access_paths, v)){
                            saasame::transport::snapshot s;
                            if (!get_snapshot_info(v.path, s)){
                            }
                            else{
                                saasame::transport::volume_bit_map bit_map_result;
                                saasame::transport::replication_result res;
                                _client->get_snapshot_bit_map(bit_map_result, _session, s.snapshot_id, 0, true);
                                if (bit_map_result.code == 0 && bit_map_result.bit_map.size()){
                                    if (bit_map_result.starting_lcn){                                        
                                        length = bit_map_result.starting_lcn * bit_map_result.cluster_size;
                                        _client->replicate_snapshot(res, _session, s.snapshot_id, 0, length, is_compress);
                                        offset = p.offset;
                                        if (!(is_replicate = write_vhd(res, file, offset, length)))
                                            break;
                                        else
                                            progress(disk, replication_state::replicating, virtual_disk_path, d.size, offset);
                                    }
                                    length = block_size;
                                    offset = 0;
                                    uint32_t number_of_bits = block_size / bit_map_result.cluster_size;
                                    uLong bit_map_length = bit_map_result.total_number_of_clusters / 8;
                                    if (bit_map_result.compressed){
                                        std::auto_ptr<BYTE> bit_map = std::auto_ptr<BYTE>(new BYTE[bit_map_length]);
                                        if (Z_OK == uncompress(bit_map.get(), (uLong*)&bit_map_length, (const unsigned char*)bit_map_result.bit_map.c_str(), (uLong)bit_map_result.bit_map.size())){
                                            for (uint64_t nBit = 0; nBit < bit_map_result.total_number_of_clusters; nBit += number_of_bits){
                                                bool is_dirty = false;
                                                uint64_t boundary_of_bits = nBit + number_of_bits;
                                                if (boundary_of_bits > bit_map_result.total_number_of_clusters){
                                                    boundary_of_bits = bit_map_result.total_number_of_clusters;
                                                }
                                                for (uint64_t newBit = nBit; newBit < boundary_of_bits; newBit++){
                                                    if (bit_map.get()[newBit >> 3] & (1 << (newBit & 7))){
                                                        is_dirty = true;
                                                        break;
                                                    }
                                                }
                                                if (is_dirty){
                                                    offset = (bit_map_result.starting_lcn + nBit) * bit_map_result.cluster_size;
                                                    _client->replicate_snapshot(res, _session, s.snapshot_id, offset, length, is_compress);
                                                    offset += p.offset;
                                                    if (!(is_replicate = write_vhd(res, file, offset, length)))
                                                        break;
                                                    else
                                                        progress(disk, replication_state::replicating, virtual_disk_path, d.size, offset);
                                                }
                                                else{
                                                    progress(disk, replication_state::replicating, virtual_disk_path, d.size, p.offset + ( (bit_map_result.starting_lcn + boundary_of_bits) * bit_map_result.cluster_size));
                                                }
                                            }
                                            if (!is_replicate)
                                                break;
                                        }
                                        else{
                                            is_replicate = false;
                                            break;
                                        }
                                    }
                                    else{
                                        // No implemented for uncompressed operation.
                                    }
                                }
                            }
                        }
                    }

                    if (d.number_of_partitions == p.partition_number){
                        uint64_t end_of_latest_partition = p.offset + p.size;
                        uint64_t unpartition_size = d.size - end_of_latest_partition;
                        if (unpartition_size > (8 * 1024 * 1024))
                            unpartition_size =  8 * 1024 *1024;
                        offset = d.size - unpartition_size;
                        length = unpartition_size;
                        while (offset < d.size){
                            saasame::transport::replication_result res;
                            _client->replicate_disk(res, _session, disk, offset, length, is_compress);
                            if (!(is_replicate = write_vhd(res, file, offset, length)))
                                break;
                            else
                                progress(disk, replication_state::replicating, virtual_disk_path, d.size, offset);
                        }
                    }
                }
                _transport->close();
                */
            }
        }
        
    }
    catch (macho::exception_base &e){
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
    }
    return result;
}

bool irm_host_mgmt::replicate_beginning_of_disk(saasame::transport::disk_info& disk, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot){

    bool result = false;
    replicate_disk_progress progress;
    progress.connect(slot);
    try{
        _transport->open();
        std::set<saasame::transport::partition_info> parts = get_partiton_info(disk.number);
        uint32_t length = block_size;
        uint64_t offset = 0;
        bool is_replicate = true;
        foreach(saasame::transport::partition_info p, parts){
            if (1 == p.partition_number){
                if (p.offset < block_size)
                    length = (uint32_t)p.offset;
                while ((!_is_interrupted) && offset < (uint64_t)p.offset){
                    saasame::transport::replication_result res;
                    _client->replicate_disk(res, _session, disk.number, offset, length, is_compress);
                    if (!(is_replicate = write_vhd(res, vhd_file, offset, length)))
                        break;
                    else
                        progress(disk.number, replication_state::replicating, vhd_file->path(), disk.size, offset);
                }
            }
        }
        result = !_is_interrupted;
        _transport->close();
    }
    catch (macho::exception_base &e){
        _transport->close();
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        _transport->close();
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        _transport->close();
    }
    return result;
}

bool irm_host_mgmt::replicate_end_of_disk(saasame::transport::disk_info& disk, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot){

    bool result = false;
    replicate_disk_progress progress;
    progress.connect(slot);
    try{
        _transport->open();
        std::set<saasame::transport::partition_info> parts = get_partiton_info(disk.number);
        uint32_t length = block_size;
        uint64_t offset = 0;
        bool is_replicate = true;
        foreach(saasame::transport::partition_info p, parts){
            if (disk.number_of_partitions == p.partition_number){
                uint64_t end_of_latest_partition = p.offset + p.size;
                uint64_t unpartition_size = disk.size - end_of_latest_partition;
                if (unpartition_size > (8 * 1024 * 1024))
                    unpartition_size = 8 * 1024 * 1024;
                offset = disk.size - unpartition_size;
                length = (uint32_t)unpartition_size;
                while ((!_is_interrupted) && offset < (uint64_t)disk.size){
                    saasame::transport::replication_result res;
                    _client->replicate_disk(res, _session, disk.number, offset, length, is_compress);
                    if (!(is_replicate = write_vhd(res, vhd_file, offset, length)))
                        break;
                    else
                        progress(disk.number, replication_state::replicating, vhd_file->path(), disk.size, offset);
                }
            }
        }
        result = !_is_interrupted;
        _transport->close();
    }
    catch (macho::exception_base &e){
        _transport->close();
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        _transport->close();
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        _transport->close();
    }
    return result;
}

bool irm_host_mgmt::replicate_partitions_of_disk(saasame::transport::disk_info& disk, uint64_t& start_offset, universal_disk_rw::ptr vhd_file, uint32_t block_size, bool is_compress, replicate_disk_progress::slot_type slot){
    bool result = false;
    replicate_disk_progress progress;
    progress.connect(slot);
    try{
        _transport->open();
        std::set<saasame::transport::partition_info> parts = get_partiton_info(disk.number);
        bool is_replicate = true;
        uint64_t end_of_partiton = 0;
        foreach(saasame::transport::partition_info p, parts){
            uint32_t length = block_size;
            if (is_replicate){
                if ( start_offset < (uint64_t)p.offset ){
                    start_offset = (uint64_t)p.offset;
                }
                end_of_partiton = (uint64_t)p.offset + (uint64_t)p.size;
                if (!p.access_paths.size() && start_offset < end_of_partiton){
                    while ((!_is_interrupted) && start_offset < end_of_partiton ){
                        if ((start_offset + length) > end_of_partiton)
                            length = end_of_partiton - start_offset;
                        saasame::transport::replication_result res;
                        _client->replicate_disk(res, _session, disk.number, start_offset, length, is_compress);
                        if (!(is_replicate = write_vhd(res, vhd_file, start_offset, length)))
                            break;
                        else
                            progress(disk.number, replication_state::replicating, vhd_file->path(), disk.size, start_offset);
                    }
                    start_offset = end_of_partiton;             
                }
                else if (start_offset >= (uint64_t)p.offset && start_offset < end_of_partiton ){
                    saasame::transport::volume_info v;
                    if (!get_volume_info(p.access_paths, v)){
                        _transport->close();
                        break;
                    }
                    else if (p.is_active && p.is_system && v.file_system == "FAT32"){ // Replicate the boot partition.
                        while ((!_is_interrupted) && start_offset < end_of_partiton){
                            if ((start_offset + length) > end_of_partiton)
                                length = end_of_partiton - start_offset;
                            saasame::transport::replication_result res;
                            _client->replicate_disk(res, _session, disk.number, start_offset, length, is_compress);
                            if (!(is_replicate = write_vhd(res, vhd_file, start_offset, length)))
                                break;
                            else
                                progress(disk.number, replication_state::replicating, vhd_file->path(), disk.size, start_offset);
                        }
                        start_offset = end_of_partiton;
                    }
                    else{
                        saasame::transport::snapshot s;
                        if (!get_snapshot_info(v.path, s)){
                            _transport->close();
                            break;
                        }
                        else{
                            uint64_t offset = start_offset - p.offset;
                            saasame::transport::volume_bit_map bit_map_result;
                            saasame::transport::replication_result res;
                            _client->get_snapshot_bit_map(bit_map_result, _session, s.snapshot_id, offset, true);
                            if (bit_map_result.bit_map.size()){
                                uint32_t number_of_bits = block_size / bit_map_result.cluster_size;
                                uLong bit_map_length = bit_map_result.total_number_of_clusters / 8 ;
                                if (bit_map_result.compressed){
                                    std::auto_ptr<BYTE> bit_map = std::auto_ptr<BYTE>(new BYTE[bit_map_length]);
                                    if (Z_OK == uncompress(bit_map.get(), (uLong*)&bit_map_length, (const unsigned char*)bit_map_result.bit_map.c_str(), (uLong)bit_map_result.bit_map.size())){
                                        for (uint64_t nBit = 0; (!_is_interrupted) && nBit < (uint64_t)bit_map_result.total_number_of_clusters; nBit += number_of_bits){
                                            bool is_dirty = false;
                                            uint64_t boundary_of_bits = nBit + number_of_bits;
                                            if (boundary_of_bits >(uint64_t)bit_map_result.total_number_of_clusters){
                                                boundary_of_bits = (uint64_t)bit_map_result.total_number_of_clusters;
                                            }
                                            for (uint64_t newBit = nBit; newBit < boundary_of_bits; newBit++){
                                                if (bit_map.get()[newBit >> 3] & (1 << (newBit & 7))){
                                                    is_dirty = true;
                                                    break;
                                                }
                                            }
                                            if (is_dirty){
                                                offset = (bit_map_result.starting_lcn + nBit) * bit_map_result.cluster_size;
                                                _client->replicate_snapshot(res, _session, s.snapshot_id, offset, length, is_compress);
                                                start_offset = offset + p.offset;
                                                if (!(is_replicate = write_vhd(res, vhd_file, start_offset, length)))
                                                    break;
                                            }
                                            else{
                                                start_offset = p.offset + (bit_map_result.starting_lcn + boundary_of_bits) * bit_map_result.cluster_size;
                                            }
                                            progress(disk.number, replication_state::replicating, vhd_file->path(), disk.size, start_offset);
                                        }
                                        if (!is_replicate)
                                            break;
                                    }
                                    else{
                                        is_replicate = false;
                                        break;
                                    }
                                }
                                else{
                                    // No implemented for uncompressed operation.
                                }
                            }
                        }
                    }
                }
            }
        }
        result = is_replicate && (!_is_interrupted);
        _transport->close();
    }
    catch (macho::exception_base &e){
        _transport->close();
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        _transport->close();
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        _transport->close();
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        _transport->close();
    }
    return result;
}

bool irm_host_mgmt::get_disk_info(uint32_t disk, saasame::transport::disk_info& info){
    foreach(saasame::transport::disk_info i, _info.disk_infos){
        if (i.number == disk){
            info = i;
            return true;
        }
    }
    return false;
}

std::set<saasame::transport::partition_info> irm_host_mgmt::get_partiton_info(uint32_t disk){
    std::set<saasame::transport::partition_info> result;
    foreach(saasame::transport::partition_info p, _info.partition_infos){
        if (p.disk_number == disk){
            result.insert(p);
        }
    }
    return result;
}

bool irm_host_mgmt::get_volume_info(std::set<std::string> access_paths, saasame::transport::volume_info& info){
    foreach(std::string access_path, access_paths){
        foreach(saasame::transport::volume_info v, _info.volume_infos){
            foreach(std::string a, v.access_paths){
                if (a.length() >= access_path.length() && strstr(a.c_str(), access_path.c_str()) != NULL){
                    info = v;
                    return true;
                }
                else if (a.length() < access_path.length() && strstr(access_path.c_str(), a.c_str()) != NULL){
                    info = v;
                    return true;
                }
            }
        }
    }
    return false;
}

bool irm_host_mgmt::get_snapshot_info(std::string volume_path, saasame::transport::snapshot& info){
    foreach(saasame::transport::snapshot s, _snapshots.snapshots){     
        if (s.original_volume_name.length() >= volume_path.length() && strstr(s.original_volume_name.c_str(), volume_path.c_str()) != NULL){
            info = s;
            return true;
        }
        else if (s.original_volume_name.length() < volume_path.length() && strstr(volume_path.c_str(), s.original_volume_name.c_str()) != NULL){
            info = s;
            return true;
        }
    }
    return false;
}

uint64_t irm_host_mgmt::estimated_output_size(uint32_t disk){
    uint64_t size = 4*1024*1024;

    std::set<saasame::transport::partition_info> partitons = get_partiton_info(disk);
    foreach(saasame::transport::partition_info p, partitons){
        saasame::transport::volume_info v;
        if (get_volume_info(p.access_paths, v)){
            if (p.is_active && p.is_system && v.file_system == "FAT32"){
                size += p.size;
            }
            else{
                size += (v.size - v.size_remaining);
            }
        }
        else{
            size += p.size;
        }
    }
    return size;
}
