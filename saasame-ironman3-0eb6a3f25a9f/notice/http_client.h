#pragma once

#ifndef __IRONMAN_NOTICE_HTTP_CLIENT__
#define __IRONMAN_NOTICE_HTTP_CLIENT__

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif

#include <curl/curl.h>
#include <string>
#include <vector>

class http_client{
public:
    explicit http_client(bool initSsl = true);
    virtual ~http_client();

    CURLcode del(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& response_headers, int port = 0);
    CURLcode del(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& response_headers , int port = 0);
    CURLcode del(const std::string& url, const std::vector<std::string>& headers, long& resp, std::string& response_headers = std::string(), int port = 0);

    CURLcode put(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& out = std::string(), std::string& response_headers = std::string(), int port = 0);
    CURLcode put(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& out, std::string& response_headers = std::string(), int port = 0);

    CURLcode post(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& out = std::string(), std::string& response_headers = std::string(), int port = 0);
    CURLcode post(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& out, std::string& response_headers, int port = 0);

    CURLcode get(const std::wstring& url, const std::wstring& username, const std::wstring& password, long& resp, std::vector<char>& buffer);
    CURLcode get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::vector<char>& buffer);
    CURLcode get(const std::wstring& url, const std::wstring& username, const std::wstring& password, long& resp, std::wstring& buffer);
    CURLcode get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::string& buffer);
    CURLcode get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::fstream& outfilep);

    std::wstring& escapeStr(std::wstring& str);
    std::string& escapeStr(std::string& str);

    std::wstring& getErrorStr(CURLcode code, std::wstring& str);
    std::string& getErrorStr(CURLcode code, std::string& str);

private:
    static size_t writeFunction(char* ptr, size_t size, size_t nmemb, std::vector<char>* buffer);
    static size_t writeFileFunction(char* ptr, size_t size, size_t nmemb, std::fstream *outfilep);
    static size_t readFunction(void *ptr, size_t size, size_t nmemb, void *userp);

    CURLcode init();
    CURLcode setUsernamePassword(const std::string& username, const std::string& password);

    CURL* curl_;
    bool globalInit_;
    bool initSsl_;
};



#endif