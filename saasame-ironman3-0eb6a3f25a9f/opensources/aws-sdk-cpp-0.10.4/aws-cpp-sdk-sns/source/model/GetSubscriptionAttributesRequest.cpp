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
#include <aws/sns/model/GetSubscriptionAttributesRequest.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

using namespace Aws::SNS::Model;
using namespace Aws::Utils;

GetSubscriptionAttributesRequest::GetSubscriptionAttributesRequest() : 
    m_subscriptionArnHasBeenSet(false)
{
}

Aws::String GetSubscriptionAttributesRequest::SerializePayload() const
{
  Aws::StringStream ss;
  ss << "Action=GetSubscriptionAttributes&";
  if(m_subscriptionArnHasBeenSet)
  {
    ss << "SubscriptionArn=" << StringUtils::URLEncode(m_subscriptionArn.c_str()) << "&";
  }
  ss << "Version=2010-03-31";
  return ss.str();
}

