#include "macho.h"
#include "carrier_rw.h"
#include "carrier_service.h"
#include <algorithm>
#include <set>

using namespace macho::windows;
using namespace macho;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

#define CARRIER_RW_MINI_BLOCK_SIZE          8388608UL
#define CARRIER_RW_MAX_BLOCK_SIZE           CARRIER_RW_IMAGE_BLOCK_SIZE

#define IRM_IMAGE_SECTOR_SIZE       512
#define BOOST_THROW_RW_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( universal_disk_rw::exception, no, message )
#define BOOST_THROW_RW_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING( universal_disk_rw::exception, message)
#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

struct connection_id_map{
    typedef boost::shared_ptr<connection_id_map> ptr;
    typedef std::vector<ptr> vtr;
    static connection_id_map::vtr get(std::map<std::string, std::set<std::string>> carriers){
        connection_id_map::vtr id_vector;
        foreach(string_set_map::value_type &c, carriers){
            std::set<std::string> set_less;
            foreach(connection_id_map::ptr &p, id_vector){
                std::set_intersection(c.second.begin(), c.second.end(),
                    p->carriers.begin(), p->carriers.end(),
                    std::inserter(set_less, set_less.begin())
                    );
                if (set_less.size()){
                    p->ids.insert(c.first);
                    if (p->carriers.size() != set_less.size())
                        p->carriers = set_less;
                    break;
                }
            }
            if (!set_less.size()){
                connection_id_map::ptr p = connection_id_map::ptr(new connection_id_map());
                p->ids.insert(c.first);
                p->carriers = c.second;
                id_vector.push_back(p);
            }
        }
        return id_vector;
    }
    std::set<std::string> ids;
    std::set<std::string> carriers;
};

