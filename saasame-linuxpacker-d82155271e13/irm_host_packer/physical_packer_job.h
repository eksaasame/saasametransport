#pragma once
#ifndef physical_packer_job_H
#define physical_packer_job_H

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"
#include "job.h"
#include "universal_disk_rw.h"
#include <deque>
#include "..\linuxfs_parser\linuxfs_parser.h"
#define JOB_EXTENSION L".wjob"

struct changed_area{
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
};

struct replication_block : public io_changed_range{
    typedef boost::shared_ptr<replication_block> ptr;
    typedef std::deque<ptr> queue;
    typedef std::vector<ptr> vtr;
    replication_block(uint64_t _index, uint64_t _start, uint64_t _length, uint64_t _offset) : index(_index){
        start = _start;
        length = _length;
        offset = _offset;
    }
    struct compare {
        bool operator() (replication_block::ptr const & lhs, replication_block::ptr const & rhs) const {
            return (*lhs).Start() + (*lhs).Length() < (*rhs).Start() + (*rhs).Length();
        }
    };
    uint64_t Start()  { return start;  }
    uint64_t Length() { return length; }
    uint64_t Offset() { return offset; }
    uint64_t End()       { return Start() + Length(); }
    uint64_t RealStart() { return Offset() + Start(); }
    uint64_t RealEnd()   { return Offset() + Start() + Length(); }
    uint64_t index;
};

struct physical_packer_job_progress{
    typedef boost::shared_ptr<physical_packer_job_progress> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    physical_packer_job_progress() : total_size(0), backup_progress(0), backup_size(0), backup_image_offset(0) {}
    std::string                         uri;
    std::string                         output;
    std::string                         base;
    std::string                         parent;
    uint64_t                            total_size;
    uint64_t                            backup_progress;
    uint64_t                            backup_size;
    uint64_t                            backup_image_offset;
    std::vector<changed_area>           cdr_changed_areas;
    replication_block::queue            completed;
    macho::windows::critical_section    lock;
};

struct rw_connection{
    typedef boost::shared_ptr<rw_connection> ptr;
    typedef std::deque<ptr> queue;
    typedef std::vector<ptr> vtr;
    rw_connection(){}
    rw_connection(universal_disk_rw::vtr& _outputs);
    universal_disk_rw::vtr outputs;
};

class physical_packer_job : public macho::removeable_job{
public:
    physical_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail);
    physical_packer_job(saasame::transport::create_packer_job_detail detail);
    typedef boost::shared_ptr<physical_packer_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;
    static physical_packer_job::ptr create(std::string id, saasame::transport::create_packer_job_detail detail);
    static physical_packer_job::ptr create(saasame::transport::create_packer_job_detail detail);
    static physical_packer_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file);
    void virtual cancel(){ _is_canceled = true; _is_interrupted = true; save_status(); }
    void virtual resume(){ _is_canceled = false; _is_interrupted = false; }
    void virtual remove();
    saasame::transport::packer_job_detail virtual get_job_detail( boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time() );
    void virtual save_config();
    virtual void interrupt();
    virtual void execute();
