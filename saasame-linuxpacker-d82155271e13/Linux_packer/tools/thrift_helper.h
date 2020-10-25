#pragma once

#ifndef thrift_helper_H
#define thrift_helper_H

#include "../gen-cpp/saasame_types.h"
#include "../gen-cpp/saasame_constants.h"
#include "../gen-cpp/management_service.h"
#include "../gen-cpp/reverse_transport.h"
#include "TCurlClient.h"
#include "log.h"
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/stdcxx.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/THttpClient.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TVirtualTransport.h>
#include <thrift/async/TAsyncChannel.h>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "sslHelper.h"
using namespace saasame::transport;
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace saasame;

extern stdcxx::shared_ptr<TSSLSocketFactory> g_factory;
extern std::shared_ptr<AccessManager> g_accessManager;

template <class Type,class ping_result_type>
class TSaasame_helper
{
public:
    typedef  boost::shared_ptr<TSaasame_helper<Type, ping_result_type>> ptr;
    typedef  std::vector<ptr> vtr;
    TSaasame_helper(std::set<std::string> address, std::string prefer, std::string uri, int port, bool bssl): _uri(uri), _port(port), _is_ssl(bssl) 
    {
        _address.push_back(prefer); 
        for (auto & a : address)
            _address.push_back(a);
        /*for (auto & a : _address)
        {
            LOG_TRACE("a = %s\r\n", a.c_str());
        }*/
    }

