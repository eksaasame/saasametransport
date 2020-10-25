// main.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "macho.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "ntfs_parser.h"
#include <ntddvol.h>
#include "..\gen-cpp\temp_drive_letter.h"
#include "..\vcbt\vcbt\fatlbr.h"
#include <VersionHelpers.h>
#include "vhdtool.h"
#include "mirror_disk.h"
#include "clone_disk.h"
#include "aliyun_upload.h"
#include "shrink_volume.h"
#include "azure_blob_op.h"
#ifdef _DEBUG
#include "common.h"
#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_MINIPORT_PROCESS_SERVICE_IRP CTL_CODE(IOCTL_SCSI_BASE,  0x040e, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif
using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;
#define BUFF_BLOCK_SZ        0x800000

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    po::options_description command("Command");
    command.add_options()
        ("number,n", po::value<int>()->default_value(0, "0"), "disk number")
        ("target,t", po::value<int>(), "target disk number")
        ("vhd,o", po::value<std::string>(), "output to vhd file")
        ("shrink,s", po::value<int>(), "shrink disk")
        ("size,z", po::value<int>(), "size of shrinked disk (GB)")
#ifdef _DEBUG
        ("mount", po::wvalue<std::wstring>(), "Mount Page Blob disk")
#endif
        ("partitions,p", "output partitions layout")
        ("volumes,v", "output volumes layout")
        ;

    po::options_description general("General");
    general.add_options()
        ("level,l", po::value<int>()->default_value(5, "5"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;

    po::options_description all("Allowed options");
    all.add(general).add(command);

    try{
        std::wstring c = GetCommandLine();
#if _UNICODE
        po::store(po::wcommand_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#else
        po::store(po::command_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#endif
        po::notify(vm);
        if (vm.count("help") || (vm["level"].as<int>() > 5) || (vm["level"].as<int>() < 0)){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("number"))
                result = true;
            else
                std::cout << title << all << std::endl;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << command << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << command << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

struct volume_logical_to_physical{
    typedef std::map<uint32_t, std::vector<volume_logical_to_physical>> map;
    typedef std::map<std::wstring, volume_logical_to_physical::map> mapper;
    volume_logical_to_physical(ULONGLONG s, ULONGLONG p, ULONGLONG l) : logical_start(s), physical_start(p), length(l) {}
    ULONGLONG logical_start;
    ULONGLONG physical_start;
    ULONGLONG length;
    struct sort_by_logical {
        bool operator() (volume_logical_to_physical const & lhs, volume_logical_to_physical const & rhs) const {
            return (rhs).logical_start > (lhs).logical_start;
        }
    };
    struct sort_by_physical {
        bool operator() (volume_logical_to_physical const & lhs, volume_logical_to_physical const & rhs) const {
            return (rhs).physical_start > (lhs).physical_start;
        }
    };
};

void get_dynamic_volumes_mapper(volume_logical_to_physical::mapper &mapper){
    storage::ptr st = storage::local();
    storage::disk::vtr disks = st->get_disks();
    foreach(storage::disk::ptr &d, disks){
        bool is_dynamic = false;
        macho::windows::storage::partition::vtr parts = d->get_partitions();
        foreach(macho::windows::storage::partition::ptr &p, parts){
            if (d->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT){
                if ((macho::guid_(p->gpt_type()) == macho::guid_("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3")) || // PARTITION_LDM_METADATA_GUID
                    (macho::guid_(p->gpt_type()) == macho::guid_("af9b60a0-1431-4f62-bc68-3311714a69ad"))){ //PARTITION_LDM_DATA_GUID
                    is_dynamic = true;
                    break;
                }
            }
            else {
                if (p->mbr_type() == MS_PARTITION_LDM){
                    is_dynamic = true;
                    break;
                }
            }
        }
        if (is_dynamic){
            macho::windows::storage::volume::vtr vols = d->get_volumes();
            foreach(macho::windows::storage::volume::ptr &v, vols){


            }
        }
    }
}

bool write_vhd(std::ofstream* vhdfile, uint64_t start, uint32_t block_number, LPBYTE buff, size_t len){
    //LOG(LOG_LEVEL_RECORD, "Start: %I64u, Length: %d", start, len);
    (*vhdfile).seekp(start, std::ios_base::beg);
    (*vhdfile).write((char*)buff, len);
    return true;
}

bool write_vhd2(aliyun_upload* aliyun, uint64_t start, uint32_t block_number, LPBYTE buff, size_t len){
    //LOG(LOG_LEVEL_RECORD, "Start: %I64u, Length: %d", start, len);
    return aliyun->upload(start, block_number, buff, len);
}

#ifdef _DEBUG
struct device_mount_task;
typedef boost::shared_ptr<device_mount_task> device_mount_task_ptr;
typedef std::vector<device_mount_task_ptr> device_mount_task_vtr;
struct device_mount_repository{
    device_mount_repository() : terminated(false), clone_rw(false), is_cache_enabled(true){
        quit_event = CreateEvent(
            NULL,   // lpEventAttributes
            TRUE,   // bManualReset
            FALSE,  // bInitialState
            NULL    // lpName
            );
    }
    virtual ~device_mount_repository(){
        CloseHandle(quit_event);
    }
    bool                        terminated;
    bool                        clone_rw;
    std::wstring				device_path;
    HANDLE			            quit_event;
    device_mount_task_vtr		tasks;
    boost::thread_group			thread_pool;
    bool                        is_cache_enabled;
    bool						is_canceled(){ return terminated; }
};

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
            connectIn.MergeIOs = TRUE;
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

int _tmain(int argc, _TCHAR* argv[])
{
    macho::windows::com_init com;
    po::variables_map vm;
    DWORD dwRet = 0;
    boost::filesystem::path exepath = macho::windows::environment::get_execution_full_path();
    std::wstring working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path logfile = macho::windows::environment::get_execution_full_path() + L".log";
    //macho::set_log_file(logfile.wstring());
    //macho::set_log_level(LOG_LEVEL_DEBUG);
    if (command_line_parser(vm)){
        macho::set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
        LOG(LOG_LEVEL_RECORD, L"%s running...", exepath.wstring().c_str());
        storage::ptr stg = storage::local();
        if (!stg){
            LOG(LOG_LEVEL_ERROR, L"Can't open storage management interface");
        }
        else{
            DWORD disk_number = vm["number"].as<int>();
            storage::disk::ptr d = stg->get_disk(disk_number);
            if (!d){
                LOG(LOG_LEVEL_ERROR, L"Can't find the disk (%d)", disk_number);
            }
            else{
                LOG(LOG_LEVEL_INFO, L"Disk Serial Number : %s ", d->serial_number().c_str());
                if (vm.count("partitions")){
                    storage::partition::vtr partitions = d->get_partitions();
                    if (!partitions.size()){
                        LOG(LOG_LEVEL_INFO, L"Can't find any partition.");
                    }
                    foreach(storage::partition::ptr p, partitions){
                        if (d->partition_style() == storage::ST_PST_MBR)
                            LOG(LOG_LEVEL_INFO, L"partition(%d) : MBR type(%d), offset(%I64u), length(%I64u), path(%s) ", p->partition_number(), p->mbr_type(), p->offset(), p->size(), p->drive_letter().c_str());
                        else
                            LOG(LOG_LEVEL_INFO, L"partition(%d) : offset(%I64u), length(%I64u), path(%s) ", p->partition_number(), p->offset(), p->size(), p->drive_letter().c_str());
                    }
                }
                if (vm.count("volumes")){
                    storage::volume::vtr volumes = d->get_volumes();
                    if (!volumes.size()){
                        LOG(LOG_LEVEL_INFO, L"Can't find any volume.");
                    }
                    foreach(storage::volume::ptr v, volumes){
                        LOG(LOG_LEVEL_INFO, L"volume(%s) : size (%I64u)", v->drive_letter().c_str(), v->size());
                    }
                }
                std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number);
                macho::windows::auto_file_handle hDevice = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (hDevice.is_invalid()){
                    LOG(LOG_LEVEL_ERROR, L"Can't open the device (%s)", path.c_str());
                }
                else if (vm.count("target")){
                    universal_disk_rw::ptr target = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % vm["target"].as<int>()), false);
                    if (target){
                        universal_disk_rw::ptr io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                        mirror_disk mirror;
                        uint64_t start = 0;
                        uint64_t size = 0;
                        std::wcout << L"Start to clone the disk...." << std::endl;
                        if (mirror.replicate_beginning_of_disk(io, *d.get(), start, size, universal_disk_rw::ptr())){
                            if (mirror.replicate_partitions_of_disk(io, *d.get(), start, size, universal_disk_rw::ptr())){
                                start = size = 0;
                                if (mirror.replicate_beginning_of_disk(io, *d.get(), start, size, target)){
                                    if (mirror.replicate_partitions_of_disk(io, *d.get(), start, size, target)){
                                        std::wcout << std::endl << L"Succeeded to clone the disk!" << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }

#if _DEBUG
                else if (vm.count("mount")){
                    std::wstring  device_path;
                    macho::windows::auto_handle handle;
                    macho::windows::device_manager  dev_mgmt;
                    hardware_device::vtr class_devices;
                    class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
                    foreach(hardware_device::ptr &d, class_devices){
                        std::wcout << L"GUID               : " << d->sz_class_guid << std::endl;
                        std::wcout << L"Class           : " << d->sz_class << std::endl;
                        std::wcout << L"Driver           : " << d->driver << std::endl;
                        std::wcout << L"Service           : " << d->service << std::endl;
                        std::wcout << L"Location           : " << d->location << std::endl;
                        std::wcout << L"Location Path      : " << d->location_path << std::endl;
                        std::wcout << L"Device Description : " << d->device_description << std::endl;
                        std::wcout << L"Device Instance Id : " << d->device_instance_id << std::endl;
                        if (d->service == L"vmp"){
                            device_path = dev_mgmt.get_device_path(d->device_instance_id, (LPGUID)&GUID_DEVINTERFACE_STORAGEPORT);
                            std::wcout << L"Device Path : " << device_path << std::endl;
                            break;
                        }
                        std::wcout << std::endl;
                    }

                    if (!device_path.empty()) {
                        DWORD							 bytesReturned;
                        COMMAND_IN						 command;
                        BOOL							 devStatus;
                        device_mount_repository      	 repository;
                        repository.device_path = device_path;
                        handle = CreateFile(device_path.c_str(),  // Name of the NT "device" to open 
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
                                std::wcout << L"Create device" << std::endl;
                                uint64_t size = 1024LL * 1024 * 1024 * 8;
                                device_mount_task::vtr tasks;
                                boost::thread_group thread_pool;
                                macho::guid_ g(vm["mount"].as<std::wstring>());
                                std::string connection_string = "UseDevelopmentStorage=true;";
                                if (azure_page_blob_op::create_vhd_page_blob(connection_string, "backup", std::string(g) + ".vhd", size, "base")){
                                    GUID guid = g;
                                    WCHAR disk_id[40];
                                    memset(disk_id, 0, sizeof(disk_id));
                                    _snwprintf_s(disk_id, sizeof(disk_id),
                                        L"%04x%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", 100,
                                        guid.Data1, guid.Data2, guid.Data3,
                                        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                                        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
                                    std::wcout << L"Disk Id :" << disk_id << std::endl;
                                    universal_disk_rw::ptr out = azure_page_blob_op::open_vhd_page_blob_for_rw(connection_string, "backup", std::string(g) + ".vhd");
                                    if (out){
                                        device_mount_task::ptr task(new device_mount_task(out, size / (1024 * 1024), disk_id, repository));
                                        task->register_task_is_canceled_function(boost::bind(&device_mount_repository::is_canceled, &repository));
                                        tasks.push_back(task);
                                    }
                                    else{
                                        std::wcout << L"Failed to open for rw." << std::endl;
                                    }
                                    if (tasks.size()){
                                        for (int i = 0; i < tasks.size(); i++)
                                            thread_pool.create_thread(boost::bind(&device_mount_task::mount, &(*tasks[i])));
                                        std::wcout << L"Press Enter to Continue" << std::endl;
                                        std::cin.ignore();
                                        repository.terminated = true;
                                        SetEvent(repository.quit_event);
                                        thread_pool.join_all();
                                    }
                                    azure_page_blob_op::delete_vhd_page_blob(connection_string, "backup", std::string(g) + ".vhd");
                                }
                            }
                        }
                        else{
                            std::wcout << L"Cannot open device" << std::endl;
                        }
                    }
                }
#endif
                else if (vm.count("vhd")){

#if 0
                    //vhd_tool::get_dirty_ranges(vm["vhd"].as<std::string>() );
                    universal_disk_rw::ptr io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                    size_t s = (((clone_disk::get_boundary_of_partitions(io) - 1) >> 30) + 1) << 30;
                    changed_area::vtr changed = clone_disk::get_clone_data_range(clone_disk_rw(io, s));
                    vhd_tool* vhd = vhd_tool::create(s, true);
                    if (vhd){                     
                        size_t size = vhd->estimate_size();
                        std::map<int, std::string> uploaded_parts;
                        azure_upload azure("UseDevelopmentStorage=true;", "backup", "test_4.vhd", size, uploaded_parts);
                        if (azure.initial()){
                            azure_upload::io_range::vtr frees;
                            size_t start = 0;                         
                            foreach(changed_area &c, changed){
                                if (start < c.start)
                                    frees.push_back(azure_upload::io_range(start, c.start - 1));
                                start = c.start + c.length;
                            }
                            if (start < s)
                                frees.push_back(azure_upload::io_range(start, s - 1));
                            azure.erase_free_ranges(frees);
                           // azure.copy_from( "backup", "test_3.vhd");
                        }
                    }
#else 
                    universal_disk_rw::ptr io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                    size_t s = (((clone_disk ::get_boundary_of_partitions(io) - 1) >> 30) + 1) << 30;
                    vhd_tool* vhd = vhd_tool::create(s, true);
                    if (vhd){                     
                        changed_area::vtr changed = clone_disk::get_clone_data_range(clone_disk_rw(io, s));
                        uint32_t written = 0, read = 0;
                        bool ret = false;
                        std::auto_ptr<BYTE> buf = std::auto_ptr<BYTE>(new BYTE[BUFF_BLOCK_SZ]);
                        std::string connection_string = "UseDevelopmentStorage=true;";
                        io_range::vtr ranges = azure_page_blob_op::get_page_ranges(connection_string, "backup", "sss1.vhd");
                        universal_disk_rw::ptr rw = azure_page_blob_op::open_vhd_page_blob_for_rw(connection_string, "backup", "sss1.vhd");
                        foreach(changed_area &c, changed){
                            memset(buf.get(), 0, BUFF_BLOCK_SZ);
                            if (!(ret = rw->read(c.start, c.length, buf.get(), read)))
                                break;
                            if (c.length != read){
                                foreach(io_range &r, ranges){
                                    uint64_t g = r.start + r.length;
                                    if (c.start >= r.start  && c.start < (r.start + r.length)){
                                        uint64_t x = r.start + r.length;
                                    }
                                }
                            }
                        }
                        /*azure_page_blob_op::delete_vhd_page_blob(connection_string, "backup", "sss1.vhd");
                        azure_page_blob_op::create_vhd_page_blob(connection_string, "backup", "sss1.vhd", s, "base");
                        universal_disk_rw::ptr rw = azure_page_blob_op::open_vhd_page_blob_for_rw(connection_string, "backup", "sss1.vhd");
                        foreach(changed_area &c, changed){
                            memset(buf.get(), 0, BUFF_BLOCK_SZ);
                            if (!(ret = io->read(c.start, c.length, buf.get(), read)))
                                break;
                            else if (!(ret = rw->write(c.start, buf.get(), c.length, written)))
                                break;
                        }*/
                       // azure_page_blob_op::create_vhd_page_blob_snapshot("UseDevelopmentStorage=true;", "backup", "sss.vhd", "snap1");
#if 0
                        size_t size = vhd->estimate_size();
                        std::map<int, std::string> uploaded_parts;
                        azure_upload azure("UseDevelopmentStorage=true;", "backup", "test_3.vhd", size, uploaded_parts);
                        if (azure.initial()){
                            vhd->reset();
                            vhd->register_flush_data_callback_function(boost::bind(&azure_upload::upload, &azure, _1, _2, _3, _4));
                            foreach(changed_area &c, changed){
                                memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                if (!(ret = io->read(c.start, c.length, buf.get(), read)))
                                    break;
                                else if (!(ret = vhd->write(c.start, buf.get(), c.length, written)))
                                    break;
                            }
                            if (ret)
                                vhd->close();
                            azure.wait();
                            azure.complete();
                        }

#endif

#if 0
                        foreach(changed_area &c, changed){
                            if (!(ret = vhd->write(c.start, NULL, c.length, written)))
                                break;
                        }
                        if (ret){
                            size_t size = vhd->estimate_size();
                            std::string upload_id;
                            std::map<int, std::string> uploaded_parts;
                            tencent_upload tencent("ap-beijing", "-", "1.vhd", "", "", size, upload_id, uploaded_parts);
                            if (tencent.initial()){
                                vhd->reset();
                                vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &tencent, _1, _2, _3, _4));
                                foreach(changed_area &c, changed){
                                    memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                    if (!(ret = io->read(c.start, c.length, buf.get(), read)))
                                        break;
                                    else if (!(ret = vhd->write(c.start, buf.get(), c.length, written)))
                                        break;
                                }
                                if (ret)
                                    vhd->close();
                                tencent.wait();
                                tencent.complete();
                            }
                        }
#endif
#if 0
                        foreach(changed_area &c, changed){
                            if (!(ret = vhd->write(c.start, NULL, c.length, written)))
                                break;
                        }
                        if (ret){
                            size_t size = vhd->estimate_size();
                            std::string upload_id;
                            std::map<int, std::string> uploaded_parts;
                            aliyun_upload aliyun("oss-ap-southeast-1", "saasame-bucket", "v1.vhd", "sss", "ssss", size, upload_id, uploaded_parts);
                            if (aliyun.initial()){
                                vhd->reset();
                                vhd->register_flush_data_callback_function(boost::bind(&aliyun_upload::upload, &aliyun, _1, _2, _3, _4));
                                foreach(changed_area &c, changed){
                                    memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                    if (!(ret = io->read(c.start, c.length, buf.get(), read)))
                                        break;
                                    else if (!(ret = vhd->write(c.start, buf.get(), c.length, written)))
                                        break;
                                }
                                if (ret)
                                    vhd->close();
                                aliyun.wait();
                                aliyun.complete();
                            }
                        }
#endif
#if 0
                        foreach(changed_area &c, changed){
                            if (!(ret = vhd->write(c.start, NULL, c.length, written)))
                                break;
                        }
                        std::ofstream vhdfile;
                        vhdfile.open(vm["vhd"].as<std::string>(), std::ios::out | std::ios::binary);
                        if (vhdfile.is_open()){
                            if (ret){
                                uint64_t size = vhd->estimate_size();
                                vhd->reset();
                                vhd->register_flush_data_callback_function(boost::bind(write_vhd, &vhdfile, _1, _2, _3, _4));
                                foreach(changed_area &c, changed){
                                    memset(buf.get(), 0, BUFF_BLOCK_SZ);
                                    if (!(ret = io->read(c.start, c.length, buf.get(), read)))
                                        break;
                                    else if (!(ret = vhd->write(c.start, buf.get(), c.length, written)))
                                        break;
                                }
                                uint64_t new_size = vhd->estimate_size();
                                if (ret)
                                    vhd->close();
                                vhdfile.close();
                            }
                        }
#endif

                        /*universal_disk_rw::ptr _vhd_ptr(vhd);
                        universal_disk_rw::ptr io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                        mirror_disk mirror;
                        uint64_t start = 0;
                        uint64_t size = 0;
                        std::wcout << L"Start to clone the disk...." << std::endl;
                        mirror.turn_on_vhd_preparation();
                        if (mirror.replicate_beginning_of_disk(io, *d.get(), start, size, _vhd_ptr)){
                            if (mirror.replicate_partitions_of_disk(io, *d.get(), start, size, _vhd_ptr)){
                                start = size = 0;
                                mirror.turn_off_vhd_preparation();
                                std::ofstream vhdfile;
                                vhdfile.open("e:\\v.vhd", std::ios::out | std::ios::binary);
                                if (vhdfile.is_open())
                                {
                                    vhd->register_flush_data_callback_function(boost::bind(write_vhd, &vhdfile, _1, _2, _3));
                                    vhd->flush_header();
                                    if (mirror.replicate_beginning_of_disk(io, *d.get(), start, size, _vhd_ptr)){
                                        if (mirror.replicate_partitions_of_disk(io, *d.get(), start, size, _vhd_ptr)){
                                            vhd->close();
                                            std::wcout << std::endl << L"Succeeded to clone the disk!" << std::endl;
                                        }
                                    }
                                    vhdfile.close();
                                }
                            }
                        }*/
                    }
#endif
                }
                else if (vm.count("shrink")){
                    if (!vm.count("size")){
                        std::wcout << L"Please specify the size (GB) for volume shrinking!" << std::endl;
                    }
                    else{
                        uint64_t max_size = (uint64_t)vm["size"].as<int>() << 30;
                        storage::disk::ptr disk = stg->get_disk(vm["shrink"].as<int>());
                        bool result;
                        if (!(result = macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true))){
                            LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_BACKUP_NAME). ");
                        }
                        if (!(result = macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true))){
                            LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_RESTORE_NAME). ");
                        }
                        if (disk->size() > max_size){
                            storage::partition::vtr parts = disk->get_partitions();
                            storage::partition::ptr part;
                            foreach(storage::partition::ptr p, parts){
                                if (NULL == part || part->offset() < p->offset())
                                    part = p;
                            }
                            if (part && part->access_paths().size()){
                                uint64_t size = (max_size - part->offset() - (16 << 20));
                                shrink_volume::ptr shrink = shrink_volume::open(part->access_paths()[0], size);
                                if (shrink){
                                    if (shrink->shrink()){
                                        std::wcout << L"Succeeded to shrink volume!" << std::endl;
                                    }
                                    else{
                                        std::wcout << L"Failed to shrink volume!" << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }
                else{
                    universal_disk_rw::ptr io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                    ntfs::volume::vtr volumes = ntfs::volume::get(io);
                    foreach(ntfs::volume::ptr v, volumes){
                        LOG(LOG_LEVEL_INFO, L"ntfs volume: offset(%I64u), length(%I64u)", v->start(), v->length());
                        ntfs::file_record::ptr root = v->root();
                        if (root){
                            ntfs::file_extent::vtr exts;
                            ntfs::file_record::ptr pagefile = v->find_sub_entry(L"pagefile.sys");
                            if (pagefile){
                                exts = pagefile->extents();
                                LOG(LOG_LEVEL_INFO, L"pagefile : file_size(%I64u), file_allocated_size(%I64u)", pagefile->file_size(), pagefile->file_allocated_size());
                            }
                            ntfs::file_record::ptr windows = v->find_sub_entry(L"windows");
                            if (windows){
                                ntfs::file_record::ptr memorydump = v->find_sub_entry(L"memory.dmp", windows);
                                ULONGLONG size = 0;
                                if (memorydump){
                                    exts = memorydump->extents();
                                    size = memorydump->file_size();
                                    size = memorydump->file_allocated_size();
                                    LOG(LOG_LEVEL_INFO, L"memory dump : file_size(%I64u), file_allocated_size(%I64u)", memorydump->file_size(), memorydump->file_allocated_size());
                                }
                                io_range::vtr ranges = v->file_system_ranges();
                            }
                        }
                    }
                }               
            }
        }
    }

    return 0;
}