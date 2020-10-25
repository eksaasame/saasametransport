//#include "macho.h"
#include "carrier_rw.h"
#include "carrier_service.h"
#include "../tools/service_status.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


//using namespace macho::windows;
//using namespace macho;
using namespace mumi;
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
    typedef std::shared_ptr<connection_id_map> ptr;
    typedef std::vector<ptr> vtr;
    static connection_id_map::vtr get(std::map<std::string, std::set<std::string>> carriers){
        connection_id_map::vtr id_vector;
        for (string_set_map::value_type &c : carriers){
            std::set<std::string> set_less;
            for (connection_id_map::ptr &p : id_vector){
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
    //FUNC_TRACER;
    bool result = false;
    //FUN_TRACE;
    //LOG_TRACE("addr = %s", addr.c_str());
    //if (initialed){
    //    result = ssl_link || transport->isOpen();
    //}
    //else
    if (proxy) {
        //LOG_TRACE("GO+PROXY")
        return proxy->connect();
    }
    else if(transport)
    {
        //LOG_TRACE("GO+NORMAL")
        int retry_count = 1;
        while (true){
            try{
                if (!transport->isOpen()){
                    /*struct addrinfo hints, *res, *res0;
                    char port[sizeof("65536")];
                    sprintf(port, "%d", saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
                    LOG_TRACE("3.1");
                    memset(&hints, 0, sizeof(hints));
                    hints.ai_family = PF_UNSPEC;
                    hints.ai_socktype = SOCK_STREAM;
                    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
                    LOG_TRACE("addr.c_str() = %s\r\n", addr.c_str());
                    LOG_TRACE("port.c_str() = %s\r\n", port);
                    int error = getaddrinfo(addr.c_str(), port, &hints, &res0);
                    LOG_TRACE("error = %d", error); */

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
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                disconnect();
            }
            catch (...){
                LOG_ERROR("others error");
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
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
    if (proxy) {
        return proxy->disconnect();
    }
    else if (transport) {

        int retry_count = 3;
        while (retry_count > 0) {
            try {
                if (transport->isOpen())
                    transport->close();
                result = !transport->isOpen();
                break;
            }
            catch (saasame::transport::invalid_operation& ex) {
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex) {
                LOG_ERROR("%s", ex.what());
            }
            catch (...) {
            }
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            retry_count--;
        }
    }
    return result;
}

std::string carrier_rw::path() const{
    return _wname;
}

bool carrier_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
    bool result = false;
    FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while (!_op->proxy){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(_op->cs);
                if (_op->connect()){
                    _op->client->read(output, _session, _image_id, start, number_of_bytes_to_read);
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                _op->disconnect();
            }
            catch (...){
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
    }
    else{
        for(carrier_op::ptr c : _carriers){
            try{
                if (!c->proxy){
                    boost::unique_lock<boost::recursive_mutex> lock(c->cs);
                    if (c->connect()) {
                        c->client->read(output, _session, _image_id, start, number_of_bytes_to_read);
                        _op = c;
                        result = true;
                        break;
                    }
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
            }
            catch (...){
            }
        }
    }
    return result;
}

bool carrier_rw::write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written) {
    bool result = false;
    //FUN_TRACE;
    if (_op) {
        int32_t retry_count = 3;
        while (true) {
            try {
                boost::unique_lock<boost::recursive_mutex> lock(_op->cs);
                if (_op->connect()) {
                    if (_op->proxy)
                        number_of_bytes_written = _op->proxy->client()->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    else
                        number_of_bytes_written = _op->client->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex) {
                LOG_ERROR("%s", ex.why.c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex) {
                LOG_ERROR("%s", ex.what());
                _op->disconnect();
            }
            catch (...) {
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
    }
    else {
        for(carrier_op::ptr c : _carriers) {
            try {
                boost::unique_lock<boost::recursive_mutex> lock(c->cs);
                if (c->connect()) {
                    if (c->proxy)
                        number_of_bytes_written = c->proxy->client()->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    else
                        number_of_bytes_written = c->client->write_ex(_session, _image_id, start, buf, original_number_of_bytes, compressed);
                    _op = c;
                    result = true;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex) {
                LOG_ERROR("%s", ex.why.c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex) {
                LOG_ERROR("%s", ex.what());
                c->disconnect();
            }
            catch (...) {
            }
        }
    }
    return result;
}

bool carrier_rw::write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
    bool result = false;
    //FUN_TRACE;
    if (_op){
        int32_t retry_count = 3;
        while (true){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(_op->cs);
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
                LOG_ERROR("%s", ex.why.c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                _op->disconnect();
            }
            catch (...){
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
    }
    else{
        for(carrier_op::ptr c : _carriers){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(c->cs);
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
                LOG_ERROR("%s", ex.why.c_str());
                if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                    throw out_of_queue_exception();
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                c->disconnect();
            }
            catch (...){
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
                boost::unique_lock<boost::recursive_mutex> lock(_op->cs);
                if (_op->connect()){
                    if (_op->proxy)
                        result = _op->proxy->client()->is_buffer_free(_session, _image_id);
                    else
                        result = _op->client->is_buffer_free(_session, _image_id);
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                _op->disconnect();
            }
            catch (...){
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
    }
    else{
        for(carrier_op::ptr c : _carriers){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(c->cs);
                if (c->connect()){
                    if(c->proxy)
                        result = c->proxy->client()->is_buffer_free(_session, _image_id);
                    else
                        result = c->client->is_buffer_free(_session, _image_id);
                    _op = c;
                    break;
                }
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                c->disconnect();
            }
            catch (...){
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

bool carrier_rw::write_ex(__in uint64_t start, __in const void *buffer, __in uint32_t compressed_byte, __in uint32_t number_of_bytes_to_write, __in bool compressed, __inout uint32_t& number_of_bytes_written) {
    std::string buf(reinterpret_cast<char const*>(buffer), compressed_byte);
    //LOG_TRACE("buf.size() = %llu, buf.length() = %llu", buf.size(), buf.length());
    return write_ex(start, buf, number_of_bytes_to_write, compressed, number_of_bytes_written);
}


universal_disk_rw::vtr carrier_rw::open(const carrier_rw::open_image_parameter & parameter){
    FUNC_TRACER;
    universal_disk_rw::vtr _return;
    if (parameter.priority_carrier.size()){
#ifndef string_map
        typedef std::map<std::string, std::string> string_map;
#endif
        std::map<std::string, std::set<std::string>> carriers;
        for(string_map::value_type var : parameter.priority_carrier){
            carriers[var.first].insert(var.second);
        }
        _return = open(carriers, parameter.base_name, parameter.name, parameter.session, parameter.timeout, parameter.encrypted, parameter.polling);
    }
    if(_return.size() == 0)
        _return = open(parameter.carriers, parameter.base_name, parameter.name, parameter.session, parameter.timeout, parameter.encrypted);
    return _return;
}

universal_disk_rw::vtr carrier_rw::open(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, std::string session, uint32_t timeout, bool encrypted, bool polling){
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    for (connection_id_map::ptr &c : connection_ids) {
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted, polling);
        for (auto & ca : c->carriers)
        {
            LOG_TRACE("ca.first = %s\r\n", ca.c_str());
        }

        if (carr->open())
        {
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
            break;
        }
        else{
            delete carr;
        }
    }
    if (carrier_rws.size() == 0)
    {
        BOOST_THROW_RW_EXCEPTION(saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, "Failed to open image.");
    }
    return carrier_rws;
}

universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, 
    bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, std::string comment, bool polling, uint8_t mode){
    FUN_TRACE;
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    for(connection_id_map::ptr &c : connection_ids){
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted, polling, mode);
        for (auto & ca : c->carriers)
        {
            LOG_TRACE("ca.first = %s\r\n", ca.c_str());
        }
        if (carr->create(size, block_size, compressed, checksum, encrypted, parent, checksum_verify, comment))
        {
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
            break;
        }
        else{
            delete carr;
            carr = NULL;
        }
    }
    /*if (carrier_rws.size() == 0)
    {
        BOOST_THROW_RW_EXCEPTION(saasame::transport::error_codes::SAASAME_E_IMAGE_CREATE_FAIL, "Failed to create image.");
    }*/ //now the error is handle already.
    LOG_TRACE("carrier_rws.size() = %d", carrier_rws.size());
    return carrier_rws;
}

universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment){
    universal_disk_rw::vtr carrier_rws;
    connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
    for(connection_id_map::ptr &c : connection_ids){
        for (auto & cc : c->carriers)
            LOG_TRACE("cc->carriers = %s", cc.c_str());
        carrier_rw* carr = new carrier_rw(session, c->ids, c->carriers, name, base_name, timeout, encrypted);
        if (carr->create(size, block_size, parent, checksum_verify, comment))
            carrier_rws.push_back(universal_disk_rw::ptr(carr));
        else {
            delete carr;
        }
        if (carrier_rws.size())
            break;
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
        for(carrier_op::ptr c : _carriers){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(c->cs);
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
                LOG_ERROR("%s", ex.why.c_str());
                ex_string = ex.why;
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                ex_string = string(ex.what());
                c->disconnect();
            }
            catch (...){
            }
        }
    }
    return result;
}

bool carrier_rw::open(){
    bool result = false;
    FUN_TRACE;
    for(carrier_op::ptr c : _carriers){
        try{
            boost::unique_lock<boost::recursive_mutex> lock(c->cs);
            if (c->connect()){
                if(c->proxy)
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
            LOG_ERROR("%s, (Image: %s, Session: %s)", ex.why.c_str(), _name.c_str(), _session.c_str());
        }
        catch (TException &ex){
            LOG_ERROR("%s", ex.what());
            c->disconnect();
        }
        catch (...){
        }
    }
    return result;

}

bool carrier_rw::create(uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string comment){
    FUN_TRACE;
    bool result = false;
    for(carrier_op::ptr c : _carriers){
        try{
            boost::unique_lock<boost::recursive_mutex> lock(c->cs);
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
                info.__set_version(create_image_option::VERSION_1);
                info.__set_mode(_mode);
                if (c->proxy)
                {
                    LOG_TRACE("GO PROXY");
                    c->proxy->client()->create(_image_id, _session, info);
                }
                else
                {
                    LOG_TRACE("GO NORMAL");
                    c->client->create(_image_id, _session, info);
                }
                if (result = _image_id.length() > 0){
                    LOG_TRACE("finish!");
                    _op = c;
                    break;
                }
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG_ERROR("%s", ex.why.c_str());
        }
        catch (TException &ex){        
            LOG_ERROR("%s", ex.what());
            c->disconnect();
        }
        catch (...){
        }
    }
    return result;
}

bool carrier_rw::create(uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string comment){
    bool result = false;
    FUN_TRACE;
    for (carrier_op::ptr c : _carriers){
        try{
            boost::unique_lock<boost::recursive_mutex> lock(c->cs);
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
                info.__set_version(create_image_option::VERSION_2);
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
            LOG_ERROR("%s", ex.why.c_str());
        }
        catch (TException &ex)
        {
            if (std::string(ex.what()).find("Invalid method name:") != std::string::npos)
            {
                LOG_WARNING("%s", ex.what());
                try{
                    boost::unique_lock<boost::recursive_mutex> lock(c->cs);
                    if (c->connect()){
                        c->client->create_ex(_image_id, _session, _connection_ids, _base_name, _name, size, block_size, parent, checksum_verify);
                        if (result = _image_id.length() > 0){
                            _op = c;
                            break;
                        }
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG_ERROR("%s", ex.why.c_str());
                }
                catch (TException &ex){
                    LOG_ERROR("%s", ex.what());
                    c->disconnect();
                }
                catch (...){
                }
            }
            else
            {
                LOG_ERROR("%s", ex.what());
            }
        }
        catch (...){
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
    for (carrier_op::ptr c : _carriers){
        try{
            boost::unique_lock<boost::recursive_mutex> lock(c->cs);
            if (!c->proxy && c->connect()){
                if (result = c->client->remove_snapshot_image(_session, images))
                    break;
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG_ERROR("%s", ex.why.c_str());
        }
        catch (TException &ex){
            LOG_ERROR("%s", ex.what());
            c->disconnect();
        }
        catch (...){
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
        for (std::string c : carriers){
            _carriers.push_back(carrier_op::ptr(new carrier_op(c, 30, true, false)));
        }
        for (carrier_op::ptr c : _carriers){
            try{
                boost::unique_lock<boost::recursive_mutex> lock(c->cs);
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
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (TException &ex){
                LOG_ERROR("%s", ex.what());
                c->disconnect();
            }
            catch (...){
            }
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG_ERROR("%s", ex.why.c_str());
    }
    catch (TException &ex){
        LOG_ERROR("%s", ex.what());
    }
    catch (...){
    }
    return machine_id;
}

bool carrier_rw::remove_snapshost_image(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint32_t timeout, bool encrypted, bool polling){
    bool result = false;
    FUN_TRACE;
    if (name.empty())
        result = true;
    else{
        universal_disk_rw::vtr carrier_rws;
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        for(connection_id_map::ptr &c : connection_ids){
            boost::shared_ptr<carrier_rw> carr = boost::shared_ptr<carrier_rw>(new carrier_rw("", c->ids, c->carriers, name, base_name, timeout, encrypted, polling));
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

bool carrier_rw::remove_connections(std::map<std::string, std::set<std::string>> carriers, uint32_t timeout, bool encrypted, bool polling){
    bool result = false;
    FUN_TRACE;
    try{
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        for (connection_id_map::ptr &c : connection_ids){
            carrier_op::vtr        carrier_ops;
            for (std::string _c : c->carriers){
                carrier_ops.push_back(carrier_op::ptr(new carrier_op(_c, timeout, encrypted, polling)));
            }
            for (carrier_op::ptr op : carrier_ops){
                try{
                    boost::unique_lock<boost::recursive_mutex> lock(op->cs);
                    if (op->connect()){
                        for (std::string id : c->ids){
                            if (!(result = op->client->remove_connection("", id)))
                                break;
                        }
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG_ERROR("%s", ex.why.c_str());
                }
                catch (TException &ex){
                    LOG_ERROR("%s", ex.what());
                    op->disconnect();
                }
                catch (...){
                }
                if (!result)
                    break;
            }
            if (!result)
                break;
        }
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG_ERROR("%s", ex.why.c_str());
    }
    catch (TException &ex){
        LOG_ERROR("%s", ex.what());
    }
    catch (...){
    }
    return result;
}

bool carrier_rw::set_session_buffer(std::map<std::string, std::set<std::string>> carriers, std::string session, uint32_t buffer_size, uint32_t timeout, bool encrypted, bool polling){
    bool result = false;
    FUN_TRACE;
    try{
        
        connection_id_map::vtr connection_ids = connection_id_map::get(carriers);
        for (connection_id_map::ptr &c : connection_ids){
            carrier_op::vtr        carrier_ops;
            for(std::string _c : c->carriers){
                carrier_ops.push_back(carrier_op::ptr(new carrier_op(_c, timeout, encrypted, polling)));
            }
            for (carrier_op::ptr op : carrier_ops){
                try{
                    boost::unique_lock<boost::recursive_mutex> lock(op->cs);
                    if (op->connect()){
                        if (result = op->client->set_buffer_size(session, buffer_size))
                            break;
                    }
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG_ERROR("%s", ex.why.c_str());
                }
                catch (TException &ex){
                    LOG_ERROR("%s", ex.what());
                    op->disconnect();
                }
                catch (...){
                }
            }
            if (!result)
                break;
        }   
    }
    catch (saasame::transport::invalid_operation& ex){
        LOG_ERROR("%s", ex.why.c_str());
    }
    catch (TException &ex){
        LOG_ERROR("%s", ex.what());
    }
    catch (...){
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

    for (carrier_op::ptr &c : rw._carriers){
        _carriers.push_back(carrier_op::ptr(new carrier_op(c->addr, rw._timeout, rw._encrypted, _polling)));
    }
    for (carrier_op::ptr c : _carriers) {
        try {
            boost::unique_lock<boost::recursive_mutex> lock(c->cs);
            if (c->connect()) {
                _op = c;
                break;
            }
        }
        catch (saasame::transport::invalid_operation& ex) {
            LOG_ERROR("%s", ex.why.c_str());
            if (ex.what_op == saasame::transport::error_codes::SAASAME_E_QUEUE_FULL)
                throw out_of_queue_exception();
        }
        catch (TException &ex) {
            LOG_ERROR("%s", ex.what());
        }
        catch (...) {
            LOG_ERROR("other error");
        }
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

carrier_rw::carrier_op::carrier_op(std::string _addr, uint32_t _timeout, bool encrypted, bool polling) : addr(_addr), ssl_link(false), initialed(false)
{
    FUNC_TRACER;
    LOG_TRACE("_addr = %s\r\n", _addr.c_str());
    if (polling) {
        service_status ss;
        int port = 0;
        std::vector<std::string> arr;
        std::string MgmtAddr = ss.getMgmtAddr();
        boost::split(arr, MgmtAddr, boost::is_any_of(":"));
        if (arr.size() == 2) {
            port = atoi(arr[1].c_str());
        }        
        http_carrier_op::ptr p =
            http_carrier_op::ptr(new http_carrier_op({}, _addr, port, true));
        if (p && p->open()) {
            proxy = p;
        }
    }
    else
    {
        boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
        LOG_TRACE("p = %s , encrypted = %d\r\n", p.string().c_str(), encrypted);
        if (encrypted && boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                std::shared_ptr<AccessManager> accessManager(new MyAccessManager());// = /*g_accessManager;*///
                factory = /*g_factory;*/std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
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
            catch (TException& ex) {
                LOG_ERROR("%s", ex.what());
            }
        }
        if (!ssl_socket)
        {
            try
            {
                socket = std::shared_ptr<TSocket>(new TSocket(_addr, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
                socket->setConnTimeout(30 * 1000);
                socket->setSendTimeout(_timeout * 1000);
                socket->setRecvTimeout(_timeout * 1000);
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
            }
            catch (TException& ex) {
                LOG_ERROR("%s", ex.what());
            }
        }
        try
        {
            protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
            client = std::shared_ptr<saasame::transport::carrier_serviceClient>(new saasame::transport::carrier_serviceClient(protocol));
        }
        catch (TException& ex) {
            LOG_ERROR("%s", ex.what());
        }
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
                LOG_ERROR("%s", ex.what());
                if (transport->isOpen())
                    transport->close();
                socket = nullptr;
                transport = nullptr;
                protocol = nullptr;
                client = nullptr;
                LOG_TRACE("haha");
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

//universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment, bool polling)
universal_disk_rw::vtr carrier_rw::create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment, bool polling)
{
    uint32_t block_size = CARRIER_RW_IMAGE_BLOCK_SIZE;
    /*Linux no reg file, now the block size is const in code*/
    /*macho::windows::registry reg;
    if (reg.open(saasame::transport::g_saasame_constants.CONFIG_PATH)){
        if (reg["BlockSize"].exists() && reg["BlockSize"].is_dword()){
            uint32_t _size = (DWORD)reg["BlockSize"];
            if (_size > 0){
                block_size = (((_size - 1) / CARRIER_RW_MINI_BLOCK_SIZE) + 1) * CARRIER_RW_MINI_BLOCK_SIZE;
                if (block_size > (CARRIER_RW_MAX_BLOCK_SIZE * 2))
                    block_size = CARRIER_RW_MAX_BLOCK_SIZE * 2;
            }
        }
    }*/
    return create(carriers, base_name, name, size, block_size, parent, checksum_verify, session, timeout, encrypted, comment);
}

universal_disk_rw::vtr carrier_rw::create(const create_image_parameter& parameter){
    uint32_t block_size = parameter.block_size;
    universal_disk_rw::vtr _return;
    /*Linux no reg file, now the block size is const in code*/

    /*macho::windows::registry reg;
    if (reg.open(saasame::transport::g_saasame_constants.CONFIG_PATH)){
        if (reg["BlockSize"].exists() && reg["BlockSize"].is_dword()){
            uint32_t _size = (DWORD)reg["BlockSize"];
            if (_size > 0){
                block_size = (((_size - 1 ) / CARRIER_RW_MINI_BLOCK_SIZE) + 1 ) * CARRIER_RW_MINI_BLOCK_SIZE;
                if (block_size > (CARRIER_RW_MAX_BLOCK_SIZE * 2))
                    block_size = CARRIER_RW_MAX_BLOCK_SIZE * 2;
            }
        }
    }*/
    if (parameter.priority_carrier.size()){
        std::map<std::string, std::set<std::string>> carriers;
        //carriers = parameter.carriers;
        for (auto & var : parameter.priority_carrier){
            carriers[var.first].insert(var.second);
            LOG_TRACE("priority_carrier.first = %s, priority_carrier.second = %s", var.first.c_str(), var.second.c_str());
        }
        _return = create(carriers, parameter.base_name, parameter.name, parameter.size, block_size, parameter.compressed, parameter.checksum, parameter.encrypted, parameter.parent, parameter.checksum_verify, parameter.session, parameter.timeout, parameter.comment, parameter.polling);
    }
    if(_return.size()==0)
        _return = create(parameter.carriers, parameter.base_name, parameter.name, parameter.size, block_size, parameter.compressed, parameter.checksum, parameter.encrypted, parameter.parent, parameter.checksum_verify, parameter.session, parameter.timeout, parameter.comment, parameter.polling);
    return _return;
}

void carrier_rw::create_image_parameter::printf()
{
    FUN_TRACE;
    for (auto & cs : carriers)
    {
        LOG_TRACE("carrier = %s", cs.first.c_str());
        for (auto & c : cs.second)
        {
            LOG_TRACE("c = %s", c.c_str());
        }
    }
    for (auto & cs : priority_carrier)
    {
        LOG_TRACE("priority_carrier = %s", cs.first.c_str());
        LOG_TRACE("priority_carrier.s = %s", cs.second.c_str());
    }
    LOG_TRACE("base_name = %s", base_name.c_str());
    LOG_TRACE("name = %s", name.c_str());
    LOG_TRACE("size = %llu", size);
    LOG_TRACE("block_size = %llu", block_size);
    LOG_TRACE("compressed = %d", compressed);
    LOG_TRACE("checksum = %d", checksum);
    LOG_TRACE("encrypted = %d", encrypted);
    LOG_TRACE("parent = %s", parent.c_str());
    LOG_TRACE("checksum_verify = %d", checksum_verify);
    LOG_TRACE("session = %s", session.c_str());
    LOG_TRACE("timeout = %u", timeout);
    LOG_TRACE("comment = %s", comment.c_str());
}

void carrier_rw::open_image_parameter::printf()
{
    FUN_TRACE;
    for (auto & cs : carriers)
    {
        LOG_TRACE("carrier = %s", cs.first.c_str());
        for (auto & c : cs.second)
        {
            LOG_TRACE("c = %s", c.c_str());
        }
    }
    for (auto & cs : priority_carrier)
    {
        LOG_TRACE("priority_carrier = %s", cs.first.c_str());
        LOG_TRACE("priority_carrier.s = %s", cs.second.c_str());
    }
    LOG_TRACE("base_name = %s", base_name.c_str());
    LOG_TRACE("name = %s", name.c_str());
    LOG_TRACE("encrypted = %d", encrypted);
    LOG_TRACE("session = %s", session.c_str());
    LOG_TRACE("timeout = %u", timeout);
}