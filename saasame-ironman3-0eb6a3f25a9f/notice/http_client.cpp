
#include "stdafx.h"
#include "libssh2.h"
#include "http_client.h"
#include "macho.h"
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")    
#pragma comment(lib, "wsock32.lib")    
#pragma comment(lib, "Secur32.lib")  
#pragma comment(lib, "libssh2.lib")  
#pragma comment(lib, "ssleay32.lib")  
#pragma comment(lib, "libeay32.lib")  
#pragma comment(lib, "wldap32.lib") 
#if _DEBUG
#pragma comment(lib, "libcurl.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif
#pragma comment(lib, "zlib.lib")    

#define CURL_FUNC(x) \
{ \
    CURLcode code = x; \
    if(CURLE_OK != code) \
        return code; \
}

namespace
{

    class CurlStr
    {
    public:
        CurlStr(char* str)
            : str_(str)
        {
        }

        ~CurlStr()
        {
            curl_free(str_);
        }

        operator const char*()
        {
            return str_;
        }

    private:
        char* str_;
    };

}

struct WriteThis {
    const char *readptr;
    long sizeleft;
};

http_client::http_client(bool initSsl)
    : curl_(NULL)
    , globalInit_(false)
    , initSsl_(initSsl)
{
}

http_client::~http_client()
{
    if (curl_)
    {
        curl_easy_cleanup(curl_);
    }
    if (globalInit_)
    {
        //curl_global_cleanup();
    }
}

CURLcode http_client::init()
{
    if (!globalInit_)
    {
        CURL_FUNC(curl_global_init(initSsl_ ? CURL_GLOBAL_ALL : CURL_GLOBAL_WIN32));
        globalInit_ = true;
    }

    if (!curl_)
    {
        curl_ = curl_easy_init();
        if (!curl_)
        {
            return CURLE_FAILED_INIT;
        }

        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L));
    }

    return CURLE_OK;
}

CURLcode http_client::setUsernamePassword(const std::string& username, const std::string& password)
{
    if (!username.empty() && !password.empty())
    {
        std::string userpass;
        macho::stringutils::format(userpass, "%s:%s", username.c_str(), password.c_str());
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_USERPWD, userpass.c_str()));
    }

    return CURLE_OK;
}

CURLcode http_client::get(const std::wstring& url, const std::wstring& username, const std::wstring& password, long& resp, std::vector<char>& buffer)
{
    std::string sUrl;
    std::string sUsername;
    std::string sPassword;

    sUrl = macho::stringutils::convert_unicode_to_utf8(url);
    sUsername = macho::stringutils::convert_unicode_to_utf8(username);
    sPassword = macho::stringutils::convert_unicode_to_utf8(password);

    CURL_FUNC(get(sUrl, sUsername, sPassword, resp, buffer));

    return CURLE_OK;
}

CURLcode http_client::get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::vector<char>& buffer)
{
    CURL_FUNC(init());
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()));
    CURL_FUNC(setUsernamePassword(username, password));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, FALSE));
    CURL_FUNC(curl_easy_perform(curl_));
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp));

    return CURLE_OK;
}

CURLcode http_client::get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::fstream& outfilep)
{
    CURL_FUNC(init());
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()));
    CURL_FUNC(setUsernamePassword(username, password));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFileFunction));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &outfilep));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, FALSE));
    CURL_FUNC(curl_easy_perform(curl_));
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp));

    return CURLE_OK;
}

CURLcode http_client::get(const std::wstring& url, const std::wstring& username, const std::wstring& password, long& resp, std::wstring& str)
{
    std::vector<char> buffer;

    CURL_FUNC(get(url, username, password, resp, buffer));

    buffer.push_back('\0');
    str = macho::stringutils::convert_utf8_to_unicode(&buffer.at(0));

    return CURLE_OK;
}

CURLcode http_client::get(const std::string& url, const std::string& username, const std::string& password, long& resp, std::string& str)
{
    std::vector<char> buffer;

    CURL_FUNC(get(url, username, password, resp, buffer));

    buffer.push_back('\0');
    str = &buffer.at(0);

    return CURLE_OK;
}