bool carrier_rw::carrier_op::connect(){
    bool result = false;
    FUN_TRACE;
    //if (initialed){
    //    result = ssl_link || transport->isOpen();
    //}
    //else
    if (proxy){
        return proxy->connect();
    }
	else if (transport){
        int retry_count = 1;
		while (true ){
            try{
                if (!transport->isOpen()){
                    transport->open();
                    if ( ssl_link && !initialed ){
                        saasame::transport::service_info svc_info;
                        client->ping(svc_info);
                    }
                }
                result = true;
                break;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            retry_count--;
            if (retry_count > 0){
                LOG(LOG_LEVEL_RECORD, L"Retry (%d)", retry_count);
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
        initialed = true;
    }
    return result;
}

bool carrier_rw::carrier_op::disconnect(){
    bool result = false;
    FUN_TRACE;
    if (proxy){
        return proxy->disconnect();
    }
	else if (transport){
        int retry_count = 3;
		while (retry_count > 0 ){
            try{
                if (transport->isOpen())
                    transport->close();
                result = !transport->isOpen();
                break;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            retry_count--;
        }
    }
    return result;
}

std::wstring carrier_rw::path() const{
    return _wname;
}

bool carrier_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
    bool result = false;
    FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while ( (!_op->proxy) && true){
            try{
                macho::windows::auto_lock lock(_op->cs);
                if (_op->connect()){
                    _op->client->read(output, _session, _image_id, start, number_of_bytes_to_read);
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                _op->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            retry_count--;
            if (retry_count > 0){
                LOG(LOG_LEVEL_RECORD, L"Retry (%d)", retry_count);
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
    }
    else{
        foreach(carrier_op::ptr c, _carriers){
            try{
                if (!c->proxy){
                    macho::windows::auto_lock lock(c->cs);
                    if (c->connect()){
                        c->client->read(output, _session, _image_id, start, number_of_bytes_to_read);
                        _op = c;
                        result = true;
                        break;
                    }
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    return result;
}

bool carrier_rw::write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written){
    bool result = false;
    FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while (true){
            try{
                macho::windows::auto_lock lock(_op->cs);
                if (_op->connect()){
                    if (_op->proxy)
                        number_of_bytes_written = _op->proxy->client()->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    else
                        number_of_bytes_written = _op->client->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                _op->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            retry_count--;
            if (retry_count > 0){
                LOG(LOG_LEVEL_RECORD, L"Retry (%d)", retry_count);
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
    }
    else{
        foreach(carrier_op::ptr c, _carriers){
            try{
                macho::windows::auto_lock lock(c->cs);
                if (c->connect()){
                    if (c->proxy)
                        number_of_bytes_written = c->proxy->client()->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    else
                        number_of_bytes_written = c->client->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    _op = c;
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                c->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    return result;
}

bool carrier_rw::write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
    bool result = false;
    FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while (true){
            try{
                macho::windows::auto_lock lock(_op->cs);
                if (_op->connect()){
                    if (_op->proxy)
                        number_of_bytes_written = _op->proxy->client()->write(_session, _image_id, start, buf, buf.size());
                    else
                        number_of_bytes_written = _op->client->write(_session, _image_id, start, buf, buf.size());
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                _op->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            retry_count--;
            if (retry_count > 0){
                LOG(LOG_LEVEL_RECORD, L"Retry (%d)", retry_count);
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
    }
    else{
        foreach(carrier_op::ptr c, _carriers){
            try{
                macho::windows::auto_lock lock(c->cs);
                if (c->connect()){
                    if (c->proxy)
                        number_of_bytes_written = c->proxy->client()->write(_session, _image_id, start, buf, buf.size());
                    else
                        number_of_bytes_written = c->client->write(_session, _image_id, start, buf, buf.size());
                    _op = c;
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                c->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    return result;
}

bool carrier_rw::is_buffer_free(){

    bool result = true;
    FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while (true){
            try{
                macho::windows::auto_lock lock(_op->cs);
                if (_op->connect()){
                    if (_op->proxy)
                        result = _op->proxy->client()->is_buffer_free(_session, _image_id);
                    else
                        result = _op->client->is_buffer_free(_session, _image_id);
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                _op->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
            retry_count--;
            if (retry_count > 0){
                LOG(LOG_LEVEL_RECORD, L"Retry (%d)", retry_count);
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
            else
                break;
        }
    }
    else{
        foreach(carrier_op::ptr c, _carriers){
            try{
                macho::windows::auto_lock lock(c->cs);
                if (c->connect()){
                    if (_op->proxy)
                        result = c->proxy->client()->is_buffer_free(_session, _image_id);
                    else
                        result = c->client->is_buffer_free(_session, _image_id);
                    _op = c;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                c->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    return result;
}

bool carrier_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read){
    std::string buf;
    bool result = read(start, number_of_bytes_to_read, buf);
    if (result){
        number_of_bytes_read = buf.size();
        memcpy(buffer, buf.c_str(), buf.size());
    }
    return result;
}

bool carrier_rw::write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
    std::string buf(reinterpret_cast<char const*>(buffer), number_of_bytes_to_write);
    return write(start, buf, number_of_bytes_written);
}

universal_disk_rw::vtr carrier_rw::open(const carrier_rw::open_image_parameter & parameter){
    if (parameter.priority_carrier.size()){
#ifndef string_map
        typedef std::map<std::string, std::string> string_map;
#endif
        try{
            std::map<std::string, std::set<std::string>> carriers;
            foreach(string_map::value_type var, parameter.priority_carrier){
                carriers[var.first].insert(var.second);
            }
            return open(carriers, parameter.base_name, parameter.name, parameter.session, parameter.timeout, parameter.encrypted, parameter.polling);
        }
        catch (...){
        }
    }
    return open(parameter.carriers, parameter.base_name, parameter.name, parameter.session, parameter.timeout, parameter.encrypted, parameter.polling);
}

universal_disk_rw::vtr carrier_rw::open(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, std::string session, uint32_t timeout, bool encrypted, bool polling){
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    foreach(connection_id_map::ptr &c, connection_ids){
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted, polling);
        if (carr->open())
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
        else{
            delete carr;
            BOOST_THROW_RW_EXCEPTION(saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, L"Failed to open image.");
        }
    }
    return carrier_rws;
}

universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, std::string comment, bool polling, bool cdr, uint8_t mode){
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    foreach(connection_id_map::ptr &c, connection_ids){
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted, polling, mode);
        if (carr->create(size, block_size, compressed, checksum, encrypted, parent, checksum_verify, comment, cdr))
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
        else{
            delete carr;
            BOOST_THROW_RW_EXCEPTION(saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, L"Failed to create image.");
        }
    }
    return carrier_rws;
}

universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment, bool polling, bool cdr, uint8_t mode){
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    foreach(connection_id_map::ptr &c, connection_ids){
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted, polling, mode);
        if (carr->create(size, block_size, parent, checksum_verify, comment, cdr))
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
        else{
            delete carr;
            BOOST_THROW_RW_EXCEPTION(saasame::transport::error_codes::SAASAME_E_IMAGE_OPEN_FAIL, L"Failed to open image.");
        }
    }
    return carrier_rws;
}

bool carrier_rw::close(const bool is_cancle){
    bool result = false;
    FUN_TRACE;
    if (!_image_id.length()){
        result = true;
    }
    else{
        foreach(carrier_op::ptr c, _carriers){
            try{
                macho::windows::auto_lock lock(c->cs);
                if (c->connect()){
                    if (c->proxy)
                        result = c->proxy->client()->close(_session, _image_id, is_cancle);
                    else
                        result = c->client->close(_session, _image_id, is_cancle);

                    if (result)
                        break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                c->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    return result;
}

bool carrier_rw::open(){
    bool result = false;
    FUN_TRACE;
    foreach(carrier_op::ptr c, _carriers){
        try{
            macho::windows::auto_lock lock(c->cs);
            if (c->connect()){
                if (c->proxy)
                    c->proxy->client()->open(_image_id, _session, _connection_ids, _base_name, _name);
                else
                    c->client->open(_image_id, _session, _connection_ids, _base_name, _name);
                if (result = _image_id.length() > 0){
                    _op = c;
                    break;
                }
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG(LOG_LEVEL_ERROR, L"%s, (Image: %s, Session: %s)", stringutils::convert_ansi_to_unicode(ex.why).c_str(), stringutils::convert_ansi_to_unicode(_name).c_str(), stringutils::convert_ansi_to_unicode(_session).c_str());
        }
        catch (TException &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            c->disconnect();
        }
        catch (std::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
        }
    }
    return result;

}

bool carrier_rw::create(uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string comment, bool cdr){
    bool result = false;
    FUN_TRACE;
    foreach(carrier_op::ptr c, _carriers){
        try{
            macho::windows::auto_lock lock(c->cs);
            if (c->connect()){
                create_image_info info;
                info.__set_base(_base_name);
                info.__set_block_size(block_size);
                info.__set_checksum(checksum);
                info.__set_checksum_verify(checksum_verify);
                info.__set_comment(comment);
                info.__set_compressed(compressed);
                info.__set_connection_ids(_connection_ids);
                info.__set_name(_name);
                info.__set_parent(parent);
                info.__set_size(size);
                info.__set_cdr(cdr);
                info.__set_version(create_image_option::VERSION_1);
                info.__set_mode(_mode);
                if (c->proxy)
                    c->proxy->client()->create(_image_id, _session, info);
                else
                    c->client->create(_image_id, _session, info);
                if (result = _image_id.length() > 0){
                    _op = c;
                    break;
                }
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (TException &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            c->disconnect();
        }
        catch (std::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
        }
    }
    return result;
}

bool carrier_rw::create(uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string comment, bool cdr){
    bool result = false;
    FUN_TRACE;
    foreach(carrier_op::ptr c, _carriers){
        try{
            macho::windows::auto_lock lock(c->cs);
            if (c->connect()){
                create_image_info info;
                info.__set_base(_base_name);
                info.__set_block_size(block_size);
                info.__set_checksum_verify(checksum_verify);
                info.__set_comment(comment);
                info.__set_connection_ids(_connection_ids);
                info.__set_name(_name);
                info.__set_parent(parent);
                info.__set_size(size);
                info.__set_cdr(cdr);
                info.__set_version(create_image_option::VERSION_2);
                info.__set_mode(_mode);
                if (c->proxy)
                    c->proxy->client()->create(_image_id, _session, info);
                else
                    c->client->create(_image_id, _session, info);
                if (result = _image_id.length() > 0){
                    _op = c;
                    break;
                }
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (TException &ex)
        {
            if (std::string(ex.what()).find("Invalid method name:") != std::string::npos)
            {
                LOG(LOG_LEVEL_WARNING, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                try{
                    macho::windows::auto_lock lock(c->cs);
                    if (c->connect()){
                        c->client->create_ex(_image_id, _session, _connection_ids, _base_name, _name, size, block_size, parent, checksum_verify);
                        if (result = _image_id.length() > 0){
                            _op = c;
                            break;
                        }
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    c->disconnect();
                }
                catch (std::exception &ex){
                    LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
                }
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
        }
        catch (std::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
        }
    }
    return result;
}

bool carrier_rw::remove_snapshost_image(){
    bool result = false;
    FUN_TRACE;
    std::map<std::string,image_map_info> images;
    image_map_info image;
    image.base_image = _base_name;
    image.image = _name;
    image.connection_ids = _connection_ids;
    image.__set_connection_ids(_connection_ids);
    images[_name] = (image);
    foreach(carrier_op::ptr c, _carriers){
        try{
            macho::windows::auto_lock lock(c->cs);
            if (!c->proxy && c->connect()){
                if (result = c->client->remove_snapshot_image(_session, images))
                    break;
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (TException &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            c->disconnect();
        }
        catch (std::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
        }
    }
    return result;
}

std::string carrier_rw::get_machine_id(std::set<std::string > carriers){
    bool result = false;
    FUN_TRACE;
    std::string machine_id;
    carrier_op::vtr          _carriers;
    try{
        foreach(std::string c, carriers){
            _carriers.push_back(carrier_op::ptr(new carrier_op(c, 30, true, false)));
        }
        foreach(carrier_op::ptr c, _carriers){
            try{
                macho::windows::auto_lock lock(c->cs);
                if (c->connect()){
                    physical_machine_info machine_info;
                    c->client->get_host_detail(machine_info, "", machine_detail_filter::SIMPLE);
                    if (machine_info.machine_id.length()){
                        machine_id = machine_info.machine_id;
                        break;
                    }
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                c->disconnect();
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return machine_id;
}

bool carrier_rw::remove_snapshost_image(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint32_t timeout, bool encrypted){
    bool result = false;
    FUN_TRACE;
    if (name.empty())
        result = true;
    else{
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        foreach(connection_id_map::ptr &c, connection_ids){
            boost::shared_ptr<carrier_rw> carr = boost::shared_ptr<carrier_rw>(new carrier_rw("", c->ids, c->carriers, name, base_name, timeout, encrypted, false));
            if (carr){
                if (!(result = carr->remove_snapshost_image()))
                    break;
            }
            else{
                result = false;
            }
        }
    }
    return result;
}

bool carrier_rw::remove_connections(std::map<std::string, std::set<std::string>> carriers, uint32_t timeout, bool encrypted){
    bool result = false;
    FUN_TRACE;
    try{
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        foreach(connection_id_map::ptr &c, connection_ids){
            carrier_op::vtr        carrier_ops;
            foreach(std::string _c, c->carriers){
                carrier_ops.push_back(carrier_op::ptr(new carrier_op(_c, timeout, encrypted, false)));
            }
            foreach(carrier_op::ptr op, carrier_ops){
                try{
                    macho::windows::auto_lock lock(op->cs);
                    if (op->connect()){
                        foreach(std::string id, c->ids){
                            saasame::transport::connection conn;
                            bool found = false;
                            try{
                                op->client->get_connection(conn, "", id);
                                found = true;
                            }
                            catch (...){
                            }
                            if (found && conn.type != saasame::transport::connection_type::LOCAL_FOLDER){
                                if (!(result = op->client->remove_connection("", id)))
                                    break;
                                else{
                                    LOG(LOG_LEVEL_WARNING, L"Remove unused connection %s", macho::stringutils::convert_ansi_to_unicode(id).c_str());
                                }
                            }
                        }
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    op->disconnect();
                }
                catch (std::exception &ex){
                    LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
                }
                if (!result)
                    break;
            }
            if (!result)
                break;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return result;
}

bool carrier_rw::set_session_buffer(std::map<std::string, std::set<std::string>> carriers, std::string session, uint32_t buffer_size, uint32_t timeout, bool encrypted){
    bool result = false;
    FUN_TRACE;
    try{
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        foreach(connection_id_map::ptr &c, connection_ids){
            carrier_op::vtr        carrier_ops;
            foreach(std::string _c, c->carriers){
                carrier_ops.push_back(carrier_op::ptr(new carrier_op(_c, timeout, encrypted, false)));
            }
            foreach(carrier_op::ptr op, carrier_ops){
                try{
                    macho::windows::auto_lock lock(op->cs);
                    if (op->connect()){
                        if (result = op->client->set_buffer_size(session, buffer_size))
                            break;
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                    op->disconnect();
                }
                catch (std::exception &ex){
                    LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
                }
            }
            if (!result)
                break;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
    }
    catch (TException &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }

    return result;
}

carrier_rw::carrier_rw(carrier_rw& rw){
    _wname = rw._wname;
    _connection_ids = rw._connection_ids;
    _base_name = rw._base_name;
    _name = rw._name;
    _session = rw._session;
    _image_id = rw._image_id;
    _polling = rw._polling;
    _mode = rw._mode;
    foreach(carrier_op::ptr &c, rw._carriers){
        _carriers.push_back(carrier_op::ptr(new carrier_op(c->addr, rw._timeout, rw._encrypted, _polling)));
    }
}

bool carrier_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    uint32_t number_of_bytes_read = 0;
    number_of_sectors_read = 0;
    if (read(start_sector * IRM_IMAGE_SECTOR_SIZE, number_of_sectors_to_read * IRM_IMAGE_SECTOR_SIZE, buffer, number_of_bytes_read)){
        number_of_sectors_read = number_of_bytes_read / IRM_IMAGE_SECTOR_SIZE;
        return true;
    }
    return false;
}

bool carrier_rw::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
    uint32_t number_of_bytes_written = 0;
    number_of_sectors_written = 0;
    if (write(start_sector * IRM_IMAGE_SECTOR_SIZE, buffer, number_of_sectors_to_write * IRM_IMAGE_SECTOR_SIZE, number_of_bytes_written)){
        number_of_sectors_written = number_of_bytes_written / IRM_IMAGE_SECTOR_SIZE;
        return true;
    }
    return false;
}

carrier_rw::carrier_op::carrier_op(std::string _addr, uint32_t _timeout, bool encrypted, bool pooling) : addr(_addr), ssl_link(false), initialed(false)
{
    macho::windows::registry reg;
    if (pooling){
        int port = 0;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MgmtAddr"].exists() && reg[L"MgmtAddr"].is_string()){
                std::vector<std::string> arr = macho::stringutils::tokenize2(reg[L"MgmtAddr"].string(), ":", 0, false);
                if (arr.size() == 2){
                    port = atoi(arr[1].c_str());
                }
            }
        }
		http_carrier_op::ptr p =
            http_carrier_op::ptr(new http_carrier_op({}, addr, port, true));
        if (p && p->open()){
            proxy = p;
        }
    }
    else{
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
        }

        if (encrypted && boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
                factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                factory->authenticate(false);
                factory->loadCertificate((p / "server.crt").string().c_str());
                factory->loadPrivateKey((p / "server.key").string().c_str());
                factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                factory->access(accessManager);

                ssl_socket = std::shared_ptr<TSSLSocket>(factory->createSocket(_addr, saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT));
                ssl_socket->setConnTimeout(30 * 1000);
                ssl_socket->setSendTimeout(_timeout * 1000);
                ssl_socket->setRecvTimeout(_timeout * 1000);
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
            }
            catch (TException& ex) {}
        }

        if (!ssl_socket)
        {
            socket = std::shared_ptr<TSocket>(new TSocket(_addr, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
            socket->setConnTimeout(30 * 1000);
            socket->setSendTimeout(_timeout * 1000);
            socket->setRecvTimeout(_timeout * 1000);
            transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
        }

        protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = std::shared_ptr<saasame::transport::carrier_serviceClient>(new saasame::transport::carrier_serviceClient(protocol));

        //test service via ssl link
        if (ssl_socket)
        {
            saasame::transport::service_info svc_info;

            try
            {
                transport->open();
                client->ping(svc_info);
                ssl_link = true;
            }
            catch (TException& ex)
            {
                if (transport->isOpen())
                    transport->close();
                socket = nullptr;
                transport = nullptr;
                protocol = nullptr;
                client = nullptr;

                //switch to not secure link
                socket = std::shared_ptr<TSocket>(new TSocket(_addr, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
                socket->setConnTimeout(30 * 1000);
                socket->setSendTimeout(_timeout * 1000);
                socket->setRecvTimeout(_timeout * 1000);
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
                protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = std::shared_ptr<saasame::transport::carrier_serviceClient>(new saasame::transport::carrier_serviceClient(protocol));
            }
        }
    }
}

universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment, bool polling, bool cdr, uint8_t mode)
{
    uint32_t block_size = CARRIER_RW_IMAGE_BLOCK_SIZE;
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"BlockSize"].exists() && reg[L"BlockSize"].is_dword()){
            uint32_t _size = (DWORD)reg[L"BlockSize"];
            if (_size > 0){
                block_size = (((_size - 1) / CARRIER_RW_MINI_BLOCK_SIZE) + 1) * CARRIER_RW_MINI_BLOCK_SIZE;
                if (block_size > (CARRIER_RW_MAX_BLOCK_SIZE * 2))
                    block_size = CARRIER_RW_MAX_BLOCK_SIZE * 2;
            }
        }
    }
    return create(carriers, base_name, name, size, block_size, parent, checksum_verify, session, timeout, encrypted, comment, polling, cdr, mode);
}

universal_disk_rw::vtr carrier_rw::create(const create_image_parameter& parameter){
    uint32_t block_size = parameter.block_size;
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"BlockSize"].exists() && reg[L"BlockSize"].is_dword()){
            uint32_t _size = (DWORD)reg[L"BlockSize"];
            if (_size > 0){
                block_size = (((_size - 1 ) / CARRIER_RW_MINI_BLOCK_SIZE) + 1 ) * CARRIER_RW_MINI_BLOCK_SIZE;
                if (block_size > (CARRIER_RW_MAX_BLOCK_SIZE * 2))
                    block_size = CARRIER_RW_MAX_BLOCK_SIZE * 2;
            }
        }
    }
    if (parameter.priority_carrier.size()){
#ifndef string_map
        typedef std::map<std::string, std::string> string_map;
#endif
        try{
            std::map<std::string, std::set<std::string>> carriers;
            foreach(string_map::value_type var, parameter.priority_carrier){
                carriers[var.first].insert(var.second);
            }
            return create(carriers, parameter.base_name, parameter.name, parameter.size, block_size, parameter.compressed, parameter.checksum, parameter.encrypted, parameter.parent, parameter.checksum_verify, parameter.session, parameter.timeout, parameter.comment, parameter.polling, parameter.cdr, parameter.mode);
        }     
        catch (...){
        }
    }
    return create(parameter.carriers, parameter.base_name, parameter.name, parameter.size, block_size, parameter.compressed, parameter.checksum, parameter.encrypted, parameter.parent, parameter.checksum_verify, parameter.session, parameter.timeout, parameter.comment, parameter.polling, parameter.cdr, parameter.mode);
}