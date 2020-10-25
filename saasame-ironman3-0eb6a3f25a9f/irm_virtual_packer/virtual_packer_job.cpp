#include "virtual_packer_job.h"
#include "common_service_handler.h"
#include "management_service.h"
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TPipe.h>
#include <thrift/transport/TSocket.h>
#include <codecvt>
#include "vmware_ex.h"
#include "universal_disk_rw.h"
#include "carrier_rw.h"
#include "..\irm_converter\irm_disk.h"
#include "ntfs_parser.h"
#include "..\ntfs_parser\ntfs_parser.h"
#include "lz4.h"
#include "lz4hc.h"
#pragma comment(lib, "liblz4.lib" )

using namespace json_spirit;
using namespace macho::windows;
using namespace macho;
using namespace mwdc::ironman::hypervisor;
using namespace mwdc::ironman::hypervisor_ex;
using namespace saasame::transport;

//GUID type:		{caddebf1-4400-4de8-b103-12117dcf3ccf}
//Partition name:   Microsoft shadow copy partition
DEFINE_GUID(PARTITION_SHADOW_COPY_GUID, 0xcaddebf1L, 0x4400, 0x4de8, 0xb1, 0x03, 0x12, 0x11, 0x7d, 0xcf, 0x3c, 0xcf);

#define MBR_PART_SIZE_SECTORS   1
#define GPT_PART_SIZE_SECTORS   34
#define MINI_BLOCK_SIZE         65536UL
#define MAX_BLOCK_SIZE          8388608UL
#define WAIT_INTERVAL_SECONDS   2
#define SIGNAL_THREAD_READ      1

typedef std::map<std::string, std::vector<io_changed_range>> io_changed_ranges_map;

#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

int _compress(__in const char* source, __inout char* dest, __in int source_size, __in int max_compressed_size){
    return LZ4_compress_fast(source, dest, source_size, max_compressed_size, 1);
}

struct esx_lock_ex: public virtual lock_able_ex{
    typedef boost::shared_ptr<esx_lock_ex> ptr;
    esx_lock_ex(std::wstring name, uint32_t max_count) : _semaphore(name, max_count){
    }
    ~esx_lock_ex(){
        unlock();
    }
    virtual bool lock(){ 
        bool result = false;
        while (!(result=_semaphore.lock(10000))){ // 10 seconds 
            if (is_canceled()){
                break;
            }
        }
        return result;
    }
    virtual bool unlock() { return _semaphore.unlock(); }
    virtual bool trylock() { return _semaphore.trylock(); }

    static esx_lock_ex::ptr get(std::wstring name, uint32_t max_count) {
        return esx_lock_ex::ptr(new esx_lock_ex(name, max_count));
    }
    macho::windows::semaphore     _semaphore;
};

struct replication_repository{
    typedef boost::shared_ptr<replication_repository> ptr;
    replication_repository(virtual_transport_job_progress& _progress) : terminated(false), progress(_progress), block_size(MAX_BLOCK_SIZE), queue_size(CARRIER_RW_IMAGE_BLOCK_SIZE * 4), compressed_by_packer(false){}
    macho::windows::critical_section                          cs;
    replication_block::queue                                  waitting;
    replication_block::vtr                                    processing;
    replication_block::queue                                  error;
    boost::mutex                                              completion_lock;
    boost::condition_variable                                 completion_cond;
    boost::mutex                                              pending_lock;
    boost::condition_variable                                 pending_cond;
    virtual_transport_job_progress&                           progress;
    uint32_t                                                  block_size;
    uint32_t                                                  queue_size;
    bool                                                      terminated;
    bool                                                      compressed_by_packer;
};

struct replication_task{
    typedef boost::shared_ptr<replication_task> ptr;
    typedef std::vector<ptr> vtr;
    replication_task(universal_disk_rw::ptr _in, universal_disk_rw::vtr _outs, replication_repository& _repository) : in(_in), outs(_outs), repository(_repository){}
    typedef boost::signals2::signal<bool(), macho::_or<bool>> job_is_canceled;
    typedef boost::signals2::signal<void()> job_save_statue;
    typedef boost::signals2::signal<void(saasame::transport::job_state::type state, int error, record_format& format)> job_record;
    inline void register_job_is_canceled_function(job_is_canceled::slot_type slot){
        is_canceled.connect(slot);
    }
    inline void register_job_save_callback_function(job_save_statue::slot_type slot){
        save_status.connect(slot);
    }
    inline void register_job_record_callback_function(job_record::slot_type slot){
        record.connect(slot);
    }
    replication_repository&                                     repository;
    universal_disk_rw::ptr                                      in;
    universal_disk_rw::vtr                                      outs;
    job_is_canceled                                             is_canceled;
    job_save_statue                                             save_status;
    job_record                                                  record;
    void                                                        replicate();
    uint32_t                                                    code;
    std::wstring                                                message;
};

void replication_task::replicate()
{
    FUN_TRACE;
    bool                             result = true;
    std::string                      buf;
    replication_block::ptr           block;
    bool                             pendding = false;
    uint32_t                         number_of_bytes_to_read = 0;
    uint32_t                         number_of_bytes_to_written = 0;
    std::string                      compressed_buf;
    std::vector<carrier_rw*>         rws;
    buf.reserve(repository.block_size);
    if (repository.compressed_by_packer)
        compressed_buf.reserve(repository.block_size);
    foreach(universal_disk_rw::ptr& o, outs){
        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
        if (rw) {
            rws.push_back(rw);
        }
    }
    while (true)
    {
        {
            macho::windows::auto_lock lock(repository.cs);
            if (block){
                if (!result){
                    repository.terminated = true;
                    repository.error.push_back(block);
                    break;
                }
                else{
                    for (replication_block::vtr::iterator b = repository.processing.begin(); b != repository.processing.end(); b++){
                        if ((*b)->index == block->index){
                            repository.processing.erase(b);
                            break;
                        }
                    }
                    {
                        bool is_updated = false;
                        {
                            macho::windows::auto_lock lock(repository.progress.lock);
                            repository.progress.completed.push_back(block);
                            std::sort(repository.progress.completed.begin(), repository.progress.completed.end(), replication_block::compare());
                            while (repository.progress.completed.size() && repository.error.size() == 0){
                                if (0 == repository.processing.size() || repository.progress.completed.front()->End() < repository.processing[0]->Start()){
                                    block = repository.progress.completed.front();
                                    repository.progress.completed.pop_front();
                                    repository.progress.backup_image_offset = block->End();
                                    repository.progress.backup_progress += block->Length(); 
                                    is_updated = true;
                                }
                                else
                                    break;
                            }
                        }
                        if (is_updated)
                            save_status();
                    }
                }
            }
            if (0 == repository.waitting.size() || repository.terminated || is_canceled())
                break;
            block = repository.waitting.front();
            repository.waitting.pop_front();
            foreach(replication_block::ptr& b, repository.progress.completed){
                if (b->start == block->start && b->length == block->length){
                    LOG(LOG_LEVEL_INFO, _T("Skip completed block( %I64u, %I64u)"), block->start, block->length);
                    if (0 == repository.waitting.size()){
                        block = NULL;
                        break;
                    }
                    block = repository.waitting.front();
                    repository.waitting.pop_front();
                }
                else if ((b->Start()) > block->End()){
                    LOG(LOG_LEVEL_INFO, _T("b->Start() > block->End()(%I64u > %I64u)"), b->Start(), block->End());
                    break;
                }
            }
            if (!block)
                break;
            repository.processing.push_back(block);
            std::sort(repository.processing.begin(), repository.processing.end(), replication_block::compare());
            if (block->Start() - repository.processing[0]->Start() > repository.queue_size)
                pendding = true;
            else
                pendding = false;

#ifdef SIGNAL_THREAD_READ
            number_of_bytes_to_read = 0;
            int read_retry = 25;
            buf.resize(block->Length());
            while ((read_retry > 0) && !(result = in->read(block->Start(), block->Length(), &buf[0], number_of_bytes_to_read))){
                read_retry--;
                boost::this_thread::sleep(boost::posix_time::seconds(WAIT_INTERVAL_SECONDS));
                if (repository.terminated)
                    break;
            }
#endif
        }
        repository.pending_cond.notify_all();

#ifndef SIGNAL_THREAD_READ
        number_of_bytes_to_read = 0;
        memset(buf.get(), 0, repository.block_size);
        int read_retry = 25;
        while ((read_retry > 0) && !(result = in->read(block->Start(), block->Length(), buf.get(), number_of_bytes_to_read))){
            read_retry--;
            boost::this_thread::sleep(boost::posix_time::seconds(WAIT_INTERVAL_SECONDS));
            if (repository.terminated)
                break;
    }
#endif
        if (!result){
            LOG(LOG_LEVEL_ERROR, _T("Cannot read( %I64u, %I64u)"), block->Start(), block->Length());
            code = saasame::transport::error_codes::SAASAME_E_IMAGE_READ;
            message = boost::str(boost::wformat(L"Cannot read( %1%, %2%)") % (block->Start()) % (block->Length()));
        }
        else{
            buf.resize(number_of_bytes_to_read);
            if (repository.compressed_by_packer){
                std::string* out = &buf;
                uint32_t compressed_length;
                bool compressed = false;
                compressed_buf.resize(number_of_bytes_to_read);
                compressed_length = _compress(reinterpret_cast<const char*>(buf.c_str()), reinterpret_cast<char*>(&compressed_buf[0]), number_of_bytes_to_read, number_of_bytes_to_read);
                if (compressed_length != 0){
                    if (compressed_length < number_of_bytes_to_read){
                        compressed = true;
                        compressed_buf.resize(compressed_length);
                        out = &compressed_buf;
                    }
                }
                while (pendding && !repository.terminated){
                    {
                        macho::windows::auto_lock lock(repository.cs);
                        if (block->Start() - repository.processing[0]->Start() < repository.queue_size)
                            break;
                    }
                    if (!repository.terminated){
                        boost::unique_lock<boost::mutex> lock(repository.pending_lock);
                        repository.pending_cond.timed_wait(lock, boost::posix_time::milliseconds(30000));
                    }
                }
                foreach(carrier_rw* o, rws){
                    if (!(result = o->write_ex(block->Start(), *out, number_of_bytes_to_read, compressed, number_of_bytes_to_written))){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), block->Start(), number_of_bytes_to_read);
                        code = saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE;
                        message = boost::str(boost::wformat(L"Cannot write( %1%, %2%)") % block->Start() % number_of_bytes_to_read);
                        break;
                    }
                }
            }
            else{
                while (pendding && !repository.terminated){
                    {
                        macho::windows::auto_lock lock(repository.cs);
                        if (block->Start() - repository.processing[0]->Start() < repository.queue_size)
                            break;
                    }
                    if (!repository.terminated){
                        boost::unique_lock<boost::mutex> lock(repository.pending_lock);
                        repository.pending_cond.timed_wait(lock, boost::posix_time::milliseconds(30000));
                    }
                }
                foreach(carrier_rw* o, rws){
                    bool need_wait_for_queue = true;
                    while (need_wait_for_queue){
                        try{
                            need_wait_for_queue = false;
                            result = false;
                            if (!(result = o->write(block->Start(), buf, number_of_bytes_to_written))){
                                LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), block->Start(), number_of_bytes_to_read);
                                code = saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE;
                                message = boost::str(boost::wformat(L"Cannot write( %1%, %2%)") % (block->Start()) % number_of_bytes_to_read);
                                break;
                            }
                        }
                        catch (const out_of_queue_exception &e){
                            need_wait_for_queue = true;
                        }

                        if (need_wait_for_queue){
                            while (!repository.terminated && !o->is_buffer_free()){
                                boost::this_thread::sleep(boost::posix_time::seconds(1));
                            }
                        }

                        if (repository.terminated){
                            result = false;
                            break;
                        }
                    }
                    if (repository.terminated || !result)
                        break;
                }
            }
        }
    }
    if (!repository.terminated && is_canceled())
        repository.terminated = true;
    repository.completion_cond.notify_one();
    repository.pending_cond.notify_all();
}

