#pragma once

#ifndef _ALIYUN_UPLOAD_H_
#define _ALIYUN_UPLOAD_H_ 1

#include "macho.h"
#include "..\notice\http_client.h"
#define QUEUE_SIZE 2

class aliyun_upload{
public:
    typedef boost::signals2::signal<void(const uint32_t, const std::string, const uint32_t length)> block_to_be_uploaded;

    aliyun_upload(std::string region
        , std::string bucket_name
        , std::string object_name
        , std::string access_key_id
        , std::string access_key_secret
        , size_t size
        , std::string& upload_id
        , std::map<int, std::string> &uploaded_parts
        , int number_of_upload_threads = 0
        , bool is_https = false)
        : _region(region)
        , _bucket_name(bucket_name)
        , _object_name(object_name)
        , _access_key_id(access_key_id)
        , _access_key_secret(access_key_secret)
        , _is_error(false)
        , _terminated(false)
        , _is_uploading(false)
        , _upload_id(upload_id)
        , _uploaded_parts(uploaded_parts)
        , _part_number(1)
        {
            _queue_size = number_of_upload_threads > 0 ? number_of_upload_threads : boost::thread::hardware_concurrency();
            for (std::size_t i = 0; i < _queue_size; ++i){
                _threads.push_back(_thread_pool.create_thread(boost::bind(&aliyun_upload::_upload, this)));
            }
            _part_size = size / 5000;
            if (_part_size < 0x500000)
                _part_size = 0x500000;
            uint64_t q = 0x8000000LL / _part_size;
            if (_queue_size < q)
                _queue_size = q;

            _url = (is_https ? "Https://" : "Http://") + _bucket_name + "." + _region + ".aliyuncs.com" + "/" + _object_name;
        }

    virtual ~aliyun_upload(){
        wait();
        terminated();
    }
    bool upload(uint64_t start, uint32_t block_number, LPBYTE buf, uint32_t length);
    bool is_error() const { return _is_error; }
    inline void register_block_to_be_uploaded_callback_function(block_to_be_uploaded::slot_type slot){
        _be_uploaded.connect(slot);
    }
    bool wait();
    bool complete();
    bool abort();
    bool initial();
protected:
	struct block{
		typedef boost::shared_ptr<block> ptr;
		typedef std::deque<ptr> queue;
		typedef std::map<int, ptr> map;
		block(uint64_t _start, uint32_t size, uint32_t num) : start(_start), part_number(num), length(0){
			buf = boost::shared_ptr<BYTE>(new BYTE[size]);
			memset(buf.get(), 0, size);
		}
		uint64_t                start;
		uint32_t                part_number;
		boost::shared_ptr<BYTE> buf;
		uint32_t                length;
		std::string             etag;
	};
    void flush();
    void terminated(){
        _terminated = true;
        _cond.notify_all();
        _thread_pool.join_all();
        foreach(boost::thread* t, _threads)
            _thread_pool.remove_thread(t);
        _threads.clear();
    }
    void _upload();
    bool upload(http_client& http, block::ptr blk);
    virtual std::string gen_signature(std::string VERB, std::string Content_Type, std::string Date, std::string BucketName, std::string ObjectName, std::string Parameters);
    std::string encode64(const std::string &val);
    std::string get_time();
    std::string                      _url;
    std::string                      _region;
    std::string                      _bucket_name;
    std::string                      _object_name;
    std::string                      _access_key_id;
    std::string                      _access_key_secret;
    boost::recursive_mutex           _cs;
    boost::condition_variable        _cond;
    boost::mutex                     _mutex;
    std::size_t                      _queue_size;
    std::size_t                      _part_size;
    block::queue                     _queue;
    bool                             _terminated;
    bool                             _is_error;
    bool                             _is_uploading;
    boost::thread_group              _thread_pool;
    std::vector<boost::thread*>      _threads;
    block_to_be_uploaded             _be_uploaded;
    block::map                       _uploading_blks;
    std::string&                     _upload_id;
    std::map<int, std::string>&      _uploaded_parts;
    uint32_t                         _part_number;
    block::ptr                       _blk;
};

class tencent_upload : public virtual aliyun_upload{
public:
    tencent_upload(std::string region
        , std::string bucket_name
        , std::string object_name
        , std::string access_key_id
        , std::string access_key_secret
        , size_t size
        , std::string& upload_id
        , std::map<int, std::string> &uploaded_parts
        , int number_of_upload_threads = 0
        , bool is_https = false)
        : aliyun_upload(region, bucket_name, object_name, access_key_id, access_key_secret, size, upload_id, uploaded_parts, number_of_upload_threads)      
    {
        _url = (is_https ? "Https://" : "Http://")  + _bucket_name + ".cos." + _region + ".myqcloud.com" + "/" + _object_name;
    }
    virtual ~tencent_upload(){
        wait();
        terminated();
    }
protected:
    std::string byte_to_string(std::string& src);
    virtual std::string gen_signature(std::string VERB, std::string Content_Type, std::string Date, std::string BucketName, std::string ObjectName, std::string Parameters);
};

#endif