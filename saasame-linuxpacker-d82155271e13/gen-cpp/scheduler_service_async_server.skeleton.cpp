// This autogenerated skeleton file illustrates one way to adapt a synchronous
// interface into an asynchronous interface. You should copy it to another
// filename to avoid overwriting it and rewrite as asynchronous any functions
// that would otherwise introduce unwanted latency.

#include "scheduler_service.h"
#include <thrift/protocol/TBinaryProtocol.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::async;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class scheduler_serviceAsyncHandler : public scheduler_serviceCobSvIf {
 public:
  scheduler_serviceAsyncHandler() {
    syncHandler_ = std::auto_ptr<scheduler_serviceHandler>(new scheduler_serviceHandler);
    // Your initialization goes here
  }
  virtual ~scheduler_serviceAsyncHandler();

  void get_physical_machine_detail(tcxx::function<void(physical_machine_info const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host, const machine_detail_filter::type filter) {
    physical_machine_info _return;
    syncHandler_->get_physical_machine_detail(_return, session_id, host, filter);
    return cob(_return);
  }

  void get_physical_machine_detail2(tcxx::function<void(physical_machine_info const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const machine_detail_filter::type filter) {
    physical_machine_info _return;
    syncHandler_->get_physical_machine_detail2(_return, session_id, host, username, password, filter);
    return cob(_return);
  }

  void get_virtual_host_info(tcxx::function<void(virtual_host const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password) {
    virtual_host _return;
    syncHandler_->get_virtual_host_info(_return, session_id, host, username, password);
    return cob(_return);
  }

  void get_virtual_machine_detail(tcxx::function<void(virtual_machine const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id) {
    virtual_machine _return;
    syncHandler_->get_virtual_machine_detail(_return, session_id, host, username, password, machine_id);
    return cob(_return);
  }

  void create_job_ex(tcxx::function<void(replica_job_detail const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job) {
    replica_job_detail _return;
    syncHandler_->create_job_ex(_return, session_id, job_id, create_job);
    return cob(_return);
  }

  void create_job(tcxx::function<void(replica_job_detail const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const create_job_detail& create_job) {
    replica_job_detail _return;
    syncHandler_->create_job(_return, session_id, create_job);
    return cob(_return);
  }

  void get_job(tcxx::function<void(replica_job_detail const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id) {
    replica_job_detail _return;
    syncHandler_->get_job(_return, session_id, job_id);
    return cob(_return);
  }

  void interrupt_job(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id) {
    bool _return = false;
    _return = syncHandler_->interrupt_job(session_id, job_id);
    return cob(_return);
  }

  void resume_job(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id) {
    bool _return = false;
    _return = syncHandler_->resume_job(session_id, job_id);
    return cob(_return);
  }

  void remove_job(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id) {
    bool _return = false;
    _return = syncHandler_->remove_job(session_id, job_id);
    return cob(_return);
  }

  void list_jobs(tcxx::function<void(std::vector<replica_job_detail>  const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id) {
    std::vector<replica_job_detail>  _return;
    syncHandler_->list_jobs(_return, session_id);
    return cob(_return);
  }

  void update_job(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job) {
    bool _return = false;
    _return = syncHandler_->update_job(session_id, job_id, create_job);
    return cob(_return);
  }

  void terminate(tcxx::function<void()> cob, const std::string& session_id) {
    syncHandler_->terminate(session_id);
    return cob();
  }

  void running_job(tcxx::function<void(bool const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& job_id) {
    bool _return = false;
    _return = syncHandler_->running_job(session_id, job_id);
    return cob(_return);
  }

  void verify_management(tcxx::function<void(bool const& _return)> cob, const std::string& management, const int32_t port, const bool is_ssl) {
    bool _return = false;
    _return = syncHandler_->verify_management(management, port, is_ssl);
    return cob(_return);
  }

  void verify_packer_to_carrier(tcxx::function<void(bool const& _return)> cob, const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl) {
    bool _return = false;
    _return = syncHandler_->verify_packer_to_carrier(packer, carrier, port, is_ssl);
    return cob(_return);
  }

  void take_packer_xray(tcxx::function<void(std::string const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host) {
    std::string _return;
    syncHandler_->take_packer_xray(_return, session_id, host);
    return cob(_return);
  }

  void get_packer_service_info(tcxx::function<void(service_info const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& host) {
    service_info _return;
    syncHandler_->get_packer_service_info(_return, session_id, host);
    return cob(_return);
  }

 protected:
  std::auto_ptr<scheduler_serviceHandler> syncHandler_;
};