std::wstring& http_client::escapeStr(std::wstring& str)
{
    std::string utf8;
    return str = macho::stringutils::convert_utf8_to_unicode(escapeStr(utf8 = macho::stringutils::convert_unicode_to_utf8(str)));
}

std::string& http_client::escapeStr(std::string& str)
{
    if (CURLE_OK == init())
    {
        CurlStr encoded(curl_easy_escape(curl_, str.c_str(), static_cast<int>(str.size())));
        if (encoded)
        {
            str = encoded;
        }
    }

    return str;
}

std::wstring& http_client::getErrorStr(CURLcode code, std::wstring& str)
{
    std::string utf8;
    getErrorStr(code, utf8);
    return str = macho::stringutils::convert_utf8_to_unicode(utf8);
}

std::string& http_client::getErrorStr(CURLcode code, std::string& str)
{
    return str = curl_easy_strerror(code);
}

size_t http_client::writeFunction(char* ptr, size_t size, size_t nmemb, std::vector<char>* buffer)
{
    if (ptr && buffer)
    {
        size_t count = size * nmemb;
        size_t oldSize = buffer->size();
        buffer->resize(oldSize + count, 0);
        memcpy(&buffer->at(oldSize), ptr, count);
        return count;
    }
    else
    {
        return 0;
    }
}

size_t http_client::writeFileFunction(char* ptr, size_t size, size_t nmemb, std::fstream *outfilep)
{
    if (ptr && outfilep)
    {
        size_t count = size * nmemb;
        outfilep->write(ptr, count);
        return count;
    }
    else
    {
        return 0;
    }
}

size_t http_client::readFunction(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct WriteThis *pooh = (struct WriteThis *)userp;
    uint32_t len = size*nmemb;

    if (size*nmemb < 1)
        return 0;
    if (pooh->sizeleft <= 0)
        return 0;
    
    if (len > pooh->sizeleft) {
        len = pooh->sizeleft;
    }

    memcpy(ptr, &pooh->readptr[0], len);
    pooh->readptr += len;
    pooh->sizeleft-= len;
    return len;
}

CURLcode http_client::post(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& out, std::string& response_headers, int port){
    return post(url, headers, in.c_str(), in.length(), resp, out, response_headers, port);
}

CURLcode http_client::post(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& out, std::string& response_headers, int port){
    auto sListDeleter = [](struct curl_slist* __headers){curl_slist_free_all(__headers); };
    struct curl_slist* _headers = NULL;
    std::vector<char> buffer;
    CURL_FUNC(init());
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()));

    foreach(const std::string &header, headers)
        _headers = curl_slist_append(_headers, header.c_str());
   
#if _DEBUG
    /* get verbose debug output please */
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
#endif
    
    curl_easy_setopt(curl_, CURLOPT_HEADER, 1L);
#if 1
    if (in && in_size){
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, in));
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, in_size));
    }
    else{
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, NULL));
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, 0));
    }
#else
    struct WriteThis pooh;
    pooh.readptr = in.c_str();
    pooh.sizeleft = (long)in.length();

    /* we want to use our own read function */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_READFUNCTION, readFunction));

    /* pointer to pass to our read function */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_READDATA, &pooh));

    //_headers = curl_slist_append(_headers, "Transfer-Encoding: chunked");
    _headers = curl_slist_append(_headers, boost::str(boost::format("Content-Length: %d") % pooh.sizeleft).c_str());
    //_headers = curl_slist_append(_headers, "Expect:");