virtual_packer_job::virtual_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail) : _state(job_state_none), _create_job_detail(detail), _is_interrupted(false), _is_canceled(false), removeable_job(id, macho::guid_(GUID_NULL)), _block_size(MAX_BLOCK_SIZE), _min_transport_size(MINI_BLOCK_SIZE), _guest_os_type(saasame::transport::hv_guest_os_type::HV_OS_UNKNOWN)
{
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
}

virtual_packer_job::virtual_packer_job(saasame::transport::create_packer_job_detail detail) : _state(job_state_none), _create_job_detail(detail), _is_interrupted(false), _is_canceled(false), removeable_job(macho::guid_::create(), macho::guid_(GUID_NULL)), _block_size(MAX_BLOCK_SIZE), _min_transport_size(MINI_BLOCK_SIZE), _guest_os_type(saasame::transport::hv_guest_os_type::HV_OS_UNKNOWN)
{
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
}

virtual_packer_job::ptr virtual_packer_job::create(std::string id, saasame::transport::create_packer_job_detail detail)
{
    FUN_TRACE;
    virtual_packer_job::ptr job;
    if (detail.type == saasame::transport::job_type::virtual_packer_job_type){
        // Physical migration type
        job = virtual_packer_job::ptr(new virtual_packer_job( macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    return job;
}

virtual_packer_job::ptr virtual_packer_job::create(saasame::transport::create_packer_job_detail detail)
{
    FUN_TRACE;
    virtual_packer_job::ptr job;
    if (detail.type == saasame::transport::job_type::virtual_packer_job_type){
        // Physical migration type
        job = virtual_packer_job::ptr(new virtual_packer_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    return job;
}

virtual_packer_job::ptr virtual_packer_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file)
{
    FUN_TRACE;
    virtual_packer_job::ptr job;
    std::wstring id;
    saasame::transport::create_packer_job_detail _create_job_detail = load_config(config_file.wstring(), id);
    if (_create_job_detail.type == saasame::transport::job_type::virtual_packer_job_type){
        // Physical migration type
        job = virtual_packer_job::ptr(new virtual_packer_job(id, _create_job_detail));
   
        job->load_status(status_file.wstring());
    }
    return job;
}

void virtual_packer_job::remove()
{
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"Job remove event captured.");
    _is_removing = true;
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        _running.unlock();
    }
}

saasame::transport::create_packer_job_detail virtual_packer_job::load_config(std::wstring config_file, std::wstring &job_id)
{
    FUN_TRACE;
    saasame::transport::create_packer_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(job_config.get_obj(), "id").get_str());
        detail.type = (saasame::transport::job_type::type)find_value(job_config.get_obj(), "type").get_int();

        mArray  connection_ids = find_value(job_config.get_obj(), "connection_ids").get_array();
        foreach(mValue i, connection_ids){
            detail.connection_ids.insert(i.get_str());
        }

        mArray  carriers = find_value(job_config.get_obj(), "carriers").get_array();
        foreach(mValue c, carriers){
            std::string connection_id = find_value_string(c.get_obj(), "connection_id");
            mArray  carrier_addr = find_value(c.get_obj(), "carrier_addr").get_array();
            std::set<std::string> _addr;
            foreach(mValue a, carrier_addr){
                _addr.insert(a.get_str());
            }
            detail.carriers[connection_id] = _addr;
        }

        detail.detail.v.host = find_value(job_config.get_obj(), "host").get_str();
    
        mArray  addr = find_value(job_config.get_obj(), "addr").get_array();
        foreach(mValue a, addr){
            detail.detail.v.addr.insert(a.get_str());
        }

        macho::bytes user, password;
        user.set(macho::stringutils::convert_utf8_to_unicode(find_value(job_config.get_obj(), "u1").get_str()));
        password.set(macho::stringutils::convert_utf8_to_unicode(find_value(job_config.get_obj(), "u2").get_str()));
        macho::bytes u1 = macho::windows::protected_data::unprotect(user, true);
        macho::bytes u2 = macho::windows::protected_data::unprotect(password, true);

        if (u1.length() && u1.ptr()){
            detail.detail.v.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
        }
        else{
            detail.detail.v.username = "";
        }
        if (u2.length() && u2.ptr()){
            detail.detail.v.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
        }
        else{
            detail.detail.v.password = "";
        }

        detail.type = (saasame::transport::job_type::type)find_value(job_config.get_obj(), "type").get_int();
        detail.detail.v.virtual_machine_id = find_value(job_config.get_obj(), "virtual_machine_id").get_str();
        detail.detail.v.snapshot = find_value(job_config.get_obj(), "snapshot").get_str();
    
        mArray disks = find_value(job_config.get_obj(), "disks").get_array();
        foreach(mValue d, disks){
            detail.detail.v.disks.insert(d.get_str());
        }

        mArray images = find_value(job_config.get_obj(), "images").get_array();
        foreach(mValue i, images){
            packer_disk_image image;
            image.name = find_value_string(i.get_obj(), "name");
            image.parent = find_value_string(i.get_obj(), "parent");
            detail.detail.v.images[find_value_string(i.get_obj(), "key")] = image;
        }

        mArray  backup_size = find_value(job_config.get_obj(), "backup_size").get_array();
        foreach(mValue i, backup_size){
            detail.detail.v.backup_size[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  backup_progress = find_value(job_config.get_obj(), "backup_progress").get_array();
        foreach(mValue i, backup_progress){
            detail.detail.v.backup_progress[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  backup_image_offset = find_value(job_config.get_obj(), "backup_image_offset").get_array();
        foreach(mValue i, backup_image_offset){
            detail.detail.v.backup_image_offset[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  previous_change_ids = find_value(job_config.get_obj(), "previous_change_ids").get_array();
        foreach(mValue t, previous_change_ids){
            detail.detail.v.previous_change_ids[find_value_string(t.get_obj(), "disk")] = find_value_string(t.get_obj(), "changeid");
        }

        mArray  completed_blocks = find_value_array(job_config.get_obj(), "completed_blocks");
        foreach(mValue i, completed_blocks){
            mArray completeds = find_value_array(i.get_obj(), "value");
            std::string key = find_value_string(i.get_obj(), "key");
            foreach(mValue c, completeds){
                io_changed_range completed;
                completed.start = find_value(c.get_obj(), "o").get_int64();
                completed.length = find_value(c.get_obj(), "l").get_int64();
                detail.detail.v.completed_blocks[key].push_back(completed);
            }
        }

        detail.timeout = find_value_int32(job_config.get_obj(), "timeout", 600);
        detail.checksum_verify = find_value(job_config.get_obj(), "checksum_verify").get_bool();
        detail.is_encrypted = find_value_bool(job_config.get_obj(), "is_encrypted");
        detail.worker_thread_number = find_value_int32(job_config.get_obj(), "worker_thread_number");
        detail.file_system_filter_enable = find_value_bool(job_config.get_obj(), "file_system_filter_enable");
        detail.min_transport_size = find_value_int32(job_config.get_obj(), "min_transport_size");
        detail.full_min_transport_size = find_value_int32(job_config.get_obj(), "full_min_transport_size");
        detail.is_compressed = find_value_bool(job_config.get_obj(), "is_compressed");
        detail.is_checksum = find_value_bool(job_config.get_obj(), "is_checksum");
        detail.is_only_single_system_disk = find_value_bool(job_config.get_obj(), "is_only_single_system_disk", false);
        detail.is_compressed_by_packer = find_value_bool(job_config.get_obj(), "is_compressed_by_packer", false);
        mArray  checksum_target = find_value_array(job_config.get_obj(), "checksum_target");
        foreach(mValue t, checksum_target){
            detail.checksum_target[find_value_string(t.get_obj(), "connection_id")] = find_value_string(t.get_obj(), "machine_id");
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return detail;
}

void virtual_packer_job::save_config()
{
    FUN_TRACE;
    try{
        using namespace json_spirit;
        mObject job_config;

        job_config["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_config["type"] = _create_job_detail.type;
        mArray  connection_ids(_create_job_detail.connection_ids.begin(), _create_job_detail.connection_ids.end());
        job_config["connection_ids"] = connection_ids;
        mArray carriers;
        foreach(string_set_map::value_type &t, _create_job_detail.carriers){
            mObject carrier;
            carrier["connection_id"] = t.first;
            mArray carrier_addr(t.second.begin(), t.second.end());
            carrier["carrier_addr"] = carrier_addr;
            carriers.push_back(carrier);
        }
        job_config["carriers"] = carriers;
        job_config["host"] = _create_job_detail.detail.v.host;
        mArray  addr(_create_job_detail.detail.v.addr.begin(), _create_job_detail.detail.v.addr.end());
        job_config["addr"] = addr;
        macho::bytes user, password;
        user.set((LPBYTE)_create_job_detail.detail.v.username.c_str(), _create_job_detail.detail.v.username.length());
        password.set((LPBYTE)_create_job_detail.detail.v.password.c_str(), _create_job_detail.detail.v.password.length());
        macho::bytes u1 = macho::windows::protected_data::protect(user, true);
        macho::bytes u2 = macho::windows::protected_data::protect(password, true);
        job_config["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
        job_config["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
        job_config["type"] = (int)_create_job_detail.type;
        job_config["virtual_machine_id"] = _create_job_detail.detail.v.virtual_machine_id;
        job_config["snapshot"] = _create_job_detail.detail.v.snapshot;
        mArray  disks(_create_job_detail.detail.v.disks.begin(), _create_job_detail.detail.v.disks.end());
        job_config["disks"] = disks;
        mArray images;
        typedef std::map<std::string, packer_disk_image>  packer_disk_image_map;
        foreach(packer_disk_image_map::value_type &i, _create_job_detail.detail.v.images){
            mObject o;
            o["key"] = i.first;
            o["name"] = i.second.name;
            o["parent"] = i.second.parent;
            images.push_back(o);
        }
        job_config["images"] = images;
        
        mArray previous_change_ids;
        foreach(string_map::value_type &i, _create_job_detail.detail.v.previous_change_ids){
            mObject d;
            d["disk"] = i.first;
            d["changeid"] = i.second;
            previous_change_ids.push_back(d);
        }
        job_config["previous_change_ids"] = previous_change_ids;

        typedef std::map<std::string, int64_t>  string_int64_t_map;

        mArray backup_size;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.v.backup_size){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_size.push_back(o);
        }
        job_config["backup_size"] = backup_size;

        mArray backup_progress;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.v.backup_progress){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_progress.push_back(o);
        }
        job_config["backup_progress"] = backup_progress;

        mArray backup_image_offset;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.v.backup_image_offset){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_image_offset.push_back(o);
        }
        job_config["backup_image_offset"] = backup_image_offset;

        mArray completed_blocks;
        foreach(io_changed_ranges_map::value_type &c, _create_job_detail.detail.v.completed_blocks){
            mObject o;
            o["key"] = c.first;
            mArray completeds;
            foreach(io_changed_range& r, c.second){
                mObject completed;
                completed["o"] = r.start;
                completed["l"] = r.length;
                completeds.push_back(completed);
            }
            o["value"] = completeds;
            completed_blocks.push_back(o);
        }
        job_config["completed_blocks"] = completed_blocks;

        job_config["checksum_verify"] = _create_job_detail.checksum_verify;
        job_config["timeout"] = _create_job_detail.timeout;
        job_config["is_encrypted"] = _create_job_detail.is_encrypted;
        job_config["worker_thread_number"] = _create_job_detail.worker_thread_number;
        job_config["file_system_filter_enable"] = _create_job_detail.file_system_filter_enable;
        job_config["min_transport_size"] = _create_job_detail.min_transport_size;
        job_config["full_min_transport_size"] = _create_job_detail.full_min_transport_size;
        job_config["is_compressed"] = _create_job_detail.is_compressed;
        job_config["is_checksum"] = _create_job_detail.is_checksum;
        job_config["is_only_single_system_disk"] = _create_job_detail.is_only_single_system_disk;
        job_config["is_compressed_by_packer"] = _create_job_detail.is_compressed_by_packer;
        mArray checksum_target;
        foreach(string_map::value_type &t, _create_job_detail.checksum_target){
            mObject _target;
            _target["connection_id"] = t.first;
            _target["machine_id"] = t.second;
            checksum_target.push_back(_target);
        }
        job_config["checksum_target"] = checksum_target;
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
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")));
    }
    catch (...){
    }
}
//
//void virtual_packer_job::record(saasame::transport::job_state::type state, uint64_t error, std::string description)
//{
//    FUN_TRACE;
//    macho::windows::auto_lock lock(_cs);
//    _histories.push_back(history_record::ptr(new history_record(state, error, description)));
//    LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), macho::stringutils::convert_ansi_to_unicode(description).c_str());
//}

void virtual_packer_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    _histories.push_back(history_record::ptr(new history_record(state, error, format)));
    LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(format.str()).c_str());
}

void virtual_packer_job:: save_status()
{
    FUN_TRACE;
    try
    {
        macho::windows::auto_lock lock(_cs);
        mObject job_status;
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);
        mArray progress;
        foreach(virtual_transport_job_progress::map::value_type &p, _progress){
            mObject o;
            o["key"] = p.first;
            o["output"] = p.second->output;
            o["base"] = p.second->base;
            o["parent"] = p.second->parent;
            o["total_size"] = p.second->total_size;
            o["backup_progress"] = p.second->backup_progress;
            o["backup_size"] = p.second->backup_size;
            o["backup_image_offset"] = p.second->backup_image_offset;
            o["uuid"] = p.second->uuid;
            o["vmdk"] = p.second->vmdk;
            {
                macho::windows::auto_lock _lock(p.second->lock);
                mArray completed_blocks;
                foreach(replication_block::ptr &b, p.second->completed){
                    mObject completed;
                    completed["o"] = b->start;
                    completed["l"] = b->length;
                    completed_blocks.push_back(completed);
                }
                o["completed_blocks"] = completed_blocks;
            }
            progress.push_back(o);
        }
        job_status["progress"] = progress;
        mArray histories;
        foreach(history_record::ptr &h, _histories)
        {
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
        
        mArray change_ids;
        foreach(string_map::value_type &i, _change_ids){
            mObject d;
            d["disk"] = i.first;
            d["changeid"] = i.second;
            change_ids.push_back(d);
        }
        job_status["change_ids"] = change_ids;
        job_status["boot_disk"] = _boot_disk;
        mArray  system_disks(_system_disks.begin(), _system_disks.end());
        job_status["system_disks"] = system_disks;
        job_status["guest_os_type"] = (int)_guest_os_type;

        mArray virtual_disk_infos;
        foreach(const saasame::transport::virtual_disk_info_ex& info, _virtual_disk_infos){
            mObject d;
            d["id"] = info.id;
            d["is_system"] = info.is_system;
            d["partition_style"] = (int)info.partition_style;
            d["guid"] = info.guid;
            d["signature"] = info.signature;
            d["size"] = info.size;
            mArray partitions;
            foreach(const saasame::transport::virtual_partition_info& p, info.partitions){
                mObject _p;
                _p["partition_number"] = p.partition_number;
                _p["offset"] = p.offset;
                _p["size"] = p.size;
                partitions.push_back(_p);
            }
            d["partitions"] = partitions;
            virtual_disk_infos.push_back(d);
        }
        job_status["virtual_disk_infos"] = virtual_disk_infos;

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

bool virtual_packer_job::load_status(std::wstring status_file)
{
    FUN_TRACE;
    try{
        if (boost::filesystem::exists(status_file))
        {
            macho::windows::auto_lock lock(_cs);
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            _state = find_value(job_status.get_obj(), "state").get_int64();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _boot_disk = find_value_string(job_status.get_obj(), "boot_disk");
            mArray progress = find_value(job_status.get_obj(), "progress").get_array();
            foreach(mValue &p, progress){
                virtual_transport_job_progress::ptr ptr = virtual_transport_job_progress::ptr(new virtual_transport_job_progress());
                ptr->uuid = find_value_string(p.get_obj(), "uuid");
                ptr->vmdk = find_value_string(p.get_obj(), "vmdk");
                ptr->output = find_value_string(p.get_obj(), "output");
                ptr->base = find_value_string(p.get_obj(), "base");
                ptr->parent = find_value_string(p.get_obj(), "parent");
                ptr->total_size = find_value(p.get_obj(), "total_size").get_int64();
                ptr->backup_image_offset = find_value(p.get_obj(), "backup_image_offset").get_int64();
                ptr->backup_progress = find_value(p.get_obj(), "backup_progress").get_int64();
                ptr->backup_size = find_value(p.get_obj(), "backup_size").get_int64();
                mArray  completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
                foreach(mValue c, completed_blocks){
                    replication_block::ptr completed(new replication_block(0, 0, 0));
                    completed->start = find_value(c.get_obj(), "o").get_int64();
                    completed->length = find_value(c.get_obj(), "l").get_int64();
                    ptr->completed.push_back(completed);
                }
                _progress[find_value_string(p.get_obj(), "key")] = ptr;
            }
            mArray  change_ids = find_value(job_status.get_obj(), "change_ids").get_array();
            foreach(mValue t, change_ids){
                _change_ids[find_value_string(t.get_obj(), "disk")] = find_value_string(t.get_obj(), "changeid");
            }

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
            mArray  system_disks = find_value(job_status.get_obj(), "system_disks").get_array();
            foreach(mValue i, system_disks){
                _system_disks.insert(i.get_str());
            }
            _guest_os_type = (saasame::transport::hv_guest_os_type::type)find_value_int32(job_status.get_obj(), "guest_os_type");

            mArray  virtual_disk_infos = find_value(job_status.get_obj(), "virtual_disk_infos").get_array();
            foreach(mValue d, virtual_disk_infos){
                saasame::transport::virtual_disk_info_ex info;
                mArray partitions = find_value(d.get_obj(), "partitions").get_array();
                foreach(mValue p, partitions){
                    saasame::transport::virtual_partition_info _p;
                    _p.partition_number = find_value_int32(p.get_obj(), "partition_number");
                    _p.offset = find_value_int64(p.get_obj(), "offset");
                    _p.size = find_value_int64(p.get_obj(), "size");
                    info.partitions.insert(_p);
                }
                info.id = find_value_string(d.get_obj(), "id");
                info.guid = find_value_string(d.get_obj(), "guid");
                info.is_system = find_value_bool(d.get_obj(), "is_system");
                info.partition_style = (saasame::transport::partition_style::type)find_value_int32(d.get_obj(), "partition_style");
                info.signature = find_value_int32(d.get_obj(), "signature");
                info.size = find_value_int64(d.get_obj(), "size");
                _virtual_disk_infos.push_back(info);
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
    return true;
}

saasame::transport::packer_job_detail virtual_packer_job::get_job_detail(boost::posix_time::ptime previous_updated_time) 
{
    FUN_TRACE;

    macho::windows::auto_lock lock(_cs);
    saasame::transport::packer_job_detail job;

    job.id = macho::stringutils::convert_unicode_to_utf8(_id);
    job.__set_type(_create_job_detail.type);
    job.created_time = boost::posix_time::to_simple_string(_created_time);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    job.updated_time = boost::posix_time::to_simple_string(update_time);
    job.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
    foreach(history_record::ptr &h, _histories)
    {
        if (h->time > previous_updated_time)
        {
            saasame::transport::job_history _h;
            _h.state = h->state;
            _h.error = h->error;
            _h.description = h->description;
            _h.time = boost::posix_time::to_simple_string(h->time);
            _h.format = h->format;
            _h.is_display = h->is_display;
            _h.arguments = h->args;
            _h.__set_arguments(_h.arguments);
            job.histories.push_back(_h);
        }
    }
    job.__set_histories(job.histories);    

    foreach(virtual_transport_job_progress::map::value_type& v, _progress)
    {
        job.detail.v.original_size[v.first] = v.second->total_size;
        job.detail.v.backup_progress[v.first] = v.second->backup_progress;
        job.detail.v.backup_image_offset[v.first] = v.second->backup_image_offset;
        job.detail.v.backup_size[v.first] = v.second->backup_size;
        std::vector<io_changed_range> completeds;
        {
            macho::windows::auto_lock _lock(v.second->lock);
            foreach(replication_block::ptr &c, v.second->completed){
                io_changed_range _c(*c);
                completeds.push_back(_c);
            }
        }
        job.detail.v.completed_blocks[v.first] = completeds;
    }
    
    job.__set_detail(job.detail);
    job.detail.__set_v(job.detail.v);
    job.detail.v.__set_change_ids(_change_ids);
    job.detail.v.__set_guest_os_type(_guest_os_type);
    job.detail.v.__set_backup_image_offset(job.detail.v.backup_image_offset);
    job.detail.v.__set_backup_progress(job.detail.v.backup_progress);
    job.detail.v.__set_backup_size(job.detail.v.backup_size);
    job.detail.v.__set_original_size(job.detail.v.original_size);
    job.detail.v.__set_completed_blocks(job.detail.p.completed_blocks);
    job.__set_boot_disk(_boot_disk);
    foreach(std::string d, _system_disks){
        job.system_disks.push_back(d);
    }
    job.__set_system_disks(job.system_disks);
    if (_running.try_lock()){
        if ((_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error)
            job.is_error = true;
        _running.unlock();
    }
    job.detail.v.__set_disk_infos(_virtual_disk_infos);
    return job;
}

void virtual_packer_job::interrupt()
{
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"Job interrupt event captured.");

    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){
        return;
    }
    _is_interrupted = true;

    save_status();
}

bool virtual_packer_job::get_output_image_info(const std::string disk, virtual_transport_job_progress::ptr &p){
    FUN_TRACE;
    if (_create_job_detail.detail.v.images.count(disk)){
        p->output = _create_job_detail.detail.v.images[disk].name;
        p->parent = _create_job_detail.detail.v.images[disk].parent;
        p->base = _create_job_detail.detail.v.images[disk].base;
        return true;
    }
    return false;
}

void virtual_packer_job::execute()
{
    VALIDATE;
    FUN_TRACE;

    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || _is_removing)
    {
        return;
    }

    _state &= ~mgmt_job_state::job_state_error;
    if (!_is_interrupted)
    {
        try
        {
            _state = mgmt_job_state::job_state_replicating;
            bool result = false;
            //std::string thumbprint;
            save_status();
            vmware_portal_ex portal_ex;
            vmware_virtual_machine::ptr vm;
            portal_ex.set_log(get_log_file(), get_log_level());
            std::wstring virtual_machine_id = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.virtual_machine_id);
            std::wstring snapshot = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.snapshot);
            std::wstring host = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.host);
            protected_data user = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.username);
            protected_data password = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.password);
            //_create_job_detail.detail.v.username.clear();
            //_create_job_detail.detail.v.password.clear();
            esx_lock_ex::ptr locker = NULL;
            {
                registry reg;
                if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                    if (reg[L"ConcurrentVMwareJobPerESX"].exists() && (DWORD)reg[L"ConcurrentVMwareJobPerESX"] > 0){
                        locker = esx_lock_ex::get(host, (DWORD)reg[L"ConcurrentVMwareJobPerESX"]);
                        locker->register_is_cancelled_function(boost::bind(&virtual_packer_job::is_canceled, this));
                        locker->lock();
                    }
                }
            }

            bool     vm_migration = true;
            uint32_t concurrent_count = boost::thread::hardware_concurrency();
            registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"ConcurrentSchedulerJobNumber"].exists() && (DWORD)reg[L"ConcurrentSchedulerJobNumber"] > 0){
                    concurrent_count = ((DWORD)reg[L"ConcurrentSchedulerJobNumber"]);
                }
                if (reg[L"DisableVMMigration"].exists() && (DWORD)reg[L"DisableVMMigration"] > 0){
                    vm_migration = false;
                }
                if (reg[L"CompressedByPacker"].exists() && (DWORD)reg[L"CompressedByPacker"] > 0){
                    _create_job_detail.is_compressed_by_packer = true;
                }
            }

            esx_lock_ex  scheduler(L"Global\\Transport", concurrent_count);
            scheduler.register_is_cancelled_function(boost::bind(&virtual_packer_job::is_canceled, this));
            scheduler.lock();
            std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.host));
            vmware_portal_::ptr portal = NULL;
            if (is_canceled()){
                _state |= mgmt_job_state::job_state_error;
            }
            else if (!(portal = vmware_portal_::connect(uri, user, password)))
            {
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                    error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST,
                    (record_format("Cannot connect to VMware host %1%.") % _create_job_detail.detail.v.host));
                _state |= mgmt_job_state::job_state_error;
            }
            /*else if (S_OK != vmware_portal_ex::get_ssl_thumbprint(macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.host), 443, thumbprint)){
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                    error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST,
                    (record_format("Cannot get thumbprint data of Packer for VMware %1%.") % _create_job_detail.detail.v.host));
                portal = NULL;
            }*/
            else if (!(vm = portal->get_virtual_machine(virtual_machine_id)))
            {
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error),
                    error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST,
                    (record_format("Cannot get virtual machine %1% info.") % _create_job_detail.detail.v.virtual_machine_id));
                portal = NULL;
                _state |= mgmt_job_state::job_state_error;
            }
            else
            {
                portal_ex.set_portal(portal, virtual_machine_id);
                try{
                    std::wstring snapshot_ref;
                    vmware_vixdisk_connection::ptr conn;
                    portal_ex.get_snapshot_ref_item(virtual_machine_id, snapshot, snapshot_ref);
                    if ( snapshot_ref.empty() ||
                        NULL == ( conn = portal_ex.get_vixdisk_connection(host, user, password, virtual_machine_id, snapshot)))
                    {
                        result = false;
                        _state |= mgmt_job_state::job_state_error;
                    }
                    else
                    {
                        std::map<std::wstring, std::string, macho::stringutils::no_case_string_less_w> disks_map;
                        foreach(std::string disk, _create_job_detail.detail.v.disks){
                            disks_map[macho::stringutils::convert_utf8_to_unicode(disk)] = disk;
                        }
                        vmware_snapshot_disk_info::map snapshot_disks = portal->get_snapshot_info(snapshot_ref);
                        if (snapshot_disks.empty()){
                            LOG(LOG_LEVEL_ERROR, L"Cannot find any snapshot disk info.");
                        }

                        std::map<std::string, universal_disk_rw::ptr> vmdk_rws;
                        std::map<std::wstring, std::string> vmdk_ids_mapping;
                        std::map<std::string, changed_disk_extent::vtr> extents_map;

                        foreach(vmware_disk_info::ptr& _disk, vm->disks){
                            std::wstring wsz_disk = _disk->uuid.empty() ? _disk->wsz_key() : _disk->uuid;
                            if (!disks_map.count(wsz_disk))
                                continue;
                            std::string disk = disks_map[wsz_disk];
                            if (!snapshot_disks.count(wsz_disk)){
                                LOG(LOG_LEVEL_ERROR, L"Cannot find snapshot disk(%s) info.", wsz_disk.c_str());
                                result = false;
                                _state |= mgmt_job_state::job_state_error;
                                break;
                            }
                            else{
                                vmware_snapshot_disk_info::ptr snapshot_disk = snapshot_disks[wsz_disk];
                                virtual_transport_job_progress::ptr p;
                                vmdk_changed_areas changed;
                                std::wstring       changed_id = L"*";
                                if (_progress.count(disk))
                                {
                                    p = _progress[disk];
                                }
                                else
                                {
                                    p = virtual_transport_job_progress::ptr(new virtual_transport_job_progress());
                                    p->uuid = disk;
                                    p->vmdk = macho::stringutils::convert_unicode_to_utf8(snapshot_disk->name);
                                    p->total_size = snapshot_disk->size;
                                    if (_create_job_detail.detail.v.backup_size.count(disk))
                                        p->backup_size = _create_job_detail.detail.v.backup_size[disk];
                                    if (_create_job_detail.detail.v.backup_progress.count(disk))
                                        p->backup_progress = _create_job_detail.detail.v.backup_progress[disk];
                                    if (_create_job_detail.detail.v.backup_image_offset.count(disk))
                                        p->backup_image_offset = _create_job_detail.detail.v.backup_image_offset[disk];
                                    if (_create_job_detail.detail.v.completed_blocks.count(disk)){
                                        foreach(io_changed_range &r, _create_job_detail.detail.v.completed_blocks[disk]){
                                            p->completed.push_back(replication_block::ptr(new replication_block(0, r.start, r.length)));
                                        }
                                    }
                                    _progress[disk] = p;
                                    save_status();
                                }
                                if (p->total_size == p->backup_image_offset){
                                    result = true;
                                }
                                else{
                                    if (!(result = get_output_image_info(disk, p)))
                                    {
                                        _state |= mgmt_job_state::job_state_error;
                                        LOG(LOG_LEVEL_ERROR, L"Invalid job spec.");
                                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_JOB_CONFIG_NOTFOUND, record_format("Invalid job spec."));
                                        break;
                                    }
                                    if (!snapshot_disk->change_id.empty())
                                        _change_ids[disk] = macho::stringutils::convert_unicode_to_utf8(snapshot_disk->change_id);

                                    if (_create_job_detail.detail.v.previous_change_ids.count(disk)){
                                        if (_create_job_detail.min_transport_size > 0 && _create_job_detail.min_transport_size <= 8192){
                                            _min_transport_size = _create_job_detail.min_transport_size * 1024;
                                        }
                                        else{
                                            _min_transport_size = MINI_BLOCK_SIZE;
                                            registry reg;
                                            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                                                if (reg[L"DeltaMiniTransportSize"].exists()){
                                                    _min_transport_size = (DWORD)reg[L"DeltaMiniTransportSize"];
                                                    if (MAX_BLOCK_SIZE < _min_transport_size && _min_transport_size < MINI_BLOCK_SIZE)
                                                        _min_transport_size = MAX_BLOCK_SIZE;
                                                }
                                            }
                                        }
                                        changed_id = macho::stringutils::convert_utf8_to_unicode(_create_job_detail.detail.v.previous_change_ids[disk]);
                                    }
                                    else{
                                        if (_create_job_detail.full_min_transport_size > 0 && _create_job_detail.full_min_transport_size <= 8192){
                                            _min_transport_size = _create_job_detail.full_min_transport_size * 1024;
                                        }
                                        else{
                                            _min_transport_size = MAX_BLOCK_SIZE;
                                            registry reg;
                                            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                                                if (reg[L"FullMiniTransportSize"].exists()){
                                                    _min_transport_size = (DWORD)reg[L"FullMiniTransportSize"];
                                                    if (MAX_BLOCK_SIZE < _min_transport_size && _min_transport_size < MINI_BLOCK_SIZE)
                                                        _min_transport_size = MAX_BLOCK_SIZE;
                                                }
                                            }
                                        }
                                    }

                                    if (vm->is_cbt_enabled)
                                    {
                                        changed = portal->get_vmdk_changed_areas(vm->vm_mor_item, snapshot_ref, snapshot_disk->wsz_key(), changed_id, 0LL, snapshot_disk->size);

                                        if (changed.last_error_code)
                                        {
                                            if (vm_migration && changed_id == L"*"){
                                                LOG(LOG_LEVEL_WARNING, L"Doing full disk replication for migration.");
                                                changed.changed_list.clear();
                                                changed.changed_list.push_back(changed_disk_extent(0, snapshot_disk->size));
                                            }
                                            else{
                                                result = false;
                                                _state |= mgmt_job_state::job_state_error;
                                                LOG(LOG_LEVEL_ERROR, L"Can't get the changed areas from change id(%s). error (%s %d).", changed_id.c_str(), changed.error_description.c_str(), changed.last_error_code);
                                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Disk %1% has incorrect changed block tracking configuration.") % boost::filesystem::path(p->vmdk).filename().string());
                                                break;
                                            }
                                        }
                                        else if (!changed.changed_list.size())
                                        {
                                            changed.changed_list.push_back(changed_disk_extent(0, MINI_BLOCK_SIZE));
                                        }
                                    }
                                    else
                                    {
                                        changed.changed_list.push_back(changed_disk_extent(0, snapshot_disk->size));
                                    }
                                }
                                universal_disk_rw::ptr vmdk_rw = portal_ex.open_vmdk_for_rw(conn, snapshot_disk->name);
                                if (!vmdk_rw)
                                {
                                    LOG(LOG_LEVEL_ERROR, L"Cannot open vmdk disk (%s) for operation.", snapshot_disk->name.c_str());
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, record_format("Cannot open vmdk disk %1%.") % boost::filesystem::path(snapshot_disk->name).filename().string());
                                    result = false;
                                    _state |= mgmt_job_state::job_state_error;
                                    break;
                                }
                                else
                                {
                                    vmdk_rws[disk] = vmdk_rw;
                                    vmdk_ids_mapping[vmdk_rw->path()] = disk;
                                    extents_map[disk] = changed.changed_list;
                                }
                            }
                        }

                        if (result){

                            registry reg;
                            bool file_system_filter_enable = true;
                            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                                if (reg[L"FileSystemFilterEnable"].exists()){
                                    file_system_filter_enable = (DWORD)reg[L"FileSystemFilterEnable"] > 0;                               
                                }
                            }

                            if (file_system_filter_enable && _create_job_detail.file_system_filter_enable){
                                _io_ranges_map = linuxfs::volume::get_file_system_ranges(vmdk_rws);
                            }

                            _lvm_groups_devices_map = linuxfs::lvm_mgmt::get_groups(vmdk_rws);

                            foreach(vmware_disk_info::ptr& _disk, vm->disks)
                            {
                                std::wstring wsz_disk = _disk->uuid.empty() ? _disk->wsz_key() : _disk->uuid;
                                if (!disks_map.count(wsz_disk))
                                    continue;
                                std::string disk = disks_map[wsz_disk];
                                virtual_transport_job_progress::ptr p = _progress[disk];
                                if (p->total_size == p->backup_image_offset)
                                    continue;
                                changed_disk_extent::vtr extents = extents_map[disk];
                                bool bootable = false;
                                bool systemdisk = false;
                                if (result = calculate_disk_backup_size(vmdk_rws[disk], extents, p->backup_size, bootable, systemdisk))
                                {
                                    extents_map[disk] = extents;
                                    if (bootable){
                                        if (saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS == _guest_os_type){
                                            if (_boot_disk.empty() && systemdisk){
                                                _boot_disk = disk;
                                                _system_disks.insert(disk);
                                            }
                                        }
                                        else {
                                            _system_disks.insert(disk);
                                            if (_boot_disk.empty()){
                                                _boot_disk = disk;
                                            }
                                            std::wstring vmdk_path = vmdk_rws[disk]->path();
                                            foreach(linuxfs::lvm_mgmt::groups_map::value_type& g, _lvm_groups_devices_map){
                                                bool found = false;
                                                foreach(std::wstring p, g.second){
                                                    if (vmdk_path == p){
                                                        found = true; 
                                                        break;
                                                    }
                                                }
                                                if (found){
                                                    foreach(std::wstring p, g.second){
                                                        _system_disks.insert(vmdk_ids_mapping[p]);
                                                    }
                                                    
                                                    if (_create_job_detail.is_only_single_system_disk && _system_disks.size() > 1){
                                                        LOG(LOG_LEVEL_ERROR, L"Multiple system disks found. Target only supports a single system disk.");
                                                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), 
                                                            saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL, 
                                                            record_format("Multiple system disks found. Target only supports a single system disk."));
                                                        result = false;
                                                        _state |= mgmt_job_state::job_state_error;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    _state |= mgmt_job_state::job_state_error;
                                    break;
                                }
                            }
                            if (result && _virtual_disk_infos.empty()){
                                _virtual_disk_infos = get_virtual_disk_infos(vmdk_rws);
                            }
                        }

                        if (result)
                        {
                            foreach(vmware_disk_info::ptr& _disk, vm->disks)
                            {
                                std::wstring wsz_disk = _disk->uuid.empty() ? _disk->wsz_key() : _disk->uuid;
                                if (!disks_map.count(wsz_disk))
                                    continue;
                                std::string disk = disks_map[wsz_disk];
                                if (!(result = replicate_disk(vmdk_rws[disk], extents_map[disk], _progress[disk])))
                                {
                                    _state |= mgmt_job_state::job_state_error;
                                    break;
                                }
                            }
                        }
                    }
                    if (result && _create_job_detail.detail.v.disks.size() == _progress.size())
                        _state = mgmt_job_state::job_state_finished;
                }
                catch (macho::exception_base& ex)
                {
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                }
                catch (const boost::filesystem::filesystem_error& ex)
                {
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                }
                catch (const boost::exception &ex)
                {
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                }
                catch (const std::exception& ex)
                {
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                }
                catch (...)
                {
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Unknown exception"));
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                }
                portal_ex.erase_portal(virtual_machine_id);
                portal = NULL;
            }
            save_status();
        }
        catch (macho::exception_base& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::filesystem::filesystem_error& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const boost::exception &ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (const std::exception& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
            _state |= mgmt_job_state::job_state_error;
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Unknown exception"));
            _state |= mgmt_job_state::job_state_error;
        }
    }
}

