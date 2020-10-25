
#define NOMINMAX 

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/Outcome.h>
#include <aws/s3/S3Client.h>
#include <aws/core/utils/ratelimiter/DefaultRateLimiter.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/UploadPartRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/GetBucketLocationRequest.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpClient.h>

#include <fstream>

#include "aws_s3.h"

#ifdef _WIN32
#pragma warning(disable: 4127)
#endif //_WIN32

#include <aws/core/http/standard/StandardHttpRequest.h>

using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::Utils;
using namespace macho;

#pragma comment(lib, "aws-cpp-sdk-core.lib" )
#pragma comment(lib, "aws-cpp-sdk-s3.lib" )

static const int TIMEOUT_MAX = 10;

class aws_s3_op : aws_s3{
public:
    aws_s3_op(std::string bucket, std::string access_key_id, std::string secret_key, Aws::Region region, std::string proxy_host, uint32_t proxy_port, std::string proxy_username, std::string proxy_password, int32_t timeout)
        : _region(region), _access_key_id(access_key_id), _secret_key(secret_key), _proxy_host(proxy_host), _proxy_port(proxy_port), _proxy_username(proxy_username), _proxy_password(proxy_password), _timeout(timeout){
        _bucket = bucket.c_str();
    }
    virtual bool create_dir(std::string _path);
    virtual bool remove_dir(std::string _path);
    virtual bool is_dir_existing(std::string _path);

    virtual bool is_existing(std::string _path);
    virtual bool get_file(std::string _path, std::ostream& data);
    virtual bool put_file(std::string _path, std::istream& data);
    virtual bool remove(std::string _path);
    virtual bool create_empty_file(std::string _path);
    virtual item::vtr enumerate_sub_items(std::string _path);
    virtual aws_s3::ptr clone();
    virtual std::string key_id() { return _access_key_id.string(); }
    virtual std::string bucket() const { return _bucket.c_str(); }
    static aws_s3::ptr connect(std::string bucket, std::string id, std::string key, Aws::Region region, std::string proxy_host, uint32_t proxy_port, std::string proxy_username, std::string proxy_password, int32_t timeout);
private:
    static const char* ALLOCATION_TAG;
    void                                                          set_up();
    bool                                                          check_or_create_bucket();
    bool                                                          wait_for_object_to_propagate(const char* object_key);
    bool                                                          wait_for_bucket_to_propagate(const Aws::String& bucket_name);
    UploadPartOutcomeCallable                                     make_upload_part_outcome_and_get_callable(unsigned partNumber, const ByteBuffer& md5OfStream,
                                                                                                            const std::shared_ptr<Aws::IOStream>& partStream,
                                                                                                            const Aws::String& bucketName, const char* objectName, const Aws::String& uploadId);
    macho::windows::critical_section                              _cs;
    Aws::String                                                   _bucket;
    Aws::Region                                                   _region;
    macho::windows::protected_data                                _access_key_id;
    macho::windows::protected_data                                _secret_key;
    std::string                                                   _proxy_host;
    uint32_t                                                      _proxy_port;
    macho::windows::protected_data                                _proxy_username;
    macho::windows::protected_data                                _proxy_password;
    int32_t                                                       _timeout;
    std::shared_ptr<S3Client>                                     _client;
    std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> _limiter;
};

const char* aws_s3_op::ALLOCATION_TAG = "aws_s3_op";

void aws_s3_op::set_up(){
    _limiter = Aws::MakeShared<Aws::Utils::RateLimits::DefaultRateLimiter<>>(ALLOCATION_TAG, 50000000);
    // Create a client
    ClientConfiguration config;
    config.region = _region;
    config.scheme = Scheme::HTTPS;
    config.connectTimeoutMs = 30000;
    config.requestTimeoutMs = _timeout * 1000;
    config.readRateLimiter = _limiter;
    config.writeRateLimiter = _limiter;
    ////to use a proxy, uncomment the next two lines.
    if (_proxy_host.length() && _proxy_port )
    {
        config.proxyHost = _proxy_host.c_str();
        config.proxyPort = _proxy_port;
        if (_proxy_username.string().length() && _proxy_password.string().length())
        {
            config.proxyUserName = _proxy_username.string().c_str();
            config.proxyPassword = _proxy_password.string().c_str();
        }
    }
    _client = Aws::MakeShared<S3Client>(ALLOCATION_TAG, AWSCredentials(_access_key_id.string().c_str(), _secret_key.string().c_str()), config);
}

