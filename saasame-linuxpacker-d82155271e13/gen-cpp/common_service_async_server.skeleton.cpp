// This autogenerated skeleton file illustrates one way to adapt a synchronous
// interface into an asynchronous interface. You should copy it to another
// filename to avoid overwriting it and rewrite as asynchronous any functions
// that would otherwise introduce unwanted latency.

#include "common_service.h"
#include <thrift/protocol/TBinaryProtocol.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::async;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class common_serviceAsyncHandler : public common_serviceCobSvIf {
 public:
  common_serviceAsyncHandler() {
    syncHandler_ = std::auto_ptr<common_serviceHandler>(new common_serviceHandler);
    // Your initialization goes here
  }
  virtual ~common_serviceAsyncHandler();

  void ping(tcxx::function<void(service_info const& _return)> cob) {
    service_info _return;
    syncHandler_->ping(_return);
    return cob(_return);
  }

  void get_host_detail(tcxx::function<void(physical_machine_info const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const machine_detail_filter::type filter) {
    physical_machine_info _return;
    syncHandler_->get_host_detail(_return, session_id, filter);
    return cob(_return);
  }

  void get_service_list(tcxx::function<void(std::set<service_info>  const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id) {
    std::set<service_info>  _return;
    syncHandler_->get_service_list(_return, session_id);
    return cob(_return);
  }

  void enumerate_disks(tcxx::function<void(std::set<disk_info>  const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const enumerate_disk_filter_style::type filter) {
    std::set<disk_info>  _return;
    syncHandler_->enumerate_disks(_return, filter);
    return cob(_return);
  }

  void verify_carrier(tcxx::function<void(bool const& _return)> cob, const std::string& carrier, const bool is_ssl) {
    bool _return = false;
    _return = syncHandler_->verify_carrier(carrier, is_ssl);
    return cob(_return);
  }

  void take_xray(tcxx::function<void(std::string const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */) {
    std::string _return;
    syncHandler_->take_xray(_return);
    return cob(_return);
  }

  void take_xrays(tcxx::function<void(std::string const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */) {
    std::string _return;
    syncHandler_->take_xrays(_return);
    return cob(_return);
  }

  void create_mutex(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session, const int16_t timeout) {
    bool _return = false;
    _return = syncHandler_->create_mutex(session, timeout);
    return cob(_return);
  }

  void delete_mutex(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session) {
    bool _return = false;
    _return = syncHandler_->delete_mutex(session);
    return cob(_return);
  }

 protected:
  std::auto_ptr<common_serviceHandler> syncHandler_;
};

