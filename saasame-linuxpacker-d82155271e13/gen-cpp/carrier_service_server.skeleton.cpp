// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "carrier_service.h"
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

class carrier_serviceHandler : virtual public carrier_serviceIf {
 public:
  carrier_serviceHandler() {
    // Your initialization goes here
  }

  void create(std::string& _return, const std::string& session_id, const create_image_info& image) {
    // Your implementation goes here
    printf("create\n");
  }

  void create_ex(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name, const int64_t size, const int32_t block_size, const std::string& parent, const bool checksum_verify) {
    // Your implementation goes here
    printf("create_ex\n");
  }

  void open(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name) {
    // Your implementation goes here
    printf("open\n");
  }

  void read(std::string& _return, const std::string& session_id, const std::string& image_id, const int64_t start, const int32_t number_of_bytes_to_read) {
    // Your implementation goes here
    printf("read\n");
  }

  int32_t write(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write) {
    // Your implementation goes here
    printf("write\n");
  }

  int32_t write_ex(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write, const bool is_compressed) {
    // Your implementation goes here
    printf("write_ex\n");
  }

  bool close(const std::string& session_id, const std::string& image_id, const bool is_cancel) {
    // Your implementation goes here
    printf("close\n");
  }

  bool remove_base_image(const std::string& session_id, const std::set<std::string> & base_images) {
    // Your implementation goes here
    printf("remove_base_image\n");
  }

  bool remove_snapshot_image(const std::string& session_id, const std::map<std::string, image_map_info> & images) {
    // Your implementation goes here
    printf("remove_snapshot_image\n");
  }

  bool verify_management(const std::string& management, const int32_t port, const bool is_ssl) {
    // Your implementation goes here
    printf("verify_management\n");
  }

  bool set_buffer_size(const std::string& session_id, const int32_t size) {
    // Your implementation goes here
    printf("set_buffer_size\n");
  }

  bool is_buffer_free(const std::string& session_id, const std::string& image_id) {
    // Your implementation goes here
    printf("is_buffer_free\n");
  }

  bool is_image_replicated(const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& image_name) {
    // Your implementation goes here
    printf("is_image_replicated\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<carrier_serviceHandler> handler(new carrier_serviceHandler());
  shared_ptr<TProcessor> processor(new carrier_serviceProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

