#include "stdafx.h"
#include "transport_service_handler.h"
#include "common_service.h"
#include "service_proxy.h"
#include "launcher_service.h"
#include "physical_packer_service.h"
#include <codecvt>
#include "service_op.h"
#include "db_op.h"
#include "license_db.h"
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "resource.h"
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include "common\license.hpp"
#include "..\notice\http_client.h"
#include "..\gen-cpp\TCurlClient.h"
#include "..\gen-cpp\mgmt_op.h"

int padding = RSA_PKCS1_PADDING;
#define JOB_EXTENSION L".tk"
#define LIC_DEBUG     0

class TReverseSocketClient : public TTransport{
public:

    TReverseSocketClient(reverse_transport_handler& handler, std::string addr) :_handler(handler), _addr(addr){}

    virtual void open(){}

    virtual bool isOpen(){
        return _handler.is_session_alive(_addr);
    }

    virtual bool peek() {
        return true;
    }

    virtual void close(){}

    uint32_t read(uint8_t* buf, uint32_t len){
        uint32_t nread = _read_buffer.read(buf, len);
        return nread;
    }

    virtual uint32_t read_virt(uint8_t*  buf, uint32_t len){
        return read(buf, len);
    }

    virtual uint32_t readEnd(){
        _read_buffer.resetBuffer();
        return 0;
    }

    void write(const uint8_t* buf, uint32_t len){
        _write_buffer.write(buf, len);
    }

    virtual void     write_virt(const uint8_t*  buf, uint32_t len){
        write(buf, len);
    }

    virtual void     flush(){
        if (_write_buffer.available_read()){
            std::string response = _handler.flush(_addr, _write_buffer.getBufferAsString());
            _write_buffer.resetBuffer();
            _read_buffer.write((const uint8_t*)response.c_str(), response.length());
        }
    }

private:
    reverse_transport_handler& _handler;
    std::string                _addr;
    TMemoryBuffer              _write_buffer;
    TMemoryBuffer              _read_buffer;
};

template <class Type>
class thrift_connect_ex : virtual public thrift_connect<Type>{
public:
    thrift_connect_ex(reverse_transport_handler& handler, int port, std::string addr) : _handler(handler), thrift_connect(port, addr){
    }

    virtual bool open(){
        if (is_proxy_mode()){
            _reverse_socket = std::shared_ptr<TReverseSocketClient>(new TReverseSocketClient(_handler, _addr));
            _transport = std::shared_ptr<TTransport>(new TBufferedTransport(_reverse_socket));
            _protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));
            _client = std::shared_ptr<Type>(new Type(_protocol));
            return true;
        }
        else{
            return thrift_connect<Type>::open();
        }
    }
    virtual void close(){
        thrift_connect<Type>::close();
        _reverse_socket = NULL;
    }
protected:
    bool is_proxy_mode(){
        try{
            guid_ g = _addr;
            return true;
        }
        catch (...){
        }
        return false;
    }
    std::shared_ptr<TReverseSocketClient>    _reverse_socket;
    reverse_transport_handler&               _handler;
};

reverse_transport_handler::reverse_transport_handler() : _is_Open(true), _long_polling_interval(30), _id(0){
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"LongPollingInterval"].exists() && reg[L"LongPollingInterval"].is_dword()){
            DWORD value = reg[L"LongPollingInterval"];
            if (value > 0 && value <= 600){
                _long_polling_interval = value;
            }
        }
    }
}

void reverse_transport_handler::receive(transport_message& _return, const std::string& session, const std::string& addr, const std::string& name) {
    if (!is_valid_session(session, addr))
        throw invalid_session();
    boost::posix_time::ptime in = boost::posix_time::second_clock::universal_time();
    session_object::ptr      session_obj;
    {
        boost::unique_lock<boost::mutex> lock(_cs);
        if (_sessions.count(addr))
            session_obj = _sessions[addr];
        else
            session_obj = _sessions[addr] = boost::shared_ptr<session_object>(new session_object());
    }
    if (session_obj){
        boost::unique_lock<boost::mutex> lck(session_obj->cs);
        int session_timeout = 300;
        session_obj->timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) + boost::posix_time::seconds(session_timeout);
        session_obj->name = name;
        bool commands_is_not_empty = false;
        while ((session_obj->commands.size() || (commands_is_not_empty = session_obj->cond.timed_wait(lck, boost::posix_time::seconds(_long_polling_interval)))) && _is_Open){
            session_obj->timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) + boost::posix_time::seconds(session_timeout);
            if (session_obj->commands.size()){
                transport_message command = session_obj->commands.front();
                session_obj->commands.pop_front();
                if (session_obj->responses[command.id]){
                    _return = command;
                    return;
                }
            }
        }
        if ((!commands_is_not_empty) || (!_is_Open)){
            session_obj->timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) + boost::posix_time::seconds(session_timeout);
            throw command_empty();
        }        
        /*
        if (session_obj->commands.size()){
            _return = session_obj->commands.front();
            session_obj->commands.pop_front();
            return;
        }
        else if (session_obj->cond.timed_wait(lck, boost::posix_time::seconds(_long_polling_interval)) && _is_Open){
            session_obj->timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) + boost::posix_time::seconds(session_timeout);
            if (session_obj->commands.size()){
                _return = session_obj->commands.front();
                session_obj->commands.pop_front();
                return;
            }
        }
        else{
            session_obj->timeout = boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) + boost::posix_time::seconds(session_timeout);
            throw command_empty();
        }*/
    }
    if (!_is_Open)
        throw TTransportException(TTransportException::UNKNOWN, "Server Closing.");
}

bool reverse_transport_handler::response(const std::string& session, const std::string& addr, const transport_message& response) {
    if (!is_valid_session(session, addr))
        throw invalid_session();
    session_object::ptr      session_obj;
    {
        boost::unique_lock<boost::mutex> lock(_cs);
        if (_sessions.count(addr))
            session_obj = _sessions[addr];
        else
            session_obj = _sessions[addr] = boost::shared_ptr<session_object>(new session_object());
    }
    if (session_obj){
        session_object::response::ptr   resp;
        {
            boost::unique_lock<boost::mutex> lock(session_obj->cs);
            if (session_obj->responses.count(response.id))
                resp = session_obj->responses[response.id];
        }
        if (resp){
            boost::unique_lock<boost::mutex> lck(resp->cs);
            resp->data = response.message;
            resp->cond.notify_one();
            return true;
        }
    }
    return false;
}

void reverse_transport_handler::release(){
    boost::unique_lock<boost::mutex> lock(_cs);
    session_object::map::iterator session = _sessions.begin();
    while (session != _sessions.end()) {
        if (session->second->timeout < boost::posix_time::ptime(boost::posix_time::second_clock::universal_time())) { 
            if (session->second->commands.size()){
                transport_message command = session->second->commands.front();
                session->second->commands.pop_front();
                LOG(LOG_LEVEL_RECORD, L"command id = %I64u", command.id);
            }
            if (session->second->responses.size()){
                foreach(session_object::response::map::value_type& v, session->second->responses){
                    LOG(LOG_LEVEL_RECORD, L"response id = %I64u", v.first);
                }
            }
            session = _sessions.erase(session);
        }
        else {
            ++session;
        }
    }
}

bool reverse_transport_handler::is_session_alive(std::string addr){
    boost::unique_lock<boost::mutex> lock(_cs);
    return _sessions.count(addr) ? _sessions[addr]->timeout > boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()) : false;
}

