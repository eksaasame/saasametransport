#include "transport_job.h"
#include "common_service_handler.h"
#include "physical_packer_service.h"
#include "carrier_rw.h"
#include <codecvt>
#include <boost/date_time/gregorian/gregorian.hpp>
#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

ULONGLONG physical_transport_job::physical_packer_job_op::get_version(std::string version_string){
    string_array values = stringutils::tokenize(macho::stringutils::convert_utf8_to_unicode(version_string), _T("."), 5);
    WORD    version[4] = { 0, 0, 0, 0 };
    switch (values.size()){
    case 5:
    case 4:
        version[3] = values[3].length() ? _ttoi(values[3].c_str()) : 0;
    case 3:
        version[2] = values[2].length() ? _ttoi(values[2].c_str()) : 0;
    case 2:
        version[1] = values[1].length() ? _ttoi(values[1].c_str()) : 0;
    case 1:
        version[0] = values[0].length() ? _ttoi(values[0].c_str()) : 0;
    };
    return MAKEDLLVERULL(version[0], version[1], version[2], version[3]);
}

bool physical_transport_job::physical_packer_job_op::initialize(std::string &priority_packer_addr) {
    FUN_TRACE;
    foreach(std::string addr, _addrs){
        if (addr.empty())
            continue;
        if (transport_job::is_uuid(addr)){
            _proxy_addr = addr;
            break;
        }
    }
    if (!_proxy_addr.empty()){
        if (!priority_packer_addr.empty()){
            thrift_connect<physical_packer_service_proxyClient>::ptr proxy(new thrift_connect<physical_packer_service_proxyClient>(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, priority_packer_addr));
            if (proxy->open()){
                try{
                    service_info info;
                    proxy->client()->packer_ping_p(info, _session, _proxy_addr);
                    _proxy = proxy;
                    proxy->transport()->close();
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                }
            }
        }
        if (!_proxy){
            foreach(std::string addr, _mgmt_addrs){
                if (addr.empty())
                    continue;
                if (!priority_packer_addr.empty() && priority_packer_addr == addr)
                    continue;
                thrift_connect<physical_packer_service_proxyClient>::ptr proxy(new thrift_connect<physical_packer_service_proxyClient>(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr));
                if (proxy->open()){
                    try{
                        service_info info;
                        proxy->client()->packer_ping_p(info, _session, _proxy_addr);
                        _proxy = proxy;
                        priority_packer_addr = addr;
                        proxy->transport()->close();
                    }
                    catch (saasame::transport::invalid_operation& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                    }
                    catch (TException &ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    }
                    break;
                }
            }
        }
    }
    else{
        if (!priority_packer_addr.empty()){
            thrift_connect<physical_packer_serviceClient>::ptr packer(new thrift_connect<physical_packer_serviceClient>(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, priority_packer_addr));
            if (packer->open()){
                try{
                    _packer = packer;
                    packer->transport()->close();
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                }
            }
        }
        if (!_packer){
            foreach(std::string addr, _addrs){
                if (addr.empty())
                    continue;
                if (!priority_packer_addr.empty() && priority_packer_addr == addr)
                    continue;
                thrift_connect<physical_packer_serviceClient>::ptr packer(new thrift_connect<physical_packer_serviceClient>(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr));
                if (packer->open()){
                    try{
                        _packer = packer;
                        priority_packer_addr = addr;
                        packer->transport()->close();
                    }
                    catch (saasame::transport::invalid_operation& ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                    }
                    catch (TException &ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    }
                    break;
                }
            }
        }
    }

    return _packer || _proxy;
}

