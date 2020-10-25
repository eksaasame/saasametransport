
#include "aliyun_upload.h"
#include <time.h>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include "..\notice\http_client.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>

using namespace macho;
using namespace macho::windows;

std::string aliyun_upload::encode64(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

std::string aliyun_upload::get_time(){
    std::string fmt = "%a, %d %b %Y %H:%M:%S GMT";
    char outstr[MAX_PATH];
    time_t t = time(NULL);
    struct tm tmp;
    errno_t err = gmtime_s(&tmp, &t);
    memset(outstr, 0, sizeof(outstr));
    if (strftime(outstr, sizeof(outstr), fmt.c_str(), &tmp) == 0)
        return "";
    else
        return std::string(outstr);
}

std::string aliyun_upload::gen_signature(std::string VERB, std::string Content_Type, std::string Date, std::string BucketName, std::string ObjectName, std::string Parameters){
    std::string CanonicalizedResource = "/" + BucketName + "/" + ObjectName + Parameters;
    std::string Content_MD5, CanonicalizedOSSHeaders;
    std::string str(VERB + "\n"
        + Content_MD5 + "\n"
        + Content_Type + "\n"
        + Date + "\n"
        + CanonicalizedOSSHeaders
        + CanonicalizedResource);
    return std::string("OSS ") + _access_key_id + ":" + encode64(macho::sha1::hmac_sha1(str, _access_key_secret));
}

bool aliyun_upload::upload(uint64_t start, uint32_t block_number, LPBYTE buf, uint32_t length){
    if (!_terminated && !_is_error){
        bool _wait = false;
        if (0 == start){
            boost::unique_lock<boost::recursive_mutex> lck(_cs);
            if (0 == _uploaded_parts.count(1)){
                block::ptr header(new block(start, length, 1));
                header->length = length;
                memcpy(&(header->buf.get()[0]), buf, length);
                _queue.push_back(header);
                _cond.notify_one();
            }
            _wait = _queue.size() > _queue_size + QUEUE_SIZE;
        }
        else{          
            
            if (_blk && (_blk->length + length) > _part_size){
                boost::unique_lock<boost::recursive_mutex> lck(_cs);
                if (0 == _uploaded_parts.count(_blk->part_number)){
                    _queue.push_back(_blk);
                    if (!_is_uploading && _queue.size() >= _queue_size){
                        _is_uploading = true;
                        _cond.notify_all();
                    }
                    else{
                        _cond.notify_one();
                    }
                }
                _blk = NULL;
                _wait = _queue.size() > _queue_size + QUEUE_SIZE;
            }

            if (NULL == _blk)
                _blk = block::ptr(new block(start, _part_size, ++_part_number));
            if (_blk){
                _blk->length += length;
                memcpy(&(_blk->buf.get()[start - _blk->start]), buf, length);
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Failed to new memory buffer.");
                return false;
            }
        }
        
        while (_wait && !_terminated && !_is_error){
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            {
                boost::unique_lock<boost::recursive_mutex> lck(_cs);
                _wait = _queue.size() > _queue_size + QUEUE_SIZE;
            }
        }
    }
    return !_terminated && !_is_error;
}

void aliyun_upload::flush(){
    if (_blk){
        boost::unique_lock<boost::recursive_mutex> lck(_cs);
        _is_uploading = true;
        if (0 == _uploaded_parts.count(_blk->part_number)){
            _queue.push_back(_blk);
            _cond.notify_all();
        }
        _blk = NULL;
    }
}

void aliyun_upload::_upload(){
    block::ptr blk = NULL;
    http_client http;
    while (!_terminated && !_is_error){
        {
            boost::unique_lock<boost::mutex> _lock(_mutex);     
            while (!_terminated && (!_queue.size() || !_is_uploading) && blk == NULL)
                _cond.timed_wait(_lock, boost::posix_time::seconds(30));
            if (_is_uploading){
                boost::unique_lock<boost::recursive_mutex> lck(_cs);
                if (blk){
                    _uploading_blks.erase(blk->part_number);
                    _uploaded_parts[blk->part_number] = blk->etag;
                    blk = NULL;
                }       
                if (_queue.size()){
                    blk = _queue.front();
                    _queue.pop_front();
                    _uploading_blks[blk->part_number] = blk;
                }
            }
            else if(blk){
                boost::unique_lock<boost::recursive_mutex> lck(_cs);
                _uploading_blks.erase(blk->part_number);
                _uploaded_parts[blk->part_number] = blk->etag;
                blk = NULL;
                continue;
            }
        }
        if (!_terminated && !_is_error && blk){
            if (!upload(http,blk)){
                boost::unique_lock<boost::recursive_mutex> lck(_cs);
                _is_error = true;
            }
        }
    }
    _cond.notify_all();
}

bool aliyun_upload::upload(http_client& http, block::ptr blk){
    std::string VERB = "PUT";
    std::string Content_Type = "application/octet-stream";
    std::string Date = get_time();
    std::string req = boost::str(boost::format("?partNumber=%1%&uploadId=%2%") % blk->part_number %_upload_id);
    std::vector<std::string> header;
    header.push_back("Content-Type: " + Content_Type);
    header.push_back("Date: " + Date);
    header.push_back("Authorization: " + gen_signature(VERB, Content_Type, Date, _bucket_name, _object_name , req));
    std::string out, _header;
    long resp = 0;
    CURLcode code = http.put(_url + req, header, (const char*)blk->buf.get(), blk->length, resp, out, _header);
    if (resp == 200){
        std::vector<std::string> headers = macho::stringutils::tokenize2(_header, "\r\n", false);
        foreach(std::string &h, headers){
            std::vector<std::string> values = macho::stringutils::tokenize2(h, ":", 2);
            if (values.size() > 1 && values[0] == "ETag"){
                blk->etag = values[1];
                _be_uploaded(blk->part_number, blk->etag, blk->length);
                return true;
            }
        }
    }
    LOG(LOG_LEVEL_ERROR, L"error code (%d) : %s", resp ? resp : code, macho::stringutils::convert_ansi_to_unicode(out).c_str());
    return false;
}

bool aliyun_upload::wait(){
    bool _wait = true;
    while (_wait && !_terminated && !_is_error){
        boost::this_thread::sleep(boost::posix_time::seconds(1));        
        {
            boost::unique_lock<boost::recursive_mutex> lck(_cs);
            _is_uploading = true;
            _cond.notify_all();
            _wait = (_uploading_blks.size() || _queue.size());
        }
    }
    return !_terminated && !_is_error;
}

bool aliyun_upload::abort(){
    bool result = _upload_id.empty();
    if (!result){
        terminated();
        std::string VERB = "DELETE";
        std::string Content_Type = "application/octet-stream";
        std::string Date = get_time();
        std::string req = "?uploadId="+ _upload_id;
        std::vector<std::string> header;
        header.push_back("Content-Type: " + Content_Type);
        header.push_back("Date: " + Date);
        header.push_back("Authorization: " + gen_signature(VERB, Content_Type, Date, _bucket_name, _object_name , req));
        http_client http;
        std::string out;
        long resp = 0;
        CURLcode code = http.del(_url + req, header, "", resp, out);
        if (resp == 200 || resp == 204 || resp == 404){
            LOG(LOG_LEVEL_RECORD, L"Succeeded to abort (%s) : %s",
                macho::stringutils::convert_ansi_to_unicode(_url).c_str(),
                macho::stringutils::convert_ansi_to_unicode(out).c_str());
            return true;
        }
        LOG(LOG_LEVEL_ERROR, L"error code (%d) : %s", resp ? resp : code, macho::stringutils::convert_ansi_to_unicode(out).c_str());
        return false;
    }
    return result;
}

bool aliyun_upload::complete(){
    bool result = false;
    flush();
    if (result = wait()){
        std::string VERB = "POST";
        std::string Content_Type = "application/xml";
        std::string Date = get_time();
        std::string req = "?uploadId=" + _upload_id;
        std::vector<std::string> header;
        header.push_back("Content-Type: " + Content_Type);
        header.push_back("Date: " + Date);
        header.push_back("Authorization: " + gen_signature(VERB, Content_Type, Date, _bucket_name, _object_name , req));
        http_client http;
        std::string in,out;
        long resp = 0;
        in.append("<CompleteMultipartUpload>\n");
        typedef std::map<int, std::string> _part_map;
        foreach(_part_map::value_type& p, _uploaded_parts){
            in.append(boost::str(boost::format("<Part><PartNumber>%1%</PartNumber><ETag>%2%</ETag></Part>") % p.first%p.second));
        }
        in.append("</CompleteMultipartUpload>\n");
        CURLcode code = http.post(_url + req, header, in, resp, out);
        if (resp == 200){
            LOG(LOG_LEVEL_RECORD, L"Succeeded to complete (%s) : %s", 
                macho::stringutils::convert_ansi_to_unicode(_url).c_str(),
                macho::stringutils::convert_ansi_to_unicode(out).c_str());
            return true;
        }
        LOG(LOG_LEVEL_ERROR, L"error code (%d) : %s", resp ? resp : code, macho::stringutils::convert_ansi_to_unicode(out).c_str());
        return false;
    }
    return result;
}

bool aliyun_upload::initial(){
    bool result = !_upload_id.empty();
    if (!result){
        std::string VERB = "POST";
        std::string Content_Type = "application/octet-stream";
        std::string Date = get_time();
        std::string req = "?uploads";
        std::vector<std::string> header;
        header.push_back("Content-Type: " + Content_Type);
        header.push_back("Date: " + Date);
        header.push_back("Authorization: " + gen_signature(VERB, Content_Type, Date, _bucket_name, _object_name, req));
        http_client http;
        std::string out;
        long resp = 0;
        CURLcode code = http.post(_url + req, header, "", resp, out);
        if (resp == 200){
            boost::property_tree::ptree response;
            std::stringstream s;
            s << (out);
            boost::property_tree::read_xml(s, response);
            foreach(boost::property_tree::ptree::value_type &v,
                response.get_child("InitiateMultipartUploadResult"))
            {
                if (v.first == "UploadId"){
                    LOG(LOG_LEVEL_RECORD, L"url : %s - UploadId : %s"
                        , macho::stringutils::convert_ansi_to_unicode(_url).c_str()
                        , macho::stringutils::convert_ansi_to_unicode(v.second.data()).c_str());
                    _upload_id = v.second.data();
                    return true;
                }
            }
        }
        LOG(LOG_LEVEL_ERROR, L"url : %s - error code (%d) : %s"
            , macho::stringutils::convert_ansi_to_unicode(_url).c_str()
            , resp ? resp : code
            , macho::stringutils::convert_ansi_to_unicode(out).c_str());
        return false;
    }
    return result;
}

std::string tencent_upload::gen_signature(std::string VERB, std::string Content_Type, std::string Date, std::string BucketName, std::string ObjectName, std::string Parameters){

    boost::posix_time::time_duration t = boost::posix_time::second_clock::universal_time() - boost::posix_time::time_from_string("1970-01-01 00:00:00");
    std::string signtime = boost::str(boost::format("%1%;%2%") % (t.total_seconds() - 60) % (t.total_seconds() + 3600));
    std::string sign_key = byte_to_string(macho::sha1::hmac_sha1(signtime, _access_key_secret));
    std::string http_string = boost::str(boost::format("%1%\n%2%\n\nhost=%3%\n") % macho::stringutils::tolower(VERB) % ("/" + ObjectName) % (_bucket_name + ".cos." + _region + ".myqcloud.com"));
    std::string sha1ed_http_string = byte_to_string(macho::sha1::_sha1(http_string));
    std::string string_to_sign = boost::str(boost::format("sha1\n%1%\n%2%\n") % signtime % sha1ed_http_string);
    std::string signature = byte_to_string(macho::sha1::hmac_sha1(string_to_sign, sign_key));

    return boost::str(boost::format("q-sign-algorithm=sha1&q-ak=%1%&q-sign-time=%2%&q-key-time=%3%&q-header-list=host&q-url-param-list=&q-signature=%4%") 
        % _access_key_id % signtime % signtime % signature);
}

std::string tencent_upload::byte_to_string(std::string& src){
    std::ostringstream out;
    out.fill('0');
    out << std::hex;
    for (std::string::const_iterator i = src.begin(), n = src.end(); i != n; ++i) {
        std::string::value_type c = (*i);
        out << std::setw(2) << int((unsigned char)c);
    }
    return out.str();
}