bool aws_s3_op::check_or_create_bucket(){
    bool result = false;
    HeadBucketRequest headBucketRequest;
    headBucketRequest.SetBucket(_bucket);
    HeadBucketOutcome headBucketOutcome = _client->HeadBucket(headBucketRequest);
    if (headBucketOutcome.IsSuccess())
    {
        result = true;
    }
    else{
        CreateBucketRequest createBucketRequest;
        createBucketRequest.SetBucket(_bucket);
        createBucketRequest.SetACL(BucketCannedACL::private_);
        CreateBucketOutcome createBucketOutcome = _client->CreateBucket(createBucketRequest);
        if (createBucketOutcome.IsSuccess()){
            const CreateBucketResult& createBucketResult = createBucketOutcome.GetResult();
            if (!createBucketResult.GetLocation().empty())
                result = wait_for_bucket_to_propagate(_bucket);
        }
        else
        {
#pragma push_macro("GetMessage")
#undef GetMessage
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(createBucketOutcome.GetError().GetMessage().c_str()));
#pragma pop_macro("GetMessage")
        }
    }
    return result;
}

aws_s3::ptr aws_s3_op::connect(std::string bucket, std::string access_key_id, std::string secret_key, Aws::Region region, std::string proxy_host, uint32_t proxy_port, std::string proxy_username, std::string proxy_password, int32_t timeout){
    aws_s3::ptr s3;
    aws_s3_op *op = new aws_s3_op(bucket, access_key_id, secret_key, region, proxy_host, proxy_port, proxy_username, proxy_password, timeout);
    if (op)
    {
        op->set_up();
        if ( op->check_or_create_bucket() )
            s3.reset((aws_s3*)op);
    }
    return s3;
}

aws_s3::item::vtr aws_s3_op::enumerate_sub_items(std::string _path){
    macho::windows::auto_lock lock(_cs);
    aws_s3::item::vtr results;
    if (_path.length() && _path[_path.length() - 1] != '/')
        _path.append("/");
    ListObjectsRequest listObjectsRequest;
    listObjectsRequest.SetBucket(_bucket);
    listObjectsRequest.SetPrefix(_path.c_str());
    ListObjectsOutcome listObjectsOutcome = _client->ListObjects(listObjectsRequest);
    if (listObjectsOutcome.IsSuccess())
    {
        for (const auto& object : listObjectsOutcome.GetResult().GetContents())
        {
            aws_s3::item::ptr i = aws_s3::item::ptr(new aws_s3::item());
            i->name = object.GetKey().c_str();
            i->is_dir = i->name[i->name.length() - 1] == '/';
            if (i->is_dir)
                i->name.erase(i->name.length());
            results.push_back(i);
        }
    }
    else
    {
#pragma push_macro("GetMessage")
#undef GetMessage
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(listObjectsOutcome.GetError().GetMessage().c_str()));
#pragma pop_macro("GetMessage")
    }
    return results;
}

bool aws_s3_op::create_dir(std::string _path)
{
    if (_path.length() && _path[_path.length() - 1] != '/')
        _path.append("/");
    return create_empty_file(_path);
}

bool aws_s3_op::remove_dir(std::string _path)
{
    if (_path.length())
        return remove(_path[_path.length() - 1] != '/' ? _path.append("/") : _path);
    else
        return true;
}

bool aws_s3_op::is_dir_existing(std::string _path)
{
    if (_path.length() && _path[_path.length() - 1] != '/')
        _path.append("/");
    return is_existing(_path);
}

