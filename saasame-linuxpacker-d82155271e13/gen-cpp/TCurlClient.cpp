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

#include <cstdlib>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")    
#pragma comment(lib, "wsock32.lib")    
#pragma comment(lib, "Secur32.lib")  
#pragma comment(lib, "libssh2.lib")  
#pragma comment(lib, "ssleay32.lib")  
#pragma comment(lib, "libeay32.lib")  
#pragma comment(lib, "wldap32.lib") 
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "zlib.lib")

#include "TCurlClient.h"

/**
 * based on thrift/lib/cpp/src/transport/THttpClient.cpp
 * and      thrift/lib/perl/lib/Thrift/HttpClient.pm
 */
namespace apache { namespace thrift { namespace transport {

using namespace std;

#define CONTENT_TYPE "application/x-thrift"
#define USE_EXPECT
#define USE_CHUNKED
//#define USE_KEEPALIVE

#define CURL_FUNC(x) \
{ \
    CURLcode code = x; \
    if(CURLE_OK != code) \
        return code; \
}

/* read HTTP response body */
size_t TCurlClient::curl_write(void *ptr, size_t size, size_t nmemb, void *data)
{
  uint32_t len = size*nmemb;
  TCurlClient *tcc = (TCurlClient *)data;

  tcc->readBuffer_.write((uint8_t *)ptr, len);
  return len;
}

/* read POST data */
size_t TCurlClient::curl_read(void *ptr, size_t size, size_t nmemb, void *data)
{
  uint32_t len = size*nmemb;
  TCurlClient *tcc = (TCurlClient *)data;
  uint32_t avail = tcc->writeBuffer_.available_read();

  if (avail < 0) {
    return 0;
  }
  else if (len > avail) {
    len = avail;
  }
  return tcc->writeBuffer_.read((uint8_t *)ptr, len);
}

TCurlClient::TCurlClient(string url, int port) :
  url_(url) ,
  port_(port) {
  isOpen_ = false;
  headers_ = NULL;
  curl_ = NULL;
}

TCurlClient::~TCurlClient() {
  cleanup();
}

void TCurlClient::open() {
#ifdef USE_KEEPALIVE
  init();
#endif
}

void TCurlClient::close() {
#ifdef USE_KEEPALIVE
  cleanup();
#endif
}

void TCurlClient::init() {

    curl_global_init(CURL_GLOBAL_ALL);
    curl_ = curl_easy_init();
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, "C++/TCurlClient");
        // SKIP_PEER_VERIFICATION
        /*
        * If you want to connect to a site who isn't using a certificate that is
        * signed by one of the certs in the CA bundle you have, you can skip the
        * verification of the server's certificate. This makes the connection
        * A LOT LESS SECURE.
        *
        * If you have a CA cert for the server stored someplace else than in the
        * default bundle, then the CURLOPT_CAPATH option might come handy for
        * you.
        */
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    // SKIP_HOSTNAME_VERIFICATION
        /*
        * If the site you're connecting to uses a different host name that what
        * they have mentioned in their server certificate's commonName (or
        * subjectAltName) fields, libcurl will refuse to connect. You can skip
        * this check, but this will make the connection less secure.
        */
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    #ifdef USE_DEBUG
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
    #endif
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, TCurlClient::curl_write);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void *)this);

    curl_easy_setopt(curl_, CURLOPT_READFUNCTION, TCurlClient::curl_read);
    curl_easy_setopt(curl_, CURLOPT_READDATA, (void *)this);

    if (port_ > 0){     
        curl_easy_setopt(curl_, CURLOPT_PORT, port_);
    }

    if (credentials_.length() > 0) {
        curl_easy_setopt(curl_, CURLOPT_USERPWD, credentials_.c_str());
    }

    /* complete within 6000 seconds */
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 6000L);

    headers_ = NULL;
    #ifdef USE_CHUNKED
    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
    #endif
    #ifndef USE_EXPECT
    headers_ = curl_slist_append(headers_, "Expect:");
    #endif
    headers_ = curl_slist_append(headers_, "Content-Type: " CONTENT_TYPE);
    headers_ = curl_slist_append(headers_, "Accept: " CONTENT_TYPE);

    for (std::vector<std::string>::iterator hdr = xheaders_.begin();
        hdr != xheaders_.end(); ++hdr) {

    headers_ = curl_slist_append(headers_, (*hdr).c_str());
    }

    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);

    for (std::map<int,std::string>::iterator opt = options_.begin();
        opt != options_.end(); opt++) {

    CURLoption key = (CURLoption)opt->first;
    if (key < CURLOPTTYPE_OBJECTPOINT) {
        long lval;
        if (opt->second.compare("true") == 0) {
        lval = 1;
        }
        else if (opt->second.compare("false") == 0) {
        lval = 0;
        }
        else {
        lval = strtol(opt->second.c_str(), NULL, 10);
        }
        curl_easy_setopt(curl_, key, lval);
    }
    else if (key < CURLOPTTYPE_OFF_T) {
        curl_easy_setopt(curl_, key, opt->second.c_str());
    }
    else {
        //XXX throw
#ifndef USE_KEEPALIVE
        cleanup();
#endif
        throw TTransportException(TTransportException::NOT_OPEN, "Failed to initialize TCurlTransport.");
    }
    }
    isOpen_ = true;
}

void TCurlClient::cleanup() {
    if (headers_) {
        curl_slist_free_all(headers_);
        headers_ = NULL;
    }
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }
    writeBuffer_.resetBuffer();
    isOpen_ = false;
}

uint32_t TCurlClient::read(uint8_t* buf, uint32_t len) {
  uint32_t nread = readBuffer_.read(buf, len);
  return nread;
}

uint32_t TCurlClient::readEnd() {
  readBuffer_.resetBuffer();
  return 0;
}

void TCurlClient::write(const uint8_t* buf, uint32_t len) {
  writeBuffer_.write(buf, len);
}

void TCurlClient::flush() {

  if(writeBuffer_.available_read() <= 0)
	return;

  CURLcode res;
  char error[CURL_ERROR_SIZE];
  long rc;

#ifndef USE_KEEPALIVE
  init();
#endif
#ifndef USE_CHUNKED
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE,
		   writeBuffer_.available_read());
#endif
  error[0] = '\0';
  curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error);
  res = curl_easy_perform(curl_);
  
  if (res) {
    string msg =
      string("flush to ") + url_ + string(" failed: ") + error;
#ifndef USE_KEEPALIVE
    cleanup();
#endif
    throw TTransportException(msg);
  }

  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &rc);
  if (rc != 200) {
    char code[4];
    string msg;
#ifdef WIN32
    _snprintf(code, sizeof(code), "%d", rc);
#else
    snprintf(code, sizeof(code), "%d", rc);
#endif
    msg = string("flush to ") + url_ + string(" failed: ") + code;
#ifndef USE_KEEPALIVE
    cleanup();
#endif
    throw TTransportException(msg);
  }
#ifndef USE_KEEPALIVE
  cleanup();
#endif
}

}}} // apache::thrift::transport
