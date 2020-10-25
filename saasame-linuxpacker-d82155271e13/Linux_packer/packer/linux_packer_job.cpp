#include "linux_packer_job.h"
#include "carrier_rw.h"

#include "../tools/clone_disk.h"
#include "../tools/system_tools.h"
#include "../tools/sslHelper.h"
#include "../tools/log.h"
#include "time.h"
#include <thrift/transport/TSSLSocket.h>
#include <memory>
#include <errno.h>
#include <signal.h>
#include <boost/pool/detail/guard.hpp>
#include "physical_packer_service_handler.h"
#include "lz4.h"
#include "lz4hc.h"
//#define CHECKTIME
//#define WRITE_FAIL_TEST

using namespace linux_storage;
extern bool b_polling_mode;

#define STRING_BUF


struct replication_repository {
    typedef boost::shared_ptr<replication_repository> ptr;
    replication_repository(uint64_t &_offset, physical_packer_job_progress::ptr _progress, uint64_t _target = 0) :
        offset(_offset),
        progress(_progress),
        target(_target),
        terminated(false),
        liveThreadNumber(0),
        block_size(CARRIER_RW_IMAGE_BLOCK_SIZE),
        queue_size(CARRIER_RW_IMAGE_BLOCK_SIZE * 4) {}
    boost::recursive_mutex                                    cs;
    boost::recursive_mutex                                    cs2;
    replication_block::queue                                  waitting;
    replication_block::vtr                                    processing;
    replication_block::queue                                  error;
    boost::mutex                                              completion_lock;
    boost::condition_variable                                 completion_cond;
    boost::mutex                                              pending_lock;
    boost::condition_variable                                 pending_cond;
    uint64_t&                                                 offset;
    physical_packer_job_progress::ptr                         progress;
    uint32_t                                                  block_size;
    uint32_t                                                  queue_size;
    bool                                                      terminated;
    bool                                                      compressed;
    uint64_t                                                  target;
    uint32_t                                                  liveThreadNumber;
};

template<typename T>
struct _or
{
    typedef T result_type;
    template<typename InputIterator>
    T operator()(InputIterator first, InputIterator last) const
    {
        // If there are no slots to call, just return the
        // default-constructed value
        if (first == last) return T(false);
        T value = *first++;
        while (first != last) {
            value = value || *first;
            ++first;
        }
        return value;
    }
};

#define TASK_SUCCESS 0
#define TASK_FAIL    1
struct replication_task {
    typedef boost::shared_ptr<replication_task> ptr;
    typedef std::vector<ptr> vtr;
    
