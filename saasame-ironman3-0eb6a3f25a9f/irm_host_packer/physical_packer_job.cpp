#include "physical_packer_job.h"
#include "common_service_handler.h"
#include "management_service.h"
#include <codecvt>
#include "universal_disk_rw.h"
#include "temp_drive_letter.h"
#include "carrier_rw.h"
#include "..\irm_converter\irm_disk.h"
#include "..\vcbt\vcbt\journal.h"
#include "..\vcbt\vcbt\umap.h"
#include "..\vcbt\vcbt\fatlbr.h"
#include <VersionHelpers.h>
#include <thrift/transport/TSSLSocket.h>
#include "..\ntfs_parser\ntfs_parser.h"
#ifndef _WIN2K3
#include "lz4.h"
#include "lz4hc.h"
#pragma comment(lib, "liblz4.lib" )
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

typedef std::map<std::string, std::vector<io_changed_range>> io_changed_ranges_map;

#define MINI_BLOCK_SIZE         65536UL
#define MAX_BLOCK_SIZE          8388608UL

#ifndef _WIN2K3
int _compress(__in const char* source, __inout char* dest, __in int source_size, __in int max_compressed_size){
    return LZ4_compress_fast(source, dest, source_size, max_compressed_size, 1);
}
#endif

static replication_block::map _get_dynamic_volumes_replication_blocks(storage::volume& v, const changed_area::vtr& changeds, uint32_t block_size, std::string volume_path);

struct replication_repository{
    typedef boost::shared_ptr<replication_repository> ptr;
    replication_repository(physical_packer_job_progress& _disk_progress, uint64_t& _progress, uint64_t _target = 0) :
        offset(_disk_progress.backup_image_offset),
        disk_progress(_disk_progress),
        progress(_progress), 
        target(_target), 
        terminated(false),
        compressed_by_packer(false),
        block_size(CARRIER_RW_IMAGE_BLOCK_SIZE), 
        queue_size(CARRIER_RW_IMAGE_BLOCK_SIZE * 4){}
    macho::windows::critical_section                          cs;
    replication_block::queue                                  waitting;
    replication_block::vtr                                    processing;
    replication_block::queue                                  error;
    boost::mutex                                              completion_lock;
    boost::condition_variable                                 completion_cond;
    boost::mutex                                              pending_lock;
    boost::condition_variable                                 pending_cond;
    physical_packer_job_progress&                             disk_progress;
    uint64_t&                                                 offset;
    uint64_t&                                                 progress;
    uint32_t                                                  block_size;
    uint32_t                                                  queue_size;
    bool                                                      terminated;
    uint64_t                                                  target;
    bool                                                      compressed_by_packer;
    std::map<std::string, universal_disk_rw::ptr>             ins;
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
    universal_disk_rw::ptr                                      get_in_ptr(boost::shared_ptr<std::string> &path){
        universal_disk_rw::ptr _in;
        if (!path)
            return in;
        if (repository.ins.count(*path) && repository.ins[*path])
            _in = repository.ins[*path];
        else if (_in = general_io_rw::open(*path)){
            repository.ins[*path] = _in;
        }
        return _in;
    }
};

