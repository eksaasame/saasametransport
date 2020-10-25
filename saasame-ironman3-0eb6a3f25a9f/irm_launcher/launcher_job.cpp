#include "launcher_job.h"
#include "common_service.h"
#include "launcher_service.h"
#include "management_service.h"
#include <codecvt>
#include "..\irm_converter\bcd_edit.h"
#include "..\irm_converter\irm_converter.h"
#include "..\gen-cpp\network_settings.h"
#include "..\irm_converter\irm_disk.h"
#include "..\gen-cpp\mgmt_op.h"
#include <boost/algorithm/string/predicate.hpp>
#include "..\irm_host_mgmt\vhdx.h"
#include "vhdtool.h"
#include "..\gen-cpp\service_op.h"
#include "shrink_volume.h"
#include <boost/range/adaptor/reversed.hpp>
#include <regex>

#undef MACHO_HEADER_ONLY
#include "windows\ssh_client.hpp"
#define MACHO_HEADER_ONLY
#define BUFF_BLOCK_SZ        0x800000
#define MAX_BLOCK_SIZE       8388608UL
#define WAIT_INTERVAL_SECONDS   2
#include "..\ntfs_parser\azure_blob_op.h"
#ifdef _VMWARE_VADP
#include "vmware.h"
#include "vmware_ex.h"
#include "common.h"
#include "difxapi.h"

#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_MINIPORT_PROCESS_SERVICE_IRP CTL_CODE(IOCTL_SCSI_BASE,  0x040e, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#pragma comment( lib, "difxapi.lib" )
#pragma comment(lib, "irm_hypervisor_ex.lib")
using namespace mwdc::ironman::hypervisor_ex;
#endif

using namespace json_spirit;
using namespace macho::windows;
using namespace macho;
#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif
typedef std::map<saasame::transport::hv_controller_type::type, std::vector<std::string> > scsi_adapter_disks_map;
#pragma comment(lib, "irm_converter.lib")

#if defined(_VMWARE_VADP) || defined(_AZURE_BLOB)
#define BUFF_SIZE  4 * 1024 * 1024
#define RETRY_COUNT 3
#define BUFF_NUM 3

struct device_buffer{
    typedef boost::shared_ptr<device_buffer> ptr;
    typedef std::vector<ptr> vtr;
    device_buffer(ULONGLONG _start, ULONG _length, BYTE* _buffer, int _buffer_size) : start(_start), count(0), buffer_size(_buffer_size){
        end = start + _length;
        buffer.reserve(_buffer_size);
        buffer.resize(_length);
        memcpy(&buffer[0], _buffer, _length);
    }

    struct compare {
        bool operator() (boost::shared_ptr<device_buffer> const & lhs, boost::shared_ptr<device_buffer> const & rhs) const {
            if (lhs->count == rhs->count)
                return lhs->start < rhs->start;
            else
                return lhs->count < rhs->count;
        }
    };

    bool update(ULONGLONG _start, ULONG _length, BYTE* _buffer){
        bool result = false;
        ULONGLONG _end = _start + _length;
        if (_start >= start){
            if (_end <= end){
                memcpy(&buffer[(_start - start)], _buffer, _length);
                result = true;
                count = 0;
            }
            else if (_start <= end){
                ULONG new_length = (_end - start);
                if (new_length <= buffer_size){
                    end = start + new_length;
                    buffer.resize(new_length);
                    memcpy(&buffer[(_start - start)], _buffer, _length);
                    result = true;
                    count = 0;
                }
                else if (_start < end){
                    start = _start;
                    end = _end;
                    buffer.resize(_length);
                    memcpy(&buffer[0], _buffer, _length);
                    result = true;
                    count = 0;
                }
            }
        }
        else if (_end > start){
            start = _start;
            end = _end;
            buffer.resize(_length);
            memcpy(&buffer[0], _buffer, _length);
            result = true;
            count = 0;
        }
        return result;
    }

    bool read(ULONGLONG _start, ULONG _length, void *_buffer){
        bool result = false;
        if ((_start >= start) && (_start + _length) <= end){
            memcpy(_buffer, &buffer[(_start - start)], _length);
            result = true;
            count = 0;
        }
        return result;
    }

    void replace(ULONGLONG _start, ULONG _length, void *_buffer){
        ULONGLONG _end = _start + _length;
        start = _start;
        end = _end;
        buffer.resize(_length);
        memcpy(&buffer[0], _buffer, _length);
        count = 0;
    }
    
    void clear(ULONGLONG _start, ULONG _length){
        ULONGLONG _end = _start + _length;
        if ((_start >= start && (_end <= end || _start <= end)) || (_start < start && _end > start)){
#pragma push_macro("max")
#undef max
            start = std::numeric_limits<ULONGLONG>::max();
            end = std::numeric_limits<ULONGLONG>::max();
            count = std::numeric_limits<uint32_t>::max();
#pragma pop_macro("max")
        }
    }
    ULONGLONG           start;
    ULONGLONG           end;
    std::vector<BYTE>   buffer;
    uint32_t            count;
    int                 buffer_size;
};

struct device_mount_task{
    typedef boost::shared_ptr<device_mount_task> ptr;
    typedef std::vector<ptr> vtr;
    typedef boost::signals2::signal<bool(), macho::_or<bool>> task_is_canceled;

    device_mount_task(universal_disk_rw::ptr _rw, unsigned long _disk_size_in_mb, std::wstring _name, device_mount_repository& _repository)
        : rw(_rw), name(_name), disk_size_in_mb(_disk_size_in_mb), repository(_repository){}
    device_mount_repository&                                     repository;
    universal_disk_rw::ptr                                       rw;
    std::wstring												 name;
    task_is_canceled                                             is_canceled;
    unsigned long										    	 disk_size_in_mb;
    device_buffer::vtr                                           buffers;
    critical_section                                             cs;
    inline void register_task_is_canceled_function(task_is_canceled::slot_type slot){
        is_canceled.connect(slot);
    }

    bool                                                         read_from_cache(ULONGLONG _start_sector, ULONG _length, void *_buffer){
        ULONGLONG _start = (_start_sector << 9);
        foreach(device_buffer::ptr& buf, buffers){
            if (buf->read(_start, _length, _buffer))
                return true;
        }
        return false;
    }

    void                                                         update_to_cache(ULONGLONG _start_sector, ULONG _length, BYTE* _buffer, int buffer_size){
        ULONGLONG _start = (_start_sector << 9);
        bool result = false;
        foreach(device_buffer::ptr& buf, buffers){
            if (buf->start)
                buf->count++;
            if (!result){
                result = buf->update(_start, _length, _buffer);
            }
            else{
                buf->clear(_start, _length);
            }
        }
        if (!result){
            if (buffers.size() == BUFF_NUM){
                std::sort(buffers.begin(), buffers.end(), device_buffer::compare());
                buffers[BUFF_NUM - 1]->replace(_start, _length, _buffer);
            }
            else{
                buffers.push_back(device_buffer::ptr(new device_buffer(_start, _length, _buffer, buffer_size)));
            }
            std::sort(buffers.begin(), buffers.end(), device_buffer::compare());
        }
    }

    void                                                         execute(HANDLE _handle, HANDLE _event, universal_disk_rw::ptr _rw, int buffer_size, PVMP_DEVICE_INFO device){
        DWORD							 bytesReturned;
        bool                             request_only = true;
        DWORD                            dwEvent;
        HANDLE					         handle, quit_event, event;
        if (!DuplicateHandle(GetCurrentProcess(), _handle, GetCurrentProcess(), &handle, 0, FALSE, DUPLICATE_SAME_ACCESS)){
        }
        else if (!DuplicateHandle(GetCurrentProcess(), _event, GetCurrentProcess(), &event, 0, FALSE, DUPLICATE_SAME_ACCESS)){
            CloseHandle(handle);
        }
        else if (!DuplicateHandle(GetCurrentProcess(), repository.quit_event, GetCurrentProcess(), &quit_event, 0, FALSE, DUPLICATE_SAME_ACCESS)){
            CloseHandle(handle);
            CloseHandle(event);
        }
        else{
            ULONG  read_buffer_size = 0;
            bool   result = false;
            bool   read_from_buffer = false;
            size_t in_size = sizeof(VMPORT_CONTROL_IN) + buffer_size;
            size_t out_size = sizeof(VMPORT_CONTROL_OUT) + buffer_size;
            std::auto_ptr<VMPORT_CONTROL_IN> in((PVMPORT_CONTROL_IN)new BYTE[in_size]);
            std::auto_ptr<VMPORT_CONTROL_OUT> out((PVMPORT_CONTROL_OUT)new BYTE[out_size]);
            memset(in.get(), 0, in_size);
            memset(out.get(), 0, out_size);
            in->Device = device;
            uint32_t number_of_sectors_read, number_of_sectors_written;
            HANDLE ghEvents[2];
            ghEvents[0] = event;
            ghEvents[1] = quit_event;
            while (true){
                if (!repository.terminated && request_only){
                    dwEvent = WaitForMultipleObjects(
                        2,           // number of objects in array
                        ghEvents,    // array of objects
                        FALSE,       // wait for any object
                        10000);      // ten-second wait
                    switch (dwEvent)
                    {
                    case WAIT_OBJECT_0 + 0:
                        LOG(LOG_LEVEL_TRACE, L"Event triggered.");
                        break;
                    case WAIT_OBJECT_0 + 1:
                        LOG(LOG_LEVEL_TRACE, L"Quit triggered.");
                        break;
                    case WAIT_TIMEOUT:
                        LOG(LOG_LEVEL_TRACE, L"TIMEOUT triggered.");
                        break;
                    default:
                        LOG(LOG_LEVEL_ERROR, L"Wait error:(%d).", GetLastError());
                    }
                }
                if (repository.terminated || is_canceled())
                    break;
                {
                    macho::windows::auto_lock lck(cs);
                    if (!request_only && repository.is_cache_enabled){
                        switch (out->RequestType) {
                            case VMPORT_READ_REQUEST: {
                                if (!read_from_buffer){
                                    update_to_cache(out->StartSector, read_buffer_size, in->ResponseBuffer, buffer_size);
                                }
                                break;
                            }
                            case VMPORT_WRITE_REQUEST: {
                                update_to_cache(out->StartSector, out->RequestBufferLength, out->RequestBuffer, buffer_size);
                                break;
                            }
                        }
                    }
                    in->Command.IoControlCode = request_only ? IOCTL_VMPORT_GET_REQUEST : IOCTL_VMPORT_SEND_AND_GET;
                    out->RequestBufferLength = buffer_size;
                    if (result = (TRUE == DeviceIoControl(_handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, in.get(), in_size,
                        out.get(), out_size, &bytesReturned, NULL))){
                        request_only = false;
                        in->RequestID = out->RequestID;
                        if (repository.is_cache_enabled && VMPORT_READ_REQUEST == out->RequestType){
                            LOG(LOG_LEVEL_TRACE, L"Read : (%I64u, %u)", out->StartSector, out->RequestBufferLength);
                            if (read_from_buffer = read_from_cache(out->StartSector, out->RequestBufferLength, in->ResponseBuffer)){
                                LOG(LOG_LEVEL_TRACE, L"Read from buffer succeeded.");
                                in->ResponseBufferLength = out->RequestBufferLength;
                                in->ErrorCode = 0;
                                continue;
                            }
                        }
                    }
                }
                if (result){
                    switch (out->RequestType) {
                    case VMPORT_READ_REQUEST: {
                        read_buffer_size = out->RequestBufferLength;
                        if (0 == out->StartSector && read_buffer_size < 65536)
                            read_buffer_size = 65536;
                        int retry = 0;
                        while (!(result = rw->sector_read(out->StartSector, read_buffer_size >> 9, in->ResponseBuffer, number_of_sectors_read)) && retry < RETRY_COUNT){
                            retry++;
                            LOG(LOG_LEVEL_ERROR, L"Read Failed. (%d)", retry);
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                        };
                        if (result){
                            in->ResponseBufferLength = out->RequestBufferLength;
                            in->ErrorCode = 0;
                            LOG(LOG_LEVEL_TRACE, L"Read succeeded.");
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"Read Failed.");
                            in->ErrorCode = 0x1;
                            repository.terminated = true;
                        }
                        break;
                    }
                    case VMPORT_WRITE_REQUEST: {
                        LOG(LOG_LEVEL_TRACE, L"Write : (%I64u, %u)", out->StartSector, out->RequestBufferLength);
                        int retry = 0;
                        while (!(result = rw->sector_write(out->StartSector, out->RequestBuffer, out->RequestBufferLength / 512, number_of_sectors_written)) && retry < RETRY_COUNT){
                            retry++;
                            LOG(LOG_LEVEL_ERROR, L"Write Failed. (%d)", retry);
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                        };
                        if (result){
                            in->ResponseBufferLength = out->RequestBufferLength;
                            in->ErrorCode = 0;
                            LOG(LOG_LEVEL_TRACE, L"Write Succeeded.");
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"Write Failed.");
                            in->ErrorCode = 0x1;
                            repository.terminated = true;
                        }
                        break;
                    }
                    case VMPORT_NONE_REQUEST:
                    default:
                        request_only = true;
                    }
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl error. (%d)", GetLastError());
                    repository.terminated = true;
                }
            }
            CloseHandle(handle);
            CloseHandle(event);
            CloseHandle(quit_event);
        }
    }
    
    void                                                         mount(){
        macho::windows::auto_handle	handle = CreateFile(repository.device_path.c_str(),  // Name of the NT "device" to open 
            GENERIC_READ | GENERIC_WRITE,  // Access rights requested
            0,                           // Share access - NONE
            0,                           // Security attributes - not used!
            OPEN_EXISTING,               // Device must exist to open it.
            FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
            0);
        if (handle.is_valid()){
            CONNECT_IN						 connectIn;
            CONNECT_IN_RESULT                connectInRet;
            DWORD							 bytesReturned;
            BOOL							 devStatus;
            memset(&connectIn, 0, sizeof(CONNECT_IN));
            memset(&connectInRet, 0, sizeof(CONNECT_IN_RESULT));
            wmemcpy_s(connectIn.DiskId, sizeof(connectIn.DiskId), name.c_str(), name.length());
            LOG(LOG_LEVEL_RECORD, L"Try to mount disk with serial number : %s", connectIn.DiskId);
            connectIn.DiskSizeMB = disk_size_in_mb;
            LOG(LOG_LEVEL_RECORD, L"Disk Size : %u MB", connectIn.DiskSizeMB);
            connectIn.Command.IoControlCode = IOCTL_VMPORT_CONNECT;
            connectIn.NotifyEvent = CreateEvent(
                NULL,   // lpEventAttributes
                TRUE,   // bManualReset
                FALSE,  // bInitialState
                NULL    // lpName
                );
            connectIn.MergeIOs = repository.clone_rw ? TRUE : FALSE;
            if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, &connectIn, sizeof(CONNECT_IN),
                &connectInRet, sizeof(CONNECT_IN_RESULT), &bytesReturned, NULL)){
                if (repository.clone_rw){
                    boost::thread_group	thread_pool;
                    for (int i = 0; i < 4; i++)
                        thread_pool.create_thread(boost::bind(&device_mount_task::execute, this, (HANDLE)handle, connectIn.NotifyEvent, rw->clone(), (connectInRet.MaxTransferLength ? connectInRet.MaxTransferLength : BUFF_SIZE), connectInRet.Device));
                    thread_pool.join_all();
                }
                else{
                    execute((HANDLE)handle, connectIn.NotifyEvent, rw, (connectInRet.MaxTransferLength ? connectInRet.MaxTransferLength : BUFF_SIZE), connectInRet.Device);
                }
                LOG(LOG_LEVEL_RECORD, L"Dismount device(%s).", name.c_str());
                connectIn.Command.IoControlCode = IOCTL_VMPORT_DISCONNECT;
                devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP, &connectIn, sizeof(CONNECT_IN),
                    NULL, 0, &bytesReturned, NULL);
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Cannot mount device(%s).", name.c_str());
            }
        }
    }
};

#endif

std::wstring const launcher_job::_services[] = {
    //VMWARE
    _T("VMTools"),
    _T("VMUpgradeHelper"),
    _T("VMUSBArbService"),
    //Hyper-V
    _T("vmicheartbeat"),
    _T("vmickvpexchange"),
    _T("vmicshutdown"),
    _T("vmictimesync"),
    _T("vmicvss"),
    //IBM
    _T("tier1slp"),
    _T("wmicimserver"),
    _T("cimlistener"),
    _T("dirserver"),
    _T("ibmsa"),
    _T("IBMTivoliCommonAgent0"),
    //HP
    _T("#CIMnotify"),
    _T("CqMgHost"),
    _T("CpqNicMgmt"),
    _T("CqMgServ"),
    _T("CqMgStor"),
    _T("CpqRcmc"),
    _T("sysdown"),
    _T("Cissesrv"),
    _T("SysMgmtHp"),
    _T("cpqvcagent"),
    _T("HPWMISTOR"),
    //DELL
    _T("Server Administrator"),
    _T("dcstor32"),
    _T("dcevt32"),
    _T("omsad"),
    _T("mr2kserv"),
    _T("#AeXAgentSrvHost"),
    _T("AltirisClientMsgDispatcher"),
    _T("ctdataloader"),
    _T("EventEngine"),
    _T("EventReceiver"),
    _T("AltirisReceiverService"),
    _T("atrshost"),
    _T("AeXSvc"),
    _T("AltirisServiceHoster"),
    _T("AltirisSupportService"),
    _T("MetricProvider"),
    _T("#AltirisAgentProvider"),
    _T("#AMTRedirectionService"),
    _T("AeXNSClient"),
    //XEN
    _T("xenbus"),  //0
    _T("xendisk"), //0
    _T("xenfilt"), //0
    _T("xenvbd"),  //0
    _T("xenbus_monitor"), //2
    _T("xenagent"),       //2
    _T("xensvc"),         //2
    _T("xeniface"),       //3
    _T("xenvif"),         //3
    _T("xennet"),         //3
    _T("xenlite")         // ??
};

launcher_job::launcher_job(std::wstring id, saasame::transport::create_job_detail detail) : 
_is_windows_update(false)
,_terminated(false)
,_state(job_state_none)
,_create_job_detail(detail)
,_is_initialized(false)
,_is_interrupted(false)
,_is_canceled(false)
, _total_upload_size(0)
, _disk_size(0)
, _upload_progress(0)
,removeable_job(id,macho::guid_(GUID_NULL)){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _journal_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.journal") % _id % JOB_EXTENSION);
    _pre_script_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.pre") % _id % JOB_EXTENSION);
    _post_script_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.post") % _id % JOB_EXTENSION);
}

launcher_job::launcher_job(saasame::transport::create_job_detail detail) : 
_is_windows_update(false)
, _terminated(false)
, _state(job_state_none)
, _create_job_detail(detail)
, _is_initialized(false)
, _is_interrupted(false)
, _is_canceled(false)
, _total_upload_size(0)
, _disk_size(0)
, _upload_progress(0)
, removeable_job(macho::guid_::create(), macho::guid_(GUID_NULL)) {
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _journal_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.journal") % _id % JOB_EXTENSION);
    _pre_script_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.pre") % _id % JOB_EXTENSION);
    _post_script_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.post") % _id % JOB_EXTENSION);
}

launcher_job::ptr launcher_job::create(std::string id, saasame::transport::create_job_detail detail){
    launcher_job::ptr job;
    FUN_TRACE;
    if (detail.type == saasame::transport::job_type::launcher_job_type){
        job = launcher_job::ptr(new launcher_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
        job->_latest_update_time = job->_created_time;
    }
    return job;
}

launcher_job::ptr launcher_job::create(saasame::transport::create_job_detail detail){
    launcher_job::ptr job;
    FUN_TRACE;
    if (detail.type == saasame::transport::job_type::launcher_job_type){
        job = launcher_job::ptr(new launcher_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
        job->_latest_update_time = job->_created_time;
    }
    return job;
}

launcher_job::ptr launcher_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file){
    launcher_job::ptr job;
    std::wstring id;
    FUN_TRACE;
    saasame::transport::create_job_detail _create_job_detail = load_config(config_file.wstring(), id);
    if (_create_job_detail.type == saasame::transport::job_type::launcher_job_type){
        job = launcher_job::ptr(new launcher_job(id, _create_job_detail));
        job->load_create_job_detail(config_file.wstring());
        job->load_status(status_file.wstring());
    }
    return job;
}

void launcher_job::remove(){
    FUN_TRACE;
    _is_removing = true; 
    _terminated = true;
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        boost::filesystem::remove(_journal_file);
        boost::filesystem::remove(_pre_script_file);
        boost::filesystem::remove(_post_script_file);
        if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN){
            aliyun_upload aliyun(_launcher_job_create_detail.options.aliyun.region,
                _launcher_job_create_detail.options.aliyun.bucketname,
                _launcher_job_create_detail.options.aliyun.objectname,
                _launcher_job_create_detail.options.aliyun.access_key,
                _launcher_job_create_detail.options.aliyun.secret_key,
                _total_upload_size,
                _upload_id,
                _uploaded_parts, -1,
                _launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE);
            aliyun.abort();
        }
        else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT){
            tencent_upload tencent(_launcher_job_create_detail.options.tencent.region,
                _launcher_job_create_detail.options.tencent.bucketname,
                _launcher_job_create_detail.options.tencent.objectname,
                _launcher_job_create_detail.options.tencent.access_key,
                _launcher_job_create_detail.options.tencent.secret_key,
                _total_upload_size,
                _upload_id,
                _uploaded_parts,
                -1,
                _launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE);
            tencent.abort();
        }
        _running.unlock();
    }
}

saasame::transport::create_job_detail launcher_job::load_config(std::wstring config_file, std::wstring &job_id){
    FUN_TRACE;
    saasame::transport::create_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "id").get_str());
        detail.management_id = find_value(_create_job_detail.get_obj(), "management_id").get_str();
        detail.mgmt_port = find_value_int32(_create_job_detail.get_obj(), "mgmt_port", 80);
        detail.is_ssl = find_value_bool(_create_job_detail.get_obj(), "is_ssl");
        mArray addr = find_value(_create_job_detail.get_obj(), "mgmt_addr").get_array();
        foreach(mValue a, addr){
            detail.mgmt_addr.insert(a.get_str());
        }
        detail.type = (saasame::transport::job_type::type)find_value(_create_job_detail.get_obj(), "type").get_int();
        mArray triggers = find_value(_create_job_detail.get_obj(), "triggers").get_array();
        foreach(mValue t, triggers){
            saasame::transport::job_trigger trigger;
            trigger.type = (saasame::transport::job_trigger_type::type)find_value(t.get_obj(), "type").get_int();
            trigger.start = find_value(t.get_obj(), "start").get_str();
            trigger.finish = find_value(t.get_obj(), "finish").get_str();
            trigger.interval = find_value(t.get_obj(), "interval").get_int();
            trigger.duration = find_value(t.get_obj(), "duration").get_int();
            trigger.id = find_value_string(t.get_obj(), "id");
            detail.triggers.push_back(trigger);
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")));
    }
    catch (...){
    }
    return detail;
}

