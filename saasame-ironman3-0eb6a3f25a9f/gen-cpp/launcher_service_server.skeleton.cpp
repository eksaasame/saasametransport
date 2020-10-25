// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "launcher_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::saasame::transport;

class launcher_serviceHandler : virtual public launcher_serviceIf {
 public:
  launcher_serviceHandler() {
    // Your initialization goes here
  }

  void create_job_ex(launcher_job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job) {
    // Your implementation goes here
    printf("create_job_ex\n");
  }

  void create_job(launcher_job_detail& _return, const std::string& session_id, const create_job_detail& create_job) {
    // Your implementation goes here
    printf("create_job\n");
  }

  void get_job(launcher_job_detail& _return, const std::string& session_id, const std::string& job_id) {
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

  bool update_job(const std::string& session_id, const std::string& job_id, const create_job_detail& job) {
    // Your implementation goes here
    printf("update_job\n");
  }

  bool remove_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("remove_job\n");
  }

  void list_jobs(std::vector<launcher_job_detail> & _return, const std::string& session_id) {
    // Your implementation goes here
    printf("list_jobs\n");
  }

  void terminate(const std::string& session_id) {
    // Your implementation goes here
    printf("terminate\n");
  }

  bool running_job(const std::string& session_id, const std::string& job_id) {
    // Your implementation goes here
    printf("running_job\n");
  }

  bool verify_management(const std::string& management, const int32_t port, const bool is_ssl) {
    // Your implementation goes here
    printf("verify_management\n");
  }

  bool unregister(const std::string& session) {
    // Your implementation goes here
    printf("unregister\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  ::apache::thrift::stdcxx::shared_ptr<launcher_serviceHandler> handler(new launcher_serviceHandler());
  ::apache::thrift::stdcxx::shared_ptr<TProcessor> processor(new launcher_serviceProcessor(handler));
  ::apache::thrift::stdcxx::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  ::apache::thrift::stdcxx::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  ::apache::thrift::stdcxx::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}