#endif

    std::unique_ptr<struct curl_slist, decltype(sListDeleter)> _headers_ptr(_headers, sListDeleter);

    /* set curl options */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, FALSE));

    if (port > 0){
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_PORT, port));
    }

    /* set default user agent */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_USERAGENT, "libcurl-agent/1.0"));

    /* Now specify we want to POST data */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POST, 1L));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, _headers));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer));

    /* set timeout */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 600));

    /* enable location redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1));

    /* set maximum allowed redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 1));
    CURL_FUNC(curl_easy_perform(curl_));
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp));
    int header_size = 0;
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_HEADER_SIZE, &header_size));
    
    buffer.push_back('\0');
    response_headers = std::string(&buffer.at(0), header_size);
    out = &buffer.at(header_size);
    return CURLE_OK;
}

CURLcode http_client::put(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& out, std::string& response_headers, int port){
    return put(url, headers, in.c_str(), in.length(), resp, out, response_headers, port);
}

CURLcode http_client::put(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& out, std::string& response_headers, int port){
    auto sListDeleter = [](struct curl_slist* __headers){curl_slist_free_all(__headers); };
    struct curl_slist* _headers = NULL;
    std::vector<char> buffer;
    CURL_FUNC(init());
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()));

    foreach(const std::string &header, headers)
        _headers = curl_slist_append(_headers, header.c_str());

#if _DEBUG
    /* get verbose debug output please */
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
#endif

    curl_easy_setopt(curl_, CURLOPT_HEADER, 1L);

#if 0
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, in));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, in_size));
#else
    struct WriteThis pooh;
    pooh.readptr = in;
    pooh.sizeleft = (long)in_size;

    /* we want to use our own read function */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_READFUNCTION, readFunction));

    /* pointer to pass to our read function */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_READDATA, &pooh));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_INFILESIZE, in_size));

#endif

    //_headers = curl_slist_append(_headers, "Expect:");
    std::unique_ptr<struct curl_slist, decltype(sListDeleter)> _headers_ptr(_headers, sListDeleter);

    /* set curl options */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, TRUE));
    //CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, TRUE));

    if (port > 0){
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_PORT, port));
    }

    /* set default user agent */
    //CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_USERAGENT, "libcurl-agent/1.0"));

    /* Now specify we want to PUT data */
#if 0    
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT"));
#else
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_PUT, 1L));
#endif
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, _headers));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer));

    /* set timeout */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 600));

    /* enable location redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1));

    /* set maximum allowed redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 1));
    CURL_FUNC(curl_easy_perform(curl_));
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp));
    int header_size = 0;
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_HEADER_SIZE, &header_size));

    buffer.push_back('\0');
    response_headers = std::string(&buffer.at(0), header_size);
    out = &buffer.at(header_size);
    return CURLE_OK;
}

CURLcode http_client::del(const std::string& url, const std::vector<std::string>& headers, const std::string& in, long& resp, std::string& response_headers, int port){
    return del(url, headers, in.c_str(), in.length(), resp, response_headers, port);
}

CURLcode http_client::del(const std::string& url, const std::vector<std::string>& headers, long& resp, std::string& response_headers, int port){
    return del(url, headers, NULL, 0, resp, response_headers, port);
}

CURLcode http_client::del(const std::string& url, const std::vector<std::string>& headers, const char* in, const size_t in_size, long& resp, std::string& response_headers, int port){
    auto sListDeleter = [](struct curl_slist* __headers){curl_slist_free_all(__headers); };
    struct curl_slist* _headers = NULL;
    std::vector<char> buffer;
    CURL_FUNC(init());
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()));

    foreach(const std::string &header, headers)
        _headers = curl_slist_append(_headers, header.c_str());

#if _DEBUG
    /* get verbose debug output please */
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
#endif

    curl_easy_setopt(curl_, CURLOPT_HEADER, 1L);
    
    if (in && in_size){
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, in));
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, in_size));
    }
    else{
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, NULL));
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, 0));
    }

    std::unique_ptr<struct curl_slist, decltype(sListDeleter)> _headers_ptr(_headers, sListDeleter);

    /* set curl options */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, FALSE));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, FALSE));

    if (port > 0){
        CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_PORT, port));
    }

    /* set default user agent */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_USERAGENT, "libcurl-agent/1.0"));

    /* Now specify we want to DELETE data */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE"));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, _headers));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction));
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer));

    /* set timeout */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 600));

    /* enable location redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1));

    /* set maximum allowed redirects */
    CURL_FUNC(curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 1));
    CURL_FUNC(curl_easy_perform(curl_));
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp));
    int header_size = 0;
    CURL_FUNC(curl_easy_getinfo(curl_, CURLINFO_HEADER_SIZE, &header_size));

    buffer.push_back('\0');
    response_headers = std::string(&buffer.at(0), header_size);
    return CURLE_OK;
}