void launcher_job::load_create_job_detail(std::wstring config_file){
    FUN_TRACE;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        const mObject job_config_obj = job_config.get_obj();
        mObject::const_iterator i = job_config_obj.find("launcher_job_create_detail");
        if (i == job_config_obj.end()){
            _is_initialized = false;
        }
        else{
            _is_initialized = true;
            mValue _job_create_detail = i->second;
            _launcher_job_create_detail = launcher_job_create_detail();

            _launcher_job_create_detail.replica_id = find_value_string(_job_create_detail.get_obj(), "replica_id");
            _launcher_job_create_detail.config = find_value_string(_job_create_detail.get_obj(), "config");
            _launcher_job_create_detail.is_sysvol_authoritative_restore = find_value(_job_create_detail.get_obj(), "is_sysvol_authoritative_restore").get_bool();
            _launcher_job_create_detail.is_enable_debug = find_value(_job_create_detail.get_obj(), "is_enable_debug").get_bool();
            _launcher_job_create_detail.is_disable_machine_password_change = find_value(_job_create_detail.get_obj(), "is_disable_machine_password_change").get_bool();
            _launcher_job_create_detail.is_force_normal_boot = find_value(_job_create_detail.get_obj(), "is_force_normal_boot").get_bool();
            _launcher_job_create_detail.gpt_to_mbr = find_value(_job_create_detail.get_obj(), "gpt_to_mbr").get_bool();

            if (_job_create_detail.get_obj().count("skip_system_injection"))
                _launcher_job_create_detail.skip_system_injection = find_value(_job_create_detail.get_obj(), "skip_system_injection").get_bool();
            if (_job_create_detail.get_obj().count("reboot_winpe"))
                _launcher_job_create_detail.reboot_winpe = find_value(_job_create_detail.get_obj(), "reboot_winpe").get_bool();
            if (_job_create_detail.get_obj().count("mode"))
                _launcher_job_create_detail.mode = (recovery_type::type)find_value(_job_create_detail.get_obj(), "mode").get_int();

            _launcher_job_create_detail.detect_type = (disk_detect_type::type)find_value_int32(_job_create_detail.get_obj(), "detect_type");

            mArray disks_lun_mapping = find_value(_job_create_detail.get_obj(), "disks_lun_mapping").get_array();
            foreach(mValue d, disks_lun_mapping){
                _launcher_job_create_detail.disks_lun_mapping[find_value_string(d.get_obj(), "disk")] = find_value_string(d.get_obj(), "lun");
            }

            mArray network_infos = find_value(_job_create_detail.get_obj(), "network_infos").get_array();
            foreach(mValue n, network_infos){
                saasame::transport::network_info net;
                net.adapter_name = find_value_string(n.get_obj(), "adapter_name");
                net.description = find_value_string(n.get_obj(), "description");
                net.mac_address = find_value_string(n.get_obj(), "mac_address");
                net.is_dhcp_v4 =  find_value(n.get_obj(), "is_dhcp_v4").get_bool();
                net.is_dhcp_v6 = find_value(n.get_obj(), "is_dhcp_v6").get_bool();
                mArray dnss = find_value(n.get_obj(), "dnss").get_array();
                foreach(mValue d, dnss){
                    net.dnss.push_back(d.get_str());
                }
                mArray gateways = find_value(n.get_obj(), "gateways").get_array();
                foreach(mValue g, gateways){
                    net.gateways.push_back(g.get_str());
                }
                mArray ip_addresses = find_value(n.get_obj(), "ip_addresses").get_array();
                foreach(mValue i, ip_addresses){
                    net.ip_addresses.push_back(i.get_str());
                }
                mArray subnet_masks = find_value(n.get_obj(), "subnet_masks").get_array();
                foreach(mValue s, subnet_masks){
                    net.subnet_masks.push_back(s.get_str());
                }
                _launcher_job_create_detail.network_infos.insert(net);
            }
            const mObject job_create_detail_obj = _job_create_detail.get_obj();
            mObject::const_iterator callbacks_iterator = job_create_detail_obj.find("callbacks");
            if (callbacks_iterator != job_create_detail_obj.end()){
                mArray callbacks = callbacks_iterator->second.get_array();
                foreach(mValue a, callbacks){
                    _launcher_job_create_detail.callbacks.insert(a.get_str());
                }
            }
            _launcher_job_create_detail.callback_timeout = find_value_int32(_job_create_detail.get_obj(), "callback_timeout");
            _launcher_job_create_detail.host_name = find_value_string(_job_create_detail.get_obj(), "host_name");
            _launcher_job_create_detail.export_disk_type = (virtual_disk_type::type)find_value_int32(_job_create_detail.get_obj(), "export_disk_type", virtual_disk_type::VHD);
            _launcher_job_create_detail.export_path = find_value_string(_job_create_detail.get_obj(), "export_path");
            _launcher_job_create_detail.target_type = (conversion_type::type)find_value_int32(_job_create_detail.get_obj(), "target_type", conversion_type::AUTO);
            _launcher_job_create_detail.os_type = (saasame::transport::hv_guest_os_type::type)find_value_int32(_job_create_detail.get_obj(), "os_type", saasame::transport::hv_guest_os_type::HV_OS_WINDOWS);
            _launcher_job_create_detail.is_update_ex = find_value_bool(_job_create_detail.get_obj(), "is_update_ex");
            _launcher_job_create_detail.options_type = (extra_options_type::type)find_value_int32(_job_create_detail.get_obj(), "options_type", extra_options_type::UNKNOWN);
            mObject::const_iterator pre_scripts_iterator = job_create_detail_obj.find("pre_scripts");
            if (callbacks_iterator != job_create_detail_obj.end()){
                mArray pre_scripts = pre_scripts_iterator->second.get_array();
                foreach(mValue a, pre_scripts){
                    _launcher_job_create_detail.pre_scripts.insert(a.get_str());
                }
            }
            mObject::const_iterator post_scripts_iterator = job_create_detail_obj.find("post_scripts");
            if (post_scripts_iterator != job_create_detail_obj.end()){
                mArray post_scripts = post_scripts_iterator->second.get_array();
                foreach(mValue a, post_scripts){
                    _launcher_job_create_detail.post_scripts.insert(a.get_str());
                }
            }
            
            if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB){
                std::string azure_storage_connection_string = find_value_string(_job_create_detail.get_obj(), "ez_azure_storage_connection_string");
                if (!azure_storage_connection_string.empty()){
                    macho::bytes ez_azure_storage_connection_string;
                    ez_azure_storage_connection_string.set(macho::stringutils::convert_utf8_to_unicode(azure_storage_connection_string));
                    macho::bytes u_ez_azure_storage_connection_string = macho::windows::protected_data::unprotect(ez_azure_storage_connection_string, true);
                    if (u_ez_azure_storage_connection_string.length() && u_ez_azure_storage_connection_string.ptr()){
                        _launcher_job_create_detail.azure_storage_connection_string = std::string(reinterpret_cast<char const*>(u_ez_azure_storage_connection_string.ptr()), u_ez_azure_storage_connection_string.length());
                    }
                    else{
                        _launcher_job_create_detail.azure_storage_connection_string = "";
                    }
                }
            }
            else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
                macho::bytes user, password;
                user.set(macho::stringutils::convert_utf8_to_unicode(find_value(_job_create_detail.get_obj(), "u1").get_str()));
                password.set(macho::stringutils::convert_utf8_to_unicode(find_value(_job_create_detail.get_obj(), "u2").get_str()));
                macho::bytes u1 = macho::windows::protected_data::unprotect(user, true);
                macho::bytes u2 = macho::windows::protected_data::unprotect(password, true);

                if (u1.length() && u1.ptr()){
                    _launcher_job_create_detail.vmware.connection.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                }
                else{
                    _launcher_job_create_detail.vmware.connection.username = "";
                }
                if (u2.length() && u2.ptr()){
                    _launcher_job_create_detail.vmware.connection.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
                }
                else{
                    _launcher_job_create_detail.vmware.connection.password = "";
                }
                _launcher_job_create_detail.vmware.connection.host = find_value_string(_job_create_detail.get_obj(), "host");
                _launcher_job_create_detail.vmware.connection.esx = find_value_string(_job_create_detail.get_obj(), "esx");
                _launcher_job_create_detail.vmware.connection.datastore = find_value_string(_job_create_detail.get_obj(), "datastore");
                _launcher_job_create_detail.vmware.connection.folder_path = find_value_string(_job_create_detail.get_obj(), "folder_path");
                _launcher_job_create_detail.vmware.virtual_machine_id = find_value_string(_job_create_detail.get_obj(), "virtual_machine_id");
                _launcher_job_create_detail.vmware.virtual_machine_snapshot = find_value_string(_job_create_detail.get_obj(), "virtual_machine_snapshot");
                _launcher_job_create_detail.vmware.number_of_cpus = find_value_int32(_job_create_detail.get_obj(), "number_of_cpus");
                _launcher_job_create_detail.vmware.number_of_memory_in_mb = find_value_int32(_job_create_detail.get_obj(), "number_of_memory_in_mb");
                _launcher_job_create_detail.vmware.vm_name = find_value_string(_job_create_detail.get_obj(), "vm_name");
                mArray network_connections = find_value(_job_create_detail.get_obj(), "network_connections").get_array();
                foreach(mValue c, network_connections){
                    _launcher_job_create_detail.vmware.network_connections.push_back(c.get_str());
                }
                mObject::const_iterator network_adapters_iterator = job_create_detail_obj.find("network_adapters");
                if (network_adapters_iterator != job_create_detail_obj.end()){
                    mArray network_adapters = network_adapters_iterator->second.get_array();
                    foreach(mValue c, network_adapters){
                        _launcher_job_create_detail.vmware.network_adapters.push_back(c.get_str());
                    }
                    mObject::const_iterator mac_addresses_iterator = job_create_detail_obj.find("mac_addresses");
                    if (mac_addresses_iterator != job_create_detail_obj.end()){
                        mArray mac_addresses = mac_addresses_iterator->second.get_array();
                        foreach(mValue m, mac_addresses){
                            _launcher_job_create_detail.vmware.mac_addresses.push_back(m.get_str());
                        }
                    }
                    _launcher_job_create_detail.vmware.guest_id = find_value_string(_job_create_detail.get_obj(), "guest_id");
                    _launcher_job_create_detail.vmware.firmware = (saasame::transport::hv_vm_firmware::type)find_value_int32(_job_create_detail.get_obj(), "firmware");
                    _launcher_job_create_detail.vmware.install_vm_tools = find_value_bool(_job_create_detail.get_obj(), "install_vm_tools");

                    mArray scsi_adapters = find_value(_job_create_detail.get_obj(), "scsi_adapters").get_array();
                    foreach(mValue s, scsi_adapters){
                        saasame::transport::hv_controller_type::type type = (saasame::transport::hv_controller_type::type)find_value_int32(s.get_obj(), "type");
                        mArray disks = find_value(s.get_obj(), "disks").get_array();
                        foreach(mValue d, disks){
                            _launcher_job_create_detail.vmware.scsi_adapters[type].push_back(d.get_str());
                        }
                    }
                }
            }
            else if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN){
                mObject::const_iterator aliyun_iterator = job_create_detail_obj.find("aliyun");
                if (aliyun_iterator != job_create_detail_obj.end()){
                    mValue aliyun = aliyun_iterator->second;
                    _launcher_job_create_detail.options.aliyun.region = find_value_string(aliyun.get_obj(), "region");
                    _launcher_job_create_detail.options.aliyun.bucketname = find_value_string(aliyun.get_obj(), "bucketname");
                    _launcher_job_create_detail.options.aliyun.objectname = find_value_string(aliyun.get_obj(), "objectname");
                    _launcher_job_create_detail.options.aliyun.max_size = find_value_int32(aliyun.get_obj(), "max_size");
                    _launcher_job_create_detail.options.aliyun.file_system_filter_enable = find_value_bool(aliyun.get_obj(), "file_system_filter_enable");
                    _launcher_job_create_detail.options.aliyun.number_of_upload_threads = find_value_int32(aliyun.get_obj(), "number_of_upload_threads");
                    macho::bytes access_key, secret_key;
                    access_key.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(aliyun.get_obj(), "access_key")));
                    secret_key.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(aliyun.get_obj(), "secret_key")));
                    macho::bytes u1 = macho::windows::protected_data::unprotect(access_key, true);
                    macho::bytes u2 = macho::windows::protected_data::unprotect(secret_key, true);
                    if (u1.length() && u1.ptr()){
                        _launcher_job_create_detail.options.aliyun.access_key = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                    }
                    else{
                        _launcher_job_create_detail.options.aliyun.access_key = "";
                    }
                    if (u2.length() && u2.ptr()){
                        _launcher_job_create_detail.options.aliyun.secret_key = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
                    }
                    else{
                        _launcher_job_create_detail.options.aliyun.secret_key = "";
                    }
                    mArray disks_object_name_mapping = find_value(aliyun.get_obj(), "disks_object_name_mapping").get_array();
                    foreach(mValue d, disks_object_name_mapping){
                        _launcher_job_create_detail.options.aliyun.disks_object_name_mapping[find_value_string(d.get_obj(), "disk")] = find_value_string(d.get_obj(), "name");
                    }
                }
            }
            else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT){
                mObject::const_iterator tencent_iterator = job_create_detail_obj.find("tencent");
                if (tencent_iterator != job_create_detail_obj.end()){
                    mValue tencent = tencent_iterator->second;
                    _launcher_job_create_detail.options.tencent.region = find_value_string(tencent.get_obj(), "region");
                    _launcher_job_create_detail.options.tencent.bucketname = find_value_string(tencent.get_obj(), "bucketname");
                    _launcher_job_create_detail.options.tencent.objectname = find_value_string(tencent.get_obj(), "objectname");
                    _launcher_job_create_detail.options.tencent.max_size = find_value_int32(tencent.get_obj(), "max_size");
                    _launcher_job_create_detail.options.tencent.file_system_filter_enable = find_value_bool(tencent.get_obj(), "file_system_filter_enable");
                    _launcher_job_create_detail.options.tencent.number_of_upload_threads = find_value_int32(tencent.get_obj(), "number_of_upload_threads");
                    macho::bytes access_key, secret_key;
                    access_key.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(tencent.get_obj(), "access_key")));
                    secret_key.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(tencent.get_obj(), "secret_key")));
                    macho::bytes u1 = macho::windows::protected_data::unprotect(access_key, true);
                    macho::bytes u2 = macho::windows::protected_data::unprotect(secret_key, true);
                    if (u1.length() && u1.ptr()){
                        _launcher_job_create_detail.options.tencent.access_key = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                    }
                    else{
                        _launcher_job_create_detail.options.tencent.access_key = "";
                    }
                    if (u2.length() && u2.ptr()){
                        _launcher_job_create_detail.options.tencent.secret_key = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
                    }
                    else{
                        _launcher_job_create_detail.options.tencent.secret_key = "";
                    }
                    mArray disks_object_name_mapping = find_value(tencent.get_obj(), "disks_object_name_mapping").get_array();
                    foreach(mValue d, disks_object_name_mapping){
                        _launcher_job_create_detail.options.tencent.disks_object_name_mapping[find_value_string(d.get_obj(), "disk")] = find_value_string(d.get_obj(), "name");
                    }
                }
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
}

