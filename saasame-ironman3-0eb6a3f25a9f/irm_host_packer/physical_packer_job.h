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
    typedef std::map<int, vtr> map;
    replication_block(uint64_t _index, uint64_t _start, uint64_t _length, uint64_t _offset) : index(_index){
        start = _start;
        length = _length;
        offset = _offset;
    }
    struct compare {
        bool operator() (replication_block::ptr const & lhs, replication_block::ptr const & rhs) const {
            return (*lhs).RealStart() < (*rhs).RealStart();
        }
    };
    uint64_t               Start()  { return start;  }
    uint64_t               Length() { return length; }
    uint64_t               Offset() { return offset; }
    uint64_t               End()       { return start + length; }
    uint64_t               RealStart() { return offset; }
    uint64_t               RealEnd()   { return offset + length; }
    uint64_t               index;
    boost::shared_ptr<std::string> path;
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
    replication_block::vtr              cdr_changed_areas;
    replication_block::queue            completed;
    bool                                is_completed() { return backup_image_offset == total_size; }
    macho::windows::critical_section    lock;
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
    typedef struct {
        LARGE_INTEGER StartingLcn;
        LARGE_INTEGER BitmapSize;
        BYTE  Buffer[1];
    } VOLUME_BITMAP_BUFFER, *PVOLUME_BITMAP_BUFFER;
    struct volume_bitmap{
        volume_bitmap() :bytes_per_cluster(0), is_delta(false), file_area_offset(0){}
        boost::shared_ptr<VOLUME_BITMAP_BUFFER> bitmap;
        uint32_t bytes_per_cluster;
        uint64_t file_area_offset;
        bool     is_delta;
    };

    void virtual                    save_status();
    bool virtual                    load_status(std::wstring status_file);

    void virtual                    save_cdr(const replication_block::map& cdr);
    bool virtual                    load_cdr(replication_block::map & cdrs);

    bool                            is_canceled() { return _is_canceled || _is_interrupted; }
    virtual bool                    is_cdr();

    virtual replication_block::map  get_replication_blocks(const std::map<std::string, storage::disk::ptr>& disks);
    virtual void                    calculate_backup_size(macho::windows::storage::disk& d, physical_packer_job_progress& p, replication_block::vtr& blocks);
    virtual replication_block::vtr  get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions);
    virtual replication_block::vtr  get_partition_replication_blocks(macho::windows::storage::partition& p, const uint64_t& start_offset);
    virtual bool                    replicate_disk(macho::windows::storage::disk& d, replication_block::vtr& blocks, physical_packer_job_progress& p);
    
    void                            get_volume_bitmap(volume_bitmap &bitmap, std::string path, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths = std::set<std::wstring>(), std::set<std::wstring> resync_paths = std::set<std::wstring>());
    replication_block::vtr          get_replication_blocks(storage::partition& p, std::string path, uint64_t start, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths = std::set<std::wstring>(), std::set<std::wstring> resync_paths = std::set<std::wstring>());
    replication_block::map          get_replication_blocks(storage::volume& v, std::string path, const physical_vcbt_journal& previous_journal, std::set<std::wstring> excluded_paths, std::set<std::wstring> resync_paths);
    void virtual                    record(saasame::transport::job_state::type state, int error, record_format& format);
    
    changed_area::vtr                                   _get_changed_areas(ULONGLONG file_area_offset, LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits, uint32_t bytes_per_cluster, uint64_t& total_number_of_delta);
    virtual bool                                        verify_and_fix_snapshots();
    bool                                                get_output_image_info(const std::string disk, physical_packer_job_progress::ptr &p);
    macho::windows::storage::volume::ptr                get_volume_info(macho::string_array_w access_paths, macho::windows::storage::volume::vtr volumes);
    static saasame::transport::create_packer_job_detail load_config(std::wstring config_file, std::wstring &job_id);
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
    uint32_t                                            _block_size;
    uint32_t                                            _queue_size;
    uint32_t                                            _mini_block_size;
    uint32_t                                            _delta_mini_block_size;
    uint32_t                                            _full_mini_block_size;
    std::string                                         _boot_disk;
    std::set<std::string>                               _system_disks;
    physical_packer_job_progress::map                   _progress;

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

    std::map<int64_t, physical_vcbt_journal>            _journals;
    std::map<std::wstring, std::set<std::wstring>>      _resync_paths;
    std::map<std::wstring, std::set<std::wstring>>      _excluded_paths;
    physical_vcbt_journal                               get_previous_journal(macho::guid_ volume_id);
    boost::shared_ptr<VOLUME_BITMAP_BUFFER>             get_journal_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, const physical_vcbt_journal& previous_journal, uint64_t start = 0);
    boost::shared_ptr<VOLUME_BITMAP_BUFFER>             get_umap_bitmap(ULONGLONG file_area_offset, std::string path, uint32_t bytes_per_cluster, uint64_t start = 0);
    boost::shared_ptr<BYTE>                             read_umap_data(boost::filesystem::path p);
    bool                                                sync_file(LPBYTE buff, boost::filesystem::path file, bool log_message, bool exclude);
    bool                                                get_snapshot_info(macho::windows::storage::disk& d, macho::windows::storage::volume& v, saasame::transport::snapshot& info);
    bool                                                _is_vcbt_enabled;
    bool                                                get_all_snapshots(std::map<std::string, std::vector<snapshot> >& snapshots);
};