void replication_task::replicate()
{
    bool                             result = true;
    std::string                      buf;
    std::string                      compressed_buf;
    replication_block::ptr           block;
    bool                             pendding = false;
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
    while (true){
        universal_disk_rw::ptr _in;
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
                            macho::windows::auto_lock progress_lock(repository.disk_progress.lock);
                            repository.disk_progress.completed.push_back(block);
                            std::sort(repository.disk_progress.completed.begin(), repository.disk_progress.completed.end(), replication_block::compare());
                            while (repository.disk_progress.completed.size() && repository.error.size() == 0){
                                if (0 == repository.processing.size() || repository.disk_progress.completed.front()->End() < repository.processing[0]->Start()){
                                    block = repository.disk_progress.completed.front();
                                    repository.disk_progress.completed.pop_front();
                                    repository.offset = repository.target + block->End();
                                    repository.progress += block->Length();
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
            foreach(replication_block::ptr& b, repository.disk_progress.completed){
                if (b->RealStart() == block->RealStart() && b->Length() == block->Length()){
                    LOG(LOG_LEVEL_INFO, _T("Skip completed block( %I64u, %I64u)"), block->RealStart(), block->Length());
                    if (0 == repository.waitting.size()){
                        block = NULL;
                        break;
                    }
                    block = repository.waitting.front();
                    repository.waitting.pop_front();
                }
                else if ((b->RealStart()) > block->RealEnd()){
                    LOG(LOG_LEVEL_INFO, _T("b->RealStart() > block->RealEnd()(%I64u > %I64u)"), b->RealStart(), block->RealEnd());
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
            _in = get_in_ptr(block->path);
        }
        repository.pending_cond.notify_all();
        uint32_t number_of_bytes_to_read = 0;
        uint32_t number_of_bytes_to_written = 0;
        if (!_in)
            result = false;
        else if (result = _in->read(block->Start(), block->length, buf)){
            number_of_bytes_to_read = buf.length();
#ifndef _WIN2K3
            if (repository.compressed_by_packer){
                std::string* out = &buf;
                uint32_t compressed_length;
                bool compressed = false;
                compressed_buf.resize(number_of_bytes_to_read);
                compressed_length = _compress(reinterpret_cast<const char*>(buf.c_str()), reinterpret_cast<char*>(&compressed_buf[0]), number_of_bytes_to_read, number_of_bytes_to_read);
                if (compressed_length != 0){
                    if (compressed_length < number_of_bytes_to_read){
                        compressed_buf.resize(compressed_length);
                        out = &compressed_buf;
                        compressed = true;
                    }
                }
                while (pendding && !repository.terminated)
                {
                    {
                        macho::windows::auto_lock lock(repository.cs);
                        if (block->RealStart() - repository.processing[0]->RealStart() < repository.queue_size)
                            break;
                    }
                    if (!repository.terminated){
                        boost::unique_lock<boost::mutex> lock(repository.pending_lock);
                        repository.pending_cond.timed_wait(lock, boost::posix_time::milliseconds(30000));
                    }
                }
                uint64_t target_start = block->RealStart();
                foreach(carrier_rw* o, rws){
                    if (!(result = o->write_ex(target_start, *out, number_of_bytes_to_read, compressed, number_of_bytes_to_written))){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), target_start, number_of_bytes_to_read);
                        code = saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE;
                        message = boost::str(boost::wformat(L"Cannot write( %1%, %2%)") % target_start % number_of_bytes_to_read);
                        break;
                    }
                }
            }
            else
#endif
            {
                while (pendding && !repository.terminated)
                {
                    {
                        macho::windows::auto_lock lock(repository.cs);
                        if (block->RealStart() - repository.processing[0]->RealStart() < repository.queue_size)
                            break;
                    }
                    if (!repository.terminated){
                        boost::unique_lock<boost::mutex> lock(repository.pending_lock);
                        repository.pending_cond.timed_wait(lock, boost::posix_time::milliseconds(30000));
                    }
                }
                uint64_t target_start = block->RealStart();
                foreach(carrier_rw* o, rws){
                    bool need_wait_for_queue = true;
                    while (need_wait_for_queue){
                        try{
                            need_wait_for_queue = false;
                            result = false;
                            if (!(result = o->write(target_start, buf, number_of_bytes_to_written))){
                                LOG(LOG_LEVEL_ERROR, _T("Cannot write( %I64u, %d)"), target_start, number_of_bytes_to_read);
                                code = saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE;
                                message = boost::str(boost::wformat(L"Cannot write( %1%, %2%)") % target_start % number_of_bytes_to_read);
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

physical_packer_job::physical_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail) 
    : _is_vcbt_enabled(false)
    , _state(job_state_none)
    , _create_job_detail(detail)
    , _is_interrupted(false)
    , _is_canceled(false)
    , removeable_job(id, macho::guid_(GUID_NULL))
    , _block_size(IsWindowsVistaOrGreater() && !g_is_pooling_mode ? 1024 * 1024 * 32 : 1024 * 1024 * 4)
    , _mini_block_size(1024 * 1024)
    , _delta_mini_block_size(MINI_BLOCK_SIZE)
    , _full_mini_block_size(MAX_BLOCK_SIZE)
    , _queue_size(g_is_pooling_mode ? 1024 * 1024 * 16 : CARRIER_RW_IMAGE_BLOCK_SIZE * 4)
    , _guest_os_type(saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _cdr_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.cdr") % _id % JOB_EXTENSION);
}

physical_packer_job::physical_packer_job(saasame::transport::create_packer_job_detail detail) 
    : _is_vcbt_enabled(false)
    , _state(job_state_none)
    , _create_job_detail(detail)
    , _is_interrupted(false)
    , _is_canceled(false)
    , removeable_job(macho::guid_::create()
    , macho::guid_(GUID_NULL))
    , _block_size(IsWindowsVistaOrGreater() && !g_is_pooling_mode ? 1024 * 1024 * 32 : 1024 * 1024 * 4)
    , _mini_block_size(1024 * 1024)
    , _delta_mini_block_size(MINI_BLOCK_SIZE)
    , _full_mini_block_size(MAX_BLOCK_SIZE)
    , _queue_size(g_is_pooling_mode ? 1024 * 1024 * 16 : CARRIER_RW_IMAGE_BLOCK_SIZE * 4)
    , _guest_os_type(saasame::transport::hv_guest_os_type::type::HV_OS_WINDOWS){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _cdr_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.cdr") % _id % JOB_EXTENSION);
}

physical_packer_job::ptr physical_packer_job::create(std::string id, saasame::transport::create_packer_job_detail detail){
    FUN_TRACE;
    physical_packer_job::ptr job;
    if (detail.type == saasame::transport::job_type::physical_packer_job_type){
        // Physical packer type
        job = physical_packer_job::ptr(new physical_packer_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    else if (detail.type == saasame::transport::job_type::winpe_packer_job_type){
        job = physical_packer_job::ptr(new winpe_packer_job(macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    return job;
}

physical_packer_job::ptr physical_packer_job::create(saasame::transport::create_packer_job_detail detail){
    FUN_TRACE;
    physical_packer_job::ptr job;
    if (detail.type == saasame::transport::job_type::physical_packer_job_type){
        // Physical packer type
        job = physical_packer_job::ptr(new physical_packer_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    else if (detail.type == saasame::transport::job_type::winpe_packer_job_type){
        job = physical_packer_job::ptr(new winpe_packer_job(detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
    }
    return job;
}

physical_packer_job::ptr physical_packer_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file){
    FUN_TRACE;
    physical_packer_job::ptr job;
    std::wstring id;
    saasame::transport::create_packer_job_detail _create_job_detail = load_config(config_file.wstring(), id);
    if (_create_job_detail.type == saasame::transport::job_type::physical_packer_job_type){
        // Physical packer type
        job = physical_packer_job::ptr(new physical_packer_job(id, _create_job_detail));
        job->load_status(status_file.wstring());
    }
    else if (_create_job_detail.type == saasame::transport::job_type::winpe_packer_job_type){
        job = physical_packer_job::ptr(new winpe_packer_job(id, _create_job_detail));
        job->load_status(status_file.wstring());
    }
    return job;
}

void physical_packer_job::remove(){
    FUN_TRACE;
    LOG(LOG_LEVEL_RECORD, L"Job remove event captured.");
    _is_removing = true;
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        boost::filesystem::remove(_cdr_file);
        _running.unlock();
    }
}

saasame::transport::create_packer_job_detail physical_packer_job::load_config(std::wstring config_file, std::wstring &job_id){
    FUN_TRACE;
    saasame::transport::create_packer_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mObject job_config_obj = job_config.get_obj();
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(job_config_obj, "id").get_str()); 
        detail.type = (saasame::transport::job_type::type)find_value(job_config_obj, "type").get_int();
        mArray  disks = find_value(job_config_obj, "disks").get_array();
        foreach(mValue d, disks){
            detail.detail.p.disks.insert(d.get_str());
        }

        mArray  connection_ids = find_value(job_config_obj, "connection_ids").get_array();
        foreach(mValue i, connection_ids){
            detail.connection_ids.insert(i.get_str());
        }

        mArray  carriers = find_value(job_config_obj, "carriers").get_array();
        foreach(mValue c, carriers){
            std::string connection_id = find_value_string(c.get_obj(), "connection_id");
            mArray  carrier_addr = find_value(c.get_obj(), "carrier_addr").get_array();
            std::set<std::string> _addr;
            foreach(mValue a, carrier_addr){
                _addr.insert(a.get_str());
            }
            detail.carriers[connection_id] = _addr;
        }

        mArray  snapshots = find_value(job_config_obj, "snapshots").get_array();
        foreach(mValue s, snapshots){
            snapshot snap;
            snap.creation_time_stamp = find_value_string(s.get_obj(), "creation_time_stamp");
            snap.original_volume_name = find_value_string(s.get_obj(), "original_volume_name");
            snap.snapshots_count = find_value(s.get_obj(), "snapshots_count").get_int();
            snap.snapshot_device_object = find_value_string(s.get_obj(), "snapshot_device_object");
            snap.snapshot_id = find_value_string(s.get_obj(), "snapshot_id");
            snap.snapshot_set_id = find_value_string(s.get_obj(), "snapshot_set_id");
            detail.detail.p.snapshots.push_back(snap);
        }

        mArray  previous_journals = find_value(job_config_obj, "previous_journals").get_array();
        foreach(mValue i, previous_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            detail.detail.p.previous_journals[journal.id] = journal;
        }

        mArray  images = find_value(job_config_obj, "images").get_array();
        foreach(mValue i, images){
            packer_disk_image image;
            image.name = find_value_string(i.get_obj(), "name");
            image.parent = find_value_string(i.get_obj(), "parent");      
            image.base = find_value_string(i.get_obj(), "base");
            detail.detail.p.images[find_value_string(i.get_obj(), "key")] = image;
        }

        mArray  backup_size = find_value(job_config_obj, "backup_size").get_array();
        foreach(mValue i, backup_size){  
            detail.detail.p.backup_size[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }
    
        mArray  backup_progress = find_value(job_config_obj, "backup_progress").get_array();
        foreach(mValue i, backup_progress){
            detail.detail.p.backup_progress[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  backup_image_offset = find_value(job_config_obj, "backup_image_offset").get_array();
        foreach(mValue i, backup_image_offset){
            detail.detail.p.backup_image_offset[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  cdr_journals = find_value_array(job_config_obj, "cdr_journals");
        foreach(mValue i, cdr_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            detail.detail.p.cdr_journals[journal.id] = journal;
        }

        mArray  cdr_changed_areas = find_value_array(job_config_obj, "cdr_changed_areas");
        foreach(mValue i, cdr_changed_areas){
            mArray changeds = find_value_array(i.get_obj(), "value");
            std::string key = find_value_string(i.get_obj(), "key");
            foreach(mValue c, changeds){
                io_changed_range changed;
                changed.start = find_value(c.get_obj(), "o").get_int64();
                changed.length = find_value(c.get_obj(), "l").get_int64();
                detail.detail.p.cdr_changed_ranges[key].push_back(changed);
            }
        }

        mArray  completed_blocks = find_value_array(job_config_obj, "completed_blocks");
        foreach(mValue i, completed_blocks){
            mArray completeds = find_value_array(i.get_obj(), "value");
            std::string key = find_value_string(i.get_obj(), "key");
            foreach(mValue c, completeds){
                io_changed_range completed;
                completed.start = find_value(c.get_obj(), "o").get_int64();
                completed.length = find_value(c.get_obj(), "l").get_int64();
                completed.offset = find_value(c.get_obj(), "t").get_int64();
                detail.detail.p.completed_blocks[key].push_back(completed);
            }
        }

        detail.checksum_verify = find_value(job_config_obj, "checksum_verify").get_bool();
        detail.timeout = find_value_int32(job_config_obj, "timeout", 600);
        detail.is_encrypted = find_value_bool(job_config_obj, "is_encrypted");
        detail.worker_thread_number = find_value_int32(job_config_obj, "worker_thread_number");
        detail.file_system_filter_enable = find_value_bool(job_config_obj, "file_system_filter_enable");
        detail.min_transport_size = find_value_int32(job_config_obj, "min_transport_size");
        detail.full_min_transport_size = find_value_int32(job_config_obj, "full_min_transport_size");
        detail.is_compressed = find_value_bool(job_config_obj, "is_compressed");
        detail.is_checksum = find_value_bool(job_config_obj, "is_checksum");
        detail.is_only_single_system_disk = find_value_bool(job_config_obj, "is_only_single_system_disk", false);
        detail.is_compressed_by_packer = find_value_bool(job_config_obj, "is_compressed_by_packer", false);

        if (job_config_obj.end() != job_config_obj.find("priority_carrier")){
            mArray  priority_carrier = find_value_array(job_config_obj, "priority_carrier");
            foreach(mValue c, carriers){
                std::string connection_id = find_value_string(c.get_obj(), "connection_id");
                std::string addr = find_value_string(c.get_obj(), "addr");
                detail.priority_carrier[connection_id] = addr;
            }
        }
        if (job_config_obj.end() != job_config_obj.find("excluded_paths")){
            mArray  excluded_paths = find_value(job_config_obj, "excluded_paths").get_array();
            foreach(mValue p, excluded_paths){
                detail.detail.p.excluded_paths.insert(p.get_str());
            }
        }
        if (job_config_obj.end() != job_config_obj.find("resync_paths")){
            mArray  resync_paths = find_value(job_config_obj, "resync_paths").get_array();
            foreach(mValue p, resync_paths){
                detail.detail.p.resync_paths.insert(p.get_str());
            }
        }
        if (job_config_obj.end() != job_config_obj.find("checksum_target")){
            mArray  checksum_target = find_value(job_config_obj, "checksum_target").get_array();
            foreach(mValue t, checksum_target){
                detail.checksum_target[find_value_string(t.get_obj(), "connection_id")] = find_value_string(t.get_obj(), "machine_id");
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return detail;
}

void physical_packer_job::save_config(){
    FUN_TRACE;
    try{
        using namespace json_spirit;
        mObject job_config;

        job_config["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_config["type"] = _create_job_detail.type;
        mArray  disks(_create_job_detail.detail.p.disks.begin(), _create_job_detail.detail.p.disks.end());
        job_config["disks"] = disks;
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

        mArray snapshots;
        foreach(snapshot &s, _create_job_detail.detail.p.snapshots){
            mObject o;
            o["creation_time_stamp"] = s.creation_time_stamp;
            o["original_volume_name"] = s.original_volume_name;
            o["snapshots_count"] = s.snapshots_count;
            o["snapshot_device_object"] = s.snapshot_device_object;
            o["snapshot_id"] = s.snapshot_id;
            o["snapshot_set_id"] = s.snapshot_set_id;
            snapshots.push_back(o);
        }   
        job_config["snapshots"] = snapshots;

        mArray previous_journals;
        typedef std::map<int64_t, physical_vcbt_journal>  i64_physical_vcbt_journal_map;
        foreach(i64_physical_vcbt_journal_map::value_type &i, _create_job_detail.detail.p.previous_journals){
            mObject o;
            o["journal_id"] = i.second.id;
            o["first_key"] = i.second.first_key;
            o["latest_key"] = i.second.latest_key;
            o["lowest_valid_key"] = i.second.lowest_valid_key;
            previous_journals.push_back(o);
        }
        job_config["previous_journals"] = previous_journals;

        mArray images;
        typedef std::map<std::string, packer_disk_image>  packer_disk_image_map;
        foreach(packer_disk_image_map::value_type &i, _create_job_detail.detail.p.images){
            mObject o;
            o["key"] = i.first;
            o["name"] = i.second.name;
            o["parent"] = i.second.parent;
            o["base"] = i.second.base;
            images.push_back(o);
        }
        job_config["images"] = images;

        typedef std::map<std::string, int64_t>  string_int64_t_map;

        mArray backup_size;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.p.backup_size){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_size.push_back(o);
        }
        job_config["backup_size"] = backup_size;

        mArray backup_progress;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.p.backup_progress){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_progress.push_back(o);
        }
        job_config["backup_progress"] = backup_progress;

        mArray backup_image_offset;
        foreach(string_int64_t_map::value_type &_size, _create_job_detail.detail.p.backup_image_offset){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_image_offset.push_back(o);
        }
        job_config["backup_image_offset"] = backup_image_offset;

        mArray cdr_journals;
        foreach(i64_physical_vcbt_journal_map::value_type &i, _create_job_detail.detail.p.cdr_journals){
            mObject o;
            o["journal_id"] = i.second.id;
            o["first_key"] = i.second.first_key;
            o["latest_key"] = i.second.latest_key;
            o["lowest_valid_key"] = i.second.lowest_valid_key;
            cdr_journals.push_back(o);
        }
        job_config["cdr_journals"] = cdr_journals;

        mArray cdr_changed_areas;
        foreach(io_changed_ranges_map::value_type &c, _create_job_detail.detail.p.cdr_changed_ranges){
            mObject o;
            o["key"] = c.first;
            mArray changeds;
            foreach(io_changed_range& r, c.second){
                mObject changed;
                changed["o"] = r.start;
                changed["l"] = r.length;
                changeds.push_back(changed);
            }
            o["value"] = changeds;
            cdr_changed_areas.push_back(o);
        }
        job_config["cdr_changed_areas"] = cdr_changed_areas;

        mArray completed_blocks;
        foreach(io_changed_ranges_map::value_type &c, _create_job_detail.detail.p.completed_blocks){
            mObject o;
            o["key"] = c.first;
            mArray completeds;
            foreach(io_changed_range& r, c.second){
                mObject completed;
                completed["o"] = r.start;
                completed["l"] = r.length;
                completed["t"] = r.offset;
                completeds.push_back(completed);
            }
            o["value"] = completeds;
            completed_blocks.push_back(o);
        }
        job_config["completed_blocks"] = completed_blocks;

        job_config["checksum_verify"]           = _create_job_detail.checksum_verify;
        job_config["timeout"]                   = _create_job_detail.timeout;
        job_config["is_encrypted"]              = _create_job_detail.is_encrypted;
        job_config["worker_thread_number"]      = _create_job_detail.worker_thread_number;
        job_config["file_system_filter_enable"] = _create_job_detail.file_system_filter_enable;
        job_config["min_transport_size"]        = _create_job_detail.min_transport_size;
        job_config["full_min_transport_size"]   = _create_job_detail.full_min_transport_size;
        job_config["is_compressed"]             = _create_job_detail.is_compressed;
        job_config["is_checksum"]               = _create_job_detail.is_checksum;
        mArray priority_carrier;
        foreach(string_map::value_type &i, _create_job_detail.priority_carrier){
            mObject target;
            target["connection_id"] = i.first;
            target["addr"] = i.second;
            priority_carrier.push_back(target);
        }
        job_config["priority_carrier"] = priority_carrier;
        job_config["is_only_single_system_disk"] = _create_job_detail.is_only_single_system_disk;
        job_config["is_compressed_by_packer"] = _create_job_detail.is_compressed_by_packer;
        mArray  excluded_paths(_create_job_detail.detail.p.excluded_paths.begin(), _create_job_detail.detail.p.excluded_paths.end());
        job_config["excluded_paths"] = excluded_paths;
        mArray  resync_paths(_create_job_detail.detail.p.resync_paths.begin(), _create_job_detail.detail.p.resync_paths.end());
        job_config["resync_paths"] = resync_paths;
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

void physical_packer_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    _histories.push_back(history_record::ptr(new history_record(state, error, format, error ? true : !is_cdr())));
    LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(format.str()).c_str());
}

void physical_packer_job:: save_status(){
    FUN_TRACE;
    try{
        macho::windows::auto_lock lock(_cs);
        mObject job_status;
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);       
        mArray progress;
        foreach(physical_packer_job_progress::map::value_type &p, _progress){
            mObject o;
            o["key"] = p.first;
            o["output"] = p.second->output;
            o["base"] = p.second->base;
            o["parent"] = p.second->parent;
            o["total_size"] = p.second->total_size;
            o["backup_progress"] = p.second->backup_progress;
            o["backup_size"] = p.second->backup_size;
            o["backup_image_offset"] = p.second->backup_image_offset;
            o["uri"] = p.second->uri;
            mArray changeds;
            foreach(replication_block::ptr& r, p.second->cdr_changed_areas){
                mObject changed;
                changed["o"] = r->start;
                changed["l"] = r->length;
                changed["t"] = r->offset;
                changeds.push_back(changed);
            }
            o["cdr_changed_areas"] = changeds;
            {
                macho::windows::auto_lock _lock(p.second->lock);
                mArray completed_blocks;
                foreach(replication_block::ptr &b, p.second->completed){
                    mObject completed;
                    completed["o"] = b->start;
                    completed["l"] = b->length;
                    completed["t"] = b->offset;
                    completed_blocks.push_back(completed);
                }
                o["completed_blocks"] = completed_blocks;
            }
            progress.push_back(o);
        }
        job_status["progress"] = progress;
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
            o["is_display"] = h->is_display;
            histories.push_back(o);
        }
        job_status["histories"] = histories;

        mArray journals;
        typedef std::map<int64_t, physical_vcbt_journal>  i64_physical_vcbt_journal_map;
        foreach(i64_physical_vcbt_journal_map::value_type &i, _journals){
            mObject o;
            o["journal_id"] = i.second.id;
            o["first_key"] = i.second.first_key;
            o["latest_key"] = i.second.latest_key;
            o["lowest_valid_key"] = i.second.lowest_valid_key;
            journals.push_back(o);
        }
        job_status["journals"] = journals;
        job_status["boot_disk"] = _boot_disk;
        mArray  system_disks(_system_disks.begin(), _system_disks.end());
        job_status["system_disks"] = system_disks;
        job_status["guest_os_type"] = (int)_guest_os_type;

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

bool physical_packer_job::load_status(std::wstring status_file){
    try
    {
        FUN_TRACE;
        if (boost::filesystem::exists(status_file)){
            macho::windows::auto_lock lock(_cs);
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            _state = find_value(job_status.get_obj(), "state").get_int();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _boot_disk = find_value_string(job_status.get_obj(), "boot_disk");
            mArray progress = find_value(job_status.get_obj(), "progress").get_array();
            foreach(mValue &p, progress){
                physical_packer_job_progress::ptr ptr = physical_packer_job_progress::ptr(new physical_packer_job_progress());
                ptr->uri = find_value_string(p.get_obj(), "uri");
                ptr->output = find_value_string(p.get_obj(), "output");
                ptr->base = find_value_string(p.get_obj(), "base");
                ptr->parent = find_value_string(p.get_obj(), "parent");
                ptr->total_size = find_value(p.get_obj(), "total_size").get_int64();
                ptr->backup_image_offset = find_value(p.get_obj(), "backup_image_offset").get_int64();
                ptr->backup_progress = find_value(p.get_obj(), "backup_progress").get_int64();
                ptr->backup_size = find_value(p.get_obj(), "backup_size").get_int64();
                mArray  changeds = find_value_array(p.get_obj(), "cdr_changed_areas");
                foreach(mValue c, changeds){
                    replication_block::ptr changed(new replication_block(0, 0, 0, 0));
                    changed->start = find_value(c.get_obj(), "o").get_int64();
                    changed->length = find_value(c.get_obj(), "l").get_int64();
                    changed->offset = find_value(c.get_obj(), "t").get_int64();
                    ptr->cdr_changed_areas.push_back(changed);
                }
                mArray  completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
                foreach(mValue c, completed_blocks){
                    replication_block::ptr completed(new replication_block(0,0,0,0));
                    completed->start = find_value(c.get_obj(), "o").get_int64();
                    completed->length = find_value(c.get_obj(), "l").get_int64();
                    completed->offset = find_value(c.get_obj(), "t").get_int64();
                    ptr->completed.push_back(completed);
                }
                _progress[find_value_string(p.get_obj(), "key")] = ptr;
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
                    arguments,
                    find_value_bool (h.get_obj(), "is_display"))));
            }

            mArray  journals = find_value(job_status.get_obj(), "journals").get_array();
            foreach(mValue i, journals){
                physical_vcbt_journal journal;
                journal.id = find_value(i.get_obj(), "journal_id").get_int64();
                journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
                journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
                journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
                _journals[journal.id] = journal;
            }

            mArray  system_disks = find_value(job_status.get_obj(), "system_disks").get_array();
            foreach(mValue i, system_disks){
                _system_disks.insert(i.get_str());
            }
            _guest_os_type = (saasame::transport::hv_guest_os_type::type)find_value_int32(job_status.get_obj(), "guest_os_type", 1);
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

void physical_packer_job::save_cdr(const replication_block::map& cdr){
    FUN_TRACE;
    try{
        macho::windows::auto_lock lock(_cs);
        mObject job_cdr;
        mArray  disk_cdrs;
        foreach(replication_block::map::value_type c, cdr){
            mArray changeds;
            foreach(replication_block::ptr& r, c.second){
                mObject changed;
                changed["o"] = r->start;
                changed["l"] = r->length;
                changed["t"] = r->offset;
                changeds.push_back(changed);
            }
            mObject disk_cdr;
            disk_cdr["disk"] = c.first;
            disk_cdr["changeds"] = changeds;
            disk_cdrs.push_back(disk_cdr);
        }
        job_cdr["cdr"] = disk_cdrs;
        boost::filesystem::path temp = _cdr_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_cdr, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _cdr_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _cdr_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output status info.")));
    }
    catch (...){
    }
}

bool physical_packer_job::load_cdr(replication_block::map & cdrs){
    try
    {
        FUN_TRACE;
        if (boost::filesystem::exists(_cdr_file)){
            macho::windows::auto_lock lock(_cs);
            std::ifstream is(_cdr_file.wstring());
            mValue job_cdr;
            read(is, job_cdr);
            mArray disk_cdrs = find_value(job_cdr.get_obj(), "cdr").get_array();
            foreach(mValue &disk_cdr, disk_cdrs){
                int disk_number = find_value(disk_cdr.get_obj(), "disk").get_int();
                mArray  changeds = find_value(disk_cdr.get_obj(), "changeds").get_array();
                replication_block::vtr disk_changeds;
                foreach(mValue c, changeds){
                    replication_block::ptr changed(new replication_block(0, 0, 0, 0));
                    changed->start = find_value(c.get_obj(), "o").get_int64();
                    changed->length = find_value(c.get_obj(), "l").get_int64();
                    changed->offset = find_value(c.get_obj(), "t").get_int64();
                    disk_changeds.push_back(changed);
                }
                cdrs[disk_number] = disk_changeds;
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

saasame::transport::packer_job_detail physical_packer_job::get_job_detail(boost::posix_time::ptime previous_updated_time) {
    FUN_TRACE;
    macho::windows::auto_lock lock(_cs);
    saasame::transport::packer_job_detail job;
    job.id = macho::stringutils::convert_unicode_to_utf8(_id);
    job.__set_type(_create_job_detail.type);
    job.created_time = boost::posix_time::to_simple_string(_created_time);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    job.updated_time = boost::posix_time::to_simple_string(update_time);
    job.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
    foreach(history_record::ptr &h, _histories){
        if (h->time > previous_updated_time){
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
    foreach(physical_packer_job_progress::map::value_type& v, _progress){
        job.detail.p.original_size[v.first] = v.second->total_size;
        job.detail.p.backup_progress[v.first] = v.second->backup_progress;
        job.detail.p.backup_image_offset[v.first] = v.second->backup_image_offset;
        job.detail.p.backup_size[v.first] = v.second->backup_size;
        std::vector<io_changed_range> changed;
        foreach(replication_block::ptr &c, v.second->cdr_changed_areas){
            io_changed_range _c(*c);
            changed.push_back(_c);
        }
        job.detail.p.cdr_changed_ranges[v.first] = changed;

        std::vector<io_changed_range> completeds;
        {
            macho::windows::auto_lock _lock(v.second->lock);
            foreach(replication_block::ptr &c, v.second->completed){
                io_changed_range _c(*c);
                completeds.push_back(_c);
            }
        }
        job.detail.p.completed_blocks[v.first] = completeds;
    }
    job.__set_detail(job.detail);
    job.detail.__set_p(job.detail.p);
    job.detail.p.__set_vcbt_journals(_journals);
    job.detail.p.__set_original_size(job.detail.p.original_size);
    job.detail.p.__set_backup_progress(job.detail.p.backup_progress);
    job.detail.p.__set_backup_size(job.detail.p.backup_size);
    job.detail.p.__set_backup_image_offset(job.detail.p.backup_image_offset);
    job.detail.p.__set_cdr_changed_ranges(job.detail.p.cdr_changed_ranges);
    job.detail.p.__set_completed_blocks(job.detail.p.completed_blocks);
    job.detail.p.__set_guest_os_type(_guest_os_type);

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
    return job;
}

void physical_packer_job::interrupt(){
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){   
    }
    else{
        _is_interrupted = true;
    }
    save_status();
}

bool physical_packer_job::get_output_image_info(const std::string disk, physical_packer_job_progress::ptr &p){
    FUN_TRACE;
    if (_create_job_detail.detail.p.images.count(disk)){
        p->output = _create_job_detail.detail.p.images[disk].name;
        p->parent = _create_job_detail.detail.p.images[disk].parent;
        p->base = _create_job_detail.detail.p.images[disk].base;
        return true;
    }
    return false;
}

void physical_packer_job::execute(){
    VALIDATE;
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || (_state & mgmt_job_state::job_state_error) || _is_removing){
        return;
    }

    macho::windows::environment::set_token_privilege(SE_MANAGE_VOLUME_NAME, true);
    macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true);
    macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true);
    
    _state &= ~mgmt_job_state::job_state_error;
    if (!_is_interrupted && verify_and_fix_snapshots()){
        _state = mgmt_job_state::job_state_replicating;
        save_status();
        bool result = false;

        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MaxBlockSize"].exists()){
                _block_size = (DWORD)reg[L"MaxBlockSize"];
            }
            if (reg[L"QueueNumber"].exists() && (DWORD)reg[L"QueueNumber"] > 0){
                _queue_size = (DWORD)reg[L"QueueNumber"] * CARRIER_RW_IMAGE_BLOCK_SIZE;
            }
            if (reg[L"CompressedByPacker"].exists() && (DWORD)reg[L"CompressedByPacker"] > 0){
                _create_job_detail.is_compressed_by_packer = true;
            }
        }

        if (_block_size < 1024 * 1024)
            _block_size = 1024 * 1024;
        if (_create_job_detail.min_transport_size > 0 && _create_job_detail.min_transport_size <= 8192){
            _full_mini_block_size = _delta_mini_block_size = _create_job_detail.min_transport_size * 1024;
        }
        if (_create_job_detail.full_min_transport_size > 0 && _create_job_detail.full_min_transport_size <= 8192){
            _full_mini_block_size = _create_job_detail.full_min_transport_size * 1024;
        }
        {
            registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"DeltaMiniBlockSize"].exists()){
                    _delta_mini_block_size = (DWORD)reg[L"DeltaMiniBlockSize"];
                }
                if (reg[L"FullMiniBlockSize"].exists()){
                    _full_mini_block_size = (DWORD)reg[L"FullMiniBlockSize"];
                }
            }
            if (_delta_mini_block_size > _block_size)
                _delta_mini_block_size = _block_size;

            if (_full_mini_block_size > _block_size)
                _full_mini_block_size = _block_size;
        }
        try{
            bool _is_cdr = is_cdr();
            std::map<std::string, storage::disk::ptr> disks;
            macho::windows::com_init  com;
            macho::windows::storage::ptr stg = IsWindowsVistaOrGreater() ? macho::windows::storage::local() : macho::windows::storage::get();
            macho::windows::storage::disk::vtr _disks = stg->get_disks();
            foreach(std::string p, _create_job_detail.detail.p.excluded_paths){
                std::wstring _p = stringutils::tolower(stringutils::convert_utf8_to_unicode(p));
                std::wstring mount_point = environment::get_volume_path_name(_p);
                if (!mount_point.empty()){
                    std::wstring volume_name = environment::get_volume_name_for_volume_mount_point(mount_point);
                    _excluded_paths[volume_name].insert(L"\\" + _p.substr(_p.length() > mount_point.length() ? mount_point.length() : _p.length()));
                }
            }

            foreach(std::string p, _create_job_detail.detail.p.resync_paths){
                std::wstring _p = stringutils::tolower(stringutils::convert_utf8_to_unicode(p));
                std::wstring mount_point = environment::get_volume_path_name(_p);
                if (!mount_point.empty()){
                    std::wstring volume_name = environment::get_volume_name_for_volume_mount_point(mount_point);
                    _resync_paths[volume_name].insert(L"\\" + _p.substr(_p.length() > mount_point.length() ? mount_point.length() : _p.length()));
                }
            }

            foreach(std::string disk, _create_job_detail.detail.p.disks){
                disk_universal_identify uri(disk);
                foreach(macho::windows::storage::disk::ptr d, _disks){
                    if (disk_universal_identify(*d) == uri){
                        disks[disk] = d;
                        physical_packer_job_progress::ptr p;
                        if (_progress.count(disk)){
                            p = _progress[disk];
                        }
                        else{
                            p = physical_packer_job_progress::ptr(new physical_packer_job_progress());
                            p->uri = disk_universal_identify(*d);
                            p->total_size = d->size();
                            if (_create_job_detail.detail.p.backup_size.count(disk))
                                p->backup_size = _create_job_detail.detail.p.backup_size[disk];
                            if (_create_job_detail.detail.p.backup_progress.count(disk))
                                p->backup_progress = _create_job_detail.detail.p.backup_progress[disk];
                            if (_create_job_detail.detail.p.backup_image_offset.count(disk))
                                p->backup_image_offset = _create_job_detail.detail.p.backup_image_offset[disk];
                            if (_create_job_detail.detail.p.cdr_changed_ranges.count(disk)){
                                foreach(io_changed_range &r, _create_job_detail.detail.p.cdr_changed_ranges[disk]){
                                    p->cdr_changed_areas.push_back(replication_block::ptr(new replication_block(0, r.start, r.length, r.offset)));
                                }
                            }
                            if (_create_job_detail.detail.p.completed_blocks.count(disk)){
                                foreach(io_changed_range &r, _create_job_detail.detail.p.completed_blocks[disk]){
                                    p->completed.push_back(replication_block::ptr(new replication_block(0, r.start, r.length, r.offset)));
                                }
                            }
                            _progress[disk] = p;
                        }
                        if (!(result = get_output_image_info(disk, p))){
                            _state |= mgmt_job_state::job_state_error;
                            break;
                        }
                    }
                }
                if (!result)
                    break;
            }
            if (result){
                replication_block::map data_changeds;
                if (_is_cdr){
                    /*if (!load_cdr(cdr_data_changeds))*/
                    if (0 == _create_job_detail.detail.p.cdr_journals.size()){
                        continuous_data_replication cdr(stg);
                        data_changeds = cdr.get_changeds(disks, _create_job_detail.detail.p.previous_journals);
                        _journals = cdr.get_journals();
                        save_cdr(data_changeds);
                        save_status();
                    }
                    else{
                        _journals = _create_job_detail.detail.p.cdr_journals;
                    }
                    foreach(std::string disk, _create_job_detail.detail.p.disks){
                        disk_universal_identify uri(disk);
                        foreach(macho::windows::storage::disk::ptr d, _disks){
                            if (disk_universal_identify(*d) == uri){
                                physical_packer_job_progress::ptr p = _progress[disk];
                                if (0 == p->backup_size && 0 == p->cdr_changed_areas.size()){
                                    p->cdr_changed_areas = data_changeds[d->number()];
                                    foreach(replication_block::ptr &c, p->cdr_changed_areas)
                                        p->backup_size += c->length;
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Backup size estimate of disk %1% : %2%.") % d->number() % p->backup_size));
                                }
                            }
                        }
                    }
                    save_status();
                }
                else{
                    data_changeds = get_replication_blocks(disks);
                }

                foreach(std::string disk, _create_job_detail.detail.p.disks){
                    disk_universal_identify uri(disk);
                    foreach(macho::windows::storage::disk::ptr d, _disks){
                        if (disk_universal_identify(*d) == uri){
                            physical_packer_job_progress::ptr p = _progress[disk];
                            if (!(result = replicate_disk(*d, data_changeds[d->number()],*p)))
                                break;
                        }
                    }
                    if (!result)
                        break;
                }
            }
            if (result && _create_job_detail.detail.p.disks.size() == _progress.size()){
                boost::filesystem::remove(_cdr_file);
                _state = mgmt_job_state::job_state_finished;
            }
        }
        catch (storage::exception& ex)
        {
            LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
            result = false;
            _state |= mgmt_job_state::job_state_error;
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
    }
    save_status();
}

changed_area::vtr physical_packer_job::_get_changed_areas(ULONGLONG file_area_offset, LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits, uint32_t bytes_per_cluster, uint64_t& total_number_of_delta){
    
    changed_area::vtr results;
    total_number_of_delta = 0;
    uint32_t mini_number_of_bits = _mini_block_size / bytes_per_cluster;
    uint32_t number_of_bits = _block_size / bytes_per_cluster;
    if (file_area_offset){
        results.push_back(changed_area(0, file_area_offset));
        total_number_of_delta += file_area_offset;
    }

#if 0
    LOG(LOG_LEVEL_RECORD, L"file_area_offset: %I64u, mini_block_size: %d, bytes_per_cluster: %d, block_size: %d", file_area_offset, _mini_block_size, bytes_per_cluster, _block_size);
#endif
    uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
    bool     dirty_range = false;
    for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++){
        if (buff[newBit >> 3] & (1 << (newBit & 7))){
            uint64_t num = (newBit - oldBit);
            if (!dirty_range){
                dirty_range = true;
                oldBit = next_dirty_bit = newBit;
            }
            else if (num == number_of_bits){
                total_number_of_delta += (num * bytes_per_cluster);
#if 0
                changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), num * bytes_per_cluster);
                LOG(LOG_LEVEL_RECORD, L"1: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
#endif
                results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), num * bytes_per_cluster));
                oldBit = next_dirty_bit = newBit;
            }
            else{
                next_dirty_bit = newBit;
            }
        }
        else {
            if( dirty_range && ( (newBit - oldBit) >= mini_number_of_bits ) ){
                uint64_t lenght = ( next_dirty_bit - oldBit + 1) * bytes_per_cluster;
                total_number_of_delta += lenght;
#if 0
                changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), lenght);
                LOG(LOG_LEVEL_RECORD, L"2: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
#endif
                results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), lenght));
                dirty_range = false;
            }
        }
    }
    if (dirty_range){
#if 0
        changed_area c(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), (next_dirty_bit - oldBit + 1) * bytes_per_cluster);
        LOG(LOG_LEVEL_RECORD, L"3: start: %I64u, length: %I64u, end:  %I64u", c.start_offset, c.length, c.start_offset + c.length);
#endif
        total_number_of_delta += ((next_dirty_bit - oldBit + 1) * bytes_per_cluster);
        results.push_back(changed_area(file_area_offset + ((start_lcn + oldBit) * bytes_per_cluster), (next_dirty_bit - oldBit + 1) * bytes_per_cluster));
    }
    return results;
}

macho::windows::storage::volume::ptr  physical_packer_job::get_volume_info(macho::string_array_w access_paths, macho::windows::storage::volume::vtr volumes){
    FUN_TRACE;
    foreach(std::wstring access_path, access_paths){
        foreach(macho::windows::storage::volume::ptr _v, volumes){
            foreach(std::wstring a, _v->access_paths()){
                if (a.length() >= access_path.length() && wcsstr(a.c_str(), access_path.c_str()) != NULL){
                    return _v;
                }
                else if (a.length() < access_path.length() && wcsstr(access_path.c_str(), a.c_str()) != NULL){
                    return _v;
                }
            }
        }
    }
    return NULL;
}

bool physical_packer_job::get_snapshot_info(macho::windows::storage::disk& d, macho::windows::storage::volume& v, saasame::transport::snapshot& info){
    std::string volume_path = macho::stringutils::convert_unicode_to_utf8(v.path());
    //std::string uri = macho::stringutils::convert_unicode_to_ansi(d.serial_number()); // Need to improve for uri path.
    foreach(saasame::transport::snapshot &s, _create_job_detail.detail.p.snapshots){
        if (!volume_path.empty() && s.original_volume_name == volume_path){
            info = s;
            return true;
        }
        else if (s.original_volume_name.length() >= volume_path.length() && strstr(s.original_volume_name.c_str(), volume_path.c_str()) != NULL){
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

bool physical_packer_job::get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots){
    bool result = false;
    try{
        macho::windows::registry reg;
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        std::shared_ptr<TSSLSocketFactory> factory;
        std::shared_ptr<TTransport> _transport;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
        }
        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                factory->authenticate(false);
                factory->loadCertificate((p / "server.crt").string().c_str());
                factory->loadPrivateKey((p / "server.key").string().c_str());
                factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
                factory->access(accessManager);
                std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket("localhost", saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
                ssl_socket->setConnTimeout(10 * 1000);
                _transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
            }
            catch (TException& ex) {
            }
        }
        else{
            std::shared_ptr<TSocket> socket(new TSocket("localhost", saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
            socket->setConnTimeout(10 * 1000);
            _transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
        }
        if (_transport){
            std::shared_ptr<TProtocol> _protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));
            std::shared_ptr<physical_packer_serviceClient> _client = std::shared_ptr<physical_packer_serviceClient>(new physical_packer_serviceClient(_protocol));
            _transport->open();
            _client->get_all_snapshots(snapshots, macho::guid_::create());
            result = true;
            _transport->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
    }
    return result;
}

bool    physical_packer_job::verify_and_fix_snapshots(){
    bool result = false;
    if (is_cdr())
        result = true;
    else if (_create_job_detail.detail.p.snapshots.size()){
        std::map<std::string, std::vector<snapshot> > snapshots;
        if (result = get_all_snapshots(snapshots)){
            if (snapshots.count(_create_job_detail.detail.p.snapshots[0].snapshot_set_id)){
                foreach(saasame::transport::snapshot &s, _create_job_detail.detail.p.snapshots){
                    bool found = false;
                    foreach(snapshot &_s, snapshots[_create_job_detail.detail.p.snapshots[0].snapshot_set_id]){
                        if (_s.snapshot_id == s.snapshot_id){
                            s.snapshot_device_object = _s.snapshot_device_object;
                            found = true;
                            break;
                        }
                    }
                    if (!found){
                        result = false;
                        _state |= mgmt_job_state::job_state_error;
                        _state |= mgmt_job_state::job_state_discard;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot find snapshot %1%.") % s.snapshot_id));
                        break;
                    }
                }
            }
            else{
                result = false;
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot find snapshots by snapshot set %1%.") % _create_job_detail.detail.p.snapshots[0].snapshot_set_id));
            }
        }
        else{
            result = false;
            _state |= mgmt_job_state::job_state_error;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot get snapshots info.")));
        }
    }
    else{
        result = false;
        _state |= mgmt_job_state::job_state_error;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_INVALID, (record_format("No snapshot found.")));
    }
    return result;
}

physical_vcbt_journal physical_packer_job::get_previous_journal(macho::guid_ volume_id){
    physical_vcbt_journal journal;
    macho::windows::auto_handle hDevice = CreateFileW(VCBT_WIN32_DEVICE_NAME,          // drive to open
        0,                // no access to the drive
        FILE_SHARE_READ | // share mode
        FILE_SHARE_WRITE,
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes      
    if (hDevice.is_invalid()){
        LOG(LOG_LEVEL_WARNING, L"VCBT driver is not ready.");
        _is_vcbt_enabled = false;
    }
    else{
        _is_vcbt_enabled = true;
        DWORD input_length = sizeof(VCBT_COMMAND_INPUT) + (sizeof(VCBT_COMMAND) * 1);
        DWORD output_length = sizeof(VCBT_COMMAND_RESULT) + (sizeof(VCOMMAND_RESULT) * 1);
        std::auto_ptr<VCBT_COMMAND_INPUT> command = std::auto_ptr<VCBT_COMMAND_INPUT>((PVCBT_COMMAND_INPUT)new BYTE[input_length]);
        std::auto_ptr<VCBT_COMMAND_RESULT> result = std::auto_ptr<VCBT_COMMAND_RESULT>((PVCBT_COMMAND_RESULT)new BYTE[output_length]);
        memset(command.get(), 0, input_length);
        memset(result.get(), 0, output_length);

        command->NumberOfCommands = 1;
        command->Commands[0].VolumeId = volume_id;
        
        if (!(DeviceIoControl(hDevice,                        // device to be queried
            IOCTL_VCBT_IS_ENABLE,                                            // operation to perform
            command.get(), input_length,                      // input buffer
            result.get(), output_length,                      // output buffer
            &output_length,                                   // # bytes returned
            (LPOVERLAPPED)NULL)))                             // synchronous I/O
        {
            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(VCBT_IS_ENABLE) failed, Error(0x%08x).", GetLastError());
        }
        else{ 
            LOG(LOG_LEVEL_INFO, L"Send command(VCBT_IS_ENABLE), Volume id : %s", ((std::wstring)volume_id).c_str());
            if (result->Results[0].Status >= 0 ){   
                LOG(LOG_LEVEL_INFO, L"Journal Id : %I64u", result->Results[0].JournalId);
                LOG(LOG_LEVEL_INFO, L"Enable : %s", (result->Results[0].Detail.Done ? L"true" : L"false"));
                if (result->Results[0].Detail.Enabled && _create_job_detail.detail.p.previous_journals.count(result->Results[0].JournalId)){
                    journal = _create_job_detail.detail.p.previous_journals[result->Results[0].JournalId];
                    journal.__set_id(journal.id);
                    journal.__set_first_key(journal.first_key);
                    journal.__set_latest_key(journal.latest_key);
                    journal.__set_lowest_valid_key(journal.lowest_valid_key);
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Error: 0x%08x", result->Results[0].Status);
            }
        }
    }
    return journal;
}

#define VCBT_VOLUME_SYS_INFO    L"\\System Volume Information"
#define min(a,b) (((a) < (b)) ? (a) : (b))

boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>  physical_packer_job::get_journal_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, const physical_vcbt_journal& previous_journal, uint64_t start){
    boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER> result;
    std::string dir = path + "\\";
    boost::filesystem::path journal = path;
    journal += VCBT_VOLUME_SYS_INFO;
    journal += CBT_JOURNAL_FILE1;
    ULARGE_INTEGER total_number_of_bytes;
    if (!_is_vcbt_enabled){
    }
    else if (!GetDiskFreeSpaceExA(dir.c_str(), NULL, &total_number_of_bytes, NULL)){
    }
    else if (boost::filesystem::exists(journal)){
        macho::windows::auto_handle handle = CreateFileW(journal.wstring().c_str(),
            GENERIC_READ,                // no access to the drive
            FILE_SHARE_READ,
            NULL,             // default security attributes
            OPEN_EXISTING,    // disposition
            0,                // file attributes
            NULL);            // do not copy file attributes      
        if (handle.is_invalid()){
            LOG(LOG_LEVEL_WARNING, L"Journal file(%s) is not ready.", journal.wstring().c_str());
        }
        else{
            changed_area::vtr     journal_changed_area;
            physical_vcbt_journal vcbt_journal;
            bool journal_found = false, terminated = false, previous_latest_key_found = false;
            DWORD read_bytes, number_of_bytes_read = 0;
            DWORD offset = 0;
            read_bytes = bytes_per_cluster * 2;
            std::auto_ptr<BYTE> pBuf = std::auto_ptr<BYTE>(new BYTE[bytes_per_cluster * 2]);
            PVCBT_JOURNAL_BLOK pJournalBlock = (PVCBT_JOURNAL_BLOK)pBuf.get();
            while (true){
                if (!ReadFile(handle, &pBuf.get()[offset], read_bytes, &number_of_bytes_read, NULL)){
                    DWORD err = GetLastError();
                    LOG(LOG_LEVEL_ERROR, _T("ReadFile(%s) error = %u"), journal.wstring().c_str(), err);
                    break;
                }
                LONG min_size = min((LONG)(number_of_bytes_read + offset), (bytes_per_cluster + sizeof(VCBT_RECORD)));
                for (;
                    (BYTE*)pJournalBlock < (pBuf.get() + min_size);
                    pJournalBlock = (PVCBT_JOURNAL_BLOK)(((BYTE*)pJournalBlock) + sizeof(VCBT_RECORD))){
                    if (offset == 0 && pJournalBlock == (PVCBT_JOURNAL_BLOK)pBuf.get()){
                        vcbt_journal.first_key = pJournalBlock->r.Key;
                        vcbt_journal.lowest_valid_key = pJournalBlock->r.Key;
                    }

                    if (pJournalBlock->r.Start != 0 && pJournalBlock->r.Length != 0){
                        if (previous_journal.id != 0){
                            if (vcbt_journal.first_key == previous_journal.first_key){
                                if (previous_latest_key_found)
                                    journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                            }
                            else{
                                if ((!previous_latest_key_found &&!journal_found) || (previous_latest_key_found &&journal_found))
                                    journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                            }
                        }
                        if (((ULONGLONG)vcbt_journal.lowest_valid_key) > pJournalBlock->r.Key)
                            vcbt_journal.lowest_valid_key = pJournalBlock->r.Key;
                    }

                    if (previous_journal.id != 0 && pJournalBlock->r.Key == previous_journal.latest_key){
                        previous_latest_key_found = true;
                        journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                    }

                    if ((pJournalBlock->j.CheckSum == (ULONG)-1) &&
                        (pJournalBlock->j.Signature[0] == '1') &&
                        (pJournalBlock->j.Signature[1] == 'c') &&
                        (pJournalBlock->j.Signature[2] == 'b') &&
                        (pJournalBlock->j.Signature[3] == 't')){

                        if (journal_found){
                            break;
                        }
                        else {
                            journal_found = true;
                            vcbt_journal.latest_key = pJournalBlock->r.Key;
                            vcbt_journal.id = pJournalBlock->j.JournalId;
                            // (scenario 1) first........pre_________cbt.......... end (the first key will be equal)
                            // (scenario 2) first________cbt.........pre___________end (the first key will be different)
                            if (vcbt_journal.first_key == previous_journal.first_key){
                                terminated = true;
                                break;
                            }
                        }
                    }
                    if (journal_found && pJournalBlock->r.Start == 0
                        && pJournalBlock->r.Length == 0
                        && pJournalBlock->r.Key == 0){
                        terminated = true;
                        break;
                    }
                }
                if (terminated)
                    break;
                read_bytes = offset = bytes_per_cluster;
                MoveMemory(&pBuf.get()[0], &pBuf.get()[bytes_per_cluster], bytes_per_cluster);
                ZeroMemory(&pBuf.get()[bytes_per_cluster], bytes_per_cluster);
                pJournalBlock = (PVCBT_JOURNAL_BLOK)(((BYTE*)pJournalBlock) - bytes_per_cluster);
            }

            if (journal_found){
                if (previous_journal.id == vcbt_journal.id && previous_latest_key_found){
                    ULONGLONG start_bit = start / bytes_per_cluster;
                    ULONGLONG start_offset = (start_bit * bytes_per_cluster) + file_area_offset;
                    ULONGLONG bits_number = (total_number_of_bytes.QuadPart / bytes_per_cluster) - start_bit;
                    ULONG BitmapSize = ULONG(sizeof(physical_packer_job::VOLUME_BITMAP_BUFFER) + (bits_number / 8) + 1);
                    result = boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>((physical_packer_job::VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
                    if (result){
                        memset(result.get(), 0, BitmapSize);
                        result->StartingLcn.QuadPart = start_bit;
                        result->BitmapSize.QuadPart = bits_number;
                        foreach(changed_area &c, journal_changed_area){
#ifdef _DEBUG
                            LOG(LOG_LEVEL_RECORD, L"Journal : start: %I64u, length: %I64u, end: %I64u", c.start_offset - start_offset, c.length, c.start_offset - start_offset + c.length);
#endif
                            set_umap(result->Buffer, c.start_offset - start_offset, (ULONG)c.length, bytes_per_cluster);
                        }
#ifdef _DEBUG
                        {
                            uint64_t newBit, oldBit = 0;
                            bool     dirty_range = false;
                            for (newBit = 0; newBit < bits_number; newBit++){
                                if (result.get()->Buffer[newBit >> 3] & (1 << (newBit & 7))){
                                    if (!dirty_range){
                                        dirty_range = true;
                                        oldBit = newBit;
                                    }
                                }
                                else {
                                    if (dirty_range){
                                        ULONGLONG start = (oldBit * bytes_per_cluster);
                                        ULONGLONG length = (newBit - oldBit) * bytes_per_cluster;
                                        LOG(LOG_LEVEL_RECORD, L"Buffer : start: %I64u, length: %I64u, end: %I64u", start, length, start + length);
                                        dirty_range = false;
                                    }
                                }
                            }
                            if (dirty_range){
                                ULONGLONG start = (oldBit * bytes_per_cluster);
                                ULONGLONG length = (newBit - oldBit) * bytes_per_cluster;
                                LOG(LOG_LEVEL_RECORD, L"Buffer : start: %I64u, length: %I64u, end: %I64u", start, length, start + length);
                            }
                        }
#endif
                    }
                }
                _journals[vcbt_journal.id] = vcbt_journal;
            }
        }
    }
    return result;
}

boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>  physical_packer_job::get_umap_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, uint64_t start){
    boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER> result;
    std::string dir = path + "\\";
    ULARGE_INTEGER total_number_of_bytes;
    if (!_is_vcbt_enabled){
    }
    else if (!GetDiskFreeSpaceExA(dir.c_str(), NULL, &total_number_of_bytes, NULL)){
    }
    else{
        int resolution = UMAP_RESOLUTION1;
        int count = 1;
        while ((total_number_of_bytes.QuadPart >> (resolution + 19)) > 0){
            count++;
            resolution++;
        }
        WCHAR buff[MAX_PATH];
        memset(buff, 0, sizeof(buff));
        wsprintf(buff, L"%s"UMAP_RESOLUTION_FILE, VCBT_VOLUME_SYS_INFO, count);
        boost::filesystem::path umap = path;
        umap += buff;
        boost::filesystem::path umap_snap = umap.wstring() + L".snap";
        boost::shared_ptr<BYTE> pSnapShot = read_umap_data(umap_snap);
        if (pSnapShot){
            boost::shared_ptr<BYTE> pUmap = read_umap_data(umap);
            if (pUmap){
                merge_umap(pUmap.get(), pSnapShot.get(), RESOLUTION_UMAP_SIZE);
                ULONGLONG start_bit = start / bytes_per_cluster;
                ULONGLONG start_offset = (start_bit * bytes_per_cluster) + file_area_offset;
                ULONGLONG bits_number = (total_number_of_bytes.QuadPart / bytes_per_cluster) - start_bit;
                ULONGLONG BitmapSize = sizeof(physical_packer_job::VOLUME_BITMAP_BUFFER) + (bits_number / 8) + 1;
                result = boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>((physical_packer_job::VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
                if (result){
                    memset(result.get(), 0, BitmapSize);
                    result->StartingLcn.QuadPart = start_bit;
                    result->BitmapSize.QuadPart = bits_number;
                    ULONG diff = (1 << resolution) / bytes_per_cluster;
                    ULONG current_bit = 0;
                    for (ULONGLONG i = (start_offset >> resolution); i < (total_number_of_bytes.QuadPart >> resolution); i++){
                        if (test_umap_bit(pUmap.get(), i)){
                            for (ULONG count = 0; count < diff; count++)
                                set_umap_bit(result->Buffer, current_bit + count);
                        }
                        current_bit += diff;
                    }
                }
            }
        }
    }
    return result;
}

boost::shared_ptr<BYTE>  physical_packer_job::read_umap_data(boost::filesystem::path p){
    boost::shared_ptr<BYTE> pByte = NULL;
    if (boost::filesystem::exists(p)){
        macho::windows::auto_handle handle = CreateFileW(p.wstring().c_str(),
            GENERIC_READ,     // no access to the drive
            FILE_SHARE_READ,
            NULL,             // default security attributes
            OPEN_EXISTING,    // disposition
            0,                // file attributes
            NULL);            // do not copy file attributes      
        if (handle.is_invalid()){
            LOG(LOG_LEVEL_WARNING, L"Umap file(%s) is not ready.", p.wstring().c_str());
        }
        else {
            pByte = boost::shared_ptr<BYTE>(new BYTE[RESOLUTION_UMAP_SIZE]);
            if (pByte){
                memset(pByte.get(), 0, RESOLUTION_UMAP_SIZE);
                DWORD number_of_bytes_read = 0;
                if (!(ReadFile(handle, pByte.get(), RESOLUTION_UMAP_SIZE, &number_of_bytes_read, NULL) && number_of_bytes_read == RESOLUTION_UMAP_SIZE)){
                    pByte = NULL;
                }
            }
        }
    }
    return pByte;
}

BOOL physical_packer_job::get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset){
    FAT_LBR		fatLBR  = { 0 };
    BOOL        bFnCall = FALSE;
    BOOL        bRetVal = FALSE;
    DWORD       dwRead = 0;
    if ((bFnCall = ReadFile(handle, &fatLBR, sizeof(FAT_LBR), &dwRead, NULL) 
       && dwRead == sizeof(fatLBR))){
        DWORD dwRootDirSectors = 0;
        DWORD dwFATSz = 0;

        // Validate jump instruction to boot code. This field has two
        // allowed forms: 
        // jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90 
        // and
        // jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
        // 0x?? indicates that any 8-bit value is allowed in that byte.
        // JmpBoot[0] = 0xEB is the more frequently used format.

        if ((fatLBR.wTrailSig != 0xAA55) ||
            ((fatLBR.pbyJmpBoot[0] != 0xEB ||
            fatLBR.pbyJmpBoot[2] != 0x90) &&
            (fatLBR.pbyJmpBoot[0] != 0xE9))){
            bRetVal = FALSE;
            goto __faild;
        }

        // Compute first sector offset for the FAT volumes:		


        // First, we determine the count of sectors occupied by the
        // root directory. Note that on a FAT32 volume the BPB_RootEntCnt
        // value is always 0, so on a FAT32 volume dwRootDirSectors is
        // always 0. The 32 in the above is the size of one FAT directory
        // entry in bytes. Note also that this computation rounds up.

        dwRootDirSectors =
            (((fatLBR.bpb.wRootEntCnt * 32) +
            (fatLBR.bpb.wBytsPerSec - 1)) /
            fatLBR.bpb.wBytsPerSec);

        // The start of the data region, the first sector of cluster 2,
        // is computed as follows:

        dwFATSz = fatLBR.bpb.wFATSz16;
        if (!dwFATSz)
            dwFATSz = fatLBR.ebpb.ebpb32.dwFATSz32;

        if (!dwFATSz){
            bRetVal = FALSE;
            goto __faild;
        }
        // 
        file_area_offset =
            (fatLBR.bpb.wRsvdSecCnt +
            (fatLBR.bpb.byNumFATs * dwFATSz) +
            dwRootDirSectors) * fatLBR.bpb.wBytsPerSec;
    }
    bRetVal = TRUE;
__faild:
    return bRetVal;
}

bool physical_packer_job::sync_file(LPBYTE buff, boost::filesystem::path file, bool log_message, bool exclude){
    bool result = false;
    if (boost::filesystem::exists(file)){
        DWORD attributes = GetFileAttributesW(file.wstring().c_str());
        if ((FILE_ATTRIBUTE_REPARSE_POINT & attributes) == FILE_ATTRIBUTE_REPARSE_POINT){
            LOG(log_message ? LOG_LEVEL_INFO : LOG_LEVEL_TRACE, L"File (%s) is reparse point.", file.wstring().c_str());
            return false;
        }
        LOG(log_message ? LOG_LEVEL_INFO : LOG_LEVEL_TRACE, L"Try to %s the data range for the file (%s).", exclude ? L"exclude" : L"resync", file.wstring().c_str());
        macho::windows::auto_file_handle handle = CreateFileW(file.wstring().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (handle.is_invalid()){
            LOG(log_message ? LOG_LEVEL_ERROR : LOG_LEVEL_INTERNAL_ERROR, L"CreateFile (%s) failed. error: %d", file.wstring().c_str(), GetLastError());
            return result;
        }
        else{
            STARTING_VCN_INPUT_BUFFER starting;
            std::auto_ptr<RETRIEVAL_POINTERS_BUFFER> pVcnPairs;
            starting.StartingVcn.QuadPart = 0;
            ULONG ulOutPutSize = 0;
            ULONG uCounts = 200;
            ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
            pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
            if (NULL == pVcnPairs.get()){
                return result;
            }
            else{
                DWORD BytesReturned = 0;
                while (!DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &starting, sizeof(STARTING_VCN_INPUT_BUFFER), pVcnPairs.get(), ulOutPutSize, &BytesReturned, NULL)){
                    DWORD err = GetLastError();
                    if (ERROR_MORE_DATA == err || ERROR_BUFFER_OVERFLOW == err){
                        uCounts += 200;
                        ulOutPutSize = sizeof(RETRIEVAL_POINTERS_BUFFER) + uCounts * sizeof(LARGE_INTEGER) * 2 + sizeof(LARGE_INTEGER);
                        pVcnPairs = std::auto_ptr<RETRIEVAL_POINTERS_BUFFER>((PRETRIEVAL_POINTERS_BUFFER)new BYTE[ulOutPutSize]);
                        if (pVcnPairs.get() == NULL)
                            return result;
                    }
                    else{
                        LOG(log_message ? LOG_LEVEL_ERROR : LOG_LEVEL_INTERNAL_ERROR, L"DeviceIoControl(%s)(FSCTL_GET_RETRIEVAL_POINTERS) failed. Error: %d", file.wstring().c_str(), err);
                        return result;
                    }
                }
                LARGE_INTEGER liVcnPrev = pVcnPairs->StartingVcn;
                for (DWORD extent = 0; extent < pVcnPairs->ExtentCount; extent++){
                    LONG      _Length = (ULONG)(pVcnPairs->Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart);
                    if (exclude)
                        clear_umap(buff, pVcnPairs->Extents[extent].Lcn.QuadPart, _Length, 1);
                    else
                        set_umap(buff, pVcnPairs->Extents[extent].Lcn.QuadPart, _Length, 1);
                    LOG(log_message ? LOG_LEVEL_DEBUG : LOG_LEVEL_TRACE, L"File (%s) Ranges: %I64u, %d", file.wstring().c_str(), pVcnPairs->Extents[extent].Lcn.QuadPart, _Length);
                    liVcnPrev = pVcnPairs->Extents[extent].NextVcn;
                }
                LOG((log_message ? LOG_LEVEL_RECORD : LOG_LEVEL_TRACE), L"%s the data replication for the file (%s).", exclude ? L"Excluded" : L"Resync", file.wstring().c_str());
                result = true;
            }
        }
    }
    return result;
}

bool physical_packer_job::is_cdr(){
    FUN_TRACE;
    if (_create_job_detail.detail.p.snapshots.size()){
        if (_create_job_detail.detail.p.snapshots[0].original_volume_name == "cdr" && _create_job_detail.detail.p.snapshots[0].snapshot_device_object == "cdr"){
            return true;
        }
    }
    return false;
}

replication_block::map   physical_packer_job::get_replication_blocks(const std::map<std::string, storage::disk::ptr>& disks){
    FUN_TRACE;
    replication_block::map         results;
    physical_vcbt_journal          null_journal;
    std::map<std::wstring, bool>   processed_dynamic_volumes;
    typedef std::map<std::string, storage::disk::ptr> disk_key_map;
    foreach(disk_key_map::value_type _d, disks){
        if (_progress.count(_d.first) && _progress[_d.first]->is_completed())
            continue;
        storage::disk::ptr d = _d.second;
        replication_block::vtr& _result = results[d->number()];
        if (!_progress[_d.first]->backup_size)
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Calculating backup size of disk %1%.") % d->number()));
        if (d->partition_style() != storage::ST_PARTITION_STYLE::ST_PST_UNKNOWN){
            macho::windows::storage::partition::vtr parts = d->get_partitions();
            if (0 == parts.size()){
                _result.push_back(replication_block::ptr(new replication_block(0, 0, 64 * 1024, 0)));
            }
            else{
                universal_disk_rw::ptr rw = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d->number()));
                if (!rw){
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot calculate backup size of disk %1%.") % d->number()));
                }
                else{
                    foreach(macho::windows::storage::partition::ptr p, parts){
                        if (1 == p->partition_number()){
                            _result.push_back(replication_block::ptr(new replication_block(0, 0, p->offset() < _block_size ? p->offset() : _block_size, 0)));
                            break;
                        }
                    }
                    replication_block::vtr changeds = get_epbrs(rw, *d, parts);
                    _result.insert(_result.end(), changeds.begin(), changeds.end());
                    uint64_t end_of_partiton = 0, backup_image_offset = 0;
                    if (_progress.count(_d.first))
                        backup_image_offset = _progress[_d.first]->backup_image_offset;
                    foreach(macho::windows::storage::partition::ptr p, parts){
                        end_of_partiton = p->offset() + p->size();
                        if (backup_image_offset >= end_of_partiton){
                        }
                        else if ((d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR && p->is_active() && p->access_paths().empty() && p->mbr_type() == 0x27) ||
                            (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_SYSTEM_GUID)))){
                            temp_drive_letter::ptr temp;
                            std::wstring drive_letter;
                            if (p->access_paths().empty()){
                                temp = temp_drive_letter::assign(p->disk_number(), p->partition_number(), false);
                                if (temp)
                                    drive_letter = temp->drive_letter();
                            }
                            else{
                                drive_letter = p->access_paths()[0];
                            }
                            replication_block::vtr  changeds = drive_letter.empty() ? get_partition_replication_blocks((*p.get()), backup_image_offset) : get_replication_blocks(*p, macho::stringutils::convert_unicode_to_utf8(drive_letter), backup_image_offset, null_journal);
                            if (p->access_paths().empty()){
                                foreach(replication_block::ptr c, changeds){
                                    c->start = c->offset;
                                    c->path = NULL;
                                }
                            }
                            _result.insert(_result.end(), changeds.begin(), changeds.end());
                        }
                        else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR &&
                            (p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)){
                        }
                        else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_LDM_METADATA_GUID))){
                            replication_block::vtr  changeds = get_partition_replication_blocks((*p.get()), backup_image_offset);
                            _result.insert(_result.end(), changeds.begin(), changeds.end());
                        }
                        else if ((d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR && p->access_paths().empty() && p->mbr_type() == 0x42) ||
                            (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT && (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_LDM_METADATA_GUID)))){
                        }
                        else if (p->access_paths().empty()){
                        }
                        else if ((!d->is_dynmaic())){
                            macho::windows::storage::volume::ptr v = get_volume_info(p->access_paths(), d->get_volumes());
                            if (!v){
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Cannot get volume object from access path %1%.") % stringutils::convert_unicode_to_utf8(p->access_paths()[0])));
                                break;
                            }
                            else if (p->is_active() && p->is_system() && v->file_system() == L"FAT32"){ // Replicate the boot partition.
                                std::vector<std::wstring> access_paths = p->access_paths();
                                std::wstring volume_path;
                                foreach(std::wstring volume_name, access_paths){
                                    if (volume_name.length() < 5 ||
                                        volume_name[0] != L'\\' ||
                                        volume_name[1] != L'\\' ||
                                        volume_name[2] != L'?' ||
                                        volume_name[3] != L'\\' ||
                                        volume_name[volume_name.length() - 1] != L'\\'){
                                    }
                                    else{
                                        volume_path = volume_name;
                                        break;
                                    }
                                }
                                replication_block::vtr  changeds = get_replication_blocks(*p, macho::stringutils::convert_unicode_to_utf8(volume_path), backup_image_offset, null_journal);
                                _result.insert(_result.end(), changeds.begin(), changeds.end());
                            }
                            else if (v->file_system() == L"NTFS" || v->file_system() == L"ReFS" || v->file_system() == L"CSVFS"){
                                saasame::transport::snapshot s;
                                if (!get_snapshot_info(*d, *v, s)){
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Cannot find VSS snapshot for volume %1%.") % stringutils::convert_unicode_to_utf8(v->access_paths()[0])));
                                    break;
                                }
                                else{
                                    replication_block::vtr  changeds = get_replication_blocks(*p, s.snapshot_device_object, backup_image_offset, get_previous_journal(v->id()), _excluded_paths[v->path()], _resync_paths[v->path()]);
                                    _result.insert(_result.end(), changeds.begin(), changeds.end());
                                }
                            }
                        }
                    }
                }
            }

            if (d->is_dynmaic()){
                foreach(storage::volume::ptr v, d->get_volumes()){
                    if (v->file_system() == L"NTFS" || v->file_system() == L"ReFS" || v->file_system() == L"CSVFS"){
                        if (!processed_dynamic_volumes.count(v->path())){
                            processed_dynamic_volumes[v->path()] = true;
                            saasame::transport::snapshot s;
                            if (!get_snapshot_info(*d, *v, s)){
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INTERNAL_FAIL, (record_format("Cannot find VSS snapshot for volume %1%.") % stringutils::convert_unicode_to_utf8(v->access_paths()[0])));
                                break;
                            }
                            else{
                                replication_block::map changeds = get_replication_blocks(*v, s.snapshot_device_object, get_previous_journal(v->id()), _excluded_paths[v->path()], _resync_paths[v->path()]);
                                foreach(replication_block::map::value_type& c, changeds){
                                    results[c.first].insert(results[c.first].end(), c.second.begin(), c.second.end());
                                }
                            }
                        }
                    }
                }
                if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR)
                    _result.push_back(replication_block::ptr(new replication_block(0, d->size() - (1024 * 1024), (1024 * 1024), d->size() - (1024 * 1024))));
            }
            calculate_backup_size(*d, *_progress[_d.first], _result);
        }
    }
    foreach(disk_key_map::value_type _d, disks){
        uint64_t start = 0;
        if (_progress.count(_d.first) && _progress[_d.first]->is_completed())
            continue;
        int disk_number = _d.second->number();
        replication_block::vtr& result = results[disk_number];
        if (_progress.count(_d.first))
            start = _progress[_d.first]->backup_image_offset;
        if (start){
            for (replication_block::vtr::iterator it = result.begin(); it != result.end() /* !!! */;){
                if ((*it)->RealEnd() < start){
                    it = result.erase(it);  // Returns the new iterator to continue from.
                }
                else{
                    ++it;
                }
            }
        }
        std::sort(result.begin(), result.end(), replication_block::compare());
    }
    return results;
}

