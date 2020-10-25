#pragma once

#ifndef mgmt_op_H
#define mgmt_op_H

#include "macho.h"
#include "saasame_types.h"
#include "saasame_constants.h"
#include "management_service.h"
#include "reverse_transport.h"
#include "TCurlClient.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/THttpClient.h>
#include <thrift/transport/TServerSocket.h>

using namespace  ::saasame::transport;
using namespace macho::windows;
using namespace macho;

template <class Type>
class mgmt_op_base{

public:
	typedef  boost::shared_ptr<mgmt_op_base<Type>> ptr;
    typedef  std::vector<ptr>           vtr;

	Type *client() const{ return _client.get(); }
    mgmt_op_base(std::set<std::string> addrs, std::string& prefer, int port, bool is_ssl, std::string uri = "", long timeout = 600) :_mgmt_addrs(addrs), _mgmt_addr(prefer), _port(port), _timeout(timeout), _is_ssl(is_ssl), _uri(uri){
    }

    bool connect(){
		FUN_TRACE;
        bool result = false;
        int retry_count = 1;
        while (true){
            try{
                if (!_transport->isOpen()){
                    _transport->open();
					client()->ping(_svc_info);
                }
                result = true;
                break;
            }
            catch (saasame::transport::invalid_operation& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
            }
            catch (apache::thrift::TException &ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
            }
            retry_count--;
            if (retry_count > 0)
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            else
                break;
        }
        return result;
    }

    bool disconnect(){
        bool result = false;
        FUN_TRACE;
        int retry_count = 3;
        while (retry_count > 0){
            try{
                if (_transport->isOpen())
                    _transport->close();
                result = !_transport->isOpen();
                break;
			}
			catch (saasame::transport::invalid_operation& ex){
				LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.why).c_str());
			}
			catch (apache::thrift::TException &ex){
				LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
			}
			catch (...){
			}
			boost::this_thread::sleep(boost::posix_time::seconds(1));
			retry_count--;
		}
		return result;
	}

	bool open(){
		FUN_TRACE;
		if (!_mgmt_addr.empty()){
			if (_is_ssl){
                _https = std::shared_ptr<apache::thrift::transport::TCurlClient>(new apache::thrift::transport::TCurlClient(boost::str(boost::format("https://%s%s") % _mgmt_addr%_uri), _port, _timeout));
                _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_https));
                _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                _client = std::shared_ptr<Type>(new Type(_protocol));
			}
			else{
                _http = std::shared_ptr<apache::thrift::transport::THttpClient>(new apache::thrift::transport::THttpClient(_mgmt_addr, _port, _uri));
                _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_http));
                _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                _client = std::shared_ptr<Type>(new Type(_protocol));
			}
			try{
				_transport->open();
				_client->ping(_svc_info);
				return true;
			}
			catch (apache::thrift::TException & tx){
				LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
			}
		}

        reverse_foreach(std::string mgmt_addr, _mgmt_addrs){
			FUN_TRACE_MSG(macho::stringutils::convert_utf8_to_unicode(mgmt_addr));
			if (_is_ssl){
                _https = std::shared_ptr<apache::thrift::transport::TCurlClient>(new apache::thrift::transport::TCurlClient(boost::str(boost::format("https://%s%s") % mgmt_addr%_uri), _port, _timeout));
                _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_https));
                _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                _client = std::shared_ptr<Type>(new Type(_protocol));
			}
			else{
                _http = std::shared_ptr<apache::thrift::transport::THttpClient>(new apache::thrift::transport::THttpClient(mgmt_addr, _port, _uri));
                _transport = std::shared_ptr<apache::thrift::transport::TTransport>(new apache::thrift::transport::TBufferedTransport(_http));
                _protocol = std::shared_ptr<apache::thrift::protocol::TProtocol>(new apache::thrift::protocol::TBinaryProtocol(_transport));
                _client = std::shared_ptr<Type>(new Type(_protocol));
			}
			try{
				_transport->open();
				_client->ping(_svc_info);
				_mgmt_addr = mgmt_addr;
				return true;
			}
			catch (apache::thrift::TException & tx){
				LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
			}
		}
		return false;
	}

	void close(){
		FUN_TRACE;
		try{
			if (_transport && _transport->isOpen())
				_transport->close();
		}
		catch (apache::thrift::TException & tx){
			LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
		}
		_client = NULL;
		_transport = NULL;
		_https = NULL;
		_http = NULL;
		_protocol = NULL;
	}
	virtual ~mgmt_op_base(){
		close();
	}
