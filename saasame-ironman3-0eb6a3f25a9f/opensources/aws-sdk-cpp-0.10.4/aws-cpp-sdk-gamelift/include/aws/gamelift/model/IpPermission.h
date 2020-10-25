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
#include <aws/gamelift/GameLift_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/gamelift/model/IpProtocol.h>

namespace Aws
{
namespace Utils
{
namespace Json
{
  class JsonValue;
} // namespace Json
} // namespace Utils
namespace GameLift
{
namespace Model
{

  /**
   * <p>IP addresses and port settings used to limit access by incoming traffic
   * (players) to a fleet. Permissions specify a range of IP addresses and port
   * settings that must be used to gain access to a game server on a fleet
   * machine.</p>
   */
  class AWS_GAMELIFT_API IpPermission
  {
  public:
    IpPermission();
    IpPermission(const Aws::Utils::Json::JsonValue& jsonValue);
    IpPermission& operator=(const Aws::Utils::Json::JsonValue& jsonValue);
    Aws::Utils::Json::JsonValue Jsonize() const;

    /**
     * <p>Starting value for a range of allowed port numbers. </p>
     */
    inline long GetFromPort() const{ return m_fromPort; }

    /**
     * <p>Starting value for a range of allowed port numbers. </p>
     */
    inline void SetFromPort(long value) { m_fromPortHasBeenSet = true; m_fromPort = value; }

    /**
     * <p>Starting value for a range of allowed port numbers. </p>
     */
    inline IpPermission& WithFromPort(long value) { SetFromPort(value); return *this;}

    /**
     * <p>Ending value for a range of allowed port numbers. Port numbers are
     * end-inclusive. This value must be higher than <i>FromPort</i>.</p>
     */
    inline long GetToPort() const{ return m_toPort; }

    /**
     * <p>Ending value for a range of allowed port numbers. Port numbers are
     * end-inclusive. This value must be higher than <i>FromPort</i>.</p>
     */
    inline void SetToPort(long value) { m_toPortHasBeenSet = true; m_toPort = value; }

    /**
     * <p>Ending value for a range of allowed port numbers. Port numbers are
     * end-inclusive. This value must be higher than <i>FromPort</i>.</p>
     */
    inline IpPermission& WithToPort(long value) { SetToPort(value); return *this;}

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline const Aws::String& GetIpRange() const{ return m_ipRange; }

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline void SetIpRange(const Aws::String& value) { m_ipRangeHasBeenSet = true; m_ipRange = value; }

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline void SetIpRange(Aws::String&& value) { m_ipRangeHasBeenSet = true; m_ipRange = value; }

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline void SetIpRange(const char* value) { m_ipRangeHasBeenSet = true; m_ipRange.assign(value); }

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline IpPermission& WithIpRange(const Aws::String& value) { SetIpRange(value); return *this;}

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline IpPermission& WithIpRange(Aws::String&& value) { SetIpRange(value); return *this;}

    /**
     * <p>Range of allowed IP addresses. This value must be expressed in <a
     * href="https://tools.ietf.org/id/cidr">CIDR notation</a>. Example:
     * "<code>000.000.000.000/[subnet mask]</code>" or optionally the shortened version
     * "<code>0.0.0.0/[subnet mask]</code>".</p>
     */
    inline IpPermission& WithIpRange(const char* value) { SetIpRange(value); return *this;}

    /**
     * <p>Network communication protocol used by the fleet.</p>
     */
    inline const IpProtocol& GetProtocol() const{ return m_protocol; }

    /**
     * <p>Network communication protocol used by the fleet.</p>
     */
    inline void SetProtocol(const IpProtocol& value) { m_protocolHasBeenSet = true; m_protocol = value; }

    /**
     * <p>Network communication protocol used by the fleet.</p>
     */
    inline void SetProtocol(IpProtocol&& value) { m_protocolHasBeenSet = true; m_protocol = value; }

    /**
     * <p>Network communication protocol used by the fleet.</p>
     */
    inline IpPermission& WithProtocol(const IpProtocol& value) { SetProtocol(value); return *this;}

    /**
     * <p>Network communication protocol used by the fleet.</p>
     */
    inline IpPermission& WithProtocol(IpProtocol&& value) { SetProtocol(value); return *this;}

  private:
    long m_fromPort;
    bool m_fromPortHasBeenSet;
    long m_toPort;
    bool m_toPortHasBeenSet;
    Aws::String m_ipRange;
    bool m_ipRangeHasBeenSet;
    IpProtocol m_protocol;
    bool m_protocolHasBeenSet;
  };

} // namespace Model
} // namespace GameLift
} // namespace Aws