void launcher_job::save_config(){
    FUN_TRACE;
    try{
        using namespace json_spirit;
        mObject job_config;
        mObject job_detail;
        job_detail["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_detail["type"] = _create_job_detail.type;
        job_detail["management_id"] = _create_job_detail.management_id;
        job_detail["mgmt_port"] = _create_job_detail.mgmt_port;
        job_detail["is_ssl"] = _create_job_detail.is_ssl;
        mArray  mgmt_addr(_create_job_detail.mgmt_addr.begin(), _create_job_detail.mgmt_addr.end());
        job_detail["mgmt_addr"] = mgmt_addr;
        mArray triggers;
        foreach(saasame::transport::job_trigger &t, _create_job_detail.triggers){
            mObject trigger;
            trigger["type"] = (int)t.type;
            trigger["start"] = t.start;
            trigger["finish"] = t.finish;
            trigger["interval"] = t.interval;
            trigger["duration"] = t.duration;
            trigger["id"] = t.id;
            triggers.push_back(trigger);
        }
        job_detail["triggers"] = triggers;
        job_config["create_job_detail"] = job_detail;

        if (_is_initialized){
            mObject _job_create_detail;
            // Need to add for loader;
            _job_create_detail["replica_id"] = _launcher_job_create_detail.replica_id;
            _job_create_detail["is_sysvol_authoritative_restore"] = _launcher_job_create_detail.is_sysvol_authoritative_restore;
            _job_create_detail["is_enable_debug"] = _launcher_job_create_detail.is_enable_debug;
            _job_create_detail["is_disable_machine_password_change"] = _launcher_job_create_detail.is_disable_machine_password_change;
            _job_create_detail["is_force_normal_boot"] = _launcher_job_create_detail.is_force_normal_boot;
            _job_create_detail["config"] = _launcher_job_create_detail.config;
            _job_create_detail["gpt_to_mbr"] = _launcher_job_create_detail.gpt_to_mbr;
            _job_create_detail["detect_type"] = _launcher_job_create_detail.detect_type;
            _job_create_detail["reboot_winpe"] = _launcher_job_create_detail.reboot_winpe;
            _job_create_detail["mode"]         = (int)_launcher_job_create_detail.mode;

            mArray disks_lun_mapping;
            foreach(string_map::value_type &d, _launcher_job_create_detail.disks_lun_mapping){
                mObject disk_lun;
                disk_lun["disk"] = d.first;
                disk_lun["lun"] = d.second;
                disks_lun_mapping.push_back(disk_lun);
            }
            _job_create_detail["disks_lun_mapping"] = disks_lun_mapping;

            mArray network_infos;
            foreach(saasame::transport::network_info n, _launcher_job_create_detail.network_infos){
                mObject net;
                net["adapter_name"] = n.adapter_name;
                net["description"] = n.description;
                net["is_dhcp_v4"] = n.is_dhcp_v4;
                net["is_dhcp_v6"] = n.is_dhcp_v6;
                net["mac_address"] = n.mac_address;
                mArray  ip_addresses(n.ip_addresses.begin(), n.ip_addresses.end());
                net["ip_addresses"] = ip_addresses;
                mArray  subnet_masks(n.subnet_masks.begin(), n.subnet_masks.end());
                net["subnet_masks"] = subnet_masks;
                mArray  gateways(n.gateways.begin(), n.gateways.end());
                net["gateways"] = gateways;
                mArray  dnss(n.dnss.begin(), n.dnss.end());
                net["dnss"] = dnss;
                network_infos.push_back(net);
            }
            
            _job_create_detail["network_infos"] = network_infos;
            mArray  callbacks(_launcher_job_create_detail.callbacks.begin(), _launcher_job_create_detail.callbacks.end());
            _job_create_detail["callbacks"] = callbacks;
            _job_create_detail["callback_timeout"] = _launcher_job_create_detail.callback_timeout;
            _job_create_detail["host_name"] = _launcher_job_create_detail.host_name;
            _job_create_detail["export_disk_type"] = _launcher_job_create_detail.export_disk_type;
            _job_create_detail["export_path"] = _launcher_job_create_detail.export_path;
            _job_create_detail["target_type"] = _launcher_job_create_detail.target_type;
            _job_create_detail["os_type"] = _launcher_job_create_detail.os_type;
            _job_create_detail["is_update_ex"] = _launcher_job_create_detail.is_update_ex;
            _job_create_detail["skip_system_injection"] = _launcher_job_create_detail.skip_system_injection;
            _job_create_detail["options_type"] = _launcher_job_create_detail.options_type;

            if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB && !_launcher_job_create_detail.azure_storage_connection_string.empty()){
                macho::bytes azure_storage_connection_string;
                azure_storage_connection_string.set((LPBYTE)_launcher_job_create_detail.azure_storage_connection_string.c_str(), _launcher_job_create_detail.azure_storage_connection_string.length());
                macho::bytes ez_azure_storage_connection_string = macho::windows::protected_data::protect(azure_storage_connection_string, true);
                _job_create_detail["ez_azure_storage_connection_string"] = macho::stringutils::convert_unicode_to_utf8(ez_azure_storage_connection_string.get());;
            }
            else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
                _job_create_detail["host"] = _launcher_job_create_detail.vmware.connection.host;
                _job_create_detail["esx"] = _launcher_job_create_detail.vmware.connection.esx;
                _job_create_detail["datastore"] = _launcher_job_create_detail.vmware.connection.datastore;
                _job_create_detail["folder_path"] = _launcher_job_create_detail.vmware.connection.folder_path;
                _job_create_detail["virtual_machine_id"] = _launcher_job_create_detail.vmware.virtual_machine_id;
                _job_create_detail["virtual_machine_snapshot"] = _launcher_job_create_detail.vmware.virtual_machine_snapshot;
                _job_create_detail["number_of_cpus"] = _launcher_job_create_detail.vmware.number_of_cpus;
                _job_create_detail["number_of_memory_in_mb"] = _launcher_job_create_detail.vmware.number_of_memory_in_mb;
                _job_create_detail["vm_name"] = _launcher_job_create_detail.vmware.vm_name;
                macho::bytes user, password;
                user.set((LPBYTE)_launcher_job_create_detail.vmware.connection.username.c_str(), _launcher_job_create_detail.vmware.connection.username.length());
                password.set((LPBYTE)_launcher_job_create_detail.vmware.connection.password.c_str(), _launcher_job_create_detail.vmware.connection.password.length());
                macho::bytes u1 = macho::windows::protected_data::protect(user, true);
                macho::bytes u2 = macho::windows::protected_data::protect(password, true);
                _job_create_detail["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
                _job_create_detail["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
                mArray  network_connections(_launcher_job_create_detail.vmware.network_connections.begin(), _launcher_job_create_detail.vmware.network_connections.end());
                _job_create_detail["network_connections"] = network_connections;
                mArray  network_adapters(_launcher_job_create_detail.vmware.network_adapters.begin(), _launcher_job_create_detail.vmware.network_adapters.end());
                _job_create_detail["network_adapters"] = network_adapters;
                mArray  mac_addresses(_launcher_job_create_detail.vmware.mac_addresses.begin(), _launcher_job_create_detail.vmware.mac_addresses.end());
                _job_create_detail["mac_addresses"] = mac_addresses;
                _job_create_detail["guest_id"] = _launcher_job_create_detail.vmware.guest_id;
                _job_create_detail["firmware"] = (int)_launcher_job_create_detail.vmware.firmware;
                _job_create_detail["install_vm_tools"] = _launcher_job_create_detail.vmware.install_vm_tools;
                mArray scsi_adapters;
                foreach(scsi_adapter_disks_map::value_type &s, _launcher_job_create_detail.vmware.scsi_adapters){
                    mObject scsi;
                    scsi["type"] = (int)s.first;
                    mArray  disk(s.second.begin(), s.second.end());
                    scsi["disks"] = disk;
                    scsi_adapters.push_back(scsi);
                }
                _job_create_detail["scsi_adapters"] = scsi_adapters;
            }

            if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN){
                mObject aliyun;
                macho::bytes access_key, secret_key;
                access_key.set((LPBYTE)_launcher_job_create_detail.options.aliyun.access_key.c_str(), _launcher_job_create_detail.options.aliyun.access_key.length());
                secret_key.set((LPBYTE)_launcher_job_create_detail.options.aliyun.secret_key.c_str(), _launcher_job_create_detail.options.aliyun.secret_key.length());
                macho::bytes u1 = macho::windows::protected_data::protect(access_key, true);
                macho::bytes u2 = macho::windows::protected_data::protect(secret_key, true);
                aliyun["access_key"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
                aliyun["secret_key"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
                aliyun["region"] = _launcher_job_create_detail.options.aliyun.region;
                aliyun["bucketname"] = _launcher_job_create_detail.options.aliyun.bucketname;
                aliyun["objectname"] = _launcher_job_create_detail.options.aliyun.objectname;
                aliyun["max_size"] = _launcher_job_create_detail.options.aliyun.max_size;
                aliyun["file_system_filter_enable"] = _launcher_job_create_detail.options.aliyun.file_system_filter_enable;
                aliyun["number_of_upload_threads"] = _launcher_job_create_detail.options.aliyun.number_of_upload_threads;
                mArray disks_object_name_mapping;
                foreach(string_map::value_type &d, _launcher_job_create_detail.options.aliyun.disks_object_name_mapping){
                    mObject disks_object;
                    disks_object["disk"] = d.first;
                    disks_object["name"] = d.second;
                    disks_object_name_mapping.push_back(disks_object);
                }
                aliyun["disks_object_name_mapping"] = disks_object_name_mapping;
                _job_create_detail["aliyun"] = aliyun;
            }
            else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT){
                mObject tencent;
                macho::bytes access_key, secret_key;
                access_key.set((LPBYTE)_launcher_job_create_detail.options.tencent.access_key.c_str(), _launcher_job_create_detail.options.tencent.access_key.length());
                secret_key.set((LPBYTE)_launcher_job_create_detail.options.tencent.secret_key.c_str(), _launcher_job_create_detail.options.tencent.secret_key.length());
                macho::bytes u1 = macho::windows::protected_data::protect(access_key, true);
                macho::bytes u2 = macho::windows::protected_data::protect(secret_key, true);
                tencent["access_key"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
                tencent["secret_key"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
                tencent["region"] = _launcher_job_create_detail.options.tencent.region;
                tencent["bucketname"] = _launcher_job_create_detail.options.tencent.bucketname;
                tencent["objectname"] = _launcher_job_create_detail.options.tencent.objectname;
                tencent["max_size"] = _launcher_job_create_detail.options.tencent.max_size;
                tencent["file_system_filter_enable"] = _launcher_job_create_detail.options.tencent.file_system_filter_enable;
                tencent["number_of_upload_threads"] = _launcher_job_create_detail.options.tencent.number_of_upload_threads;
                mArray disks_object_name_mapping;
                foreach(string_map::value_type &d, _launcher_job_create_detail.options.tencent.disks_object_name_mapping){
                    mObject disks_object;
                    disks_object["disk"] = d.first;
                    disks_object["name"] = d.second;
                    disks_object_name_mapping.push_back(disks_object);
                }
                tencent["disks_object_name_mapping"] = disks_object_name_mapping;
                _job_create_detail["tencent"] = tencent;
            }

            mArray  pre_scripts(_launcher_job_create_detail.pre_scripts.begin(), _launcher_job_create_detail.pre_scripts.end());
            _job_create_detail["pre_scripts"] = pre_scripts;
            mArray  post_scripts(_launcher_job_create_detail.post_scripts.begin(), _launcher_job_create_detail.post_scripts.end());
            _job_create_detail["post_scripts"] = post_scripts;

            job_config["launcher_job_create_detail"] = _job_create_detail;
        }
        boost::filesystem::path temp = _config_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _config_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _config_file.wstring().c_str(), GetLastError());
        }
        else{
            long      response;
            if (_launcher_job_create_detail.pre_scripts.empty()){  
                boost::filesystem::remove(_pre_script_file);
            }
            else if (!boost::filesystem::exists(_pre_script_file)){
                foreach(std::string pre_script, _launcher_job_create_detail.pre_scripts){
                    http_client client;
                    std::fstream output(_pre_script_file.string(), std::ios::binary | std::ios::out | std::ios::trunc);
                    if (CURLE_OK == client.get(pre_script, "", "", response, output) && response == 200){
                        output.close();
                        LOG(LOG_LEVEL_RECORD, _T("Successfully download the pre-script file \"%s\""), macho::stringutils::convert_utf8_to_unicode(pre_script).c_str());
                        break;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, _T("Cannot download the pre-script file \"%s\" (%d)"), macho::stringutils::convert_utf8_to_unicode(pre_script).c_str(), response);
                        output.close();
                        boost::filesystem::remove(_pre_script_file);
                    }
                }
            }
            if (_launcher_job_create_detail.post_scripts.empty()){
                boost::filesystem::remove(_post_script_file);
            }
            else if (!boost::filesystem::exists(_post_script_file)){
                foreach(std::string post_script, _launcher_job_create_detail.post_scripts){
                    http_client client;
                    std::fstream output(_post_script_file.string(), std::ios::binary | std::ios::out | std::ios::trunc);
                    if (CURLE_OK == client.get(post_script, "", "", response, output) && response == 200){
                        output.close();
                        LOG(LOG_LEVEL_RECORD, _T("Successfully download the post-script file \"%s\""), macho::stringutils::convert_utf8_to_unicode(post_script).c_str());
                        break;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, _T("Cannot download the post-script file \"%s\" (%d)"), macho::stringutils::convert_utf8_to_unicode(post_script).c_str(), response);
                        output.close();
                        boost::filesystem::remove(_post_script_file);
                    }
                }
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")));
    }
    catch (...){
    }
}

void launcher_job::record(saasame::transport::job_state::type state, int error, std::string description){
    macho::windows::auto_lock lock(_cs);
    _histories.push_back(history_record::ptr(new history_record(state, error, description)));
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

void launcher_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    macho::windows::auto_lock lock(_cs);
    if (_histories.size() == 0 || _histories[_histories.size() - 1]->description != format.str()){
        _histories.push_back(history_record::ptr(new history_record(state, error, format)));
        LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(format.str()).c_str());
    }
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

bool launcher_job::get_launcher_job_create_detail(saasame::transport::launcher_job_create_detail &detail){
    FUN_TRACE;
    bool result = false;
    int retry = 3;
    while (!result){
        mgmt_op op(_create_job_detail.mgmt_addr, _mgmt_addr, _create_job_detail.mgmt_port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                FUN_TRACE_MSG(boost::str(boost::wformat(L"get_launcher_job_create_detail(%s)") % _id));
                op.client()->get_launcher_job_create_detail(detail, "", macho::stringutils::convert_unicode_to_ansi(_id));
                result = true;
            }
            catch (saasame::transport::invalid_operation& op){
                LOG(LOG_LEVEL_ERROR, L"Invalid operation (0x%08x): %s", op.what_op, macho::stringutils::convert_ansi_to_unicode(op.why).c_str());
                if (op.what_op == saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND){
                    _is_removing = true;
                    break;
                }
            }
            catch (apache::thrift::TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
        retry--;
        if (result || 0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return result;
}

bool launcher_job::update_state(const saasame::transport::launcher_job_detail &detail){
    FUN_TRACE;
    bool result = false;
    int retry = 3;
    while (!result){
        mgmt_op op(_create_job_detail.mgmt_addr, _mgmt_addr, _create_job_detail.mgmt_port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                if (_launcher_job_create_detail.is_update_ex)
                    op.client()->update_launcher_job_state_ex("", detail);
                else
                    op.client()->update_launcher_job_state("", detail);
                result = true;
                break;
            }
            catch (apache::thrift::TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
        }
        retry--;
        if (0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return result;
}

bool launcher_job::is_launcher_job_image_ready(){
    FUN_TRACE;
    bool result = false;
    mgmt_op op(_create_job_detail.mgmt_addr, _mgmt_addr, _create_job_detail.mgmt_port, _create_job_detail.is_ssl);
    if (op.open()){
        try{            
            result = op.client()->is_launcher_job_image_ready("", macho::stringutils::convert_unicode_to_ansi(_id));
        }
        catch (apache::thrift::TException & tx){
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    return result;
}

void launcher_job:: save_status(){
    FUN_TRACE;
    try{
        macho::windows::auto_lock lock(_cs);
        mObject job_status;
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["is_initialized"] = _is_initialized;
        job_status["state"] = _state;
        job_status["is_windows_update"] = _is_windows_update;

        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);
        mArray histories;
        foreach(history_record::ptr &h, _histories){
            mObject o;
            o["time"] = boost::posix_time::to_simple_string(h->time);
            o["state"] = (int)h->state;
            o["error"] = h->error;
            o["description"] = h->description;
            o["format"] = h->format;
            mArray  args(h->args.begin(), h->args.end());
            o["arguments"] = args;
            histories.push_back(o);
        }
        job_status["histories"] = histories;
        job_status["boot_disk"] = _boot_disk;
        job_status["latest_update_time"] = boost::posix_time::to_simple_string(_latest_update_time);
        job_status["platform"] = _platform;
        job_status["architecture"] = _architecture;
        job_status["total_upload_size"] = _total_upload_size;
        job_status["upload_progress"] = _upload_progress;
        job_status["disk_size"] = (int)_disk_size;
        job_status["upload_id"] = _upload_id;
        job_status["host_name"] = _host_name;
        job_status["virtual_machine_id"] = macho::stringutils::convert_unicode_to_utf8(_virtual_machine_id);
        job_status["uploading_disk"] = _uploading_disk;

        mArray upload_progress_map;
        foreach(upload_progress::map::value_type &p, _upload_maps){
            mObject o;
            o["disk"] = p.first;
            o["upload_id"] = p.second->upload_id;
            o["vhd_size"] = p.second->vhd_size;
            o["size"] = p.second->size;
            o["progress"] = p.second->progress;
            o["is_completed"] = p.second->is_completed;
            upload_progress_map.push_back(o);
        }
        job_status["upload_progress_map"] = upload_progress_map;

        boost::filesystem::path temp = _status_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_status, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _status_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _status_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output status info.")));
    }
    catch (...){
    }
}

bool launcher_job::load_status(std::wstring status_file){
    FUN_TRACE;
    try{
        if (boost::filesystem::exists(status_file)){
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            //_is_interrupted = find_value(job_status.get_obj(), "is_interrupted").get_bool();
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
           
            if (job_status.get_obj().count("is_initialized"))
                _is_initialized = find_value(job_status.get_obj(), "is_initialized").get_bool();
            
            _state = find_value(job_status.get_obj(), "state").get_int();

            if (job_status.get_obj().count("is_windows_update"))
                _is_windows_update = find_value(job_status.get_obj(), "is_windows_update").get_bool();

            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _latest_update_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "latest_update_time").get_str());
            mArray histories = find_value(job_status.get_obj(), "histories").get_array();
            foreach(mValue &h, histories){
                std::vector<std::string> arguments;
                mArray args = find_value(h.get_obj(), "arguments").get_array();
                foreach(mValue a, args){
                    arguments.push_back(a.get_str());
                }
                _histories.push_back(history_record::ptr(new history_record(
                    find_value_string(h.get_obj(), "time"),
                    find_value_string(h.get_obj(), "original_time"),
                    (saasame::transport::job_state::type)find_value_int32(h.get_obj(), "state"),
                    find_value_int32(h.get_obj(), "error"),
                    find_value_string(h.get_obj(), "description"),
                    find_value_string(h.get_obj(), "format"),
                    arguments)));
            }
            _boot_disk = find_value_string(job_status.get_obj(), "boot_disk");
            _platform = find_value_string(job_status.get_obj(), "platform");
            _architecture = find_value_string(job_status.get_obj(), "architecture");
            _total_upload_size = find_value_int64(job_status.get_obj(), "total_upload_size");
            _upload_progress = find_value_int64(job_status.get_obj(), "upload_progress");
            _disk_size = find_value_int32(job_status.get_obj(), "disk_size");
            _upload_id = find_value_int32(job_status.get_obj(), "upload_id");
            _uploading_disk = find_value_string(job_status.get_obj(), "uploading_disk");
            _host_name = find_value_string(job_status.get_obj(), "host_name");
            _virtual_machine_id = macho::stringutils::convert_utf8_to_unicode(find_value_string(job_status.get_obj(), "virtual_machine_id"));
            const mObject job_status_obj = job_status.get_obj();
            mObject::const_iterator x = job_status_obj.find("upload_progress_map");
            if (x != job_status_obj.end()){
                mArray upload_progress_map = find_value(job_status.get_obj(), "upload_progress_map").get_array();
                foreach(mValue &p, upload_progress_map){
                    std::string disk = find_value_string(p.get_obj(),"disk");
                    _upload_maps[disk] = upload_progress::ptr(new upload_progress(
                        find_value_string(p.get_obj(), "upload_id"),
                        find_value_int64(p.get_obj(), "vhd_size"),
                        find_value_int64(p.get_obj(), "size"),
                        find_value_int64(p.get_obj(), "progress"),
                        find_value_bool(p.get_obj(), "is_completed")));
                }
            }
            return true;
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read status info.")));
    }
    catch (...)
    {
    }
    return false;
}

saasame::transport::launcher_job_detail launcher_job::get_job_detail(bool complete) {
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    launcher_job_detail update_detail;
    update_detail.created_time = boost::posix_time::to_simple_string(_created_time);
    update_detail.id = macho::guid_(_id);
    update_detail.replica_id = _launcher_job_create_detail.replica_id;
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    update_detail.updated_time = boost::posix_time::to_simple_string(update_time);
    update_detail.boot_disk = _boot_disk;
    update_detail.is_windows_update = _is_windows_update;

    update_detail.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
    update_detail.__set_created_time(update_detail.created_time);
    update_detail.__set_id(update_detail.id);
    update_detail.__set_updated_time(update_detail.updated_time);
    update_detail.__set_boot_disk(update_detail.boot_disk);
    update_detail.__set_replica_id(update_detail.replica_id);
    update_detail.__set_state(update_detail.state);
    update_detail.__set_is_windows_update(update_detail.is_windows_update);
    update_detail.__set_platform(_platform);
    update_detail.__set_architecture(_architecture);
    update_detail.__set_vhd_size(_total_upload_size);
    update_detail.__set_progress(_upload_progress);
    update_detail.__set_size(_disk_size);
    update_detail.__set_upload_id(_upload_id);
    update_detail.__set_host_name(_host_name);
    if (complete){
        foreach(history_record::ptr &h, _histories){
            saasame::transport::job_history _h;
            _h.state = h->state;
            _h.error = h->error;
            _h.description = h->description;
            _h.time = boost::posix_time::to_simple_string(h->time);
            _h.format = h->format;
            _h.is_display = h->is_display;
            _h.arguments = h->args;
            update_detail.histories.push_back(_h);
        }
        update_detail.__set_histories(update_detail.histories);
    }
    update_detail.is_error = (_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error;
    update_detail.__set_is_error(update_detail.is_error);
    foreach(upload_progress::map::value_type &p, _upload_maps){
        update_detail.vhd_upload_progress[p.first].upload_id = p.second->upload_id;
        update_detail.vhd_upload_progress[p.first].vhd_size = p.second->vhd_size;
        update_detail.vhd_upload_progress[p.first].size = p.second->size;
        update_detail.vhd_upload_progress[p.first].progress = p.second->progress;
        update_detail.vhd_upload_progress[p.first].completed = p.second->is_completed;
    }
    update_detail.__set_vhd_upload_progress(update_detail.vhd_upload_progress);
    return update_detail;
}

void launcher_job::interrupt(){
    FUN_TRACE;
    _is_interrupted = true;
#ifdef _VMWARE_VADP
    _portal_ex.interrupt(true);
#endif
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){
        return;
    }
    if (_running.try_lock()){
        save_status();
        _running.unlock();
    }
}

void launcher_job::_update( bool loop ){
    FUN_TRACE;
    launcher_job_detail update_detail;
    update_detail.created_time = boost::posix_time::to_simple_string(_created_time);
    update_detail.id = macho::guid_(_id);
    int count = 1;
    int err_count = 1;
    do{
        if (!_terminated && loop) 
            boost::this_thread::sleep(boost::posix_time::seconds(15));
        {
            macho::windows::auto_lock lock(_cs);
            boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
            update_detail.updated_time = boost::posix_time::to_simple_string(update_time);
            update_detail.boot_disk = _boot_disk;
            update_detail.replica_id = _launcher_job_create_detail.replica_id;
            update_detail.is_windows_update = _is_windows_update;
            update_detail.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
            update_detail.__set_created_time(update_detail.created_time);
            update_detail.__set_id(update_detail.id);
            update_detail.__set_updated_time(update_detail.updated_time);
            update_detail.__set_boot_disk(update_detail.boot_disk);
            update_detail.__set_replica_id(update_detail.replica_id);
            update_detail.__set_state(update_detail.state);
            update_detail.__set_is_windows_update(update_detail.is_windows_update);
            update_detail.__set_platform(_platform);
            update_detail.__set_architecture(_architecture);
            update_detail.__set_vhd_size(_total_upload_size);
            update_detail.__set_progress(_upload_progress);
            update_detail.__set_size(_disk_size);
            update_detail.__set_upload_id(_upload_id);
            update_detail.__set_host_name(_host_name);
            update_detail.__set_virtual_machine_id(macho::stringutils::convert_unicode_to_utf8(_virtual_machine_id));
            update_detail.histories.clear();
            foreach(history_record::ptr &h, _histories){
                if (h->time > _latest_update_time){
                    saasame::transport::job_history _h;
                    _h.state = h->state;
                    _h.error = h->error;
                    _h.description = h->description;
                    _h.time = boost::posix_time::to_simple_string(h->time);
                    _h.format = h->format;
                    _h.is_display = h->is_display;
                    _h.arguments = h->args;
                    _h.__set_arguments(_h.arguments);
                    update_detail.histories.push_back(_h);
                }
            }
            update_detail.__set_histories(update_detail.histories);
            update_detail.is_error = (_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error;
            update_detail.__set_is_error(update_detail.is_error);
            foreach(upload_progress::map::value_type &p, _upload_maps){
                update_detail.vhd_upload_progress[p.first].upload_id = p.second->upload_id;
                update_detail.vhd_upload_progress[p.first].vhd_size = p.second->vhd_size;
                update_detail.vhd_upload_progress[p.first].size = p.second->size;
                update_detail.vhd_upload_progress[p.first].progress = p.second->progress;
                update_detail.vhd_upload_progress[p.first].completed = p.second->is_completed;
            }
            update_detail.__set_vhd_upload_progress(update_detail.vhd_upload_progress);
            if (update_state(update_detail)){
                _latest_update_time = update_time;
                if (_terminated){
                    err_count = 1;
                    if (count)
                    {
                        count--;
                        boost::this_thread::sleep(boost::posix_time::seconds(15));
                    }
                    else if (!update_detail.histories.size())
                        break;
                }
            }
            else if ( loop )
            {
                LOG(LOG_LEVEL_WARNING, L"Cannot update launcher status to mgmt service.");
                if (_terminated)
                {
                    if (err_count)
                    {
                        err_count--;
                        boost::this_thread::sleep(boost::posix_time::seconds(15));
                    }
                    else
                    {
                        LOG(LOG_LEVEL_ERROR, L"Abort to update last launcher status to mgmt service due to retry exhausted.");
                        break;
                    }
                }
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"Cannot update launcher status to mgmt service.");
            }
        }
    } while (loop);
}

std::string  launcher_job::read_from_file(boost::filesystem::path file_path){
    std::fstream file(file_path.string(), std::ios::in | std::ios::binary);
    std::string result = std::string();
    if (file.is_open()){
        file.seekg(0, std::ios::end);
        result.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&result[0], result.size());
        file.close();
    }
    return result;
}

ULONGLONG launcher_job::get_version(std::string version_string){
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

bool launcher_job::linux_convert(disk_map &_disk_map){
    FUN_TRACE;
    bool result = true;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path qemu_install_dir = work_dir / "emulator\\qemu";
    registry reg;
    int shrink = 0;
    if (reg.open(_T("SOFTWARE\\QEMU")) && reg[_T("Install_Dir")].exists()){
        qemu_install_dir = reg[_T("Install_Dir")].wstring();
    }

    boost::filesystem::path emulator = qemu_install_dir / "qemu-system-x86_64.exe";
    boost::filesystem::path linux_launcher = work_dir / "emulator\\linux_launcher.img";
    
    if (macho::windows::environment::is_winpe()){
        if (!boost::filesystem::exists(emulator)){
            LOG(LOG_LEVEL_RECORD, L"Looking for emulator in other drive.");
            storage::ptr stg = storage::local();
            storage::volume::vtr vols = stg->get_volumes();
            foreach(storage::volume::ptr v, vols){
                bool found = false;
                foreach(std::wstring access_path, v->access_paths()){
                    if (temp_drive_letter::is_drive_letter(access_path)){
                        LOG(LOG_LEVEL_RECORD, L"Searching volume: %s", access_path.c_str());
                        emulator = boost::filesystem::path(access_path) / "emulator\\qemu\\qemu-system-x86_64.exe";
                        linux_launcher = boost::filesystem::path(access_path) / "emulator\\linux_launcher.img";
                        found = boost::filesystem::exists(emulator) && boost::filesystem::exists(linux_launcher);
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
    }

    if ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN)
        || (_launcher_job_create_detail.options_type == extra_options_type::TENCENT)){
        disk_map boot_disk;
        if (_disk_map.size() > 1){
            foreach(disk_map::value_type d, _disk_map){
                universal_disk_rw::ptr u = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d.second));
                if (u && clone_disk::is_bootable_disk(u)){
                    boot_disk[d.first] = d.second;
                    break;
                }
            }        
        }
        else{
            boot_disk = _disk_map;
        }
        if (boot_disk.size() > 0){
            universal_disk_rw::ptr io = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % boot_disk.begin()->second));
            uint64_t aliyun_max_size = ((uint64_t)_launcher_job_create_detail.options.aliyun.max_size << 30);
            if (clone_disk::get_boundary_of_partitions(io) > aliyun_max_size){
                if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)) && reg[L"LinuxShrink"].exists() && (DWORD)reg[L"LinuxShrink"] > 0){
                    shrink = _launcher_job_create_detail.options.aliyun.max_size;
                }
                else{
                    record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Disk partition is larger than %1%G.") % _launcher_job_create_detail.options.aliyun.max_size));
                    LOG(LOG_LEVEL_ERROR, L"Disk partition is larger than %dG.", _launcher_job_create_detail.options.aliyun.max_size);
                    result = false;
                }
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot find any bootable disk.");
            record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Cannot find any bootable disk.")));
            result = false;
        }
    }

    if (!result){}
    else if (!(result = boost::filesystem::exists(emulator))){
        LOG(LOG_LEVEL_ERROR, L"Qemu emulater %s does not exist.", emulator.wstring().c_str());
    } 
    else if (!(result = boost::filesystem::exists(linux_launcher))){
        LOG(LOG_LEVEL_ERROR, L"Linux Launcher image %s does not exist.", linux_launcher.wstring().c_str());
    }
    else{
        macho::windows::process::ptr p = macho::windows::process::find(emulator);
        while (p){
            LOG(LOG_LEVEL_WARNING, L"Qemu emulater %s was running. Try to terminate it...", emulator.wstring().c_str());
            if (!p->terminate())
                break;
            boost::this_thread::sleep(boost::posix_time::seconds(5));
            p = macho::windows::process::find(emulator);
        }
        result = false;
        std::string conversion_type = "Unknown";
        int selinux = 1;
        if (macho::windows::environment::is_winpe() || _launcher_job_create_detail.target_type == conversion_type::AUTO){
            wmi_services wmi;
            wmi.connect(L"CIMV2");
            wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
            std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
            std::wstring model = macho::stringutils::tolower((std::wstring)computer_system[L"Model"]);

            if (std::wstring::npos != manufacturer.find(L"vmware")){
                conversion_type = "VMWare";
            }
            else if (std::wstring::npos != manufacturer.find(L"microsoft")){
                conversion_type = "Microsoft";
            }
            else if (std::wstring::npos != manufacturer.find(L"openstack") ||
                std::wstring::npos != model.find(L"openstack")){
                conversion_type = "OpenStack";
            } else if (std::wstring::npos != manufacturer.find(L"alibaba")){
                conversion_type = "OpenStack";
                selinux = 0;
            }
            else if (std::wstring::npos != manufacturer.find(L"bochs") || 
                std::wstring::npos != manufacturer.find(L"qemu") || 
                std::wstring::npos != manufacturer.find(L"sangfor")){
                conversion_type = "OpenStack";
            }
            else if (std::wstring::npos != manufacturer.find(L"xen")){
                conversion_type = "Xen";
            }
            else if (std::wstring::npos != manufacturer.find(L"red hat") && std::wstring::npos != model.find(L"kvm")) {
                conversion_type = "OpenStack";
                device_manager devmgr;
                hardware_device::vtr devices = devmgr.get_devices(L"DiskDrive");
                if (devices.size()){
                    foreach(hardware_device::ptr dev, devices){
                        hardware_device::ptr parent = devmgr.get_device(dev->parent);
                        if (parent){
                            if (macho::guid_(parent->sz_class_guid) == macho::guid_(L"{4d36e96a-e325-11ce-bfc1-08002be10318}")){ // HDC
                                conversion_type = "Unknown";
                                break;
                            }
                        }
                    }
                }
            }
            else if (!macho::windows::environment::is_winpe() && ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN &&
                !_launcher_job_create_detail.options.aliyun.bucketname.empty() &&
                !_launcher_job_create_detail.options.aliyun.region.empty() &&
                !_launcher_job_create_detail.options.aliyun.objectname.empty() &&
                !_launcher_job_create_detail.options.aliyun.access_key.empty() &&
                !_launcher_job_create_detail.options.aliyun.secret_key.empty()) ||
                (_launcher_job_create_detail.options_type == extra_options_type::TENCENT &&
                !_launcher_job_create_detail.options.tencent.bucketname.empty() &&
                !_launcher_job_create_detail.options.tencent.region.empty() &&
                !_launcher_job_create_detail.options.tencent.objectname.empty() &&
                !_launcher_job_create_detail.options.tencent.access_key.empty() &&
                !_launcher_job_create_detail.options.tencent.secret_key.empty())
                )){
                conversion_type = "OpenStack";
            }
        }
        else{
            switch (_launcher_job_create_detail.target_type){
            case conversion_type::VMWARE:
                conversion_type = "VMWare";
#ifdef _VMWARE_VADP
                if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP && 
                    _launcher_job_create_detail.vmware.scsi_adapters.size() == 1){
                    boost::filesystem::path version_file = linux_launcher.parent_path() / "version.txt";
                    if (boost::filesystem::exists(version_file)){
                        std::vector<std::string> versions = stringutils::tokenize2(read_from_file(version_file), " ", 0, false);
                        if (2 == versions.size() && get_version(versions[1]) > get_version("1.7.35")){
                            switch ((mwdc::ironman::hypervisor::hv_controller_type)_launcher_job_create_detail.vmware.scsi_adapters.begin()->first){
                            case mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_PARA_VIRT_SCSI:
                                conversion_type = "VMWare_Paravirtual";
                                break;
                            case mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC:
                                conversion_type = "VMWare_LSI_Parallel";
                                break;
                            case mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC_SAS:
                                conversion_type = "VMWare_LSI_SAS";
                                break;
                            }
                        }
                    }
                }
#endif
                break;
            case conversion_type::HYPERV:
                conversion_type = "Microsoft";
                break;
            case conversion_type::OPENSTACK:
                conversion_type = "OpenStack";
                selinux = 0;
                break;
            case conversion_type::XEN:
                conversion_type = "Xen";
                break;
            case conversion_type::ANY_TO_ANY:
            default:
                conversion_type = "Unknown";
                break;
            }
        }
        /*
         L"\"C:\\Program Files\\qemu\\qemu-system-x86_64.exe\" -device virtio-scsi-pci,id=scsi"
        L" -device scsi-hd,drive=hd0 -drive if=none,id=hd0,file=\"G:\\linuxlauncher.img\",index=0,media=disk,snapshot=on"
        //L" -device scsi-hd,drive=hd1 -drive if=none,id=hd1,file=\"G:\\Oracle_Linux_6.9_LVM\\Oracle_Linux_6.vhd\",index=1,media=disk,snapshot=off,format=vpc" 
        L" -device scsi-hd,drive=hd1 -drive if=none,id=hd1,file=\"\\\\.\\PhysicalDrive4\",index=1,media=disk,snapshot=off,format=raw" 
        L" -net nic,macaddr=ba:be:00:fa:ce:01,model=virtio -net user,hostfwd=tcp::2003-:22 -boot c -m 1024 -localtime -name Oracle_Linux_6.9_LVM");
        */
        //srand(time(NULL)); // Seed the time
        //int port = rand() % (15000 - 10000 + 1) + 10000;

        int memory_size = 256;
#ifdef _DEBUG
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"DisableSELinux"].exists() && reg[L"DisableSELinux"].is_dword() && (DWORD)reg[L"DisableSELinux"] > 0)
                selinux = 0;
            if (reg[L"EmulatorMemorySize"].exists() && reg[L"EmulatorMemorySize"].is_dword() && (DWORD)reg[L"EmulatorMemorySize"] >= 256){
                int m = (DWORD)reg[L"EmulatorMemorySize"];
                memory_size =(((m - 1) / 256) + 1 ) * 256;
            }
        }