    replication_task(/*universal_disk_rw::ptr _in,*/ universal_disk_rw::vtr _outs, replication_repository& _repository, snapshot_manager::ptr sh) : /*in(_in),*/ outs(_outs), repository(_repository), b_force_zero_broken_sector(false),status(TASK_SUCCESS),_sh(sh) {
#ifdef WRITE_FAIL_TEST
        test_fault_ratio = 0; 
#endif
    }
    typedef boost::signals2::signal<bool(), _or<bool>> job_is_canceled;
    typedef boost::signals2::signal<void()> job_save_statue;
    typedef boost::signals2::signal<void(saasame::transport::job_state::type state, int error, record_format format)> job_record;
    inline void register_job_is_canceled_function(job_is_canceled::slot_type slot) {
        is_canceled.connect(slot);
    }
    inline void register_job_save_callback_function(job_save_statue::slot_type slot) {
        save_status.connect(slot);
    }
    inline void register_job_record_callback_function(job_record::slot_type slot) {
        record.connect(slot);
    }
    replication_repository&                                     repository;
    int                                                         index;
    universal_disk_rw::vtr                                      outs;
    job_is_canceled                                             is_canceled;
    job_save_statue                                             save_status;
    job_record                                                  record;
    void                                                        replicate();
    uint32_t                                                    code;
    std::string                                                 message;
    bool                                                        b_force_zero_broken_sector;
    snapshot_manager::ptr                                       _sh;
#ifdef WRITE_FAIL_TEST
    int                                                         test_fault_ratio;
#endif
    uint32_t                                                    status;
};
//#define ENABLE_REPLICATE_TRACE_LOG
#ifdef ENABLE_REPLICATE_TRACE_LOG
#define LOG_TRACE_REP(fmt, ...) \
	LOG_TRACE(fmt,##__VA_ARGS__)
#else
#define LOG_TRACE_REP(fmt, ...)
#endif

void replication_task::replicate()
{
    bool completed_queue_change = false;
    {
        boost::unique_lock<boost::recursive_mutex> lock(repository.cs);
        repository.liveThreadNumber++;
    }
    bool                             result = true;
#ifdef STRING_BUF
    std::string                      buf;
    std::string                      buf_dest;
#else
    std::unique_ptr<char>   buf_old(new char[repository.block_size]);
    std::unique_ptr<char>   buf_dest_old(new char[repository.block_size]);
    std::unique_ptr<char>   buf_valid_old(new char[repository.block_size]);
    std::unique_ptr<char>   buf_valid_old2(new char[repository.block_size]);
#endif
    replication_block::ptr           block;
    bool                             pendding = false;
    uint32_t number_of_bytes_to_read = 0;
    uint32_t number_of_bytes_to_written = 0;
    uint32_t number_of_sector_to_read;
    uint64_t target_start;
    uint64_t compressed_length;
    bool need_wait_for_queue;
    replication_block::vtr::iterator b;
    /*disable SIGPIPE*/
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    signal(SIGPIPE, SIG_IGN);
    struct timespec spec;
#ifdef CHECKTIME
    time_t start_s,start_l, end_s,end_l;
    int64_t first_s = 0, read_s = 0, write_s = 0, afterread_s = 0, afterwrite_s = 0, first_l = 0, read_l = 0, write_l = 0, afterread_l = 0, afterwrite_l = 0;
#endif
#ifdef WRITE_FAIL_TEST
    if(test_fault_ratio)
        srand((unsigned)time(NULL));
#endif
    char * inbuf;
    char * outbuf;

    /*reserve 8M data*/
    buf.reserve(repository.block_size);
    if(repository.compressed)
        buf_dest.reserve(repository.block_size);

    while (!is_canceled() /*&& !repository.terminated*/) {
        {
#ifdef CHECKTIME
            clock_gettime(CLOCK_REALTIME, &spec);
            start_s = spec.tv_sec;
            start_l = spec.tv_nsec;
#endif
            //LOG_TRACE("index = %d , 7", index);
            boost::unique_lock<boost::recursive_mutex> lock(repository.cs);
            //LOG_TRACE("index = %d , 8", index);
            if (block) {
                for (b = repository.processing.begin(); b != repository.processing.end(); b++) {
                    //LOG_TRACE("index = %d , (*b)->index = %d block->index = %d", index, (*b)->index, block->index);
                    if ((*b)->index == block->index) {
                        //LOG_TRACE("index = %d , 11", index);
                        repository.processing.erase(b);
                        break;
                    }
                }
                if (!result) {
                    //LOG_TRACE("index = %d , 10", index);
                    //repository.terminated = true;
                    LOG_ERROR("push to error queue block->start = %llu, block->length = %llu\r\n", block->start, block->length);
                    repository.error.push_back(block);
                    save_status(); // is there are error, save status;
                    status = TASK_FAIL;
                    break;
                }
                else
                {
                    //LOG_TRACE("index = %d , 12", index);
                    completed_queue_change = false;
                    {
                        boost::unique_lock<boost::recursive_mutex> lock(repository.progress->lock);
                        repository.progress->completed.push_back(replication_block::ptr(new replication_block(block->index, block->start, block->length, block->start - block->offset, NULL)));
                        //repository.progress->completed.push_back(block);
                        std::sort(repository.progress->completed.begin(), repository.progress->completed.end(), replication_block::compare());
                        while (repository.progress->completed.size() /*&& repository.error.size() == 0*/) {
                            //LOG_TRACE("repository.progress->completed.size() = %d , repository.error.size() = %d", repository.progress->completed.size(), repository.error.size());
                            if (0 == repository.processing.size() || repository.progress->completed.front()->end() < repository.processing[0]->end()) {
                                block = repository.progress->completed.front();
                                repository.progress->completed.pop_front();
                                repository.progress->backup_image_offset = block->start + block->length;
                                repository.progress->backup_progress += block->length;
                                //LOG_TRACE("repository.progress->backup_progress = %llu", repository.progress->backup_progress);
                                completed_queue_change = true;
                            }
                            else
                                break;
                        }
                        if (completed_queue_change)
                            save_status();
                    }
                }
            }

            if (0 == repository.waitting.size())
                break;
            if (0 != repository.error.size())
            {
                block = repository.error.front();
                LOG_ERROR("got block from error queue, block->start = %llu, block->length = %llu\r\n", block->start, block->length);
                repository.error.pop_front();
            }
            else
            {
                block = repository.waitting.front();
                repository.waitting.pop_front();
            }

            //LOG_TRACE("block->start = %llu", block->start);
            repository.processing.push_back(block);
            std::sort(repository.processing.begin(), repository.processing.end(), replication_block::compare());
            if (block->start - repository.processing[0]->start > repository.queue_size)
            {
                //LOG_TRACE("pendding, index = %d, block->start = %llu;, repository.processing[0]->start = %llu, repository.queue_size = %llu\r\n", index, block->start, repository.processing[0]->start, repository.queue_size);
                pendding = true;
            }
            else
            {
                //LOG_TRACE("no pendding, index = %d, block->start = %llu;, repository.processing[0]->start = %llu, repository.queue_size = %llu\r\n", index, block->start, repository.processing[0]->start, repository.queue_size);
                pendding = false;
            }
#ifdef CHECKTIME
            clock_gettime(CLOCK_REALTIME, &spec);
            end_s = spec.tv_sec;
            end_l = spec.tv_nsec;
            first_s += (end_s - start_s);
            if (end_l < start_l)
            {
                first_s--;
                end_l += 1.0e9;
            }
            first_l += (end_l - start_l);
            first_s += first_l / 1.0e9;
            first_l %= (int64_t)1.0e9;
#endif

        
#ifdef CHECKTIME
        clock_gettime(CLOCK_REALTIME, &spec);
        start_s = spec.tv_sec;
        start_l = spec.tv_nsec;
#endif
#ifdef STRING_BUF
        buf.resize(block->length);
        //LOG_TRACE("0buf.capacity() = %llu", buf.capacity());
        inbuf = reinterpret_cast<char*>(&buf[0]);
#else
        inbuf = buf_old.get();
#endif
        LOG_TRACE_REP("read block->start = %llu, block->length = %llu\r\n", block->src_start, block->length);
        result = block->in->read(block->src_start, block->length, inbuf, number_of_bytes_to_read);
        }
        repository.pending_cond.notify_all();
        //number_of_bytes_to_read = block->length;
        if (!result)
        {
#ifdef STRING_BUF
            buf.resize(repository.block_size);
#endif
            LOG_ERROR("read error happen block->start = %llu, block->length = %llu\r\n", block->start, block->length);
            if (b_force_zero_broken_sector) //force zero the broken sector
            {
                LOG_ERROR("force_zero_broken_sector\r\n");
                number_of_sector_to_read = ((number_of_bytes_to_read-1 >> 9) + 1) << 9;
                memset(inbuf + number_of_bytes_to_read, 0, number_of_sector_to_read - number_of_bytes_to_read); //fill zero to the error sector;
                for (int i = number_of_sector_to_read; i < block->length; i+=512)
                {
                    result = block->in->read(block->src_start + i, 512, inbuf + i, number_of_bytes_to_read);
                    if (!result)
                    {
                        record_format rf("read error, block->start = %1% , block->length = %2%, number_of_bytes_to_read = %3%.");
                        rf % (block->src_start + i) % 512 % number_of_bytes_to_read;
                        memset(inbuf + number_of_bytes_to_read, 0, 512 - number_of_bytes_to_read); //fill zero to the error sector;
                        record((job_state::type)(mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL, record_format(rf));
                    }
                }      
                result = true;
            }
            else
            {
                record_format rf("read error, block->start = %1% , block->length = %2%, number_of_bytes_to_read = %3%.");
                rf % block->start % block->length % number_of_bytes_to_read;
                record((job_state::type)(mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL,
                    record_format(rf));
            }
            /*here to check the snapshot's status*/
            /*1. emulate the snapshot instance by the path name*/
            snapshot_instance::ptr si =_sh->get_snapshot_by_datto_device_name(block->in->path());
            if (si)
            {
                /*2. get the info of the snapshots error*/
                si->get_info();
                /*3. get the error status */
                if (si->get_dattobd_info().error)
                {
                    record_format rf("datto error, error_device = %1%, error_status = %2%.");
                    rf % block->in->path() % si->get_dattobd_info().error;
                    record((job_state::type)(mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL,
                        record_format(rf));
                    {
                        boost::unique_lock<boost::recursive_mutex> lock(repository.progress->lock);
                        repository.progress->cow_error = true;
                    }

                }

            }
        }
#ifdef CHECKTIME
        clock_gettime(CLOCK_REALTIME, &spec);
        end_s = spec.tv_sec;
        end_l = spec.tv_nsec;
        read_s += (end_s - start_s);
        if (end_l < start_l)
        {
            read_s--;
            end_l += 1.0e9;
        }
        read_l += (end_l - start_l);
        read_s += read_l / 1.0e9;
        read_l %= (int64_t)1.0e9;
#endif
        if (result) {
#ifdef CHECKTIME
            clock_gettime(CLOCK_REALTIME, &spec);
            start_s = spec.tv_sec;
            start_l = spec.tv_nsec;
#endif
            while (pendding /*&& !repository.terminated*/ && repository.liveThreadNumber > 1)
            {
                LOG_TRACE_REP("index = %d ,pendding = %d, repository.terminated = %d\r\n",index, pendding, repository.terminated);
                {
                    boost::unique_lock<boost::recursive_mutex> lock(repository.cs);
                    if (block->start - repository.processing[0]->start < repository.queue_size)
                    {
                        LOG_TRACE_REP("index = %d , 1", index);
                        break;
                    }
                }
                /*if (!repository.terminated)*/ {
                    //LOG_TRACE("index = %d , 2", index);
                    boost::unique_lock<boost::mutex> lock(repository.pending_lock);
                    repository.pending_cond.timed_wait(lock, boost::posix_time::milliseconds(30000));
                    //LOG_TRACE("index = %d , 3", index);
                }
            }
#ifdef CHECKTIME
            clock_gettime(CLOCK_REALTIME, &spec);
            end_s = spec.tv_sec;
            end_l = spec.tv_nsec;
            afterread_s += (end_s - start_s);
            if (end_l < start_l)
            {
                afterread_s--;
                end_l += 1.0e9;
                }
            afterread_l += (end_l - start_l);
            afterread_s += afterread_l / 1.0e9;
            afterread_l %= (int64_t)1.0e9;
#endif
            for (universal_disk_rw::ptr& o : outs) {
                target_start = repository.target + block->start;
                need_wait_for_queue = true;
#ifdef WRITE_FAIL_TEST
                int rand_value = 100;
                if (test_fault_ratio != 0)
                {
                    rand_value = rand() % 100;
                    LOG_TRACE("enter test_fault_ratio = %d index = %d , rand_value = %d", test_fault_ratio, index, rand_value);
                }
                if (rand_value < test_fault_ratio)
                {
                    result = false;
                }
                else
                {
#endif

                while (need_wait_for_queue) {
#ifdef CHECKTIME
                    clock_gettime(CLOCK_REALTIME, &spec);
                    start_s = spec.tv_sec;
                    start_l = spec.tv_nsec;
#endif
                        try {
                                need_wait_for_queue = false;
                                result = false;
                            if (repository.compressed)
                            {
#ifdef STRING_BUF
                                buf_dest.resize(number_of_bytes_to_read);
                                outbuf = reinterpret_cast<char*>(&buf_dest[0]);
#else
                                outbuf = buf_dest_old.get();
#endif
                                LOG_TRACE_REP("number_of_bytes_to_read = %llu", number_of_bytes_to_read);
                                compressed_length = LZ4_compress_fast(inbuf, outbuf, number_of_bytes_to_read, number_of_bytes_to_read, 1);
                                //LOG_TRACE("write start");
                                if (compressed_length > 0 && compressed_length < number_of_bytes_to_read)
                                {
#ifdef STRING_BUF
                                    buf_dest.resize(compressed_length);
                                    LOG_TRACE_REP("buf_dest.size() = %llu, compressed_length = %llu", buf_dest.size(), compressed_length);
                                    result = o->write_ex(target_start, buf_dest, number_of_bytes_to_read, true, number_of_bytes_to_written);
#else
                                    //LOG_TRACE("index = %d, before write_ex",index);
                                    result = o->write_ex(target_start, outbuf, compressed_length, number_of_bytes_to_read, true, number_of_bytes_to_written);
                                    //LOG_TRACE("index = %d, after write_ex",index);
#endif
                                }
                                else
                                {
#ifdef STRING_BUF
                                    LOG_TRACE_REP("buf.capacity() = %llu, number_of_bytes_to_read = %llu", buf.capacity(), number_of_bytes_to_read);
                                    result = o->write_ex(target_start, buf, number_of_bytes_to_read, false, number_of_bytes_to_written);
#else
                                    //LOG_TRACE("index = %d, before write_ex", index);
                                    result = o->write_ex(target_start, inbuf, number_of_bytes_to_read, number_of_bytes_to_read, false, number_of_bytes_to_written);
                                    //LOG_TRACE("index = %d, after write_ex", index);
#endif
                                }
                                //LOG_TRACE("write finish");
                            }
                            else
                            {
#ifdef STRING_BUF
                                //LOG_TRACE("buf.capacity() = %llu, number_of_bytes_to_read = %llu", buf.capacity(), number_of_bytes_to_read);
                                result = o->write(target_start, buf, number_of_bytes_to_written);
                                result = 1;
                                LOG_TRACE_REP("write finish");
#else
                                LOG_TRACE("target_start = %llu", target_start);
                                result = o->write(target_start, /*inbuf*/buf_old.get(), number_of_bytes_to_read, number_of_bytes_to_written);
                                LOG_TRACE("write finish");
#endif
                            }
                            if (!result) {
                                LOG_ERROR("Cannot write( %I64u, %d)", target_start, number_of_bytes_to_read);
                                code = saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE;
                                message = boost::str(boost::format("Cannot write( %1%, %2%)") % target_start % number_of_bytes_to_read);
                                break;
                            }
                        }
                        catch (const out_of_queue_exception &e) {
                            LOG_ERROR("out_of_queue_exception = %d , 4", index);
                            need_wait_for_queue = true;
                        }
                        catch (...)
                        {
                            LOG_ERROR("error, happen");
                        }
#ifdef CHECKTIME
                        clock_gettime(CLOCK_REALTIME, &spec);
                        end_s = spec.tv_sec;
                        end_l = spec.tv_nsec;
                        write_s += (end_s - start_s);
                        if (end_l < start_l)
                        {
                            write_s--;
                            end_l += 1.0e9;
                        }
                        write_l += (end_l - start_l);
                        write_s += write_l / 1.0e9;
                        write_l %= (int64_t)1.0e9;
#endif
#ifdef CHECKTIME
                        clock_gettime(CLOCK_REALTIME, &spec);
                        start_s = spec.tv_sec;
                        start_l = spec.tv_nsec;
#endif
                        if (need_wait_for_queue) {
                            LOG_ERROR("need_wait_for_queue");
                            carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                            if (rw) {
                                while (/*!repository.terminated &&*/ !rw->is_buffer_free()) {
                                    //LOG_TRACE("index = %d , 6", index);
                                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                                }
                            }
                        }
#ifdef CHECKTIME
                        clock_gettime(CLOCK_REALTIME, &spec);
                        end_s = spec.tv_sec;
                        end_l = spec.tv_nsec;
                        afterwrite_s += (end_s - start_s);
                        if (end_l < start_l)
                        {
                            afterwrite_s--;
                            end_l += 1.0e9;
                        }
                        afterwrite_l += (end_l - start_l);
                        afterwrite_s += afterwrite_l / 1.0e9;
                        afterwrite_l %= (int64_t)1.0e9;
#endif
                        /*if (repository.terminated) {
                            LOG_ERROR("index = %d , repository.terminated", index);
                            result = false;
                            break;
                        }*/
                    }
#ifdef WRITE_FAIL_TEST
                }
#endif
                if (/*repository.terminated ||*/ !result)
                {
                    LOG_ERROR("thread %d fail!",index);
                    break;
                }
                }
            }
        else
        {
            LOG_ERROR("read block error, block->src_start = %llu,  block->start = %llu ,block->length = %llu, block->in->path() = %s\r\n", block->src_start, block->start, block->length, block->in->path().c_str());
        }
    }
    /*if (!repository.terminated && is_canceled())
        repository.terminated = true;*/
    /*if (completed_queue_change)
        save_status();*/
    repository.completion_cond.notify_one();
    repository.pending_cond.notify_all();
    {
        boost::unique_lock<boost::recursive_mutex> lock(repository.cs);
        LOG_TRACE_REP("thread index = %d close", index);
        repository.liveThreadNumber--;
    }
#ifdef CHECKTIME
    LOG_TRACE("index = %d,first_s = %llu, first_l = %llu\r\n", index, first_s, first_l);
    LOG_TRACE("index = %d,read_s = %llu, read_l = %llu\r\n", index, read_s, read_l);
    LOG_TRACE("index = %d,write_s = %llu, write_l = %llu\r\n", index, write_s, write_l);
    LOG_TRACE("index = %d,afterread_s = %llu, afterread_l = %llu\r\n", index, afterread_s, afterread_l);
    LOG_TRACE("index = %d,afterwrite_s = %llu, afterwrite_l = %llu\r\n", index, afterwrite_s, afterwrite_l);
#endif
}


linux_packer_job::linux_packer_job(std::string id,const saasame::transport::create_packer_job_detail &detail, snapshot_manager::ptr sh)
    : _state(job_state_none)
    , _create_job_detail(detail)
    , _is_interrupted(false)
    , _is_canceled(false)
    , _is_force_full(false)
    , _is_skip_read_error(false)
    , removeable_job(id, "00000000-0000-0000-0000-000000000000")
    , _block_size(1024 * 1024 * 8)
    , _mini_block_size(1024 * 1024)
    , _queue_size(CARRIER_RW_IMAGE_BLOCK_SIZE * 4)
    , _guest_os_type(saasame::transport::hv_guest_os_type::type::HV_OS_LINUX)
    , _sh(sh)
    {
    FUN_TRACE;
    boost::filesystem::path work_dir = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    boost::filesystem::create_directories(work_dir / "jobs");
    _config_file = work_dir / "jobs" / boost::str(boost::format("%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / "jobs" / boost::str(boost::format("%s%s.status") % _id % JOB_EXTENSION);
    for (auto & cb : detail.detail.p.completed_blocks)
    {
        for (auto & b : cb.second)
        {
            if (_progress.count(cb.first))
            {
                boost::unique_lock<boost::recursive_mutex> lock(_progress[cb.first]->lock);
                _progress[cb.first]->completed.push_back(replication_block::ptr(new replication_block(0, b.start, b.length, b.start - b.offset, NULL)));
            }
        }
    }
    for (auto & pc: _create_job_detail.priority_carrier)
    {
        LOG_TRACE("priority_carrier = %s", pc.first.c_str());
        LOG_TRACE("priority_carrier.s = %s", pc.second.c_str());
    }
}

linux_packer_job::ptr linux_packer_job::create(std::string id, saasame::transport::create_packer_job_detail detail, snapshot_manager::ptr sh, physical_packer_service_handler* packer, bool b_force_full, bool is_skip_read_error){
    FUN_TRACE;
    linux_packer_job::ptr job;
    if (detail.type == saasame::transport::job_type::physical_packer_job_type){
        // Physical packer type
        job = linux_packer_job::ptr(new linux_packer_job(/*macho::guid_(id)*/id, detail,sh)); //guid maybe need handle
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
        job->set_packer(packer);
        job->set_modify_disks(packer->modify_disks);
        if (b_force_full)
            job->setting_force_full();
        if (is_skip_read_error)
            job->setting_skip_read_error();
    }
    return job;
}

linux_packer_job::ptr linux_packer_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file, snapshot_manager::ptr sh, bool b_force_full){
    FUN_TRACE;
    linux_packer_job::ptr job = NULL;
    std::string id;
    saasame::transport::create_packer_job_detail _create_job_detail = load_config(config_file.string(), id);
    if (_create_job_detail.type == saasame::transport::job_type::physical_packer_job_type){
        // Physical packer type
        job = linux_packer_job::ptr(new linux_packer_job(id, _create_job_detail, sh));
        if(job)
            job->load_status(status_file.string());
    }
    if (b_force_full)
    {
        job->setting_force_full();
        b_force_full = false;
    }
    return job;
}

void linux_packer_job:: save_status(){
    //FUN_TRACE;
    if (_cs.try_lock()) // if it's lock, not write
    {
        _cs.unlock();
    }
    else
    {
        return;
    }
    try{
        boost::unique_lock<boost::recursive_mutex> lck(_cs);
        mObject job_status;
        job_status["id"] = _id;
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);       
        mArray progress;
        for(physical_packer_job_progress::map::value_type &p : _progress){
            boost::unique_lock<boost::recursive_mutex> lock(p.second->lock);
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
            o["cow_error"] = p.second->cow_error;
            /*completed info*/
            mArray o_complete;
            for (auto & a : p.second->completed)
            {
                mObject o_io;
                o_io["start"] = a->start;
                o_io["length"] = a->length;
                o_io["offset"] = a->offset;
                o_complete.push_back(o_io);
            }
            o["completed"] = o_complete;
            /*completed info*/
            progress.push_back(o);
        }
        job_status["progress"] = progress;
        mArray histories;
        for(history_record::ptr &h : _histories){
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
        mArray journals;
        typedef std::map<int64_t, physical_vcbt_journal>  i64_physical_vcbt_journal_map;
        for(i64_physical_vcbt_journal_map::value_type &i : _journals){
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
            /*std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);*/  /*that's cause compilier error*/
            write(job_status, output, json_spirit::pretty_print | json_spirit::raw_utf8);
            output.close();
        }
        int ret = rename(temp.string().c_str(), _status_file.string().c_str());
        if (ret) {
            LOG_ERROR("rename('%s', '%s') with error(%d)", temp.string().c_str(), _status_file.string().c_str(), ret);
        }/*change to the linux version*/
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR,boost::exception_detail::get_diagnostic_information(ex, "Cannot output status info."));
    }
    catch (...){
    }
}

bool linux_packer_job::load_status(std::string status_file){
    try
    {
        FUN_TRACE;
        if (boost::filesystem::exists(status_file)){
            boost::unique_lock<boost::recursive_mutex> lck(_cs);
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            _state = find_value(job_status.get_obj(), "state").get_int();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _boot_disk = find_value_string(job_status.get_obj(), "boot_disk");
            mArray progress = find_value(job_status.get_obj(), "progress").get_array();
            for(mValue &p : progress){
                physical_packer_job_progress::ptr ptr = physical_packer_job_progress::ptr(new physical_packer_job_progress());
                boost::unique_lock<boost::recursive_mutex> lock(ptr->lock);
                ptr->uri = find_value_string(p.get_obj(), "uri");
                ptr->output = find_value_string(p.get_obj(), "output");
                ptr->base = find_value_string(p.get_obj(), "base");
                ptr->parent = find_value_string(p.get_obj(), "parent");
                ptr->total_size = find_value(p.get_obj(), "total_size").get_int64();
                ptr->backup_image_offset = find_value(p.get_obj(), "backup_image_offset").get_int64();
                ptr->backup_progress = find_value(p.get_obj(), "backup_progress").get_int64();
                ptr->backup_size = find_value(p.get_obj(), "backup_size").get_int64();
                mArray completed = find_value(p.get_obj(), "completed").get_array();
                ptr->cow_error = find_value_bool(p.get_obj(), "cow_error", false);
                if (!ptr->cow_error)/*if there are no error, read the completed block list*/
                {
                    for (mValue &c : completed) {
                        uint64_t start = find_value(c.get_obj(), "start").get_int64();
                        uint64_t length = find_value(c.get_obj(), "length").get_int64();
                        uint64_t offset = find_value(c.get_obj(), "offset").get_int64();
                        ptr->completed.push_back(replication_block::ptr(new replication_block(0, start, length, start - offset, NULL)));
                    }
                }
                else
                {
                    setting_force_full();
                    break;
                }
                _progress[find_value_string(p.get_obj(), "key")] = ptr;
            }

            mArray histories = find_value(job_status.get_obj(), "histories").get_array();
            for (mValue &h : histories){
                std::vector<std::string> arguments;
                mArray args = find_value(h.get_obj(), "arguments").get_array();
                for (mValue a: args){
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
            /*now have no journals*/
            mArray  journals = find_value(job_status.get_obj(), "journals").get_array();
            for (mValue i: journals){
                physical_vcbt_journal journal;
                journal.id = find_value(i.get_obj(), "journal_id").get_int64();
                journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
                journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
                journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
                _journals[journal.id] = journal;
            }

            mArray  system_disks = find_value(job_status.get_obj(), "system_disks").get_array();
            for (mValue i: system_disks){
                _system_disks.insert(i.get_str());
            }
            _guest_os_type = (saasame::transport::hv_guest_os_type::type)find_value_int32(job_status.get_obj(), "guest_os_type", 1);
            return true;
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, boost::exception_detail::get_diagnostic_information(ex, "Cannot read status info."));
    }
    catch (...)
    {
    }
    return false;
}

saasame::transport::create_packer_job_detail linux_packer_job::load_config(std::string config_file, std::string &job_id){
    FUN_TRACE;
    saasame::transport::create_packer_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mObject job_config_obj = job_config.get_obj();
        job_id = (std::string)find_value(job_config_obj, "id").get_str(); 
        detail.type = (saasame::transport::job_type::type)find_value(job_config_obj, "type").get_int();
        mArray  disks = find_value(job_config_obj, "disks").get_array();
        for (mValue d: disks){
            detail.detail.p.disks.insert(d.get_str());
        }

        mArray  connection_ids = find_value(job_config_obj, "connection_ids").get_array();
        for (mValue i: connection_ids){
            detail.connection_ids.insert(i.get_str());
        }

        mArray  carriers = find_value(job_config_obj, "carriers").get_array();
        for (mValue c: carriers){
            std::string connection_id = find_value_string(c.get_obj(), "connection_id");
            mArray  carrier_addr = find_value(c.get_obj(), "carrier_addr").get_array();
            std::set<std::string> _addr;
            for (mValue a: carrier_addr){
                _addr.insert(a.get_str());
            }
            detail.carriers[connection_id] = _addr;
        }

        mArray  snapshots = find_value(job_config_obj, "snapshots").get_array();
        for (mValue s: snapshots){
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
        for (mValue i: previous_journals){
            physical_vcbt_journal journal;
            journal.id = find_value(i.get_obj(), "journal_id").get_int64();
            journal.first_key = find_value(i.get_obj(), "first_key").get_int64();
            journal.latest_key = find_value(i.get_obj(), "latest_key").get_int64();
            journal.lowest_valid_key = find_value(i.get_obj(), "lowest_valid_key").get_int64();
            detail.detail.p.previous_journals[journal.id] = journal;
        }

        mArray  images = find_value(job_config_obj, "images").get_array();
        for (mValue i: images){
            packer_disk_image image;
            image.name = find_value_string(i.get_obj(), "name");
            image.parent = find_value_string(i.get_obj(), "parent");      
            image.base = find_value_string(i.get_obj(), "base");
            detail.detail.p.images[find_value_string(i.get_obj(), "key")] = image;
        }

        mArray  backup_size = find_value(job_config_obj, "backup_size").get_array();
        for (mValue i : backup_size){
            detail.detail.p.backup_size[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }
    
        mArray  backup_progress = find_value(job_config_obj, "backup_progress").get_array();
        for (mValue i : backup_progress){
            detail.detail.p.backup_progress[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
        }

        mArray  backup_image_offset = find_value(job_config_obj, "backup_image_offset").get_array();
        for (mValue i : backup_image_offset){
            detail.detail.p.backup_image_offset[find_value_string(i.get_obj(), "key")] = find_value(i.get_obj(), "value").get_int64();
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
            for (mValue c : priority_carrier){
                std::string connection_id = find_value_string(c.get_obj(), "connection_id");
                std::string addr = find_value_string(c.get_obj(), "addr");
                detail.priority_carrier[connection_id] = addr;
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info."));
    }
    catch (...){
    }
    return detail;
}

void linux_packer_job::save_config(){
    FUN_TRACE;
    try{
        using namespace json_spirit;
        mObject job_config;

        job_config["id"] = _id;
        job_config["type"] = _create_job_detail.type;
        mArray  disks(_create_job_detail.detail.p.disks.begin(), _create_job_detail.detail.p.disks.end());
        job_config["disks"] = disks;
        mArray  connection_ids(_create_job_detail.connection_ids.begin(), _create_job_detail.connection_ids.end());
        job_config["connection_ids"] = connection_ids;
        mArray carriers;
        for (string_set_map::value_type &t : _create_job_detail.carriers){
            mObject carrier;
            carrier["connection_id"] = t.first;
            mArray carrier_addr(t.second.begin(), t.second.end());
            carrier["carrier_addr"] = carrier_addr;
            carriers.push_back(carrier);
        }
        job_config["carriers"] = carriers;

        mArray snapshots;
        for (snapshot &s : _create_job_detail.detail.p.snapshots){
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
        for (i64_physical_vcbt_journal_map::value_type &i : _create_job_detail.detail.p.previous_journals){
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
        for (packer_disk_image_map::value_type &i : _create_job_detail.detail.p.images){
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
        for (string_int64_t_map::value_type &_size : _create_job_detail.detail.p.backup_size){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_size.push_back(o);
        }
        job_config["backup_size"] = backup_size;

        mArray backup_progress;
        for (string_int64_t_map::value_type &_size : _create_job_detail.detail.p.backup_progress){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_progress.push_back(o);
        }
        job_config["backup_progress"] = backup_progress;

        mArray backup_image_offset;
        for (string_int64_t_map::value_type &_size : _create_job_detail.detail.p.backup_image_offset){
            mObject o;
            o["key"] = _size.first;
            o["value"] = _size.second;
            backup_image_offset.push_back(o);
        }
        job_config["backup_image_offset"]       = backup_image_offset;
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
        for (string_map::value_type &i : _create_job_detail.priority_carrier){
            mObject target;
            target["connection_id"] = i.first;
            target["addr"] = i.second;
            priority_carrier.push_back(target);
        }
        job_config["priority_carrier"] = priority_carrier;
        job_config["is_only_single_system_disk"] = _create_job_detail.is_only_single_system_disk;
        job_config["is_compressed_by_packer"] = _create_job_detail.is_compressed_by_packer;
        boost::filesystem::path temp = _config_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            /*std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);*//*!!*/
            write(job_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        int ret = rename(temp.string().c_str(), _config_file.string().c_str());
        if (ret) {
            LOG_ERROR("rename('%s', '%s') with error(%d)", temp.string().c_str(), _config_file.string().c_str(), ret);
        }/*change to the linux version*/
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info."));
    }
    catch (...){
    }
}

void linux_packer_job::remove(){
    FUN_TRACE;
    LOG_RECORD("Job remove event captured.");
    _is_removing = true;
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        _running.unlock();
    }
}

void linux_packer_job::record(saasame::transport::job_state::type state, int error, record_format format){
    FUN_TRACE;
    boost::unique_lock<boost::recursive_mutex> lck(_cs);
    _histories.push_back(history_record::ptr(new history_record(state, error, format)));
    LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), "(%s)%s", _id.c_str(), format.str().c_str());
}


saasame::transport::packer_job_detail linux_packer_job::get_job_detail(boost::posix_time::ptime previous_updated_time) {
    //FUN_TRACE;
    boost::unique_lock<boost::recursive_mutex> lck(_cs);
    saasame::transport::packer_job_detail job;
    job.id = _id;
    job.__set_type(_create_job_detail.type);
    job.created_time = boost::posix_time::to_simple_string(_created_time);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    job.updated_time = boost::posix_time::to_simple_string(update_time);
    job.state = (saasame::transport::job_state::type)((_state) & ~mgmt_job_state::job_state_error);
    //LOG_TRACE("previous_updated_time = %s", boost::posix_time::to_simple_string(previous_updated_time).c_str());
    for (history_record::ptr &h : _histories){
        if (h->time > previous_updated_time){
            saasame::transport::job_history _h;
            _h.state = h->state;
            _h.error = h->error;
            _h.description = h->description;
            LOG_TRACE("h.des = %s", _h.description.c_str());
            _h.time = boost::posix_time::to_simple_string(h->time);
            LOG_TRACE("_h.time = %s", _h.time.c_str());
            _h.format = h->format;
            _h.arguments = h->args;
            _h.__set_arguments(_h.arguments);
            job.histories.push_back(_h);
        }
    }
    job.__set_histories(job.histories);
    for (physical_packer_job_progress::map::value_type& v : _progress){
        boost::unique_lock<boost::recursive_mutex> lock(v.second->lock);
        job.detail.p.original_size[v.first] = v.second->total_size;
        job.detail.p.backup_progress[v.first] = v.second->backup_progress;
        job.detail.p.backup_image_offset[v.first] = v.second->backup_image_offset;
        job.detail.p.backup_size[v.first] = v.second->backup_size;
        std::vector<io_changed_range> completeds;
        for(auto &c : v.second->completed) {
            io_changed_range _c(*c);
            completeds.push_back(_c);
        }
        if (v.second->cow_error)
            if(_packer->cg_retry_count<=0) // if there are no times to retry, discard
                job.state |= saasame::transport::job_state::job_state_discard;
        job.detail.p.completed_blocks[v.first] = completeds;
    }
    job.__set_detail(job.detail);
    job.detail.__set_p(job.detail.p);
    job.detail.p.__set_vcbt_journals(_journals);
    job.detail.p.__set_original_size(job.detail.p.original_size);
    job.detail.p.__set_backup_progress(job.detail.p.backup_progress);
    job.detail.p.__set_backup_size(job.detail.p.backup_size);
    job.detail.p.__set_backup_image_offset(job.detail.p.backup_image_offset);
    job.detail.p.__set_completed_blocks(job.detail.p.completed_blocks);
    job.detail.p.__set_guest_os_type(_guest_os_type);

    job.__set_boot_disk(_boot_disk);
    for (std::string d : _system_disks){
        job.system_disks.push_back(d);
    }
    job.__set_system_disks(job.system_disks);
    if (_running.try_lock()){
        if ((_state & mgmt_job_state::job_state_error) == mgmt_job_state::job_state_error)
            job.is_error = true;
        _running.unlock();
    }
    LOG_TRACE("job.state = %x\r\n", job.state);
    return job;
}

void linux_packer_job::interrupt(){
    FUN_TRACE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled){   
    }
    else{
        _is_interrupted = true;
    }
    save_status();
}

bool linux_packer_job::get_output_image_info(const std::string disk, physical_packer_job_progress::ptr &p){
    FUN_TRACE;
    if (_create_job_detail.detail.p.images.count(disk)){
        boost::unique_lock<boost::recursive_mutex> lock(p->lock);
        p->output = _create_job_detail.detail.p.images[disk].name;
        p->parent = _create_job_detail.detail.p.images[disk].parent;
        p->base = _create_job_detail.detail.p.images[disk].base;
        return true;
    }
    return false;
}

void linux_packer_job::execute(){
    FUN_TRACE;
    //_state |= mgmt_job_state::job_state_error; //force error
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || (_state & mgmt_job_state::job_state_error) || _is_removing){
        return;
    }
    universal_disk_rw::vtr rws;
    changed_area::map cam;
    _state &= ~mgmt_job_state::job_state_error;
    snapshot_instance::vtr sis;
    storage::ptr stg = storage::get_storage();

    /**/
    /*struct addrinfo hints, *res, *res0;
    char port[sizeof("65536")];
    sprintf(port, "%d", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
    LOG_TRACE("3.1");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    LOG_TRACE("carrier.c_str() = %s\r\n", "tpe2.saasame.com");
    LOG_TRACE("port.c_str() = %s\r\n", port);
    int error = getaddrinfo("tpe2.saasame.com", port, &hints, &res0);
    LOG_TRACE("error = %d", error); */

    /**/


    /*disable SIGPIPE*/
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    //try {
        if (!_is_interrupted) {
            _state = mgmt_job_state::job_state_replicating;
            save_status();
            bool result = false;
            disk::vtr _disks = stg->get_all_disk();
            bool b_full;
            if (_create_job_detail.detail.p.previous_journals.empty())
            {
                LOG_TRACE("p.previous_journals.empty(), do full\n");
                physical_vcbt_journal zero_journal;
                zero_journal.__set_id(0);
                zero_journal.__set_first_key(0);
                zero_journal.__set_latest_key(0);
                zero_journal.__set_lowest_valid_key(0);
                _journals[0] = zero_journal;
                b_full = true | _is_force_full;
            }
            else
            {
                LOG_TRACE("p.previous_journals.empty(), do delta\n");
                _journals = _create_job_detail.detail.p.previous_journals;
                b_full = false | _is_force_full;
            }
            string boot_disk = system_tools::execute_command("df -P /boot | tail -1 | cut -d' ' -f 1");
            for (std::string disk : _create_job_detail.detail.p.disks) {
                disk_universal_identify uri(disk);
                for (auto & d : _disks) {
                    if (disk_universal_identify(*d) == uri) {
                        physical_packer_job_progress::ptr p;
                        uint64_t offset = 0;
                        if(d->is_include_string(boot_disk))
                            _boot_disk = disk;
                        if (d->is_boot() || d->is_root())                        
                            _system_disks.insert(disk);
                        if (_progress.count(disk))
                            p = _progress[disk];
                        else {
                            p = physical_packer_job_progress::ptr(new physical_packer_job_progress());
                            boost::unique_lock<boost::recursive_mutex> lock(p->lock);
                            p->uri = disk_universal_identify(*d);
                            p->total_size = d->blocks;
                            if (_create_job_detail.detail.p.backup_size.count(disk))
                                p->backup_size = _create_job_detail.detail.p.backup_size[disk];
                            if (_create_job_detail.detail.p.backup_progress.count(disk))
                                p->backup_progress = _create_job_detail.detail.p.backup_progress[disk];
                            if (_create_job_detail.detail.p.backup_image_offset.count(disk))
                                p->backup_image_offset = _create_job_detail.detail.p.backup_image_offset[disk];
                            _progress[disk] = p;
                        }
                        if (!(result = get_output_image_info(disk, p))) {
                            _state |= mgmt_job_state::job_state_error;
                            break;
                        }
                        /*if the jounaery is null , do full*/
                        /*get change area*/
                        universal_disk_rw::ptr rw = general_io_rw::open_rw(d->ab_path);
                        if (!rw) {
                            LOG_ERROR("error opening clone_disk rws, rw = %s\n", d->ab_path.c_str());
                        }
                        else
                        {
                            LOG_TRACE("rw->path =  %s", rw->path().c_str());
                            rws.push_back(rw);                            
                            result = true;
                        }
                    }
                }
                if (!result)
                    break;
            }
            if (result)
            {
                result = false;
                _changed_areas = clone_disk::get_clone_data_range(rws, stg,_sh, _modify_disks, _create_job_detail.detail.p.excluded_paths, _create_job_detail.detail.p.resync_paths,_create_job_detail.file_system_filter_enable,b_full);
                for (std::string disk : _create_job_detail.detail.p.disks) {
                    LOG_TRACE("disk = %s\r\n", disk.c_str());
                    disk_universal_identify uri(disk);
                    for (auto & d : _disks) {
                        LOG_TRACE("d->string_uri() = %s\r\n", d->string_uri.c_str());
                        if (disk_universal_identify(*d) == uri) {
                            LOG_TRACE("disk_universal_identify(*d) == uri\r\n");
                            physical_packer_job_progress::ptr p;
                            if (_progress.count(disk)) {
                                LOG_TRACE("_progress.count(disk)\r\n");
                                p = _progress[disk];
                                boost::unique_lock<boost::recursive_mutex> lock(p->lock);
                                /*need to clear the backup_size*/
                                p->backup_size = 0;
                                for (auto & c : _changed_areas[d->ab_path])
                                    p->backup_size += c.length;
                                if (p->backup_size == 0 && _changed_areas[d->ab_path].size() == 0)
                                {
                                    LOG_TRACE("p->backup_size == 0 && _changed_areas[d->ab_path].size() == 0\r\n");
                                }//add else and break
                                else
                                    result = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (result) {
                /*for (auto & ca : _changed_areas)
                {
                    printf("execute--ca.first = %s\r\n", ca.first.c_str());
                    for (auto & c : ca.second)
                    {
                        printf("execute--c.start = %llu , c.src_start = %llu, c.length = %llu\r\n, c._rw->path().c_str() = %s\r\n", c.start, c.src_start, c.length, c._rw->path().c_str());
                    }
                }*/


                for (std::string disk : _create_job_detail.detail.p.disks) {
                    LOG_TRACE("disk = %s\r\n", disk.c_str());
                    disk_universal_identify uri(disk); //this is weried, why should use the disk name to build the uri and 
                    for (auto & d : _disks) {
                        LOG_TRACE("d->string_uri() = %s\r\n", d->string_uri.c_str());
                        if (disk_universal_identify(*d) == uri) {
                            physical_packer_job_progress::ptr p = _progress[disk];
                            LOG_TRACE("p->backup_size == %llu\r\n", p->backup_size);
                            LOG_TRACE("_changed_areas[%s].size() = %lu", d->ab_path.c_str(), _changed_areas[d->ab_path].size());
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Backup size estimate of disk %1% : %2%") % d->ab_path % p->backup_size));
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Backup package count estimate of disk %1% : %2%, merge_size = %3% bytes") % d->ab_path % _changed_areas[d->ab_path].size() % _sh->get_merge_size()));
                            if (p->backup_progress >= p->backup_size)
                                continue;
                            if (!(result = replicate_disk(d, p->output, p->base, p->backup_image_offset, p->backup_progress, p->parent)))
                                break;
                        }
                    }
                    if (!result)
                        break;
                }
            }
            _changed_areas.clear();
            if (result && _create_job_detail.detail.p.disks.size() == _progress.size())
                _state = mgmt_job_state::job_state_finished;
            else
                _state = mgmt_job_state::job_state_error;
        }
    /*}
    catch (macho::exception_base& ex)
    {
        LOG(LOG_LEVEL_ERROR, "%s", macho::get_diagnostic_information(ex).c_str());
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(macho::stringutils::convert_unicode_to_utf8(macho::get_diagnostic_information(ex))));
        result = false;
        _state |= mgmt_job_state::job_state_error;
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        LOG(LOG_LEVEL_ERROR, "%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
        result = false;
        _state |= mgmt_job_state::job_state_error;
    }
    catch (const boost::exception &ex)
    {
        LOG(LOG_LEVEL_ERROR, "%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(boost::exception_detail::get_diagnostic_information(ex, "error:")));
        result = false;
        _state |= mgmt_job_state::job_state_error;
    }
    catch (const std::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, "%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format(ex.what()));
        result = false;
        _state |= mgmt_job_state::job_state_error;
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, "Unknown exception");
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL, record_format("Unknown exception"));
        result = false;
        _state |= mgmt_job_state::job_state_error;
    }*/
    save_status();
}

bool linux_packer_job::calculate_disk_backup_size(disk::ptr d, uint64_t& start, uint64_t& backup_size){
    FUN_TRACE;
    bool result = false;
    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Calculating backup size of disk %1%.") % d->device_filename));
    try{

        save_status();
    }
    catch (invalid_operation& error){
        _state |= mgmt_job_state::job_state_error;
        result = false;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_FAIL, (record_format(error.why)));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot estimate backup size of disk %1%.") % d->device_filename));
    }
    catch (...){
        _state |= mgmt_job_state::job_state_error;
        result = false;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot estimate backup size of disk %1%.") % d->device_filename));
    }
    save_status();
    return result;
}


bool linux_packer_job::replicate_disk(disk::ptr d, const std::string& image_name, const std::string& base_name, uint64_t& start, uint64_t& backup_size, std::string parent){
    FUN_TRACE;
    bool result = false;
    universal_disk_rw::vtr outputs;
    universal_disk_rw::vtr output2;
    int32_t retry_count = _create_job_detail.carriers.size();
    //_create_job_detail.is_compressed_by_packer = true; // this is for test
    for (auto & a : _create_job_detail.priority_carrier)
        a.second = system_tools::analysis_ip_address(a.second);
    for (auto & a : _create_job_detail.carriers)
    {
        for (auto & s : a.second)
            s = system_tools::analysis_ip_address(s);
    }

    do{
        _state &= ~mgmt_job_state::job_state_error;

        try{
            if (start > 0){
                carrier_rw::open_image_parameter parameter;
                parameter.carriers = _create_job_detail.carriers;
                parameter.priority_carrier = _create_job_detail.priority_carrier;
                parameter.base_name = base_name;
                parameter.name = image_name;
                parameter.session = _create_job_detail.detail.p.snapshots[0].snapshot_set_id;
                parameter.timeout = _create_job_detail.timeout;
                parameter.encrypted = _create_job_detail.is_encrypted;
                parameter.polling = b_polling_mode;
                parameter.printf();
                outputs = carrier_rw::open(parameter);

            }
            else{
                for (auto & pc : _create_job_detail.priority_carrier)
                {
                    LOG_TRACE("priority_carrier = %s", pc.first.c_str());
                    LOG_TRACE("priority_carrier.s = %s", pc.second.c_str());
                }
                carrier_rw::create_image_parameter parameter;
                parameter.carriers = _create_job_detail.carriers;
                parameter.priority_carrier = _create_job_detail.priority_carrier;
                for (auto & pc : parameter.priority_carrier)
                {
                    LOG_TRACE("priority_carrier = %s", pc.first.c_str());
                    LOG_TRACE("priority_carrier.s = %s", pc.second.c_str());
                }
                parameter.base_name = base_name;
                parameter.name = image_name;
                parameter.size = d->size();
                parameter.parent = parent;
                parameter.checksum_verify = _create_job_detail.checksum_verify;
                parameter.session = _create_job_detail.detail.p.snapshots[0].snapshot_set_id;
                parameter.timeout = _create_job_detail.timeout;
                LOG_TRACE("parameter.timeout = ")
                parameter.encrypted = _create_job_detail.is_encrypted;
                parameter.comment = boost::str(boost::format("%1%") % d->ab_path);
                parameter.checksum = _create_job_detail.is_checksum;
                parameter.compressed = _create_job_detail.is_compressed;
                parameter.polling = b_polling_mode;
                parameter.mode = 2; //always 2
                parameter.printf();
                outputs = carrier_rw::create(parameter);
            }
        }
        catch (universal_disk_rw::exception &e){
            _state |= mgmt_job_state::job_state_error;
            outputs.clear();
        }
		
        if (0 == outputs.size()){ //output create fail
            if (start > 0)
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, (record_format("Cannot open disk image %1%.") % image_name));
            else
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, (record_format("Cannot create disk image %1%.") % image_name));
        }
        else if (verify_and_fix_snapshots()){
            //retry_count = 1;
            try{
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup started.") % d->get_ab_path()));
                universal_disk_rw::ptr rw = general_io_rw::open_rw(d->get_ab_path());
                /*i think we should call replicated change area directedly here*/

                /*there are no static input rw, so remove the first parameter*/
                if (replicate_changed_areas(d, start, 0, backup_size, _changed_areas[d->get_ab_path()], outputs))
                {
                    for (universal_disk_rw::ptr o : outputs) {
                        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                        if (rw) {
                            result = rw->close();
                            if (!result) {
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("carrier close fail , ex %1%.") % rw->ex_string));
                                _state |= mgmt_job_state::job_state_error;
                                break;
                            }
                        }
                    }
                    if (result)
                    {
                        /*if it's OK*/
                        LOG_TRACE("Disk %s backup completed with size : %llu", d->ab_path.c_str(), backup_size);
                        _state &= ~mgmt_job_state::job_state_error;
                        LOG_RECORD("Disk (%s) backup completed.", d->ab_path.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup completed with size : %2%") % d->ab_path % backup_size));
                        save_status();
                        break;
                    }
                }
                else
                {
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_INVALID, (record_format("Disk %1% backup failed.") % d->ab_path));
                    _state |= mgmt_job_state::job_state_error;
                    if (_progress[d->string_uri]->cow_error)
                    {
                        retry_count = 0;
                        _packer->set_next_job_force_full();
                    }
                }
                /*no matter the result, output should closed*/

                /*if (!rw){
                    _state |= mgmt_job_state::job_state_error;
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                }
                else if (start == 0 && !(result = replicate_beginning_of_disk(rw, d, start, backup_size, outputs))){
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                }
                else if (!(result = replicate_partitions_of_disk(rw, d, start, backup_size, outputs))){
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                }
                else if (!(result = replicate_end_of_disk(rw, d, start, backup_size, outputs))){
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                }
                else{
                    for(universal_disk_rw::ptr o, outputs){
                        carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                        if (rw) {
                            if (!(result = rw->close())){
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Disk %1% backup failed.") % d.number()));
                                _state |= mgmt_job_state::job_state_error;
                                break;
                            }
                        }
                    }
                    if (result){
                        _state &= ~mgmt_job_state::job_state_error;
                        LOG(LOG_LEVEL_RECORD, "Disk (%d) backup completed.", d.number());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_S_OK, (record_format("Disk %1% backup completed with size : %2%") % d.number() % backup_size));
                        save_status();
                        break;
                    }
                }*/
            }
            catch (invalid_operation& error){
                result = false;
                LOG_RECORD("Cannot backup disk %s: invalid_operation(%s)", d->get_ab_path().c_str(), error.why.c_str());
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d->get_ab_path()));
            }
            catch (...){
                LOG_RECORD("Cannot backup disk %s: unknown error.", d->get_ab_path().c_str());
                result = false;
                _state |= mgmt_job_state::job_state_error;
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_IMAGE_WRITE, (record_format("Cannot backup disk %1%.") % d->get_ab_path()));
            }
        }
		
        save_status();
        LOG_RECORD("result : %s , is_canceled : %s , retry_count : %d", result ? "true" : "false", is_canceled() ? "true" : "false", retry_count);
        if ((!result) && (!is_canceled()) && ((retry_count - 1) > 0))
        {
            sleep(5);
        }
    } while ( (!result) && (!is_canceled()) && ((retry_count--) > 0));
    return result;
}

bool linux_packer_job::verify_and_fix_snapshots() {
    FUNC_TRACER;
    bool result = false;
    if (_create_job_detail.detail.p.snapshots.size()) {
        std::map<std::string, std::vector<snapshot> > snapshots;
        result = get_all_snapshots(snapshots);
        LOG_TRACE("result = %d\r\n", result);
        if (result) {
            for(saasame::transport::snapshot &s :_create_job_detail.detail.p.snapshots) {
                LOG_TRACE("s.snapshot_set_id = %s\r\n", s.snapshot_set_id.c_str());
                if (snapshots.count(s.snapshot_set_id)) {
                    bool found = false;
                    LOG_TRACE("s.snapshot_id = %s\r\n", s.snapshot_id.c_str());
                    LOG_TRACE("s.snapshot_set_id = %s\r\n", s.snapshot_set_id.c_str());
                    for (snapshot &_s : snapshots[s.snapshot_set_id]) {
                        LOG_TRACE("_s.snapshot_id = %s\r\n", _s.snapshot_id.c_str());
                        if (_s.snapshot_id == s.snapshot_id) {
                            s.snapshot_device_object = _s.snapshot_device_object;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        result = false;
                        _state |= mgmt_job_state::job_state_error;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot find snapshot %1%.") % s.snapshot_id));
                        break;
                    }
                }
                else
                {
                    result = false;
                    _state |= mgmt_job_state::job_state_error;
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot find snapshots by snapshot set %1%.") % s.snapshot_set_id));
                }
            }        
        }
        else {
            result = false;
            _state |= mgmt_job_state::job_state_error;
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_NOTFOUND, (record_format("Cannot get snapshots info.")));
        }
    }
    else {
        result = false;
        _state |= mgmt_job_state::job_state_error;
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), saasame::transport::error_codes::SAASAME_E_SNAPSHOT_INVALID, (record_format("No snapshot found.")));
    }
    return result;
}

bool linux_packer_job::get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots) {
    FUNC_TRACER;
    bool result = false;
    /*try {
        boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
        std::shared_ptr<TTransport> _transport;
        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                std::shared_ptr<TSSLSocketFactory> factory;
                factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                factory->authenticate(false);
                factory->loadCertificate((p / "server.crt").string().c_str());
                factory->loadPrivateKey((p / "server.key").string().c_str());
                factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
                factory->access(accessManager);
                std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket("127.0.0.1", saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
                ssl_socket->setConnTimeout(1000);
                _transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
            }
            catch (TException& ex) {
            }
        }
        else {
            std::shared_ptr<TSocket> socket(new TSocket("127.0.0.1", saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
            socket->setConnTimeout(1000);
            _transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
        }
        if (_transport) {
            std::shared_ptr<TProtocol> _protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));
            std::shared_ptr<physical_packer_serviceClient> _client = std::shared_ptr<physical_packer_serviceClient>(new physical_packer_serviceClient(_protocol));
            _transport->open();
            _client->get_all_snapshots(snapshots,"");//i think use the null session id is OK
            result = true;
            _transport->close();
        }
    }
    catch (saasame::transport::invalid_operation& ex) {
        LOG(LOG_LEVEL_ERROR, "%s", ex.why.c_str());
    }
    catch (TException &ex) {
        LOG(LOG_LEVEL_ERROR, "%s", ex.what());
    }
    catch (...) {
    }*/
    _packer->get_all_snapshots(snapshots, "");
    result = true;
    return result;
}

bool linux_packer_job::replicate(universal_disk_rw::ptr &rw, uint64_t& start, const uint32_t length, universal_disk_rw::vtr& outputs, const uint64_t target_offset) {
    FUN_TRACE;
    if (0 == outputs.size()) {
        start += length;
        return true;
    }
    bool result = false;
    std::unique_ptr<BYTE> buf = std::unique_ptr<BYTE>(new BYTE[length]);

    uint32_t nRead = 0;
    if (!(result = rw->read(start, length, buf.get(), nRead))) {
        LOG_ERROR("Cannot Read( %I64u, %d). Error: 0x%08X", start, length, errno);
        _state |= mgmt_job_state::job_state_error;
        invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_IMAGE_READ;
        error.why = "Cannot read disk data.";
        throw error;
    }
    else {
        uint32_t number_of_bytes_written = 0;
        for(universal_disk_rw::ptr o : outputs) {
            bool need_wait_for_queue = true;
            while (need_wait_for_queue) {
                try {
                    need_wait_for_queue = false;
                    result = false;
                    if (!(result = o->write(target_offset + start, buf.get(), nRead, number_of_bytes_written))) {
                        _state |= mgmt_job_state::job_state_error;
                        LOG_ERROR("Cannot write( %I64u, %d)", target_offset + start, nRead);
                        break;
                    }
                }
                catch (const out_of_queue_exception &e) {
                    need_wait_for_queue = true;
                }

                if (need_wait_for_queue) {
                    carrier_rw* rw = dynamic_cast<carrier_rw*>(o.get());
                    if (rw) {
                        while (!_is_interrupted && !rw->is_buffer_free()) {
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                        }
                    }
                }

                if (_is_interrupted) {
                    result = false;
                    break;
                }
            }
            if (_is_interrupted || !result)
                break;
        }
        if (result) {
            start += number_of_bytes_written;
        }
    }
    return result;
}

bool linux_packer_job::replicate_changed_areas(/*universal_disk_rw::ptr &rw,*/ disk::ptr d, uint64_t& start_offset, uint64_t offset, uint64_t& backup_size, changed_area::vtr &changeds, universal_disk_rw::vtr& outputs) {
    FUN_TRACE;
    bool is_replicated = false;
    uint64_t real_start_offset = (_is_force_full) ? 0: start_offset;
    if (changeds.size()) {
        boost::thread_group thread_pool;
        LOG_TRACE("d->string_uri = %s\r\n", d->string_uri.c_str());
        LOG_TRACE("_progress.count(d->string_uri) = %d", _progress.count(d->string_uri));
        physical_packer_job_progress::ptr p = NULL;
        for (auto & pe : _progress)
        {
            if (disk_universal_identify(pe.first) == disk_universal_identify(d->string_uri))
                p = pe.second;
        }
        if (!p)
        {
            LOG_ERROR("can't find fit disk");
            return false;
        }
        replication_repository repository(real_start_offset, p, offset); //maybe some error check;
        int worker_thread_number = _create_job_detail.worker_thread_number ? _create_job_detail.worker_thread_number : 4;
        repository.block_size = _block_size;
        repository.queue_size = _queue_size;
        repository.compressed = _create_job_detail.is_compressed_by_packer;
        uint64_t index = 0;
        replication_task::vtr  tasks;
        uint64_t size = 0;
        //printf("replicate_changed_areas:d->path = %s\r\n", d->ab_path.c_str());
        LOG_TRACE("outputs.size() = %d\r\n", outputs.size());
        /*fill waiting queue*/
        for(changed_area &c : changeds) {
            //printf("replicate_changed_areas:--c.start = %llu , c.src_start = %llu, c.length = %llu, c._rw->path().c_str() = %s\r\n", c.start, c.src_start, c.length, c._rw->path().c_str());
            //printf("ca.second.start = %llu\r\n, ca.second.lenght = %llu\r\n", c.start, c.length);
            uint64_t start = c.start;
            if (_is_interrupted)
                break;
            else if (outputs.size() > 0) {
                if ((offset + start) >= real_start_offset)
                {
                    bool b_push = true;
                    {
                        boost::unique_lock<boost::recursive_mutex> lock(repository.progress->lock);
                        for (replication_block::queue::iterator cp = repository.progress->completed.begin(); cp != repository.progress->completed.end(); ++cp)
                        {
                            if ((*cp)->start == start && (*cp)->length == c.length)
                            {
                                b_push = false;
                                repository.progress->completed.erase(cp);
                                break;
                            }
                        }
                    }
                    if (b_push)
                    {
                        LOG_RECORD("repository.waitting.push_back offset = %llu, real_start_offset = %llu ,(start: %llu, length: %llu, src_start: %llu, rw : %s)", offset, real_start_offset, start, c.length, c.src_start, c._rw->path().c_str());
                        repository.waitting.push_back(replication_block::ptr(new replication_block(index++, start, c.length, c.src_start, c._rw)));
                        size += c.length;
                    }
                }
                else {
                    LOG_RECORD("Skipped the write block. offset = %llu, real_start_offset = %llu ,(start: %llu, length: %llu, c.src_start: %llu, rw : %s)", offset, real_start_offset,start, c.length, c.src_start, c._rw->path().c_str());
                }
            }
            else {
                is_replicated = true;
                real_start_offset = offset + start;
                backup_size += c.length;
            }
        }
        LOG_TRACE("size = %llu\r\n", size);

        if (outputs.size() > 0)
        {
            for (;worker_thread_number > 0; --worker_thread_number)
            {
                tasks.clear();
                if (repository.waitting.size())
                {
                    if (worker_thread_number > repository.waitting.size())
                        worker_thread_number = repository.waitting.size();

                    for (int i = 0; i < worker_thread_number; i++)
                    {
                        universal_disk_rw::ptr in;
                        universal_disk_rw::vtr outs;
                        if (i == 0)
                        {
                            outs = outputs;
                        }
                        else
                        {
                            for (auto & o : outputs)
                            {
                                outs.push_back(o->clone());
                            }
                        }
                        if (outs.size())
                        {
                            replication_task::ptr task(new replication_task(/*in,*/ outs, repository,_sh));
                            task->index = i;
                            task->b_force_zero_broken_sector = _is_skip_read_error;
                            task->register_job_is_canceled_function(boost::bind(&linux_packer_job::is_canceled, this));
                            task->register_job_save_callback_function(boost::bind(&linux_packer_job::save_status, this));
                            task->register_job_record_callback_function(boost::bind(&linux_packer_job::record, this, _1, _2, _3));
                            tasks.push_back(task);
                        }
                    }

                    LOG_RECORD("Number of running threads is %d. Waitting list size (%d)", tasks.size(), repository.waitting.size());
                    //global_verbose = 0;
                    signal(SIGPIPE, SIG_IGN);
                    for (int i = 0; i < tasks.size(); i++)
                    {
                        LOG_TRACE("thread %d creating.", i);
#ifdef WRITE_FAIL_TEST
                        if (i > 0)
                        {
                            tasks[i]->test_fault_ratio = 10;
                        }
#endif
                        LOG_FLUSH_BUFFERED;
                        thread_pool.create_thread(boost::bind(&replication_task::replicate, &(*tasks[i])));
                        LOG_TRACE("thread %d created.", i);
                    }

                    boost::unique_lock<boost::mutex> _lock(repository.completion_lock);
                    while (true)
                    {
                        repository.completion_cond.timed_wait(_lock, boost::posix_time::milliseconds(60000));
                        {
                            boost::unique_lock<boost::recursive_mutex> lock(repository.cs);
                            if (repository.error.size() != 0)
                            {
                                _state |= mgmt_job_state::job_state_error;
                                break;
                            }
                            else if ((repository.processing.size() == 0 && repository.waitting.size() == 0) || is_canceled())
                            {
                                repository.terminated = true;
                                break;
                            }
                        }
                    }
                    thread_pool.join_all();
                    LOG_FLUSH_IMMEDIATELY;
                    //global_verbose = 10;
                }
                /*modify the thread number by the status*/
                is_replicated = (repository.error.size() == 0 && repository.processing.size() == 0 && repository.waitting.size() == 0);
                if (is_replicated || repository.progress->cow_error)/*stop if the replicated is finished or cow file error,*/
                {
                    progress(d, mgmt_job_state::job_state_replicating, outputs[0]->path(), real_start_offset, 0, d->size());
                    break;
                }
            }
        }
    }
    else {
        is_replicated = true;
    }
    return is_replicated;
}

bool linux_packer_job::modify_change_area_with_datto_snapshot(changed_area::map & cam, snapshot_manager::ptr sh)
{
    std::map<std::string, snapshot_instance::vtr> d_s_map;
    snapshot_manager sm;
    linux_storage::storage::ptr str = linux_storage::storage::get_storage();
    //linux_storage::storage::ptr str = linux_storage::storage::ptr(new linux_storage::storage());
    //str->scan_disk();
    changed_area::map cam_sn;
    snapshot_instance::vtr sns;
    for (auto & cas : cam)
    {
        sns = sm.enumerate_snapshot_by_disk_ab_path(cas.first);
        for (auto & sn : sns)
        {
            linux_storage::partitionA::ptr p = boost::static_pointer_cast<linux_storage::partitionA>(str->get_instance_from_ab_path(sn->get_block_device_path()));
            uint64_t offset = p->partition_start_offset;
            //printf("offset = %llu\r\n", offset);
            uint64_t end = offset + p->size();
            //printf("end = %llu\r\n", end);
            //printf("cas.size = %d\r\n", cas.second.size());
            changed_area::vtr::iterator ica;
            /*for (auto & ca : cas.second)
            {
                printf("ca.start = %llu , ca.start + ca.length = %llu\r\n", ca.start, ca.start + ca.length);
            }*/
            ica = cas.second.begin();
            changed_area::vtr ca_for_snapshot;
            for (ica = cas.second.begin(); ica != cas.second.end() /*&& ica->start < end*/;)
            {
                uint64_t iend = ica->start + ica->length;
                printf("ica->start = %llu , iend = %llu\r\n", ica->start, iend);
                if (ica->start<offset && iend>offset)
                {
                    printf("1\r\n");
                    printf("ica->start = %llu , ica->length = %llu\r\n", ica->start, ica->length);

                    ica = cas.second.insert(ica+1, changed_area(ica->start, offset - ica->start));
                    ica--;
                    printf("ica->start = %llu , ica->length = %llu\r\n", ica->start, ica->length);
                    printf("1\r\n");
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());
                    printf("%llu, %llu\r\n", 0, iend - offset);
                    //ca_for_snapshot.push_back(changed_area(offset, iend - offset));
                    ca_for_snapshot.push_back(changed_area(0, iend - offset));
                    cam_sn[sn->get_datto_device()].push_back(changed_area(0, iend - offset));
                    printf("cam_sn[sn->get_datto_device()].size() = %d\r\n", cam_sn[sn->get_datto_device()].size());
                    for (auto & ca : cam_sn[sn->get_datto_device()])
                        printf("!? %llu , %llu\r\n", ca.start, ca.length);
                    printf("1\r\n");
                    printf("ica->start = %llu , ica->length = %llu\r\n", ica->start, ica->length);
                    ica = cas.second.erase(ica);
                    printf("ica->start = %llu , ica->length = %llu\r\n", ica->start, ica->length);

                    printf("1\r\n");
                    for (auto & ca : cam_sn[sn->get_datto_device()])
                        printf("!? %llu , %llu\r\n", ca.start, ca.length);

                }
                else if(ica->start>=offset && iend<= end)
                {
                    printf("2\r\n");
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());

                    printf("%llu, %llu\r\n", ica->start - offset, ica->length);
                    //changed_area tca(ica->start - offset, ica->length);
                    printf("2\r\n");
                    //ca_for_snapshot.push_back(changed_area(ica->start - offset, ica->length));
                    ca_for_snapshot.push_back(changed_area(ica->start - offset, ica->length));
                    printf("2\r\n");
                    for (auto & ca : cam_sn[sn->get_datto_device()])
                        printf("!! %llu , %llu\r\n",ca.start,ca.length);
                    cam_sn[sn->get_datto_device()].push_back(changed_area(ica->start - offset, ica->length));
                    printf("2\r\n");
                    ica = cas.second.erase(ica);
                    printf("2\r\n");
                }
                else if (ica->start < end && iend >end)
                {
                    printf("3\r\n");
                    printf("sn->get_datto_device() = %s\r\n", sn->get_datto_device().c_str());
                    printf("%llu, %llu\r\n", ica->start, end - ica->start);
                    //ca_for_snapshot.push_back(changed_area(ica->start, end - ica->start));
                    ca_for_snapshot.push_back(changed_area(ica->start - offset, end - ica->start));
                    printf("3\r\n");
                    cam_sn[sn->get_datto_device()].push_back(changed_area(ica->start - offset, end - ica->start));
                    printf("3\r\n");
                    cas.second.insert(ica, changed_area(end, iend - end));
                    printf("3\r\n");
                    ica = cas.second.erase(ica);
                    printf("3\r\n");
                }
                else if (ica->start >= end)
                {
                    break;
                }
                else
                {
                    ++ica;
                }
            }
        }
    }
    if (cam_sn.size() > 0)
    {
        cam.insert(cam_sn.begin(), cam_sn.end());
    }
    return true;
}
