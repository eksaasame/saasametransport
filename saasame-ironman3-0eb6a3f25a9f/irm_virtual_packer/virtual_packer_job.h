#pragma once
#ifndef virtual_packer_job_H
#define virtual_packer_job_H

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"
#include "job.h"
#include "universal_disk_rw.h"
#include "vmware_ex.h"
#include "..\linuxfs_parser\linuxfs_parser.h"

#define JOB_EXTENSION L".vjob"

struct replication_block : public io_changed_range{
    typedef boost::shared_ptr<replication_block> ptr;
    typedef std::deque<ptr> queue;
    typedef std::vector<ptr> vtr;
    replication_block(uint64_t _index, uint64_t _start, uint64_t _length) : index(_index){
        start = _start;
        length = _length;
    }
    struct compare {
        bool operator() (replication_block::ptr const & lhs, replication_block::ptr const & rhs) const {
            return (*lhs).Start() + (*lhs).Length() < (*rhs).Start() + (*rhs).Length();
        }
    };
    uint64_t Start() { return start; }
    uint64_t Length() { return length; }
    uint64_t End()       { return Start() + Length(); }
    uint64_t index;
};

struct virtual_transport_job_progress
{
    typedef boost::shared_ptr<virtual_transport_job_progress> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr, macho::stringutils::no_case_string_less_a> map;
    virtual_transport_job_progress() : total_size(0), backup_progress(0), backup_size(0), backup_image_offset(0) {}
    std::string                         uuid;
    std::string                         vmdk;
    std::string                         output;
    std::string                         base;
    std::string                         parent;
    uint64_t                            total_size;
    uint64_t                            backup_progress;
    uint64_t                            backup_size;
    uint64_t                            backup_image_offset;
    replication_block::queue            completed;
    macho::windows::critical_section    lock;
};

class virtual_packer_job : public macho::removeable_job
{
public:
    virtual_packer_job(std::wstring id, saasame::transport::create_packer_job_detail detail);
    virtual_packer_job(saasame::transport::create_packer_job_detail detail);
    typedef boost::shared_ptr<virtual_packer_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;
    static virtual_packer_job::ptr create(std::string id, saasame::transport::create_packer_job_detail detail);
    static virtual_packer_job::ptr create(saasame::transport::create_packer_job_detail detail);
    static virtual_packer_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file);
    void virtual cancel(){ _is_canceled = true; interrupt(); }
    void virtual remove();
    saasame::transport::packer_job_detail virtual get_job_detail(boost::posix_time::ptime previous_updated_time = boost::posix_time::microsec_clock::universal_time());
    void virtual save_config();
    virtual void interrupt();
    virtual void execute();
protected:

    std::vector<saasame::transport::virtual_disk_info_ex> get_virtual_disk_infos(std::map<std::string, universal_disk_rw::ptr> &vmdk_rws);
    bool get_vss_reserved_sectors(universal_disk_rw::ptr &rw, mwdc::ironman::hypervisor::changed_disk_extent &extent, bool &bootable_disk);
    mwdc::ironman::hypervisor::changed_disk_extent::vtr get_windows_excluded_file_extents(universal_disk_rw::ptr &rw, bool &system_disk);
    mwdc::ironman::hypervisor::changed_disk_extent::vtr get_linux_excluded_file_extents(universal_disk_rw::ptr &rw);
    mwdc::ironman::hypervisor::changed_disk_extent::vtr get_linux_partition_excluded_extents(const std::wstring path, uint64_t start, uint64_t size);
    mwdc::ironman::hypervisor::changed_disk_extent::vtr final_extents(const mwdc::ironman::hypervisor::changed_disk_extent::vtr& src, mwdc::ironman::hypervisor::changed_disk_extent::vtr& excludes);
    virtual bool            calculate_disk_backup_size(universal_disk_rw::ptr &rw, mwdc::ironman::hypervisor::changed_disk_extent::vtr& extents, uint64_t& backup_size, bool &bootable_disk, bool &system_disk);
    virtual bool            replicate_disk(universal_disk_rw::ptr &rw, mwdc::ironman::hypervisor::changed_disk_extent::vtr& extents, virtual_transport_job_progress::ptr& p);

    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);
    //void virtual record(saasame::transport::job_state::type state, uint64_t error, std::string description);
    void virtual save_status();
    static saasame::transport::create_packer_job_detail  load_config(std::wstring config_file, std::wstring &job_id);
    bool                                                 get_output_image_info(const std::string disk, virtual_transport_job_progress::ptr &p);

    bool virtual load_status(std::wstring status_file);
    bool        _is_canceled;
    bool        _is_interrupted;
    int64_t     _state;
    boost::filesystem::path                         _config_file;
    boost::filesystem::path                         _status_file;
    boost::posix_time::ptime                        _created_time;
    saasame::transport::create_packer_job_detail    _create_job_detail;
    history_record::vtr                             _histories;
    macho::windows::critical_section                _cs;
private:
    bool is_canceled() { return _is_canceled || _is_interrupted; }
    std::map<std::string, std::string>              _change_ids;
    virtual_transport_job_progress::map             _progress;
    uint32_t                                        _block_size;
    saasame::transport::hv_guest_os_type::type      _guest_os_type;
    uint32_t                                        _min_transport_size;
    std::string                                     _boot_disk;
    std::set<std::string>                           _system_disks;
    io_range::map                                   _io_ranges_map;
    linuxfs::lvm_mgmt::groups_map                         _lvm_groups_devices_map;
    std::vector<saasame::transport::virtual_disk_info_ex> _virtual_disk_infos;
};

#endif