std::string reverse_transport_handler::flush(const std::string& addr, const std::string& command, int timeout){
    boost::posix_time::ptime in = boost::posix_time::second_clock::universal_time();
    uint64_t                        id = 0;
    session_object::ptr             session_obj;
    session_object::response::ptr   response;
    {
        boost::unique_lock<boost::mutex> lock(_cs);
        if (_sessions.count(addr))
            session_obj = _sessions[addr];
        id = (++_id);
    }
    if (_is_Open){
        if (session_obj && session_obj->timeout > boost::posix_time::ptime(boost::posix_time::second_clock::universal_time())){
            boost::unique_lock<boost::mutex> lock(session_obj->cs);
            transport_message msg;
            msg.id = id;
            LOG(LOG_LEVEL_RECORD, L"Message Id = %I64u (%s)", msg.id, macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            msg.message = command;
            session_obj->commands.push_back(msg);
            response = session_obj->responses[id] = session_object::response::ptr(new session_object::response());
            session_obj->cond.notify_one();
        }
        else{
            if (session_obj)
                throw TTransportException(TTransportException::NOT_OPEN, "Failed to open TReverseTransport : session timeout");
            else
                throw TTransportException(TTransportException::NOT_OPEN, "Failed to open TReverseTransport : cannot connect.");
        }
    }
    if (session_obj && response){
        boost::unique_lock<boost::mutex> lck(response->cs);
        if (response->cond.timed_wait(lck, boost::posix_time::seconds(timeout)) && _is_Open){
            boost::unique_lock<boost::mutex> lock(session_obj->cs);
            session_obj->responses.erase(id);
            return response->data;
        }
        else{
            boost::unique_lock<boost::mutex> lock(session_obj->cs);
            session_obj->responses.erase(id);
            throw TTransportException(TTransportException::TIMED_OUT, "Time out.");
        }
    }
    if (!_is_Open)
        throw TTransportException(TTransportException::UNKNOWN, "Server Closing.");
    return "";
}

void reverse_transport_handler::ping(service_info& _return){
    _return.id = saasame::transport::g_saasame_constants.TRANSPORTER_SERVICE;
    _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
}

bool reverse_transport_handler::is_valid_session(const std::string& session, const std::string& addr){
    //return true;
    return session == encode64(macho::sha1::hmac_sha1(addr, _machine_id));
}

void reverse_transport_handler::generate_session(std::string& _return, const std::string& addr){
    _return = encode64(macho::sha1::hmac_sha1(addr, _machine_id));
    session_object::ptr      session_obj;
    {
        boost::unique_lock<boost::mutex> lock(_cs);
        if (_sessions.count(addr))
            session_obj = _sessions[addr];
        else
            session_obj = _sessions[addr] = boost::shared_ptr<session_object>(new session_object());
    }
    if (session_obj){
        boost::unique_lock<boost::mutex> lock(session_obj->cs);
        session_obj->commands.clear();
        session_obj->responses.clear();       
    }
}

std::string reverse_transport_handler::encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

void transport_service_handler::generate_session(std::string& _return, const std::string& addr){
    _reverse.generate_session(_return, addr);
}

void transport_service_handler::initialize(){
    //System Volume Information
    macho::windows::registry reg(macho::windows::REGISTRY_CREATE);
    boost::filesystem::path root_path = boost::str(boost::wformat(L"%1%:\\System Volume Information") % macho::windows::environment::get_windows_directory()[0]);
    physical_machine_info machine_info;
    get_host_detail(machine_info, "", machine_detail_filter::type::FULL);
    _id = stringutils::toupper(machine_info.machine_id);
    _reverse.set_machine_id(_id);
    boost::filesystem::path db_path = root_path / stringutils::tolower(boost::str(boost::format("{%1%}") % machine_info.machine_id));
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"PurgeDB"].exists() && (DWORD)reg[L"PurgeDB"] > 0){
            if (boost::filesystem::exists(db_path)){
                boost::filesystem::remove(db_path);
            }
            reg[L"PurgeDB"].delete_value();
        }
    }
    if (boost::filesystem::exists(db_path)){
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            std::string mac;
            if (reg[L"MAC"].exists()){
                mac = stringutils::tolower(reg[L"MAC"].string());
            }
            else if (reg[L"DigitalKey"].exists()){
                macho::bytes digital_key = reg[_T("DigitalKey")];
                if (digital_key.length()){
                    macho::bytes u1 = macho::windows::protected_data::unprotect(digital_key, true);
                    if (u1.length())
                        mac = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                }
            }
            if (!mac.empty()){
                foreach(saasame::transport::network_info net, machine_info.network_infos){
                    if (mac == stringutils::tolower(net.mac_address)){
                        std::string ps = stringutils::toupper(machine_info.machine_id) + stringutils::tolower(net.mac_address);
                        _db = license_db::open(db_path.string(), ps);
                        if (_db){
                            _ps = ps;
                            _mac = stringutils::tolower(net.mac_address);
                            LOG(LOG_LEVEL_RECORD, L"Load license database by digital key.");
                            break;
                        }
                    }
                }
            }
        }
        if (NULL == _db){
            foreach(saasame::transport::network_info net, machine_info.network_infos){
                std::string ps = stringutils::toupper(machine_info.machine_id) + stringutils::tolower(net.mac_address);
                _db = license_db::open(db_path.string(), ps);
                if (_db){
                    _ps = ps;
                    _mac = stringutils::tolower(net.mac_address);
                    macho::windows::registry reg;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (!reg[_T("DigitalKey")].exists()){
                            macho::bytes digital_key;
                            std::string mac = (std::string)_mac;
                            digital_key.set((LPBYTE)mac.c_str(), mac.length());
                            macho::bytes u1 = macho::windows::protected_data::protect(digital_key, true);
                            reg[_T("DigitalKey")] = u1;
                        }
                    }
                    break;
                }
            }
            if (NULL == _db){
                for (int i = 8; i >= 0; i--){
                    boost::filesystem::path new_db_bk_path = root_path / stringutils::tolower(boost::str(boost::format("{%1%}BAK.%d") % machine_info.machine_id % (i + 1)));
                    boost::filesystem::path old_db_bk_path = root_path / stringutils::tolower(boost::str(boost::format("{%1%}BAK.%d") % machine_info.machine_id%i));
                    if (boost::filesystem::exists(new_db_bk_path))
                        boost::filesystem::remove(new_db_bk_path);
                    if (boost::filesystem::exists(old_db_bk_path))
                        boost::filesystem::rename(old_db_bk_path, new_db_bk_path);
                    if (0 == i)
                        boost::filesystem::rename(db_path, old_db_bk_path);
                }
            }
        }
    }
    if (NULL == _db && machine_info.network_infos.size()){
        foreach(saasame::transport::network_info net, machine_info.network_infos){
            _ps = stringutils::toupper(machine_info.machine_id) + stringutils::tolower(net.mac_address);
            _mac = stringutils::tolower(net.mac_address);
            break;
        }
        _db = license_db::open(db_path.string(), _ps);
        if (_db){
            macho::windows::registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                macho::bytes digital_key;
                std::string mac = (std::string)_mac;
                digital_key.set((LPBYTE)mac.c_str(), mac.length());
                macho::bytes u1 = macho::windows::protected_data::protect(digital_key, true);
                reg[_T("DigitalKey")] = u1;
            }
            RSA_ptr rsa(RSA_new(), ::RSA_free);
            BN_ptr bn(BN_new(), ::BN_free);
            BIO_ptr priv(BIO_new(BIO_s_mem()), ::BIO_free);
            BN_set_word(bn.get(), RSA_F4);
            RSA_generate_key_ex(rsa.get(), 2048, bn.get(), NULL);
            PEM_write_bio_RSAPrivateKey(priv.get(), rsa.get(), NULL, NULL, 0, NULL, NULL);
            int pri_len = BIO_pending(priv.get());
            std::auto_ptr<char> priv_key(new char[pri_len + 1]);
            BIO_read(priv.get(), priv_key.get(), pri_len);
            priv_key.get()[pri_len] = '\0';
            private_key k;
            k.key = priv_key.get();
            k.comment = _id;
            _db->put_private_key(k);
        }
    }
    else if (_db){
        private_key::ptr _k = _db->get_private_key();
        if (NULL == _k){
            RSA_ptr rsa(RSA_new(), ::RSA_free);
            BN_ptr bn(BN_new(), ::BN_free);
            BIO_ptr priv(BIO_new(BIO_s_mem()), ::BIO_free);
            BN_set_word(bn.get(), RSA_F4);
            RSA_generate_key_ex(rsa.get(), 2048, bn.get(), NULL);
            PEM_write_bio_RSAPrivateKey(priv.get(), rsa.get(), NULL, NULL, 0, NULL, NULL);
            int pri_len = BIO_pending(priv.get());
            std::auto_ptr<char> priv_key(new char[pri_len + 1]);
            BIO_read(priv.get(), priv_key.get(), pri_len);
            priv_key.get()[pri_len] = '\0';
            private_key k;
            k.key = priv_key.get();
            k.comment = _id;
            _db->put_private_key(k);
        }
    }
    if (!_db){
        LOG(LOG_LEVEL_ERROR, L"Invalid license configuration");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL;
        error.why = "Invalid license configuration";
        throw error;
    }
}

