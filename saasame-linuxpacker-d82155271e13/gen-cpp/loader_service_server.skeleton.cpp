// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "loader_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::saasame::transport;

class loader_serviceHandler : virtual public loader_serviceIf {
 public:
  loader_serviceHandler() {
    // Your initialization goes here
  }

  void create_job_ex(loader_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job) {
    // Your implementation goes here
    printf("create_job_ex\n");
  }

  void create_job(loader_job_detail& _return, const std::string& session_id, const create_job_detail& create_job) {
    // Your implementation goes here
    printf("create_job\n");
  }

  void get_job(loader_job_detail& _return, const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("get_job\n");
  }

  bool interrupt_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("interrupt_job\n");
  }

  bool resume_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("resume_job\n");
  }

  bool remove_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("remove_job\n");
  }

  void list_jobs(std::vector<loader_job_detail> & _return, const std::string& session_id) {
    // Your implementation goes here
    printf("list_jobs\n");
  }

  bool update_job(const std::string& session_id, const std::string& job_id, const create_job_detail& job) {
    // Your implementation goes here
    printf("update_job\n");
  }

  void terminate(const std::string& session_id) {
    // Your implementation goes here
    printf("terminate\n");
  }

  bool remove_snapshot_image(const std::string& session_id, const std::map<std::string, image_map_info> & images) {
    // Your implementation goes here
    printf("remove_snapshot_image\n");
  }

  bool running_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("running_job\n");
  }

  bool verify_management(const std::string& management, const int32_t port, const bool is_ssl) {
    // Your implementation goes here
    printf("verify_management\n");
  }

  bool set_customized_id(const std::string& session_id, const std::string& disk_addr, const std::string& disk_id) {
    // Your implementation goes here
    printf("set_customized_id\n");
  }

  void create_vhd_disk_from_snapshot(std::string& _return, const std::string& connection_string, const std::string& container, const std::string& original_disk_name, const std::string& target_disk_name, const std::string& snapshot) {
    // Your implementation goes here
    printf("create_vhd_disk_from_snapshot\n");
  }

  bool is_snapshot_vhd_disk_ready(const std::string& task_id) {
    // Your implementation goes here
    printf("is_snapshot_vhd_disk_ready\n");
  }

  bool delete_vhd_disk(const std::string& connection_string, const std::string& container, const std::string& disk_name) {
    // Your implementation goes here
    printf("delete_vhd_disk\n");
  }

  bool delete_vhd_disk_snapshot(const std::string& connection_string, const std::string& container, const std::string& disk_name, const std::string& snapshot) {
    // Your implementation goes here
    printf("delete_vhd_disk_snapshot\n");
  }

  void get_vhd_disk_snapshots(std::vector<vhd_snapshot> & _return, const std::string& connection_string, const std::string& container, const std::string& disk_name) {
    // Your implementation goes here
    printf("get_vhd_disk_snapshots\n");
  }

  bool verify_connection_string(const std::string& connection_string) {
    // Your implementation goes here
    printf("verify_connection_string\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<loader_serviceHandler> handler(new loader_serviceHandler());
  shared_ptr<TProcessor> processor(new loader_serviceProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