class winpe_packer_job : public physical_packer_job{
public:
    winpe_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail);
    winpe_packer_job(saasame::transport::create_packer_job_detail detail);
protected:
    virtual replication_block::map  get_replication_blocks(const std::map<std::string, storage::disk::ptr>& disks);
    virtual bool                    verify_and_fix_snapshots();
private:
    replication_block::vtr          get_replication_blocks(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset);
    replication_block::vtr          get_replication_blocks(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, const uint64_t& start_offset);
    replication_block::vtr          _get_replication_blocks(const std::wstring path, const uint64_t start, const uint64_t end);
    io_range::map                   _io_ranges_map;
    linuxfs::lvm_mgmt::groups_map   _lvm_groups_devices_map;};

class continuous_data_replication{
public:
    continuous_data_replication(macho::windows::storage::ptr stg) : _stg(stg){}
    continuous_data_replication() : _stg(macho::windows::storage::get()){}
    replication_block::map                   get_changeds(std::map<std::string, storage::disk::ptr> disks, std::map<int64_t, physical_vcbt_journal>&  previous_journals);
    std::map<int64_t, physical_vcbt_journal> get_journals() const { return _journals; }
    virtual ~continuous_data_replication(){}
private:
    std::map<std::wstring, bool>              _processed_dynamic_volumes;
    replication_block::map          		  _changed_areas_map;
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

        changed_blocks_track() : journal_id(0), first_key(0), latest_key(0), bytes_per_sector(0), file_area_offset(0), bytes_per_cluster(0), partition_offset(0){}
        uint64_t	          journal_id;
        uint64_t              first_key;
        uint64_t	          latest_key;
        uint32_t		      bytes_per_sector;
        uint32_t		      file_area_offset;
        uint32_t		      bytes_per_cluster;
        journal_block::vtr    journal_blocks;
        uint64_t		      partition_offset;
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
    changed_blocks_track::vtr _get_changed_blocks_tracks(storage::disk& d, std::map<int64_t, physical_vcbt_journal>&  previous_journals);
    changed_area::vtr         _get_journal_changed_blocks(HANDLE handle, const std::wstring& path, uint64_t offset, std::map<int64_t, physical_vcbt_journal>&  previous_journals, changed_blocks_track::vtr& tracks);
    macho::windows::storage::ptr _stg;
};

#endif