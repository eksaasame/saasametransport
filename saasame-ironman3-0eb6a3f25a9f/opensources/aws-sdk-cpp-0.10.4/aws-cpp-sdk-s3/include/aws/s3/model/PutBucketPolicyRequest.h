/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/
#pragma once
#include <aws/s3/S3_EXPORTS.h>
#include <aws/s3/S3Request.h>
#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
namespace S3
{
namespace Model
{

  /**
   */
  class AWS_S3_API PutBucketPolicyRequest : public StreamingS3Request
  {
  public:
    PutBucketPolicyRequest();
    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;

    inline bool ShouldComputeContentMd5() const override { return true; }

    
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    
    inline PutBucketPolicyRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    
    inline PutBucketPolicyRequest& WithBucket(Aws::String&& value) { SetBucket(value); return *this;}

    
    inline PutBucketPolicyRequest& WithBucket(const char* value) { SetBucket(value); return *this;}

    
    inline const Aws::String& GetContentMD5() const{ return m_contentMD5; }

    
    inline void SetContentMD5(const Aws::String& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = value; }

    
    inline void SetContentMD5(Aws::String&& value) { m_contentMD5HasBeenSet = true; m_contentMD5 = value; }

    
    inline void SetContentMD5(const char* value) { m_contentMD5HasBeenSet = true; m_contentMD5.assign(value); }

    
    inline PutBucketPolicyRequest& WithContentMD5(const Aws::String& value) { SetContentMD5(value); return *this;}

    
    inline PutBucketPolicyRequest& WithContentMD5(Aws::String&& value) { SetContentMD5(value); return *this;}

    
    inline PutBucketPolicyRequest& WithContentMD5(const char* value) { SetContentMD5(value); return *this;}

  private:
    Aws::String m_bucket;
    bool m_bucketHasBeenSet;
    Aws::String m_contentMD5;
    bool m_contentMD5HasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