#endif
        int port = g_saasame_constants.LAUNCHER_EMULATOR_PORT;
        std::wstring command = boost::str(boost::wformat(L"%s -nographic -net nic,macaddr=ba:be:00:fa:ce:01,model=virtio -net user,hostfwd=tcp::%d-:22 -boot c -m %d -localtime -name LinuxLauncher -device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd0 -drive if=none,id=hd0,file=\"%s\",index=0,media=disk,snapshot=on") % emulator.wstring() % port % memory_size % linux_launcher.wstring());
        int index = 1;
        foreach(disk_map::value_type &d, _disk_map){
            command.append(boost::str(boost::wformat(L" -device scsi-hd,drive=hd%1% -drive if=none,id=hd%1%,file=\"\\\\.\\PhysicalDrive%2%\",index=%1%,media=disk,snapshot=off,format=raw") % index %d.second));      
#ifdef _DEBUG
            macho::windows::registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"AppendOptions"].exists() && reg[L"AppendOptions"].is_string() && reg[L"AppendOptions"].wstring().length()){
                    command.append(reg[L"AppendOptions"].wstring());
                }
            }
#endif
            index++;
        }
        record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("Conversion started.")));
        LOG(LOG_LEVEL_RECORD, L"Start to convert the disks");
        _state = saasame::transport::job_state::type::job_state_converting;
        save_status();
        LOG(LOG_LEVEL_DEBUG, L"Command: %s)", command.c_str());
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (!CreateProcess(NULL,        // No module name (use command line)
            (LPTSTR)command.c_str(),    // Command line
            NULL,                       // Process handle not inheritable
            NULL,                       // Thread handle not inheritable
            FALSE,                      // Set handle inheritance to FALSE
            0,                          // No creation flags
            NULL,                       // Use parent's environment block
            NULL,                       // Use parent's starting directory 
            &si,                        // Pointer to STARTUPINFO structure
            &pi)                        // Pointer to PROCESS_INFORMATION structure
            )
        {
            DWORD error = GetLastError();
            LOG(LOG_LEVEL_ERROR, L"Cannot launch Linux Launcher emulator. Error(%d, %s)", error, environment::get_system_message(error).c_str());
        }
        else{
            unsigned long exit = 0;  //process exit code
            boost::this_thread::sleep(boost::posix_time::seconds(15));
            GetExitCodeProcess(pi.hProcess, &exit);      //while the process is running
            if (exit == STILL_ACTIVE){
                ssh_client::ptr ssh;
                boost::posix_time::ptime  start_time = boost::posix_time::microsec_clock::universal_time();
                boost::posix_time::time_duration diff = boost::posix_time::minutes(15);
                LOG(LOG_LEVEL_RECORD, L"Try to connect to Linux Launcher (%d)", port);
                while (NULL == ssh){
                    diff = boost::posix_time::microsec_clock::universal_time() - start_time;
                    if (diff.total_seconds() >= 900)
                        break;
                    boost::this_thread::sleep(boost::posix_time::seconds(30));
                    ssh = ssh_client::connect("127.0.0.1", "root", "Cloud168.ssm", port);
                }
                if (ssh){
                    LOG(LOG_LEVEL_RECORD, L"Connected to Linux Launcher");
                    std::string ret;
                    //ret = ssh->run("systemctl stop linux2v");
                    //boost::this_thread::sleep(boost::posix_time::seconds(3));
#ifdef _DEBUG
                    macho::windows::registry reg;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"PreCommands"].exists() && reg[L"PreCommands"].is_multi_sz()){
                            for(int i = 0; i < reg[L"PreCommands"].get_multi_count(); ++i){
                                std::wstring command = reg[L"PreCommands"].get_multi_at(i);
                                if (command.length()){
                                    LOG(LOG_LEVEL_RECORD, L"Pre command : %s", command.c_str());
                                    std::string r = ssh->run(macho::stringutils::convert_unicode_to_ansi(command));
                                    LOG(LOG_LEVEL_RECORD, L"Result : %s", macho::stringutils::convert_ansi_to_unicode(r).c_str());
                                }
                            }                     
                        }
                    }
#endif
                    std::string callbacks;
                    if (_launcher_job_create_detail.detect_type != disk_detect_type::type::EXPORT_IMAGE){
                        int c = 0;
                        foreach(const std::string& callback, _launcher_job_create_detail.callbacks){
                            if (c > 0)
                                callbacks.append(";");
                            callbacks.append(callback);
                            c++;
                        }
                    }
                    LOG(LOG_LEVEL_RECORD, L"Linux conversion type : %s, %d", macho::stringutils::convert_ansi_to_unicode(conversion_type).c_str(), selinux);
                    std::string convert = boost::str(boost::format("/usr/local/linux2v/bin/linux2v convert \"%s\" %s %d 1 %d") % callbacks %conversion_type %_launcher_job_create_detail.callback_timeout %selinux);
                    if (shrink > 0 ){
                        convert = boost::str(boost::format("/usr/local/linux2v/bin/linux2v convert \"%s\" %s %d 1 %d bios %d") % callbacks %conversion_type %_launcher_job_create_detail.callback_timeout %selinux %shrink);
                    }
                    if (macho::windows::environment::is_winpe()){
                        GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);
                        if (GetLastError() == ERROR_INVALID_FUNCTION) { // This.. is.. LEGACY BIOOOOOOOOS....
                            LOG(LOG_LEVEL_RECORD, L"BIOS Mode : Legacy");
                            _launcher_job_create_detail.gpt_to_mbr = true;
                        }
                        else{
                            LOG(LOG_LEVEL_RECORD, L"BIOS Mode : UEFI");
                            convert = boost::str(boost::format("/usr/local/linux2v/bin/linux2v convert \"%s\" %s %d 1 %d efi") % callbacks %conversion_type %_launcher_job_create_detail.callback_timeout %selinux);
                        }
                    }
#ifdef _DEBUG
                    LOG(LOG_LEVEL_RECORD, L"Conversion command : %s", macho::stringutils::convert_ansi_to_unicode(convert).c_str());
#endif
                    if (boost::filesystem::exists(_post_script_file)){
                        LOG(LOG_LEVEL_RECORD, L"Upload file : %s", _post_script_file.wstring().c_str());
                        ssh->upload_file("/root/post_script.tar.gz", 0644, _post_script_file);
                    }
                    if (!_launcher_job_create_detail.network_infos.empty()){
                        LOG(LOG_LEVEL_RECORD, L"Upload network settings");
                        ssh->upload_file("/root/network_infos.json", 0644, network_settings::to_string(_launcher_job_create_detail.network_infos, ""));
                    }
                    ret = ssh->run(convert);
#ifdef _DEBUG
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"PostCommands"].exists() && reg[L"PostCommands"].is_multi_sz()){
                            for (int i = 0; i < reg[L"PostCommands"].get_multi_count(); ++i){
                                std::wstring command = reg[L"PostCommands"].get_multi_at(i);
                                if (command.length()){
                                    LOG(LOG_LEVEL_RECORD, L"Post command : %s", command.c_str());
                                    std::string r = ssh->run(macho::stringutils::convert_unicode_to_ansi(command));
                                    LOG(LOG_LEVEL_RECORD, L"Result : %s", macho::stringutils::convert_ansi_to_unicode(r).c_str());
                                }
                            }
                        }
                    }
#endif
                    ssh->run("shutdown -P now");
                    if (!TerminateProcess(pi.hProcess, 1)){
                        DWORD error = GetLastError();
                        LOG(LOG_LEVEL_ERROR, L"Cannot terminate Linux Launcher emulator. Error(%d, %s)", error, environment::get_system_message(error).c_str());
                    }
                    std::vector<std::string> arr = stringutils::tokenize2(ret, "\r\n", 0, false);

                    for (auto s : boost::adaptors::reverse(arr)){
                        if (result = (s.find("Succeeded.") == 0))
                            break;
                    }

                    if (!result){
                        LOG(LOG_LEVEL_ERROR, L"Launch Command: \r\n%s.", command.c_str());
                        LOG(LOG_LEVEL_ERROR, L"Disk(s) conversion failed. \r\n%s.", macho::stringutils::convert_ansi_to_unicode(ret).c_str());
                    }
                    else{
                        for (auto s : boost::adaptors::reverse(arr)){
                            if (0 == s.find("PLATFORM:")){
                                _platform = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(s.substr(std::string("PLATFORM:").length())));
                            }
                            else if (0 == s.find("ARCHITECTURE:")){
                                _architecture = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(s.substr(std::string("ARCHITECTURE:").length())));
                            }
                            else if (0 == s.find("HOSTNAME:")){
                                _host_name = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(s.substr(std::string("HOSTNAME:").length())));
                            }
                            else if (0 == s.find("ERROR MESSAGE:")){
                                std::string error_message = macho::stringutils::remove_begining_whitespaces(macho::stringutils::erase_trailing_whitespaces(s.substr(std::string("ERROR MESSAGE:").length())));
                                if (!error_message.empty()){
                                    record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, (record_format(error_message)));
                                }
                            }
                            if (!_platform.empty() && !_architecture.empty())
                                break;
                        }

                        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            if (reg[L"DebugLinuxLauncher"].exists() && reg[L"DebugLinuxLauncher"].is_dword() && (DWORD)reg[L"DebugLinuxLauncher"] > 0){
                                LOG(LOG_LEVEL_RECORD, L"Result: \r\n%s", macho::stringutils::convert_ansi_to_unicode(ret).c_str());
                            }
                        }
                    }              
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"Cannot connect to Linux Launcher (timeout).");
                    if (!TerminateProcess(pi.hProcess, 1)){
                        DWORD error = GetLastError();
                        LOG(LOG_LEVEL_ERROR, L"Cannot terminate Linux Launcher emulator. Error(%d, %s)", error, environment::get_system_message(error).c_str());
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Cannot launch Linux Launcher emulator. exit code (%d)", exit);
            }

            DWORD timeout = 30000;
            while (true){
                if (WAIT_TIMEOUT != WaitForSingleObject(pi.hProcess, timeout))
                    break;
                else{
                    if (!TerminateProcess(pi.hProcess, 1)){
                        DWORD error = GetLastError();
                        LOG(LOG_LEVEL_ERROR, L"Cannot terminate Linux Launcher emulator. Error(%d, %s)", error, environment::get_system_message(error).c_str());
                    }
                }
            }
            // Wait until child process exits.
           
            // Close process and thread handles. 
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    return result;
}

