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
#include <aws/ecr/ECR_EXPORTS.h>
#include <aws/ecr/model/Repository.h>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Json
{
  class JsonValue;
} // namespace Json
} // namespace Utils
namespace ECR
{
namespace Model
{
  class AWS_ECR_API CreateRepositoryResult
  {
  public:
    CreateRepositoryResult();
    CreateRepositoryResult(const AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);
    CreateRepositoryResult& operator=(const AmazonWebServiceResult<Aws::Utils::Json::JsonValue>& result);

    
    inline const Repository& GetRepository() const{ return m_repository; }

    
    inline void SetRepository(const Repository& value) { m_repository = value; }

    
    inline void SetRepository(Repository&& value) { m_repository = value; }

    
    inline CreateRepositoryResult& WithRepository(const Repository& value) { SetRepository(value); return *this;}

    
    inline CreateRepositoryResult& WithRepository(Repository&& value) { SetRepository(value); return *this;}

  private:
    Repository m_repository;
  };

} // namespace Model
} // namespace ECR
} // namespace Aws
