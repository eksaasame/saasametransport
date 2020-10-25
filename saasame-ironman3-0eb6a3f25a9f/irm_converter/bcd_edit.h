#pragma once
#ifndef __BCD_EDIT__
#define __BCD_EDIT__

#include "macho.h"

class bcd_edit{
public:
    typedef enum BootStatusPolicy{
        BootStatusPolicyDisplayAllFailures = 0,     // Display all failures.
        BootStatusPolicyIgnoreAllFailures = 1,     // Ignore all failures.
        BootStatusPolicyIgnoreShutdownFailures = 2,     // Ignore all shutdown failures.
        BootStatusPolicyIgnoreBootFailures = 3      // Ignore all boot failures.
    }BootStatusPolicy;
    typedef boost::shared_ptr<bcd_edit> ptr;
    struct  exception : virtual public macho::exception_base {};
    bcd_edit(std::wstring bcd_file_path = L"");
    virtual bool set_safe_mode(__in bool is_enable, __in bool is_domain_controller);
    virtual bool set_boot_status_policy_option(__in bool is_enable, BootStatusPolicy policy = BootStatusPolicyIgnoreShutdownFailures);
    virtual bool set_detect_hal_option(__in bool is_enable);
    virtual bool set_boot_system_device(__in std::wstring system_volume_path, __in bool is_virtual_disk);
    virtual bool set_debug_settings(__in bool is_debug = false, __in bool is_boot_debug = false);

    virtual ~bcd_edit(){}
private:
    macho::windows::wmi_object _get_bcd_boot_manager();
    macho::windows::wmi_object _get_bcd_default_boot_entry(macho::windows::wmi_object& bcd_boot_manager);
    std::wstring               _get_bcd_default_entry(macho::windows::wmi_object& bcd_boot_manager);
    macho::windows::wmi_services _wmi;
    macho::windows::wmi_object   _bcd_store;

};

class bcd_edit_cli : virtual public bcd_edit{
public:
    bcd_edit_cli(std::wstring bcd_file_path = L"");
    virtual bool set_safe_mode(__in bool is_enable, __in bool is_domain_controller);
    virtual bool set_boot_status_policy_option(__in bool is_enable, BootStatusPolicy policy = BootStatusPolicyIgnoreShutdownFailures);
    virtual bool set_detect_hal_option(__in bool is_enable);
    virtual bool set_boot_system_device(__in std::wstring system_volume_path, __in bool is_virtual_disk);
    virtual bool set_debug_settings(__in bool is_debug = false, __in bool is_boot_debug = false);
    virtual ~bcd_edit_cli(){}
private:
    std::wstring _get_default_entry_path();
    std::wstring _bcd_file;
};

#endif