replication_block::vtr  physical_packer_job::get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions){
    FUN_TRACE;
    replication_block::vtr results;
    int length = 4096;
    if (d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR){
        std::auto_ptr<BYTE> pBuf = std::auto_ptr<BYTE>(new BYTE[length]);
        if (NULL != pBuf.get()){
            foreach(macho::windows::storage::partition::ptr p, partitions){
                if (!(p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED))
                    continue;
                uint64_t start = p->offset();
                bool loop = true;
                while (loop){
                    loop = false;
                    memset(pBuf.get(), 0, length);
                    uint32_t nRead = 0;
                    BOOL result = FALSE;
                    if (result = disk_rw->read(start, length, pBuf.get(), nRead)){
                        PLEGACY_MBR pMBR = (PLEGACY_MBR)pBuf.get();
                        if (pMBR->Signature != 0xAA55)
                            break;
                        results.push_back(replication_block::ptr(new replication_block(0, start, d.logical_sector_size(), start)));
                        for (int i = 0; i < 2; i++){
                            if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                start = p->offset() + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * d.logical_sector_size());
                                loop = true;
                                break;
                            }
                            else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
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
    std::sort(results.begin(), results.end(), replication_block::compare());
    return results;
}

replication_block::vtr  physical_packer_job::get_partition_replication_blocks(macho::windows::storage::partition& p, const uint64_t& start_offset){
    FUN_TRACE;
    replication_block::vtr results;
    ULONG             full_mini_block_size = 8 * 1024 * 1024;
    changed_area::vtr changes;
    uint64_t start = start_offset;
    uint64_t partition_end = p.offset() + p.size();
    while (start < partition_end){
        uint64_t length = partition_end - start;
        if (length > full_mini_block_size)
            length = full_mini_block_size;
        results.push_back(replication_block::ptr(new replication_block(0, start, length, start)));
        start += length;
    }
    return results;
}

void physical_packer_job::get_volume_bitmap(volume_bitmap &bitmap, std::string path, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths, std::set<std::wstring> resync_paths){
    FUN_TRACE;
    macho::windows::auto_handle handle = CreateFileA(path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle.is_invalid()){
        invalid_operation error;
        error.what_op = ERROR_INVALID_HANDLE;
        error.why = boost::str(boost::format("Cannot open(\"%s\")") % path);
        throw error;
    }
    else{
        DWORD SectorsPerCluster = 0;
        DWORD BytesPerSector = 0;
        DWORD BytesPerCluster = 0;
        std::string device_object = path + "\\";
        if (!GetDiskFreeSpaceA(device_object.c_str(), &SectorsPerCluster, &BytesPerSector, NULL, NULL)){
            invalid_operation error;
            error.what_op = GetLastError();
            error.why = boost::str(boost::format("Cannot get disk free space(\"%s\")") % device_object);
            throw error;
        }
        else{
            bitmap.bytes_per_cluster = BytesPerCluster = SectorsPerCluster * BytesPerSector;
            STARTING_LCN_INPUT_BUFFER StartingLCN;
            UINT32 BitmapSize;
            DWORD BytesReturned;
            BOOL Result;
            ULONGLONG ClusterCount = 0;
            StartingLCN.StartingLcn.QuadPart = 0;
            BitmapSize = sizeof(physical_packer_job::VOLUME_BITMAP_BUFFER) + 4;
            bitmap.bitmap = boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>((physical_packer_job::VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
            Result = DeviceIoControl(
                handle,
                FSCTL_GET_VOLUME_BITMAP,
                &StartingLCN,
                sizeof(StartingLCN),
                bitmap.bitmap.get(),
                BitmapSize,
                &BytesReturned,
                NULL);
            DWORD err = GetLastError();
            if (Result == FALSE && err != ERROR_MORE_DATA){
                LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) faided for %s. (Error : 0x%08x)", 0, macho::stringutils::convert_ansi_to_unicode(path).c_str(), err);
                invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL;
                error.why = boost::str(boost::format("Cannot get volume bitmap for volume %s.") % path);
                throw error;
            }
            else{
                BitmapSize = (ULONG)(sizeof(physical_packer_job::VOLUME_BITMAP_BUFFER) + (bitmap.bitmap->BitmapSize.QuadPart / 8) + 1);
                bitmap.bitmap = boost::shared_ptr<physical_packer_job::VOLUME_BITMAP_BUFFER>((physical_packer_job::VOLUME_BITMAP_BUFFER*)new BYTE[BitmapSize]);
                Result = DeviceIoControl(
                    handle,
                    FSCTL_GET_VOLUME_BITMAP,
                    &StartingLCN,
                    sizeof(StartingLCN),
                    bitmap.bitmap.get(),
                    BitmapSize,
                    &BytesReturned,
                    NULL);
                if (Result == FALSE){
                    LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_VOLUME_BITMAP, %I64u) failed for %s. (Error : 0x%08x)", 0, macho::stringutils::convert_ansi_to_unicode(path).c_str(), GetLastError());
                    invalid_operation error;
                    error.what_op = saasame::transport::error_codes::SAASAME_E_INTERNAL_FAIL;
                    error.why = boost::str(boost::format("Cannot get volume bitmap for volume %s.") % path);
                    throw error;
                }
                else{
                    ULONGLONG total_number_of_delta = 0;
                    ULONG     delta_mini_block_size = _delta_mini_block_size < bitmap.bytes_per_cluster ? bitmap.bytes_per_cluster : _delta_mini_block_size;
                    ULONG     full_mini_block_size = _full_mini_block_size < bitmap.bytes_per_cluster ? bitmap.bytes_per_cluster : _full_mini_block_size;
                    ULONGLONG file_area_offset = 0;
                    if (IsWindows7OrGreater()){
                        RETRIEVAL_POINTER_BASE retrieval_pointer_base;
                        if (!(Result = DeviceIoControl(
                            handle,
                            FSCTL_GET_RETRIEVAL_POINTER_BASE,
                            NULL,
                            0,
                            &retrieval_pointer_base,
                            sizeof(retrieval_pointer_base),
                            &BytesReturned,
                            NULL))){
                            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(FSCTL_GET_RETRIEVAL_POINTER_BASE) failed for %s. (Error : 0x%08x)", macho::stringutils::convert_ansi_to_unicode(path).c_str(), GetLastError());
                        }
                        else{
                            file_area_offset = retrieval_pointer_base.FileAreaOffset.QuadPart * BytesPerSector;
                        }
                    }
                    else{
                        get_fat_first_sector_offset(handle, file_area_offset);
                    }
                    {
                        bitmap.file_area_offset = file_area_offset;
                        boost::shared_ptr<VOLUME_BITMAP_BUFFER> pBitmap = get_journal_bitmap(file_area_offset, path, BytesPerCluster, previous_journal, 0);
                        if (pBitmap){
                            bitmap.is_delta = true;
                            _mini_block_size = delta_mini_block_size;
                            _get_changed_areas(file_area_offset, bitmap.bitmap->Buffer, bitmap.bitmap->StartingLcn.QuadPart, bitmap.bitmap->BitmapSize.QuadPart, BytesPerCluster, total_number_of_delta);
                            LOG(LOG_LEVEL_RECORD, L"Journal Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);
                            if (previous_journal.id != 0){
                                for (uint64_t bit = 0; bit < (uint64_t)bitmap.bitmap->BitmapSize.QuadPart; bit++){
                                    bitmap.bitmap->Buffer[bit >> 3] &= pBitmap->Buffer[bit >> 3];
                                }
                            }
                        }
                        else{
                            _mini_block_size = full_mini_block_size;
                        }
                    }

                    if (_create_job_detail.file_system_filter_enable){
                        _get_changed_areas(file_area_offset, bitmap.bitmap->Buffer, bitmap.bitmap->StartingLcn.QuadPart, bitmap.bitmap->BitmapSize.QuadPart, BytesPerCluster, total_number_of_delta);
                        LOG(LOG_LEVEL_RECORD, L"Before excluding, Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);
                        sync_file(bitmap.bitmap->Buffer, boost::filesystem::path(path) / L"pagefile.sys", true, true);
                        sync_file(bitmap.bitmap->Buffer, boost::filesystem::path(path) / L"hiberfil.sys", true, true);
                        sync_file(bitmap.bitmap->Buffer, boost::filesystem::path(path) / L"swapfile.sys", true, true);
                        sync_file(bitmap.bitmap->Buffer, boost::filesystem::path(path) / L"windows" / L"memory.dmp", true, true);
                        foreach(std::wstring file, resync_paths){
                            boost::filesystem::path _p = boost::filesystem::path(path) / file.substr(1);
                            LOG(LOG_LEVEL_RECORD, L"Resync \"%s\"", _p.wstring().c_str());
                            if (boost::filesystem::exists(_p)){
                                if (boost::filesystem::is_directory(_p)){
                                    std::vector<boost::filesystem::path> _files = environment::get_files(_p, _T("*"), true);
                                    foreach(boost::filesystem::path& f, _files){
                                        sync_file(bitmap.bitmap->Buffer, f, false, false);
                                    }
                                }
                                else{
                                    sync_file(bitmap.bitmap->Buffer, _p, false, false);
                                }
                            }
                        }
                        boost::filesystem::path excluded_file = boost::filesystem::path(path) / L".excluded";
                        foreach(std::wstring file, excluded_paths){
                            boost::filesystem::path _p = boost::filesystem::path(path) / file.substr(1);
                            LOG(LOG_LEVEL_RECORD, L"Exclude \"%s\"", _p.wstring().c_str());
                            if (boost::filesystem::exists(_p)){
                                if (boost::filesystem::is_directory(_p)){
                                    std::vector<boost::filesystem::path> _files = environment::get_files(_p, _T("*"), true);
                                    foreach(boost::filesystem::path& f, _files){
                                        if (f != excluded_file)
                                            sync_file(bitmap.bitmap->Buffer, f, false, true);
                                    }
                                }
                                else if (_p != excluded_file){
                                    sync_file(bitmap.bitmap->Buffer, _p, false, true);
                                }
                            }
                        }
                        std::vector<boost::filesystem::path> files = environment::get_files(boost::filesystem::path(path) / L"System Volume Information", L"*{3808876b-c176-4e48-b7ae-04046e6cc752}");
                        if (files.size()){
                            universal_disk_rw::ptr rw = universal_disk_rw::ptr(new general_io_rw(handle, macho::stringutils::convert_ansi_to_unicode(path)));
                            ntfs::volume::ptr volume = ntfs::volume::get(rw, 0ULL);
                            if (volume){
                                ntfs::file_record::ptr system_volume_information = volume->find_sub_entry(L"System Volume Information");
                                if (system_volume_information){
                                    foreach(boost::filesystem::path &file, files){
                                        ntfs::file_record::ptr _file = volume->find_sub_entry(file.filename().wstring(), system_volume_information);
                                        if (_file){
                                            ntfs::file_extent::vtr _exts = _file->extents();
                                            foreach(auto e, _exts){
                                                LOG(LOG_LEVEL_INFO, L"(%s): %I64u(%I64u):%I64u", file.wstring().c_str(), e.start, e.physical, e.length);
                                                if (ULLONG_MAX != e.physical)
                                                    clear_umap(bitmap.bitmap->Buffer, e.physical, e.length, BytesPerCluster);
                                            }
                                            LOG(LOG_LEVEL_RECORD, L"Excluded the data replication for the file (%s).", file.wstring().c_str());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

replication_block::vtr physical_packer_job::get_replication_blocks(storage::partition& p, std::string path, uint64_t start, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths, std::set<std::wstring> resync_paths){
    FUN_TRACE;
    replication_block::vtr results;
    if (path[path.length() - 1] == '\\')
        path.erase(path.length() - 1);
    if (temp_drive_letter::is_drive_letter(path))
        path = boost::str(boost::format("\\\\?\\%s") % path);
    volume_bitmap bitmap;
    get_volume_bitmap(bitmap, path, previous_journal, excluded_paths, resync_paths);
    if (bitmap.bitmap){
        uint64_t total_number_of_delta = 0;
        changed_area::vtr changeds = _get_changed_areas(bitmap.file_area_offset, bitmap.bitmap->Buffer, bitmap.bitmap->StartingLcn.QuadPart, bitmap.bitmap->BitmapSize.QuadPart, bitmap.bytes_per_cluster, total_number_of_delta);
        LOG(LOG_LEVEL_RECORD, L"After excluding, Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);
        if (start){
            for (changed_area::vtr::iterator it = changeds.begin(); it != changeds.end() /* !!! */;){
                if ((p.offset() + it->start_offset + it->length) < start){
                    it = changeds.erase(it);  // Returns the new iterator to continue from.
                }
                else{
                    ++it;
                }
            }
        }
        boost::shared_ptr<std::string> _path(new std::string(path));
        foreach(changed_area &c, changeds){
            replication_block::ptr b(new replication_block(0, c.start_offset, c.length, p.offset() + c.start_offset));
            b->path = _path;
            results.push_back(b);
        }
    }
    return results;
}

replication_block::map physical_packer_job::get_replication_blocks(storage::volume& v, std::string path, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths, std::set<std::wstring> resync_paths){
    FUN_TRACE;
    replication_block::vtr results;
    if (path[path.length() - 1] == '\\')
        path.erase(path.length() - 1);
    volume_bitmap bitmap;
    get_volume_bitmap(bitmap, path, previous_journal, excluded_paths, resync_paths);
    if (bitmap.bitmap){
        uint64_t total_number_of_delta = 0;
        changed_area::vtr changeds = _get_changed_areas(bitmap.file_area_offset, bitmap.bitmap->Buffer, bitmap.bitmap->StartingLcn.QuadPart, bitmap.bitmap->BitmapSize.QuadPart, bitmap.bytes_per_cluster, total_number_of_delta);
        LOG(LOG_LEVEL_RECORD, L"After excluding, Delta Size for volume(%s): %I64u", macho::stringutils::convert_ansi_to_unicode(path).c_str(), total_number_of_delta);
        return _get_dynamic_volumes_replication_blocks(v, changeds, _block_size, path);
    }
    return replication_block::map();
}

void physical_packer_job::calculate_backup_size(macho::windows::storage::disk& d, physical_packer_job_progress& p, replication_block::vtr& blocks){
    FUN_TRACE;
    if (p.backup_size == 0){
        foreach(replication_block::ptr &c, blocks)
            p.backup_size += c->length;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Backup size estimate of disk %1% : %2%.") % d.number() % p.backup_size));
    }
}

bool physical_packer_job::replicate_disk(macho::windows::storage::disk& d, replication_block::vtr& blocks, physical_packer_job_progress& p){
    FUN_TRACE;
    bool result = false;
    if (p.is_completed())
        return true;
    _state &= ~mgmt_job_state::job_state_error;
    universal_disk_rw::vtr outputs;
    int32_t retry_count = 1;
    do{
        try{
            if (p.backup_image_offset > 0){
                carrier_rw::open_image_parameter parameter;
                parameter.carriers = _create_job_detail.carriers;
                parameter.priority_carrier = _create_job_detail.priority_carrier;
                parameter.base_name = p.base;
                parameter.name = p.output;
                parameter.session = _create_job_detail.detail.p.snapshots[0].snapshot_set_id;
                parameter.timeout = _create_job_detail.timeout;
                parameter.encrypted = _create_job_detail.is_encrypted;
                parameter.polling = g_is_pooling_mode;
                outputs = carrier_rw::open(parameter);
            }
            else{
                carrier_rw::create_image_parameter parameter;
                parameter.carriers = _create_job_detail.carriers;
                parameter.priority_carrier = _create_job_detail.priority_carrier;
                parameter.base_name = p.base;
                parameter.name = p.output;
                parameter.size = d.size();
                parameter.parent = p.parent;
                parameter.checksum_verify = _create_job_detail.checksum_verify;
                parameter.session = _create_job_detail.detail.p.snapshots[0].snapshot_set_id;
                parameter.timeout = _create_job_detail.timeout;
                parameter.encrypted = _create_job_detail.is_encrypted;
                parameter.comment = boost::str(boost::format("%1%") % d.number());
                parameter.checksum = _create_job_detail.is_checksum;
                parameter.compressed = _create_job_detail.is_compressed;
                parameter.polling = g_is_pooling_mode;
                parameter.cdr = is_cdr();
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
        catch (universal_disk_rw::exception &e){
            _state |= mgmt_job_state::job_state_error;
            outputs.clear();
        }
        if (0 == outputs.size()){
            if (p.backup_image_offset > 0)
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, (record_format("Cannot open disk image %1%.") % p.output ));
            else
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, (record_format("Cannot create disk image %1%.") % p.output));
        }
        else{
            retry_count = 1;
            try{
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup started.") % d.number()));
                universal_disk_rw::ptr rw = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % d.number()));
                if (!rw){
                    _state |= mgmt_job_state::job_state_error;
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                }
                else{
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
                    replication_repository repository(p,p.backup_progress, p.backup_image_offset);
                    repository.block_size = _block_size;
                    repository.queue_size = _queue_size;
                    repository.compressed_by_packer = _create_job_detail.is_compressed_by_packer;
                    uint64_t index = 0;
                    std::sort(blocks.begin(), blocks.end(), replication_block::compare());
                    foreach(auto e, blocks){
                        e->index = index ++;
                        if (e->offset >= p.backup_image_offset)
                            repository.waitting.push_back(e);
                    }
                    if (repository.waitting.size()){
                        while (worker_thread_number > 0){
                            boost::thread_group    thread_pool;
                            replication_task::vtr  tasks;
                            repository.terminated = false;
                            _state &= ~mgmt_job_state::job_state_error;

                            if (worker_thread_number > repository.waitting.size())
                                worker_thread_number = repository.waitting.size();

                            for (int i = 0; i < worker_thread_number; i++){
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
                                    task->register_job_is_canceled_function(boost::bind(&physical_packer_job::is_canceled, this));
                                    task->register_job_save_callback_function(boost::bind(&physical_packer_job::save_status, this));
                                    task->register_job_record_callback_function(boost::bind(&physical_packer_job::record, this, _1, _2, _3));
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
                                            in = rw;
                                            if (in && outs.size())
                                            {
                                                replication_task::ptr task(new replication_task(in, outs, repository));
                                                task->register_job_is_canceled_function(boost::bind(&physical_packer_job::is_canceled, this));
                                                task->register_job_save_callback_function(boost::bind(&physical_packer_job::save_status, this));
                                                task->register_job_record_callback_function(boost::bind(&physical_packer_job::record, this, _1, _2, _3));
                                                tasks.push_back(task);
                                                thread_pool.create_thread(boost::bind(&replication_task::replicate, &(*tasks[tasks.size() - 1])));
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
                        macho::windows::auto_lock lock(p.lock);
                        p.backup_image_offset = d.size();
                        foreach(universal_disk_rw::ptr o, outputs){
                            carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                            if (rw)
                            {
                                if (!(result = rw->close()))
                                {
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d.number()));
                                    _state |= mgmt_job_state::job_state_error;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d.number()));
                    }
                    if (result)
                    {
                        _state &= ~mgmt_job_state::job_state_error;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup completed with size : %2%") % d.number() % p.backup_size));
                        break;
                    }
                }
            }
            catch (invalid_operation& error){
                result = false;
                LOG(LOG_LEVEL_RECORD, L"Cannot backup disk %d: invalid_operation(%s)", d.number(), macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d.number()));
            }
            catch (...){
                LOG(LOG_LEVEL_RECORD, L"Cannot backup disk %d: unknown error.", d.number());
                result = false;
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d.number()));
            }
        }
        save_status();
        LOG(LOG_LEVEL_RECORD, L"result : %s , is_canceled : %s , retry_count : %d", result ? L"true" : L"false", is_canceled() ? L"true" : L"false", retry_count);
    } while ((!result) && (!is_canceled()) && ((retry_count--) > 0));
    return result;
}

replication_block::map continuous_data_replication::get_changeds(std::map<std::string, storage::disk::ptr> disks, std::map<int64_t, physical_vcbt_journal>& previous_journals){
    FUN_TRACE;
    typedef std::map<std::string, storage::disk::ptr> disk_key_map;
    _journals = previous_journals;
    foreach(disk_key_map::value_type _d, disks){
        _get_changed_blocks_tracks(*_d.second, previous_journals);
    }
    return _changed_areas_map;
}

bool continuous_data_replication::changed_blocks_track::read_file(HANDLE handle, uint32_t start, uint32_t length, LPBYTE buffer, DWORD& number_of_bytes_read){
    FUN_TRACE;
    bool result = false;
    uint64_t _start = start;
    uint64_t __end = start + length;
    foreach(journal_block& j, journal_blocks){
        uint64_t _end = j.start + j.length;
        if (j.start <= _start && _end > _start){
            uint64_t _offset = j.lcn + (_start - j.start) + file_area_offset;
            uint64_t _length = _end > __end ? length : _end - _start;
            {
                OVERLAPPED overlapped;
                memset(&overlapped, 0, sizeof(overlapped));
                DWORD opStatus = ERROR_SUCCESS;
                LARGE_INTEGER offset;
                offset.QuadPart = _offset;
                overlapped.Offset = offset.LowPart;
                overlapped.OffsetHigh = offset.HighPart;
                DWORD _number_of_bytes_read = 0;
                if (!ReadFile(
                    handle,
                    &(buffer[_start - start]),
                    (DWORD)_length,
                    (LPDWORD)&_number_of_bytes_read,
                    &overlapped)){
                    if (ERROR_IO_PENDING == (opStatus = GetLastError())){
                        if (!GetOverlappedResult(handle, &overlapped, (LPDWORD)&_number_of_bytes_read, TRUE)){
                            opStatus = GetLastError();
                        }
                    }
                    if (opStatus != ERROR_SUCCESS){
                        LOG(LOG_LEVEL_ERROR, _T(" (%s) error = %u"), volume_path.c_str(), opStatus);
                        return false;
                    }
                }
                else{
                    result = true;
                }
                number_of_bytes_read += _number_of_bytes_read;
            }

            if (__end <= _end)
                break;
            _start += _length;
        }
    }
    return result;
}

changed_area::vtr continuous_data_replication::changed_blocks_track::get_changed_areas(const physical_vcbt_journal& previous_journal){
    FUN_TRACE;
    std::wstring path = volume_path;
    if (path[path.length() - 1] == '\\')
        path.erase(path.length() - 1);
    macho::windows::auto_file_handle handle = CreateFileW(path.c_str(),
        GENERIC_READ,                // no access to the drive
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);            // do not copy file attributes      
    if (handle.is_invalid()){
        LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", path.c_str(), GetLastError());
    }
    else{
        changed_area::vtr journal_changed_area;
        bool journal_found = false, terminated = false, previous_latest_key_found = false;
        DWORD read_bytes, number_of_bytes_read = 0;
        DWORD offset = 0;
        DWORD start = 0;
        read_bytes = bytes_per_cluster * 2;
        std::auto_ptr<BYTE> pBuf = std::auto_ptr<BYTE>(new BYTE[bytes_per_cluster * 2]);
        PVCBT_JOURNAL_BLOK pJournalBlock = (PVCBT_JOURNAL_BLOK)pBuf.get();
        while (true){
            if (!read_file(handle, start, read_bytes, &(pBuf.get()[offset]), number_of_bytes_read)){
                DWORD err = GetLastError();
                LOG(LOG_LEVEL_ERROR, _T("ReadFile(%s) error = %u"), volume_path.c_str(), err);
                break;
            }
            start += read_bytes;
            LONG min_size = min((LONG)(number_of_bytes_read + offset), (bytes_per_cluster + sizeof(VCBT_RECORD)));
            for (;
                (BYTE*)pJournalBlock < (pBuf.get() + min_size);
                pJournalBlock = (PVCBT_JOURNAL_BLOK)(((BYTE*)pJournalBlock) + sizeof(VCBT_RECORD))){
                if (offset == 0 && pJournalBlock == (PVCBT_JOURNAL_BLOK)pBuf.get()){
                    vcbt_journal.first_key = pJournalBlock->r.Key;
                    vcbt_journal.lowest_valid_key = pJournalBlock->r.Key;
                }

                if (pJournalBlock->r.Start != 0 && pJournalBlock->r.Length != 0){
                    if (previous_journal.id != 0){
                        if (vcbt_journal.first_key == previous_journal.first_key){
                            if (previous_latest_key_found)
                                journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                        }
                        else{
                            if ((!previous_latest_key_found &&!journal_found) || (previous_latest_key_found &&journal_found))
                                journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                        }
                    }
                    if (((ULONGLONG)vcbt_journal.lowest_valid_key) > pJournalBlock->r.Key)
                        vcbt_journal.lowest_valid_key = pJournalBlock->r.Key;
                }

                if (previous_journal.id != 0 && pJournalBlock->r.Key == previous_journal.latest_key){
                    previous_latest_key_found = true;
                    journal_changed_area.push_back(changed_area(pJournalBlock->r.Start, pJournalBlock->r.Length));
                }

                if ((pJournalBlock->j.CheckSum == (ULONG)-1) &&
                    (pJournalBlock->j.Signature[0] == '1') &&
                    (pJournalBlock->j.Signature[1] == 'c') &&
                    (pJournalBlock->j.Signature[2] == 'b') &&
                    (pJournalBlock->j.Signature[3] == 't')){

                    if (journal_found){
                        break;
                    }
                    else {
                        journal_found = true;
                        vcbt_journal.latest_key = pJournalBlock->r.Key;
                        vcbt_journal.id = pJournalBlock->j.JournalId;
                        // (scenario 1) first........pre_________cbt.......... end (the first key will be equal)
                        // (scenario 2) first________cbt.........pre___________end (the first key will be different)
                        if (vcbt_journal.first_key == previous_journal.first_key){
                            terminated = true;
                            break;
                        }
                    }
                }
                if (journal_found && pJournalBlock->r.Start == 0
                    && pJournalBlock->r.Length == 0
                    && pJournalBlock->r.Key == 0){
                    terminated = true;
                    break;
                }
            }
            if (terminated)
                break;
            read_bytes = offset = bytes_per_cluster;
            MoveMemory(&pBuf.get()[0], &pBuf.get()[bytes_per_cluster], bytes_per_cluster);
            ZeroMemory(&pBuf.get()[bytes_per_cluster], bytes_per_cluster);
            pJournalBlock = (PVCBT_JOURNAL_BLOK)(((BYTE*)pJournalBlock) - bytes_per_cluster);
        }
        if (journal_found){
            changed_area::vtr chunks = final_changed_area(journal_changed_area);
            foreach(changed_area &c, chunks){
                c.start_offset = c.start_offset + partition_offset;
            }
            return chunks;
        }
    }
    return changed_area::vtr();
}

changed_area::vtr continuous_data_replication::_get_journal_changed_blocks(HANDLE handle, const std::wstring& path, uint64_t offset, std::map<int64_t, physical_vcbt_journal>&  previous_journals, continuous_data_replication::changed_blocks_track::vtr& tracks){
    FUN_TRACE;
    std::wstring::size_type first = path.find_first_of(L"{");
    std::wstring::size_type last = path.find_last_of(L"}");
    DWORD err = ERROR_SUCCESS;
    if (first != std::wstring::npos && last != std::wstring::npos){
        macho::guid_ volume_id(path.substr(first + 1, last - first - 1));
        VCBT_RUNTIME_COMMAND cmd;
        DWORD output_length = sizeof(VCBT_RUNTIME_RESULT);
        std::auto_ptr<VCBT_RUNTIME_RESULT> result = std::auto_ptr<VCBT_RUNTIME_RESULT>((PVCBT_RUNTIME_RESULT)new BYTE[output_length]);
        cmd.VolumeId = volume_id;
        cmd.Flag = JOURNAL;
        if (!(DeviceIoControl(handle,                       // device to be queried
            IOCTL_VCBT_RUNTIME,                        // operation to perform
            &cmd, sizeof(VCBT_RUNTIME_COMMAND),
            (LPVOID)result.get(),
            output_length,                   // # bytes returned
            &output_length,
            (LPOVERLAPPED)NULL)))          // synchronous I/O
        {
            err = GetLastError();
            LOG(LOG_LEVEL_ERROR, L"Failed to DeviceIoControl(IOCTL_VCBT_RUNTIME), Error(0x%08x).", err);
        }
        else{
            changed_blocks_track::ptr track(new changed_blocks_track());
            track->partition_offset = offset;
            track->volume_path = path;
            track->bytes_per_sector = result->BytesPerSector;
            track->file_area_offset = (uint32_t)result->FileAreaOffset;
            track->bytes_per_cluster = result->FsClusterSize;
            track->first_key = result->JournalMetaData.FirstKey;
            track->latest_key = result->JournalMetaData.Block.r.Key;
            track->journal_id = result->JournalMetaData.Block.j.JournalId;
            LARGE_INTEGER               liVcnPrev = result->RetrievalPointers.StartingVcn;
            for (DWORD extent = 0; extent < result->RetrievalPointers.ExtentCount; extent++){
                LONGLONG _Start = liVcnPrev.QuadPart * result->FsClusterSize;
                LONGLONG _Length = (LONGLONG)(result->RetrievalPointers.Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * result->FsClusterSize;
                LONGLONG _Lcn = result->RetrievalPointers.Extents[extent].Lcn.QuadPart * result->FsClusterSize;
                track->journal_blocks.push_back(changed_blocks_track::journal_block(_Start, _Length, _Lcn));
                liVcnPrev = result->RetrievalPointers.Extents[extent].NextVcn;
            }
            if (previous_journals.count(track->journal_id)){
                changed_area::vtr changeds = track->get_changed_areas(previous_journals[track->journal_id]);
                if (changeds.size()){
                    _journals[track->journal_id] = track->vcbt_journal;
                    tracks.push_back(track);
                    return changeds;
                }
            }
        }
    }
    return changed_area::vtr();
}

continuous_data_replication::changed_blocks_track::vtr continuous_data_replication::_get_changed_blocks_tracks(storage::disk& d, std::map<int64_t, physical_vcbt_journal>& previous_journals){
    FUN_TRACE;
    changed_blocks_track::vtr tracks;
    BOOL bResult = FALSE;                 // results flag
    DWORD err = ERROR_SUCCESS;
    auto_handle _handle = CreateFileW(VCBT_WIN32_DEVICE_NAME,          // drive to open
        0,                // no access to the drive
        FILE_SHARE_READ | // share mode
        FILE_SHARE_WRITE,
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes
    if (_handle.is_invalid()){
        LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", VCBT_WIN32_DEVICE_NAME, GetLastError());
    }
    else{
        if (d.is_dynmaic()){
            macho::windows::storage::volume::vtr volumes = d.get_volumes();
            foreach(macho::windows::storage::volume::ptr vol, volumes){
                if (vol->type() != macho::windows::storage::ST_VOLUME_TYPE::ST_VT_MIRROR &&
                    vol->type() != macho::windows::storage::ST_VOLUME_TYPE::ST_VT_PARITY &&
                    !_processed_dynamic_volumes.count(vol->path())){
                    foreach(std::wstring path, vol->access_paths()){
                        if ((path[0] == _T('\\')) &&
                            (path[1] == _T('\\')) &&
                            (path[2] == _T('?')) &&
                            (path[3] == _T('\\'))){
                            changed_area::vtr changed_areas = _get_journal_changed_blocks(_handle, path, 0, previous_journals, tracks);
                            _processed_dynamic_volumes[vol->path()] = true;
                            if (path[path.length() - 1] == '\\')
                                path.erase(path.length() - 1);
                            replication_block::map changeds = _get_dynamic_volumes_replication_blocks(*vol, changed_areas, 4 * 1024 * 1024, "");
                            foreach(replication_block::map::value_type& c, changeds){
                                _changed_areas_map[c.first].insert(_changed_areas_map[c.first].end(), c.second.begin(), c.second.end());
                            }
                            break;
                        }
                    }
                }
            }
        }
        else{
            changed_area::vtr disk_changed_areas;
            macho::windows::storage::partition::vtr partitions = d.get_partitions();
            foreach(macho::windows::storage::partition::ptr part, partitions){
                foreach(std::wstring path, part->access_paths()){
                    if ((path[0] == _T('\\')) &&
                        (path[1] == _T('\\')) &&
                        (path[2] == _T('?')) &&
                        (path[3] == _T('\\'))){
                        changed_area::vtr _changed_areas = _get_journal_changed_blocks(_handle, path, part->offset(), previous_journals, tracks);
                        disk_changed_areas.insert(disk_changed_areas.end(), _changed_areas.begin(), _changed_areas.end());
                        break;
                    }
                }
            }
            if (disk_changed_areas.size()){
                foreach(changed_area &c, disk_changed_areas){
                    _changed_areas_map[d.number()].push_back(replication_block::ptr(new replication_block(0, c.start_offset, c.length, c.start_offset)));
                }
            }
        }
        if (_changed_areas_map[d.number()].empty()){
            _changed_areas_map[d.number()].push_back(replication_block::ptr(new replication_block(0, 0, 4096, 0)));
        }
    }
    return tracks;
}

changed_area::vtr continuous_data_replication::changed_blocks_track::final_changed_area(changed_area::vtr& src, uint64_t max_size){
    FUN_TRACE;
    changed_area::vtr chunks;
    if (src.empty() || 1 == src.size())
        return src;
    else
    {
        std::sort(src.begin(), src.end(), [](changed_area const& lhs, changed_area const& rhs){ return (lhs.start_offset + lhs.length) < (rhs.start_offset + rhs.length); });
        uint32_t byte_per_bit = bytes_per_cluster;
        uint32_t mini_number_of_bits = bytes_per_cluster / byte_per_bit;
        uint32_t number_of_bits = max_size / byte_per_bit;
        uint64_t size = src[src.size() - 1].start_offset + src[src.size() - 1].length;
        uint64_t total_number_of_bits = ((size - 1) / byte_per_bit) + 1;
        uint64_t buff_size = ((total_number_of_bits - 1) / 8) + 1;
        std::auto_ptr<BYTE> buf(new BYTE[buff_size]);
        memset(buf.get(), 0, buff_size);
        foreach(auto s, src){
            set_umap(buf.get(), s.start_offset, s.length, byte_per_bit);
        }
        uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
        bool     dirty_range = false;
        for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++){
            if (buf.get()[newBit >> 3] & (1 << (newBit & 7))){
                uint64_t num = (newBit - oldBit);
                if (!dirty_range){
                    dirty_range = true;
                    oldBit = next_dirty_bit = newBit;
                }
                else if (num == number_of_bits){
                    chunks.push_back(changed_area(oldBit * byte_per_bit, num * byte_per_bit));
                    oldBit = next_dirty_bit = newBit;
                }
                else{
                    next_dirty_bit = newBit;
                }
            }
            else {
                if (dirty_range && ((newBit - oldBit) >= mini_number_of_bits)){
                    uint64_t lenght = (next_dirty_bit - oldBit + 1) * byte_per_bit;
                    chunks.push_back(changed_area(oldBit * byte_per_bit, lenght));
                    dirty_range = false;
                }
            }
        }
        if (dirty_range){
            chunks.push_back(changed_area(oldBit * byte_per_bit, (next_dirty_bit - oldBit + 1) * byte_per_bit));
        }
    }
    return chunks;
}

#ifndef _WIN2K3

#include <ntddvol.h>

#else

//
// IOCTL_VOLUME_LOGICAL_TO_PHYSICAL
//
// Input Buffer:
//     Structure of type VOLUME_LOGICAL_OFFSET
//
// Output Buffer:
//     Structure of type VOLUME_PHYSICAL_OFFSETS
//

typedef struct _VOLUME_LOGICAL_OFFSET {

    //
    // Specifies the logical location
    // that needs to be translated to
    // a physical location.
    //
    LONGLONG LogicalOffset;

} VOLUME_LOGICAL_OFFSET, *PVOLUME_LOGICAL_OFFSET;

typedef struct _VOLUME_PHYSICAL_OFFSET {

    //
    // Specifies the physical location
    // that needs to be  translated to
    // a logical location.
    //
    ULONG DiskNumber;
    LONGLONG Offset;

} VOLUME_PHYSICAL_OFFSET, *PVOLUME_PHYSICAL_OFFSET;

typedef struct _VOLUME_PHYSICAL_OFFSETS {

    //
    // Specifies one or more physical
    // locations on which the logical
    // location resides.
    //
    ULONG NumberOfPhysicalOffsets;
    VOLUME_PHYSICAL_OFFSET PhysicalOffset[ANYSIZE_ARRAY];

} VOLUME_PHYSICAL_OFFSETS, *PVOLUME_PHYSICAL_OFFSETS;

#define IOCTL_VOLUME_LOGICAL_TO_PHYSICAL        CTL_CODE(IOCTL_VOLUME_BASE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif

BOOL get_volume_logical_to_physical(HANDLE hVol, LONGLONG VolumeOffset, boost::shared_ptr<VOLUME_PHYSICAL_OFFSETS>& pPhysicalOffsets, LONGLONG &cbOffsets){
    FUN_TRACE;
    BOOL                                       status;
    VOLUME_LOGICAL_OFFSET                      LogicalOffset;
    ULONG                                      err;
    ULONG                                      returnedLength;
    LogicalOffset.LogicalOffset = VolumeOffset;
    
    while (!(status= DeviceIoControl(hVol,
        IOCTL_VOLUME_LOGICAL_TO_PHYSICAL,
        &LogicalOffset,
        sizeof(VOLUME_LOGICAL_OFFSET),
        pPhysicalOffsets.get(),
        (DWORD)cbOffsets,
        &returnedLength,
        0))){
        DWORD err = GetLastError();
        if (ERROR_MORE_DATA == err || ERROR_BUFFER_OVERFLOW == err){
            cbOffsets *= 10;
            pPhysicalOffsets = boost::shared_ptr<VOLUME_PHYSICAL_OFFSETS>((PVOLUME_PHYSICAL_OFFSETS)new BYTE[(DWORD)cbOffsets]);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"DeviceIoControl(IOCTL_VOLUME_LOGICAL_TO_PHYSICAL) failed. Error: %d", err);
            break;
        }
    }
    return status;
}

replication_block::map _get_dynamic_volumes_replication_blocks(storage::volume& v, const changed_area::vtr& changeds, uint32_t block_size, std::string volume_path){
    FUN_TRACE;
    replication_block::map results;
    foreach(std::wstring path, v.access_paths()){
        if ((path[0] == _T('\\')) &&
            (path[1] == _T('\\')) &&
            (path[2] == _T('?')) &&
            (path[3] == _T('\\'))){
            if (path[path.length() - 1] == '\\')
                path.erase(path.length() - 1);
            macho::windows::auto_file_handle hVolume = CreateFileW(path.c_str(),
                FILE_READ_ATTRIBUTES | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_NO_BUFFERING, // no buffering
                NULL);
            if (hVolume.is_invalid()){
                LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", path.c_str(), GetLastError());
            }
            else{
                NTFS_VOLUME_DATA_BUFFER     VolumeData;
                DWORD                       cbOutput;
                if (!DeviceIoControl(
                    hVolume,
                    FSCTL_GET_NTFS_VOLUME_DATA, // dwIoControlCode
                    NULL,
                    0,
                    &VolumeData,
                    sizeof(VolumeData),
                    &cbOutput,
                    NULL
                    )){
                    LOG(LOG_LEVEL_ERROR, L"Failed to FSCTL_GET_NTFS_VOLUME_DATA(%s), Error(0x%08x).", path.c_str(), GetLastError());
                }
                else{
                    boost::shared_ptr<VOLUME_PHYSICAL_OFFSETS> pPhysicalOffsets;
                    LONGLONG cbOffsets = sizeof(VOLUME_PHYSICAL_OFFSETS);
                    pPhysicalOffsets = boost::shared_ptr<VOLUME_PHYSICAL_OFFSETS>((PVOLUME_PHYSICAL_OFFSETS)new BYTE[(DWORD)cbOffsets]);
                    if (NULL == pPhysicalOffsets) {
                        LOG(LOG_LEVEL_ERROR, L"Failed to allocate Offsets buffer . Error: %d", GetLastError());
                    }
                    else{
                        uint64_t                    start = 0, end = 0;
                        ULONG                       i;
                        PVOLUME_PHYSICAL_OFFSET     PhysicalOffset;
                        foreach(const changed_area &c, changeds){
                            start = c.start_offset;
                            end = c.start_offset + c.length;
                            while (start < end){
                                if (get_volume_logical_to_physical(hVolume, start, pPhysicalOffsets, cbOffsets)){
                                    for (i = 0; i < pPhysicalOffsets->NumberOfPhysicalOffsets; i++) {
                                        PhysicalOffset = &pPhysicalOffsets->PhysicalOffset[i];
                                        replication_block::vtr& result = results[PhysicalOffset->DiskNumber];
                                        if (result.empty()){
                                            result.push_back(replication_block::ptr(new replication_block(0, start, VolumeData.BytesPerCluster, PhysicalOffset->Offset)));
                                        }
                                        else{
                                            replication_block::ptr r = result[result.size() - 1];
                                            if (start == r->End() && PhysicalOffset->Offset == r->RealEnd() && block_size > r->length){
                                                r->length += VolumeData.BytesPerCluster;
                                            }
                                            else{
                                                result.push_back(replication_block::ptr(new replication_block(0, start, VolumeData.BytesPerCluster, PhysicalOffset->Offset)));
                                            }
                                        }
                                    }
                                    start += VolumeData.BytesPerCluster;
                                }
                                else{
                                    results.clear();
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    boost::shared_ptr<std::string> _path(new std::string(volume_path));
    foreach(replication_block::map::value_type& r, results){
        foreach(replication_block::ptr& v, r.second){
            if (volume_path.empty())
                v->start = v->offset;
            else
                v->path = _path;
        }
    }
    return results;
}