std::string transport_service_handler::decode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
    });
}

std::string transport_service_handler::encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

void transport_service_handler::ping(service_info& _return){
    _return.id = saasame::transport::g_saasame_constants.TRANSPORTER_SERVICE;
    _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
}

void transport_service_handler::_get_package_info(std::string& _return, const std::string& email, const std::string& name, const std::string& key){
    macho::windows::auto_lock _lock(_cs);
    macho::LIC_INFO_V1 _lic;
    memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
    if (key.length() == 25 && macho::license::unpack_keycode(key, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
        private_key::ptr k = _db->get_private_key();
        saasame::transport::license::ptr lic = _db->get_license(key);
        if (lic == NULL &&_lic.year && _lic.mon && _lic.day){
            long exp = ((2000 + _lic.year) * 12) + _lic.mon + 1;
            long year = ((exp - 1) / 12);
            long mon = ((exp - 1) % 12) + 1;
            if (boost::posix_time::ptime(boost::gregorian::date(year, mon, 1)) < boost::posix_time::second_clock::universal_time()){
                LOG(LOG_LEVEL_RECORD, L"Expired Key: %s", macho::stringutils::convert_ansi_to_unicode(key).c_str());
#ifndef _DEBUG
                saasame::transport::invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE_KEY;
                error.format = "Expired Key '%1%'.";
                error.arguments.push_back(key);
                error.why = boost::str(boost::format(error.format) % key);
                throw error;
#endif
            }
        }
        if (k && (NULL == lic || lic->lic.empty())){
            BIO_ptr priv(BIO_new_mem_buf(k->key.c_str(), -1), ::BIO_free);
            RSA_ptr rsa(PEM_read_bio_RSAPrivateKey(priv.get(), NULL, NULL, NULL), ::RSA_free);
            BIO_ptr pub(BIO_new(BIO_s_mem()), ::BIO_free);
            PEM_write_bio_RSA_PUBKEY(pub.get(), rsa.get());
            int pub_len = BIO_pending(pub.get());
            std::auto_ptr<char> pub_key(new char[pub_len + 1]);
            BIO_read(pub.get(), pub_key.get(), pub_len);
            pub_key.get()[pub_len] = '\0';
            physical_machine_info machine_info;
            get_host_detail(machine_info, "", machine_detail_filter::type::FULL);
            using boost::property_tree::ptree;
            using boost::property_tree::basic_ptree;
            ptree root;
            root.put<std::string>("email", email);
            root.put<std::string>("name", name);
            std::string time = boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time());
            if (lic && !lic->comment.empty())
                time = lic->comment;
            root.put<std::string>("time", time);
            root.put<std::string>("key", key);
            root.put<std::string>("machine_id", machine_info.machine_id);
            root.put<std::string>("client_id", machine_info.client_id);
            root.put<std::string>("os_name", machine_info.os_name);
            root.put<std::string>("architecture", machine_info.architecture);
            root.put<std::string>("manufacturer", machine_info.manufacturer);
            root.put<std::string>("pub_key", std::string(reinterpret_cast<char const*>(pub_key.get()), pub_len));
            root.put<std::string>("ps", (std::string)_ps);
            ptree mac_node;
            foreach(saasame::transport::network_info net, machine_info.network_infos){
                ptree mac;
                mac.put("", net.mac_address);
                mac_node.push_back(std::make_pair("", mac));
            }
            root.add_child("mac", mac_node);
            std::stringstream result;
            boost::property_tree::write_json(result, root);
            std::string public_key = extract_file_resource("FILE", IDR_LICENSE);
            _return = public_encrypt(result.str(), public_key);
#if _DEBUG
            LOG(LOG_LEVEL_RECORD, L"Package:\n%s", macho::stringutils::convert_ansi_to_unicode(_return).c_str());
#endif           
            if (!_return.empty()){
                if (NULL == lic){
                    lic = saasame::transport::license::ptr(new saasame::transport::license());
                    if (lic){
                        lic->key = key;
                        lic->name = name;
                        lic->email = email;
                        lic->active = macho::xor_crypto::encrypt2(key, time, _ps);
                        lic->comment = time;
                        if (!_db->insert_license(*lic.get())){
                            _return = "";
                        }
                    }
                }
                else{
                    lic->name = name;
                    lic->email = email;
                    lic->active = macho::xor_crypto::encrypt2(key, time, _ps);
                    if (!_db->update_license(*lic.get())){
                        _return = "";
                    }
                }
            }
        }
        else if (NULL == k){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL;
            error.why = "Invalid license configuration";
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
            throw error;
        }
        else if (lic){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE_KEY;
            error.format = "Key '%1%' already activated.";
            error.arguments.push_back(key);
            error.why = boost::str(boost::format(error.format) % key);
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
            throw error;
        }
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE_KEY;
        error.format = "Invalid Key '%1%'.";
        error.arguments.push_back(key);
        error.why = boost::str(boost::format(error.format) % key);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
        throw error;
    }
}

bool transport_service_handler::active_license(const std::string& email, const std::string& user, const std::string& key){
    std::string package;
    macho::windows::auto_lock lck(_cs);
    std::string _key = key;
    _key = stringutils::remove_begining_whitespaces(stringutils::erase_trailing_whitespaces(stringutils::toupper(_key)));
    _get_package_info(package, email, user, _key);
    if (!package.empty()){
        using boost::property_tree::ptree;
        using boost::property_tree::basic_ptree;
        ptree post_data;
        post_data.put<std::string>("package", package);
        std::stringstream request;
        boost::property_tree::write_json(request, post_data);
        std::string url = "https://authz.saasame.com/api/active";
        http_client client(url.find("https://") != std::string::npos);
        long rep = 0;
        std::string in = request.str();
        std::string out,header;
        std::vector<std::string> headers;
        headers.push_back("Accept: application/json; charset=UTF-8");
        headers.push_back("Content-Type: application/json; charset=UTF-8");
        client.post(url, headers, in, rep, out, header, 4430);
        if (!out.empty()){
            std::string::size_type p = out.find_first_of("{");
            if (p != std::string::npos)
                out = out.substr(p, out.length() - p);
            ptree response;
            std::stringstream s;
            s << (out);
            boost::property_tree::read_json(s, response);
            std::string key = response.get<std::string>("Key", std::string());
            std::string result = response.get<std::string>("Result", std::string());
            std::string error_message = response.get<std::string>("Error", std::string());
            if (error_message.empty() || error_message == "null"){
                return _add_license(result, true, key);
            }
            else{
                saasame::transport::invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE_KEY;
                error.format = error_message;
                error.arguments.push_back(key);
                error.why = error_message;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
                throw error;
            }
        }
    }
    return false;
}