bool launcher_job::windows_convert(storage::ptr& stg, disk_map &_disk_map){
    bool result = false, foundsys = false;
    foreach(disk_map::value_type &d, _disk_map){
        if (_is_interrupted || _is_canceled)
            break;
        DWORD disk_number = d.second;
        irm_converter converter;
        if (!(result = converter.initialize(disk_number))){
            try{
                LOG(LOG_LEVEL_WARNING, L"Cannot find any system information from disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                storage::disk::ptr disk = stg->get_disk(disk_number);
                if (disk)
                    disk->offline();
                save_status();
            }
            catch (...){
            }
            continue;
        }
        else
        {
            foundsys = true;
            try{
                _boot_disk = d.first;
                record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("Conversion started.")));
                LOG(LOG_LEVEL_RECORD, L"Start to convert disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                _state = saasame::transport::job_state::type::job_state_converting;
                save_status();
                if ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN)
                    || (_launcher_job_create_detail.options_type == extra_options_type::TENCENT)
                    ){
                    storage::disk::ptr disk = stg->get_disk(disk_number);
                    uint64_t boot_disk_max_size = 500ULL * 1024 * 1024 * 1024;
                    if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN)
                        boot_disk_max_size = ((uint64_t)_launcher_job_create_detail.options.aliyun.max_size << 30);
                    else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT)
                        boot_disk_max_size = ((uint64_t)_launcher_job_create_detail.options.tencent.max_size << 30);
                    if (disk->size() > boot_disk_max_size){
                        result = false;
                        record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("The volume shrink operation started.")));
                        LOG(LOG_LEVEL_RECORD, L"The volume shrink operation started on disk(%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                        storage::partition::vtr parts = disk->get_partitions();
                        storage::partition::ptr part, ext_part;
                        foreach(storage::partition::ptr p, parts){
                            if (disk->partition_style() == storage::ST_PST_MBR && (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)){
                                if (ext_part){
                                    if (ext_part->offset() < p->offset())
                                        ext_part = p;
                                }
                                else{
                                    ext_part = p;
                                }
                            }
                            else if (NULL == part || part->offset() < p->offset())
                                part = p;
                        }
                        if (part && part->access_paths().size()){
                            if ((part->offset() + ((uint64_t)16 << 20)) >= boot_disk_max_size){
                                record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("There is not enough space available on the disk(s) to complete the shrink operation.")));
                                LOG(LOG_LEVEL_RECORD, L"There is not enough space available on the disk(s) to complete the shrink operation. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                            }
                            else if (ext_part && ((part->offset() > ext_part->offset() && ((part->offset() + part->size()) <= (ext_part->offset() + ext_part->size()))))){
                                record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("There is not enough space available on the disk(s) to complete the shrink operation.")));
                                LOG(LOG_LEVEL_RECORD, L"There is not enough space available on the disk(s) to complete the shrink operation. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                            }
                            else{
                                uint64_t size = (boot_disk_max_size - part->offset() - ((uint64_t)16 << 20));
                                shrink_volume::ptr shrink = shrink_volume::open(part->access_paths()[0], size);
                                if (shrink){
                                    if (!shrink->can_be_shrink()){
                                        record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("There is not enough space available on the disk(s) to complete the shrink operation.")));
                                        LOG(LOG_LEVEL_RECORD, L"There is not enough space available on the disk(s) to complete the shrink operation. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                    }
                                    else if (result = shrink->shrink()){
                                        record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("The volume shrink operation completed.")));
                                        LOG(LOG_LEVEL_RECORD, L"The volume shrink operation completed on disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                    }
                                    else{
                                        record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("The volume shrink operation failed.")));
                                        LOG(LOG_LEVEL_ERROR, L"The volume shrink operation failed on disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                    }
                                }
                                else{
                                    record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("The volume shrink operation failed.")));
                                    LOG(LOG_LEVEL_ERROR, L"The volume shrink operation failed on disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                }
                            }
                        }
                        else{
                            record(saasame::transport::job_state::job_state_converting, saasame::transport::error_codes::SAASAME_NOERROR, (record_format("The volume shrink operation failed.")));
                            LOG(LOG_LEVEL_ERROR, L"The volume shrink operation failed on disk (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                        }
                    }
                }

                if (result){
                    if (macho::windows::environment::is_winpe()){
                        GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);
                        if (GetLastError() == ERROR_INVALID_FUNCTION) { // This.. is.. LEGACY BIOOOOOOOOS....
                            LOG(LOG_LEVEL_RECORD, L"BIOS Mode : Legacy");
                            _launcher_job_create_detail.gpt_to_mbr = true;
                        }
                        else{
                            LOG(LOG_LEVEL_RECORD, L"BIOS Mode : UEFI");
                            _launcher_job_create_detail.gpt_to_mbr = false;
                            storage::disk::ptr disk = stg->get_disk(disk_number);
                            if (disk->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_MBR){
                                if (!converter.remove_hybird_mbr()){
                                    LOG(LOG_LEVEL_ERROR, L"Please make sure MBR disk to boot from UEFI BIOS.");
                                    record(saasame::transport::job_state::type::job_state_converting,
                                        saasame::transport::error_codes::SAASAME_NOERROR,
                                        (record_format("Please make sure MBR disk to boot from UEFI BIOS.")));                                   
                                    registry reg(REGISTRY_CREATE);
                                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                                        reg[L"MBROnUEFIBIOS"] = (DWORD)0x1;
                                    }
                                }
                            }
                        }
                    }

                    if (_launcher_job_create_detail.gpt_to_mbr){
                        storage::disk::ptr disk = stg->get_disk(disk_number);
                        if (disk->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
                            storage::partition::vtr partitions = stg->get_partitions(disk_number);
                            uint64_t end_of_partitions = 0ULL;
                            foreach(storage::partition::ptr p, partitions){
                                uint64_t new_end_of_partitions = p->offset() + p->size();
                                if (end_of_partitions < new_end_of_partitions)
                                    end_of_partitions = new_end_of_partitions;
                            }
                            if (end_of_partitions > 2199023255040ULL){
                                result = false;
                                LOG(LOG_LEVEL_ERROR, L"Cannot convert partition style from GPT to MBR : disk(%s) size over 2 TB", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                record(saasame::transport::job_state::type::job_state_converting,
                                    saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
                                    (record_format("Cannot convert partition style from GPT to MBR : disk size over 2 TB")));
                            }
                            else if (!(result = converter.gpt_to_mbr())){
                                LOG(LOG_LEVEL_ERROR, L"Cannot convert partition style from GPT to MBR. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                record(saasame::transport::job_state::type::job_state_converting,
                                    saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
                                    (record_format("Cannot convert partition style from GPT to MBR.")));
                            }
                            else if (_launcher_job_create_detail.detect_type == disk_detect_type::type::CUSTOMIZED_ID){
                                int count = 10;
                                disk = stg->get_disk(disk_number);
                                while (count > 0){
                                    general_io_rw::ptr wd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % disk_number), false);
                                    if (wd){
                                        uint32_t r = 0, w = 0;
                                        uint64_t sec = (disk->size() >> 9) - 40;
                                        std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                                        memset(data.get(), 0, 512);
                                        if (wd->sector_read(sec, 1, data.get(), r)){
                                            GUID guid = macho::guid_(_launcher_job_create_detail.disks_lun_mapping[_boot_disk]);
                                            memcpy(&(data.get()[512 - 16]), &guid, 16);
                                            result = wd->sector_write(sec, data.get(), 1, w);
                                        }
                                        count--;
                                        if (result)
                                            break;
                                        else
                                            boost::this_thread::sleep(boost::posix_time::seconds(5));
                                    }
                                }

                                if (!result){
                                    LOG(LOG_LEVEL_ERROR, L"Failed to reset CUSTOMIZED_ID. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                    record(saasame::transport::job_state::type::job_state_converting,
                                        saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
                                        (record_format("Cannot convert partition style from GPT to MBR.")));
                                }
                            }
                        }
                    }
                }
                if (result){
                    std::string default_mtu = "";
                    irm_conv_parameter parameter;
                    parameter.sectors_per_track = 63;
                    parameter.tracks_per_cylinder = 255;
                    parameter.is_disable_machine_password_change = _launcher_job_create_detail.is_disable_machine_password_change;
                    parameter.is_enable_debug = _launcher_job_create_detail.is_enable_debug;
                    parameter.is_force_normal_boot = _launcher_job_create_detail.is_force_normal_boot;
                    parameter.is_sysvol_authoritative_restore = _launcher_job_create_detail.is_sysvol_authoritative_restore;
                    parameter.skip_system_injection = _launcher_job_create_detail.skip_system_injection;
                    if (_launcher_job_create_detail.detect_type != disk_detect_type::type::EXPORT_IMAGE){
                        parameter.callbacks = _launcher_job_create_detail.callbacks;
                        parameter.callback_timeout = _launcher_job_create_detail.callback_timeout;
                    }
                    for (int i = 0; i < 55; i++){
                        parameter.services[_services[i]] = 4;
                    }
                    {
                        wmi_services wmi;
                        wmi.connect(L"CIMV2");
                        wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
                        std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
                        std::wstring model = macho::stringutils::tolower((std::wstring)computer_system[L"Model"]);

                        if (macho::windows::environment::is_winpe()){
                            storage::disk::ptr _d = stg->get_disk(disk_number);
                            if (_d){
                                parameter.sectors_per_track = _d->sectors_per_track();
                                parameter.tracks_per_cylinder = _d->tracks_per_cylinder();
                            }
                            parameter.type = conversion::type::any_to_any;
                            /*
                            Serial number: SELECT IdentifyingNumber FROM Win32_ComputerSystemProduct
                            System Manufacturer: SELECT Manufacturer FROM Win32_ComputerSystem
                            System Model: SELECT Model FROM Win32_ComputerSystem
                            System Model Name: SELECT Name FROM Win32_ComputerSystemProduct
                            System Model Version: SELECT Version FROM Win32_ComputerSystemProduct
                            */
                            if (std::wstring::npos != manufacturer.find(L"vmware")){
                                parameter.type = conversion::type::vmware;
                                macho::windows::device_manager  dev_mgmt;
                                hardware_device::vtr class_devices;
                                class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
                                foreach(hardware_device::ptr &d, class_devices){
                                    irm_conv_device device;
                                    device.description = d->device_description;
                                    device.hardware_ids = d->hardware_ids;
                                    device.compatible_ids = d->compatible_ids;
                                    parameter.devices.push_back(device);
                                }
                                for (int i = 0; i < 3; i++){
                                    parameter.services[_services[i]] = 2;
                                }
                            }
                            else if (std::wstring::npos != manufacturer.find(L"microsoft")){
                                parameter.type = conversion::type::hyperv;
                                for (int i = 3; i < 5; i++){
                                    parameter.services[_services[i]] = 2;
                                }
                            }
                            else if (std::wstring::npos != manufacturer.find(L"ibm")){
                                for (int i = 8; i < 6; i++){
                                    parameter.services[_services[i]] = 2;
                                }
                            }
                            else if (std::wstring::npos != manufacturer.find(L"hewlett-packard")){ // Hewlett-Packard
                                for (int i = 15; i < 10; i++){
                                    parameter.services[_services[i]] = 2;
                                }
                            }
                            else if (std::wstring::npos != manufacturer.find(L"dell")){
                                for (int i = 25; i < 19; i++){
                                    if (_services[i][0] == _T('#'))
                                        parameter.services[_services[i]] = 3;
                                    else
                                        parameter.services[_services[i]] = 2;
                                }
                            }
                            else if (std::wstring::npos != manufacturer.find(L"openstack") || std::wstring::npos != model.find(L"openstack")){
                                parameter.type = conversion::type::openstack;
                                default_mtu = "1400";
                            }
                            else if (std::wstring::npos != manufacturer.find(L"bochs") ||
                                std::wstring::npos != manufacturer.find(L"sangfor")){
                                parameter.type = conversion::type::openstack;
                            }
                            else if (std::wstring::npos != manufacturer.find(L"qemu"))
                                parameter.type = conversion::type::qemu;
                            else if (std::wstring::npos != manufacturer.find(L"alibaba"))
                                parameter.type = conversion::type::openstack;
                            else if (std::wstring::npos != manufacturer.find(L"xen")){
                                parameter.type = conversion::type::xen;
                                for (int i = 44; i < 48; i++){
                                    parameter.services[_services[i]] = 0;
                                }
                                for (int i = 48; i < 51; i++){
                                    parameter.services[_services[i]] = 2;
                                }
                                for (int i = 51; i < 54; i++){
                                    parameter.services[_services[i]] = 3;
                                }
                            }
                            else{
                                if (std::wstring::npos != manufacturer.find(L"red hat") && std::wstring::npos != model.find(L"kvm"))
                                    parameter.type = conversion::type::openstack;
                                device_manager devmgr;
                                hardware_device::vtr devices = devmgr.get_devices(L"DiskDrive");
                                if (devices.size()){
                                    foreach(hardware_device::ptr dev, devices){
                                        hardware_device::ptr parent = devmgr.get_device(dev->parent);
                                        if (parent){
                                            if (macho::guid_(parent->sz_class_guid) == macho::guid_(L"{4d36e96a-e325-11ce-bfc1-08002be10318}")){ // HDC
                                                parameter.type = conversion::type::hyperv;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else{
                            if (_launcher_job_create_detail.target_type == conversion_type::AUTO){
#ifdef _DEBUG
                                parameter.type = conversion::type::openstack;
                                default_mtu = "1400";
#else
                                if (std::wstring::npos != manufacturer.find(L"vmware"))
                                    parameter.type = conversion::type::vmware;
                                else if (std::wstring::npos != manufacturer.find(L"microsoft"))
                                    parameter.type = conversion::type::hyperv;
                                else if (std::wstring::npos != manufacturer.find(L"openstack") || std::wstring::npos != model.find(L"openstack")){
                                    parameter.type = conversion::type::openstack;
                                    default_mtu = "1400";
                                }
                                else if (std::wstring::npos != manufacturer.find(L"bochs") ||
                                    std::wstring::npos != manufacturer.find(L"sangfor")){
                                    parameter.type = conversion::type::openstack;
                                }
                                else if (std::wstring::npos != manufacturer.find(L"qemu"))
                                    parameter.type = conversion::type::qemu;
                                else if (std::wstring::npos != manufacturer.find(L"alibaba"))
                                    parameter.type = conversion::type::openstack;
                                else if (std::wstring::npos != manufacturer.find(L"xen"))
                                    parameter.type = conversion::type::xen;
                                else if ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN &&
                                    !_launcher_job_create_detail.options.aliyun.bucketname.empty() &&
                                    !_launcher_job_create_detail.options.aliyun.region.empty() &&
                                    !_launcher_job_create_detail.options.aliyun.objectname.empty() &&
                                    !_launcher_job_create_detail.options.aliyun.access_key.empty() &&
                                    !_launcher_job_create_detail.options.aliyun.secret_key.empty()) ||
                                    (_launcher_job_create_detail.options_type == extra_options_type::TENCENT &&
                                    !_launcher_job_create_detail.options.tencent.bucketname.empty() &&
                                    !_launcher_job_create_detail.options.tencent.region.empty() &&
                                    !_launcher_job_create_detail.options.tencent.objectname.empty() &&
                                    !_launcher_job_create_detail.options.tencent.access_key.empty() &&
                                    !_launcher_job_create_detail.options.tencent.secret_key.empty())
                                    ){
                                    parameter.type = conversion::type::openstack;
                                }
#endif
                            }
                            else{
                                parameter.type = (conversion::type)(int32_t)_launcher_job_create_detail.target_type;
                                if (parameter.type == conversion::type::openstack)
                                    default_mtu = "1400";
                            }
                        }
                    }
                    {
                        registry reg_local;
                        if (reg_local.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            if (reg_local[L"DrvRoot"].exists()){
                                parameter.drivers_path = reg_local[L"DrvRoot"].wstring();
                            }
                            else if (conversion::type::any_to_any == parameter.type&& macho::windows::environment::is_winpe()){
                                storage::ptr stg = storage::local();
                                storage::volume::vtr vols = stg->get_volumes();
                                foreach(storage::volume::ptr v, vols){
                                    bool found = false;
                                    boost::filesystem::path drv_path;
                                    foreach(std::wstring access_path, v->access_paths()){
                                        if (temp_drive_letter::is_drive_letter(access_path)){
                                            LOG(LOG_LEVEL_RECORD, L"Searching volume: %s", access_path.c_str());
                                            drv_path = boost::filesystem::path(access_path) / "drivers";
                                            found = boost::filesystem::exists(drv_path) && boost::filesystem::is_directory(drv_path);
                                            break;
                                        }
                                    }
                                    if (found){
                                        parameter.drivers_path = drv_path.wstring();
                                        macho::windows::device_manager  dev_mgmt;
                                        hardware_device::vtr class_devices;
                                        class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
                                        foreach(hardware_device::ptr &d, class_devices){
                                            irm_conv_device device;
                                            device.description = d->device_description;
                                            device.hardware_ids = d->hardware_ids;
                                            device.compatible_ids = d->compatible_ids;
                                            parameter.devices.push_back(device);
                                        }
                                        break;
                                    }
                                }
                            }
                            if (reg_local[L"SkipDriverInjection"].exists() && (DWORD)reg_local[L"SkipDriverInjection"] > 0){
                                parameter.drivers_path.clear();
                                parameter.type = conversion::type::any_to_any;
                            }
                            else if (reg_local[L"ConversionType"].exists() && reg_local[L"ConversionType"].is_dword()){
                                parameter.type = (conversion::type)(DWORD)reg_local[L"ConversionType"];
                            }
                            if (reg_local[L"DefaultMTU"].exists() && reg_local[L"DefaultMTU"].is_string()){
                                default_mtu = reg_local[L"DefaultMTU"].string();
                            }
                            reg_local.close();
                        }
                    }

                    parameter.config = network_settings::to_string(_launcher_job_create_detail.network_infos, default_mtu);
                    parameter.post_script = boost::filesystem::exists(_post_script_file) ? _post_script_file.string() : "";

                    if (!(result = converter.convert(parameter))){
                        LOG(LOG_LEVEL_ERROR, L"Disk conversion failed. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                        record(saasame::transport::job_state::type::job_state_converting,
                            saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
                            (record_format("Disk conversion failed.")));
                    }
                    else
                    {
                        macho::windows::operating_system os = converter.get_operating_system_info();
                        _platform = macho::stringutils::convert_unicode_to_utf8(os.name);
                        _architecture = macho::stringutils::convert_unicode_to_utf8(os.sz_cpu_architecture());
                        _host_name = macho::stringutils::convert_unicode_to_utf8(converter.computer_name());
                        if (converter.is_pending_windows_update()){
                            _is_windows_update = true;
                            record(saasame::transport::job_state::type::job_state_finished,
                                saasame::transport::error_codes::SAASAME_NOERROR,
                                (record_format("System booting takes longer due to Windows Update.")));
                        }
                        storage::disk::ptr _d = stg->get_disk(disk_number);
                        while (_d && !_d->is_offline()){
                            _d->offline();
                            boost::this_thread::sleep(boost::posix_time::seconds(2));
                            _d = stg->get_disk(disk_number);
                        }
                        LOG(LOG_LEVEL_RECORD, L"System disk conversion completed. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                    }
                }
            }
            catch (macho::exception_base& ex){
                record(saasame::transport::job_state::type::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
                result = false;
            }
            catch (const boost::filesystem::filesystem_error& ex){
                record(saasame::transport::job_state::type::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, record_format(ex.what()));
                result = false;
            }
            catch (const boost::exception &ex){
                record(saasame::transport::job_state::type::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
                result = false;
            }
            catch (const std::exception& ex){
                record(saasame::transport::job_state::type::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, record_format(ex.what()));
                result = false;
            }
            catch (...){
                record(saasame::transport::job_state::type::job_state_converting, saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL, record_format("Unknown exception"));
                result = false;
            }
            break;
        }
    }
    if (!foundsys){
        record(saasame::transport::job_state::job_state_initialed,
            saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
            record_format("Cannot find any system information from disk(s)."));
    }
    return result;
}

#ifdef _VMWARE_VADP

vmware_virtual_machine::ptr launcher_job::clone_virtual_machine(vmware_portal_::ptr &portal, mwdc::ironman::hypervisor_ex::vmware_portal_ex &portal_ex, vmware_virtual_machine::ptr src_vm){
    vmware_virtual_machine::ptr vm;
    portal_ex.set_portal(portal, src_vm->key);
    std::wstring snapshot_ref;
    std::wstring datastore = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.datastore);
    std::wstring folder_path = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.folder_path);
    std::wstring virtual_machine_snapshot = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.virtual_machine_snapshot);
    std::wstring virtual_machine_name = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.vm_name);
    if (virtual_machine_name.empty()){
        virtual_machine_name = src_vm->name + L"-" + _id.substr(0, 8);
    }
    else{
        std::wstring vm_name = virtual_machine_name;
        key_map vm_name_map;
        vmware_host::vtr hosts = portal->get_hosts();
        foreach(vmware_host::ptr& h, hosts){
            foreach(key_map::value_type v, h->vms){
                vm_name_map[v.second] = v.first;
            }
        }
        int index = 0;
        while (vm_name_map.count(virtual_machine_name)){
            index++;
            virtual_machine_name = boost::str(boost::wformat(L"%s(%d)") % vm_name % index);
        }
    }
    portal_ex.get_snapshot_ref_item(src_vm->key,
        virtual_machine_snapshot,
        snapshot_ref);
    if (snapshot_ref.empty()){
        record(saasame::transport::job_state::type::job_state_none,
            saasame::transport::error_codes::SAASAME_NOERROR,
            (record_format("Missing virtual machine snapshot '%1%'.") % _launcher_job_create_detail.vmware.virtual_machine_snapshot));
    }
    else{
        mwdc::ironman::hypervisor::hv_connection_type connection_type = portal->get_connection_type();
        vmware_clone_virtual_machine_config_spec clone_spec;
        clone_spec.vm_name = virtual_machine_name;
        clone_spec.snapshot_mor_item = snapshot_ref;
        clone_spec.datastore = datastore;
        clone_spec.folder_path = folder_path;
        clone_spec.uuid = _id;
        switch (_launcher_job_create_detail.mode){
        case recovery_type::type::DISASTER_RECOVERY:
            clone_spec.disk_move_type = vmware_clone_virtual_machine_config_spec::createNewChildDiskBacking;
            clone_spec.transformation_type = vmware_clone_virtual_machine_config_spec::Transformation__flat;
            break;
        case recovery_type::type::TEST_RECOVERY:
            clone_spec.disk_move_type = vmware_clone_virtual_machine_config_spec::moveChildMostDiskBacking;
            clone_spec.transformation_type = vmware_clone_virtual_machine_config_spec::Transformation__flat;
            break;
        };
        if (mwdc::ironman::hypervisor::hv_connection_type::HV_CONNECTION_TYPE_VCENTER == connection_type){
            record(saasame::transport::job_state::type::job_state_none,
                saasame::transport::error_codes::SAASAME_NOERROR,
                (record_format("Creating virtual machine.")));
            vm = portal->clone_virtual_machine(src_vm->key, clone_spec, boost::bind(&launcher_job::clone_progess, this, _1, _2));
            if (vm){
                record(saasame::transport::job_state::type::job_state_none,
                    saasame::transport::error_codes::SAASAME_NOERROR,
                    (record_format("Progress : %1%") % boost::str(boost::format("%1%") % 100).append("%")));
                record(saasame::transport::job_state::type::job_state_none,
                    saasame::transport::error_codes::SAASAME_NOERROR,
                    (record_format("Virtual machine '%1%' Created.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
            }
            else{
                record(saasame::transport::job_state::type::job_state_none,
                    saasame::transport::error_codes::SAASAME_E_FAIL,
                    (record_format("%1%") % macho::stringutils::convert_unicode_to_utf8(portal->get_last_error_message())));
            }
        }
        else if (mwdc::ironman::hypervisor::hv_connection_type::HV_CONNECTION_TYPE_HOST == connection_type){
            if (_launcher_job_create_detail.mode == recovery_type::type::DISASTER_RECOVERY){
                record(saasame::transport::job_state::type::job_state_none,
                    saasame::transport::error_codes::SAASAME_NOERROR,
                    (record_format("Reverting to selected snapshot in preparation for virtual machine recovery.")));
                vmware_vm_revert_to_snapshot_parm snapshot_request_parm;
                snapshot_request_parm.mor_ref = snapshot_ref;
                int nReturn = portal->revert_virtual_machine_snapshot(src_vm->key, snapshot_request_parm, boost::bind(&launcher_job::clone_progess, this, _1, _2));
                if (nReturn == TASK_STATE::STATE_SUCCESS){
                    vm = src_vm;
                    record(saasame::transport::job_state::type::job_state_none,
                        saasame::transport::error_codes::SAASAME_NOERROR,
                        (record_format("Progress : %1%") % boost::str(boost::format("%1%") % 100).append("%")));
                }
                else{
                    record(saasame::transport::job_state::type::job_state_none,
                        saasame::transport::error_codes::SAASAME_E_FAIL,
                        (record_format("%1%") % macho::stringutils::convert_unicode_to_utf8(portal->get_last_error_message())));
                }
            }
            else{
                record(saasame::transport::job_state::type::job_state_none,
                    saasame::transport::error_codes::SAASAME_NOERROR,
                    (record_format("Creating virtual machine.")));
                vmware_virtual_machine_config_spec config_spec(*src_vm);
                config_spec.name = virtual_machine_name;
                config_spec.uuid = _id;
                config_spec.host_key = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.esx);
                config_spec.config_path = datastore;
                config_spec.folder_path = folder_path;
                config_spec.guest_id = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.guest_id);
                foreach(vmware_disk_info::ptr d, config_spec.disks){
                    d->name = boost::filesystem::path(d->name).filename().wstring();
                }
                foreach(vmware_virtual_network_adapter::ptr& n, config_spec.nics){
                    n->mac_address.clear();
                }
                vm = portal->create_virtual_machine(config_spec);
                if (!vm){
                    record(saasame::transport::job_state::type::job_state_none,
                        saasame::transport::error_codes::SAASAME_E_FAIL,
                        (record_format("%1%") % macho::stringutils::convert_unicode_to_utf8(portal->get_last_error_message())));
                }
                else{
                    bool result = false;
                    portal_ex.set_portal(portal, src_vm->key);
                    portal_ex.set_portal(portal, vm->key);
                    {
                        std::wstring host = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.host);
                        std::wstring user = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.username);
                        std::wstring password = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.password);
                        vmware_vixdisk_connection::ptr source_conn, target_conn;
                        vmware_snapshot_disk_info::map snapshot_disks = portal->get_snapshot_info(snapshot_ref);
                        source_conn = portal_ex.get_vixdisk_connection(host, user, password, src_vm->key, virtual_machine_snapshot);
                        target_conn = portal_ex.get_vixdisk_connection(host, user, password, vm->key, L"", false);
                        clone_disk_rw::map ins;
                        foreach(vmware_snapshot_disk_info::map::value_type &snap, snapshot_disks){
                            ins[snap.second->name] = clone_disk_rw(portal_ex.open_vmdk_for_rw(source_conn, snap.second->name), snap.second->size);
                        }
                        if (ins.size() != snapshot_disks.size()){
                            result = false;
                        }
                        else{
                            changed_disk_extent::map changed_disk_extent_map;
                            uint64_t totle_size = 0, progress = 0, percentage = 0;
                            typedef std::map<std::wstring, changed_area::vtr> changed_areas_map;
                            changed_areas_map totle_changed_areas;
                            totle_changed_areas = clone_disk::get_clone_data_range(ins);
                            foreach(vmware_snapshot_disk_info::map::value_type &snap, snapshot_disks){
                                universal_disk_rw::ptr in = ins[snap.second->name].rw;
                                changed_disk_extent::vtr excludeds = get_excluded_extents(totle_changed_areas[in->path()], snap.second->size);
                                vmdk_changed_areas changed = portal->get_vmdk_changed_areas(src_vm->vm_mor_item, snapshot_ref, snap.second->wsz_key(), L"*", 0LL, snap.second->size);
                                if (changed.last_error_code){
                                    changed.changed_list.clear();
                                    changed.changed_list.push_back(changed_disk_extent(0, snap.second->size));
                                }
                                changed_disk_extent_map[snap.second->name] = final_changed_disk_extents(changed.changed_list, excludeds, MAX_BLOCK_SIZE);
                                foreach(changed_disk_extent &ext, changed_disk_extent_map[snap.second->name]){
                                    totle_size += ext.length;
                                }
                            }

                            foreach(vmware_snapshot_disk_info::map::value_type &snap, snapshot_disks){
                                std::wstring name;
                                result = false;
                                foreach(vmware_disk_info::map::value_type& _d, vm->disks_map){
                                    if (_d.second->uuid == snap.second->uuid){
                                        name = _d.second->name;
                                        break;
                                    }
                                }
                                if (!name.empty()){
                                    universal_disk_rw::ptr out = portal_ex.open_vmdk_for_rw(target_conn, name, false);
                                    universal_disk_rw::ptr in = ins[snap.second->name].rw;
                                    if (in && out){
                                        std::string buf;
                                        foreach(changed_disk_extent &ext, changed_disk_extent_map[snap.second->name]){
                                            int retry = 0;
                                            int read_retry = 25;
                                            while ((read_retry > 0) && !(result = in->read(ext.start, ext.length, buf))){
                                                read_retry--;
                                                boost::this_thread::sleep(boost::posix_time::seconds(WAIT_INTERVAL_SECONDS));
                                                if (is_canceled())
                                                    break;
                                            }
                                            if (result){
                                                read_retry = 25;
                                                uint32_t number_of_bytes_written = 0;
                                                while ((read_retry > 0) && !(result = out->write(ext.start, buf, number_of_bytes_written))){
                                                    read_retry--;
                                                    boost::this_thread::sleep(boost::posix_time::seconds(WAIT_INTERVAL_SECONDS));
                                                    if (is_canceled())
                                                        break;
                                                }
                                            }
                                            if (!result)
                                                break;
                                            else if (is_canceled()){
                                                result = false;
                                                break;
                                            }
                                            else{
                                                progress += ext.length;
                                                uint64_t p = ((progress * 20) / totle_size) * 5;
                                                if (percentage != p){
                                                    percentage = p;
                                                    record(saasame::transport::job_state::type::job_state_none,
                                                        saasame::transport::error_codes::SAASAME_NOERROR,
                                                        (record_format("Progress : %1%") % boost::str(boost::format("%1%") % percentage).append("%")));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (!result){
                        portal->reset_flags();
                        portal->delete_virtual_machine(vm->key);
                        vm = nullptr;
                    }
                    else{
                        vmware_virtual_machine::ptr updated_vm = portal->get_virtual_machine(vm->uuid);
                        if (updated_vm){
                            vm = updated_vm;
                        }
                        record(saasame::transport::job_state::type::job_state_none,
                            saasame::transport::error_codes::SAASAME_NOERROR,
                            (record_format("Progress : %1%") % boost::str(boost::format("%1%") % 100).append("%")));
                        record(saasame::transport::job_state::type::job_state_none,
                            saasame::transport::error_codes::SAASAME_NOERROR,
                            (record_format("Virtual machine '%1%' Created.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
                    }
                }
            }
        }
        if (!vm){
            record(saasame::transport::job_state::type::job_state_none,
                saasame::transport::error_codes::SAASAME_E_FAIL,
                (record_format("Unable to create virtual machine from snapshot.")));
        }
    }
    return vm;
}

void launcher_job::clone_progess(const std::wstring& message, const int& percentage){
    record(saasame::transport::job_state::type::job_state_none,
        saasame::transport::error_codes::SAASAME_NOERROR,
        (record_format("Progress : %1%") % boost::str(boost::format("%1%") % percentage).append("%")));
}

std::map<std::wstring, std::wstring> launcher_job::initial_vmware_vadp_disks(
    vmware_portal_::ptr&                            portal,
    vmware_virtual_machine::ptr&                    vm,
    vmware_vixdisk_connection_ptr&                  conn,
    device_mount_repository&      				    repository){
    std::map<std::wstring, std::wstring>		   vmdk_disks_serial_number_map;
    if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
        _launcher_job_create_detail.target_type = conversion_type::VMWARE;
        _launcher_job_create_detail.gpt_to_mbr = false;
        bool skip_system_injection = _launcher_job_create_detail.skip_system_injection &&
            !_launcher_job_create_detail.vmware.guest_id.empty() &&
            _launcher_job_create_detail.vmware.network_adapters.size() &&
            _launcher_job_create_detail.vmware.scsi_adapters.size();
        std::wstring device_path = get_vmp_device_path();
        if (device_path.empty()){
            LOG(LOG_LEVEL_ERROR, L"SaaSaMe StorPort Virtual Adapter device is not ready.");
        }
        else{
            DWORD							 bytesReturned;
            COMMAND_IN						 command;
            BOOL							 devStatus;
            repository.device_path = device_path;
            macho::windows::auto_handle handle = CreateFile(device_path.c_str(),  // Name of the NT "device" to open 
                GENERIC_READ | GENERIC_WRITE,  // Access rights requested
                0,                           // Share access - NONE
                0,                           // Security attributes - not used!
                OPEN_EXISTING,               // Device must exist to open it.
                FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
                0);                          // extended attributes - not used!
            if (handle.is_valid()){
                command.IoControlCode = IOCTL_VMPORT_SCSIPORT;
                if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP,
                    &command, sizeof(command), &command, sizeof(command), &bytesReturned, NULL))
                {
                    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.host));
                    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.username),
                        macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.password));
                    if (portal){
                        vmware_host::ptr host = portal->get_host(macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.esx));
                        if (host){
                            if (std::wstring::npos != host->version.find(L"2.5")){
                                _launcher_job_create_detail.gpt_to_mbr = true;
                            }
                            else if (std::wstring::npos != host->version.find(L"4.0")){
                                _launcher_job_create_detail.gpt_to_mbr = true;
                            }
                            else if (std::wstring::npos != host->version.find(L"4.1")){
                                _launcher_job_create_detail.gpt_to_mbr = true;
                            }
                        }
                        mwdc::ironman::hypervisor::hv_connection_type connection_type = portal->get_connection_type();
                        vmware_virtual_machine::ptr src_vm = portal->get_virtual_machine(macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.virtual_machine_id));
                        if (!src_vm){
                            record(saasame::transport::job_state::type::job_state_none,
                                saasame::transport::error_codes::SAASAME_E_FAIL,
                                (record_format("Cannot find the source virtual machine '%1%'.") % _launcher_job_create_detail.vmware.virtual_machine_id));
                        }
                        else{
                            if (_launcher_job_create_detail.mode == recovery_type::type::MIGRATION_RECOVERY){
                                _virtual_machine_id = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.virtual_machine_id);
                                vm = src_vm;
                            }
                            else if (!_launcher_job_create_detail.vmware.virtual_machine_snapshot.empty()){
                                vm = clone_virtual_machine(portal, _portal_ex, src_vm);
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR, L"Virtual machine snapshot must be specified.");
                            }

                            if (vm){
                                _virtual_machine_id = vm->key;
                                _portal_ex.set_portal(portal, _virtual_machine_id);
                                conn = _portal_ex.get_vixdisk_connection(
                                    macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.host),
                                    macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.username),
                                    macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.password),
                                    vm->key, L"", false);

                                GUID guid = macho::guid_(vm->uuid);
                                
                                foreach(vmware_disk_info::map::value_type& _d, vm->disks_map){
                                    bool matched = false;
                                    std::wstring uuid;
                                    if ((mwdc::ironman::hypervisor::hv_connection_type::HV_CONNECTION_TYPE_VCENTER == connection_type)&&
                                        (recovery_type::type::TEST_RECOVERY == _launcher_job_create_detail.mode)){
                                        foreach(vmware_disk_info::map::value_type& _src_disk, src_vm->disks_map){
                                            if (_src_disk.second->key == _d.second->key){
                                                foreach(string_map::value_type &d, _launcher_job_create_detail.disks_lun_mapping){
                                                    if (macho::guid_(_src_disk.second->uuid) == macho::guid_(d.first.substr(0, 36))){
                                                        matched = true;
                                                        uuid = _src_disk.second->uuid;
                                                        foreach(scsi_adapter_disks_map::value_type &s, _launcher_job_create_detail.vmware.scsi_adapters){
                                                            std::vector<std::string>::iterator __d = s.second.begin();
                                                            while (__d != s.second.end()){
                                                                if (macho::guid_(_src_disk.second->uuid) == macho::guid_(*__d)){
                                                                    __d = s.second.erase(__d);
                                                                    s.second.push_back(macho::guid_(_d.second->uuid));
                                                                    break;
                                                                }
                                                                else{
                                                                    __d++;
                                                                }
                                                            }
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            if (matched)
                                                break;
                                        }
                                    }
                                    else{
                                        foreach(string_map::value_type &d, _launcher_job_create_detail.disks_lun_mapping){
                                            if (macho::guid_(_d.second->uuid) == macho::guid_(d.first.substr(0, 36))){
                                                matched = true;
                                                uuid = _d.second->uuid;
                                                break;
                                            }
                                        }
                                    }
                                    if (matched && !skip_system_injection){
                                        WCHAR disk_id[40];
                                        memset(disk_id, 0, sizeof(disk_id));
                                        _snwprintf_s(disk_id, sizeof(disk_id),
                                            L"%04x%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", _d.second->key,
                                            guid.Data1, guid.Data2, guid.Data3,
                                            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                                            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
                                        LOG(LOG_LEVEL_INFO, L"Disk Id : %s", disk_id);
                                        universal_disk_rw::ptr out = _portal_ex.open_vmdk_for_rw(conn, _d.second->name, false);
                                        if (out){
                                            device_mount_task::ptr task(new device_mount_task(out, _d.second->size / (1024 * 1024), std::wstring(disk_id), repository));
                                            task->register_task_is_canceled_function(boost::bind(&device_mount_repository::is_canceled, &repository));
                                            repository.tasks.push_back(task);
                                            vmdk_disks_serial_number_map[macho::stringutils::toupper(uuid)] = disk_id;
                                        }
                                        else{
                                            LOG(LOG_LEVEL_ERROR, L"Failed to open virtual disk (%s) for rw.", _d.second->name.c_str());
                                            break;
                                        }
                                    }
                                }
                                if (repository.tasks.size() == _launcher_job_create_detail.disks_lun_mapping.size()){
                                    foreach(device_mount_task::ptr task, repository.tasks)
                                        repository.thread_pool.create_thread(boost::bind(&device_mount_task::mount, task.get()));
                                }
                                else if (skip_system_injection){
                                    repository.terminated = true;
                                    SetEvent(repository.quit_event);
                                    repository.thread_pool.join_all();
                                    repository.tasks.clear();
                                    conn = NULL;
                                    _state = saasame::transport::job_state::type::job_state_finished;
                                    save_status();
                                    _update(false);
                                    launch_vmware_virtual_machine(portal, vm, _launcher_job_create_detail.vmware.firmware == saasame::transport::hv_vm_firmware::HV_VM_FIRMWARE_EFI);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return vmdk_disks_serial_number_map;
}

void launcher_job::launch_vmware_virtual_machine(
    vmware_portal_::ptr&                            portal,
    vmware_virtual_machine::ptr&                    vm,
    bool is_need_uefi_boot){
    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.host));
    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.username),
        macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.connection.password));
    if (portal){
        if (is_canceled()){
            if (_launcher_job_create_detail.mode != recovery_type::type::MIGRATION_RECOVERY){
                portal->reset_flags();
                portal->delete_virtual_machine(vm->key);
            }
        }
        else{
            vmware_virtual_machine_config_spec config_spec(*vm);
            if (_launcher_job_create_detail.mode == recovery_type::type::MIGRATION_RECOVERY || 
                _launcher_job_create_detail.mode == recovery_type::type::DISASTER_RECOVERY){
                std::wstring virtual_machine_name = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.vm_name);
                if (!virtual_machine_name.empty() && 0 != config_spec.name.find(virtual_machine_name)){
                    std::wstring vm_name = virtual_machine_name;
                    key_map vm_name_map;
                    vmware_host::vtr hosts = portal->get_hosts();
                    foreach(vmware_host::ptr& h, hosts){
                        foreach(key_map::value_type v, h->vms){
                            vm_name_map[v.second] = v.first;
                        }
                    }
                    int index = 0;
                    while (vm_name_map.count(virtual_machine_name)){
                        index++;
                        virtual_machine_name = boost::str(boost::wformat(L"%s(%d)") % vm_name % index);
                    }
                    config_spec.name = virtual_machine_name;
                }
            }
            config_spec.firmware = is_need_uefi_boot ? HV_VM_FIRMWARE_EFI : HV_VM_FIRMWARE_BIOS;
            config_spec.memory_mb = _launcher_job_create_detail.vmware.number_of_memory_in_mb;
            config_spec.number_of_cpu = _launcher_job_create_detail.vmware.number_of_cpus;
            config_spec.has_cdrom = _launcher_job_create_detail.vmware.install_vm_tools;
            config_spec.has_serial_port = _launcher_job_create_detail.vmware.guest_id.empty() || 0 != _launcher_job_create_detail.vmware.guest_id.find("win");
            config_spec.nics.clear();
            if (!_launcher_job_create_detail.vmware.guest_id.empty() && 
                _launcher_job_create_detail.vmware.network_adapters.size() && 
                _launcher_job_create_detail.vmware.scsi_adapters.size()){
                config_spec.guest_id = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.guest_id);
                config_spec.firmware = (mwdc::ironman::hypervisor::hv_vm_firmware) _launcher_job_create_detail.vmware.firmware;
                size_t index = 0;
                foreach (std::string network_adapter, _launcher_job_create_detail.vmware.network_adapters){
                    vmware_virtual_network_adapter::ptr adapter(new vmware_virtual_network_adapter());
                    if (_launcher_job_create_detail.vmware.network_connections.size() > index){
                        adapter->network = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.network_connections[index]);
                    }
                    else if (_launcher_job_create_detail.vmware.network_connections.size() > 0){
                        adapter->network = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.network_connections[_launcher_job_create_detail.vmware.network_connections.size()-1]);
                    }
                    if (_launcher_job_create_detail.vmware.mac_addresses.size() > index){
                        std::regex	mac_regex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$", std::regex_constants::icase);
                        std::smatch m;
                        if (17 == _launcher_job_create_detail.vmware.mac_addresses[index].length() &&
                            std::regex_match(macho::stringutils::tolower(_launcher_job_create_detail.vmware.mac_addresses[index]), m, mac_regex)){
                            adapter->mac_address = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.mac_addresses[index]);
                        }
                    }
                    adapter->is_allow_guest_control = true;
                    adapter->is_connected = true;
                    adapter->is_start_connected = true;
                    adapter->type = macho::stringutils::convert_utf8_to_unicode(network_adapter);
                    config_spec.nics.push_back(adapter);
                    index++;
                }
                if (_launcher_job_create_detail.vmware.scsi_adapters.size() == 1){
                    foreach(vmware_disk_info::ptr disk, config_spec.disks){
                        disk->controller->type = (mwdc::ironman::hypervisor::hv_controller_type)_launcher_job_create_detail.vmware.scsi_adapters.begin()->first;
                    }
                }
                else{
                    foreach(scsi_adapter_disks_map::value_type &s, _launcher_job_create_detail.vmware.scsi_adapters){
                        foreach(std::string d, s.second){
                            foreach(vmware_disk_info::ptr disk, config_spec.disks){
                                if (macho::guid_(disk->uuid) == macho::guid_(d)){
                                    disk->controller->type = (mwdc::ironman::hypervisor::hv_controller_type)s.first;
                                }
                            }
                        }
                    }
                }
            }
            else{
                hv_guest_os::type                guest_os_type;
                hv_guest_os::architecture        guest_os_architecture;
                guest_os_architecture = hv_guest_os::architecture::X86;
                if (macho::stringutils::tolower(_architecture) == "amd64")
                    guest_os_architecture = hv_guest_os::architecture::X64;
                std::string platform = macho::stringutils::tolower(_platform);
                foreach(std::string network_connection, _launcher_job_create_detail.vmware.network_connections){
                    vmware_virtual_network_adapter::ptr adapter(new vmware_virtual_network_adapter());
                    adapter->network = macho::stringutils::convert_utf8_to_unicode(network_connection);
                    adapter->is_allow_guest_control = true;
                    adapter->is_connected = true;
                    adapter->is_start_connected = true;
                    adapter->type = L"E1000";
                    config_spec.nics.push_back(adapter);
                }
                if (std::string::npos != platform.find("windows")){
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::HV_CTRL_LSI_LOGIC_SAS;
                    if (std::string::npos != platform.find("2016")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2016;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("2012 r2")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2012R2;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("2012")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2012;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("2008 r2")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2008R2;
                    }
                    else if (std::string::npos != platform.find("2008")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2008;
                    }
                    else if (std::string::npos != platform.find("2003")){
                        foreach(vmware_disk_info::ptr disk, config_spec.disks)
                            disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                        if (std::string::npos != platform.find("web"))
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_WEB;
                        else if (std::string::npos != platform.find("business"))
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_BUSINESS;
                        else if (std::string::npos != platform.find("datacenter"))
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_DATACENTER;
                        else if (std::string::npos != platform.find("standard"))
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_STANDARD;
                        else if (std::string::npos != platform.find("enterprise"))
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_ENTERPRISE;
                        else
                            guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_2003_STANDARD;
                    }
                    else if (std::string::npos != platform.find("10")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_10;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("8.1")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_8_1;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("8")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_8;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("7")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_7;
                    }
                    else if (std::string::npos != platform.find("vista")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_WINDOWS_VISTA;
                    }
                }
                else if (std::string::npos != platform.find("redhat")){
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_PARA_VIRT_SCSI;
                    if (std::string::npos != platform.find("7.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_REDHAT_7;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                    else if (std::string::npos != platform.find("6.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_REDHAT_6;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"Vmxnet3";
                    }
                    else if (std::string::npos != platform.find("5.")) {
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_REDHAT_5;
                        foreach(vmware_disk_info::ptr disk, config_spec.disks)
                            disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                    }
                    else if (std::string::npos != platform.find("4.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_REDHAT_4;
                        foreach(vmware_disk_info::ptr disk, config_spec.disks)
                            disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                    }
                    else{
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_REDHAT_7;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"E1000E";
                    }
                }
                else if (std::string::npos != platform.find("ubuntu")){
                    guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_UBUNTU;
                    foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                        adapter->type = L"Vmxnet3";
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                }
                else if (std::string::npos != platform.find("suse")){
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                    if (std::string::npos != platform.find("12."))
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_SUSE_12;
                    else if (std::string::npos != platform.find("11."))
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_SUSE_11;
                    else if (std::string::npos != platform.find("10."))
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_SUSE_10;
                    else
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_SUSE_12;
                }
                else if (std::string::npos != platform.find("centos")){
                    guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_CENTOS;
                    foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                        adapter->type = L"Vmxnet3";
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                }
                else if (std::string::npos != platform.find("debian")){
                    foreach(vmware_disk_info::ptr disk, config_spec.disks)
                        disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_PARA_VIRT_SCSI;
                    if (std::string::npos != platform.find("8.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_DEBIAN_8;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"Vmxnet3";
                    }
                    else if (std::string::npos != platform.find("7.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_DEBIAN_7;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"Vmxnet3";
                    }
                    else if (std::string::npos != platform.find("6.")){
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_DEBIAN_6;
                        foreach(vmware_disk_info::ptr disk, config_spec.disks)
                            disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC;
                    }
                    else{
                        guest_os_type = mwdc::ironman::hypervisor::hv_guest_os::type::HV_OS_DEBIAN_8;
                        foreach(vmware_virtual_network_adapter::ptr adapter, config_spec.nics)
                            adapter->type = L"Vmxnet3";
                    }
                }
                config_spec.guest_id = vmware_virtual_machine_config_spec::guest_os_id(guest_os_type, guest_os_architecture);
            }
            mwdc::ironman::hypervisor::hv_connection_type connection_type = portal->get_connection_type();
            vmware_virtual_machine::ptr updated_vm;		
            if (mwdc::ironman::hypervisor::hv_connection_type::HV_CONNECTION_TYPE_VCENTER == connection_type){
                vmware_virtual_machine_config_spec _config_spec(*vm);
                _config_spec.guest_id = macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.vmware.guest_id);
                updated_vm = portal->modify_virtual_machine(vm->uuid, _config_spec);
                if (updated_vm){
                    if (portal->unregister_virtual_machine(updated_vm->uuid)){
                        vmware_add_existing_virtual_machine_spec spec;
                        spec.host_key = updated_vm->host_key;
                        spec.config_file_path = updated_vm->config_path_file;
                        spec.resource_pool_path = updated_vm->resource_pool_path;
                        spec.folder_path = updated_vm->folder_path;
                        updated_vm = portal->add_existing_virtual_machine(spec);
                        if (updated_vm)
                            updated_vm = portal->modify_virtual_machine(updated_vm->uuid, config_spec);
                    }
                }
            }
            else{
                updated_vm = portal->modify_virtual_machine(vm->uuid, config_spec);
            }

            if (updated_vm){
                vm = updated_vm;
                if (_launcher_job_create_detail.vmware.install_vm_tools && !_launcher_job_create_detail.skip_system_injection)
                    portal->mount_vm_tools(vm->uuid);
                record(saasame::transport::job_state::type::job_state_finished,
                    saasame::transport::error_codes::SAASAME_NOERROR,
                    (record_format("Successfully update virtual machine '%1%' settings.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
                int try_count = 3;
                bool power_on_vm = false;
                do{
                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                    power_on_vm = portal->power_on_virtual_machine(vm->uuid);
                    try_count--;
                } 
                while (!power_on_vm && try_count >= 0);

                if (power_on_vm){
                    record(saasame::transport::job_state::type::job_state_finished,
                        saasame::transport::error_codes::SAASAME_NOERROR,
                        (record_format("Power on virtual machine '%1%'.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
                }
                else{
                    _state |= mgmt_job_state::job_state_error;
                    record(saasame::transport::job_state::type::job_state_finished,
                        saasame::transport::error_codes::SAASAME_E_FAIL,
                        (record_format("Failed to power on virtual machine '%1%'.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
                }
            }
            else{
                _state |= mgmt_job_state::job_state_error;
                record(saasame::transport::job_state::type::job_state_finished,
                    saasame::transport::error_codes::SAASAME_E_FAIL,
                    (record_format("Failed to update virtual machine '%1%' settings.") % macho::stringutils::convert_unicode_to_utf8(vm->name)));
            }
        }
    }
}
#endif

#if defined(_VMWARE_VADP) || defined(_AZURE_BLOB)
std::wstring launcher_job::get_vmp_device_path(){
    std::wstring  device_path;
    std::wstring  driver_package_inf_path = boost::filesystem::path(boost::filesystem::path(macho::windows::environment::get_execution_full_path()).parent_path() / L"DRIVER" / L"vmp.inf").wstring();
    int count = 3;
    bool installed = false;
    do{
        macho::windows::device_manager  dev_mgmt;
        hardware_device::vtr class_devices;
        class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
        foreach(hardware_device::ptr &d, class_devices){
            if (d->service == L"vmp"){
                device_path = dev_mgmt.get_device_path(d->device_instance_id, (LPGUID)&GUID_DEVINTERFACE_STORAGEPORT);
                LOG(LOG_LEVEL_INFO, L"Device Path : %s", device_path.c_str());
                break;
            }
        }
        LPTSTR _driver_package_inf_path = (LPTSTR)driver_package_inf_path.c_str();  // An INF file for PnP driver package
        if (!device_path.empty()){
            DWORD Flags = DRIVER_PACKAGE_LEGACY_MODE | DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_FORCE;	
            INSTALLERINFO *pAppInfo = NULL;      // No application association
            BOOL NeedReboot = FALSE;
            if (boost::filesystem::exists(_driver_package_inf_path)){
                DWORD ReturnCode = DriverPackageInstall(_driver_package_inf_path, Flags, pAppInfo, &NeedReboot);
                if (ERROR_SUCCESS != ReturnCode){
                    std::wstring str = get_error_message(ReturnCode);
                    LOG(LOG_LEVEL_WARNING, _T("Install Driver (%s) - Result (0x%08X - %s)\n"), _driver_package_inf_path, ReturnCode, str.c_str());
                }
            }
            break;
        }
        else if (!installed){
            DWORD ReturnCode = DriverPackagePreinstall(_driver_package_inf_path, DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_LEGACY_MODE);
            if (ERROR_SUCCESS != ReturnCode){
                if (ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED == ReturnCode){
                    certificate_store::ptr trusted_publisher = certificate_store::open_store(CERT_SYSTEM_STORE_LOCAL_MACHINE, L"TrustedPublisher");
                    if (trusted_publisher){
                        setup_inf_file inf;
                        inf.load(driver_package_inf_path);
                        boost::filesystem::path inf_file = driver_package_inf_path;
                        boost::filesystem::path catalog_file = inf_file.parent_path() / inf.catalog_file();
                        authenticode_signed_info::ptr signed_info = certificate_store::get_authenticode_signed_info(catalog_file);
                        if (signed_info && signed_info->signer_certificate){
                            std::wstring cert_name = signed_info->signer_certificate->friendly_name();
                            if (trusted_publisher->add_certificate(*signed_info->signer_certificate)){
                                LOG(LOG_LEVEL_RECORD, _T("Add certificate(%s) into Trusted Publisher."), cert_name.c_str());
                                continue;
                            }
                        }
                    }
                }
                std::wstring str = get_error_message(ReturnCode);
                LOG(LOG_LEVEL_ERROR, _T("PreInstall Driver (%s) - Result (0x%08X - %s)\n"), driver_package_inf_path, ReturnCode, str.c_str());
            }
            installed = dev_mgmt.install(driver_package_inf_path, L"root\\vmp");
        }
        count--;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    } while (count > 0);
    return device_path;
}

#endif

#ifdef _AZURE_BLOB
std::map<std::wstring, std::wstring> launcher_job::initial_page_blob_disks(device_mount_repository& repository){
    std::map<std::wstring, std::wstring>		   disks_serial_number_map;
    if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB){
        _launcher_job_create_detail.target_type = conversion_type::HYPERV;
        std::wstring device_path = get_vmp_device_path();
        if (device_path.empty()){
            LOG(LOG_LEVEL_ERROR, L"SaaSaMe StorPort Virtual Adapter device is not ready.");
        }
        else{
            DWORD							 bytesReturned;
            COMMAND_IN						 command;
            BOOL							 devStatus;
            repository.device_path = device_path;
            repository.clone_rw = true;
            repository.is_cache_enabled = true;
            macho::windows::auto_handle handle = CreateFile(device_path.c_str(),  // Name of the NT "device" to open 
                GENERIC_READ | GENERIC_WRITE,  // Access rights requested
                0,                           // Share access - NONE
                0,                           // Security attributes - not used!
                OPEN_EXISTING,               // Device must exist to open it.
                FILE_FLAG_OVERLAPPED,        // Open for overlapped I/O
                0);                          // extended attributes - not used!
            if (handle.is_valid()){
                command.IoControlCode = IOCTL_VMPORT_SCSIPORT;
                if (devStatus = DeviceIoControl(handle, IOCTL_MINIPORT_PROCESS_SERVICE_IRP,
                    &command, sizeof(command), &command, sizeof(command), &bytesReturned, NULL))
                {
                    int count = 0;
                    GUID guid = macho::guid_(_id);
                    foreach(string_map::value_type &d, _launcher_job_create_detail.disks_lun_mapping){
                        std::wstring uuid = macho::guid_(d.first.substr(0, 36));
                        WCHAR disk_id[40];
                        memset(disk_id, 0, sizeof(disk_id));
                        _snwprintf_s(disk_id, sizeof(disk_id),
                            L"%04x%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", count++,
                            guid.Data1, guid.Data2, guid.Data3,
                            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
                        LOG(LOG_LEVEL_INFO, L"Disk Id : %s", disk_id);
                        uint64_t size = azure_page_blob_op::get_vhd_page_blob_size(_launcher_job_create_detail.azure_storage_connection_string, _launcher_job_create_detail.export_path, _launcher_job_create_detail.disks_lun_mapping[d.first]);
                        universal_disk_rw::ptr out = azure_page_blob_op::open_vhd_page_blob_for_rw(_launcher_job_create_detail.azure_storage_connection_string, _launcher_job_create_detail.export_path, _launcher_job_create_detail.disks_lun_mapping[d.first]);
                        if (size && out){
                            device_mount_task::ptr task(new device_mount_task(out, size / (1024 * 1024), std::wstring(disk_id), repository));
                            task->register_task_is_canceled_function(boost::bind(&device_mount_repository::is_canceled, &repository));
                            repository.tasks.push_back(task);
                            disks_serial_number_map[macho::stringutils::toupper(uuid)] = disk_id;
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"Failed to open azure page blob (%s) for rw.", _launcher_job_create_detail.export_path, stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.disks_lun_mapping[d.first]).c_str());
                            break;
                        }
                        if (repository.tasks.size() == _launcher_job_create_detail.disks_lun_mapping.size()){
                            foreach(device_mount_task::ptr task, repository.tasks)
                                repository.thread_pool.create_thread(boost::bind(&device_mount_task::mount, task.get()));
                        }
                    }
                }
            }
        }
    }
    return disks_serial_number_map;
}
#endif

void launcher_job::execute(){
    VALIDATE;
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || _is_removing){
        return;
    }
    _terminated = false;
    _thread = boost::thread(&launcher_job::_update, this, true);
    _state &= ~mgmt_job_state::job_state_error;
    _is_initialized = get_launcher_job_create_detail(_launcher_job_create_detail);
    save_config();
    if (_state == mgmt_job_state::job_state_none){
        LOG(LOG_LEVEL_RECORD, L"Initializing a Launcher job (%s).", _id.c_str());
        record(saasame::transport::job_state::type::job_state_none,
            saasame::transport::error_codes::SAASAME_NOERROR,
            record_format("Initializing a Launcher job."));
    }
    else{
        LOG(LOG_LEVEL_RECORD, L"Re-initializing a Launcher job (%s).", _id.c_str());
    }
    if (!_is_initialized){
        record(saasame::transport::job_state::job_state_none, saasame::transport::error_codes::SAASAME_E_FAIL, record_format("Cannot get the Launcher job detail from management server."));
        _state |= mgmt_job_state::job_state_error;
        save_status();
    }
    else{
        int retry_count = 1;

#if defined(_VMWARE_VADP) || defined(_AZURE_BLOB)
        vmware_portal_::ptr                            portal;
        vmware_virtual_machine::ptr                    vm;
        vmware_vixdisk_connection_ptr                  conn;
        device_mount_repository      				   repository;
        std::map<std::wstring, std::wstring>		   vmdk_disks_serial_number_map;
        if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
            vmware_portal_ex::set_log(get_log_file(), get_log_level());
            vmdk_disks_serial_number_map = initial_vmware_vadp_disks(portal, vm, conn, repository);
            if (!vmdk_disks_serial_number_map.empty()) {
                boost::this_thread::sleep(boost::posix_time::seconds(10));
                retry_count = 5;
            }
            else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP &&
                _launcher_job_create_detail.skip_system_injection &&
                !_launcher_job_create_detail.vmware.guest_id.empty() &&
                _launcher_job_create_detail.vmware.network_adapters.size() &&
                _launcher_job_create_detail.vmware.scsi_adapters.size()){
                save_status();
                _terminated = true;
                _update(false);
                if (_thread.joinable())
                    _thread.join();
                return;
            }
            else{
                _state |= mgmt_job_state::job_state_error;
                save_status();
                _terminated = true;
                _update(false);
                if (_thread.joinable())
                    _thread.join();
                return;
            }
        }
        else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB){
            vmdk_disks_serial_number_map = initial_page_blob_disks(repository);
            if (!vmdk_disks_serial_number_map.empty()) {
                boost::this_thread::sleep(boost::posix_time::seconds(10));
                retry_count = 5;
            }
            else{
                _state |= mgmt_job_state::job_state_error;
                save_status();
                _terminated = true;
                _update(false);
                if (_thread.joinable())
                    _thread.join();
                return;
            }
        }
#endif
        com_init com;
        storage::ptr stg = storage::get();
        storage::disk::vtr disks = stg->get_disks();
        disk_map _disk_map;
        virtual_disk_map _virtual_disk_map;
        while (_disk_map.size() != _launcher_job_create_detail.disks_lun_mapping.size() &&
            retry_count >= 0){
            stg = retry_count > 0 ? storage::get() : storage::local();
            storage::disk::vtr disks = stg->get_disks();
            foreach(string_map::value_type &d, _launcher_job_create_detail.disks_lun_mapping){
                if (_is_interrupted || _is_canceled)
                    break;
                if (_disk_map.count(d.first))
                    continue;
                if (_launcher_job_create_detail.detect_type == disk_detect_type::type::SCSI_ADDRESS){
                    DWORD scsi_port = 0, scsi_bus = 0, scsi_target_id = 0, scsi_lun = 0;
                    if (4 == sscanf_s(d.second.c_str(), "%d:%d:%d:%d", &scsi_port, &scsi_bus, &scsi_target_id, &scsi_lun)){
                        foreach(storage::disk::ptr p, disks){
                            if (p->scsi_port() == scsi_port &&
                                p->scsi_bus() == scsi_bus &&
                                p->scsi_target_id() == scsi_target_id &&
                                p->scsi_logical_unit() == scsi_lun){
                                _disk_map[d.first] = p->number();
                                p->offline();
                                p->clear_read_only_flag();
                                break;
                            }
                        }
                    }
                }
                else if (_launcher_job_create_detail.detect_type == disk_detect_type::type::SERIAL_NUMBER){
                    std::wstring serial_number = stringutils::convert_ansi_to_unicode(d.second);
                    foreach(storage::disk::ptr p, disks){
                        if (p->serial_number().length() > 0 &&
                            boost::starts_with(serial_number, p->serial_number())){
                            _disk_map[d.first] = p->number();
                            p->offline();
                            p->clear_read_only_flag();
                            break;
                        }
                    }
                }
                else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB){
#ifdef _AZURE_BLOB
                    std::wstring disk_id = macho::stringutils::toupper((std::wstring)macho::guid_(d.first.substr(0, 36)));
                    if (vmdk_disks_serial_number_map.count(disk_id)){
                        std::wstring serial_number = vmdk_disks_serial_number_map[disk_id];
                        foreach(storage::disk::ptr p, disks){
                            if (p->serial_number().length() > 0 &&
                                boost::starts_with(serial_number, p->serial_number())){
                                _disk_map[d.first] = p->number();
                                p->offline();
                                p->clear_read_only_flag();
                                break;
                            }
                        }
                    }
#endif
                }
                else if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
#ifdef _VMWARE_VADP
                    if (vm){
                        std::wstring disk_id = macho::stringutils::toupper((std::wstring)macho::guid_(d.first.substr(0, 36)));
                        if (vmdk_disks_serial_number_map.count(disk_id)){
                            std::wstring serial_number = vmdk_disks_serial_number_map[disk_id];
                            foreach(storage::disk::ptr p, disks){
                                if (p->serial_number().length() > 0 &&
                                    boost::starts_with(serial_number, p->serial_number())){
                                    _disk_map[d.first] = p->number();
                                    p->offline();
                                    p->clear_read_only_flag();
                                    break;
                                }
                            }
                        }
                    }
#endif
                }
                else if (_launcher_job_create_detail.detect_type == disk_detect_type::type::UNIQUE_ID){
                    std::wstring unique_id = stringutils::convert_ansi_to_unicode(d.second);
                    foreach(storage::disk::ptr p, disks){
                        if (p->unique_id().length() > 0 &&
                            (unique_id == p->unique_id())){
                            _disk_map[d.first] = p->number();
                            p->offline();
                            p->clear_read_only_flag();
                            break;
                        }
                    }
                }
                else if (_launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE){
                    boost::filesystem::path vhd_file = boost::filesystem::path(macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.export_path)) / macho::stringutils::convert_utf8_to_unicode(_launcher_job_create_detail.host_name) / macho::stringutils::convert_utf8_to_unicode(d.second);
                    switch (_launcher_job_create_detail.export_disk_type){
                    case virtual_disk_type::type::VHDX:
                        vhd_file = vhd_file.wstring() + (L".vhdx");
                        break;
                    case virtual_disk_type::type::VHD:
                    default:
                        vhd_file = vhd_file.wstring() + (L".vhd");
                        break;
                    }
                    if (boost::filesystem::exists(vhd_file)){
                        _virtual_disk_map[d.first] = vhd_file;
                        boost::filesystem::path temp_vhd_file = vhd_file.parent_path() / boost::str(boost::wformat(L"%s_diff%s") % vhd_file.filename().stem().wstring() % vhd_file.extension().wstring());
                        LOG(LOG_LEVEL_RECORD, L"Convert the virtual disk : %s", vhd_file.wstring().c_str());
                        vhd_disk_info disk_info;
                        _vhd_export_lock = macho::windows::file_lock_ex::ptr(new macho::windows::file_lock_ex(vhd_file.parent_path() / "export", "r"));
                        _vhd_export_lock->lock();
                        if (!boost::filesystem::exists(temp_vhd_file)){
                            if (!(ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_information(vhd_file.wstring(), disk_info) &&
                                ERROR_SUCCESS == win_vhdx_mgmt::create_vhdx_file(temp_vhd_file.wstring(), CREATE_VIRTUAL_DISK_FLAG_NONE, disk_info.virtual_size, disk_info.block_size, disk_info.logical_sector_size, disk_info.physical_sector_size, vhd_file.wstring()))){
                                LOG(LOG_LEVEL_ERROR, L"Failed to create the temp virtual disk file (%s) for conversion.", temp_vhd_file.wstring().c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Failed to create the temp virtual disk file %1% for conversion.") % macho::stringutils::convert_unicode_to_utf8(temp_vhd_file.wstring())));
                            }
                        }
                        if (boost::filesystem::exists(temp_vhd_file)){
                            macho::windows::mutex mx(L"AttachVHD");
                            macho::windows::auto_lock lock(mx);
                            DWORD disk_number = -1;
                            if ((ERROR_SUCCESS == win_vhdx_mgmt::get_attached_physical_disk_number(temp_vhd_file.wstring(), disk_number)) ||
                                (ERROR_SUCCESS == win_vhdx_mgmt::attach_virtual_disk(temp_vhd_file.wstring()) && (ERROR_SUCCESS == win_vhdx_mgmt::get_attached_physical_disk_number(temp_vhd_file.wstring(), disk_number)))){
                                storage::disk::ptr disk = stg->get_disk(disk_number);
                                if (disk){
                                    disk->offline();
                                    disk->clear_read_only_flag();
                                }
                                _disk_map[d.first] = disk_number;
                                _virtual_disk_map[d.first] = temp_vhd_file;
                            }
                            else{
                                boost::filesystem::remove(temp_vhd_file);
                                LOG(LOG_LEVEL_ERROR, L"Failed to attach the temp virtual disk file (%s) for conversion.", temp_vhd_file.wstring().c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Failed to attach the temp virtual disk file %1% for conversion.") % macho::stringutils::convert_unicode_to_utf8(temp_vhd_file.wstring())));
                            }
                        }
                    }
                    else {
                        LOG(LOG_LEVEL_ERROR, _T("Cannot find virtual disk (%s)."), vhd_file.wstring().c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Cannot find virtual disk %1%.") % macho::stringutils::convert_unicode_to_utf8(vhd_file.wstring())));
                    }
                }
                else if (_launcher_job_create_detail.detect_type == disk_detect_type::type::CUSTOMIZED_ID){
                    foreach(storage::disk::ptr p, disks){
                        general_io_rw::ptr rd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % p->number()));
                        if (rd){
                            uint32_t r = 0;
                            std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                            memset(data.get(), 0, 512);
                            if (rd->sector_read((p->size() >> 9) - 40, 1, data.get(), r)){
                                GUID guid;
                                memcpy(&guid, &(data.get()[512 - 16]), 16);
                                try{
                                    if (macho::guid_(guid) == macho::guid_(d.second)){
                                        _disk_map[d.first] = p->number();
                                        p->offline();
                                        p->clear_read_only_flag();
                                        break;
                                    }
                                }
                                catch (...){
                                }
                            }
                        }
                    }
                }

                if (!_disk_map.count(d.first) && 0 == retry_count){
                    record(saasame::transport::job_state::job_state_none, saasame::transport::error_codes::SAASAME_E_INVALID_ARG, (record_format("Cannot find any disk for recovery.")));
                    _state |= mgmt_job_state::job_state_error;
                    save_status();
                    break;
                }
            }
            retry_count--;
#ifdef _VMWARE_VADP
            if (!vmdk_disks_serial_number_map.empty())
                boost::this_thread::sleep(boost::posix_time::seconds(10));
#endif
        }
        if (!(_state & mgmt_job_state::job_state_error)){
            if (_state == saasame::transport::job_state::type::job_state_uploading){
                disk_map boot_disk;
                if (_launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE)
                    boot_disk = _disk_map;
                else
                    boot_disk[_boot_disk] = _disk_map[_boot_disk];
                bool result = (boot_disk.size() > 0) ? upload_vhd_to_cloud(boot_disk) : false;
                if (result){
                    record(saasame::transport::job_state::type::job_state_upload_completed,
                        saasame::transport::error_codes::SAASAME_NOERROR,
                        (record_format("VHD upload completed.")));
                    save_status();
                    _update(false);
                    if (_vhd_export_lock)
                        _vhd_export_lock->unlock();
                    foreach(disk_map::value_type &d, _disk_map){
                        std::wstring physical_path;
                        if (_virtual_disk_map.count(d.first)){
                            while (ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_physical_path(_virtual_disk_map[d.first].wstring(), physical_path)){
                                if (ERROR_SUCCESS == win_vhdx_mgmt::detach_virtual_disk(_virtual_disk_map[d.first].wstring())){
                                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                                    break;
                                }
                            }
                            boost::filesystem::remove(_virtual_disk_map[d.first]);
                        }
                    }
                }
            }
            else if (_state == saasame::transport::job_state::type::job_state_upload_completed){
                bool result = is_launcher_job_image_ready();
                if (result){
                    modify_job_scheduler_interval();
                    if (boost::filesystem::exists(_pre_script_file)){
                        try{
                            boost::filesystem::path temp = macho::windows::environment::create_temp_folder();
                            {
                                archive::unzip::ptr unzip_ptr = archive::unzip::open(_pre_script_file);
                                if (unzip_ptr){
                                    unzip_ptr->decompress_archive(temp);
                                    boost::filesystem::path pre_script_run = temp / "run.cmd";
                                    if (boost::filesystem::exists(pre_script_run)){
                                        std::wstring ret;
                                        process::exec_console_application_with_timeout(pre_script_run.wstring(), ret, -1);
                                        LOG(LOG_LEVEL_RECORD, L"Pre script return (%s)", ret.c_str());
                                    }
                                }
                            }
                            boost::filesystem::remove_all(temp);
                        }
                        catch (const boost::filesystem::filesystem_error& ex){
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                        }
                        catch (const boost::exception &ex){
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                        }
                        catch (const std::exception& ex){
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                        }
                        catch (...){
                            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                        }
                    }
                    _state = saasame::transport::job_state::type::job_state_finished;
                    save_status();
                    _update(false);
                }
            }
            else{
                bool result = false;
                _state = saasame::transport::job_state::type::job_state_initialed;
                save_status();
                if (_launcher_job_create_detail.os_type == saasame::transport::hv_guest_os_type::HV_OS_LINUX){
                    macho::windows::mutex mx(L"LinuxLauncher");
                    bool lock = false;
                    while (!(lock = mx.trylock())){
                        boost::this_thread::sleep(boost::posix_time::seconds(5));
                        if (_is_interrupted || _is_canceled)
                            break;
                    }
                    if (lock){
                        if (!(_is_interrupted || _is_canceled))
                            result = linux_convert(_disk_map);
                        mx.unlock();
                        if (result){
                            if (_disk_map.size() > 1){
                                foreach(disk_map::value_type d, _disk_map){
                                    universal_disk_rw::ptr u = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d.second));
                                    if (u && clone_disk::is_bootable_disk(u)){
                                        _boot_disk = d.first;
                                        break;
                                    }
                                }
                            }
                            else{
                                _boot_disk = _disk_map.begin()->first;
                            }
                        }
                    }
                }
                else if (_launcher_job_create_detail.os_type == saasame::transport::hv_guest_os_type::HV_OS_WINDOWS){
                    result = windows_convert(stg, _disk_map);
                }

                if (result){
                    save_status();
                    if ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN)
                        || (_launcher_job_create_detail.options_type == extra_options_type::TENCENT)
                        ){
                        modify_job_scheduler_interval(1);
                        record(saasame::transport::job_state::type::job_state_uploading,
                            saasame::transport::error_codes::SAASAME_NOERROR,
                            (record_format("Disk(s) conversion completed.")));
                        disk_map boot_disk;
                        if (_launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE)
                            boot_disk = _disk_map;
                        else
                            boot_disk[_boot_disk] = _disk_map[_boot_disk];
                        result = (boot_disk.size() > 0) ? upload_vhd_to_cloud(boot_disk) : false;
                        if (result){
                            record(saasame::transport::job_state::type::job_state_upload_completed,
                                saasame::transport::error_codes::SAASAME_NOERROR,
                                (record_format("VHD upload completed.")));
                            save_status();
                            _update(false);
                            if (_vhd_export_lock)
                                _vhd_export_lock->unlock();
                            foreach(disk_map::value_type &d, _disk_map){
                                std::wstring physical_path;
                                if (_virtual_disk_map.count(d.first)){
                                    while (ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_physical_path(_virtual_disk_map[d.first].wstring(), physical_path)){
                                        if (ERROR_SUCCESS == win_vhdx_mgmt::detach_virtual_disk(_virtual_disk_map[d.first].wstring())){
                                            boost::this_thread::sleep(boost::posix_time::seconds(10));
                                            break;
                                        }
                                    }
                                    boost::filesystem::remove(_virtual_disk_map[d.first]);
                                }
                            }
                        }
                    }
                    else if (result){
                        foreach(disk_map::value_type &d, _disk_map){
                            std::wstring physical_path;
                            if (_virtual_disk_map.count(d.first)){
                                while (ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_physical_path(_virtual_disk_map[d.first].wstring(), physical_path)){
                                    if (ERROR_SUCCESS == win_vhdx_mgmt::detach_virtual_disk(_virtual_disk_map[d.first].wstring())){
                                        boost::this_thread::sleep(boost::posix_time::seconds(10));
                                        break;
                                    }
                                }
                                if (d.first == _boot_disk || _launcher_job_create_detail.os_type == saasame::transport::hv_guest_os_type::HV_OS_LINUX){
                                    win_vhdx_mgmt::merge_virtual_disk(_virtual_disk_map[_boot_disk].wstring());
                                }
                                boost::filesystem::remove(_virtual_disk_map[d.first]);
                            }
                        }
                        if (_vhd_export_lock)
                            _vhd_export_lock->unlock();
                        if (boost::filesystem::exists(_pre_script_file)){
                            try{
                                boost::filesystem::path temp = macho::windows::environment::create_temp_folder();
                                {
                                    archive::unzip::ptr unzip_ptr = archive::unzip::open(_pre_script_file);
                                    if (unzip_ptr){
                                        unzip_ptr->decompress_archive(temp);
                                        boost::filesystem::path pre_script_run = temp / "run.cmd";
                                        if (boost::filesystem::exists(pre_script_run)){
                                            std::wstring ret;
                                            process::exec_console_application_with_timeout(pre_script_run.wstring(), ret, -1);
                                            LOG(LOG_LEVEL_RECORD, L"Pre script return (%s)", ret.c_str());
                                        }
                                    }
                                }
                                boost::filesystem::remove_all(temp);
                            }
                            catch (const boost::filesystem::filesystem_error& ex){
                                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                            }
                            catch (const boost::exception &ex){
                                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                            }
                            catch (const std::exception& ex){
                                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                            }
                            catch (...){
                                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                            }
                        }
                        LOG(LOG_LEVEL_RECORD, L"Disk(s) conversion completed.");
                        record(saasame::transport::job_state::type::job_state_finished,
                            saasame::transport::error_codes::SAASAME_NOERROR,
                            (record_format("Disk(s) conversion completed.")));
                        _state = saasame::transport::job_state::type::job_state_finished;
                        save_status();
                        _update(false);
#ifdef _VMWARE_VADP
                        if (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
                            bool is_need_uefi_boot = false;
                            if (!_launcher_job_create_detail.gpt_to_mbr){
                                storage::disk::ptr vm_boot_disk = stg->get_disk(_disk_map[_boot_disk]);
                                if (vm_boot_disk && vm_boot_disk->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
                                    is_need_uefi_boot = true;
                                    foreach(storage::partition::ptr p, vm_boot_disk->get_partitions()){
                                        if (0 == _wcsicmp(p->gpt_type().c_str(), L"21686148-6449-6E6F-744E-656564454649")){
                                            is_need_uefi_boot = false;
                                            break;
                                        }
                                    }
                                }
                            }
                            _disk_map.clear();
                            repository.terminated = true;
                            SetEvent(repository.quit_event);
                            repository.thread_pool.join_all();
                            repository.tasks.clear();
                            conn = NULL;
                            launch_vmware_virtual_machine(portal, vm, is_need_uefi_boot);
                        }
#endif
                    }
                    if (macho::windows::environment::is_winpe()){
                        registry reg(REGISTRY_CREATE);
                        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            reg[L"RecoveryImageReady"] = (DWORD)0x1;
                        }
                        if (_launcher_job_create_detail.reboot_winpe){
                            macho::windows::environment::shutdown_system(true,L"",0);
                            std::wstring ret;
                            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s\\wpeutil.exe reboot") % macho::windows::environment::get_system_directory()), ret, -1, true);
                        }
                    }
                }
                else{
                    if (_vhd_export_lock)
                        _vhd_export_lock->unlock();
                    foreach(disk_map::value_type &d, _disk_map){
                        std::wstring physical_path;
                        if (_virtual_disk_map.count(d.first)){
                            while (ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_physical_path(_virtual_disk_map[d.first].wstring(), physical_path)){
                                if (ERROR_SUCCESS == win_vhdx_mgmt::detach_virtual_disk(_virtual_disk_map[d.first].wstring())){
                                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                                    break;
                                }
                            }
                            boost::filesystem::remove(_virtual_disk_map[d.first]);
                        }
                    }
                    record(saasame::transport::job_state::type::job_state_converting,
                        saasame::transport::error_codes::SAASAME_E_JOB_CONVERT_FAIL,
                        (record_format("Disk(s) conversion failed.")));
                    _state |= mgmt_job_state::job_state_error;
                    save_status();
                }
            }
        }

