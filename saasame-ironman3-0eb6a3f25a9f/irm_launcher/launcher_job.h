#pragma once
#ifndef launcher_job_H
#define launcher_job_H

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"
#include "job.h"
#include "aliyun_upload.h"
#include "clone_disk.h"
#ifdef _VMWARE_VADP
#include "vmware.h"
#include "vmware_ex.h"
#pragma comment(lib, "irm_hypervisor_ex.lib")
#endif
#define JOB_EXTENSION L".ujob"
#define _MAX_HISTORY_RECORDS 50

#ifdef _VMWARE_VADP

using namespace mwdc::ironman::hypervisor_ex;
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

#endif

class launcher_job : public macho::removeable_job{
public:
    launcher_job(std::wstring id, saasame::transport::create_job_detail detail);
    launcher_job(saasame::transport::create_job_detail detail);
    typedef boost::shared_ptr<launcher_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;
    void virtual update_create_detail(const saasame::transport::create_job_detail& detail);
    static launcher_job::ptr create(std::string id, saasame::transport::create_job_detail detail);
    static launcher_job::ptr create(saasame::transport::create_job_detail detail);
    static launcher_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file);
    void virtual cancel(){ _is_canceled = true; interrupt(); }
    void virtual remove();
    saasame::transport::launcher_job_detail virtual get_job_detail(bool complete = false);
    void virtual save_config();
    virtual void interrupt();
    virtual void execute();
    macho::trigger::vtr virtual get_triggers();
    bool is_uploading();
    boost::posix_time::ptime get_latest_leave_time() const { return _latest_update_time; }
protected: 	
    ULONGLONG get_version(std::string version_string);
    std::string  read_from_file(boost::filesystem::path file_path);
    bool is_canceled() { return _is_canceled || _is_interrupted; }
    bool virtual get_launcher_job_create_detail(saasame::transport::launcher_job_create_detail &detail);
    bool virtual update_state(const saasame::transport::launcher_job_detail &detail);
    bool virtual is_launcher_job_image_ready();
    void clone_progess(const std::wstring&, const int&);
    void virtual record(saasame::transport::job_state::type state, int error, std::string description);
    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);
    void virtual save_status();
    static saasame::transport::create_job_detail  load_config(std::wstring config_file, std::wstring &job_id);
    void virtual load_create_job_detail(std::wstring config_file);
    bool virtual load_status(std::wstring status_file);
    bool _is_canceled;
    bool _is_interrupted;
    bool _is_initialized;
    int  _state;
    boost::filesystem::path                           _config_file;
    boost::filesystem::path                           _status_file;
    boost::filesystem::path                           _journal_file;
    boost::filesystem::path                           _pre_script_file;
    boost::filesystem::path                           _post_script_file;
    boost::posix_time::ptime                          _created_time;
    saasame::transport::create_job_detail             _create_job_detail;
    saasame::transport::launcher_job_create_detail    _launcher_job_create_detail;
    history_record::vtr                               _histories;
private:

#if defined(_VMWARE_VADP) || defined(_AZURE_BLOB)
    std::wstring get_vmp_device_path();
#endif

#ifdef _VMWARE_VADP
    vmware_virtual_machine::ptr          clone_virtual_machine(vmware_portal_::ptr &portal, mwdc::ironman::hypervisor_ex::vmware_portal_ex &portal_ex, vmware_virtual_machine::ptr src_vm);
    std::map<std::wstring, std::wstring> initial_vmware_vadp_disks(vmware_portal_::ptr& portal,	vmware_virtual_machine::ptr& vm, vmware_vixdisk_connection_ptr& conn, device_mount_repository& repository);
    void launch_vmware_virtual_machine(vmware_portal_::ptr& portal, vmware_virtual_machine::ptr& vm, bool is_need_uefi_boot);
    changed_disk_extent::vtr get_excluded_extents(const changed_area::vtr& changes, uint64_t size);
    static changed_disk_extent::vtr final_changed_disk_extents(changed_disk_extent::vtr& src, changed_disk_extent::vtr& excludes, uint64_t max_size);
    mwdc::ironman::hypervisor_ex::vmware_portal_ex _portal_ex;
#endif

#ifdef _AZURE_BLOB
    std::map<std::wstring, std::wstring> initial_page_blob_disks(device_mount_repository& repository);
#endif

    typedef std::map<std::string, DWORD> disk_map;
    typedef std::map<std::string, boost::filesystem::path> virtual_disk_map; 
    struct upload_progress{
        typedef boost::shared_ptr<upload_progress> ptr;
        typedef std::map<std::string, ptr> map;
        upload_progress() : vhd_size(0), size(0), progress(0), is_completed(false){}
        upload_progress(std::string id, uint64_t v, uint64_t s, uint64_t p, bool completed) : upload_id(id), vhd_size(v), size(s), progress(p), is_completed(completed){}
        std::string upload_id;
        uint64_t    vhd_size;
        uint64_t    size;
        uint64_t    progress;
        bool        is_completed;
    };
    virtual void modify_job_scheduler_interval(uint32_t minutes = INT_MAX);
    bool upload_vhd_to_cloud(disk_map &disk_map);
    void upload_status(const uint32_t part_number, const std::string etag, const uint32_t length);
    bool virtual windows_convert(storage::ptr& stg, disk_map &_disk_map);
    bool virtual linux_convert(disk_map &_disk_map);
    macho::windows::critical_section                  _cs;
    macho::windows::critical_section                  _config_lock;
    void                                              _update( bool loop );
    bool                                              _terminated;
    boost::thread                                     _thread;
    std::string                                       _boot_disk;
    bool                                              _is_windows_update;
    std::string                                       _mgmt_addr;
    boost::posix_time::ptime                          _latest_update_time;
    std::string                                       _platform;
    std::string                                       _architecture;
    uint64_t                                          _total_upload_size;
    uint32_t                                          _disk_size;
    uint64_t                                          _upload_progress;
    std::string                                       _upload_id;
    std::string                                       _uploading_disk;
    std::map<int, std::string>                        _uploaded_parts;
    static std::wstring const                         _services[];
    std::string                                       _host_name;
    upload_progress::map                              _upload_maps;
    macho::windows::file_lock_ex::ptr                 _vhd_export_lock;
    std::wstring                                      _virtual_machine_id;
};

#endif