protected:
    void virtual save_status();
    bool virtual load_status(std::wstring status_file);

	void virtual save_cdr(const std::map<int, changed_area::vtr>& cdr);
	bool virtual load_cdr(std::map<int, changed_area::vtr> & cdrs );

    bool is_canceled() { return _is_canceled || _is_interrupted; }
    void                    replicate_progress(macho::windows::storage::disk& d, const mgmt_job_state state, const std::wstring& message, const uint64_t& start, const uint64_t& length, const uint64_t& total_size);
    virtual bool            calculate_disk_backup_size(macho::windows::storage::disk& d, uint64_t& backup_size);
	virtual void            calculate_cdr_backup_size(macho::windows::storage::disk& d, changed_area::vtr &cdrs, uint64_t& backup_size);
    virtual bool            cdr_replicate_disk(macho::windows::storage::disk& d, changed_area::vtr &cdrs, const std::string& image_name, const std::string& base_name, physical_packer_job_progress& progress, std::string parent = "");

    virtual bool            replicate_disk(macho::windows::storage::disk& d, const std::string& image_name, const std::string& base_name, physical_packer_job_progress& progress, std::string parent = "");
    virtual bool            replicate_beginning_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs);
    virtual bool            replicate_end_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs);
    virtual bool            replicate_partitions_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs);
    virtual bool            replicate_changed_areas(macho::windows::storage::disk& d, macho::windows::storage::partition& p, std::string path, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs, const physical_vcbt_journal& previous_journal, bool cached = true);
    inline bool             replicate_changed_areas(macho::windows::storage::disk& d, macho::windows::storage::partition& p, std::wstring path, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs, const physical_vcbt_journal& previous_journal, bool cached = true){
        return replicate_changed_areas(d, p, macho::stringutils::convert_unicode_to_ansi(path), progress, backup_size, outputs, previous_journal, cached);
    }
    virtual bool            replicate(universal_disk_rw::ptr &rw, uint64_t& start, const uint32_t length, universal_disk_rw::vtr& outputs, const uint64_t target_offset = 0);
    virtual bool            replicate_changed_areas(universal_disk_rw::ptr &rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t offset, uint64_t& backup_size, changed_area::vtr &changeds, universal_disk_rw::vtr& outputs);
    
    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);
    //void virtual record(saasame::transport::job_state::type state, int error, std::string description);
    
    virtual bool            is_cdr();

    static saasame::transport::create_packer_job_detail  load_config(std::wstring config_file, std::wstring &job_id);
    saasame::transport::hv_guest_os_type::type          _guest_os_type;
    bool _is_canceled;
    bool _is_interrupted;
    int  _state;
    boost::filesystem::path                             _config_file;
    boost::filesystem::path                             _status_file;
	boost::filesystem::path                             _cdr_file;
    boost::posix_time::ptime                            _created_time;
    saasame::transport::create_packer_job_detail        _create_job_detail;
    history_record::vtr                                 _histories;
    macho::windows::critical_section                    _cs;
    virtual bool                                        verify_and_fix_snapshots();
    uint32_t                                            _block_size;
    uint32_t                                            _queue_size;
    uint32_t                                            _mini_block_size;
    rw_connection::queue                                _connections_pool;
    std::string                                         _boot_disk;
    std::set<std::string>                               _system_disks;
    macho::windows::storage::volume::ptr                get_volume_info(macho::string_array_w access_paths, macho::windows::storage::volume::vtr volumes);
    changed_area::vtr                                   get_changed_areas(macho::windows::storage::partition& p, const uint64_t& start_offset);
    changed_area::vtr                                   get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions, const uint64_t& start_offset);
    bool                                                get_output_image_info(const std::string disk, physical_packer_job_progress::ptr &p);
    physical_packer_job_progress::map                   _progress;
    std::map<std::string, changed_area::vtr>            _changed_areas;
private:

    BOOL get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset);

    inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] |= (1 << (bit & 7));
    }

    inline int	test_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] & (1 << (bit & 7));
    }

    inline int clear_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] &= ~(1 << (bit & 7));
    }

    inline void merge_umap(unsigned char* buffer, unsigned char* source, ULONG size_in_bytes){
        for (ULONG index = 0; index < size_in_bytes; index++)
            buffer[index] |= source[index];
    }

    inline int clear_umap(unsigned char* buffer, ULONGLONG start, ULONGLONG length, ULONG resolution){
        int ret = 0;
        for (ULONGLONG curbit = (start / resolution); curbit <= (start + length - 1) / resolution; curbit++){
            ret = clear_umap_bit(buffer, curbit);
        }
        return ret;
    }

    inline int set_umap(unsigned char* buffer, ULONGLONG start, ULONGLONG length, ULONG resolution){
        int ret = 0;
        for (ULONGLONG curbit = (start / resolution); curbit <= (start + length - 1) / resolution; curbit++){ 
            if (0 == test_umap_bit(buffer, curbit)){
                ret = set_umap_bit(buffer, curbit);
            }
        }
        return ret;
    }

    typedef struct {
        LARGE_INTEGER StartingLcn;
        LARGE_INTEGER BitmapSize;
        BYTE  Buffer[1];
    } VOLUME_BITMAP_BUFFER, *PVOLUME_BITMAP_BUFFER;

    std::map<int64_t, physical_vcbt_journal>            _journals;
    physical_vcbt_journal                               get_previous_journal(macho::guid_ volume_id);
    changed_area::vtr                                   get_changed_areas(std::string path, uint64_t start, const physical_vcbt_journal& previous_journal, bool cached = true);
    changed_area::vtr                                   _get_changed_areas(ULONGLONG file_area_offset, LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits, uint32_t bytes_per_cluster, uint64_t& total_number_of_delta);
    boost::shared_ptr<VOLUME_BITMAP_BUFFER>             get_journal_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, const physical_vcbt_journal& previous_journal, uint64_t start = 0);
    boost::shared_ptr<VOLUME_BITMAP_BUFFER>             get_umap_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, uint64_t start = 0);
    boost::shared_ptr<BYTE>                             read_umap_data(boost::filesystem::path p);
    bool                                                exclude_file(LPBYTE buff, boost::filesystem::path file);
    bool                                                get_snapshot_info(macho::windows::storage::disk& d, macho::windows::storage::volume& v, saasame::transport::snapshot& info);
    bool                                                _is_vcbt_enabled;
    bool                                                get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots);
};