    bool open()
    {
        //FUN_TRACE;  
        if (!_transport || (_transport && !_transport->isOpen()))
        {
            if (!_address.empty())
            {
                for (auto & address : _address)
                {
                    if (_is_ssl)
                    {
                        //LOG_TRACE("_is_ssl");
                        _https = std::shared_ptr<apache::thrift::transport::TCurlClient>(new apache::thrift::transport::TCurlClient(string_op::strprintf("https://%s%s", address.c_str(), _uri.c_str()), _port));
                        _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_https));
                    }
                    else
                    {
                        //LOG_TRACE("_not_ssl");
                        _http = std::shared_ptr<apache::thrift::transport::THttpClient>(new apache::thrift::transport::THttpClient(address, _port, _uri));
                        _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_http));
                    }
                    _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                    _client = std::shared_ptr<Type>(new Type(_protocol));
                    try {
                        //LOG_TRACE("ping");
                        _transport->open();
                        _client->ping(_ping_result);
                        return true;
                    }
                    catch (apache::thrift::TException & tx) {
                        LOG_ERROR("TExceptioin: %s", tx.what());
                    }
                }
                LOG_ERROR("all address is fail.");
                return false;
            }
            LOG_ERROR("address is empty.");
            return false;
        }
        return true;
    }
    bool close()
    {
        //FUN_TRACE;
        if (_transport && _transport->isOpen())
        {
            try {
                _transport->close();
            }
            catch (apache::thrift::TException & tx) {
                LOG_ERROR("TExceptioin: %s", tx.what());
            }
            _client = NULL;
            _transport = NULL;
            _https = NULL;
            _http = NULL;
            _protocol = NULL;
        }
    }
    bool connect() {
        //FUN_TRACE;
        bool result = false;
        int retry_count = 1;
        while (true) {
            try {
                if (!_transport->isOpen()) {
                    _transport->open();
                    client()->ping(_ping_result);
                }
                result = true;
                break;
            }
            catch (saasame::transport::invalid_operation& ex) {
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (apache::thrift::TException &ex) {
                LOG_ERROR("%s", ex.what());
            }
            catch (...) {
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
        return result;
    }
    bool disconnect()
    {
        bool result = false;
        //FUN_TRACE;
        int retry_count = 3;
        while (retry_count > 0) {
            try {
                if (_transport->isOpen())
                    _transport->close();
                result = !_transport->isOpen();
                break;
            }
            catch (saasame::transport::invalid_operation& ex) {
                LOG_ERROR("%s", ex.why.c_str());
            }
            catch (apache::thrift::TException &ex) {
                LOG_ERROR("%s", ex.what());
            }
            catch (...) {
            }
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            retry_count--;
        }
        return result;
    }
    std::shared_ptr<Type> client() { return _client; }
    ~TSaasame_helper() { close(); }
private:
    ping_result_type    _ping_result;
    std::vector<std::string>         _address;
    std::string         _uri;
    int                 _port;
    bool                _is_ssl;
    std::shared_ptr<Type>											 _client;
    std::shared_ptr<apache::thrift::transport::TTransport>           _transport;
    std::shared_ptr<apache::thrift::transport::TCurlClient>          _https;
    std::shared_ptr<apache::thrift::transport::THttpClient>          _http;
    std::shared_ptr<apache::thrift::protocol::TProtocol>             _protocol;
};

class mgmt_op : public TSaasame_helper<saasame::transport::management_serviceClient, saasame::transport::service_info> {
public:
    mgmt_op(std::set<std::string> addrs, std::string addr, int port, bool is_ssl) : TSaasame_helper<saasame::transport::management_serviceClient,
        saasame::transport::service_info>(addrs,addr, saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH, port, is_ssl) {}
};

class http_reverse_transport_op : public TSaasame_helper<saasame::transport::reverse_transportClient, saasame::transport::service_info> {
public:
    http_reverse_transport_op(std::set<std::string> addrs, std::string addr, int port, bool is_ssl) : TSaasame_helper<saasame::transport::reverse_transportClient, 
        saasame::transport::service_info>(addrs,addr, saasame::transport::g_saasame_constants.TRANSPORTER_SERVICE_PATH, port, is_ssl) {}
};


class TReverseSocketTransport : public apache::thrift::transport::TVirtualTransport<TReverseSocketTransport>
{
public:
    TReverseSocketTransport(std::string server, std::string addr, std::string session, transport_message & recieve) :
        _mgmt({ server },server, 0, true),
        _addr(addr), _session(session) {
        _id = recieve.id;
        _read_buffer.write((const unsigned char * )recieve.message.c_str(), recieve.message.length());
        /*for test*/
        /*max = 512;
        sbuf = (uint8_t*)malloc(max);
        start = sbuf;
        length = 0;*/
    }
    virtual void open() {}
    virtual bool isOpen() { return true; }
    virtual bool peek(){ return true; }
    virtual void close(){}

    uint32_t read(uint8_t * buf, uint32_t len)
    {
        //FUN_TRACE
        return _read_buffer.read(buf, len);
    }
    virtual uint32_t read_virt(uint8_t * buf, uint32_t len) {
        //FUN_TRACE;
        return read(buf, len); 
    }
    virtual uint32_t readEnd() {
        //FUN_TRACE;
        _read_buffer.resetBuffer(); return 0; }
    void write(const uint8_t* buf, uint32_t len) {
        //FUN_TRACE;
        //LOG_TRACE("%s", buf);
        /*if (length + len > max)
        {
            while(length + len > max)
                max *= 2;
            sbuf = (uint8_t*)realloc(sbuf, max);
            start = sbuf + length;
        }
        memcpy(start,buf,len);
        start += len;
        length += len;*/
        _write_buffer.write(buf, len);
    }

    virtual void write_virt(uint8_t * buf, uint32_t len) { 
        //FUN_TRACE;
        write(buf, len); }
    virtual void flush() {
        //FUN_TRACE;
        int count = 3;
        while (count > 0 && _write_buffer.available_read())
        {
            //LOG_TRACE("_write_buffer.available_read() is OK");
            try {
                if (_mgmt.open()) {
                    transport_message response;
                    response.id = _id;
                    response.message = _write_buffer.getBufferAsString();
                    //LOG_TRACE("response.message = %s\r\n", response.message.c_str());
                    _mgmt.client()->response(_session, _addr, response);
                    _mgmt.close();
                    _write_buffer.resetBuffer();

                    /*length = 0;
                    start = sbuf;*/

                    break;
                }
            }
            catch (apache::thrift::TException& ex) {
                LOG_ERROR("%s", ex.what());
            }
            catch (...) {
                LOG_ERROR("Unknown exceotion");
            }
            count--;
        }
    }
private:
    uint64_t        _id;
    std::string     _addr;
    std::string     _session;
    http_reverse_transport_op               _mgmt;
    /*uint8_t * sbuf;
    int max;
    uint8_t * start;
    int length;*/
    apache::thrift::transport::TMemoryBuffer    _write_buffer;
    apache::thrift::transport::TMemoryBuffer    _read_buffer;
};

class TServerReverseSocket : public apache::thrift::transport::TServerTransport {
public:

    TServerReverseSocket(std::string server, std::string session, std::string addr, std::string name) :
        _is_Open(true),
        _addr(addr),
        _mgmt({ server },server, 0, true),
        _server(server),
        _session(session),
        _name(name),
        error(0){
    }
    virtual void listen() {
    }

    virtual void close() {
        _is_Open = false;
        _cond.notify_one();
    }
protected:
    virtual std::shared_ptr<apache::thrift::transport::TTransport> acceptImpl();
private:
    bool                                    _is_Open;
    std::string                             _addr;
    std::string                             _session;
    std::string                             _name;
    std::string                             _server;
    http_reverse_transport_op                _mgmt;
    boost::mutex                            _mutex;
    boost::condition_variable               _cond;
    int                                     error;
};

#endif