protected:
	saasame::transport::service_info                                   _svc_info;
	std::string&                                                       _mgmt_addr;
	std::string                                                        _uri;
	std::set<std::string>                                              _mgmt_addrs;
	bool                                                               _is_ssl;
	int                                                                _port;
    long                                                               _timeout;
    std::shared_ptr<Type>											   _client;
    std::shared_ptr<apache::thrift::transport::TTransport>             _transport;
    std::shared_ptr<apache::thrift::transport::TCurlClient>            _https;
    std::shared_ptr<apache::thrift::transport::THttpClient>            _http;
    std::shared_ptr<apache::thrift::protocol::TProtocol>               _protocol;
};

class mgmt_op : public mgmt_op_base<saasame::transport::management_serviceClient>{
public:
	mgmt_op(std::set<std::string> addrs, std::string& prefer, int port, bool is_ssl) :mgmt_op_base<saasame::transport::management_serviceClient>(addrs, prefer, port, is_ssl){
		_uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
	}
};

class http_reverse_transport_op : public mgmt_op_base<saasame::transport::reverse_transportClient>{
public:
    http_reverse_transport_op(std::set<std::string> addrs, std::string& prefer, int port, bool is_ssl, long timeout) :mgmt_op_base<saasame::transport::reverse_transportClient>(addrs, prefer, port, is_ssl, "", timeout){
		_uri = saasame::transport::g_saasame_constants.TRANSPORTER_SERVICE_PATH;
	}
};

#if 1

class TReverseSocketTransport : public apache::thrift::transport::TVirtualTransport<TReverseSocketTransport> {
public:
    TReverseSocketTransport(std::string server, std::string session, std::string addr, transport_message& recive) :
        _server(server),
        _mgmt({ server }, _server, 0, true, 600),
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
                if (_mgmt.open()){
					transport_message response;
					response.id = _id;
					response.message = _write_buffer.getBufferAsString();
                    _mgmt.client()->response(_session, _addr, response);
                    _mgmt.close();
                    _write_buffer.resetBuffer();
                    break;
                }
            }
            catch (apache::thrift::TException& ex){
                LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
            count--;
        }
    }

private:
    uint64_t                                 _id;
    std::string                              _addr;
    std::string                              _session;
	http_reverse_transport_op                _mgmt;
    apache::thrift::transport::TMemoryBuffer _write_buffer;
    apache::thrift::transport::TMemoryBuffer _read_buffer;
    std::string                              _server;
};

class TServerReverseSocket : public apache::thrift::transport::TServerTransport{
public:

    TServerReverseSocket(std::string server, std::string session, std::string addr, std::string name) :
        _is_Open(true),
        _addr(addr),
        _mgmt({ server }, _server, 0, true, 35),
        _server(server),
        _session(session),
        _name(name){
    }
    virtual void listen() {
        /* if (!_thrift.open()){
        throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::NOT_OPEN, "Could not listen");
        }*/
    }

    virtual void close(){
        _is_Open = false;
        _cond.notify_one();
    }
protected:
    virtual std::shared_ptr<apache::thrift::transport::TTransport> acceptImpl(){
        int                         error = 0;
        while (_is_Open){
            boost::unique_lock<boost::mutex> lock(_mutex);
            bool is_connected = false;
            if (is_connected = _mgmt.open()){
                try{
                    transport_message recive;
                    _mgmt.client()->receive(recive, _session, _addr, _name);
					if (error){
						error = 0;
						LOG(LOG_LEVEL_RECORD, L"Recovery Session.");
					}
                    LOG(LOG_LEVEL_RECORD, L"Message Id = %I64u", recive.id);
                    return std::shared_ptr<TReverseSocketTransport>(new TReverseSocketTransport(_server, _session, _addr, recive));
                }
                catch (command_empty &empty){
					if (error){
						error = 0;
						LOG(LOG_LEVEL_RECORD, L"Recovery Session.");
					}
                    continue;
                }
                catch (invalid_session &invalid){
                    _mgmt.close();
                    is_connected = false;
                    LOG(LOG_LEVEL_ERROR, L"Invalid Session.");
                }
				catch (invalid_operation &invalid){
					_mgmt.close();
					is_connected = false;
					LOG(LOG_LEVEL_ERROR, L"Invalid Operation.");
				}
                catch (apache::thrift::TException& ex){
                    _mgmt.close();
                    is_connected = false;
                    LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
                catch (...){
                    _mgmt.close();
                    is_connected = false;
                    LOG(LOG_LEVEL_ERROR, L"Unknown exception");
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
	http_reverse_transport_op               _mgmt;
    boost::mutex                            _mutex;
    boost::condition_variable               _cond;
};

#endif

#endif