bool transport_service_handler::_add_license(const std::string& license, bool online, const std::string& key){
    macho::windows::auto_lock lck(_cs);
    bool activated = false;
    if ((!license.empty())){
        private_key::ptr k = _db->get_private_key();
        if (k){
            std::string license_package = stringutils::remove_begining_whitespaces(stringutils::erase_trailing_whitespaces(license));
            std::string decrypted = private_decrypt(license_package, k->key);
            if (!decrypted.empty()){
                // Short alias for this namespace
                namespace pt = boost::property_tree;
                // Create a root
                pt::ptree root;
                std::stringstream s;
                s << (decrypted);
                pt::read_json(s, root);
                std::string time = root.get<std::string>("time");
                std::string active = root.get<std::string>("active");
                std::string _license = root.get<std::string>("license");
#if LIC_DEBUG
                LOG(LOG_LEVEL_RECORD, L"Key : %s, Active : %s, License : %s, Time : %s ",
                    macho::stringutils::convert_ansi_to_unicode(key).c_str(),
                    macho::stringutils::convert_ansi_to_unicode(active).c_str(),
                    macho::stringutils::convert_ansi_to_unicode(_license).c_str(),
                    macho::stringutils::convert_ansi_to_unicode(time).c_str());
#endif
                if (boost::posix_time::time_from_string(active).date() > (boost::posix_time::second_clock::universal_time().date())){
                    saasame::transport::invalid_operation error;
                    error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE;
                    error.format = "License could not be activated, please make sure that your time zone is set correctly on your machine.";
                    error.why = error.format;
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(error.why).c_str());
                    throw error;
                }

                if (online){    
                    saasame::transport::license::ptr lic = _db->get_license(key);
                    if (NULL == lic){
                        lic = saasame::transport::license::ptr(new saasame::transport::license());
                        if (lic){
                            lic->key = key;
                            lic->lic = _license;
                            lic->active = macho::xor_crypto::encrypt(active, _ps);
                            activated = _db->insert_license(*lic.get());
                        }
                    }
                    else{
                        lic->lic = _license;
                        lic->active = macho::xor_crypto::encrypt(active, _ps);
                        activated = _db->update_license(*lic.get());
                    }
                }
                else{
                    if (key.empty()){
                        saasame::transport::license::vtr lics = _db->get_licenses();
                        foreach(saasame::transport::license::ptr &lic, lics){
                            if (lic->lic.empty() && !lic->active.empty() && !lic->key.empty()){
                                if (lic->key == macho::xor_crypto::decrypt2(lic->active, time, _ps)){
                                    lic->lic = _license;
                                    lic->active = macho::xor_crypto::encrypt(active, _ps);
                                    if (activated = _db->update_license(*lic.get()))
                                        break;
                                }
                            }
                        }
                    }
                    else{
                        saasame::transport::license::ptr lic = _db->get_license(key);
                        if (lic){
                            if (lic->lic.empty() && !lic->active.empty() && !lic->key.empty()){
                                if (lic->key == macho::xor_crypto::decrypt2(lic->active, time, _ps)){
                                    lic->lic = _license;
                                    lic->active = macho::xor_crypto::encrypt(active, _ps);
                                    activated = _db->update_license(*lic.get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return activated;
}

void transport_service_handler::get_licenses(license_infos& _return){
    macho::windows::auto_lock lck(_cs);
    if (sync_db()){
        saasame::transport::license::vtr _lics = _db->get_licenses();
#if LIC_DEBUG
        foreach(saasame::transport::license::ptr lic, _lics){
            LOG(LOG_LEVEL_RECORD, L"Key : %s", macho::stringutils::convert_utf8_to_unicode(lic->key).c_str());
            LOG(LOG_LEVEL_RECORD, L"LIC : %s", macho::stringutils::convert_utf8_to_unicode(lic->lic).c_str());
            LOG(LOG_LEVEL_RECORD, L"Status : %s", macho::stringutils::convert_utf8_to_unicode(lic->status).c_str());
            LOG(LOG_LEVEL_RECORD, L"Comment : %s", macho::stringutils::convert_utf8_to_unicode(lic->comment).c_str());
            if (!lic->lic.empty()){
                std::string activated = macho::xor_crypto::decrypt(lic->active, _ps);
                LOG(LOG_LEVEL_RECORD, L"Activated : %s", macho::stringutils::convert_utf8_to_unicode(activated).c_str());
                LOG(LOG_LEVEL_RECORD, L"PS : %s", macho::stringutils::convert_utf8_to_unicode((std::string)_ps).c_str());
                macho::LIC_INFO_V1 _lic;
                memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
                if (macho::license::unpack_keycode2(lic->lic, (std::string)_ps + activated, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
                    LOG(LOG_LEVEL_RECORD, L"Count : %d", _lic.count);
                }
                //else if (lic->key == "4HHQVTSVADFMAHS3NW4U5KRXN"){
                //    lic->comment = lic->lic;
                //    lic->active.clear();
                //    lic->lic.clear();
                //    _db->update_license(*lic.get());
                //}
            }
        }
#endif
        saasame::transport::license::vtr activated_lics, non_activated_lics;
        std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(activated_lics), [](const saasame::transport::license::ptr& obj){return obj->lic.empty() || obj->status == "x"; });
        std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(non_activated_lics), [](const saasame::transport::license::ptr& obj){return !obj->lic.empty() || obj->status == "x"; });
        foreach(saasame::transport::license::ptr lic, activated_lics){
            license_info lic_info;
            lic_info.key = lic->key;
            lic_info.is_active = true;
            lic_info.name = lic->name;
            lic_info.email = lic->email;
            lic_info.status = lic->status;
            lic_info.activated = macho::xor_crypto::decrypt(lic->active, _ps);
            macho::LIC_INFO_V1 _lic;
            memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
            if (macho::license::unpack_keycode2(lic->lic, (std::string)_ps + lic_info.activated, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
                lic_info.count = _lic.count;
                if (_lic.year && _lic.mon && _lic.day)
                    lic_info.expired_date = boost::posix_time::to_simple_string(boost::posix_time::ptime(boost::gregorian::date(2000 + _lic.year, _lic.mon, _lic.day)));
                _return.licenses.push_back(lic_info);
            }
        }
        std::sort(_return.licenses.begin(), _return.licenses.end(), [](const license_info & l, const license_info & r) -> bool {
            if (l.expired_date.empty() && r.expired_date.empty()){
                return boost::posix_time::time_from_string(l.activated) < boost::posix_time::time_from_string(r.activated);
            }
            else if (!l.expired_date.empty() && !r.expired_date.empty()){
                return boost::posix_time::time_from_string(l.expired_date) < boost::posix_time::time_from_string(r.expired_date);
            }
            return l.expired_date.length() > r.expired_date.length();
        });
        bool can_be_run = false;
        workload_consumed_map_type consumed;
        workload_consumed_map_type consumeds = calculated_license_consumed(_return.licenses, std::string(), can_be_run, consumed, false);
        foreach(saasame::transport::license::ptr lic, non_activated_lics){
            license_info lic_info;
            lic_info.key = lic->key;
            lic_info.name = lic->name;
            lic_info.email = lic->email;
            lic_info.status = lic->status;
            if (!lic->lic.empty())
                lic_info.activated = macho::xor_crypto::decrypt(lic->active, _ps);
            lic_info.is_active = false;
            lic_info.status = "n";
            _return.licenses.push_back(lic_info);
        }
        uint32_t bill = 0;
        foreach(workload_consumed_map_type::value_type &wc, consumeds){
            for (auto it = wc.second.cbegin(); it != wc.second.cend()/* not hoisted */; /* no increment */){
                wc.second.erase(it++);
                bill++;
            }
        }
        if (bill){
            license_info bill_lic_info;
            bill_lic_info.key = "0000000000000000000000000";
            bill_lic_info.count = 0;
            bill_lic_info.consumed = bill;
            bill_lic_info.is_active = false;
            _return.licenses.push_back(bill_lic_info);
        }

        foreach(workload_consumed_map_type::value_type &wc, consumed){
            workload_history wh;
            wh.machine_id = wc.first;
            workload::vtr workloads = _db->get_workload(wc.first);
            foreach(workload::ptr w, workloads){
                wh.name = w->name;
                wh.type = w->type;
                break;
            }
            for (auto it = wc.second.cbegin(); it != wc.second.cend()/* not hoisted */; /* no increment */){
                wh.histories.push_back(it->first);
                wc.second.erase(it++);
            }
            wh.__set_histories(wh.histories);
            _return.histories.push_back(wh);
        }
    }
    _return.__set_licenses(_return.licenses);
    _return.__set_histories(_return.histories);
}

bool transport_service_handler::check_license_expiration(const int8_t days){
    bool will_be_expired = true;
    macho::windows::auto_lock lck(_cs);
    if (sync_db()){
        std::vector<license_info>  licenses;
        saasame::transport::license::vtr _lics = _db->get_licenses();
        saasame::transport::license::vtr activated_lics;
        std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(activated_lics), [](const saasame::transport::license::ptr& obj){return obj->lic.empty() || obj->status == "x"; });
        foreach(saasame::transport::license::ptr lic, activated_lics){
            license_info lic_info;
            lic_info.key = lic->key;
            lic_info.is_active = true;
            lic_info.name = lic->name;
            lic_info.email = lic->email;
            lic_info.status = lic->status;
            lic_info.activated = macho::xor_crypto::decrypt(lic->active, _ps);
            macho::LIC_INFO_V1 _lic;
            memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
            if (macho::license::unpack_keycode2(lic->lic, (std::string)_ps + lic_info.activated, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
                lic_info.count = _lic.count;
                if (_lic.year && _lic.mon && _lic.day)
                    lic_info.expired_date = boost::posix_time::to_simple_string(boost::posix_time::ptime(boost::gregorian::date(2000 + _lic.year, _lic.mon, _lic.day)));
                licenses.push_back(lic_info);
            }
        }
        std::sort(licenses.begin(), licenses.end(), [](const license_info & l, const license_info & r) -> bool {
            if (l.expired_date.empty() && r.expired_date.empty()){
                return boost::posix_time::time_from_string(l.activated) < boost::posix_time::time_from_string(r.activated);
            }
            else if (!l.expired_date.empty() && !r.expired_date.empty()){
                return boost::posix_time::time_from_string(l.expired_date) < boost::posix_time::time_from_string(r.expired_date);
            }
            return l.expired_date.length() > r.expired_date.length();
        });
        bool can_be_run = false;
        workload_consumed_map_type consumed;
        workload_consumed_map_type consumeds = calculated_license_consumed(licenses, std::string(), can_be_run, consumed, false, boost::posix_time::second_clock::universal_time() + boost::posix_time::hours(((uint32_t)days) * 24));
        will_be_expired = !can_be_run;
    }
    return will_be_expired;
}

bool transport_service_handler::remove_license(const std::string& key){
    macho::windows::auto_lock lck(_cs);
    saasame::transport::license::ptr lic = _db->get_license(key);
    if (lic){
        if (lic->lic.empty()){
            lic->status = "x";
            return _db->update_license(*lic.get());
        }
        else{
            return false;
        }
    }
    return true;
}

void transport_service_handler::query_package_info(std::string& _return, const std::string& key){
    macho::windows::auto_lock lck(_cs);
    std::string _key = key;
    _key = macho::stringutils::toupper(_key);
    saasame::transport::license::ptr lic = _db->get_license(_key);
    if (lic){
        _get_package_info(_return, lic->email, lic->name, lic->key);
    }
}

bool transport_service_handler::is_license_valid(const std::string& job_id){
    try{
        check_license(job_id, false);
    }
    catch (saasame::transport::invalid_operation ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
        return false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown error");
        return false;
    }
    return true;
}

bool transport_service_handler::is_license_valid_ex(const std::string& job_id, const bool is_recovery){
    try{
        check_license(job_id, is_recovery);
    }
    catch (saasame::transport::invalid_operation ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
        return false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown error");
        return false;
    }
    return true;
}

void transport_service_handler::ping_p(service_info& _return, const std::string& addr){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->ping(_return);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_host_detail_p(physical_machine_info& _return, const std::string& addr, const machine_detail_filter::type filter){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->get_host_detail(_return, "", filter);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_service_list_p(std::set<service_info> & _return, const std::string& addr){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->get_service_list(_return, "");
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::enumerate_disks_p(std::set<disk_info> & _return, const std::string& addr, const enumerate_disk_filter_style::type filter){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->enumerate_disks(_return, filter);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::verify_carrier_p(const std::string& addr, const std::string& carrier, const bool is_ssl){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Carrier : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str(), macho::stringutils::convert_utf8_to_unicode(carrier).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        return thrift.client()->verify_carrier(carrier, is_ssl);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::take_xray_p(std::string& _return, const std::string& addr){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->take_xray(_return);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::take_xrays_p(std::string& _return, const std::string& addr){
    LOG(LOG_LEVEL_RECORD, L"Address : %s", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
    thrift_connect_ex<common_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        thrift.client()->take_xrays(_return);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::create_job_ex_p(job_detail& _return, const std::string& addr, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){   
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
            check_license(job_id, true);
            thrift.client()->create_job_ex(_return.launcher, "", job_id, create_job);
        }
        else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
            thrift.client()->create_job_ex_p(_return, "", job_id, create_job, service_type);
        }
        else{
            check_license(job_id, false);
            thrift.client()->create_job_ex_p(_return, "", job_id, create_job, service_type);
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_job_p(job_detail& _return, const std::string& addr, const std::string& job_id, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE))
            thrift.client()->get_job(_return.launcher, "", job_id);
        else
            thrift.client()->get_job_p(_return, "", job_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::interrupt_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE))
            return thrift.client()->interrupt_job( "", job_id);
        else
            return thrift.client()->interrupt_job_p("", job_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::resume_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE))
            return thrift.client()->resume_job("", job_id);
        else
            return thrift.client()->resume_job_p("", job_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::remove_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE))
            return thrift.client()->remove_job("", job_id);
        else
            return thrift.client()->remove_job_p("", job_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::running_job_p(const std::string& addr, const std::string& job_id, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    macho::guid_  _service_type(service_type);
    if (thrift.open()){
        if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE))
            return thrift.client()->running_job("", job_id);
        else
            return thrift.client()->running_job_p("", job_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::update_job_p(const std::string& addr, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Job Id : %s, Type : %s", 
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(job_id).c_str(),
        macho::stringutils::convert_utf8_to_unicode(service_type).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->update_job_p("", job_id, create_job, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::remove_snapshot_image_p(const std::string& addr, const std::map<std::string, image_map_info> & images, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->remove_snapshot_image_p("", images, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::test_connection_p(const std::string& addr, const connection& conn, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->test_connection_p("", conn, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::add_connection_p(const std::string& addr, const connection& conn, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->add_connection_p("", conn, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::remove_connection_p(const std::string& addr, const std::string& connection_id, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->remove_connection_p("", connection_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::modify_connection_p(const std::string& addr, const connection& conn, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->modify_connection_p("", conn, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::enumerate_connections_p(std::vector<connection> & _return, const std::string& addr, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->enumerate_connections_p(_return, "", service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_connection_p(connection& _return, const std::string& addr, const std::string& connection_id, const std::string& service_type){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->get_connection_p(_return, "", connection_id, service_type);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_virtual_host_info_p(virtual_host& _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->get_virtual_host_info_p(_return, "", host, username, password);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_virtual_machine_detail_p(virtual_machine& _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->get_virtual_machine_detail_p(_return, "", host, username, password, machine_id);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_virtual_hosts_p(std::vector<virtual_host> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->get_virtual_hosts_p(_return, "", host, username, password);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::power_off_virtual_machine_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->power_off_virtual_machine_p("", host, username, password, machine_id);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::remove_virtual_machine_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->remove_virtual_machine_p("", host, username, password, machine_id);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::get_virtual_machine_snapshots_p(std::vector<vmware_snapshot> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        thrift.client()->get_virtual_machine_snapshots_p(_return,"", host, username, password, machine_id);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::remove_virtual_machine_snapshot_p(const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id, const std::string& snapshot_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->remove_virtual_machine_snapshot_p("", host, username, password, machine_id, snapshot_id);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::get_datacenter_folder_list_p(std::vector<std::string> & _return, const std::string& addr, const std::string& host, const std::string& username, const std::string& password, const std::string& datacenter){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->get_datacenter_folder_list_p(_return, "", host, username, password, datacenter);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_physical_machine_detail_p(physical_machine_info& _return, const std::string& addr, const std::string& host, const machine_detail_filter::type filter){
    if (is_uuid(host)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
        if (thrift.open()){
            thrift.client()->get_host_detail(_return, "", filter);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->get_physical_machine_detail_p(_return, "", host, filter);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
}

void transport_service_handler::take_packer_xray_p(std::string& _return, const std::string& addr, const std::string& host){
    if (is_uuid(host)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
        if (thrift.open()){
            thrift.client()->take_xray(_return);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->take_packer_xray_p(_return, "", host);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
}

void transport_service_handler::get_packer_service_info_p(service_info& _return, const std::string& addr, const std::string& host){
    if (is_uuid(host)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, host);
        if (thrift.open()){
            thrift.client()->ping(_return);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->get_packer_service_info_p(_return, "", host);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
}

bool transport_service_handler::verify_management_p(const std::string& addr, const std::string& management, const int32_t port, const bool is_ssl){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open()){
        return thrift.client()->verify_management(management, port, is_ssl);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::verify_packer_to_carrier_p(const std::string& addr, const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl){
    if (is_uuid(packer)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, packer);
        if (thrift.open()){
            return thrift.client()->verify_carrier(carrier, is_ssl);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{

        thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->verify_packer_to_carrier_p(packer, carrier, port, is_ssl);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    return false;
}

void transport_service_handler::get_replica_job_create_detail(replica_job_create_detail& _return, const std::string& session_id, const std::string& job_id){

}

void transport_service_handler::get_loader_job_create_detail(loader_job_create_detail& _return, const std::string& session_id, const std::string& job_id){

}

void transport_service_handler::get_launcher_job_create_detail(launcher_job_create_detail& _return, const std::string& session_id, const std::string& job_id){

}

void transport_service_handler::terminate(const std::string& session_id){

}

std::string transport_service_handler::extract_file_resource(std::string custom_resource_name, int resource_id){
    HGLOBAL hResourceLoaded;		// handle to loaded resource 
    HRSRC hRes;						// handle/ptr. to res. info. 
    char *lpResLock;				// pointer to resource data 
    DWORD dwSizeRes;
    // find location of the resource and get handle to it
    hRes = FindResourceA(NULL, MAKEINTRESOURCEA(resource_id), custom_resource_name.c_str());
    // loads the specified resource into global memory. 
    hResourceLoaded = LoadResource(NULL, hRes);
    // get a pointer to the loaded resource!
    lpResLock = (char*)LockResource(hResourceLoaded);
    // determine the size of the resource, so we know how much to write out to file!  
    dwSizeRes = SizeofResource(NULL, hRes);
    return std::string((const char*)lpResLock, dwSizeRes);
}

std::string transport_service_handler::public_encrypt(const std::string& data, const std::string& key){

    RSA *rsa = NULL;
    BIO *bio;
    std::string ret;
    bio = BIO_new_mem_buf((void*)key.c_str(), (int)key.length());
    if (bio == NULL){
        LOG(LOG_LEVEL_ERROR, L"Failed to create key BIO");
    }
    else{
        rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, 0, NULL);
        if (rsa == NULL){
            LOG(LOG_LEVEL_ERROR, L"Failed to create RSA");
        }
        else{
            int maxSize = RSA_size(rsa);
            std::auto_ptr<unsigned char> output(new unsigned char[maxSize]);
            int data_size = 245;
            for (int i = 0; i < data.length(); i += data_size){
                memset(output.get(), 0, sizeof(char) *maxSize);
                int size = data.length() - i > data_size ? data_size : data.length() - i;
                int result = RSA_public_encrypt( size, (unsigned char *)&data.c_str()[i], output.get(), rsa, padding);
                if (result == -1){
                    LOG(LOG_LEVEL_ERROR, L"Public Encrypt failed");
                    break;
                }
                else{
                    ret.append(std::string(reinterpret_cast<char const*>(output.get()), result));
                }
            }
        }
        BIO_free(bio);
    }
    return encode64(ret);
}

std::string transport_service_handler::private_decrypt(const std::string& info, const std::string& key){

    RSA *rsa = NULL;
    BIO *bio;
    std::string ret;
    bio = BIO_new_mem_buf((void*)key.c_str(), (int)key.length());
    if (bio == NULL){
        LOG(LOG_LEVEL_ERROR, L"Failed to create key BIO");
    }
    else{
        rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, 0, NULL);
        if (rsa == NULL){
            LOG(LOG_LEVEL_ERROR, L"Failed to create RSA");
        }
        else{
            std::string data = decode64(info);
            int maxSize = RSA_size(rsa);
            std::auto_ptr<unsigned char> output(new unsigned char[maxSize]);
            for (int i = 0; i < data.length(); i += maxSize){
                memset(output.get(), 0, sizeof(char) *maxSize);
                int size = data.length() - i > maxSize ? maxSize : data.length() - i;
                int result = RSA_private_decrypt(size, (unsigned char *)&data.c_str()[i], output.get(), rsa, padding);
                if (result == -1){
                    LOG(LOG_LEVEL_ERROR, L"Public Decrypt failed");
                    break;
                }
                else{
                    ret.append(std::string(reinterpret_cast<char const*>(output.get()), result));
                }
            }
            RSA_free(rsa);
        }
        BIO_free(bio);
    }
    return ret;
}

bool transport_service_handler::sync_db(const std::string& job_id, std::string& machine_id, bool is_recovery){
    mysql_op::ptr db = mysql_op::open();
    bool is_sync = false;
    if (db){
        if (!job_id.empty())
            machine_id = db->query_replica_machine_id(job_id, is_recovery);
        replica::vtr repls = db->query_replicas();
        is_sync = repls.size() == 0;
        foreach(replica& repl, repls){
            workload::ptr w = _db->get_workload(repl.hosts[0].uuid, repl.create_time);
            if (w){  
                is_sync = w->deleted == repl.delete_time && w->updated == repl.update_time;
                if (!is_sync){
                    if (w->deleted < repl.delete_time)
                        w->deleted = repl.delete_time;
                    if (w->updated < repl.update_time)
                        w->updated = repl.update_time;
                    if (!(is_sync = _db->update_workload(*w.get())))
                        break;
                }
            }
            else{
                if (-1 == repl.delete_time){
                    workload::vtr workloads = _db->get_workload(repl.hosts[0].uuid);
                    foreach(workload::ptr workload, workloads){
                        if (-1 == workload->deleted){
                            if (-1 == workload->updated)
                                workload->deleted = boost::posix_time::to_time_t(boost::posix_time::second_clock::universal_time());
                            else
                                workload->deleted = workload->updated;
                            _db->update_workload(*workload.get());
                        }
                    }
                }
                workload _w;
                _w.host = repl.hosts[0].uuid;
                _w.created = repl.create_time;
                _w.deleted = repl.delete_time;
                _w.updated = repl.update_time;
                _w.type = repl.hosts[0].type;
                _w.name = repl.hosts[0].name;
                if (!(is_sync = _db->insert_workload(_w)))
                    break;
            }
        }
        workload::vtr workloads = _db->get_workloads();
        foreach(workload::ptr workload, workloads){
            if (-1 == workload->deleted){
                bool found = false;
                foreach(replica& repl, repls){
                    if (workload->host == repl.hosts[0].uuid){
                        found = true;
                        break;
                    }
                }
                if (!found){
                    if (-1 == workload->updated)
                        workload->deleted = boost::posix_time::to_time_t(boost::posix_time::second_clock::universal_time());
                    else
                        workload->deleted = workload->updated;
                    _db->update_workload(*workload.get());
                }
            }
        }
        std::string  mac_checksum = md5((std::string)_mac);
        boost::filesystem::path back_up_db = boost::filesystem::path(macho::windows::environment::get_working_directory()) / "logs" / boost::str(boost::format("%s.bak") % mac_checksum);
        _db->save(back_up_db.string(), ((std::string)_mac) + std::string("SaaSaMeFTW"));
        return is_sync;
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE;
        error.why = "Invalid License.";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(error.why).c_str());
        throw error;
    }
    return is_sync;
}

void transport_service_handler::check_license(const std::string& job_id, bool is_recovery){
    macho::windows::auto_lock lck(_cs);
    std::string machine_id;
    if (sync_db(job_id, machine_id, is_recovery)){
        workload::vtr workloads = _db->get_workloads();
        saasame::transport::license::vtr _lics = _db->get_licenses();
        saasame::transport::license::vtr activated_lics;
        std::remove_copy_if(_lics.begin(), _lics.end(), std::back_inserter(activated_lics), [](const saasame::transport::license::ptr& obj){return obj->lic.empty(); });
        if (activated_lics.empty()){
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE;
            error.why = "Invalid License.";
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(error.why).c_str());
            throw error;
        }
        else{
            std::vector<license_info> _return;
            foreach(saasame::transport::license::ptr lic, activated_lics){
                license_info lic_info;
                lic_info.key = lic->key;
                lic_info.is_active = true;
                lic_info.activated = macho::xor_crypto::decrypt(lic->active, _ps);
                macho::LIC_INFO_V1 _lic;
                memset(&_lic, 0, sizeof(macho::LIC_INFO_V1));
                if (macho::license::unpack_keycode2(lic->lic, (std::string)_ps + lic_info.activated, _lic) && _lic.product == PRODUCT_CODE::Product_Transport){
                    lic_info.count = _lic.count;
                    if (_lic.year && _lic.mon && _lic.day)
                        lic_info.expired_date = boost::posix_time::to_simple_string(boost::posix_time::ptime(boost::gregorian::date(2000 + _lic.year, _lic.mon, _lic.day)));
                    _return.push_back(lic_info);
                }
            }

            std::sort(_return.begin(), _return.end(), [](const license_info & l, const license_info & r) -> bool {
                if (l.expired_date.empty() && r.expired_date.empty()){
                    return boost::posix_time::time_from_string(l.activated) < boost::posix_time::time_from_string(r.activated);
                }
                else if (!l.expired_date.empty() && !r.expired_date.empty()){
                    return boost::posix_time::time_from_string(l.expired_date) < boost::posix_time::time_from_string(r.expired_date);
                }
                return l.expired_date.length() > r.expired_date.length();
            });
            bool can_be_run = false;
            workload_consumed_map_type consumed;
            calculated_license_consumed(_return, machine_id, can_be_run, consumed, is_recovery);
            if (!can_be_run){
                saasame::transport::invalid_operation error;
                error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_LICENSE;
                error.why = "Invalid License.";
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(error.why).c_str());
                throw error;   
            }
        }
    }
}

transport_service_handler::workload_consumed_map_type transport_service_handler::calculated_consumed(workload::map& workloads){
    workload_consumed_map_type workload_consumeds;
    foreach(workload::map::value_type& w, workloads){
        workload_consumeds[w.first] = calculated_consumed(w.second);
    }
    return workload_consumeds;
}

transport_service_handler::consumed_map_type transport_service_handler::calculated_consumed(workload::vtr& workloads){
    consumed_map_type consumeds;
    foreach(workload::ptr workload, workloads){
        boost::posix_time::ptime::date_type updated = boost::posix_time::from_time_t(workload->updated).date();
        boost::posix_time::ptime::date_type created = boost::posix_time::from_time_t(workload->created).date();
        boost::posix_time::ptime::date_type current = boost::posix_time::second_clock::universal_time().date();
        boost::posix_time::ptime::date_type deleted;
        if (-1 == workload->deleted){
            if (updated > current)
                deleted = updated;
            else
                deleted = current;
        }
        else{
            deleted = boost::posix_time::from_time_t(workload->deleted).date();
            if (updated > deleted)
                deleted = updated;
        }
        for (uint32_t s = (created.year() * 12 + created.month()); s <= (deleted.year() * 12 + deleted.month()); ++s)
            consumeds[s]++;
    }
    return consumeds;
}

transport_service_handler::workload_consumed_map_type transport_service_handler::calculated_license_consumed(std::vector<license_info> & _return, const std::string& machine_id, bool &can_be_run, workload_consumed_map_type& consumed, bool is_recovery, boost::posix_time::ptime datetime){
    workload::map workloads = _db->get_workloads_map();
    workload_consumed_map_type consumeds = calculated_consumed(workloads);
    if (machine_id.empty())
        consumed = consumeds;
    boost::posix_time::ptime::date_type current = datetime.date();
    uint32_t _current = current.year() * 12 + current.month();
    workload_consumed_map_type backup_consumeds = consumeds;
    if (consumeds.size()){
        uint32_t min = -1, max = 0;
        foreach(workload_consumed_map_type::value_type &wc, consumeds){
            for (auto it = wc.second.cbegin(); it != wc.second.cend(); it++){
                if (it->first < min)
                    min = it->first;
                if (it->first > max)
                    max = it->first;
            }
        }
        for (uint32_t d = min; d <= max; d++){
            foreach(license_info &info, _return){
                if (!info.expired_date.empty()){
                    boost::posix_time::ptime::date_type expired = boost::posix_time::time_from_string(info.expired_date).date();
                    uint32_t _expired = expired.year() * 12 + expired.month();
                    foreach(workload_consumed_map_type::value_type &wc, consumeds){
                        if (info.consumed < info.count){
                            if (d <= _expired){
                                if (wc.second.count(d)){
                                    info.consumed++;
                                    wc.second.erase(d);
                                }
                            }
                            else{
                                break;
                            }
                        }
                        else{
                            break;
                        }
                    }
                    if (_current > _expired){
                        info.status = "i";
                        can_be_run = false;
                    }
                    else
                        can_be_run = info.consumed < info.count;
                }
                else{
                    foreach(workload_consumed_map_type::value_type &wc, consumeds){
                        if (info.consumed < info.count){
                            if (wc.second.count(d)){
                                info.consumed++;
                                wc.second.erase(d);
                            }
                        }
                        else{
                            break;
                        }
                    }
                    can_be_run = info.consumed < info.count;
                }
            }
        }
        if (!is_recovery && !can_be_run && !machine_id.empty() && backup_consumeds.count(machine_id)){
            can_be_run = backup_consumeds[machine_id].count(_current) != 0;
        }
    }
    else{
        foreach(license_info &info, _return){
            if (!info.expired_date.empty()){
                boost::posix_time::ptime::date_type expired = boost::posix_time::time_from_string(info.expired_date).date();
                uint32_t _expired = expired.year() * 12 + expired.month();         
                if (_current > _expired)
                    can_be_run = false;
                else
                    can_be_run = info.consumed < info.count;
            }
            else{
                can_be_run = info.consumed < info.count;
            }
        }
    }
    if (is_recovery && !can_be_run && !machine_id.empty() && backup_consumeds.count(machine_id)){
        uint32_t bill = 0;
        workload_consumed_map_type _consumeds = consumeds;
        foreach(workload_consumed_map_type::value_type &wc, _consumeds){
            for (auto it = wc.second.cbegin(); it != wc.second.cend()/* not hoisted */; /* no increment */){
                wc.second.erase(it++);
                bill++;
            }
        }
        can_be_run = (0 == bill);
    }
    return consumeds;
}

bool transport_service_handler::set_customized_id_p(const std::string& addr, const std::string& disk_addr, const std::string& disk_id){
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        return thrift.client()->set_customized_id_p("",disk_addr, disk_id);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::timer_sync_db(const trigger::ptr& job, const job_execution_context& ec){
    try{
        {
            macho::windows::auto_lock lck(_cs);
            sync_db();
        }
        _reverse.release();
    }
    catch (...){
    }
}

bool transport_service_handler::create_mutex_p(const std::string& addr, const std::string& session, const int16_t timeout){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Session : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(session).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        return thrift.client()->create_mutex(session, timeout);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::delete_mutex_p(const std::string& addr, const std::string& session){
    LOG(LOG_LEVEL_RECORD, L"Address : %s, Session : %s",
        macho::stringutils::convert_utf8_to_unicode(addr).c_str(),
        macho::stringutils::convert_utf8_to_unicode(session).c_str());
    thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
    if (thrift.open())
        return thrift.client()->delete_mutex(session);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::create_task(const running_task& task){
    try{
        macho::windows::auto_lock lck(_sync_cs);
        _sync.initial(0);
        if (_running_tasks.count(task.id)){
            _running_tasks[task.id]->update(task);
            if (_sync.is_scheduled(_running_tasks[task.id]->id()))
                _sync.update_job_triggers(macho::stringutils::convert_utf8_to_unicode(task.id), _running_tasks[task.id]->get_trigger());
            else
                _sync.schedule_job(_running_tasks[task.id], _running_tasks[task.id]->get_trigger());
        }
        else{
            long_running_task::ptr tk = long_running_task::create(task);
            tk->save();
            _running_tasks[task.id] = tk;
            _sync.schedule_job(tk, tk->get_trigger());
        }
        return true;
    }
    catch (...){}
    return false;
}

bool transport_service_handler::remove_task(const std::string& task_id){
    macho::windows::auto_lock lck(_sync_cs);
    if (_running_tasks.count(task_id)){
        _running_tasks.erase(task_id);
        _sync.remove_job(macho::stringutils::convert_utf8_to_unicode(task_id));
    }
    return true;
}

void transport_service_handler::load_tasks(){
    macho::windows::auto_lock lck(_sync_cs);
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    namespace fs = boost::filesystem;
    fs::path job_dir = working_dir / "jobs";
    fs::directory_iterator end_iter;
    typedef std::multimap<std::time_t, fs::path> result_set_t;
    result_set_t result_set;
    if (fs::exists(job_dir) && fs::is_directory(job_dir)){
        for (fs::directory_iterator dir_iter(job_dir); dir_iter != end_iter; ++dir_iter){
            if (fs::is_regular_file(dir_iter->status())){
                if (dir_iter->path().extension() == JOB_EXTENSION){
                    _sync.initial(0);
                    long_running_task::ptr tk = long_running_task::load(dir_iter->path());
                    _running_tasks[macho::stringutils::convert_unicode_to_utf8(tk->id())] = tk;
                    _sync.schedule_job(tk, tk->get_trigger());
                }
            }
        }
    }
}

long_running_task::long_running_task(running_task task) 
    : _task(task)
    , removeable_job(macho::stringutils::convert_utf8_to_unicode(task.id), macho::guid_(GUID_NULL)){
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
}

long_running_task::ptr long_running_task::create(running_task task){
    return long_running_task::ptr(new long_running_task(task));
}

void long_running_task::save(){
    try{
    using namespace json_spirit;
    mObject task_config;
    task_config["id"] = _task.id;
    task_config["mgmt_addr"] = _task.mgmt_addr;
    task_config["mgmt_port"] = _task.mgmt_port;
    task_config["is_ssl"] = _task.is_ssl;
    mArray triggers;
    foreach(saasame::transport::job_trigger &t, _task.triggers){
        mObject trigger;
        if (0 == t.start.length())
            t.start = boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time());
        trigger["type"] = (int)t.type;
        trigger["start"] = t.start;
        trigger["finish"] = t.finish;
        trigger["interval"] = t.interval;
        trigger["duration"] = t.duration;
        trigger["id"] = t.id;
        triggers.push_back(trigger);
    }
    task_config["triggers"] = triggers;
    task_config["parameters"] = _task.parameters;
    boost::filesystem::path temp = _config_file.string() + ".tmp";
    {
        std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
        std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
        output.imbue(loc);
        write(task_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
    }
    if (!MoveFileEx(temp.wstring().c_str(), _config_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
        LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _config_file.wstring().c_str(), GetLastError());
    }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")).c_str());
    }
    catch (...){
    }
}

long_running_task::ptr long_running_task::load(boost::filesystem::path config_file){
    try{
        std::ifstream is(config_file.string());
        mValue task_config;
        read(is, task_config);
        saasame::transport::running_task task;
        task.id = (std::string)find_value(task_config.get_obj(), "id").get_str();
        task.mgmt_addr = (std::string)find_value(task_config.get_obj(), "mgmt_addr").get_str();
        task.mgmt_port = find_value_int32(task_config.get_obj(), "mgmt_port", saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PORT);
        task.is_ssl = find_value_bool(task_config.get_obj(), "is_ssl");
        mArray triggers = find_value(task_config.get_obj(), "triggers").get_array();
        foreach(mValue t, triggers){
            saasame::transport::job_trigger trigger;
            trigger.type = (saasame::transport::job_trigger_type::type)find_value(t.get_obj(), "type").get_int();
            trigger.start = find_value(t.get_obj(), "start").get_str();
            trigger.finish = find_value(t.get_obj(), "finish").get_str();
            trigger.interval = find_value(t.get_obj(), "interval").get_int();
            trigger.duration = find_value(t.get_obj(), "duration").get_int();
            trigger.id = find_value_string(t.get_obj(), "id");
            task.triggers.push_back(trigger);
        }
        task.parameters = (std::string)find_value(task_config.get_obj(), "parameters").get_str();
        return long_running_task::ptr(new long_running_task(task));
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return long_running_task::ptr();
}

void long_running_task::execute(){
    std::set<std::string> mgmt_addrs;
    mgmt_addrs.insert(_task.mgmt_addr);
    mgmt_op op(mgmt_addrs, _task.mgmt_addr, _task.mgmt_port, _task.is_ssl);
    if (op.open()){
        try{
            _is_removing = op.client()->check_running_task(_task.id, _task.parameters);
            if (_is_removing)
                LOG(LOG_LEVEL_RECORD, L"Task '%s' will be removed.", _id.c_str());
            else
                LOG(LOG_LEVEL_RECORD, L"Task '%s' has been triggered.", _id.c_str());
        }
        catch (TException & tx){
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }   
}

void long_running_task::remove(){
    boost::filesystem::remove(_config_file);
}

trigger::vtr long_running_task::get_trigger(){
    trigger::vtr triggers;
    foreach(saasame::transport::job_trigger t, _task.triggers){
        trigger::ptr _t;
        switch (t.type){
        case saasame::transport::job_trigger_type::interval_trigger:{
            if (t.start.length() && t.finish.length()){
                _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish)));
            }
            else if (t.start.length()){
                _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time));
            }
            else if (t.finish.length()){
                _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish)));
            }
            else{
                _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time));
            }
            break;
        }
        case saasame::transport::job_trigger_type::runonce_trigger:{
            if (t.start.length()){
                _t = trigger::ptr(new run_once_trigger(boost::posix_time::time_from_string(t.start)));
            }
            else{
                _t = trigger::ptr(new run_once_trigger());
            }
            break;
        }
        }
        triggers.push_back(_t);
    }
    return triggers;
}

void transport_service_handler::packer_ping_p(service_info& _return, const std::string& session_id, const std::string& addr){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->ping(_return);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::take_snapshots_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->take_snapshots(_return, session_id, disks);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::take_snapshots_ex_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->take_snapshots_ex(_return, session_id, disks, pre_script, post_script);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::take_snapshots2_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const take_snapshots_parameters& parameters){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->take_snapshots2(_return, session_id, parameters);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::delete_snapshot_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->delete_snapshot(_return, session_id, snapshot_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::delete_snapshot_set_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_set_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->delete_snapshot_set(_return, session_id, snapshot_set_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_all_snapshots_p(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id, const std::string& addr) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->get_all_snapshots(_return, session_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::create_packer_job_ex_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const create_packer_job_detail& create_job) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->create_job_ex(_return, session_id, job_id, create_job);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

void transport_service_handler::get_packer_job_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const std::string& previous_updated_time) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->get_job(_return, session_id, job_id, previous_updated_time);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::interrupt_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->interrupt_job(session_id, job_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::resume_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->resume_job(session_id, job_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::remove_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->remove_job(session_id, job_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

bool transport_service_handler::running_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id) {
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->running_job(session_id, job_id);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::enumerate_packer_disks_p(std::set<disk_info> & _return, const std::string& session_id, const std::string& addr, const enumerate_disk_filter_style::type filter){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->enumerate_disks(_return, filter);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::verify_packer_carrier_p(const std::string& session_id, const std::string& addr, const std::string& carrier, const bool is_ssl){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->verify_carrier( carrier, is_ssl);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
    return false;
}

void transport_service_handler::get_packer_host_detail_p(physical_machine_info& _return, const std::string& session_id, const std::string& addr, const machine_detail_filter::type filter){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            thrift.client()->get_host_detail(_return, session_id, filter);
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to host.";
        error.arguments.push_back(addr);
        throw error;
    }
}

bool transport_service_handler::unregister_packer_p(const std::string& addr){
    if (is_uuid(addr)){
        thrift_connect_ex<physical_packer_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->unregister("");
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    return false;
}

bool transport_service_handler::unregister_server_p(const std::string& addr){
    if (is_uuid(addr)){
        thrift_connect_ex<launcher_serviceClient> thrift(_reverse, saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT, addr);
        if (thrift.open()){
            return thrift.client()->unregister("");
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Cannot connect to host (%s).", macho::stringutils::convert_utf8_to_unicode(addr).c_str());
            saasame::transport::invalid_operation error;
            error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
            error.why = "Cannot connect to host.";
            error.arguments.push_back(addr);
            throw error;
        }
    }
    return false;
}

void transport_service_handler::create_vhd_disk_from_snapshot(std::string& _return, const std::string& connection_string, const std::string& container, const std::string& original_disk_name, const std::string& target_disk_name, const std::string& snapshot){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->create_vhd_disk_from_snapshot(_return, connection_string, container, original_disk_name, target_disk_name, snapshot);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
}

bool transport_service_handler::is_snapshot_vhd_disk_ready(const std::string& task_id){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->is_snapshot_vhd_disk_ready(task_id);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
    return false;
}

bool transport_service_handler::delete_vhd_disk(const std::string& connection_string, const std::string& container, const std::string& disk_name){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->delete_vhd_disk(connection_string, container, disk_name);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
    return false;
}

bool transport_service_handler::delete_vhd_disk_snapshot(const std::string& connection_string, const std::string& container, const std::string& disk_name, const std::string& snapshot){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->delete_vhd_disk_snapshot(connection_string, container, disk_name, snapshot);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
    return false;
}

void transport_service_handler::get_vhd_disk_snapshots(std::vector<vhd_snapshot> & _return, const std::string& connection_string, const std::string& container, const std::string& disk_name){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_vhd_disk_snapshots(_return, connection_string, container, disk_name);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
}

bool transport_service_handler::verify_connection_string(const std::string& connection_string){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->verify_connection_string(connection_string);
    else{
        LOG(LOG_LEVEL_ERROR, L"Cannot connect to loader service.");
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_CANNOT_CONNECT_TO_HOST;
        error.why = "Cannot connect to loader service.";
        throw error;
    }
}
