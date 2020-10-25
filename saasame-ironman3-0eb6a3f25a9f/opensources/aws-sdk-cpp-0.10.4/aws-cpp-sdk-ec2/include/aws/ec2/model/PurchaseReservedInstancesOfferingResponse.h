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
#include <aws/ec2/EC2_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/ec2/model/ResponseMetadata.h>

namespace Aws
{
template<typename RESULT_TYPE>
class AmazonWebServiceResult;

namespace Utils
{
namespace Xml
{
  class XmlDocument;
} // namespace Xml
} // namespace Utils
namespace EC2
{
namespace Model
{
  class AWS_EC2_API PurchaseReservedInstancesOfferingResponse
  {
  public:
    PurchaseReservedInstancesOfferingResponse();
    PurchaseReservedInstancesOfferingResponse(const AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);
    PurchaseReservedInstancesOfferingResponse& operator=(const AmazonWebServiceResult<Aws::Utils::Xml::XmlDocument>& result);

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline const Aws::String& GetReservedInstancesId() const{ return m_reservedInstancesId; }

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline void SetReservedInstancesId(const Aws::String& value) { m_reservedInstancesId = value; }

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline void SetReservedInstancesId(Aws::String&& value) { m_reservedInstancesId = value; }

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline void SetReservedInstancesId(const char* value) { m_reservedInstancesId.assign(value); }

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline PurchaseReservedInstancesOfferingResponse& WithReservedInstancesId(const Aws::String& value) { SetReservedInstancesId(value); return *this;}

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline PurchaseReservedInstancesOfferingResponse& WithReservedInstancesId(Aws::String&& value) { SetReservedInstancesId(value); return *this;}

    /**
     * <p>The IDs of the purchased Reserved Instances.</p>
     */
    inline PurchaseReservedInstancesOfferingResponse& WithReservedInstancesId(const char* value) { SetReservedInstancesId(value); return *this;}

    
    inline const ResponseMetadata& GetResponseMetadata() const{ return m_responseMetadata; }

    
    inline void SetResponseMetadata(const ResponseMetadata& value) { m_responseMetadata = value; }

    
    inline void SetResponseMetadata(ResponseMetadata&& value) { m_responseMetadata = value; }

    
    inline PurchaseReservedInstancesOfferingResponse& WithResponseMetadata(const ResponseMetadata& value) { SetResponseMetadata(value); return *this;}

    
    inline PurchaseReservedInstancesOfferingResponse& WithResponseMetadata(ResponseMetadata&& value) { SetResponseMetadata(value); return *this;}

  private:
    Aws::String m_reservedInstancesId;
    ResponseMetadata m_responseMetadata;
  };

} // namespace Model
} // namespace EC2
} // namespace Aws