bool physical_transport_job::physical_packer_job_op::enumerate_disks(std::set<saasame::transport::disk_info>& disks){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
           _proxy->transport()->open();
           _proxy->client()->enumerate_packer_disks_p(disks, _session, _proxy_addr, saasame::transport::enumerate_disk_filter_style::ALL_DISK);
           _proxy->transport()->close();
           result = true;
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->enumerate_disks(disks, saasame::transport::enumerate_disk_filter_style::ALL_DISK);
            _packer->transport()->close();
            result = true;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::take_snapshots(std::vector<snapshot>& snapshots, const saasame::transport::take_snapshots_parameters& parameter){
    bool result = false;
    FUN_TRACE;
    try{
        service_info          info;
        if (_proxy){
            _proxy->transport()->open();
            if ((!parameter.pre_script.empty()) || (!parameter.post_script.empty() || (!parameter.excluded_paths.empty()))){
                _proxy->client()->packer_ping_p(info, _session, _proxy_addr);
                if (info.version.length() && get_version(info.version) > get_version("1.1.917.0"))
                    _proxy->client()->take_snapshots2_p(snapshots, _session, _proxy_addr, parameter);
                else if (info.version.length() && get_version(info.version) > get_version("1.1.800.0"))
                    _proxy->client()->take_snapshots_ex_p(snapshots, _session, _proxy_addr, parameter.disks, parameter.pre_script, parameter.post_script);
                else
                    _proxy->client()->take_snapshots_p(snapshots, _session, _proxy_addr, parameter.disks);
            }
            else{
                _proxy->client()->take_snapshots_p(snapshots, _session, _proxy_addr, parameter.disks);
            }
            _proxy->transport()->close();
            result = true;
        }
        else if (_packer){
            _packer->transport()->open();
            if ((!parameter.pre_script.empty()) || (!parameter.post_script.empty() || (!parameter.excluded_paths.empty()))){
                _packer->client()->ping(info);
                if (info.version.length() && get_version(info.version) > get_version("1.1.917.0"))
                    _packer->client()->take_snapshots2(snapshots, _session, parameter);
                else if (info.version.length() && get_version(info.version) > get_version("1.1.802.0"))
                    _packer->client()->take_snapshots_ex(snapshots, _session, parameter.disks, parameter.pre_script, parameter.post_script);
                else
                    _packer->client()->take_snapshots(snapshots, _session, parameter.disks);
            }
            else{
                _packer->client()->take_snapshots(snapshots, _session, parameter.disks);
            }
            _packer->transport()->close();
            result = true;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }  
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool  physical_transport_job::physical_packer_job_op::delete_snapshot_set(const std::string snapshot_set_id, delete_snapshot_result& _delete_snapshot_result){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->delete_snapshot_set_p(_delete_snapshot_result, _session, _proxy_addr, snapshot_set_id);
            _proxy->transport()->close();
            result = true;
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->delete_snapshot_set(_delete_snapshot_result, _session, snapshot_set_id);
            _packer->transport()->close();
            result = true;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(snapshot_set_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->get_all_snapshots_p(snapshots, _session, _proxy_addr);
            _proxy->transport()->close();
            result = true;
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->get_all_snapshots(snapshots, _session);
            _packer->transport()->close();
            result = true;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::get_packer_job_detail(const std::string job_id, const boost::posix_time::ptime previous_updated_time, saasame::transport::packer_job_detail &detail){
    bool result = false;
    FUN_TRACE;
    try{
        std::string _previous_update_time = boost::posix_time::to_simple_string(previous_updated_time);
        LOG(LOG_LEVEL_DEBUG, L"previous update time %s", stringutils::convert_ansi_to_unicode(_previous_update_time).c_str());
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->get_packer_job_p(detail, _session, _proxy_addr, job_id, _previous_update_time);
            _proxy->transport()->close();
            result = true;
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->get_job(detail, _session, job_id, _previous_update_time);
            _packer->transport()->close();
            result = true;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    LOG(LOG_LEVEL_DEBUG, L"update time %s", stringutils::convert_ansi_to_unicode(detail.updated_time).c_str());
    foreach(job_history h, detail.histories){
        LOG(LOG_LEVEL_DEBUG, L"Packer Message: (%s) - %s", macho::stringutils::convert_utf8_to_unicode(h.time).c_str(), macho::stringutils::convert_utf8_to_unicode(h.description).c_str());
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::create_packer_job(const std::set<std::string> connections,
    const replica_job_create_detail& _replica_job_create_detail,
    const physical_create_packer_job_detail& detail,
    const std::set<std::string>& excluded_paths,
    const std::set<std::string>& resync_paths,
    std::string& job_id,
    boost::posix_time::ptime& created_time,
    saasame::transport::job_type::type type){
    FUN_TRACE;
    bool result = false;
    saasame::transport::create_packer_job_detail create_detail;
    saasame::transport::packer_job_detail _return;
    create_detail.__set_type(type);
    create_detail.__set_connection_ids(connections);
    create_detail.__set_carriers(_replica_job_create_detail.carriers);
    create_detail.__set_checksum_verify(_replica_job_create_detail.checksum_verify);
    create_detail.__set_detail(create_detail.detail);
    create_detail.detail.__set_p(create_detail.detail.p);
    create_detail.detail.p.__set_disks(_replica_job_create_detail.disks);
    create_detail.detail.p.__set_snapshots(detail.snapshots);
    create_detail.detail.p.__set_images(detail.images);
    create_detail.detail.p.__set_previous_journals(detail.previous_journals);
    create_detail.detail.p.__set_backup_size(detail.backup_size);
    create_detail.detail.p.__set_backup_progress(detail.backup_progress);
    create_detail.detail.p.__set_backup_image_offset(detail.backup_image_offset);
    create_detail.detail.p.__set_cdr_journals(detail.cdr_journals);
    create_detail.detail.p.__set_cdr_changed_ranges(detail.cdr_changed_ranges);
    create_detail.detail.p.__set_completed_blocks(detail.completed_blocks);
    create_detail.__set_timeout(_replica_job_create_detail.timeout);
    create_detail.__set_is_encrypted(_replica_job_create_detail.is_encrypted);
    create_detail.__set_worker_thread_number(_replica_job_create_detail.worker_thread_number);
    create_detail.__set_file_system_filter_enable(_replica_job_create_detail.file_system_filter_enable);
    create_detail.__set_min_transport_size(_replica_job_create_detail.min_transport_size);
    create_detail.__set_full_min_transport_size(_replica_job_create_detail.full_min_transport_size);
    create_detail.__set_is_checksum(_replica_job_create_detail.is_checksum);
    create_detail.__set_is_compressed(_replica_job_create_detail.is_compressed);
    create_detail.__set_priority_carrier(_replica_job_create_detail.priority_carrier);
    create_detail.__set_is_only_single_system_disk(_replica_job_create_detail.is_only_single_system_disk);
    create_detail.__set_is_compressed_by_packer(_replica_job_create_detail.is_compressed_by_packer);
    create_detail.__set_checksum_target(_replica_job_create_detail.checksum_target);
    create_detail.detail.p.__set_excluded_paths(excluded_paths);
    create_detail.detail.p.__set_resync_paths(resync_paths);

    try{
        if (job_id.empty())
            job_id = guid_::create();
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->create_packer_job_ex_p(_return, _session, _proxy_addr, job_id, create_detail);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->create_job_ex(_return, _session, job_id, create_detail);
            _packer->transport()->close();
        }
        if (_return.id.length()){
            job_id = _return.id;
            created_time = boost::posix_time::time_from_string(_return.created_time);
            result = true;
        }
        else{
            job_id.erase();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::remove_packer_job(const std::string job_id){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            result = _proxy->client()->remove_packer_job_p(_session, _proxy_addr, job_id);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            result = _packer->client()->remove_job(_session, job_id);
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::resume_packer_job(const std::string job_id){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            result = _proxy->client()->resume_packer_job_p(_session, _proxy_addr, job_id);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            result = _packer->client()->resume_job(_session, job_id);
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::interrupt_packer_job(const std::string job_id){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            result = _proxy->client()->interrupt_packer_job_p(_session, _proxy_addr, job_id);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            result = _packer->client()->interrupt_job(_session, job_id);
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::is_packer_job_running(const std::string job_id){
    bool result = false;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            result = _proxy->client()->running_packer_job_p(_session, _proxy_addr, job_id);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            result = _packer->client()->running_job(_session, job_id);
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::is_packer_job_existing(const std::string job_id){
    bool result = true;
    FUN_TRACE;
    try{
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->running_packer_job_p(_session, _proxy_addr, job_id);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->running_job(_session, job_id);
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        if (ex.what_op == saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND)
            result = false;
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

std::string physical_transport_job::physical_packer_job_op::get_machine_id()
{
    FUN_TRACE;
    std::string machine_id;
    try
    {
        physical_machine_info machine_info;
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->get_packer_host_detail_p(machine_info, _session, _proxy_addr, machine_detail_filter::SIMPLE);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->get_host_detail(machine_info, _session, machine_detail_filter::SIMPLE);
            _packer->transport()->close();
        }
        if (machine_info.machine_id.length()){
            machine_id = machine_info.machine_id;
        }
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return machine_id;
}

bool physical_transport_job::physical_packer_job_op::is_cdr_support(){
    bool result = false;
    FUN_TRACE;
    try
    {
        service_info          info;
        physical_machine_info machine_info;
        if (_proxy){
            _proxy->transport()->open();
            _proxy->client()->packer_ping_p(info, _session, _proxy_addr);
            _proxy->client()->get_packer_host_detail_p(machine_info, _session, _proxy_addr, machine_detail_filter::SIMPLE);
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            _packer->client()->ping(info);
            _packer->client()->get_host_detail(machine_info, _session, machine_detail_filter::SIMPLE);
            _packer->transport()->close();
        }
        if (info.version.length()){
            result = get_version(info.version) > get_version("1.1.800.0") && machine_info.is_vcbt_enabled && !machine_info.is_winpe && machine_info.os_name.find("Windows") != std::string::npos;
        }
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::can_connect_to_carriers(std::map<std::string, std::set<std::string> >& carriers, bool is_ssl){
    FUN_TRACE;
    bool result = false;
#ifndef string_set_map
    typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif
    try
    {
        if (_proxy){
            _proxy->transport()->open();
            foreach(string_set_map::value_type &c, carriers){
                foreach(std::string _c, c.second){
                    try{
                        if (result = _proxy->client()->verify_packer_carrier_p(_session, _proxy_addr,_c, is_ssl))
                            break;
                    }
                    catch (TException &ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    }
                }
                if (result)
                    break;
            }
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            foreach(string_set_map::value_type &c, carriers){
                foreach(std::string _c, c.second){
                    try{
                        if (result = _packer->client()->verify_carrier(_c, is_ssl))
                            break;
                    }
                    catch (TException &ex){
                        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    }
                }
                if (result)
                    break;
            }
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        if (std::string(ex.what()).find("Invalid method name:") != std::string::npos)
        {
            result = true;
            LOG(LOG_LEVEL_WARNING, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

bool physical_transport_job::physical_packer_job_op::can_connect_to_carriers(std::map<std::string, std::string>& carriers, bool is_ssl){
    FUN_TRACE;
    bool result = false;
#ifndef string_map
    typedef std::map<std::string, std::string> string_map;
#endif
    try
    {
        if (_proxy){
            _proxy->transport()->open();
            foreach(string_map::value_type &c, carriers){
                if (result = _proxy->client()->verify_packer_carrier_p(_session, _proxy_addr, c.second, is_ssl))
                    break;
            }
            _proxy->transport()->close();
        }
        else if (_packer){
            _packer->transport()->open();
            foreach(string_map::value_type &c, carriers){
                if (result = _packer->client()->verify_carrier(c.second, is_ssl))
                    break;
            }
            _packer->transport()->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex)
    {
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex)
    {
        if (std::string(ex.what()).find("Invalid method name:") != std::string::npos)
        {
            result = true;
            LOG(LOG_LEVEL_WARNING, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    if (_proxy){
        if (_proxy->transport() && _proxy->transport()->isOpen())
            _proxy->transport()->close();
    }
    else if (_packer){
        if (_packer->transport() && _packer->transport()->isOpen())
            _packer->transport()->close();
    }
    return result;
}

std::string physical_transport_job::_optimize_file_name(const std::string& name){
    std::string tokens = "\\/:*?\"<>| ";
    std::string result;
    std::vector<std::string> results = macho::stringutils::tokenize(name, tokens);
    foreach(std::string &s, results){
        if (result.length())
            result.append("_").append(s);
        else
            result = s;
    }
    return result;
}

void physical_transport_job::save_status(){
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    mObject job_status;
    mArray snapshots;
    foreach(saasame::transport::snapshot &s, _snapshots){
        mObject o;
        o["creation_time_stamp"] = s.creation_time_stamp;
        o["original_volume_name"] = s.original_volume_name;
        o["snapshots_count"] = s.snapshots_count;
        o["snapshot_device_object"] = s.snapshot_device_object;
        o["snapshot_id"] = s.snapshot_id;
        o["snapshot_set_id"] = s.snapshot_set_id;
        snapshots.push_back(o);
    }
    job_status["snapshots"] = snapshots;
    mArray progress;
    foreach(physical_transport_job_progress::map::value_type &p, _progress){
        mObject o;
        o["key"] = p.first;
        o["disk_uri"] = p.second->disk_uri;
        o["output"] = p.second->output;
        o["base"] = p.second->base;
        o["parent"] = p.second->parent;
        o["discard"] = p.second->discard;
        o["total_size"] = p.second->total_size;
        o["backup_progress"] = p.second->backup_progress;
        o["backup_size"] = p.second->backup_size;
        o["backup_image_offset"] = p.second->backup_image_offset;
        o["number"] = (int)p.second->number;
        o["friendly_name"] = p.second->friendly_name;
        mArray changeds;
        foreach(io_changed_range& r, p.second->cdr_changed_areas){
            mObject changed;
            changed["o"] = r.start;
            changed["l"] = r.length;
            changed["t"] = r.offset;
            changeds.push_back(changed);
        }
        o["cdr_changed_areas"] = changeds;
        mArray completeds;
        foreach(io_changed_range& r, p.second->completed_blocks){
            mObject completed;
            completed["o"] = r.start;
            completed["l"] = r.length;
            completed["t"] = r.offset;
            completeds.push_back(completed);
        }
        o["completed_blocks"] = completeds;
        progress.push_back(o);
    }
    job_status["progress"] = progress;
    mArray previous_journals;
    typedef std::map<int64_t, physical_vcbt_journal>  i64_physical_vcbt_journal_map;
    foreach(i64_physical_vcbt_journal_map::value_type &i, _previous_journals){
        mObject o;
        o["journal_id"] = i.second.id;
        o["first_key"] = i.second.first_key;
        o["latest_key"] = i.second.latest_key;
        o["lowest_valid_key"] = i.second.lowest_valid_key;
        previous_journals.push_back(o);
    }
    job_status["previous_journals"] = previous_journals;

    mArray cdr_journals;
    foreach(i64_physical_vcbt_journal_map::value_type &i, _cdr_journals){
        mObject o;
        o["journal_id"] = i.second.id;
        o["first_key"] = i.second.first_key;
        o["latest_key"] = i.second.latest_key;
        o["lowest_valid_key"] = i.second.lowest_valid_key;
        cdr_journals.push_back(o);
    }
    job_status["cdr_journals"] = cdr_journals;
    job_status["snapshot_info"] = _snapshot_info;
    job_status["priority_packer_addr"] = _priority_packer_addr;
    job_status["latest_connect_packer_time"] = boost::posix_time::to_simple_string(_latest_connect_packer_time);
    mArray  excluded_paths(_excluded_paths.begin(), _excluded_paths.end());
    job_status["excluded_paths"] = excluded_paths;
    mArray  resync_paths(_resync_paths.begin(), _resync_paths.end());
    job_status["resync_paths"] = resync_paths;
    transport_job::save_status(job_status);
}

bool physical_transport_job::load_status(std::wstring status_file){
    FUN_TRACE;
    mValue job_status;
    if (transport_job::load_status(status_file, job_status)){
        mArray snapshots = find_value(job_status.get_obj(), "snapshots").get_array();
        foreach(mValue &s, snapshots){
            saasame::transport::snapshot snapshot;
            snapshot.creation_time_stamp = find_value(s.get_obj(), "creation_time_stamp").get_str();
            snapshot.original_volume_name = find_value(s.get_obj(), "original_volume_name").get_str();
            snapshot.snapshots_count = find_value(s.get_obj(), "snapshots_count").get_int();
            snapshot.snapshot_device_object = find_value(s.get_obj(), "snapshot_device_object").get_str();
            snapshot.snapshot_id = find_value(s.get_obj(), "snapshot_id").get_str();
            snapshot.snapshot_set_id = find_value(s.get_obj(), "snapshot_set_id").get_str();
            _snapshots.push_back(snapshot);
        }
        mArray progress = find_value(job_status.get_obj(), "progress").get_array();
        foreach(mValue &p, progress){
            physical_transport_job_progress::ptr ptr = physical_transport_job_progress::ptr(new physical_transport_job_progress());
            ptr->disk_uri = find_value(p.get_obj(), "disk_uri").get_str();
            ptr->output = find_value(p.get_obj(), "output").get_str();
            ptr->discard = find_value_string(p.get_obj(), "discard");
            ptr->base = find_value_string(p.get_obj(), "base");
            ptr->parent = find_value(p.get_obj(), "parent").get_str();
            ptr->total_size = find_value(p.get_obj(), "total_size").get_int64();
            ptr->backup_image_offset = find_value(p.get_obj(), "backup_image_offset").get_int64();
            ptr->backup_progress = find_value(p.get_obj(), "backup_progress").get_int64();
            ptr->backup_size = find_value(p.get_obj(), "backup_size").get_int64();
            ptr->number = find_value(p.get_obj(), "number").get_int();
            ptr->friendly_name = find_value(p.get_obj(), "friendly_name").get_str();
            mArray changeds = find_value_array(p.get_obj(), "cdr_changed_areas");
            foreach(mValue c, changeds){
                io_changed_range changed;
                changed.start = find_value(c.get_obj(), "o").get_int64();
                changed.length = find_value(c.get_obj(), "l").get_int64();
                changed.offset = find_value(c.get_obj(), "t").get_int64();
                ptr->cdr_changed_areas.push_back(changed);
            }
            mArray  completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
            foreach(mValue c, completed_blocks){
                io_changed_range completed;
                completed.start = find_value(c.get_obj(), "o").get_int64();
                completed.length = find_value(c.get_obj(), "l").get_int64();
                completed.offset = find_value(c.get_obj(), "t").get_int64();
                ptr->completed_blocks.push_back(completed);
            }
            _progress[ptr->disk_uri = find_value(p.get_obj(), "key").get_str()] = ptr;
        }
        
        mArray  previous_journals = find_value(job_status.get_obj(), "previous_journals").get_array();
        foreach(mValue i, previous_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            _previous_journals[journal.id] = journal;
        }

        mArray  cdr_journals = find_value_array(job_status.get_obj(), "cdr_journals");
        foreach(mValue i, cdr_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            _cdr_journals[journal.id] = journal;
        }

        _snapshot_info = find_value_string(job_status.get_obj(), "snapshot_info");
        _priority_packer_addr = find_value_string(job_status.get_obj(), "priority_packer_addr");
        std::string latest_connect_packer_time_str = find_value_string(job_status.get_obj(), "latest_connect_packer_time");
        if (0 == latest_connect_packer_time_str.length() || latest_connect_packer_time_str == "not-a-date-time")
            _latest_connect_packer_time = boost::date_time::not_a_date_time;
        else
            _latest_connect_packer_time = boost::posix_time::time_from_string(latest_connect_packer_time_str);
        mObject job_status_obj = job_status.get_obj();
        if (job_status_obj.end() != job_status_obj.find("excluded_paths")){
            mArray  excluded_paths = find_value(job_status_obj, "excluded_paths").get_array();
            foreach(mValue p, excluded_paths){
                _excluded_paths.insert(p.get_str());
            }
        }
        if (job_status_obj.end() != job_status_obj.find("resync_paths")){
            mArray  resync_paths = find_value(job_status_obj, "resync_paths").get_array();
            foreach(mValue p, resync_paths){
                _resync_paths.insert(p.get_str());
            }
        }
        return true;
    }
    return false;
}

bool physical_transport_job::update_packer_job_progress(packer_job_detail &packer_detail){
    bool need_update = false;
    typedef std::map<std::string, int64_t> str_int64_map;

    foreach(str_int64_map::value_type p, packer_detail.detail.p.original_size){
        _progress[p.first]->total_size = p.second;
    }
    foreach(str_int64_map::value_type p, packer_detail.detail.p.backup_progress){
        _progress[p.first]->backup_progress = p.second;
    }
    foreach(str_int64_map::value_type p, packer_detail.detail.p.backup_image_offset){
        if (_progress[p.first]->backup_image_offset != p.second)
            need_update = true;
        _progress[p.first]->backup_image_offset = p.second;
    }
    foreach(str_int64_map::value_type p, packer_detail.detail.p.backup_size){
        _progress[p.first]->backup_size = p.second;
    }

    typedef std::map<std::string, std::vector<io_changed_range>> io_changed_range_map;
    foreach(io_changed_range_map::value_type c, packer_detail.detail.p.cdr_changed_ranges){
        _progress[c.first]->cdr_changed_areas = c.second;
    }

    foreach(io_changed_range_map::value_type c, packer_detail.detail.p.completed_blocks){
        _progress[c.first]->completed_blocks = c.second;
    }

    if (!_snapshots.empty() && _snapshots[0].original_volume_name == "cdr")
        _cdr_journals = packer_detail.detail.p.vcbt_journals;

    return need_update;
}

saasame::transport::replica_job_detail physical_transport_job::get_job_detail(const boost::posix_time::ptime previous_updated_time, const std::string connection_id) {
    FUN_TRACE;
    saasame::transport::replica_job_detail job;
    macho::windows::auto_lock lock(_cs);
    job.id = macho::stringutils::convert_unicode_to_utf8(_id);
    job.__set_type(_create_job_detail.type);
    job.created_time = boost::posix_time::to_simple_string(_created_time);
    job.host = _replica_job_create_detail.host;
    job.disks = _replica_job_create_detail.disks;
    job.state = (job_state::type)(_state & ~mgmt_job_state::job_state_error);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    job.updated_time = boost::posix_time::to_simple_string(update_time);
    job.__set_is_pending_rerun(_is_pending_rerun);
    history_record::vtr histories;
    if (_is_initialized){
        std::string job_id;
        physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
        if (_packer_jobs.size() && !op.initialize(_priority_packer_addr)){
            if (_packer_jobs.count(connection_id)){
                job_id = _packer_jobs[connection_id];             
                if (_packers_update_time.count(job_id)){
                    boost::posix_time::time_duration duration(update_time - _packers_update_time[job_id]);
                    if (duration.total_seconds() >= 90){
                        _jobs_state[job_id].disconnected = _jobs_state[job_id].is_error = true;
                        if (_replica_job_create_detail.type == saasame::transport::job_type::physical_transport_type){
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Connection retry timeout to Packer."));
                        }
                        else{
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Connection retry timeout to Offline Packer."));
                        }
                    }
                }
            }
        }
        else{
            _latest_connect_packer_time = boost::posix_time::microsec_clock::universal_time();
            packer_job_detail packer_detail;
            if (_packer_jobs.size() && connection_id.length()){              
                if (_packer_jobs.count(connection_id)){
                    job_id = _packer_jobs[connection_id];
                    boost::posix_time::ptime query_time;
                    boost::posix_time::time_duration diff = boost::posix_time::seconds(0);
                    if (_packer_time_diffs.count(job_id) && _packer_previous_update_time.count(job_id)){
                        diff = _packer_time_diffs[job_id];
                        query_time = boost::posix_time::time_from_string(_packer_previous_update_time[job_id]);
                    }
                    else if (_packer_time_diffs.count(job_id)){
                        diff = _packer_time_diffs[job_id];
                        query_time = previous_updated_time - _packer_time_diffs[job_id];
                    }
                    else{
                        query_time = previous_updated_time;
                    }
                    if (!op.get_packer_job_detail(job_id, query_time, packer_detail)){
                        if (_packers_update_time.count(job_id)){
                            boost::posix_time::time_duration duration(update_time - _packers_update_time[job_id]);
                            if (duration.total_seconds() >= 180){
                                _jobs_state[job_id].disconnected = true;
                                if (_replica_job_create_detail.type == saasame::transport::job_type::physical_transport_type){
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Response timeout from Packer."));
                                }
                                else{
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Response timeout from Offline Packer."));
                                }
                            }
                        }
                        if (!op.is_packer_job_existing(job_id)){
                            job.is_error = true;
                            LOG(LOG_LEVEL_ERROR, L"Packer Job (%s) was missing.", job_id.c_str());
                        }
                    }
                    else{
                        if (_jobs_state[job_id].disconnected){
                            if (_replica_job_create_detail.type == saasame::transport::job_type::physical_transport_type){
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Resume connection to Packer."));
                            }
                            else{
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Resume connection to Offline Packer."));
                            }
                        }
                        _jobs_state[job_id].disconnected = false;
                        _packers_update_time[job_id] = update_time;
                        _packer_previous_update_time[job_id] = packer_detail.updated_time;
                        job.is_error = packer_detail.is_error;
                       
                        _boot_disk = job.boot_disk = packer_detail.boot_disk;
                        _system_disks = job.system_disks = packer_detail.system_disks;
                        _jobs_state[job_id].state = packer_detail.state;
                        _jobs_state[job_id].is_error = packer_detail.is_error;

                        _need_update = update_packer_job_progress(packer_detail);

                        foreach(job_history h, packer_detail.histories){
                            history_record::ptr _h = history_record::ptr(new history_record(h.time, h.state, h.error, h.description));
                            _h->time += diff;
                            _h->format = h.format;
                            _h->is_display = h.is_display;
                            _h->args = h.arguments;
                            _h->is_display = h.is_display;
                            histories.push_back(_h);
                        }
                        merge_histories(packer_detail.histories, diff);
                    }
                    if (!_replica_job_create_detail.targets.empty())
                        job.replica_id = _replica_job_create_detail.targets.begin()->second;

                    job.connection_id = connection_id;

                    if (_snapshots.size() &&
                        (_state & mgmt_job_state::job_state_replicating ||
                        _state & mgmt_job_state::job_state_replicated ||
                        _state & mgmt_job_state::job_state_sche_completed ||
                        _state & mgmt_job_state::job_state_finished)){
                        job.snapshot_time = _snapshots[0].creation_time_stamp;
                        job.snapshot_info = _snapshots[0].snapshot_set_id;
                    }

                    foreach(physical_transport_job_progress::map::value_type p, _progress){
                        std::string image_snapshot_name = p.second->output;
                        if (!p.second->discard.empty()&&(p.second->parent == p.second->output))
                            image_snapshot_name = p.second->discard;
                        if (_replica_job_create_detail.disk_ids.size()){
                            job.snapshot_mapping[_replica_job_create_detail.disk_ids[p.first]] = image_snapshot_name;
                        }
                        else{
                            job.snapshot_mapping[p.first] = image_snapshot_name;
                        }
                    }
                    if (_previous_journals.size()){
                        mObject obj;
                        mArray previous_journals;
                        typedef std::map<int64_t, physical_vcbt_journal>  i64_physical_vcbt_journal_map;
                        foreach(i64_physical_vcbt_journal_map::value_type &i, _previous_journals){
                            mObject o;
                            o["journal_id"] = i.second.id;
                            o["first_key"] = i.second.first_key;
                            o["latest_key"] = i.second.latest_key;
                            o["lowest_valid_key"] = i.second.lowest_valid_key;
                            previous_journals.push_back(o);
                        }
                        obj["previous_journals"] = previous_journals;
                        job.cbt_info = write(obj, json_spirit::raw_utf8);
                    }
                    foreach(physical_transport_job_progress::map::value_type p, _progress){
                        if (_replica_job_create_detail.disk_ids.count(p.first)){
                            job.original_size[_replica_job_create_detail.disk_ids[p.first]] = p.second->total_size;
                            job.backup_size[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_size;
                            job.backup_progress[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_progress;
                            job.backup_image_offset[_replica_job_create_detail.disk_ids[p.first]] = p.second->backup_image_offset;
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"Invalid replica_job_create_detail format.");
                            saasame::transport::invalid_operation error;
                            error.what_op = saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL;
                            error.why = "Invalid replica_job_create_detail format.";
                            throw error;
                            //    job.original_size[p.first] = p.second->total_size;
                            //    job.backup_size[p.first] = p.second->backup_size;
                            //    job.backup_progress[p.first] = p.second->backup_progress;
                            //    job.backup_image_offset[p.first] = p.second->backup_image_offset;
                        }                       
                    }
                    job.__set_excluded_paths(_excluded_paths);
                }
            }
        }
    }
    foreach(history_record::ptr &h, _histories){
        if (h->time >= previous_updated_time){
            histories.push_back(h);
        }
    }
    foreach(history_record::ptr &h, histories){
        saasame::transport::job_history _h;
        _h.state = h->state;
        _h.error = h->error;
        _h.description = h->description;
        _h.time = boost::posix_time::to_simple_string(h->time);
        _h.format = h->format;
        _h.is_display = h->is_display;
        _h.arguments = h->args;
        _h.__set_arguments(_h.arguments);
        bool exist = false;
        BOOST_REVERSE_FOREACH(job_history p, job.histories){
            if (_h.time == p.time && 0 == strcmp(_h.description.c_str(), p.description.c_str())){
                exist = true;
                break;
            }
        }
        if (!exist){
            job.histories.push_back(_h);
        }
    }
    if ((_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error)
        job.is_error = true;
    
    job.is_cdr = is_cdr_trigger();

    save_status();
    return job;
}

void physical_transport_job::cancel(){
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){
    }
    else{
        physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
        if (!op.initialize(_priority_packer_addr)){
        }
        else{
            _latest_connect_packer_time = boost::posix_time::microsec_clock::universal_time();
            bool result;
            macho::windows::auto_lock lock(_cs);
            typedef std::map<std::string, std::string> string_map;
            for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
                result = false;
                if (!(result = op.interrupt_packer_job(job->second)))
                    break;
            }
        }
    }
    _is_interrupted = true;
    _is_canceled = true;
    save_status();
}

void physical_transport_job::interrupt(){
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"(%s)Job interrupt event captured.", _id.c_str());
    _is_interrupted = true;
    save_status();
}

void  physical_transport_job::get_previous_journals(const std::string &cbt_info){
    if (cbt_info.length()){
        std::stringstream is;
        is << cbt_info;
        mValue cbt_config;
        read(is, cbt_config);
        mArray  previous_journals = find_value(cbt_config.get_obj(), "previous_journals").get_array();
        foreach(mValue i, previous_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            _previous_journals[journal.id] = journal;
        }
    }
}

bool physical_transport_job::remove_snapshots(physical_packer_job_op &op){
    FUN_TRACE;
    bool result = false;
    if (_snapshots.size()){
        delete_snapshot_result r;
        carrier_rw::set_session_buffer(_replica_job_create_detail.carriers, _snapshots[0].snapshot_set_id, 0, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
        if (op.delete_snapshot_set(_snapshots[0].snapshot_set_id, r) && (r.deleted_snapshots == _snapshots.size() || (r.deleted_snapshots == 0 && r.non_deleted_snapshot_id.empty()))){
            _snapshots.clear();
            result = true;
        }
    }
    else
        result = true;
    return result;
}

bool physical_transport_job::packer_jobs_exist(physical_packer_job_op &op)
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    bool result = _packer_jobs.size() > 0;
    typedef std::map<std::string, std::string> string_map;
    for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++)
    {
        LOG(LOG_LEVEL_RECORD, L"(%s)checking  packer job...", _id.c_str());
        if (result = op.is_packer_job_existing(job->second))
        {
            LOG(LOG_LEVEL_RECORD, L"(%s)packer job exists.", _id.c_str());
            break;
        }
    }
    return result;
}

bool physical_transport_job::is_packer_job_running(physical_packer_job_op &op){
    bool result = false;
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    typedef std::map<std::string, std::string> string_map;
    for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
        if (result = op.is_packer_job_running(job->second)){
            break;
        }
    }
    return result;
}

bool physical_transport_job::resume_packer_jobs(physical_packer_job_op &op){
    FUN_TRACE;
    bool result = false;
    bool _updated = false;
    macho::windows::auto_lock lock(_cs);
    if (packer_jobs_exist(op)){
        typedef std::map<std::string, std::string> string_map;
        for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
            if (!_updated){
                packer_job_detail detail;
                if (result = op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                    if (detail.is_error || detail.state != job_state::type::job_state_finished){
                        if (!(result = op.resume_packer_job(job->second))){
                            break;
                        }
                    }
                    else if (detail.state & job_state::type::job_state_finished){
                        result = false;
                        _updated = true;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

bool physical_transport_job::remove_packer_jobs(physical_packer_job_op &op, bool erase_all){
    bool result = true;
    FUN_TRACE;
    bool previous_journals_updated = false;
    typedef std::map<std::string, std::string> string_map;

    if (packer_jobs_exist(op) && (result = _running.try_lock())){
        LOG(LOG_LEVEL_RECORD, L"(%s)removing packer job...", _id.c_str());

        string_map packer_jobs;
        {
            macho::windows::auto_lock lock(_cs);
            packer_jobs = _packer_jobs;
        }

        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
            if (!previous_journals_updated){
                packer_job_detail detail;
                if (result = op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                    if ( detail.is_error || detail.state != job_state::type::job_state_finished){
                        //discard images
                        if (erase_all){
                            while (op.is_packer_job_running(job->second)){
                                result = op.interrupt_packer_job(job->second);
                                boost::this_thread::sleep(boost::posix_time::seconds(1));
                            }
                            std::map<std::string, std::set<std::string> > carriers = _replica_job_create_detail.carriers;
                            std::map<std::string, std::string > priority_carrier = _replica_job_create_detail.priority_carrier;
                            if (result){
                                foreach(string_set_map::value_type &c, carriers){
                                    std::string carrier_id = carrier_rw::get_machine_id(c.second);
                                    if (carrier_id.empty())
                                    {
                                        result = false;
                                        break;
                                    }
                                }
                            }
                            if (result){
                                bool has_synced_data = false;
                                std::string session = _snapshot_info;
                                foreach(physical_transport_job_progress::map::value_type p, _progress){
                                    try{
                                        if (detail.detail.p.backup_image_offset[p.first] != 0)
                                            has_synced_data = true;

                                        if (p.second->discard.empty()){
                                            result = false;
                                            carrier_rw::open_image_parameter parameter;
                                            parameter.carriers = carriers;
                                            parameter.priority_carrier = priority_carrier;
                                            parameter.base_name = p.second->base;
                                            parameter.name = p.second->output;
                                            parameter.session = session;
                                            parameter.timeout = _replica_job_create_detail.timeout;
                                            parameter.encrypted = _replica_job_create_detail.is_encrypted;
                                            universal_disk_rw::vtr outputs = carrier_rw::open(parameter);
                                            //universal_disk_rw::vtr outputs = carrier_rw::open(carriers, p.second->base, p.second->output, session, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                                            foreach(universal_disk_rw::ptr o, outputs){
                                                carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                                                if (rw){
                                                    result = rw->close(true);
                                                }
                                            }
                                            if (result){
                                                p.second->discard = p.second->output;
                                                p.second->output = p.second->parent;
                                            }
                                        }
                                    }
                                    catch (...){
                                        result = detail.detail.p.backup_image_offset[p.first] == 0;
                                        if (result){
                                            p.second->discard = p.second->output;
                                            p.second->output = p.second->parent;
                                        }
                                    }
                                    if (!result)
                                        break;
                                }
                                if (result){
                                    if (!has_synced_data)
                                        result = discard_snapshot(_snapshot_info);
                                    if (result){
                                        _state = job_state::type::job_state_discard;
                                        if (_replica_job_create_detail.block_mode_enable && has_synced_data){
                                            result = false;
                                            int count = 100;
                                            while (!_is_interrupted && count > 0 && !(result = is_snapshot_ready(_snapshot_info))){
                                                boost::this_thread::sleep(boost::posix_time::seconds(5));
                                                count--;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (detail.state & job_state::type::job_state_finished){
                        _state = mgmt_job_state::job_state_replicated;
                        _previous_journals = detail.detail.p.vcbt_journals;
                        previous_journals_updated = true;
                        save_status();
                        result = true;
                    }
                    update(false);
                }
            }
            if (result){
                if (!(result = op.remove_packer_job(job->second)))
                    break;
                else{
                    macho::windows::auto_lock lock(_cs);
                    _packer_time_diffs.erase(job->second);
                    _packer_previous_update_time.erase(job->second);
                    _jobs_state.erase(job->second);
                    _packer_jobs.erase(job->first);
                    if (_state & mgmt_job_state::job_state_replicated)
                        _state = mgmt_job_state::job_state_sche_completed;
                }
            }
        }
        save_status();
        _running.unlock();
    }
    return result;
}

void physical_transport_job::execute(){
    VALIDATE;
    FUN_TRACE;
    if (_is_canceled || _is_removing){
        return;
    }
    bool is_get_replica_job_create_detail = false;
    if (!_is_initialized) 
        is_get_replica_job_create_detail = _is_initialized = get_replica_job_create_detail(_replica_job_create_detail);
    else
        is_get_replica_job_create_detail = get_replica_job_create_detail(_replica_job_create_detail);
    save_config();
    save_status();
    if ( !is_get_replica_job_create_detail ){
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot get the replica job detail."));
        _state |= mgmt_job_state::job_state_error;
        save_status();
    }
    else if (!_replica_job_create_detail.is_paused){
        try{
            uint32_t concurrent_count = boost::thread::hardware_concurrency();
            registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"ConcurrentSchedulerJobNumber"].exists() && (DWORD)reg[L"ConcurrentSchedulerJobNumber"] > 0){
                    concurrent_count = ((DWORD)reg[L"ConcurrentSchedulerJobNumber"]);
                }
            }
            macho::windows::semaphore  scheduler(L"Global\\Transport", concurrent_count);
            _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
            _latest_enter_time = boost::posix_time::microsec_clock::universal_time(); 
            _terminated = false;
            _thread = boost::thread(&transport_job::update, this, true);
            _state &= ~mgmt_job_state::job_state_error;
            physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
            if (!op.initialize(_priority_packer_addr)){
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST, record_format("Cannot connect to the physical host."));
            }
            else if (!(op.can_connect_to_carriers(_replica_job_create_detail.priority_carrier, _replica_job_create_detail.is_encrypted) || op.can_connect_to_carriers(_replica_job_create_detail.carriers, _replica_job_create_detail.is_encrypted))){
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("The Packer cannot connect to the Carrier."));
            }
            else if (_will_be_resumed = !scheduler.trylock()){
                LOG(LOG_LEVEL_WARNING, L"Resource limited. Job %s will be resumed later.", _id.c_str());
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_QUEUE_FULL,
                    record_format("Pending for available job resource."));
            }
            else{
                _latest_connect_packer_time = boost::posix_time::microsec_clock::universal_time();
                _is_interrupted = false;
                _is_source_ready = true;
                if (_state & mgmt_job_state::job_state_none){
                    if (_previous_journals.empty()){
                        get_previous_journals(_replica_job_create_detail.cbt_info);
                    }
                }
                if (_is_continuous_data_replication != _replica_job_create_detail.is_continuous_data_replication){
                    if (_replica_job_create_detail.is_continuous_data_replication && op.is_cdr_support()){
                        _is_continuous_data_replication = true;
                    }
                    else{
                        _is_continuous_data_replication = false;
                    }
                    if ((!_enable_cdr_trigger && _is_continuous_data_replication) || (!_is_continuous_data_replication && _enable_cdr_trigger))
                        update_job_triggers();          
                }
                save_status();

                if (_state & mgmt_job_state::job_state_replicating){ // Error handling for the re-enter job
                    //Checking snapshoting and job ....
                    if (!is_packer_job_running(op)){
                        bool is_source_snapshot_ready = false;
                        //source snapshot is ready or not...
                        if (!_snapshot_info.empty()){
                            if (!_snapshots.empty() && _snapshots[0].original_volume_name == "cdr"){
                                is_source_snapshot_ready = (_cdr_journals.size() && _is_continuous_data_replication && is_cdr_trigger());
                            }
                            else{
                                typedef  std::map<std::string, std::vector<snapshot> > snapshot_map;
                                std::map<std::string, std::vector<snapshot> > snapshots;
                                if (op.get_all_snapshots(snapshots)){
                                    delete_snapshot_result result;
                                    foreach(snapshot_map::value_type s, snapshots){
                                        if (s.first == _snapshot_info){
                                            is_source_snapshot_ready = true;
                                            break;
                                        }
                                    }
                                }

                                if (is_source_snapshot_ready && is_runonce_trigger()){
                                    for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
                                        packer_job_detail detail;
                                        if (op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                                            if (detail.is_error && (detail.state & job_state::type::job_state_discard) == job_state::type::job_state_discard){
                                                is_source_snapshot_ready = false;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        if (is_source_snapshot_ready){
                            if (remove_packer_jobs(op, false)){
                                if (_state & mgmt_job_state::job_state_sche_completed){
                                    remove_snapshots(op);
                                }
                                else{
                                    macho::windows::auto_lock lock(_cs);
                                    _state = mgmt_job_state::job_state_initialed;
                                }
                            }
                        }
                        else{
                            if (remove_packer_jobs(op, true)){
                                remove_snapshots(op);
                                if (!(_state & mgmt_job_state::job_state_sche_completed)){
                                    macho::windows::auto_lock lock(_cs);
                                    _state = mgmt_job_state::job_state_none;
                                    _cdr_journals.clear();
                                    foreach(physical_transport_job_progress::map::value_type p, _progress){
                                        p.second->backup_image_offset = 0;
                                        p.second->backup_progress = 0;
                                        p.second->backup_size = 0;
                                        p.second->cdr_changed_areas.clear();
                                    }
                                }
                            }
                        }
                    }
                }

                if (_state & mgmt_job_state::job_state_replicated) {
                    //Remove snapshots and job when replication was finished.                
                    if (remove_snapshots(op) && remove_packer_jobs(op, true)){
                        _state = mgmt_job_state::job_state_sche_completed;
                    }
                    save_status();
                }
            
                if (_state & mgmt_job_state::job_state_sche_completed){
                    if (_update(true)){
                        if (!_replica_job_create_detail.block_mode_enable || is_snapshot_ready(_snapshot_info)){
                            macho::windows::auto_lock lock(_cs);
                            _state = mgmt_job_state::job_state_none;
                            _cdr_journals.clear();
                            foreach(physical_transport_job_progress::map::value_type p, _progress){
                                p.second->backup_image_offset = 0;
                                p.second->backup_progress = 0;
                                p.second->backup_size = 0;
                                p.second->cdr_changed_areas.clear();
                            }
                        }
                        else {
                            LOG(LOG_LEVEL_WARNING, L"(%s)Snapshot(%s) is not ready.", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(_snapshot_info).c_str());
                        }
                    }
                    else{
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot connect to the management server."));
                    }
                }

                if (_state & mgmt_job_state::job_state_none){
                    _is_pending_rerun = false;
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Carrier job started."));
                    if (_replica_job_create_detail.snapshot_info.length()){
                        delete_snapshot_result r;
                        op.delete_snapshot_set(_replica_job_create_detail.snapshot_info, r);
                    }
                    registry reg;     
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"EraseAllSnapshots"].exists() && (DWORD)reg[L"EraseAllSnapshots"] > 0){
                            typedef  std::map<std::string, std::vector<snapshot> > snapshot_map;
                            std::map<std::string, std::vector<snapshot> > snapshots;
                            if (op.get_all_snapshots(snapshots)){
                                delete_snapshot_result result;
                                foreach(snapshot_map::value_type s, snapshots)
                                    op.delete_snapshot_set(s.first, result);
                            }
                            reg[L"EraseAllSnapshots"] = 0;
                        }
                    }
                    //Initial checking 
                    std::set<disk_info> disks;
                    if (op.enumerate_disks(disks)){
                        bool found = false;
                        foreach(std::string uri, _replica_job_create_detail.disks){
                            disk_universal_identify disk_id(uri);
                            found = false;
                            foreach(disk_info d, disks){
                                if (found = (disk_id == disk_universal_identify(d.uri))){
                                    physical_transport_job_progress::ptr progress;
                                    if (!_progress.count(uri)){
                                        progress = physical_transport_job_progress::ptr(new physical_transport_job_progress());
                                        _progress[uri] = progress;
                                    }
                                    else{
                                        progress = _progress[uri];
                                    }
                                    progress->disk_uri = d.uri;
                                    progress->total_size = d.size;
                                    progress->number = d.number;
                                    progress->friendly_name = _optimize_file_name(d.friendly_name);
                                    break;
                                }
                            }
                            if (!found){
                                _state |= mgmt_job_state::job_state_error;
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_PHYSICAL_CONFIG_FAILED, (record_format("Cannot find disks from host %1%.") % _replica_job_create_detail.host));
                                break;
                            }
                        }
                        if (found){
                            saasame::transport::take_snapshots_parameters parameters;
                            parameters.disks = _replica_job_create_detail.disks;
                            parameters.pre_script = _replica_job_create_detail.pre_snapshot_script;
                            parameters.post_script = _replica_job_create_detail.post_snapshot_script;
                            parameters.excluded_paths = _replica_job_create_detail.excluded_paths;
                            if (_is_continuous_data_replication && is_cdr_trigger()){
                                _snapshots.clear();
                                saasame::transport::snapshot snap;
                                snap.creation_time_stamp = boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time());
                                snap.snapshots_count = 0;
                                snap.snapshot_set_id = macho::guid_::create();
                                snap.snapshot_id = macho::guid_(GUID_NULL);
                                snap.original_volume_name = "cdr";
                                snap.snapshot_device_object = "cdr";
                                _snapshots.push_back(snap);
                                _snapshot_info = _snapshots[0].snapshot_set_id;
                                _state = mgmt_job_state::job_state_initialed;
                            }
                            else if (op.take_snapshots(_snapshots, parameters) && _snapshots.size()){
                                _snapshot_info = _snapshots[0].snapshot_set_id;
                                _state = mgmt_job_state::job_state_initialed;
                                _excluded_paths = _replica_job_create_detail.excluded_paths;
                                _resync_paths.clear();
                                foreach(const std::string& p, _replica_job_create_detail.previous_excluded_paths){
                                    bool _removed = true;
                                    foreach(const std::string& _p, _replica_job_create_detail.excluded_paths){
                                        if (_p == p){
                                            _removed = false;
                                            break;
                                        }
                                    }
                                    if (_removed)
                                        _resync_paths.insert(p);
                                }
                            }
                            else{
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, (record_format("Cannot take snapshots of host %1%.") % _replica_job_create_detail.host));
                                _state |= mgmt_job_state::job_state_error;
                            }
                        }
                        save_status();
                    }
                }

                if (_state & mgmt_job_state::job_state_initialed){
                    // Create replication job._snapshot_info
                    std::set<std::string> connections;
                    physical_create_packer_job_detail detail;
                    std::string job_id = macho::stringutils::convert_unicode_to_utf8(_id);
                    foreach(string_map::value_type t, _replica_job_create_detail.targets)
                        connections.insert(t.first);

                    {
                        macho::windows::auto_lock lock(_cs);
                        foreach(physical_transport_job_progress::map::value_type p, _progress){
                            p.second->discard.clear();
                            if (p.second->base.length())
                                detail.images[p.first].base = p.second->base;
                            else{
                                if (_replica_job_create_detail.disk_ids.count(p.first))
                                    detail.images[p.first].base = p.second->base = _replica_job_create_detail.disk_ids[p.first];
                                else
                                    detail.images[p.first].base = p.second->base = boost::str(boost::format("%s_%d") % p.second->friendly_name %p.second->number);
                            }
                            if (p.second->backup_size == 0 && p.second->output != boost::str(boost::format("%s_%s") % p.second->base % _snapshot_info)){
                                detail.images[p.first].parent = p.second->parent = p.second->output;
                                detail.images[p.first].name = p.second->output = boost::str(boost::format("%s_%s") % p.second->base % _snapshot_info);
                            }
                            else{
                                detail.images[p.first].parent = p.second->parent;
                                detail.images[p.first].name = p.second->output;
                            }
                            if (p.second->backup_size != 0)
                                detail.backup_size[p.first] = p.second->backup_size;
                            if (p.second->backup_progress != 0)
                                detail.backup_progress[p.first] = p.second->backup_progress;
                            if (p.second->backup_image_offset != 0)
                                detail.backup_image_offset[p.first] = p.second->backup_image_offset;
                            if (p.second->cdr_changed_areas.size() != 0)
                                detail.cdr_changed_ranges[p.first] = p.second->cdr_changed_areas;
                            if (p.second->completed_blocks.size() != 0)
                                detail.completed_blocks[p.first] = p.second->completed_blocks;
                        }
                    }
                    if (_replica_job_create_detail.is_full_replica)
                        _previous_journals.clear();
                    if (_replica_job_create_detail.buffer_size)
                        carrier_rw::set_session_buffer(_replica_job_create_detail.carriers, _snapshot_info,_replica_job_create_detail.buffer_size, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                    boost::posix_time::ptime created_time;
                    detail.snapshots = _snapshots;
                    detail.previous_journals = _previous_journals;
                    if (op.create_packer_job(
                        connections,
                        _replica_job_create_detail,
                        detail,
                        _excluded_paths,
                        _resync_paths,
                        job_id, 
                        created_time, 
                        job_type::physical_packer_job_type)){
                        foreach(string_map::value_type t, _replica_job_create_detail.targets){
                            macho::windows::auto_lock lock(_cs);
                            _packer_jobs[t.first] = job_id;
                            _packer_time_diffs[job_id] = boost::posix_time::microsec_clock::universal_time() - created_time;
                            _packer_previous_update_time[job_id] = boost::posix_time::to_simple_string(created_time);
                            _jobs_state[job_id].state = job_state::type::job_state_none;
                        }
                        save_status();
                    }
                    else{
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_CREATE_FAIL, (record_format("Cannot create a Packer job for host %1%.") % _replica_job_create_detail.host));
                        if (_snapshots.size()){
                            delete_snapshot_result r;
                            if (op.delete_snapshot_set(_snapshots[0].snapshot_set_id, r) && (r.deleted_snapshots == _snapshots.size() || (r.deleted_snapshots == 0 && r.non_deleted_snapshot_id.empty()))){
                                _snapshots.clear();
                                _state = mgmt_job_state::job_state_none;
                            }
                        }
                        _state |= mgmt_job_state::job_state_error;
                        save_status();
                    }
                }

                if (!(_state & mgmt_job_state::job_state_error)){
                    if ((_state & mgmt_job_state::job_state_initialed) || (_state & mgmt_job_state::job_state_replicating)){
                        //checking replication status
                        _state = mgmt_job_state::job_state_replicating;
                        save_status();
                        bool is_error = false;
                        while ((!is_error) && (!_is_interrupted) && (!_is_canceled)){
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                            {
                                macho::windows::auto_lock lock(_cs);
                                bool finished = false;
                                typedef std::map<std::string, running_job_state>  job_state_map;
                                foreach(job_state_map::value_type s, _jobs_state){
                                    finished = false;
                                    if (s.second.is_error){
                                        _state |= mgmt_job_state::job_state_error;
                                        is_error = true;
                                        break;
                                    }
                                    else if (s.second.disconnected){
                                        is_error = true;
                                        break;
                                    }
                                    else if (!(finished = (job_state::type::job_state_finished == (s.second.state & job_state::type::job_state_finished))))
                                        break;
                                }
                                if (finished && _jobs_state.size() == _packer_jobs.size()){
                                    _state = mgmt_job_state::job_state_replicated;
                                    save_status();
                                    break;
                                }
                            }
                        }
                    }
                }

                if (_state & mgmt_job_state::job_state_replicated) {
                    //Remove snapshots and job when replication was finished.                
                    if (remove_snapshots(op) && remove_packer_jobs(op, true)){
                        _state = mgmt_job_state::job_state_sche_completed;
                    }
                    save_status();
                }
                else if (_is_canceled) {
                    //Remove snapshots and job when replication was finished.                
                    if (remove_packer_jobs(op, true) && remove_snapshots(op)){
                        _state = mgmt_job_state::job_state_sche_completed;
                    }
                    save_status();
                }
                else if (_state & mgmt_job_state::job_state_error){
                    boost::this_thread::sleep(boost::posix_time::seconds(15));
                    if (packer_jobs_exist(op) && !is_packer_job_running(op)){
                        if (!(_replica_job_create_detail.always_retry || 0 == _previous_journals.size() || (_is_continuous_data_replication && is_cdr_trigger()))){
                            if (remove_packer_jobs(op, true)){
                                if (remove_snapshots(op)){
                                    _state = mgmt_job_state::job_state_none;
                                    _cdr_journals.clear();
                                    foreach(physical_transport_job_progress::map::value_type p, _progress){
                                        p.second->backup_image_offset = 0;
                                        p.second->backup_progress = 0;
                                        p.second->backup_size = 0;
                                        p.second->cdr_changed_areas.clear();
                                    }
                                }
                            }
                            save_status();
                        }
                    }
                }
            }
        }
        catch (macho::exception_base& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
        }
        catch (const boost::filesystem::filesystem_error& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (const boost::exception &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        _latest_leave_time = boost::posix_time::microsec_clock::universal_time();
        save_status();
        _terminated = true;
        if (_thread.joinable())
            _thread.join();
    }
}

void physical_transport_job::remove(){
    FUN_TRACE;
    transport_job::remove();
}

bool physical_transport_job::need_to_be_resumed(){
    FUN_TRACE;
    bool result = false;
    if (_running.try_lock()){
        try{
            is_replica_job_alive();
            if ((_state & job_state_none) || (_state & mgmt_job_state::job_state_replicating)){
                physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
                if (op.initialize(_priority_packer_addr)){
                    _latest_connect_packer_time = boost::posix_time::microsec_clock::universal_time();
                    if (_will_be_resumed){
                        result = true;
                    }
                    else if (!(result = is_packer_job_running(op))){
                        string_map packer_jobs;
                        {
                            macho::windows::auto_lock lock(_cs);
                            packer_jobs = _packer_jobs;
                        }
                        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                            result = false;
                            packer_job_detail detail;
                            if (op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                                update(false);
                                if ((!detail.is_error) && detail.state == job_state::type::job_state_finished){
                                    if (remove_snapshots(op)){
                                        if (remove_packer_jobs(op, true)){
                                            _state = mgmt_job_state::job_state_sche_completed;
                                            save_status();
                                            update(false);
                                        }
                                    }
                                }
                                else if ((!(!_snapshots.empty() && _snapshots[0].original_volume_name == "cdr")) && 
                                    ((detail.state & job_state::type::job_state_discard) != job_state::type::job_state_discard)){
                                        result = _replica_job_create_detail.always_retry;
                                }
                                break;
                            }
                        }
                    }
                    else{
                        string_map packer_jobs;
                        {
                            macho::windows::auto_lock lock(_cs);
                            packer_jobs = _packer_jobs;
                        }
                        for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                            result = false;
                            packer_job_detail detail;
                            if (op.get_packer_job_detail(job->second, boost::posix_time::microsec_clock::universal_time(), detail)){
                                update(false);
                                break;
                            }
                        }
                    }
                }
            }
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"(%s)Unkonwn exception.", _id.c_str());
        }
        _running.unlock();
    }
    return result;
}

bool physical_transport_job::prepare_for_job_removing(){
    FUN_TRACE;
    bool result = false;
    if ((result = _running.try_lock()) && _is_source_ready && (!_replica_job_create_detail.addr.empty())){
        std::map<std::string, std::set<std::string> > carriers = _replica_job_create_detail.carriers;
        std::map<std::string, std::string > priority_carrier = _replica_job_create_detail.priority_carrier;
        physical_packer_job_op op(_replica_job_create_detail.addr, _create_job_detail.mgmt_addr);
        if (!(op.initialize(_priority_packer_addr))){
            foreach(string_set_map::value_type &c, carriers){
                std::string carrier_id = carrier_rw::get_machine_id(c.second);
                if (carrier_id.empty())
                {
                    result = false;
                    break;
                }
            }
            std::map<std::string, std::set<std::string>> missing_connections;
            if (result && carrier_rw::are_connections_ready(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted, missing_connections)){
                foreach(string_set_map::value_type &c, missing_connections){
                    foreach(std::string addr, c.second)
                        carriers[c.first].erase(addr);
                    if (carriers[c.first].empty())
                        carriers.erase(c.first);
                }			
                foreach(physical_transport_job_progress::map::value_type p, _progress){
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->discard, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->output, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted))){
                        foreach(physical_transport_job_progress::map::value_type p, _progress){
                            try{
                                if (p.second->discard.empty()){
                                    carrier_rw::open_image_parameter parameter;
                                    parameter.carriers = carriers;
                                    parameter.priority_carrier = priority_carrier;
                                    parameter.base_name = p.second->base;
                                    parameter.name = p.second->output;
                                    parameter.session = _snapshot_info;
                                    parameter.timeout = _replica_job_create_detail.timeout;
                                    parameter.encrypted = _replica_job_create_detail.is_encrypted;
                                    universal_disk_rw::vtr outputs = carrier_rw::open(parameter);
                                    //universal_disk_rw::vtr outputs = carrier_rw::open(carriers, p.second->base, p.second->output, _snapshot_info, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                                    foreach(universal_disk_rw::ptr o, outputs){
                                        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                                        if (rw){
                                            rw->close(true);
                                        }
                                    }
                                }
                            }
                            catch (...){
                            }
                        }
                        break;
                    }
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->parent, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                }
            }
            if (result){          
                result = (_create_job_detail.type == saasame::transport::job_type::winpe_transport_job_type) || (_latest_connect_packer_time == boost::date_time::not_a_date_time);
                if (!result){
                    boost::gregorian::date_duration dates = boost::posix_time::second_clock::universal_time().date() - _latest_connect_packer_time.date();
                    int expiry_days = 14;
                    if (result = dates.days() > expiry_days){
                        LOG(LOG_LEVEL_WARNING, L"Cannot connect to the packer > %d days (%s).", expiry_days, _id.c_str());
                    }
                }
            }
            if (result){
                carrier_rw::remove_connections(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
            }
        }
        else{
            _latest_connect_packer_time = boost::posix_time::microsec_clock::universal_time();
            bool _is_packer_job_running = false;
            if (_packer_jobs.size()){
                macho::windows::auto_lock lock(_cs);
                for (string_map::iterator job = _packer_jobs.begin(); job != _packer_jobs.end(); job++){
                    if (op.is_packer_job_running(job->second)){
                        _is_packer_job_running = true;
                        op.interrupt_packer_job(job->second);
                    }
                }
            }
            result = remove_snapshots(op);
            if (_packer_jobs.size()){
                if (result && (result = (!_is_packer_job_running))){
                    string_map packer_jobs;
                    {
                        macho::windows::auto_lock lock(_cs);
                        packer_jobs = _packer_jobs;
                    }

                    for (string_map::iterator job = packer_jobs.begin(); job != packer_jobs.end(); job++){
                        if (op.is_packer_job_existing(job->second))
                        {
                            while (op.is_packer_job_running(job->second)){
                                result = op.interrupt_packer_job(job->second);
                                boost::this_thread::sleep(boost::posix_time::seconds(1));
                            }

                            if (!(result = op.remove_packer_job(job->second)))
                                break;
                            else{
                                macho::windows::auto_lock lock(_cs);
                                _packer_time_diffs.erase(job->second);
                                _packer_previous_update_time.erase(job->second);
                                _jobs_state.erase(job->second);
                                _packer_jobs.erase(job->first);
                                save_status();
                            }
                        }
                        else{
                            macho::windows::auto_lock lock(_cs);
                            _packer_time_diffs.erase(job->second);
                            _packer_previous_update_time.erase(job->second);
                            _jobs_state.erase(job->second);
                            _packer_jobs.erase(job->first);
                            save_status();
                        }
                    }
                }
            }
            if (result){
                foreach(string_set_map::value_type &c, carriers){
                    std::string carrier_id = carrier_rw::get_machine_id(c.second);
                    if (carrier_id.empty())
                    {
                        result = false;
                        break;
                    }
                }
            }

            std::map<std::string, std::set<std::string>> missing_connections;
            if (result && carrier_rw::are_connections_ready(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted, missing_connections)){
                foreach(string_set_map::value_type &c, missing_connections){
                    foreach(std::string addr, c.second)
                        carriers[c.first].erase(addr);
                    if (carriers[c.first].empty())
                        carriers.erase(c.first);
                }
                foreach(physical_transport_job_progress::map::value_type p, _progress){
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->discard, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->output, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted))){
                        foreach(physical_transport_job_progress::map::value_type p, _progress){
                            try{
                                if (p.second->discard.empty()){
                                    carrier_rw::open_image_parameter parameter;
                                    parameter.carriers = carriers;
                                    parameter.priority_carrier = priority_carrier;
                                    parameter.base_name = p.second->base;
                                    parameter.name = p.second->output;
                                    parameter.session = _snapshot_info;
                                    parameter.timeout = _replica_job_create_detail.timeout;
                                    parameter.encrypted = _replica_job_create_detail.is_encrypted;
                                    universal_disk_rw::vtr outputs = carrier_rw::open(parameter);
                                    //universal_disk_rw::vtr outputs = carrier_rw::open(carriers, p.second->base, p.second->output, _snapshot_info, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                                    foreach(universal_disk_rw::ptr o, outputs){
                                        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                                        if (rw){
                                            rw->close(true);
                                        }
                                    }                 
                                }
                            }
                            catch (...){
                            }
                        }
                        break;
                    }
                    if (!(result = carrier_rw::remove_snapshost_image(carriers, p.second->base, p.second->parent, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted)))
                        break;
                }
                if (result){
                    carrier_rw::remove_connections(carriers, _replica_job_create_detail.timeout, _replica_job_create_detail.is_encrypted);
                }
            }                     
        }
        save_status();
        _running.unlock();
    }
    return result;
}