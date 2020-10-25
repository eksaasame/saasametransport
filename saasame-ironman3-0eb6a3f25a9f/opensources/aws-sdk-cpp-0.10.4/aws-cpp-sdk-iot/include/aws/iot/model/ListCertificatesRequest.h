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
#include <aws/iot/IoT_EXPORTS.h>
#include <aws/iot/IoTRequest.h>
#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
namespace Http
{
    class URI;
} //namespace Http
namespace IoT
{
namespace Model
{

  /**
   * <p>The input for the ListCertificates operation.</p>
   */
  class AWS_IOT_API ListCertificatesRequest : public IoTRequest
  {
  public:
    ListCertificatesRequest();
    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    /**
     * <p>The result page size.</p>
     */
    inline long GetPageSize() const{ return m_pageSize; }

    /**
     * <p>The result page size.</p>
     */
    inline void SetPageSize(long value) { m_pageSizeHasBeenSet = true; m_pageSize = value; }

    /**
     * <p>The result page size.</p>
     */
    inline ListCertificatesRequest& WithPageSize(long value) { SetPageSize(value); return *this;}

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline const Aws::String& GetMarker() const{ return m_marker; }

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline void SetMarker(const Aws::String& value) { m_markerHasBeenSet = true; m_marker = value; }

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline void SetMarker(Aws::String&& value) { m_markerHasBeenSet = true; m_marker = value; }

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline void SetMarker(const char* value) { m_markerHasBeenSet = true; m_marker.assign(value); }

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline ListCertificatesRequest& WithMarker(const Aws::String& value) { SetMarker(value); return *this;}

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline ListCertificatesRequest& WithMarker(Aws::String&& value) { SetMarker(value); return *this;}

    /**
     * <p>The marker for the next set of results.</p>
     */
    inline ListCertificatesRequest& WithMarker(const char* value) { SetMarker(value); return *this;}

    /**
     * <p>Specifies the order for results. If True, the results are returned in
     * ascending order, based on the creation date.</p>
     */
    inline bool GetAscendingOrder() const{ return m_ascendingOrder; }

    /**
     * <p>Specifies the order for results. If True, the results are returned in
     * ascending order, based on the creation date.</p>
     */
    inline void SetAscendingOrder(bool value) { m_ascendingOrderHasBeenSet = true; m_ascendingOrder = value; }

    /**
     * <p>Specifies the order for results. If True, the results are returned in
     * ascending order, based on the creation date.</p>
     */
    inline ListCertificatesRequest& WithAscendingOrder(bool value) { SetAscendingOrder(value); return *this;}

  private:
    long m_pageSize;
    bool m_pageSizeHasBeenSet;
    Aws::String m_marker;
    bool m_markerHasBeenSet;
    bool m_ascendingOrder;
    bool m_ascendingOrderHasBeenSet;
  };

} // namespace Model
} // namespace IoT
} // namespace Aws
