#pragma once
#ifndef webdav_H
#define webdav_H

#include "macho.h"
#include <ne_request.h> 
#include <ne_auth.h>
#include <ne_basic.h>
#include <ne_locks.h>
#include <ne_props.h>
#include <deque>
#include "common\exception_base.hpp"

class webdav{
public:
    typedef boost::shared_ptr<webdav> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::deque<ptr> queue;
    struct item{
        typedef boost::shared_ptr<item> ptr;
        typedef std::vector<ptr> vtr;
        item() : is_dir(false){}
        std::string name;
        bool        is_dir;
    };

    virtual ~webdav();
    virtual bool create_dir(std::string _path);
    virtual bool remove_dir(std::string _path);
    virtual bool remove(std::string _path);
    virtual bool is_dir_existing(std::string _path);
    virtual bool is_existing(std::string _path);
    virtual bool get_file(std::string _path, std::ostream& data);
    virtual bool enumerate_sub_items(std::string _path, item::vtr& items);
    virtual item::vtr enumerate_sub_items(std::string _path);
    virtual bool put_file(std::string _path, std::istream& data);
    virtual bool create_empty_file(std::string _path);    
    webdav::ptr clone();
    static webdav::ptr connect(std::string url, std::string user, std::string password, uint32_t port = 0, std::string proxy_host = "", uint32_t proxy_port = 0, std::string proxy_username = "", std::string proxy_password = "", int32_t timeout = 300);
    static int inline initialize(){
        return ne_sock_init();
    }
    static void inline uninitialize(){
        ne_sock_exit();
    }
    std::string uri() const { return url; }
private:
    static int _dispatch_to_buffer(ne_request *req, std::ostream& data, const char *range);
    static int _read_response_to_buffer(ne_request *req, std::ostream& data, int32_t length);
    static void _props_result(void *userdata, const ne_uri *uri, const ne_prop_result_set *results);
    static int  _define_auth(void *userdata, const char *realm, int attempts, char *username, char *password);
    static int  _ssl_verify(void *userdata, int failures, const ne_ssl_certificate *cert);
    static ssize_t _provide_body(void *userdata, char *buffer, size_t buflen);
    webdav();
    ne_session*   dav;
    ne_uri        nuri;
    unsigned int  caps;
    std::string                    url;
    macho::windows::protected_data user;
    macho::windows::protected_data password;
    macho::windows::critical_section cs;
    uint32_t                       port;
    uint32_t                       proxy_port;
    std::string                    proxy_host;
    macho::windows::protected_data proxy_username;
    macho::windows::protected_data proxy_password;
    int32_t                        timeout;

public:
    class lock_ex : virtual public macho::windows::lock_able_ex {
    public:
        lock_ex(boost::filesystem::path file, webdav::ptr dav, std::string flag, int wait_count = 1000) : _lock(false), _dav(dav), _flag(false), _count(wait_count){
            _lock_file = file.string() + ".lock";
            if (flag.length())
                _flag_file = _lock_file + "/" + flag;
        }
        ~lock_ex(){
            unlock();
        }
        bool lock(){
            if (!_lock){
                bool _check_flag = false;
                int  count = _count;
                while (!(_lock = _dav->create_dir(_lock_file))){
                    if (!_check_flag && !_flag_file.empty()){
                        if (_flag = _dav->is_dir_existing(_flag_file)){
                            _lock = true;
                            break;
                        }
                        _check_flag = true;
                    }
                    if (is_canceled()){
                        BOOST_THROW_EXCEPTION_BASE_STRING(lock_able::exception, boost::str(boost::wformat(L"Cancelled lock file (%s).") % macho::stringutils::convert_utf8_to_unicode(_lock_file)));
                    }
                    else if (count){
                        Sleep(1000);
                        count--;
                    }
                    else{
                        if (!_flag_file.empty()){
                            item::vtr items;
                            using namespace macho;
                            if (_dav->enumerate_sub_items(_lock_file, items) && 0 == items.size()){
                                if (_flag = _dav->create_dir(_flag_file)){
                                    _lock = true;
                                    LOG(LOG_LEVEL_RECORD, L"Recovery from deadlock: %s", macho::stringutils::convert_utf8_to_unicode(_lock_file).c_str());
                                    break;
                                }
                            }
                            else{
                                foreach(item::ptr it, items)
                                    LOG(LOG_LEVEL_RECORD, L"Subitem: %s/%s", macho::stringutils::convert_utf8_to_unicode(_lock_file).c_str(), macho::stringutils::convert_utf8_to_unicode(it->name).c_str());
                                BOOST_THROW_EXCEPTION_BASE_STRING(lock_able::exception, boost::str(boost::wformat(L"Lock file (%s) time out.") % macho::stringutils::convert_utf8_to_unicode(_lock_file)));
                            }
                        }
                        else{
                            BOOST_THROW_EXCEPTION_BASE_STRING(lock_able::exception, boost::str(boost::wformat(L"Lock file (%s) time out.") % macho::stringutils::convert_utf8_to_unicode(_lock_file)));
                        }
                    }
                }
                if (!_flag && !_flag_file.empty())
                    _flag = _dav->create_dir(_flag_file);
            }
            return _lock;
        }
        bool unlock(){
            if (_lock){
                _lock = !_dav->remove_dir(_lock_file);
                _flag = _lock;
            }
            return !_lock;
        }
        bool trylock(){
            _lock = _dav->create_dir(_lock_file);
            if (!_flag_file.empty()){
                if (_lock)
                    _flag = _dav->create_dir(_flag_file);
                else if (_flag = _dav->is_dir_existing(_flag_file))
                    _lock = true;
            }
            return _lock;
        }
    private:
        std::string             _lock_file;
        webdav::ptr             _dav;
        bool                    _lock;
        std::string             _flag_file;
        bool                    _flag;
        int                     _count;
    };
};

#endif