// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "management_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::saasame::transport;

class management_serviceHandler : virtual public management_serviceIf {
 public:
  management_serviceHandler() {
    // Your initialization goes here
  }

  void get_replica_job_create_detail(replica_job_create_detail& _return, const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("get_replica_job_create_detail\n");
  }

  void update_replica_job_state(const std::string& session_id, const replica_job_detail& state) {
    // Your implementation goes here
    printf("update_replica_job_state\n");
  }

  bool is_replica_job_alive(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("is_replica_job_alive\n");
  }

  void get_loader_job_create_detail(loader_job_create_detail& _return, const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("get_loader_job_create_detail\n");
  }

  void update_loader_job_state(const std::string& session_id, const loader_job_detail& state) {
    // Your implementation goes here
    printf("update_loader_job_state\n");
  }

  bool take_snapshots(const std::string& session_id, const std::string& snapshot_id) {
    // Your implementation goes here
    printf("take_snapshots\n");
  }

  bool check_snapshots(const std::string& session_id, const std::string& snapshots_id) {
    // Your implementation goes here
    printf("check_snapshots\n");
  }

  void get_launcher_job_create_detail(launcher_job_create_detail& _return, const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("get_launcher_job_create_detail\n");
  }

  void update_launcher_job_state(const std::string& session_id, const launcher_job_detail& state) {
    // Your implementation goes here
    printf("update_launcher_job_state\n");
  }

  void update_launcher_job_state_ex(const std::string& session_id, const launcher_job_detail& state) {
    // Your implementation goes here
    printf("update_launcher_job_state_ex\n");
  }

  bool is_launcher_job_image_ready(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("is_launcher_job_image_ready\n");
  }

  bool is_loader_job_devices_ready(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("is_loader_job_devices_ready\n");
  }

  bool mount_loader_job_devices(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("mount_loader_job_devices\n");
  }

  bool dismount_loader_job_devices(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("dismount_loader_job_devices\n");
  }

  bool discard_snapshots(const std::string& session_id, const std::string& snapshots_id) {
    // Your implementation goes here
    printf("discard_snapshots\n");
  }

  void register_service(register_return& _return, const std::string& session_id, const register_service_info& register_info, const physical_machine_info& machine_info) {
    // Your implementation goes here
    printf("register_service\n");
  }

  void register_physical_packer(register_return& _return, const std::string& session_id, const register_physical_packer_info& packer_info, const physical_machine_info& machine_info) {
    // Your implementation goes here
    printf("register_physical_packer\n");
  }

  bool check_running_task(const std::string& task_id, const std::string& parameters) {
    // Your implementation goes here
    printf("check_running_task\n");
  }

  void ping(service_info& _return) {
    // Your implementation goes here
    printf("ping\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  ::apache::thrift::stdcxx::shared_ptr<management_serviceHandler> handler(new management_serviceHandler());
  ::apache::thrift::stdcxx::shared_ptr<TProcessor> processor(new management_serviceProcessor(handler));
  ::apache::thrift::stdcxx::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  ::apache::thrift::stdcxx::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  ::apache::thrift::stdcxx::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