bool virtual_packer_job::get_vss_reserved_sectors(universal_disk_rw::ptr &rw, mwdc::ironman::hypervisor::changed_disk_extent &extent, bool &bootable_disk){
    FUN_TRACE;
    bool result = false;
    std::auto_ptr<uint8> data_buf(new uint8[GPT_PART_SIZE_SECTORS * VIXDISKLIB_SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)){
        part_table = (PLEGACY_MBR)data_buf.get();
        unsigned __int64 start_lba = ULLONG_MAX;
        
        if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
        {
            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[VIXDISKLIB_SECTOR_SIZE];
            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[VIXDISKLIB_SECTOR_SIZE * 2];

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                if (macho::guid_(gpt_part[i].PartitionTypeGuid) == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")) // EFI System Partition
                {
                    bootable_disk = true;
                    break;
                }
            }

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                if (gpt_part[i].PartitionTypeGuid == PARTITION_SHADOW_COPY_GUID)
                {
                    _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                    LOG(LOG_LEVEL_RECORD, L"GPT/VSS part hit.");
                    extent.start = gpt_part[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE;
                    extent.length = (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * VIXDISKLIB_SECTOR_SIZE;
                    if (start_lba == ULLONG_MAX)
                        start_lba = extent.start;
                    LOG(LOG_LEVEL_DEBUG, "%I64u:%I64u", extent.start, extent.length);
                    break;
                }

                if (gpt_part[i].StartingLBA != 0 && gpt_part[i].StartingLBA < start_lba)
                    start_lba = gpt_part[i].StartingLBA;
            }

            //if (chunk->length == 0)
            //    chunk->start = GPT_PART_SIZE_SECTORS * VIXDISKLIB_SECTOR_SIZE;
        }
        else
        {
            for (int i = 1; i < GPT_PART_SIZE_SECTORS; i++)
            {
                if (!memcmp(data_buf.get() + (i * VIXDISKLIB_SECTOR_SIZE), "SNAPPART", 8))
                {
                    _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                    LOG(LOG_LEVEL_RECORD, L"MBR/VSS part hit.");
                    extent.start = i * VIXDISKLIB_SECTOR_SIZE;
                    extent.length = VIXDISKLIB_SECTOR_SIZE;
                    LOG(LOG_LEVEL_DEBUG, "%I64u:%I64u", extent.start, extent.length);
                    break;
                }
            }
            for (int index = 0; index < 4; index++)
            {
                if (part_table->PartitionRecord[index].BootIndicator)
                {
                    bootable_disk = true;
                    break;
                }
            }
        }
    }
    return result;
}

changed_disk_extent::vtr virtual_packer_job::get_linux_partition_excluded_extents(const std::wstring path, uint64_t start, uint64_t size){
    FUN_TRACE;
    uint64_t end = start + size;
    changed_disk_extent::vtr excluded_extents;
    changed_disk_extent::vtr changes;
    if (_io_ranges_map.count(path)){
        foreach(io_range& r, _io_ranges_map[path]){
            //uint64_t r_end = r.start + r.length;
            if (r.start >= start && r.start < end){
                changes.push_back(changed_disk_extent(r.start, r.length));
            }
            else if (r.start > end)
                break;
        }
        if (changes.size()){
            changed_disk_extent extent;
            int64_t next_start = start;
            foreach(auto s, changes)
            {
                if (next_start != 0 && next_start < s.start)
                {
                    extent.start = next_start;
                    extent.length = s.start - next_start;
                    if (extent.length >= _min_transport_size)
                    {
                        excluded_extents.push_back(extent);
                        LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
                    }
                }
                next_start = s.start + s.length;
            }
            extent.start = next_start;
            extent.length = end - next_start;
            if (extent.length >= _min_transport_size)
            {
                excluded_extents.push_back(extent);
                LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
            }
        }
    }
    return excluded_extents;
}

changed_disk_extent::vtr virtual_packer_job::get_linux_excluded_file_extents(universal_disk_rw::ptr &rw){
    FUN_TRACE;
    changed_disk_extent::vtr excluded_extents;
    bool result = false;
    std::auto_ptr<uint8> data_buf(new uint8[GPT_PART_SIZE_SECTORS * VIXDISKLIB_SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)){
        part_table = (PLEGACY_MBR)data_buf.get();
        if (part_table->Signature != 0xAA55){
            //RAW partition....
        }
        else if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
        {
            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[VIXDISKLIB_SECTOR_SIZE];
            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[VIXDISKLIB_SECTOR_SIZE * 2];

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                macho::guid_ partition_type = macho::guid_(gpt_part[i].PartitionTypeGuid);
                if (macho::guid_(gpt_part[i].PartitionTypeGuid) == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")) // Linux SWAP partition
                {
                    changed_disk_extent extent(gpt_part[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE, (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * VIXDISKLIB_SECTOR_SIZE);
                    _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                    LOG(LOG_LEVEL_RECORD, L"SWAP part hit.");
                    LOG(LOG_LEVEL_RECORD, "%I64u:%I64u", extent.start, extent.length);
                    excluded_extents.push_back(extent);
                }
                else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                    partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                    partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                    partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                    partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                    partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928") || // Logical Volume Manager (LVM) partition
                    partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                    )
                {
                    changed_disk_extent::vtr _excluded_extents = get_linux_partition_excluded_extents(rw->path(), gpt_part[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE, (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * VIXDISKLIB_SECTOR_SIZE);
                    if (_excluded_extents.size()){
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                        excluded_extents.insert(excluded_extents.end(), _excluded_extents.begin(), _excluded_extents.end());
                    }
                }
            }
        }
        else
        {
            for (int index = 0; index < 4; index++)
            {
                uint64_t start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * VIXDISKLIB_SECTOR_SIZE;
                if (part_table->PartitionRecord[index].PartitionType == 82 && part_table->PartitionRecord[index].SizeInLBA != 0) // Linux SWAP partition
                {
                    changed_disk_extent extent(start, part_table->PartitionRecord[index].SizeInLBA * VIXDISKLIB_SECTOR_SIZE);
                    _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                    LOG(LOG_LEVEL_RECORD, L"SWAP part hit.");
                    LOG(LOG_LEVEL_RECORD, "%I64u:%I64u", extent.start, extent.length);
                    excluded_extents.push_back(extent);
                }
                if (part_table->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                    changed_disk_extent::vtr _excluded_extents = get_linux_partition_excluded_extents(rw->path(), start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * VIXDISKLIB_SECTOR_SIZE));
                    if (_excluded_extents.size()){
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                        excluded_extents.insert(excluded_extents.end(), _excluded_extents.begin(), _excluded_extents.end());
                    }
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM){
                    changed_disk_extent::vtr _excluded_extents = get_linux_partition_excluded_extents(rw->path(), start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * VIXDISKLIB_SECTOR_SIZE));
                    if (_excluded_extents.size()){
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                        excluded_extents.insert(excluded_extents.end(), _excluded_extents.begin(), _excluded_extents.end());
                    }
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                    part_table->PartitionRecord[index].PartitionType == PARTITION_EXTENDED)
                {
                    std::string buf2;
                    bool loop = true;
                    while (loop)
                    {
                        loop = false;
                        if (rw->read(start, VIXDISKLIB_SECTOR_SIZE, buf2))
                        {
                            PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                            if (pMBR->Signature != 0xAA55)
                                break;
                            for (int i = 0; i < 2; i++)
                            {
                                if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                    pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED)
                                {
                                    start = ((uint64_t)((uint64_t)part_table->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * VIXDISKLIB_SECTOR_SIZE);
                                    loop = true;
                                    break;
                                }
                                else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0)
                                {
                                    if (pMBR->PartitionRecord[i].PartitionType == 82)// Linux SWAP partition
                                    {
                                        changed_disk_extent extent(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE), pMBR->PartitionRecord[i].SizeInLBA * VIXDISKLIB_SECTOR_SIZE);
                                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                                        LOG(LOG_LEVEL_RECORD, L"SWAP part hit.");
                                        LOG(LOG_LEVEL_RECORD, "%I64u:%I64u", extent.start, extent.length);
                                        excluded_extents.push_back(extent);
                                    }
                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                        changed_disk_extent::vtr _excluded_extents = get_linux_partition_excluded_extents(rw->path(), start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE), pMBR->PartitionRecord[i].SizeInLBA * VIXDISKLIB_SECTOR_SIZE);
                                        if (_excluded_extents.size()){
                                            _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                                            excluded_extents.insert(excluded_extents.end(), _excluded_extents.begin(), _excluded_extents.end());
                                        }
                                    }
                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM){
                                        changed_disk_extent::vtr _excluded_extents = get_linux_partition_excluded_extents(rw->path(), start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * VIXDISKLIB_SECTOR_SIZE), pMBR->PartitionRecord[i].SizeInLBA * VIXDISKLIB_SECTOR_SIZE);
                                        if (_excluded_extents.size()){
                                            _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_LINUX;
                                            excluded_extents.insert(excluded_extents.end(), _excluded_extents.begin(), _excluded_extents.end());
                                        }
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return excluded_extents;
}

changed_disk_extent::vtr virtual_packer_job::get_windows_excluded_file_extents(universal_disk_rw::ptr &rw, bool &system_disk){
    FUN_TRACE;
    changed_disk_extent::vtr excluded_extents;
    system_disk = false;
    ntfs::volume::vtr volumes = ntfs::volume::get(rw);
    if (volumes.size()){
        foreach(ntfs::volume::ptr v, volumes)
        {
            ntfs::file_record::ptr root = v->root();
            if (root)
            {
                ntfs::file_extent::vtr exts;
                if (_create_job_detail.file_system_filter_enable)
                {
                    ntfs::file_record::ptr pagefile = v->find_sub_entry(L"pagefile.sys");
                    if (pagefile)
                    {
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                        LOG(LOG_LEVEL_RECORD, L"pagefile hit.");
                        exts = pagefile->extents();

                        foreach(auto e, exts)
                        {
                            changed_disk_extent extent(e.physical, e.length);
                            excluded_extents.push_back(extent);
                            LOG(LOG_LEVEL_DEBUG, "p/%I64u:%I64u", extent.start, extent.length);
                        }
                    }

                    ntfs::file_record::ptr hiberfile = v->find_sub_entry(L"hiberfil.sys");
                    if (hiberfile)
                    {
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                        LOG(LOG_LEVEL_RECORD, L"hiberfile hit.");
                        exts = hiberfile->extents();

                        foreach(auto e, exts)
                        {
                            changed_disk_extent extent(e.physical, e.length);
                            excluded_extents.push_back(extent);
                            LOG(LOG_LEVEL_DEBUG, "h/%I64u:%I64u", extent.start, extent.length);
                        }
                    }

                    ntfs::file_record::ptr swapfile = v->find_sub_entry(L"swapfile.sys");
                    if (swapfile)
                    {
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                        LOG(LOG_LEVEL_RECORD, L"swapfile hit.");
                        exts = swapfile->extents();
                        foreach(auto e, exts)
                        {
                            changed_disk_extent extent(e.physical, e.length);
                            excluded_extents.push_back(extent);
                            LOG(LOG_LEVEL_DEBUG, "h/%I64u:%I64u", extent.start, extent.length);
                        }
                    }
                }

                ntfs::file_record::ptr windows = v->find_sub_entry(L"windows");
                if (windows)
                {
                    ntfs::file_record::ptr system32 = v->find_sub_entry(L"system32", windows);
                    if (system32)
                    {
                        system_disk = true;
                        _guest_os_type = saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS;
                    }
                    if (_create_job_detail.file_system_filter_enable)
                    {
                        ntfs::file_record::ptr memorydump = v->find_sub_entry(L"memory.dmp", windows);
                        ULONGLONG size = 0;
                        if (memorydump)
                        {
                            LOG(LOG_LEVEL_RECORD, L"dumpfile hit.");
                            exts = memorydump->extents();
                            foreach(auto e, exts)
                            {
                                changed_disk_extent extent(e.physical, e.length);
                                excluded_extents.push_back(extent);
                                LOG(LOG_LEVEL_DEBUG, "d/%I64u:%I64u", extent.start, extent.length);
                            }
                        }
                    }
                }
                
                if (_create_job_detail.file_system_filter_enable)
                {
                    io_range::vtr vol_data = v->file_system_ranges();
                    if (vol_data.size())
                    {
                        changed_disk_extent extent;
                        int64_t next_start = v->start();
                        foreach(auto s, vol_data)
                        {
                            if (next_start != 0 && next_start < s.start)
                            {
                                extent.start = next_start;
                                extent.length = s.start - next_start;
                                if (extent.length >= _min_transport_size)
                                {
                                    excluded_extents.push_back(extent);
                                    LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
                                }
                            }
                            next_start = s.start + s.length;
                        }
                        extent.start = next_start;
                        extent.length = v->start() + v->length() - next_start;
                        if (extent.length >= _min_transport_size)
                        {
                            excluded_extents.push_back(extent);
                            LOG(LOG_LEVEL_DEBUG, "vx/%I64u:%I64u", extent.start, extent.length);
                        }
                    }
                }
            }
        }
    }
    if (_create_job_detail.file_system_filter_enable)
        return excluded_extents;
    else
        return changed_disk_extent::vtr();
}

changed_disk_extent::vtr virtual_packer_job::final_extents(const changed_disk_extent::vtr& src, changed_disk_extent::vtr& excludes)
{
    FUN_TRACE;
    changed_disk_extent::vtr chunks;
    if (src.empty())
        return src;
    else if (excludes.empty())
        chunks = src;
    else
    {
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
                    it++;
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
                            changed_disk_extent chunk;

                            if (it->start - start > 0)
                            {
                                chunk.start = start;
                                chunk.length = it->start - start;
                                chunks.push_back(chunk);
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
                                    chunks.push_back(chunk);
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
        if (s.length <= _block_size)
            _chunks.push_back(s);
        else{
            UINT64 length = s.length;
            UINT64 next_start = s.start;
            while (length > _block_size){
                _chunks.push_back(changed_disk_extent(next_start, _block_size));
                length -= _block_size;
                next_start += _block_size;
            }
            _chunks.push_back(changed_disk_extent(next_start, length));
        }
    }
    std::sort(_chunks.begin(), _chunks.end(), [](changed_disk_extent const& lhs, changed_disk_extent const& rhs){ return lhs.start < rhs.start; });
    return _chunks;
}

bool virtual_packer_job::calculate_disk_backup_size(universal_disk_rw::ptr &rw, changed_disk_extent::vtr& extents, uint64_t& backup_size, bool &bootable_disk, bool &system_disk)
{
    FUN_TRACE;
    bool result = false;
    uint64_t _backup_size = 0;
    std::string path = macho::stringutils::convert_unicode_to_utf8(boost::filesystem::path(rw->path()).filename().wstring());
    try{
        changed_disk_extent vss_reserved_sectors;
        
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Calculating backup size of disk %1%.") % path));
        if (result = get_vss_reserved_sectors(rw, vss_reserved_sectors, bootable_disk)){
            _backup_size = 0;
            foreach(auto c, extents)
                _backup_size += c.length;
            LOG(LOG_LEVEL_RECORD, L"Before excluding, Delta Size for disk(%s): %I64u", rw->path().c_str(), _backup_size);
            changed_disk_extent::vtr linux_excluded_extents = get_linux_excluded_file_extents(rw);
            changed_disk_extent::vtr excluded_extents = get_windows_excluded_file_extents(rw, system_disk);
            if (vss_reserved_sectors.length > 0)
                excluded_extents.insert(excluded_extents.begin(), vss_reserved_sectors);
            if (linux_excluded_extents.size())
                excluded_extents.insert(excluded_extents.end(), linux_excluded_extents.begin(), linux_excluded_extents.end());
            extents = final_extents(extents, excluded_extents);
            _backup_size = 0;
            foreach(auto c, extents)
                _backup_size += c.length;
            LOG(LOG_LEVEL_RECORD, L"After excluding, Delta Size for disk(%s): %I64u", rw->path().c_str(), _backup_size);
            if (backup_size == 0){
                backup_size = _backup_size;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Backup size estimate of disk %1% : %2%.") % path % backup_size));
            }
        }
        else{
            _state |= mgmt_job_state::job_state_error;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot calculate backup size of disk %1%.") % path));
            backup_size = 0;
        }
        save_status();
    }
    catch (universal_disk_rw::exception& ex){
        _state |= mgmt_job_state::job_state_error;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot calculate backup size of disk %1%.") % path));
        backup_size = 0;
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(macho::stringutils::convert_unicode_to_utf8(get_diagnostic_information(ex))).c_str());
    }
    return result;
}

bool virtual_packer_job::replicate_disk(universal_disk_rw::ptr &rw, changed_disk_extent::vtr& extents, virtual_transport_job_progress::ptr& p)
{
    FUN_TRACE;
    bool result = false;
    if (p->total_size == p->backup_image_offset)
        return true;
    int32_t retry_count = 1;
    std::string vmdkfile = macho::stringutils::convert_unicode_to_utf8(boost::filesystem::path(rw->path()).filename().wstring());  
    do
    {
        _state &= ~mgmt_job_state::job_state_error;
        universal_disk_rw::vtr outputs;
        if (retry_count == 0)
        {
            rw = rw->clone();
            if (!rw)
            {
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Cannot open vmdk disk %1%.") % vmdkfile));
                break;
            }
        }
       
        try
        {
            if (p->backup_image_offset > 0)
                outputs = carrier_rw::open(_create_job_detail.carriers, p->base, p->output, _create_job_detail.detail.v.snapshot, _create_job_detail.timeout, _create_job_detail.is_encrypted, false);
            else{
                carrier_rw::create_image_parameter parameter;
                parameter.carriers = _create_job_detail.carriers;
                parameter.base_name = p->base;
                parameter.name = p->output;
                parameter.size = p->total_size;
                parameter.parent = p->parent;
                parameter.checksum_verify = _create_job_detail.checksum_verify;
                parameter.session = _create_job_detail.detail.v.snapshot;
                parameter.timeout = _create_job_detail.timeout;
                parameter.encrypted = _create_job_detail.is_encrypted;
                parameter.comment = vmdkfile;
                parameter.checksum = _create_job_detail.is_checksum;
                parameter.compressed = _create_job_detail.is_compressed;
                parameter.priority_carrier = _create_job_detail.priority_carrier;
                macho::windows::registry reg;
                if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                    if (reg[L"ImageMode"].exists() && reg[L"ImageMode"].is_dword()){
                        parameter.mode = (DWORD)reg[L"ImageMode"];                       
                    }
                }
                parameter.mode = _create_job_detail.is_compressed_by_packer ? 2 : parameter.mode;
                outputs = carrier_rw::create(parameter);
            }
        }
        catch (universal_disk_rw::exception &e)
        {
            _state |= mgmt_job_state::job_state_error;
            outputs.clear();
        }

        if (0 == outputs.size())
        {
            if (p->backup_image_offset > 0)
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, (record_format("Cannot open disk image %1%.") % p->output));
            else
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, (record_format("Cannot create disk image %1%.") % p->output));
            save_status();
        }
        else
        {
            //retry_count = 1;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup started.") % vmdkfile));
            int                    worker_thread_number = _create_job_detail.worker_thread_number ? _create_job_detail.worker_thread_number : 4;
            int					   max_worker_thread_number = worker_thread_number;
            int                    resume_thread_count = RESUME_THREAD_COUNT;
            int                    resume_thread_time = RESUME_THREAD_TIME;
            registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"ResumeThreadCount"].exists() && (DWORD)reg[L"ResumeThreadCount"] > 0){
                    resume_thread_count = (DWORD)reg[L"ResumeThreadCount"];
                }
                if (reg[L"ResumeThreadTime"].exists() && (DWORD)reg[L"ResumeThreadTime"] > 0){
                    resume_thread_time = (DWORD)reg[L"ResumeThreadTime"];
                }
            }
            replication_repository repository(*p);
            repository.compressed_by_packer = _create_job_detail.is_compressed_by_packer;
            uint64_t index = 0;
            foreach(auto e, extents)
            {
                if ( e.start >= p->backup_image_offset)
                    repository.waitting.push_back(replication_block::ptr(new replication_block(index++, e.start, e.length)));
            }
            if (repository.waitting.size())
            {
                while (worker_thread_number > 0)
                {
                    boost::thread_group    thread_pool;
                    replication_task::vtr  tasks;
                    repository.terminated = false;
                    _state &= ~mgmt_job_state::job_state_error;

                    if (worker_thread_number > repository.waitting.size())
                        worker_thread_number = repository.waitting.size();

                    for (int i = 0; i < worker_thread_number; i++)
                    {
                        universal_disk_rw::ptr in;
                        universal_disk_rw::vtr outs;
                        if (i == 0)
                        {
                            in = rw;
                            outs = outputs;
                        }
                        else
                        {
                            foreach(auto o, outputs)
                                outs.push_back(o->clone());
#ifdef SIGNAL_THREAD_READ
                            in = rw;
#else
                            in = rw->clone();
#endif
                        }
                        if (in && outs.size())
                        {
                            replication_task::ptr task(new replication_task(in, outs, repository));
                            task->register_job_is_canceled_function(boost::bind(&virtual_packer_job::is_canceled, this));
                            task->register_job_save_callback_function(boost::bind(&virtual_packer_job::save_status, this));
                            task->register_job_record_callback_function(boost::bind(&virtual_packer_job::record, this, _1, _2, _3));
                            tasks.push_back(task);
                        }
                    }

                    LOG(LOG_LEVEL_RECORD, L"Number of running threads is %d. Waitting list size (%d)", tasks.size(), repository.waitting.size());
                    for (int i = 0; i < tasks.size(); i++)
                        thread_pool.create_thread(boost::bind(&replication_task::replicate, &(*tasks[i])));

                    boost::unique_lock<boost::mutex> _lock(repository.completion_lock);
                    int waitting_count = 0;
                    while (true)
                    {
                        repository.completion_cond.timed_wait(_lock, boost::posix_time::milliseconds(60000));
                        {
                            macho::windows::auto_lock lock(repository.cs);
                            if (repository.error.size() != 0)
                            {
                                _state |= mgmt_job_state::job_state_error;
                                repository.terminated = true;
                                resume_thread_count--;
                                break;
                            }
                            else if ((repository.processing.size() == 0 && repository.waitting.size() == 0) || is_canceled())
                            {
                                repository.terminated = true;
                                break;
                            }
                            else if (resume_thread_count && max_worker_thread_number > worker_thread_number && repository.waitting.size() > worker_thread_number){
                                if (waitting_count >= resume_thread_time){
                                    universal_disk_rw::ptr in;
                                    universal_disk_rw::vtr outs;
                                    foreach(auto o, outputs)
                                        outs.push_back(o->clone());
#ifdef SIGNAL_THREAD_READ
                                    in = rw;
#else
                                    in = rw->clone();
#endif
                                    if (in && outs.size())
                                    {
                                        replication_task::ptr task(new replication_task(in, outs, repository));
                                        task->register_job_is_canceled_function(boost::bind(&virtual_packer_job::is_canceled, this));
                                        task->register_job_save_callback_function(boost::bind(&virtual_packer_job::save_status, this));
                                        task->register_job_record_callback_function(boost::bind(&virtual_packer_job::record, this, _1, _2, _3));
                                        tasks.push_back(task);
                                        thread_pool.create_thread(boost::bind(&replication_task::replicate, &(*tasks[tasks.size()-1])));
                                        worker_thread_number++;
                                        waitting_count = 0;
                                    }
                                }
                                waitting_count++;
                            }
                        }
                    }
                    thread_pool.join_all();
                    worker_thread_number--;
                    if ((repository.error.size() == 0 && repository.processing.size() == 0 && repository.waitting.size() == 0) || is_canceled()){
                        break;
                    }
                    else if (worker_thread_number){
                        foreach(replication_block::ptr& b, repository.processing){
                            repository.waitting.push_front(b);
                        }
                        repository.processing.clear();
                        repository.error.clear();
                        std::sort(repository.waitting.begin(), repository.waitting.end(), replication_block::compare());
                        boost::this_thread::sleep(boost::posix_time::seconds(5));
                    }
                    else {
                        break;
                    }
                }
            }
            if (result = (repository.error.size() == 0 && repository.processing.size() == 0 && repository.waitting.size() == 0))
            {
                macho::windows::auto_lock lock(repository.progress.lock);
                repository.progress.backup_image_offset = repository.progress.total_size;
                foreach(universal_disk_rw::ptr o, outputs){
                    carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                    if (rw) 
                    {
                        if (!(result = rw->close()))
                        {
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % vmdkfile));
                            _state |= mgmt_job_state::job_state_error;
                            break;
                        }
                    }
                }
            }
            else
            {
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % vmdkfile));
            }
            if (result)
            {
                _state &= ~mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup completed with size : %2%") % vmdkfile % p->backup_size));
                break;
            }
        }
    } while ((!is_canceled()) && ((retry_count--) > 0));
    save_status();
    return result;
}