#if defined(_VMWARE_VADP) || defined(_AZURE_BLOB)
        if ((_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP) ||
            (_launcher_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB)){
            repository.terminated = true;
            SetEvent(repository.quit_event);
            repository.thread_pool.join_all();
            repository.tasks.clear();
        }
#endif

    }

    if (_is_canceled){
        //record((saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_JOB_CANCELLED, record_format("Job cancelled."));
    }  
    else if (_is_interrupted){
        record((saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_JOB_INTERRUPTED, record_format("Job interrupted."));
    }
    else if (_state & mgmt_job_state::job_state_finished){
        //record((saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, record_format("Job completed."));
    }
    save_status();
    _terminated = true;
    _update(false);
    if (_thread.joinable())
        _thread.join();
}

trigger::vtr launcher_job::get_triggers(){
    trigger::vtr triggers;
    foreach(saasame::transport::job_trigger t, _create_job_detail.triggers){
        trigger::ptr _t;
        switch (t.type){
            case saasame::transport::job_trigger_type::interval_trigger:{
                if (t.start.length() && t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish)));
                }
                else if (t.start.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time));
                }
                else if (t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish)));
                }
                else{
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration)));
                }
                break;
            }
            case saasame::transport::job_trigger_type::runonce_trigger:{
                if (t.start.length()){
                    _t = trigger::ptr(new run_once_trigger(boost::posix_time::time_from_string(t.start)));
                }
                else{
                    _t = trigger::ptr(new run_once_trigger());
                }
                break;
            }
        }
        triggers.push_back(_t);
    }
    return triggers;
}

