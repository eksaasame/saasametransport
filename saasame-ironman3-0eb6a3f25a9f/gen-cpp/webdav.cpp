
#include "webdav.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libneon.lib" )
#pragma comment(lib, "libexpat.lib" )
#pragma comment(lib, "ssleay32.lib" )
#pragma comment(lib, "libeay32.lib" )

using namespace macho::windows;
using namespace macho;

static const ne_propname _props[] = {
    { "DAV:", "resourcetype" },
    { NULL }
};

struct put_data{
    put_data(std::istream& _data, size_t _size) : data(_data), size(_size){
    }
    std::istream& data;
    int64_t       size;
};

int webdav::_define_auth(void *userdata, const char *realm, int attempts, char *username, char *password){
    webdav*  _webdav = (webdav*)userdata;
    strncpy(username, _webdav->user.string().c_str(), NE_ABUFSIZ); strncpy(password, _webdav->password.string().c_str(), NE_ABUFSIZ);
    return attempts;
}

int webdav::_ssl_verify(void *userdata, int failures, const ne_ssl_certificate *cert){
    return NE_OK;
}

ssize_t webdav::_provide_body(void *userdata, char *buffer, size_t buflen){
    size_t data_size = 0;
    if (buflen){
        put_data* p_data = (put_data*)userdata;
        int64_t file_ofs = p_data->data.tellg();
        if ((file_ofs + buflen) > p_data->size)
            buflen = p_data->size - file_ofs;
        p_data->data.read(buffer, buflen);
        data_size = (p_data->data.tellg() - file_ofs);
    }
    return data_size;
}

void webdav::_props_result(void *userdata, const ne_uri *uri, const ne_prop_result_set *results){
    if (NULL != userdata){       
        item::vtr* pitems = (item::vtr*)userdata;
        item::ptr i = item::ptr(new item());
        const char* value = ne_propset_value(results, _props);
        i->is_dir = value != NULL ? _stricmp(value, "<DAV:collection></DAV:collection>") == 0 : __noop;
        i->name = uri->path;
        pitems->push_back(i);
    }
}

webdav::webdav()
: dav(NULL),
caps(0),
port(0),
proxy_port(0),
timeout(300){
}

webdav::~webdav(){
    ne_uri_unparse(&nuri);
    if (NULL!= dav)
        ne_session_destroy(dav);
}

webdav::ptr webdav::clone(){
    return connect(url, user, password, port, proxy_host, proxy_port, proxy_username, proxy_password, timeout);
}

