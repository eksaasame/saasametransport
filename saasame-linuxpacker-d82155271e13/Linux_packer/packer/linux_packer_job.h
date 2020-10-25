#pragma once
#ifndef linux_packer_job_H
#define linux_packer_job_H


#include "../tools/jobs_scheduler.h"
#include "include.h"
#include "../tools/storage.h"
using namespace linux_storage;
#include "../tools/snapshot.h"
#include "../tools/clone_disk.h"
#include "job.h"
#define JOB_EXTENSION ".xjob"

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif
#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif
class physical_packer_service_handler;

struct replication_block : public io_changed_range {
    typedef boost::shared_ptr<replication_block> ptr;
    typedef std::deque<ptr> queue;
    typedef std::vector<ptr> vtr;
    replication_block(uint64_t _index, uint64_t _start, uint64_t _length, uint64_t _src_start, universal_disk_rw::ptr _in) : index(_index), src_start(_src_start), in(_in) 
    { 
        start = _start;
        length = _length;
        offset = start - src_start; 
    }
    struct compare {
        bool operator() (replication_block::ptr const & lhs, replication_block::ptr const & rhs) const {
            return (*lhs).start < (*rhs).start;
        }
    };
    universal_disk_rw::ptr in;
    uint64_t src_start;
    uint64_t index;
    uint64_t end() { return start + length; }
};


struct physical_packer_job_progress {
    typedef boost::shared_ptr<physical_packer_job_progress> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    physical_packer_job_progress() : total_size(0), backup_progress(0), backup_size(0), backup_image_offset(0), cow_error(false){}
    std::string                         uri;
    std::string                         output;
    std::string                         base;
    std::string                         parent;
    uint64_t                            total_size;
    uint64_t                            backup_progress;       //
    uint64_t                            backup_size;           //it mean the finined block sizes
    uint64_t                            backup_image_offset;
    replication_block::queue            completed;
    bool                                cow_error;
    bool                                b_force_full;
    boost::recursive_mutex              lock;
};


class linux_packer_job : public removeable_job
{
public:
    typedef boost::shared_ptr<linux_packer_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
	linux_packer_job(std::string id,const saasame::transport::create_packer_job_detail &detail, snapshot_manager::ptr sh);
	
    static linux_packer_job::ptr create(std::string id, saasame::transport::create_packer_job_detail detail, snapshot_manager::ptr sh, physical_packer_service_handler* packer, bool b_force_full, bool is_skip_read_error);
    static linux_packer_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file, snapshot_manager::ptr sh, bool b_force_full);
    saasame::transport::packer_job_detail get_job_detail( boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time() );
    bool verify_and_fix_snapshots();
    bool get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots);
    bool replicate(universal_disk_rw::ptr &rw, uint64_t& start, const uint32_t length, universal_disk_rw::vtr& outputs, const uint64_t target_offset);
    bool replicate_changed_areas(/*universal_disk_rw::ptr &rw,*/ disk::ptr d, uint64_t& start_offset, uint64_t offset, uint64_t& backup_size, changed_area::vtr &changeds, universal_disk_rw::vtr& outputs);
    bool modify_change_area_with_datto_snapshot(changed_area::map & cam, snapshot_manager::ptr sh);
    void setting_force_full() { _is_force_full = true; _progress.clear();}
    bool is_force_full() { return _is_force_full;}
    void setting_skip_read_error() { _is_skip_read_error = true; }
    void set_packer(physical_packer_service_handler* packer) { _packer = packer; }
    void set_modify_disks(disk::map input) { _modify_disks = input; }
    saasame::transport::create_packer_job_detail * get_create_job_detail() { return &_create_job_detail; }
	virtual void remove();
	virtual void interrupt();
	virtual void execute();
    void virtual cancel() { _is_canceled = true; _is_interrupted = true; save_status(); }
    void virtual resume() { _is_canceled = false; _is_interrupted = false; }

	
	virtual void save_config();

    void record(saasame::transport::job_state::type state, int error, record_format format);
private:
	/*struct changed_area{
		typedef std::vector<changed_area> vtr;
		changed_area() : start_offset(0), length(0){}
		changed_area(uint64_t _start_offset, uint64_t _length) : start_offset(_start_offset), length(_length){}
		struct compare {
			bool operator() (changed_area const & lhs, changed_area const & rhs) const {
				return lhs.start_offset < rhs.start_offset;
			}
		};
		uint64_t start_offset;
		uint64_t length;
	};*/
protected:
    static saasame::transport::create_packer_job_detail  load_config(std::string config_file, std::string &job_id);
    void virtual save_status();
    bool virtual load_status(std::string status_file);
    bool                                                get_output_image_info(const std::string disk, physical_packer_job_progress::ptr &p);
    bool 											    calculate_disk_backup_size(disk::ptr d, uint64_t& start, uint64_t& backup_size);
    bool            									replicate_disk(disk::ptr d, const std::string& image_name, const std::string& base_name, uint64_t& start, uint64_t& backup_size, std::string parent = "");
    bool is_canceled() { return _is_canceled || _is_interrupted; }
    void progress(disk::ptr d, const mgmt_job_state state, const std::string& message, const uint64_t& start, const uint64_t& length, const uint64_t& total_size) {
#ifdef _DEBUG    
        LOG_WARNING("Replicated the path (%s) (%I64u, %I64u, %I64u)", message.c_str(), start, length, total_size);
#endif
        save_status();
    }

    std::map<int64_t, physical_vcbt_journal>            _journals;
	boost::filesystem::path                             _config_file;
    boost::filesystem::path                             _status_file;
    boost::posix_time::ptime                            _created_time;
    saasame::transport::create_packer_job_detail        _create_job_detail;
    history_record::vtr                                 _histories;
	boost::recursive_mutex           				    _cs;
	saasame::transport::hv_guest_os_type::type          _guest_os_type;
    std::set<std::string>                               _system_disks;
    std::string                                         _boot_disk;
	bool												_is_interrupted;
	bool												_is_canceled;
    bool												_is_force_full;
    bool												_is_skip_read_error;
	int													_state;
	int													_block_size;
	int													_mini_block_size;
	physical_packer_job_progress::map                   _progress;
    changed_area::map                                   _changed_areas;
    uint32_t                                            _queue_size;
    snapshot_manager::ptr                               _sh;
    physical_packer_service_handler*                    _packer;
    disk::map                                           _modify_disks;
};
#endif