bool aws_s3_op::is_existing(std::string _path)
{
    macho::windows::auto_lock lock(_cs);
    HeadObjectRequest headObjectRequest;
    headObjectRequest.SetKey(_path.c_str());
    headObjectRequest.SetBucket(_bucket);
    auto headObjectOutcome = _client->HeadObject(headObjectRequest);
    if (headObjectOutcome.IsSuccess())
        return true;
    return false;
}

bool aws_s3_op::get_file(std::string _path, std::ostream& data)
{
    macho::windows::auto_lock lock(_cs);
    GetObjectRequest getObjectRequest;
    getObjectRequest.SetBucket(_bucket);
    getObjectRequest.SetKey(_path.c_str());
#pragma push_macro("GetObject")
#undef GetObject
    GetObjectOutcome getObjectOutcome = _client->GetObject(getObjectRequest);
    if (getObjectOutcome.IsSuccess())
    {
        data << getObjectOutcome.GetResult().GetBody().rdbuf();
        return true;
    }
#pragma pop_macro("GetObject")
    return false;
}

bool aws_s3_op::put_file(std::string _path, std::istream& data)
{
    bool result = false;
    macho::windows::auto_lock lock(_cs);
    unsigned fiveMbSize = 5 * 1024 * 1024;
    CreateMultipartUploadRequest createMultipartUploadRequest;
    createMultipartUploadRequest.SetBucket(_bucket);
    createMultipartUploadRequest.SetKey(_path.c_str());
    createMultipartUploadRequest.SetContentType("text/plain");
    CreateMultipartUploadOutcome createMultipartUploadOutcome = _client->CreateMultipartUpload(
        createMultipartUploadRequest);
    if (!createMultipartUploadOutcome.IsSuccess())
    {
#pragma push_macro("GetMessage")
#undef GetMessage
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(createMultipartUploadOutcome.GetError().GetMessage().c_str()));
#pragma pop_macro("GetMessage")
    }
    else{
        CompleteMultipartUploadRequest completeMultipartUploadRequest;
        completeMultipartUploadRequest.SetBucket(_bucket);
        completeMultipartUploadRequest.SetKey(_path.c_str());
        completeMultipartUploadRequest.SetUploadId(createMultipartUploadOutcome.GetResult().GetUploadId());
        CompletedMultipartUpload completedMultipartUpload;

        data.seekg(0, std::ios::end);
        std::stringstream::pos_type offset = data.tellg();
        data.seekg(0, std::ios::beg);
        std::auto_ptr<char> buf = std::auto_ptr<char>(new char[fiveMbSize]);
        int i = 1;
        while (offset > 0)
        {
            std::shared_ptr<Aws::StringStream> streamPtr = Aws::MakeShared<Aws::StringStream>(ALLOCATION_TAG);
            memset(buf.get(), 0, fiveMbSize);
            uint64_t length = fiveMbSize;
            if (offset > fiveMbSize)
                offset -= fiveMbSize;
            else
            {
                length = offset;
                offset = 0;
            }
            data.read(buf.get(), length);
            streamPtr->write(buf.get(), length);
            ByteBuffer Md5(HashingUtils::CalculateMD5(*streamPtr));
            UploadPartOutcomeCallable upload_part_out_come_callable = make_upload_part_outcome_and_get_callable(i, Md5, streamPtr, _bucket, _path.c_str(), createMultipartUploadOutcome.GetResult().GetUploadId());
            CompletedPart completedPart;
            completedPart.SetETag(upload_part_out_come_callable.get().GetResult().GetETag());
            completedPart.SetPartNumber(i);
            completedMultipartUpload.AddParts(completedPart);
            i++;
        }
        completeMultipartUploadRequest.WithMultipartUpload(completedMultipartUpload);
        CompleteMultipartUploadOutcome completeMultipartUploadOutcome = _client->CompleteMultipartUpload(
            completeMultipartUploadRequest);
        result = wait_for_object_to_propagate(_path.c_str());
    }
    return result;
}