webdav::ptr webdav::connect(std::string url, std::string user, std::string password, uint32_t port, std::string proxy_host, uint32_t proxy_port, std::string proxy_username, std::string proxy_password, int32_t timeout ){
    webdav::ptr _webdav = webdav::ptr(new webdav());
    int ret = ne_uri_parse(url.c_str(), &_webdav->nuri);
    if (ret != NE_OK){
        LOG(LOG_LEVEL_ERROR, L"Request failed: ne_uri_parse(%s)", macho::stringutils::convert_utf8_to_unicode(url).c_str());
        _webdav = NULL;
    }
    else if (0 == strlen(_webdav->nuri.scheme) || 0 == strlen(_webdav->nuri.host)){
        LOG(LOG_LEVEL_ERROR, L"Invalid uri format: %s", macho::stringutils::convert_utf8_to_unicode(url).c_str());
        _webdav = NULL;
    }
    else{
        if (0 == port){
            port = 80;
            if (0 == _stricmp(_webdav->nuri.scheme, "https"))
                port = 443;
        }
        _webdav->port = port;
        _webdav->url = url;
        _webdav->user = user;
        _webdav->password = password;
        _webdav->dav = ne_session_create(_webdav->nuri.scheme, _webdav->nuri.host, port);
        if (proxy_host.length() && proxy_port)
        {
            _webdav->proxy_host = proxy_host;
            _webdav->proxy_port = proxy_port;
            if (proxy_username.length() && proxy_password.length())
            {
                _webdav->proxy_username = proxy_username;
                _webdav->proxy_password = proxy_password;
                ne_session_socks_proxy(_webdav->dav, ne_sock_sversion::NE_SOCK_SOCKSV5, proxy_host.c_str(), proxy_port, proxy_username.c_str(), proxy_password.c_str());
            }
            else
                ne_session_proxy(_webdav->dav, proxy_host.c_str(), proxy_port);
        }
        else
        {
            ne_session_system_proxy(_webdav->dav, 0); // Using default system proxy settings
        }
        _webdav->timeout = timeout;
        ne_set_connect_timeout(_webdav->dav, 30);
        ne_set_read_timeout(_webdav->dav, timeout);
        ne_set_server_auth(_webdav->dav, webdav::_define_auth, _webdav.get());
        ne_ssl_set_verify(_webdav->dav, webdav::_ssl_verify, NULL);
        std::string p = _webdav->nuri.path;
        p.append("/");
        int retry_count = 1;
        LOG(LOG_LEVEL_RECORD, L"Try to connect: %s",
            stringutils::convert_utf8_to_unicode(url).c_str());
        while (NE_OK != (ret = ne_options2(_webdav->dav, p.c_str(), &_webdav->caps))){
            LOG(LOG_LEVEL_ERROR, L"Request failed: %s (%s)", 
                stringutils::convert_ansi_to_unicode(std::string(ne_get_error(_webdav->dav))).c_str(),
                stringutils::convert_utf8_to_unicode(url).c_str());
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
        if (ret != NE_OK){
            _webdav = NULL;
        }
        else {
            int session_persistent = ne_get_session_flag(_webdav->dav, ne_session_flag::NE_SESSFLAG_PERSIST);
            LOG(LOG_LEVEL_RECORD, L"NE_SESSFLAG_PERSIST: %d (%s)", session_persistent, macho::stringutils::convert_utf8_to_unicode(url).c_str());
            if (0 == session_persistent){
                ne_set_session_flag(_webdav->dav, ne_session_flag::NE_SESSFLAG_PERSIST, 1);
            }
        }
    }
    return _webdav;
}

bool webdav::create_dir(std::string _path){
    std::string p = boost::str(boost::format("%s/%s/") %nuri.path %_path);
    macho::windows::auto_lock lock(cs);
    int ret = ne_mkcol(dav, p.c_str());
    if (ret != NE_OK){
        std::string error = ne_get_error(dav);
        LOG((error.find("405") != std::string::npos) ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR, L"(%s) - Request failed: %s", 
            stringutils::convert_ansi_to_unicode(_path).c_str(), 
            stringutils::convert_ansi_to_unicode(error).c_str());
        return false;
    }
    return true;
}

bool webdav::remove_dir(std::string _path){
    return remove(_path[_path.length() - 1] != '/' ? _path.append("/") : _path);
}

bool webdav::remove(std::string _path){
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    macho::windows::auto_lock lock(cs);
    int ret = ne_delete(dav, p.c_str());
    if (ret != NE_OK){
        std::string error = ne_get_error(dav);
        if (error.find("404") == std::string::npos){
            LOG(LOG_LEVEL_ERROR, L"(%s) - Request failed: %s", 
                stringutils::convert_ansi_to_unicode(_path).c_str(), 
                stringutils::convert_ansi_to_unicode(error).c_str());
            return false;
        }
    }
    return true;
}

bool webdav::is_dir_existing(std::string _path){
    if (_path.length() && _path[_path.length() - 1] != '/')
        _path.append("/");
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    macho::windows::auto_lock lock(cs);
    int ret = ne_simple_propfind(dav, p.c_str(), NE_DEPTH_ZERO, _props, webdav::_props_result, NULL);
    if (ret != NE_OK){
        LOG(LOG_LEVEL_INFO, L"(%s) - Request failed: %s", 
            stringutils::convert_ansi_to_unicode(_path).c_str(),
            stringutils::convert_ansi_to_unicode(std::string(ne_get_error(dav))).c_str());
        return false;
    }
    return true;
}

bool webdav::is_existing(std::string _path){
    time_t t;
    macho::windows::auto_lock lock(cs);
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    int ret = ne_getmodtime(dav, p.c_str(), &t);
    if (ret != NE_OK){
        LOG(LOG_LEVEL_INFO, L"(%s) - Request failed: %s", 
            stringutils::convert_ansi_to_unicode(_path).c_str(),
            stringutils::convert_ansi_to_unicode(std::string(ne_get_error(dav))).c_str());
        return false;
    }
    return true;
}

bool webdav::get_file(std::string _path, std::ostream& data){
    macho::windows::auto_lock lock(cs);
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    ne_request *req = ne_request_create(dav, "GET", p.c_str());
    data.seekp((size_t)0, std::iostream::beg);
    int ret = _dispatch_to_buffer(req, data, NULL);
    if (ret == NE_OK && ne_get_status(req)->klass != 2) {
        ret = NE_ERROR;
    }
    if (ret != NE_OK){
        LOG(LOG_LEVEL_ERROR, L"(%s) - Request failed: %s", 
            stringutils::convert_ansi_to_unicode(_path).c_str(), 
            stringutils::convert_ansi_to_unicode(std::string(ne_get_error(dav))).c_str());
    }
    ne_request_destroy(req);
    return NE_OK == ret;
}

int webdav::_dispatch_to_buffer(ne_request *req, std::ostream& data, const char *range){
    ne_session *const sess = ne_get_session(req);
    const ne_status *const st = ne_get_status(req);
    int ret;
    size_t rlen;

    /* length of bytespec after "bytes=" */
    rlen = range ? strlen(range + 6) : 0;

    do {
        const char *value;

        ret = ne_begin_request(req);
        if (ret != NE_OK) break;
        int32_t content_length = 0;
        value = ne_get_response_header(req, "Content-Length");
        if (value)
            content_length = strtol(value, NULL, 10);

        value = ne_get_response_header(req, "Content-Range");

        /* For a 206 response, check that a Content-Range header is
        * given which matches the Range request header. */
        if (range && st->code == 206
            && (value == NULL || strncmp(value, "bytes ", 6) != 0
            || strncmp(range + 6, value + 6, rlen)
            || (range[5 + rlen] != '-' && value[6 + rlen] != '/'))) {
            ne_set_error(sess, "Response did not include requested range");
            return NE_ERROR;
        }

        if ((range && st->code == 206) || (!range && st->klass == 2)) {
            ret = _read_response_to_buffer(req, data, content_length);
        }
        else {
            ret = ne_discard_response(req);
        }

        if (ret == NE_OK) ret = ne_end_request(req);
    } while (ret == NE_RETRY);

    return ret;
}

int webdav::_read_response_to_buffer(ne_request *req, std::ostream& data, int32_t length){
    char buf[8192];
    ssize_t bytesread = 0;
    ssize_t bytes = sizeof(buf);
    ssize_t len;
    while ((length == 0 || length > bytesread) && (len = ne_read_response_block(req, buf, bytes)) > 0) {
        data.write(buf, len);
        bytesread += len;
    }
    return (len == 0 || length == bytesread) ? NE_OK : NE_ERROR;
}

bool webdav::enumerate_sub_items(std::string _path, item::vtr& items){
    if (_path.length() && _path[_path.length() - 1] != '/')
        _path.append("/");
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    items.clear();
    macho::windows::auto_lock lock(cs);
    int ret = ne_simple_propfind(dav, p.c_str(), NE_DEPTH_ONE, _props, webdav::_props_result, &items);
    if (ret == NE_OK){
        size_t len = p.length();
        foreach(item::ptr i, items){
            i->name = i->name.substr(len, -1);
            if (i->name.length() && i->is_dir){
                i->name.erase(i->name.length() - 1);
            }
        }
        if (items.size() && !items[0]->name.length()){
            items.erase(items.begin());
        }
        return true;
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Request failed: %s", stringutils::convert_ansi_to_unicode(std::string(ne_get_error(dav))).c_str());
    }
    return false;
}

webdav::item::vtr webdav::enumerate_sub_items(std::string _path){
    item::vtr items;
    enumerate_sub_items(_path, items);
    return items;
}

bool webdav::put_file(std::string _path, std::istream& data){
    macho::windows::auto_lock lock(cs);
    ne_request *req;
    std::string p = boost::str(boost::format("%s/%s") % nuri.path %_path);
    req = ne_request_create(dav, "PUT", p.c_str());
    ne_lock_using_resource(req, p.c_str(), 0);
    ne_lock_using_parent(req, p.c_str());
    data.seekg((size_t)0, std::iostream::end);
    size_t data_size = data.tellg();
    data.seekg((size_t)0, std::iostream::beg);
    put_data put( data,data_size);
    ne_set_request_body_provider(req, data_size, _provide_body, &put);
    int ret = ne_request_dispatch(req);
    if (ret == NE_OK && ne_get_status(req)->klass != 2)
        ret = NE_ERROR;
    ne_request_destroy(req);
    if (ret != NE_OK){
        LOG(LOG_LEVEL_ERROR, L"(%s) - Request failed: %s", 
            stringutils::convert_ansi_to_unicode(_path).c_str(),
            stringutils::convert_ansi_to_unicode(std::string(ne_get_error(dav))).c_str());
        return false;
    }
    return true;
}

bool webdav::create_empty_file(std::string _path){
    std::stringstream data;
    return put_file(_path, data);
}