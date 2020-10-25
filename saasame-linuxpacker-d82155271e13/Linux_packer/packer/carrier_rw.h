#pragma once
#ifndef __CARRIER_RW__
#define __CARRIER_RW__

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/async/TEvhttpClientChannel.h>
#include <thrift/async/TAsyncChannel.h>
#include <thrift/stdcxx.h>
#include <thrift/transport/TSSLSocket.h>
#include <set>
#include "saasame_types.h"
#include "saasame_constants.h"
#include "carrier_service.h"
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSSLSocket.h>
#include <boost/algorithm/string.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "../tools/universal_disk_rw.h"
#include "../tools/system_tools.h"
#include "../tools/exception_base.h"
#include "../tools/thrift_helper.h"



using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace saasame::transport;

#define CARRIER_RW_IMAGE_BLOCK_SIZE    33554432UL

class out_of_queue_exception : public std::exception {
    virtual const char* what() const throw(){
        return "out of queue exception occurred";
    }
};

class http_carrier_op : public TSaasame_helper<saasame::transport::carrier_serviceClient, saasame::transport::service_info> {
public:
    http_carrier_op(std::set<std::string> addrs,std::string addr, int port, bool is_ssl) :TSaasame_helper<saasame::transport::carrier_serviceClient, saasame::transport::service_info> //there should be some error.
        (addrs, addr, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PATH, port, is_ssl) {}
};


class carrier_rw : virtual public universal_disk_rw{
public:
    struct create_image_parameter{
        create_image_parameter() : size(0), block_size(CARRIER_RW_IMAGE_BLOCK_SIZE), timeout(600), compressed(true), checksum(false), encrypted(false), checksum_verify(true){}
        std::map<std::string, std::set<std::string>> carriers;
        std::map<std::string, std::string> priority_carrier;
        std::string base_name;
        std::string name;
        uint64_t size;
        uint32_t block_size;
        bool compressed;
        bool checksum;
        std::string parent;
        bool checksum_verify;
        std::string session;
        uint32_t timeout;
        std::string comment;
        bool        encrypted;
        bool        polling;
        uint8_t     mode;
    public:
        void printf();
    };

    struct open_image_parameter{
        open_image_parameter() :  timeout(600), encrypted(false){}
        std::map<std::string, std::set<std::string>> carriers;
        std::map<std::string, std::string> priority_carrier;
        std::string base_name;
        std::string name;
        std::string session;
        uint32_t timeout;
        bool encrypted;
        bool        polling;
        uint8_t     mode;
        void printf();
    };

    virtual ~carrier_rw(){ /*close();*/ }
    static universal_disk_rw::vtr open(const open_image_parameter & parameter); // This encrypted flag is only used for packer to carrier.
    static universal_disk_rw::vtr open(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, std::string session = system_tools::gen_random_uuid(), uint32_t timeout = 600, bool encrypted = false, bool polling = false); // This encrypted flag is only used for packer to carrier.
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, std::string parent = "", bool checksum_verify = true, std::string session = system_tools::gen_random_uuid(), uint32_t timeout = 600, bool encrypted = false, std::string comment = "", bool polling = false);
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, std::string comment, bool polling = false, uint8_t mode = 2);
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment);
    static universal_disk_rw::vtr create(const create_image_parameter & parameter);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written);
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual bool write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written);
    virtual bool write_ex(__in uint64_t start, __in const void *buffer, __in uint32_t compressed_byte , __in uint32_t number_of_bytes_to_write, __in bool compressed, __inout uint32_t& number_of_bytes_written);
    virtual bool is_buffer_free();

    virtual std::string path() const;

    static std::string get_machine_id(std::set<std::string > carriers);
    static bool remove_snapshost_image(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint32_t timeout, bool encrypted, bool polling);
    static bool set_session_buffer(std::map<std::string, std::set<std::string>> carriers, std::string session, uint32_t buffer_size,  uint32_t timeout, bool encrypted, bool polling);

    bool close(const bool is_cancle = false);
    bool is_ssl_link()
    {
        for(carrier_op::ptr &c : _carriers)
        {
            if (c->ssl_link)
                return true;
        }
        return false;
    }
    static bool remove_connections(std::map<std::string, std::set<std::string>> carriers, uint32_t timeout, bool encrypted,bool polling);
    string      ex_string;
private:
    bool remove_snapshost_image();
    struct carrier_op {
        typedef  boost::shared_ptr<carrier_op> ptr;
        typedef  std::vector<ptr> vtr;

        carrier_op(std::string _addr, uint32_t _timeout, bool encrypted, bool polling);
        virtual ~carrier_op() { disconnect(); }
        bool connect();
        bool disconnect();
        std::shared_ptr<AccessManager>                              accessManager;
        std::string                                                   addr;
        std::shared_ptr<TSocket>                                    socket;
        std::shared_ptr<TSSLSocket>                                 ssl_socket;
        std::shared_ptr<TTransport>                                 transport;
        std::shared_ptr<TProtocol>                                  protocol;
        std::shared_ptr<TSSLSocketFactory>                          factory;
        std::shared_ptr<saasame::transport::carrier_serviceClient>  client;
        std::shared_ptr<boost::recursive_mutex>                       scs;
        boost::recursive_mutex           				              cs;
        bool                                                          ssl_link;
        bool                                                          initialed;
        http_carrier_op::ptr                                        proxy;
        //TSaasame_helper<saasame::transport::reverse_transportClient, saasame::transport::service_info>::ptr       proxy;        
    };

    carrier_rw(std::string session, std::set<std::string> connection_ids, std::set<std::string>  carriers, std::string name, std::string base_name, uint32_t timeout, bool encrypted, bool polling = false, uint8_t mode = 2) :
        _connection_ids(connection_ids), _base_name(base_name), _name(name), _session(session), _polling(polling), _mode(mode), _wname(name), _timeout(timeout), _encrypted(encrypted)
    {
        for(std::string c : carriers){
            LOG_TRACE("c = %s\r\n", c.c_str());
            carrier_op::ptr cop = carrier_op::ptr(new carrier_op(c, timeout, encrypted, _polling));
            if(cop->connect())
                _carriers.push_back(cop);
        }
    } 
    bool open();
    bool create(uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string comment);
    bool create(uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string comment);

    std::string              _wname;
    std::string              _name;
    std::string              _base_name;
    std::string              _session;
    std::string              _image_id;
    uint32_t                 _timeout;
    bool                     _encrypted;
    bool                     _polling;
    uint8_t                  _mode;
    std::set<std::string>    _connection_ids;
    carrier_op::vtr          _carriers;
    carrier_op::ptr          _op;
public:
    carrier_rw(carrier_rw& rw);
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone() {
        return universal_disk_rw::ptr(new carrier_rw(*this));
    }
};

#endif