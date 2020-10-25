#include <macho.h>
#include "common_service_handler.h"
#include "common_connection_service_handler.h"
#include <VersionHelpers.h>
#include <codecvt>
#include "..\vcbt\vcbt\journal.h"
#include "json_spirit.h"
#include "webdav.h"
#include "aws_s3.h"

using namespace macho::windows;
using namespace macho;
using namespace json_spirit;

std::string common_connection_service_handler::get_machine_id(){
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)) && reg[_T("Machine")].exists()){
        return reg[_T("Machine")].string();
    }
    return "";
}

void common_connection_service_handler::initialize(){
    macho::windows::auto_lock lock(cs);
    connection_folder = macho::windows::environment::get_working_directory();
    connection_folder /= "connections";
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"ConnectionFolder"].exists() && reg[L"ConnectionFolder"].is_string()){
            std::wstring path = reg[L"ConnectionFolder"].wstring();
            if (path.length()){
                if (boost::filesystem::exists(path) && boost::filesystem::is_directory(path))
                    connection_folder = path;
                else if (boost::filesystem::create_directories(path))
                    connection_folder = path;
            }
        }
    }
    namespace fs = boost::filesystem;
    fs::directory_iterator end_iter;
    typedef std::multimap<std::time_t, fs::path> result_set_t;
    result_set_t result_set;
    if (fs::exists(connection_folder) && fs::is_directory(connection_folder)){
        for (fs::directory_iterator dir_iter(connection_folder); dir_iter != end_iter; ++dir_iter){
            if (fs::is_regular_file(dir_iter->status())){
                if (dir_iter->path().extension() == ".conn"){
                    try{
                        connection conn = load(dir_iter->path().string());
                        if (conn.id.length()){
                            connections_map[conn.id] = conn;
                        }
                    }
                    catch (...){
                        boost::filesystem::remove(dir_iter->path());
                    }
                }
            }
        }
    }
}

connection common_connection_service_handler::load_connection(mValue& conn){
    bool is_winpe = macho::windows::environment::is_winpe();
    connection _conn;
    _conn.id = find_value(conn.get_obj(), "id").get_str();
    _conn.type = (saasame::transport::connection_type::type)(find_value(conn.get_obj(), "type").get_int());
    mArray options = find_value(conn.get_obj(), "options").get_array();
    foreach(mValue &o, options){
        _conn.options[find_value(o.get_obj(), "key").get_str()] = find_value(o.get_obj(), "value").get_str();
    }

    _conn.checksum = find_value_bool(conn.get_obj(), "checksum");
    _conn.compressed = find_value_bool(conn.get_obj(), "compressed");
    _conn.encrypted = find_value_bool(conn.get_obj(), "encrypted");

    macho::bytes user, password, u1, u2;
    macho::bytes proxy_user, proxy_password, u3, u4;

    switch (_conn.type){
    case connection_type::LOCAL_FOLDER_EX:
        _conn.detail.remote.path = find_value_string(conn.get_obj(), "remote_path");
    case connection_type::LOCAL_FOLDER:
        _conn.detail.local.path = find_value_string(conn.get_obj(), "path");
        if (!_conn.options.count("folder"))
            _conn.options["folder"] = _conn.detail.local.path;
        break;
    case connection_type::WEBDAV_EX:
    case connection_type::S3_BUCKET_EX:
        _conn.detail.local.path = find_value_string(conn.get_obj(), "local_path");
    case connection_type::S3_BUCKET:
    case connection_type::WEBDAV_WITH_SSL:
        _conn.detail.remote.port = find_value_int32(conn.get_obj(), "port");
        _conn.detail.remote.proxy_host = find_value_string(conn.get_obj(), "proxy_host");
        _conn.detail.remote.proxy_port = find_value_int32(conn.get_obj(), "proxy_port");
        if (_conn.type == connection_type::S3_BUCKET || _conn.type == connection_type::S3_BUCKET_EX)
            _conn.detail.remote.s3_region = (saasame::transport::aws_region::type)find_value_int32(conn.get_obj(), "s3_region");
        if (is_winpe){
            std::string machine_id = get_machine_id();
            std::string u3 = find_value_string(conn.get_obj(), "u3");
            std::string u4 = find_value_string(conn.get_obj(), "u4");
            if (!u3.empty()){
                _conn.detail.remote.proxy_username = macho::xor_crypto::decrypt(u3, machine_id);
            }
            if (!u4.empty()){
                _conn.detail.remote.proxy_password = macho::xor_crypto::decrypt(u4, machine_id);
            }
        }
        else{
            proxy_user.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u3")));
            proxy_password.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u4")));
            u3 = macho::windows::protected_data::unprotect(proxy_user, true);
            u4 = macho::windows::protected_data::unprotect(proxy_password, true);
            if (u3.length() && u3.ptr()){
                _conn.detail.remote.proxy_username = std::string(reinterpret_cast<char const*>(u3.ptr()), u3.length());
            }
            else{
                _conn.detail.remote.proxy_username = "";
            }
            if (u4.length() && u4.ptr()){
                _conn.detail.remote.proxy_password = std::string(reinterpret_cast<char const*>(u4.ptr()), u4.length());
            }
            else{
                _conn.detail.remote.proxy_password = "";
            }
        }
        _conn.detail.remote.timeout = find_value_int32(conn.get_obj(), "timeout");

    case connection_type::NFS_FOLDER:
        _conn.detail.remote.path = find_value_string(conn.get_obj(), "path");
        if (is_winpe){
            std::string machine_id = get_machine_id();
            std::string u1 = find_value_string(conn.get_obj(), "u1");
            std::string u2 = find_value_string(conn.get_obj(), "u2");
            if (!u1.empty()){
                _conn.detail.remote.username = macho::xor_crypto::decrypt(u1, machine_id);
            }
            if (!u2.empty()){
                _conn.detail.remote.password = macho::xor_crypto::decrypt(u2, machine_id);
            }
        }
        else{
            user.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u1")));
            password.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u2")));
            u1 = macho::windows::protected_data::unprotect(user, true);
            u2 = macho::windows::protected_data::unprotect(password, true);
            if (u1.length() && u1.ptr()){
                _conn.detail.remote.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
            }
            else{
                _conn.detail.remote.username = "";
            }
            if (u2.length() && u2.ptr()){
                _conn.detail.remote.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
            }
            else{
                _conn.detail.remote.password = "";
            }
        }
        break;
    }
    return _conn;
}

