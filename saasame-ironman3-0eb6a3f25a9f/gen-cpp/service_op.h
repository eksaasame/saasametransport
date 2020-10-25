#pragma once

#ifndef service_op_H
#define service_op_H

#include "macho.h"
#include "saasame_types.h"
#include "management_service.h"
#include "common_service_handler.h"
#include "loader_service.h"
#include "launcher_service.h"
#include "reverse_transport.h"

using namespace  ::saasame::transport;
using namespace macho::windows;
using namespace macho;

template <class Type>
class thrift_connect{
protected:
    std::string                                                               _addr;
    int                                                                       _port;
    std::shared_ptr<apache::thrift::transport::TSocket>                       _socket;
    std::shared_ptr<apache::thrift::transport::TSSLSocket>                    _ssl_socket;
    std::shared_ptr<apache::thrift::transport::TTransport>                    _transport;
    std::shared_ptr<apache::thrift::protocol::TProtocol>                      _protocol;
    std::shared_ptr<Type>                                                     _client;
    std::string                                      _session;
    saasame::transport::service_info                 _svc_info;
public:
    typedef std::shared_ptr<thrift_connect<Type>> ptr;
    Type* client() const { return _client.get(); }
    apache::thrift::transport::TTransport* transport() const  { return _transport.get(); }
    saasame::transport::service_info info() { return _svc_info; }
    thrift_connect(int port, std::string addr = "localhost") : _port(port), _addr(addr){}
    virtual ~thrift_connect(){
        close();
    }
    virtual void close(){
		FUN_TRACE;
        try{
            if (_transport && _transport->isOpen())
                _transport->close();
        }
        catch (apache::thrift::TException& ex){
        }
        _socket = NULL;
        _ssl_socket = NULL;
        _transport = NULL;
        _protocol = NULL;
        _client = NULL;
    }

    virtual bool open(){
        bool result = false;
		FUN_TRACE;
        try{
            macho::windows::registry reg;
            boost::filesystem::path p(macho::windows::environment::get_working_directory());
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                    p = reg[L"KeyPath"].wstring();
            }
            if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
            {
                try
                {
                    std::shared_ptr<apache::thrift::transport::TSSLSocketFactory> factory;
                    factory = std::shared_ptr<apache::thrift::transport::TSSLSocketFactory>(new apache::thrift::transport::TSSLSocketFactory());
                    factory->authenticate(false);
                    factory->loadCertificate((p / "server.crt").string().c_str());
                    factory->loadPrivateKey((p / "server.key").string().c_str());
                    factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                    std::shared_ptr<apache::thrift::transport::AccessManager> accessManager(new MyAccessManager());
                    factory->access(accessManager);
                    _ssl_socket = std::shared_ptr<apache::thrift::transport::TSSLSocket>(factory->createSocket(_addr, _port));
                    _ssl_socket->setConnTimeout(30 * 1000);
                    _ssl_socket->setSendTimeout(10 * 60 * 1000);
                    _ssl_socket->setRecvTimeout(10 * 60 * 1000);
                    _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_ssl_socket));
                }
                catch (apache::thrift::TException& ex) {
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
            }
            if (!_transport){
                _socket = std::shared_ptr<apache::thrift::transport::TSocket>(new apache::thrift::transport::TSocket(_addr, _port));
                _socket->setConnTimeout(30 * 1000);
                _socket->setSendTimeout(10 * 60 * 1000);
                _socket->setRecvTimeout(10 * 60 * 1000);
                _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_socket));
            }
            if (_transport){
                _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                _client = std::shared_ptr<Type>(new Type(_protocol));
                try{
                    _transport->open();
                    _client->ping(_svc_info);
                    result = true;
                }
                catch (apache::thrift::TException& ex1){
                    if (_transport->isOpen())
                        _transport->close();
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex1.what()).c_str());
                    if (_ssl_socket){
                        _socket = std::shared_ptr<apache::thrift::transport::TSocket>(new apache::thrift::transport::TSocket(_addr, _port));
                        _socket->setConnTimeout(30 * 1000);
                        _socket->setSendTimeout(10 * 60 * 1000);
                        _socket->setRecvTimeout(10 * 60 * 1000);
                        _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_socket));
                        _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                        _client = std::shared_ptr<Type>(new Type(_protocol));
                        try{
                            if (_transport){
                                _transport->open();
                                result = true;
                            }
                        }
                        catch (apache::thrift::TException& ex2){
                            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex2.what()).c_str());
                        }                        
                    }
                }
            }
        }
        catch (saasame::transport::invalid_operation& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
        }
        catch (apache::thrift::TException &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return result;
    }
};

template < class Type, class JobDetail >
class job_op : virtual public thrift_connect<Type>{
public:
    virtual ~job_op(){
        close();
    }

    job_op(int port, std::string addr = "localhost") : thrift_connect<Type>(port, addr){
    }

