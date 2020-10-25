// This autogenerated skeleton file illustrates one way to adapt a synchronous
// interface into an asynchronous interface. You should copy it to another
// filename to avoid overwriting it and rewrite as asynchronous any functions
// that would otherwise introduce unwanted latency.

#include "host_agent_service.h"
#include <thrift/protocol/TBinaryProtocol.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::async;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class host_agent_serviceAsyncHandler : public host_agent_serviceCobSvIf {
 public:
  host_agent_serviceAsyncHandler() {
    syncHandler_ = std::auto_ptr<host_agent_serviceHandler>(new host_agent_serviceHandler);
    // Your initialization goes here
  }
  virtual ~host_agent_serviceAsyncHandler();

  void get_client_info(tcxx::function<void(client_info const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id) {
    client_info _return;
    syncHandler_->get_client_info(_return, session_id);
    return cob(_return);
  }

  void take_snapshots(tcxx::function<void(snapshot_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::set<int32_t> & disks) {
    snapshot_result _return;
    syncHandler_->take_snapshots(_return, session_id, disks);
    return cob(_return);
  }

  void get_latest_snapshots_info(tcxx::function<void(snapshot_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id) {
    snapshot_result _return;
    syncHandler_->get_latest_snapshots_info(_return, session_id);
    return cob(_return);
  }

  void replicate_disk(tcxx::function<void(replication_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const int32_t disk, const int64_t start, const int32_t length, const bool be_compressed) {
    replication_result _return;
    syncHandler_->replicate_disk(_return, session_id, disk, start, length, be_compressed);
    return cob(_return);
  }

  void replicate_snapshot(tcxx::function<void(replication_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& snapshot_id, const int64_t start, const int32_t length, const bool be_compressed) {
    replication_result _return;
    syncHandler_->replicate_snapshot(_return, session_id, snapshot_id, start, length, be_compressed);
    return cob(_return);
  }

  void get_snapshot_bit_map(tcxx::function<void(volume_bit_map const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& snapshot_id, const int64_t start, const bool be_compressed) {
    volume_bit_map _return;
    syncHandler_->get_snapshot_bit_map(_return, session_id, snapshot_id, start, be_compressed);
    return cob(_return);
  }

  void delete_snapshot(tcxx::function<void(delete_snapshot_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& snapshot_id) {
    delete_snapshot_result _return;
    syncHandler_->delete_snapshot(_return, session_id, snapshot_id);
    return cob(_return);
  }

  void delete_snapshot_set(tcxx::function<void(delete_snapshot_result const& _return)> cob, tcxx::function<void(::apache::thrift::TDelayedException* _throw)> /* exn_cob */, const std::string& session_id, const std::string& snapshot_set_id) {
    delete_snapshot_result _return;
    syncHandler_->delete_snapshot_set(_return, session_id, snapshot_set_id);
    return cob(_return);
  }

  void exit(tcxx::function<void()> cob, const std::string& session_id) {
    syncHandler_->exit(session_id);
    return cob();
  }

 protected:
  std::auto_ptr<host_agent_serviceHandler> syncHandler_;
};