mObject common_connection_service_handler::save_connection(const connection &conn){
    bool is_winpe = macho::windows::environment::is_winpe();
    mObject _conn;
    _conn["id"] = conn.id;
    _conn["type"] = (int)conn.type;
    macho::bytes user, password, u1, u2;
    macho::bytes proxy_user, proxy_password, u3, u4;
    switch (conn.type){
    case connection_type::LOCAL_FOLDER_EX:
        _conn["remote_path"] = conn.detail.remote.path;
    case connection_type::LOCAL_FOLDER:
        _conn["path"] = conn.detail.local.path;
        break;
    case connection_type::WEBDAV_EX:
    case connection_type::S3_BUCKET_EX:
        _conn["local_path"] = conn.detail.local.path;
    case connection_type::S3_BUCKET:
    case connection_type::WEBDAV_WITH_SSL:
        _conn["port"] = conn.detail.remote.port;
        _conn["proxy_host"] = conn.detail.remote.proxy_host;
        _conn["proxy_port"] = conn.detail.remote.proxy_port;
        if (conn.type == connection_type::S3_BUCKET || conn.type == connection_type::S3_BUCKET_EX)
            _conn["s3_region"] = (int)conn.detail.remote.s3_region;
        if (is_winpe){
            std::string machine_id = get_machine_id();
            _conn["u3"] = macho::xor_crypto::encrypt(conn.detail.remote.proxy_username, machine_id);
            _conn["u4"] = macho::xor_crypto::encrypt(conn.detail.remote.proxy_password, machine_id);
        }
        else{
            proxy_user.set((LPBYTE)conn.detail.remote.proxy_username.c_str(), conn.detail.remote.proxy_username.length());
            proxy_password.set((LPBYTE)conn.detail.remote.proxy_password.c_str(), conn.detail.remote.proxy_password.length());
            u3 = macho::windows::protected_data::protect(proxy_user, true);
            u4 = macho::windows::protected_data::protect(proxy_password, true);
            _conn["u3"] = macho::stringutils::convert_unicode_to_utf8(u3.get());
            _conn["u4"] = macho::stringutils::convert_unicode_to_utf8(u4.get());
        }
        _conn["timeout"] = conn.detail.remote.timeout;

    case connection_type::NFS_FOLDER:
        _conn["path"] = conn.detail.remote.path;
        if (is_winpe){
            std::string machine_id = get_machine_id();
            _conn["u1"] = macho::xor_crypto::encrypt(conn.detail.remote.username, machine_id);
            _conn["u2"] = macho::xor_crypto::encrypt(conn.detail.remote.password, machine_id);
        }
        else{
            user.set((LPBYTE)conn.detail.remote.username.c_str(), conn.detail.remote.username.length());
            password.set((LPBYTE)conn.detail.remote.password.c_str(), conn.detail.remote.password.length());
            u1 = macho::windows::protected_data::protect(user, true);
            u2 = macho::windows::protected_data::protect(password, true);
            _conn["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
            _conn["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
        }
        break;
    }
    std::string local_folder = conn.detail.local.path;
    mArray options;
    typedef std::map<std::string, std::string> string_map;
    foreach(const string_map::value_type &option, conn.options){
        mObject opt;
        opt["key"] = option.first;
        if (option.first == "folder")
            local_folder = option.second;
        opt["value"] = option.second;
        options.push_back(opt);
    }
    _conn["options"] = options;
    _conn["checksum"] = conn.checksum;
    _conn["compressed"] = conn.compressed;
    _conn["encrypted"] = conn.encrypted;
    return _conn;
}

void common_connection_service_handler::save(const connection& conn){
    try{
        boost::filesystem::create_directory(connection_folder);
        boost::filesystem::path _connection_file = connection_folder / boost::str(boost::format("%s.conn") % conn.id);
        mObject _conn = save_connection(conn);
        std::ofstream output(_connection_file.string(), std::ios::out | std::ios::trunc);
        std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
        output.imbue(loc);
        write(_conn, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        std::string local_folder = conn.detail.local.path;
        mArray options;
        typedef std::map<std::string, std::string> string_map;
        foreach(const string_map::value_type &option, conn.options){
            if (option.first == "folder")
                local_folder = option.second;
        }
        if (!local_folder.empty()){
            try{
                com_init com;
                wmi_object inputOpenStore;
                wmi_object outputOpenStore;
                DWORD dwReturn;
                wmi_services wmi_defender;
                HRESULT hr = wmi_defender.connect(L"Microsoft\\Windows\\Defender");
                hr = wmi_defender.get_input_parameters(L"MSFT_MpPreference", L"Add", inputOpenStore);
                string_table_w exclusion_path;
                exclusion_path.push_back(macho::stringutils::convert_utf8_to_unicode(local_folder));
                inputOpenStore.set_parameter(L"ExclusionPath", exclusion_path);
                inputOpenStore.set_parameter(L"Force", true);
                hr = wmi_defender.exec_method(L"MSFT_MpPreference", L"Add", inputOpenStore, outputOpenStore, dwReturn);
            }
            catch (...){
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output connection info.")));
    }
    catch (...){
    }
}

connection common_connection_service_handler::load(std::string path){
    connection _conn;
    if (boost::filesystem::exists(path)){
        std::ifstream is(path);
        mValue conn;
        read(is, conn);
        _conn = load_connection(conn);
    }
    return _conn;
}

bool common_connection_service_handler::verify(const connection& conn){
    bool result = false;
    std::map<std::string, std::string > options;
    switch (conn.type){  
    case saasame::transport::connection_type::LOCAL_FOLDER:{
        std::string folder;
        if (conn.options.count("folder")){
            folder = conn.options.at("folder");
        }
        else{
            folder = conn.detail.local.path;
        }
        result = boost::filesystem::exists(folder) && boost::filesystem::is_directory(folder);
    }
        break;
    case saasame::transport::connection_type::NFS_FOLDER:{
        macho::windows::wnet_connection::ptr wnet =
            macho::windows::wnet_connection::connect(
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.path),
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.username),
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.password), false);
        result = NULL != wnet;
    }
        break;
#ifndef IRM_TRANSPORTER
    case saasame::transport::connection_type::S3_BUCKET:
        if (conn.detail.remote.path.length())
            result = NULL != aws_s3::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.s3_region, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
        break;
#endif
    case saasame::transport::connection_type::WEBDAV_WITH_SSL:
        if (conn.detail.remote.path.length())
            result = NULL != webdav::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.port, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
        break;
    case saasame::transport::connection_type::WEBDAV_EX:
        if (result = boost::filesystem::exists(conn.detail.local.path) && boost::filesystem::is_directory(conn.detail.local.path)){
            if (conn.detail.remote.path.length())
                result = NULL != webdav::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.port, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
            else
                result = false;
        }
        break;
#ifndef IRM_TRANSPORTER
    case saasame::transport::connection_type::S3_BUCKET_EX:
        if (result = boost::filesystem::exists(conn.detail.local.path) && boost::filesystem::is_directory(conn.detail.local.path)){
            if (conn.detail.remote.path.length())
                result = NULL != aws_s3::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.s3_region, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
            else
                result = false;
        }
        break;
#endif
    case connection_type::LOCAL_FOLDER_EX:
        if (result = boost::filesystem::exists(conn.detail.remote.path) && boost::filesystem::is_directory(conn.detail.remote.path)){
            std::string folder;
            if (conn.options.count("folder")){
                folder = conn.options.at("folder");
            }
            else{
                folder = conn.detail.local.path;
            }
            result = boost::filesystem::exists(folder) && boost::filesystem::is_directory(folder);
        }
        break;
    }
    return result;
}

bool common_connection_service_handler::test_connection(const std::string& session_id, const connection& conn){
    VALIDATE;
    return verify(conn);
}

bool common_connection_service_handler::add_connection(const std::string& session_id, const connection& conn){
    VALIDATE;
    macho::windows::auto_lock lock(cs);
    if ((!connections_map.count(conn.id)) && verify(conn)){
        save(conn);
        connections_map[conn.id] = conn;
        return true;
    }
    return false;
}

bool common_connection_service_handler::remove_connection(const std::string& session_id, const std::string& connection_id){
    macho::windows::auto_lock lock(cs);
    if (connections_map.count(connection_id)){
        connections_map.erase(connection_id);
        boost::filesystem::path _connection_file = connection_folder / boost::str(boost::format("%s.conn") % connection_id);
        SetFileAttributesW(_connection_file.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
        boost::filesystem::remove(_connection_file);
    }
    return true;
}

bool common_connection_service_handler::modify_connection(const std::string& session_id, const connection& conn){
    macho::windows::auto_lock lock(cs);
    if (connections_map.count(conn.id) && verify(conn)){
        save(conn);
        connections_map[conn.id] = conn;
        return true;
    }
    return false;
}

void common_connection_service_handler::enumerate_connections(std::vector<connection> & _return, const std::string& session_id){
    macho::windows::auto_lock lock(cs);
    foreach(connections_map_type::value_type &c, connections_map){
        _return.push_back(c.second);
    }
}

void common_connection_service_handler::get_connection(connection& _return, const std::string& session_id, const std::string& connection_id) {
    macho::windows::auto_lock lock(cs);
    if (connections_map.count(connection_id))
    {
        _return = connections_map[connection_id];
        _return.__set_options(_return.options);
        _return.detail.local.__set_path(_return.detail.local.path);
        _return.detail.remote.__set_path(_return.detail.remote.path);
        _return.detail.remote.__set_username(_return.detail.remote.username);
        _return.detail.remote.__set_password(_return.detail.remote.password);
        _return.detail.remote.__set_port(_return.detail.remote.port);
        _return.detail.remote.__set_proxy_host(_return.detail.remote.proxy_host);
        _return.detail.remote.__set_proxy_username(_return.detail.remote.proxy_username);
        _return.detail.remote.__set_proxy_password(_return.detail.remote.proxy_password);
        _return.detail.remote.__set_proxy_port(_return.detail.remote.proxy_port);
        _return.detail.remote.__set_s3_region(_return.detail.remote.s3_region);
        _return.detail.remote.__set_timeout(_return.detail.remote.timeout);

        _return.detail.__set_remote(_return.detail.remote);
        _return.__set_detail(_return.detail);
    }
    else{
        invalid_operation err;
        err.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err.why = "Can't find the connection";
        throw err;
    }
}

int64_t common_connection_service_handler::get_available_bytes(const std::string& session_id, const std::string& connection_id){
    macho::windows::auto_lock lock(cs);
    int64_t available_bytes = -1;
    ULARGE_INTEGER FreeBytesAvailable;

    if (connections_map.count(connection_id))
    {
        connection conn = connections_map[connection_id];
        switch (conn.type)
        {
        case saasame::transport::connection_type::LOCAL_FOLDER:
        case saasame::transport::connection_type::WEBDAV_EX:
        case saasame::transport::connection_type::S3_BUCKET_EX:
        case connection_type::LOCAL_FOLDER_EX:
            available_bytes = 0;
            if (GetDiskFreeSpaceExA(conn.detail.local.path.c_str(), &FreeBytesAvailable, NULL, NULL)){
                available_bytes = FreeBytesAvailable.QuadPart;
            }            
        }
    }
    else{
        invalid_operation err;
        err.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err.why = "Can't find the connection";
        throw err;
    }
    return available_bytes;
}