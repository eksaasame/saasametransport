/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _THRIFT_TRANSPORT_TCURLCLIENT_H_
#define _THRIFT_TRANSPORT_TCURLCLIENT_H_ 1

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif

#include <thrift/transport/TBufferTransports.h>

#include <curl/curl.h>

namespace apache { namespace thrift { namespace transport {

class TCurlClient : public TTransport {
 public:
  TCurlClient(std::string url="", int port = 0);

  virtual ~TCurlClient();

  void setCredentials(std::string credentials) {
    credentials_ = credentials;
  }

  void setOption(int opt, std::string val) {
    options_[opt] = val;
  }

  void addHeader(std::string val) {
    xheaders_.push_back(val);
  }

  virtual void open();

  virtual bool isOpen() {
    return isOpen_;
  }

  virtual bool peek() {
    return true;
  }

  virtual void close();

  uint32_t read(uint8_t* buf, uint32_t len);
  virtual uint32_t read_virt(uint8_t*  buf, uint32_t len) {
      return read(buf, len);
  }

  virtual uint32_t readEnd();

  void write(const uint8_t* buf, uint32_t len);
  virtual void write_virt(const uint8_t*  buf, uint32_t len ){
      write(buf, len);
  }

  virtual void flush();

 private:
  void init();
  void cleanup();
  bool isOpen_;
  static size_t curl_write(void *ptr, size_t size, size_t nmemb, void *data);
  static size_t curl_read(void *ptr, size_t size, size_t nmemb, void *data);

 protected:
  int                       port_;
  std::string               url_;
  std::string               credentials_;
  CURL                      *curl_;
  struct curl_slist         *headers_;
  std::vector<std::string>  xheaders_;
  std::map<int,std::string> options_;
  TMemoryBuffer             writeBuffer_;
  TMemoryBuffer             readBuffer_;
};

}}} // apache::thrift::transport

#endif // #ifndef _THRIFT_TRANSPORT_TCURLCLIENT_H_
