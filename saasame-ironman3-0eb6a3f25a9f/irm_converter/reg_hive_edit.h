#pragma once
#ifndef __irm_reg_hive_edit__
#define __irm_reg_hive_edit__

#include <macho.h>

class reg_hive_edit : virtual public macho::windows::reg_native_edit{
public:
    struct  exception : virtual public macho::exception_base {};
    typedef boost::shared_ptr<reg_hive_edit> ptr;
    typedef std::vector<ptr> vtr;
    reg_hive_edit(boost::filesystem::path dir);
    virtual ~reg_hive_edit();

    virtual LONG delete_key(
        __in        HKEY hKey,
        __in        LPCTSTR lpSubKey,
        __in        REGSAM samDesired,
        __reserved  DWORD Reserved
        ){
        std::wstring subKey = map_offline_key_path(hKey, lpSubKey);
        return reg_native_edit::delete_key(hKey, subKey.c_str(), samDesired, Reserved);
    }

    virtual LONG delete_key(__in  HKEY hKey, __in  LPCTSTR lpSubKey){
        std::wstring subKey = map_offline_key_path(hKey, lpSubKey);
        return reg_native_edit::delete_key(hKey, subKey.c_str());
    }

    virtual LONG create_key(
        __in        HKEY hKey,
        __in        LPCTSTR lpSubKey,
        __reserved  DWORD Reserved,
        __in_opt    LPTSTR lpClass,
        __in        DWORD dwOptions,
        __in        REGSAM samDesired,
        __in_opt    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        __out       PHKEY phkResult,
        __out_opt   LPDWORD lpdwDisposition
        ){
        std::wstring subKey = map_offline_key_path(hKey, lpSubKey);
        return reg_native_edit::create_key(hKey, subKey.c_str(), Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
    }

    virtual LONG open_key(
        __in        HKEY hKey,
        __in_opt    LPCTSTR lpSubKey,
        __reserved  DWORD ulOptions,
        __in        REGSAM samDesired,
        __out       PHKEY phkResult
        ){
        std::wstring subKey = map_offline_key_path(hKey, lpSubKey);
        return reg_native_edit::open_key(hKey, subKey.c_str(), ulOptions, samDesired, phkResult);
    }

    virtual LONG delete_tree(__in HKEY hKey, __in_opt  LPCTSTR lpSubKey){
        std::wstring subKey = map_offline_key_path(hKey, lpSubKey);
        return reg_native_edit::delete_tree(hKey, subKey.c_str());
    }
private:

    std::wstring map_offline_key_path(HKEY hKey, std::wstring szSubKey);

    class hive_file{
    public:
        std::wstring name() const { return _name; }
        std::wstring key() const { return _key; }
        typedef boost::shared_ptr<hive_file> ptr;
        typedef std::map<std::wstring, ptr> map;
        static hive_file::ptr load_key(std::wstring name, boost::filesystem::path path);
        ~hive_file();
    private:
        hive_file(){}
        std::wstring                        _key;
        std::wstring                        _name;
        macho::windows::auto_reg_key_handle _handle;
    };
    hive_file::map        _hives;
    std::wstring          _default_control_set;
};

#endif