#pragma once
#ifndef __CARRIER_RW__
#define __CARRIER_RW__

#include "universal_disk_rw.h"
#include <set>
#include "saasame_types.h"
#include "saasame_constants.h"
#include "carrier_service.h"
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSSLSocket.h>
#include <boost/algorithm/string.hpp>
#include "mgmt_op.h"

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

class http_carrier_op : public mgmt_op_base<saasame::transport::carrier_serviceClient>{
public:
    http_carrier_op(std::set<std::string> addrs, std::string& prefer, int port, bool is_ssl) :mgmt_op_base<saasame::transport::carrier_serviceClient>(addrs, prefer, port, is_ssl){
        _uri = saasame::transport::g_saasame_constants.CARRIER_SERVICE_PATH;
    }
};

class carrier_rw : virtual public universal_disk_rw{
public:
    struct create_image_parameter{
        create_image_parameter() : size(0), block_size(CARRIER_RW_IMAGE_BLOCK_SIZE), timeout(600), compressed(true), checksum(false), encrypted(false), checksum_verify(true), polling(false), cdr(false), mode(2){}
        std::map<std::string, std::set<std::string>> carriers;
        std::map<std::string, std::string> priority_carrier;
        std::string base_name;
        std::string name;
        uint64_t    size;
        uint32_t    block_size;
        bool        compressed;
        bool        checksum;
        bool        encrypted;
        std::string parent;
        bool        checksum_verify;
        std::string session;
        uint32_t    timeout;
        std::string comment;
        bool        polling;
        bool        cdr;
        uint8_t     mode;
    };

    struct open_image_parameter{
        open_image_parameter() : timeout(600), encrypted(false), polling(false){}
        std::map<std::string, std::set<std::string>> carriers;
        std::map<std::string, std::string> priority_carrier;
        std::string base_name;
        std::string name;
        std::string session;
        uint32_t timeout;
        bool encrypted;
        bool polling;
    };

    virtual ~carrier_rw(){ /*close();*/ }
    
    static universal_disk_rw::vtr open(const open_image_parameter & parameter); // This encrypted flag is only used for packer to carrier.
    static universal_disk_rw::vtr open(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, std::string session = macho::guid_::create(), uint32_t timeout = 600, bool encrypted = false, bool polling = false); // This encrypted flag is only used for packer to carrier.
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, std::string parent = "", bool checksum_verify = true, std::string session = macho::guid_::create(), uint32_t timeout = 600, bool encrypted = false, std::string comment = "", bool polling = false, bool cdr = false, uint8_t mode = 2);
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, std::string comment, bool polling = false, bool cdr = false, uint8_t mode = 2);
    static universal_disk_rw::vtr create(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string session, uint32_t timeout, bool encrypted, std::string comment, bool polling = false, bool cdr = false, uint8_t mode = 2);
    static universal_disk_rw::vtr create(const create_image_parameter & parameter);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written);
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual bool write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written);
    virtual bool is_buffer_free();

    virtual std::wstring path() const;

    static std::string get_machine_id(std::set<std::string > carriers);
    static bool remove_snapshost_image(std::map<std::string, std::set<std::string>> carriers, std::string base_name, std::string name, uint32_t timeout, bool encrypted);
    static bool set_session_buffer(std::map<std::string, std::set<std::string>> carriers, std::string session, uint32_t buffer_size,  uint32_t timeout, bool encrypted);
    static bool send_checksum_info(std::set<std::string> carriers, const std::string& connection_id, const std::string& file_name, const std::string& data, uint32_t timeout, bool encrypted);
    static std::string receive_checksum_result(std::set<std::string> carriers, const std::string& connection_id, const std::string& file_name, uint32_t timeout, bool encrypted);

    bool close(const bool is_cancle = false);
    bool is_ssl_link()
    {
        foreach(carrier_op::ptr &c, _carriers)
        {
            if (c->ssl_link)
                return true;
        }
        return false;
    }
    static bool remove_connections(std::map<std::string, std::set<std::string>> carriers, uint32_t timeout, bool encrypted);
    static bool are_connections_ready(std::map<std::string, std::set<std::string>> carriers, uint32_t timeout, bool encrypted, std::map<std::string, std::set<std::string>> &missing_connections);
private:
    bool remove_snapshost_image();
    class carrier_op {
    public:
        typedef  boost::shared_ptr<carrier_op> ptr;
        typedef  std::vector<ptr> vtr;

        carrier_op(std::string _addr, uint32_t _timeout, bool encrypted, bool polling);
        virtual ~carrier_op() { disconnect(); }
        bool connect();
        bool disconnect();
        std::string                                                   addr;
        std::shared_ptr<TSocket>                                      socket;
        std::shared_ptr<TSSLSocket>                                   ssl_socket;
        std::shared_ptr<TTransport>                                   transport;
        std::shared_ptr<TProtocol>                                    protocol;
        std::shared_ptr<TSSLSocketFactory>                            factory;
        std::shared_ptr<saasame::transport::carrier_serviceClient>    client;
        macho::windows::critical_section                              cs;
        bool                                                          ssl_link;
        bool                                                          initialed;
        http_carrier_op::ptr                                          proxy;

    private:
        class MyAccessManager : public AccessManager
        {
            Decision verify(const sockaddr_storage& sa) throw()
            {
                return SKIP;
            }

            Decision verify(const std::string& host, const char* name, int size) throw()
            {
                if (name)
                {
                    std::string cn(name);

                    if (boost::iequals(cn, std::string("saasame")))
                        return ALLOW;
                }
                //return DENY;
                return ALLOW;
            }

            Decision verify(const sockaddr_storage& sa, const char* data, int size) throw()
            {
                return DENY;
            }
        };
    };

    carrier_rw(std::string session, std::set<std::string> connection_ids, std::set<std::string>  carriers, std::string name, std::string base_name, uint32_t timeout, bool encrypted, bool polling = false, uint8_t mode = 2) :
        _connection_ids(connection_ids), _base_name(base_name), _name(name), _session(session), _polling(polling), _mode(mode){
        _wname = macho::stringutils::convert_ansi_to_unicode(_name);
        _timeout = timeout;
        _encrypted = encrypted;
        foreach(std::string c, carriers){
            _carriers.push_back(carrier_op::ptr(new carrier_op(c, timeout, encrypted, polling)));
        }
    } 
    bool open();
    bool create(uint64_t size, uint32_t block_size, bool compressed, bool checksum, bool encrypted, std::string parent, bool checksum_verify, std::string comment, bool cdr);
    bool create(uint64_t size, uint32_t block_size, std::string parent, bool checksum_verify, std::string comment, bool cdr);
    std::wstring             _wname;
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