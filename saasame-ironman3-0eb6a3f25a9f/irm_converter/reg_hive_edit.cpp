#include "reg_hive_edit.h"

using namespace macho;
using namespace macho::windows;

reg_hive_edit::hive_file::~hive_file(){
    RegFlushKey(_handle);
    _handle = NULL;
    RegUnLoadKey(HKEY_LOCAL_MACHINE, _key.c_str());
}

reg_hive_edit::hive_file::ptr reg_hive_edit::hive_file::load_key(std::wstring name, boost::filesystem::path path){

    if (!boost::filesystem::exists(path)) {
        BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, boost::str(boost::wformat(L"ERROR_FILE_NOT_FOUND(%s).") % path.wstring()));
    }
    else{
        macho::windows::mutex m(L"hive_file");
        macho::windows::auto_lock lock(m);
        LONG result;
        LONG count = 0;
        while (true){
            HKEY hKey;
            std::wstring key = boost::str(boost::wformat(L"_%s%08d") % name% count);
            result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey);
            if (result != ERROR_SUCCESS){
                if (result == ERROR_FILE_NOT_FOUND){
                    result = RegLoadKeyW(HKEY_LOCAL_MACHINE, key.c_str(), path.wstring().c_str());
                    if (result == ERROR_SUCCESS){
                        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey);
                        if (result == ERROR_SUCCESS){
                            hive_file::ptr p = hive_file::ptr(new hive_file());
                            p->_key = key;
                            p->_name = name;
                            p->_handle = hKey;
                            return p;
                        }
                        else{
                            RegUnLoadKey(HKEY_LOCAL_MACHINE, key.c_str());
                            BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, result, boost::str(boost::wformat(L"RegOpenKeyExW(HKEY_LOCAL_MACHINE, %s) Error.") % key));
                            break;
                        }
                    }
                    else{
                        BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, result, boost::str(boost::wformat(L"RegLoadKeyW( HKEY_LOCAL_MACHINE, %s, %s ) Error.") % key % path));
                        break;
                    }
                }
                else{
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, result, boost::str(boost::wformat(L"RegOpenKeyExW(HKEY_LOCAL_MACHINE, %s) Error.") % key));
                    break;
                }
            }
            else{
                RegCloseKey(hKey);
            }
            count++;
        }
    }
    return NULL;
}

reg_hive_edit::reg_hive_edit(boost::filesystem::path dir){

    macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true);
    macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true);

    if (boost::filesystem::exists(dir / L"system")){
        hive_file::ptr p = hive_file::load_key(L"system", dir / L"system");
        if (p){
            _hives[L"system"] = p;
            // Get Default Control Set;
            DWORD       dwNum = 0, cbData = sizeof(DWORD);
            DWORD       dwType = REG_DWORD;
            LONG        lStatus = ERROR_SUCCESS;
            HKEY        hKey;
            std::wstring offline_select = boost::str(boost::wformat(L"%s\\Select") % p->key());
            if (ERROR_SUCCESS == (lStatus = open_key(HKEY_LOCAL_MACHINE, (LPCTSTR)offline_select.c_str(), 0, KEY_READ, &hKey))){
                lStatus = query_value(hKey, _T("Default"), NULL, &dwType, (LPBYTE)&dwNum, &cbData);
                close_key(hKey);
            }
            if (!dwNum)
                SetLastError(lStatus);
            else{
                _default_control_set = boost::str(boost::wformat(L"ControlSet%03d") % dwNum);
            }
        }
    }

    if (boost::filesystem::exists(dir / "software")){
        hive_file::ptr p = hive_file::load_key(L"software", dir / L"software");
        if (p) _hives[L"software"] = p;
    }

    if (macho::windows::environment::is_running_as_local_system()){

        if (boost::filesystem::exists(dir / "security")){
            hive_file::ptr p = hive_file::load_key(L"security", dir / L"security");
            if (p) _hives[L"security"] = p;
        }
        if (boost::filesystem::exists(dir / "sam")){
            hive_file::ptr p = hive_file::load_key(L"sam", dir / L"sam");
            if (p) _hives[L"sam"] = p;
        }
    }

    if (boost::filesystem::exists(dir / "bcd")){
        hive_file::ptr p = hive_file::load_key(L"bcd", dir / L"bcd");
        if (p) _hives[L"bcd"] = p;
    }
}

reg_hive_edit::~reg_hive_edit(){
    _hives.clear();
    //macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, false);
    //macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, false);
}

std::wstring reg_hive_edit::map_offline_key_path(HKEY hKey, std::wstring szSubKey){

    string_array_w arr_sub_keys;
    if (HKEY_LOCAL_MACHINE == hKey && szSubKey.length()){
        if (szSubKey[0] == _T('\\'))
            szSubKey = szSubKey.substr(1, szSubKey.length() - 1);
        arr_sub_keys = stringutils::tokenize(szSubKey, _T("\\"), 3, false);
        if (arr_sub_keys.size() > 1){
            if (0 == _tcsicmp(L"system", arr_sub_keys[0].c_str())){
                if (!_hives.count(L"system")){
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, L"System hive file is not loaded.");
                }
                else{
                    arr_sub_keys[0] = _hives[L"system"]->key();
                    if (0 == _tcsicmp(L"CurrentControlSet", arr_sub_keys[1].c_str()) && _default_control_set.length()){
                        arr_sub_keys[1] = _default_control_set;
                    }
                }
            }
            if (0 == _tcsicmp(L"software", arr_sub_keys[0].c_str())){
                if (!_hives.count(L"software")){
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, L"Software hive file is not loaded.");
                }
                else{
                    arr_sub_keys[0] = _hives[L"software"]->key();
                }
            }
            if (0 == _tcsicmp(L"security", arr_sub_keys[0].c_str())){
                if (!_hives.count(L"security")){
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, L"Security hive file is not loaded.");
                }
                else{
                    arr_sub_keys[0] = _hives[L"software"]->key();
                }
            }
            if (0 == _tcsicmp(L"sam", arr_sub_keys[0].c_str())){
                if (!_hives.count(L"sam")){
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, L"Sam hive file is not loaded.");
                }
                else{
                    arr_sub_keys[0] = _hives[L"sam"]->key();
                }
            }
            if (0 == _tcsicmp(L"bcd", arr_sub_keys[0].c_str())){
                if (!_hives.count(L"bcd")){
                    BOOST_THROW_EXCEPTION_BASE(reg_hive_edit::exception, ERROR_FILE_NOT_FOUND, L"BCD hive file is not loaded.");
                }
                else{
                    arr_sub_keys[0] = _hives[L"bcd"]->key();
                }
            }
            szSubKey.clear();
            for (size_t i = 0; i < arr_sub_keys.size(); ++i){
                if (i > 0)
                    szSubKey.append(_T("\\"));
                szSubKey.append(arr_sub_keys[i]);
            }
        }
    }
    return szSubKey;
}