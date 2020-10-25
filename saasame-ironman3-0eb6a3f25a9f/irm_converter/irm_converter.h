#pragma once
#ifndef __irm_converter__
#define __irm_converter__

#include <macho.h>
#include "bcd_edit.h"
#include "temp_drive_letter.h"
#include "os_image_info.h"

struct windows_element{
    windows_element();
    std::wstring windows_dir;
    bool      is_boot_volume;
    bool      is_mbr_disk;
    bool      is_template;
    GUID      disk_guid;
    GUID      partition_guid;
    DWORD     mbr_signature;
    ULONGLONG partition_offset;
};

struct conversion{
    enum type{
        any_to_any = 0,
        openstack = 1,
        xen = 2,
        vmware = 3,
        hyperv = 4,
        qemu = 5
    };
};

struct irm_conv_device{
    typedef std::vector<irm_conv_device> vtr;
    std::wstring              description;
    std::vector<std::wstring> hardware_ids;
    std::vector<std::wstring> compatible_ids;
};

struct irm_conv_parameter{
    irm_conv_parameter(); 
    irm_conv_parameter(const irm_conv_parameter& param);
    void copy(const irm_conv_parameter& param);
    const irm_conv_parameter &operator =(const irm_conv_parameter& param);
    std::wstring                   drivers_path;
    std::string                    config;
    std::string                    post_script;
    std::set<std::string>          callbacks;
    int                            callback_timeout;
    bool                           is_sysvol_authoritative_restore;
    bool                           is_enable_debug;
    bool                           is_disable_machine_password_change;
    bool                           is_force_normal_boot;
    bool                           skip_system_injection;
    DWORD                          sectors_per_track;
    DWORD                          tracks_per_cylinder;
    conversion::type               type; 
    irm_conv_device::vtr           devices;
    std::map<std::wstring, int>    services;
};

class irm_converter{
public:
    irm_converter();
    ~irm_converter();
    bool initialize(__in const int disk_number);
    bool gpt_to_mbr();
    bool remove_hybird_mbr();
    bool convert(irm_conv_parameter parameter);
    bool is_pending_windows_update() const {
        return _is_pending_windows_update;
    }
    macho::windows::operating_system get_operating_system_info() const {
        return _operating_system;
    }
    std::wstring computer_name() { return _computer_name; }
private:
    void			  mount_volumes();
    void              dismount_volumes();
    bool              get_bcd_edit();
    bool              extract_system_driver_file(os_image_info &os, std::wstring name);
    bool              hyperv(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config);
    bool              vmware(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, irm_conv_device::vtr& devices, std::string config);
    bool              openstack(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config);
    bool              xen(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config);
    bool              qemu(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config);
    bool              any_to_any(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, irm_conv_device::vtr& devices, std::string config);

    bool                               _is_boot_ini_file;
    boost::filesystem::path            _windows_volume;
    boost::filesystem::path            _boot_volume;
    boost::filesystem::path            _boot_cfg;
    boost::filesystem::path            _boot_cfg_folder;
    macho::windows::storage::ptr       _stg;
    macho::windows::storage::disk::ptr _disk;
    bcd_edit::ptr                      _bcd;
    temp_drive_letter::ptr             _system_drive;
    temp_drive_letter::ptr             _windows_drive;
    windows_element                    _win_element;
    bool                               _is_pending_windows_update;
    macho::windows::operating_system   _operating_system;
    std::wstring                       _computer_name;
};

#endif