std::vector<saasame::transport::virtual_disk_info_ex> virtual_packer_job::get_virtual_disk_infos(std::map<std::string, universal_disk_rw::ptr> &vmdk_rws){
    typedef std::map<std::string, universal_disk_rw::ptr> universal_disk_rw_map;
    std::vector<saasame::transport::virtual_disk_info_ex> disks;
    std::string buf;
    linuxfs::volume::vtr results;
    foreach(universal_disk_rw_map::value_type& _rw, vmdk_rws){
        universal_disk_rw::ptr rw = _rw.second;
        if (rw->read(0, rw->sector_size() * 34, buf)){
            saasame::transport::virtual_disk_info_ex disk;
            disk.id = _rw.first;
            disk.size = _progress[disk.id]->total_size;
            int partition_number = 0;
            PLEGACY_MBR                pLegacyMBR = (PLEGACY_MBR)&buf[0];
            PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
            if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
                PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&buf[rw->sector_size() * pGptPartitonHeader->PartitionEntryLBA];
                disk.partition_style = saasame::transport::partition_style::PARTITION_GPT;
                disk.guid = macho::guid_(pGptPartitonHeader->DiskGUID);		
                for (int index = 0; index < pGptPartitonHeader->NumberOfPartitionEntries; index++){
                    if (pGptPartitionEntries[index].StartingLBA && pGptPartitionEntries[index].EndingLBA){
                        partition_number++;
                        macho::guid_ partition_type = macho::guid_(pGptPartitionEntries[index].PartitionTypeGuid);
                        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition     
                        else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                            partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                            partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                            partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                            partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                            partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                            )
                        {
                            linuxfs::volume::ptr v = linuxfs::volume::get(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size(), (pGptPartitionEntries[index].EndingLBA - pGptPartitionEntries[index].StartingLBA + 1) * rw->sector_size());
                            if (v){
                                virtual_partition_info part;
                                part.offset = ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size();
                                part.size = (pGptPartitionEntries[index].EndingLBA - pGptPartitionEntries[index].StartingLBA + 1) * rw->sector_size();
                                part.partition_number = partition_number;
                                disk.partitions.insert(part);
                            }
                            else{
                                ntfs::volume::ptr _v = ntfs::volume::get(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size());
                                if (_v){
                                    virtual_partition_info part;
                                    part.offset = ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size();
                                    part.size = (pGptPartitionEntries[index].EndingLBA - pGptPartitionEntries[index].StartingLBA + 1) * rw->sector_size();
                                    part.partition_number = partition_number;
                                    disk.partitions.insert(part);
                                }
                            }
                        }
                        else if (partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                        {
                        }
                    }
                }
            }
            else{
                disk.partition_style = saasame::transport::partition_style::PARTITION_MBR;
                disk.signature = pLegacyMBR->UniqueMBRSignature;
                for (int index = 0; index < 4; index++){
                    uint64_t start = ((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (start && ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size())){
                        partition_number++;
                        if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_IFS){
                            ntfs::volume::ptr v = ntfs::volume::get(rw, start);
                            if (v){
                                virtual_partition_info part;
                                part.offset = start;
                                part.size = ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size());
                                part.partition_number = partition_number;
                                disk.partitions.insert(part);
                            }
                        }
                        else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                            linuxfs::volume::ptr v = linuxfs::volume::get(rw, start, ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size()));
                            if (v){
                                virtual_partition_info part;
                                part.offset = start;
                                part.size = ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size());
                                part.partition_number = partition_number;
                                disk.partitions.insert(part);
                            }
                        }
                        else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM){
                        }
                        else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                            pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXTENDED){
                            std::string buf2;
                            bool loop = true;
                            while (loop){
                                loop = false;
                                if (rw->read(start, rw->sector_size(), buf2)){
                                    PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                                    if (pMBR->Signature != 0xAA55)
                                        break;
                                    for (int i = 0; i < 2; i++){
                                        if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                            pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                            start = ((uint64_t)((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
                                            loop = true;
                                            break;
                                        }
                                        else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                                            partition_number++;
                                            if (pMBR->PartitionRecord[i].PartitionType == PARTITION_IFS){
                                                ntfs::volume::ptr v = ntfs::volume::get(rw, start);
                                                if (v){
                                                    virtual_partition_info part;
                                                    part.offset = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                                    part.size = ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size());
                                                    part.partition_number = partition_number;
                                                    disk.partitions.insert(part);
                                                }
                                            }
                                            else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                                linuxfs::volume::ptr v = linuxfs::volume::get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()));
                                                if (v){
                                                    virtual_partition_info part;
                                                    part.offset = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                                    part.size = ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size());
                                                    part.partition_number = partition_number;
                                                    disk.partitions.insert(part);
                                                }
                                            }
                                            else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM){
                                            }
                                        }
                                        else{
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (disk.partitions.size()){
                disk.__set_partitions(disk.partitions);
                foreach(std::string id, _system_disks){
                    if (disk.id == id){
                        disk.is_system = true;
                        break;
                    }
                }
                disks.push_back(disk);
            }
        }
    }
    return disks;
}