#pragma once
#ifndef __BOOT_INI__
#define __BOOT_INI__

#include "macho.h"

class boot_ini_edit{
public:
    boot_ini_edit(std::wstring path) : _path(path){}
    bool set_safe_mode(__in bool is_enable, __in bool is_domain_controller);
    bool set_debug_settings(__in bool is_debug = false, __in bool is_boot_debug = false);
    static bool set_safe_mode(__in std::wstring path, __in bool is_enable, __in bool is_domain_controller);
    static bool set_debug_settings(__in std::wstring path, __in bool is_debug = false, __in bool is_boot_debug = false);
    virtual ~boot_ini_edit(){}
private:
    std::wstring _path;
};

#endif