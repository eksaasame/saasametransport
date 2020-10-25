
#include "boot_ini.h"

#define NETWORK_SAFEBOOT_BOOTOPTION         _T(" /safeboot:network")
#define DSREPAIR_SAFEBOOT_BOOTOPTION        _T(" /safeboot:dsrepair")
#define SOS_BOOTOPTION                      _T(" /sos")
#define BOOTLOG_BOOTOPTION                  _T(" /bootlog")
#define NOGUI_BOOTOPTION                    _T(" /noguiboot")
#define DEBUG_BOOTOPTIONS                   _T(" /debug /debugport=COM1: /baudrate=115200")

template<typename T>
void remove_substrs(std::basic_string<T>& s,
    const std::basic_string<T>& p) {
    std::basic_string<T>::size_type n = p.length();

    for (std::basic_string<T>::size_type i = s.find(p);
        i != std::basic_string<T>::npos;
        i = s.find(p))
        s.erase(i, n);
}

bool boot_ini_edit::set_safe_mode(__in std::wstring path, __in bool is_enable, __in bool is_domain_controller){
    boot_ini_edit edit(path);
    return edit.set_safe_mode(is_enable, is_domain_controller);
}

bool boot_ini_edit::set_debug_settings(__in std::wstring path, __in bool is_debug, __in bool is_boot_debug){
    boot_ini_edit edit(path);
    return edit.set_debug_settings(is_debug, is_boot_debug);
}

bool boot_ini_edit::set_safe_mode(__in bool is_enable, __in bool is_domain_controller){
    BOOL bChanged = FALSE;
    TCHAR buf[512] = { 0 };
    bool result = false;
    DWORD  attrs = GetFileAttributes(_path.c_str());
    if (attrs & FILE_ATTRIBUTE_READONLY)
        bChanged = SetFileAttributes(_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    DWORD ret = GetPrivateProfileString(L"boot loader", L"default", L"", buf, sizeof(buf), _path.c_str());
    if (ret){
        std::wstring default_boot_entry = buf;
        ret = GetPrivateProfileString(L"operating systems", default_boot_entry.c_str(), L"", buf, sizeof(buf), _path.c_str());
        if (ret){
            std::wstring boot_parameters = buf;
            remove_substrs(boot_parameters, std::wstring(DSREPAIR_SAFEBOOT_BOOTOPTION));
            remove_substrs(boot_parameters, std::wstring(NETWORK_SAFEBOOT_BOOTOPTION));
            remove_substrs(boot_parameters, std::wstring(SOS_BOOTOPTION));
            remove_substrs(boot_parameters, std::wstring(BOOTLOG_BOOTOPTION));
            remove_substrs(boot_parameters, std::wstring(NOGUI_BOOTOPTION));
            if (is_enable){
                std::wstring boot_options = is_domain_controller ? DSREPAIR_SAFEBOOT_BOOTOPTION : NETWORK_SAFEBOOT_BOOTOPTION;
                boot_options += SOS_BOOTOPTION;
                if (!is_domain_controller){
                    boot_options += BOOTLOG_BOOTOPTION;
                    boot_options += NOGUI_BOOTOPTION;
                }
                boot_parameters += boot_options;
            }
            result = (TRUE == WritePrivateProfileString(_T("operating systems"), default_boot_entry.c_str(), boot_parameters.c_str(), _path.c_str()));
        }
    }
    if (bChanged)
        SetFileAttributes(_path.c_str(), attrs);
    return result;
}

bool boot_ini_edit::set_debug_settings(__in bool is_debug, __in bool is_boot_debug){
    BOOL bChanged = FALSE;
    TCHAR buf[512] = { 0 };
    bool result = false;
    DWORD  attrs = GetFileAttributes(_path.c_str());
    if (attrs & FILE_ATTRIBUTE_READONLY)
        bChanged = SetFileAttributes(_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    DWORD ret = GetPrivateProfileString(L"boot loader", L"default", L"", buf, sizeof(buf), _path.c_str());
    if (ret){
        std::wstring default_boot_entry = buf;
        ret = GetPrivateProfileString(L"operating systems", default_boot_entry.c_str(), L"", buf, sizeof(buf), _path.c_str());
        if (ret){
            std::wstring boot_parameters = buf;
            remove_substrs(boot_parameters, std::wstring(DEBUG_BOOTOPTIONS));
            if (is_debug){
                std::wstring boot_options = DEBUG_BOOTOPTIONS;
                boot_parameters += boot_options;
            }
            result = (TRUE == WritePrivateProfileString(_T("operating systems"), default_boot_entry.c_str(), boot_parameters.c_str(), _path.c_str()));
        }
    }
    if (bChanged)
        SetFileAttributes(_path.c_str(), attrs);
    return result;
}