void launcher_job::upload_status(const uint32_t part_number, const std::string etag, const uint32_t length){
    macho::windows::auto_lock lock(_cs);
    _upload_progress += length;
    //_upload_maps[_uploading_disk]->progress = _upload_progress;
    save_status();
    std::ofstream outfile;
    outfile.open(_journal_file.string(), std::ofstream::out | std::ofstream::app);
    outfile << boost::str(boost::format("%1%:%2%\r\n") %part_number%etag);
}

bool launcher_job::upload_vhd_to_cloud(disk_map &disk_map){
    bool result = true;
    FUN_TRACE;
#if 1
    if ((_launcher_job_create_detail.options_type == extra_options_type::ALIYUN &&
        !_launcher_job_create_detail.options.aliyun.bucketname.empty() &&
        !_launcher_job_create_detail.options.aliyun.region.empty() &&
        !_launcher_job_create_detail.options.aliyun.objectname.empty() &&
        !_launcher_job_create_detail.options.aliyun.access_key.empty() &&
        !_launcher_job_create_detail.options.aliyun.secret_key.empty()) || 
        (_launcher_job_create_detail.options_type == extra_options_type::TENCENT &&
        !_launcher_job_create_detail.options.tencent.bucketname.empty() &&
        !_launcher_job_create_detail.options.tencent.region.empty() &&
        !_launcher_job_create_detail.options.tencent.objectname.empty() &&
        !_launcher_job_create_detail.options.tencent.access_key.empty() &&
        !_launcher_job_create_detail.options.tencent.secret_key.empty())
        ){
        result = false;
        _state = saasame::transport::job_state::type::job_state_uploading;
        record(saasame::transport::job_state::type::job_state_uploading,
            saasame::transport::error_codes::SAASAME_NOERROR,
            (record_format("Uploading VHD.")));
        save_status();
        universal_disk_rw::ptr io = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_map.begin()->second));
        if (io){
            size_t s = (((clone_disk::get_boundary_of_partitions(io) - 1) >> 30) + 1);
            _disk_size = s;
            std::auto_ptr<vhd_tool> vhd = std::auto_ptr<vhd_tool>(vhd_tool::create((uint64_t)s << 30));
            if (vhd.get() != NULL){            
                changed_area::vtr changed = clone_disk::get_clone_data_range(clone_disk_rw(io, _disk_size));
                uint32_t written = 0, read = 0;   
                std::auto_ptr<BYTE> buf = std::auto_ptr<BYTE>(new BYTE[BUFF_BLOCK_SZ]);
                foreach(changed_area &c, changed){
                    if (!(result = vhd->write(c.start, NULL, c.length, written)))
                        break;
                }
                if (result){
                    if (0 == _total_upload_size)
                        _total_upload_size = vhd->estimate_size();
                    save_status();
                    if (boost::filesystem::exists(_journal_file)){
                        std::ifstream file(_journal_file.string());
                        std::string temp;
                        _uploaded_parts.clear();
                        while (std::getline(file, temp)) {
                            if (!temp.empty()){
                                std::vector<std::string> values = stringutils::tokenize2(temp, ":", 2);
                                if (values.size() == 2){
                                    _uploaded_parts[std::stol(values[0])] = values[1];
                                }
                            }
                        }
                    }
                    if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN){
                        aliyun_upload aliyun(_launcher_job_create_detail.options.aliyun.region,
                            _launcher_job_create_detail.options.aliyun.bucketname,
                            _launcher_job_create_detail.options.aliyun.objectname,
                            _launcher_job_create_detail.options.aliyun.access_key,
                            _launcher_job_create_detail.options.aliyun.secret_key,
                            _total_upload_size,
                            _upload_id,
                            _uploaded_parts,
                            _launcher_job_create_detail.options.aliyun.number_of_upload_threads);
                        if (result = aliyun.initial()){
                            vhd->reset();
                            aliyun.register_block_to_be_uploaded_callback_function(boost::bind(&launcher_job::upload_status, this, _1, _2, _3));
                            vhd->register_cancel_callback_function(boost::bind(&launcher_job::is_canceled, this));
                            vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &aliyun, _1, _2, _3, _4));
                            foreach(changed_area &c, changed){
                                memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                if (!(result = io->read(c.start, c.length, buf.get(), read)))
                                    break;
                                else if (!(result = vhd->write(c.start, buf.get(), c.length, written)))
                                    break;
                            }
                            save_status();
                            if (result){
                                result = vhd->close();
                                if (result){
                                    _total_upload_size = vhd->estimate_size();
                                    result = aliyun.complete();
                                }
                            }
                        }
                    }
                    else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT){
                        tencent_upload tencent(_launcher_job_create_detail.options.tencent.region,
                            _launcher_job_create_detail.options.tencent.bucketname,
                            _launcher_job_create_detail.options.tencent.objectname,
                            _launcher_job_create_detail.options.tencent.access_key,
                            _launcher_job_create_detail.options.tencent.secret_key,
                            _total_upload_size,
                            _upload_id,
                            _uploaded_parts,
                            _launcher_job_create_detail.options.tencent.number_of_upload_threads);
                        if (result = tencent.initial()){
                            vhd->reset();
                            tencent.register_block_to_be_uploaded_callback_function(boost::bind(&launcher_job::upload_status, this, _1, _2, _3));
                            vhd->register_cancel_callback_function(boost::bind(&launcher_job::is_canceled, this));
                            vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &tencent, _1, _2, _3, _4));
                            foreach(changed_area &c, changed){
                                memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                if (!(result = io->read(c.start, c.length, buf.get(), read)))
                                    break;
                                else if (!(result = vhd->write(c.start, buf.get(), c.length, written)))
                                    break;
                            }
                            save_status();
                            if (result){
                                result = vhd->close();
                                if (result){
                                    _total_upload_size = vhd->estimate_size();
                                    result = tencent.complete();
                                }
                            }
                        }
                    }
                }
            }
        }
        if (result){
            _state = saasame::transport::job_state::type::job_state_upload_completed;
            save_status();
            LOG(LOG_LEVEL_RECORD, L"Disk upload completed. (%s)", macho::stringutils::convert_ansi_to_unicode(disk_map.begin()->first).c_str());
        }
    }    