bool aws_s3_op::remove(std::string _path)
{
    macho::windows::auto_lock lock(_cs);
    DeleteObjectRequest deleteObjectRequest;
    deleteObjectRequest.SetKey(_path.c_str());
    deleteObjectRequest.SetBucket(_bucket);
    auto deleteObjectOutcome = _client->DeleteObject(deleteObjectRequest);
    if (deleteObjectOutcome.IsSuccess())
        return true;
    else
    {
#pragma push_macro("GetMessage")
#undef GetMessage
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(deleteObjectOutcome.GetError().GetMessage().c_str()));
#pragma pop_macro("GetMessage")
    }
    return false;
}

bool aws_s3_op::create_empty_file(std::string _path)
{
    macho::windows::auto_lock lock(_cs);
    PutObjectRequest putObjectRequest;
    putObjectRequest.SetKey(_path.c_str());
    putObjectRequest.SetBucket(_bucket);
    putObjectRequest.SetACL(ObjectCannedACL::private_);
    putObjectRequest.SetStorageClass(StorageClass::STANDARD);
    auto putObjectOutcome = _client->PutObject(putObjectRequest);
    if (putObjectOutcome.IsSuccess())
        return wait_for_object_to_propagate(_path.c_str());
    else
    {
#pragma push_macro("GetMessage")
#undef GetMessage
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(putObjectOutcome.GetError().GetMessage().c_str()));
#pragma pop_macro("GetMessage")
    }
    return false;
}

aws_s3::ptr aws_s3_op::clone()
{
    aws_s3::ptr s3;
    aws_s3_op* op = new aws_s3_op(_bucket.c_str(), _access_key_id, _secret_key, _region, _proxy_host, _proxy_port, _proxy_username, _proxy_password, _timeout);
    if (op)
    {
        op->set_up();
        s3.reset((aws_s3*)op);
    }
    return s3;
}

bool aws_s3_op::wait_for_object_to_propagate(const char* object_key)
{
    unsigned timeoutCount = 0;
    while (timeoutCount++ < TIMEOUT_MAX)
    {
        HeadObjectRequest headObjectRequest;
        headObjectRequest.SetBucket(_bucket);
        headObjectRequest.SetKey(object_key);
        HeadObjectOutcome headObjectOutcome = _client->HeadObject(headObjectRequest);
        if (headObjectOutcome.IsSuccess())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return false;
}

bool aws_s3_op::wait_for_bucket_to_propagate(const Aws::String& bucket_name)
{
    unsigned timeoutCount = 0;
    while (timeoutCount++ < TIMEOUT_MAX)
    {
        HeadBucketRequest headBucketRequest;
        headBucketRequest.SetBucket(bucket_name);
        HeadBucketOutcome headBucketOutcome = _client->HeadBucket(headBucketRequest);
        if (headBucketOutcome.IsSuccess())
        {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return false;
}

aws_s3::ptr aws_s3::connect(std::string bucket, std::string access_key_id, std::string secret_key, int region, std::string proxy_host, uint32_t proxy_port, std::string proxy_username, std::string proxy_password, int32_t timeout)
{
    return aws_s3_op::connect(bucket, access_key_id, secret_key, (Aws::Region) region, proxy_host, proxy_port, proxy_username, proxy_password, timeout);
}

UploadPartOutcomeCallable aws_s3_op::make_upload_part_outcome_and_get_callable(unsigned partNumber, const ByteBuffer& md5OfStream,
    const std::shared_ptr<Aws::IOStream>& partStream,
    const Aws::String& bucketName, const char* objectName, const Aws::String& uploadId)
{
    UploadPartRequest uploadPart1Request;
    uploadPart1Request.SetBucket(bucketName);
    uploadPart1Request.SetKey(objectName);
    uploadPart1Request.SetPartNumber(partNumber);
    uploadPart1Request.SetUploadId(uploadId);
    uploadPart1Request.SetBody(partStream);
    uploadPart1Request.SetContentMD5(HashingUtils::Base64Encode(md5OfStream));

    auto startingPoint = partStream->tellg();
    partStream->seekg(0LL, partStream->end);
    uploadPart1Request.SetContentLength(static_cast<long>(partStream->tellg()));
    partStream->seekg(startingPoint);

    return _client->UploadPartCallable(uploadPart1Request);
}