class winpe_packer_job : public physical_packer_job{
public:
    winpe_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail);
    winpe_packer_job(saasame::transport::create_packer_job_detail detail);
    virtual void execute();
protected:
    virtual bool            replicate_beginning_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs);
    virtual bool            replicate_partitions_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, physical_packer_job_progress& progress, uint64_t& backup_size, universal_disk_rw::vtr& outputs);
    virtual bool            verify_and_fix_snapshots();
    changed_area::vtr       get_changed_areas(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset);
    changed_area::vtr       get_changed_areas(universal_disk_rw::ptr &disk_rw, ::windows::storage::disk& d, const uint64_t& start_offset);
private:
    changed_area::vtr                               _get_changed_areas(const std::wstring path, const uint64_t start, const uint64_t end);
    linuxfs::io_range::map                          _io_ranges_map;
    linuxfs::lvm_mgmt::groups_map                   _lvm_groups_devices_map;

};

class continuous_data_replication{
public:
	continuous_data_replication(macho::windows::storage::ptr stg) : _stg(stg){}
	continuous_data_replication() : _stg(macho::windows::storage::get()){}
	std::map<int, changed_area::vtr>         get_changeds(std::vector<int> disks, std::map<int64_t, physical_vcbt_journal>&  previous_journals);
	std::map<int64_t, physical_vcbt_journal> get_journals() const { return _journals; }
	virtual ~continuous_data_replication(){}
private:
	std::map<int, changed_area::vtr>		  _changed_areas_map;
	std::map<int64_t, physical_vcbt_journal>  _journals;
	struct changed_blocks_track{
		typedef boost::shared_ptr<changed_blocks_track> ptr;
		typedef std::vector<ptr> vtr;
		struct journal_block{
			typedef std::vector<journal_block> vtr;
			journal_block(uint64_t _start, uint64_t _length, uint64_t _lcn) : start(_start), length(_length), lcn(_lcn){}
			uint64_t	 start;
			uint64_t     length;
			uint64_t	 lcn;
		};

		changed_blocks_track() : journal_id(0), first_key(0), latest_key(0), bytes_per_sector(0), file_area_offset(0), bytes_per_cluster(0), partition_offset(0), partition_size(0), disk_number(-1), partition_number(0){}
		uint64_t	          journal_id;
		uint64_t              first_key;
		uint64_t	          latest_key;
		uint32_t		      bytes_per_sector;
		uint32_t		      file_area_offset;
		uint32_t		      bytes_per_cluster;
		journal_block::vtr    journal_blocks;
		uint64_t		      partition_offset;
		uint64_t		      partition_size;
		uint32_t		      disk_number;
		uint32_t		      partition_number;
		std::wstring	      volume_path;
		physical_vcbt_journal vcbt_journal;
		changed_area::vtr	  get_changed_areas(const physical_vcbt_journal& previous_journal);
		bool				  read_file(HANDLE handle, uint32_t start, uint32_t length, LPBYTE buffer, DWORD& number_of_bytes_read);
		changed_area::vtr     final_changed_area(changed_area::vtr& src, uint64_t max_size = 4 * 1024 * 1024);
		static inline int set_umap(unsigned char* buffer, ULONGLONG start, ULONGLONG length, ULONG resolution){
			int ret = 0;
			for (ULONGLONG curbit = (start / resolution); curbit <= ((start + length - 1) / resolution); curbit++){
				if (0 == test_umap_bit(buffer, curbit)){
					ret = set_umap_bit(buffer, curbit);
				}
			}
			return ret;
		}
		static inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
			return buffer[bit >> 3] |= (1 << (bit & 7));
		}

		static inline int	test_umap_bit(unsigned char* buffer, ULONGLONG bit){
			return buffer[bit >> 3] & (1 << (bit & 7));
		}
	};
	changed_blocks_track::vtr _get_changed_blocks_tracks(int disk_number, std::map<int64_t, physical_vcbt_journal>&  previous_journals);
	macho::windows::storage::ptr _stg;
};

#endif