#else
    typedef std::map<std::wstring, changed_area::vtr> changed_areas_map;
    changed_areas_map totle_changed_areas;
    {
        universal_disk_rw::vtr rws;
        foreach(disk_map::value_type& d, disk_map){
            universal_disk_rw::ptr io = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d.second));
            if (io){
                rws.push_back(io);
            }
        }
        totle_changed_areas = clone_disk::get_clone_data_range(rws);
    }

    if ( (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN &&
        !_launcher_job_create_detail.options.aliyun.bucketname.empty() &&
        !_launcher_job_create_detail.options.aliyun.region.empty() &&
        !_launcher_job_create_detail.options.aliyun.objectname.empty() &&
        !_launcher_job_create_detail.options.aliyun.access_key.empty() &&
        !_launcher_job_create_detail.options.aliyun.secret_key.empty())  
        || (_launcher_job_create_detail.options_type == extra_options_type::TENCENT &&
        !_launcher_job_create_detail.options.tencent.bucketname.empty() &&
        !_launcher_job_create_detail.options.tencent.region.empty() &&
        !_launcher_job_create_detail.options.tencent.objectname.empty() &&
        !_launcher_job_create_detail.options.tencent.access_key.empty() &&
        !_launcher_job_create_detail.options.tencent.secret_key.empty())
        ){
        result = false;
        _state = saasame::transport::job_state::type::job_state_uploading;
        record(saasame::transport::job_state::type::job_state_uploading,
            saasame::transport::error_codes::SAASAME_NOERROR,
            (record_format("Uploading VHD.")));
        save_status();
        foreach(disk_map::value_type& d, disk_map){
            std::string object_name = _launcher_job_create_detail.options.aliyun.objectname;
            if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT)
                object_name = _launcher_job_create_detail.options.tencent.objectname;
            universal_disk_rw::ptr io = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d.second));
            if (io){
                size_t s = (((clone_disk::get_boundary_of_partitions(io) - 1) >> 30) + 1);
                _disk_size = s;
                std::auto_ptr<vhd_tool> vhd = std::auto_ptr<vhd_tool>(vhd_tool::create((uint64_t)s << 30));
                if (vhd.get() != NULL){
                    changed_area::vtr changed = totle_changed_areas[io->path()];
                    if (!_upload_maps.count(d.first)){
                        boost::filesystem::remove(_journal_file);
                        _uploading_disk = d.first;
                        _upload_maps[_uploading_disk] = upload_progress::ptr(new upload_progress());
                        _upload_maps[_uploading_disk]->vhd_size = s;
                        save_status();
                    }
                    else{
                        if (result = _upload_maps[d.first]->is_completed)
                            continue;
                    }
                    uint32_t written = 0, read = 0;
                    std::auto_ptr<BYTE> buf = std::auto_ptr<BYTE>(new BYTE[BUFF_BLOCK_SZ]);
                    foreach(changed_area &c, changed){
                        if (!(result = vhd->write(c.start, NULL, c.length, written)))
                            break;
                    }
                    if (result){
                        if (0 == _total_upload_size)
                            _upload_maps[_uploading_disk]->size = _total_upload_size = vhd->estimate_size();
                        save_status();
                        if (boost::filesystem::exists(_journal_file)){
                            std::ifstream file(_journal_file.string());
                            std::string temp;
                            _uploaded_parts.clear();
                            while (std::getline(file, temp)) {
                                if (!temp.empty()){
                                    std::vector<std::string> values = stringutils::tokenize2(temp, ":", 2);
                                    if (values.size() == 2){
                                        _uploaded_parts[std::stol(values[0])] = values[1];
                                    }
                                }
                            }
                        }
                        if (_launcher_job_create_detail.options_type == extra_options_type::ALIYUN){
                            aliyun_upload aliyun(_launcher_job_create_detail.options.aliyun.region,
                                _launcher_job_create_detail.options.aliyun.bucketname,
                                object_name,
                                _launcher_job_create_detail.options.aliyun.access_key,
                                _launcher_job_create_detail.options.aliyun.secret_key,
                                _total_upload_size,
                                _upload_id,
                                _uploaded_parts,
                                _launcher_job_create_detail.options.aliyun.number_of_upload_threads,
                                _launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE);
                            if (result = aliyun.initial()){
                                vhd->reset();
                                aliyun.register_block_to_be_uploaded_callback_function(boost::bind(&launcher_job::upload_status, this, _1, _2, _3));
                                vhd->register_cancel_callback_function(boost::bind(&launcher_job::is_canceled, this));
                                vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &aliyun, _1, _2, _3, _4));
                                foreach(changed_area &c, changed){
                                    memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                    if (!(result = io->read(c.start, c.length, buf.get(), read)))
                                        break;
                                    else if (!(result = vhd->write(c.start, buf.get(), c.length, written)))
                                        break;
                                }
                                save_status();
                                if (result){
                                    result = vhd->close();
                                    if (result){
                                        _total_upload_size = vhd->estimate_size();
                                        result = aliyun.complete();
                                    }
                                }
                            }
                        }
                        else if (_launcher_job_create_detail.options_type == extra_options_type::TENCENT){
                            tencent_upload tencent(_launcher_job_create_detail.options.tencent.region,
                                _launcher_job_create_detail.options.tencent.bucketname,
                                object_name,
                                _launcher_job_create_detail.options.tencent.access_key,
                                _launcher_job_create_detail.options.tencent.secret_key,
                                _total_upload_size,
                                _upload_id,
                                _uploaded_parts,
                                _launcher_job_create_detail.options.tencent.number_of_upload_threads,
                                _launcher_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE);
                            if (result = tencent.initial()){
                                vhd->reset();
                                tencent.register_block_to_be_uploaded_callback_function(boost::bind(&launcher_job::upload_status, this, _1, _2, _3));
                                vhd->register_cancel_callback_function(boost::bind(&launcher_job::is_canceled, this));
                                vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &tencent, _1, _2, _3, _4));
                                foreach(changed_area &c, changed){
                                    memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                    if (!(result = io->read(c.start, c.length, buf.get(), read)))
                                        break;
                                    else if (!(result = vhd->write(c.start, buf.get(), c.length, written)))
                                        break;
                                }
                                save_status();
                                if (result){
                                    result = vhd->close();
                                    if (result){
                                        _total_upload_size = vhd->estimate_size();
                                        result = tencent.complete();
                                    }
                                }
                            }
                        }
                    }
                    if (result){
                        boost::filesystem::remove(_journal_file);
                        _uploaded_parts.clear();
                        _upload_id.clear();
                        _uploading_disk.clear();
                        _upload_maps[_uploading_disk]->size = _total_upload_size;
                        _upload_maps[_uploading_disk]->is_completed = true;
                        _total_upload_size = 0;
                        save_status();
                    }
                }
            }
            if (result){
                LOG(LOG_LEVEL_RECORD, L"Disk upload completed. (%s)", macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
            }
            else{
                break;
            }
        }
        if (result && disk_map.size() == _upload_maps.size()){
            foreach(upload_progress::map::value_type &p, _upload_maps){
                if (!p.second->is_completed){
                    result = false;
                    break;
                }
            }
            if (result)                
                _state = saasame::transport::job_state::type::job_state_upload_completed;
        }
    }  
#endif
    return result;
}

bool launcher_job::is_uploading(){
    int state = _state;
    state &= ~mgmt_job_state::job_state_error;
    return state == saasame::transport::job_state::type::job_state_uploading || state == saasame::transport::job_state::type::job_state_upload_completed;
}

void launcher_job::modify_job_scheduler_interval(uint32_t minutes){
    FUN_TRACE;
    thrift_connect<launcher_serviceClient> thrift(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, "localhost");
    if (thrift.open()){
        saasame::transport::job_trigger t;
        if (_create_job_detail.triggers.size()){
            t = _create_job_detail.triggers.at(0);
            _create_job_detail.triggers.clear();
            t.start = boost::posix_time::to_simple_string(_created_time);
            t.interval = minutes;
            t.type = saasame::transport::job_trigger_type::interval_trigger;
        }
        else{
            t.id = guid_::create();
            t.start = boost::posix_time::to_simple_string(_created_time);
            t.type = saasame::transport::job_trigger_type::interval_trigger;
            t.interval = minutes;
        }
        _create_job_detail.triggers.push_back(t);
        _create_job_detail.__set_triggers(_create_job_detail.triggers);
        _create_job_detail.__set_is_ssl(_create_job_detail.is_ssl);
        _create_job_detail.__set_management_id(_create_job_detail.management_id);
        _create_job_detail.__set_mgmt_addr(_create_job_detail.mgmt_addr);
        _create_job_detail.__set_mgmt_port(_create_job_detail.mgmt_port);
        _create_job_detail.__set_triggers(_create_job_detail.triggers);
        _create_job_detail.__set_type(_create_job_detail.type);
        thrift.client()->update_job("", macho::stringutils::convert_unicode_to_utf8(_id), _create_job_detail);
    }
}

void launcher_job::update_create_detail(const saasame::transport::create_job_detail& detail){
    FUN_TRACE;
    {
        macho::windows::auto_lock lock(_config_lock);
        _create_job_detail = detail;
    }
    save_config();
}
#ifdef _VMWARE_VADP
changed_disk_extent::vtr launcher_job::get_excluded_extents(const changed_area::vtr& changes, uint64_t size){
    FUN_TRACE;
    uint64_t min_excluded_size = 8388608UL;
    uint64_t start = 0;
    uint64_t end = start + size;
    changed_disk_extent::vtr excluded_extents;
    if (changes.size()){
        changed_disk_extent extent;
        int64_t next_start = start;
        foreach(auto s, changes)
        {
            if (next_start != 0 && next_start < s.start)
            {
                extent.start = next_start;
                extent.length = s.start - next_start;
                if (extent.length >= min_excluded_size)
                {
                    excluded_extents.push_back(extent);
                    LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
                }
            }
            next_start = s.start + s.length;
        }
        extent.start = next_start;
        extent.length = end - next_start;
        if (extent.length >= min_excluded_size)
        {
            excluded_extents.push_back(extent);
            LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
        }
    }
    return excluded_extents;
}

changed_disk_extent::vtr launcher_job::final_changed_disk_extents(changed_disk_extent::vtr& src, changed_disk_extent::vtr& excludes, uint64_t max_size){

    changed_disk_extent::vtr chunks;
    if (src.empty())
        return src;
    else if (excludes.empty())
        chunks = src;
    else
    {
        std::sort(src.begin(), src.end(), [](changed_disk_extent const& lhs, changed_disk_extent const& rhs){ return lhs.start < rhs.start; });
        std::sort(excludes.begin(), excludes.end(), [](changed_disk_extent const& lhs, changed_disk_extent const& rhs){ return lhs.start < rhs.start; });

        changed_disk_extent::vtr::iterator it = excludes.begin();
        int64_t start = 0;

        foreach(auto e, src)
        {
            LOG(LOG_LEVEL_DEBUG, "s/%I64u:%I64u", e.start, e.length);
            start = e.start;

            if (it != excludes.end())
            {
                while (it->start + it->length < start)
                {
                    it = excludes.erase(it);
                    if (it == excludes.end())
                        break;
                }

                if (it != excludes.end())
                {
                    if ((e.start + e.length) < it->start)
                        chunks.push_back(e);
                    else
                    {
                        while (start < e.start + e.length)
                        {
                            changed_area chunk;

                            if (it->start - start > 0)
                            {
                                chunk.start = start;
                                chunk.length = it->start - start;
                                chunks.push_back(changed_disk_extent(chunk.start,chunk.length));
                                LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", chunk.start, chunk.length);
                            }

                            if (it->start + it->length < e.start + e.length)
                            {
                                start = it->start + it->length;
                                it++;
                                if (it == excludes.end() || it->start >(e.start + e.length))
                                {
                                    chunk.start = start;
                                    chunk.length = e.start + e.length - start;
                                    chunks.push_back(changed_disk_extent(chunk.start, chunk.length));
                                    LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", chunk.start, chunk.length);
                                    break;
                                }
                            }
                            else
                                break;
                        }
                    }
                }
                else
                {
                    chunks.push_back(e);
                    LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", e.start, e.length);
                }
            }
            else
            {
                chunks.push_back(e);
                LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", e.start, e.length);
            }
        }
    }
    changed_disk_extent::vtr _chunks;

    foreach(auto s, chunks){
        if (s.length <= max_size)
            _chunks.push_back(s);
        else{
            UINT64 length = s.length;
            UINT64 next_start = s.start;
            while (length > max_size){
                _chunks.push_back(changed_disk_extent(next_start, max_size));
                length -= max_size;
                next_start += max_size;
            }
            _chunks.push_back(changed_disk_extent(next_start, length));
        }
    }
    std::sort(_chunks.begin(), _chunks.end(), [](changed_disk_extent const& lhs, changed_disk_extent const& rhs){ return lhs.start < rhs.start; });

    return _chunks;
}
#endif