    bool ping(){
        bool result = open();
        close();
        return result;
    }
    bool has_job(const std::string job_id){
        bool result = false;
        if (open()){
            try{
                JobDetail detail;
                _client->get_job(detail, _session, job_id);
                result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool create_job(const std::string job_id, const create_job_detail& detail, JobDetail& _return = JobDetail()){
        bool result = false;
        if (open()){
            try{
                _client->create_job_ex(_return, _session, job_id, detail);
                result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool get_job(const std::string job_id, JobDetail& _return){
        bool result = false;
        if (open()){
            try{
                _client->get_job(_return, _session, job_id);
                result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool suspend_job(const std::string job_id){
        bool result = false;
        if (open()){
            try{
                result = _client->interrupt_job(_session, job_id);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool resume_job(const std::string job_id){
        bool result = false;
        if (open()){
            try{
                result = _client->resume_job(_session, job_id);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool remove_job(const std::string job_id){
        bool result = false;
        if (open()){
            try{
                result = _client->remove_job(_session, job_id);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }

    bool running_job(const std::string job_id){
        bool result = false;
        if (open()){
            try{
                result = _client->running_job(_session, job_id);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }
};

template < class Type, class JobDetail >
class job_op_ex : virtual public job_op<Type, JobDetail >
{
public:
    job_op_ex(int port, std::string addr = "localhost") : job_op<Type, JobDetail >(port, addr), thrift_connect<Type >(port, addr){
    }
    bool update_job(const std::string job_id, const create_job_detail& detail){
        bool result = false;
        if (open()){
            try{
                result = _client->update_job(_session, job_id, detail);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"(%s) Unknown exception", stringutils::convert_ansi_to_unicode(job_id).c_str());
            }
        }
        close();
        return result;
    }
};

class svc_connection_op :virtual public thrift_connect<common_connection_serviceClient>{
public:
    svc_connection_op(int port, std::string addr = "localhost") : thrift_connect<common_connection_serviceClient>(port, addr){}
    bool create_or_modify_connection(const connection& conn){
        bool result = false;
        if (open()){
            try{
                connection _conn;
                _client->get_connection(_conn, "", conn.id);
                result = _client->modify_connection("", conn);
            }
            catch (invalid_operation& ex){
                try{
                    result = _client->add_connection("", conn);
                }
                catch (saasame::transport::invalid_operation& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
                }
                catch (apache::thrift::TException &ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                }
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }      
        close();
        return result;
    }

    bool add_connection(const connection& conn){
        bool result = false;
        if (open()){
            try{
                result = _client->add_connection("", conn);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }

    bool modify_connection(const connection& conn){
        bool result = false;
        if (open()){
            try{
                result = _client->modify_connection("", conn);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }

    bool test_connection(const connection& conn){
        bool result = false;
        if (open()){
            try{
                result = _client->test_connection("", conn);
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }

    bool get_connection(const std::string conn_id, connection& conn){
        bool result = false;
        if (open()){
            try{
                 _client->get_connection(conn, "", conn_id);
                 result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }

    bool remove_connection(const std::string conn_id){
        bool result = false;
        if (open()){
            try{
                _client->remove_connection("", conn_id);
                result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }

    bool enumerate_connections(std::vector<connection> & _return){
        bool result = false;
        if (open()){
            try{
                _client->enumerate_connections(_return,"");
                result = true;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        close();
        return result;
    }
};

#if 0

class TReverseSocketTransport : public TVirtualTransport<TReverseSocketTransport> {
public:
    TReverseSocketTransport(std::string server, std::string session, std::string addr, transport_message& recive) :
        _thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT, server),
        _session(session),
        _addr(addr){
        _id = recive.id;
        _read_buffer.write((const uint8_t*)recive.message.c_str(), recive.message.length());
    }

    virtual void open(){
    }

    virtual bool isOpen(){
        return true;
    }

    virtual bool peek() {
        return true;
    }

    virtual void close(){
    }

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
        int count = 3;
		while (count > 0 && _write_buffer.available_read()){
			try{
                if (_thrift.open()){
                    transport_message response;
                    response.id = _id;
                    response.message = _write_buffer.getBufferAsString();
                    _thrift.client()->response(_session, _addr, response);
                    _thrift.close();
                    _write_buffer.resetBuffer();
                    break;
                }
            }
            catch (TException& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
            count--;
        }
    }

private:
    uint64_t                                _id;
    std::string                             _addr;
    std::string                             _session;
    thrift_connect<reverse_transportClient> _thrift;
    TMemoryBuffer                           _write_buffer;
    TMemoryBuffer                           _read_buffer;
};

class TServerReverseSocket : public TServerTransport{
public:

    TServerReverseSocket(std::string server, std::string session, std::string addr, std::string name) :
        _is_Open(true),
        _addr(addr),
        _server(server),
        _session(session),
        _name(name),
        _thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT, server){
    }
    virtual void listen() {
        /* if (!_thrift.open()){
        throw TTransportException(TTransportException::NOT_OPEN, "Could not listen");
        }*/
    }

    virtual void close(){
        _is_Open = false;
        _cond.notify_one();
    }
protected:
    virtual boost::shared_ptr<TTransport> acceptImpl(){
        int                         error = 0;
        while (_is_Open){
            boost::unique_lock<boost::mutex> lock(_mutex);
            bool is_connected = false;
            if (is_connected = _thrift.open()){
                try{
                    transport_message recive;
                    _thrift.client()->receive(recive, _session, _addr, _name);
                    error = 0;
                    return std::shared_ptr<TReverseSocketTransport>(new TReverseSocketTransport(_server, _session, _addr, recive));
                }
                catch (command_empty &empty){
                    continue;
                }
                catch (...){
                    _thrift.close();
                    is_connected = false;
                }
            }
            if (!is_connected){
                int sec = 1;
                if (error > 65){
                    sec = 15;
                }
                else if (error > 5){
                    error++;
                    sec = 5;
                }
                else {
                    error++;
                }
                _cond.timed_wait(lock, boost::posix_time::seconds(sec));
            }
        }
        return std::shared_ptr<TReverseSocketTransport>();
    }
private:
    bool                                    _is_Open;
    std::string                             _addr;
    std::string                             _session;
    std::string                             _name;
    std::string                             _server;
    thrift_connect<reverse_transportClient> _thrift;
    boost::mutex                            _mutex;
    boost::condition_variable               _cond;
};
#endif
#endif