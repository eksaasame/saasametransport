#include "thrift_helper.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include "string_operation.h"
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/stdcxx.h>

stdcxx::shared_ptr<TSSLSocketFactory> g_factory; //(new TSSLSocketFactory());
std::shared_ptr<AccessManager> g_accessManager; //(new MyAccessManager());

std::shared_ptr<apache::thrift::transport::TTransport> TServerReverseSocket::acceptImpl() {
    while (_is_Open) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        bool is_connected = false;
        if (is_connected = _mgmt.open())
        {
            try {
                transport_message recive;
                _mgmt.client()->receive(recive, _session, _addr, _name);
                LOG_TRACE("recive.id = %llu.",recive.id);
                if (error != 0)
                {
                    error = 0;
                    LOG_TRACE("Recovery Session.");
                }
                return std::shared_ptr<TReverseSocketTransport>(new TReverseSocketTransport(_server, _addr, _session, recive));
            }
            catch (command_empty &empty) {
                if (error) {
                    error = 0;
                    LOG_TRACE("Recovery Session.empty.what() = %s", empty.what());
                }
                continue;
            }
            catch (invalid_session &invalid) {
                _mgmt.close();
                is_connected = false;
                LOG_ERROR("Invalid Session.");
            }
            catch (invalid_operation &invalid) {
                _mgmt.close();
                is_connected = false;
                LOG_ERROR("Invalid Operation.");
            }
            catch (apache::thrift::TException& ex) {
                _mgmt.close();
                is_connected = false;
                LOG_ERROR("%s", ex.what());
            }
            catch (...) {
                _mgmt.close();
                is_connected = false;
                LOG_ERROR("Unknown exception");
            }
        }
        if (!is_connected)
        {
            int sec = 1;
            if (error > 65) {
                sec = 15;
            }
            else if (error > 5) {
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