<?php
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
namespace TencentCloud\Mariadb\V20170312\Models;
use TencentCloud\Common\AbstractModel;

/**
 * @method MonitorData getBinlogDiskAvailable() 获取binlog日志磁盘可用空间,单位GB
 * @method void setBinlogDiskAvailable(MonitorData $BinlogDiskAvailable) 设置binlog日志磁盘可用空间,单位GB
 * @method MonitorData getDataDiskAvailable() 获取磁盘可用空间,单位GB
 * @method void setDataDiskAvailable(MonitorData $DataDiskAvailable) 设置磁盘可用空间,单位GB
 * @method MonitorData getCpuUsageRate() 获取CPU利用率
 * @method void setCpuUsageRate(MonitorData $CpuUsageRate) 设置CPU利用率
 * @method MonitorData getMemAvailable() 获取内存可用空间,单位GB
 * @method void setMemAvailable(MonitorData $MemAvailable) 设置内存可用空间,单位GB
 * @method string getRequestId() 获取唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
 * @method void setRequestId(string $RequestId) 设置唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
 */

/**
 *DescribeDBResourceUsage返回参数结构体
 */
class DescribeDBResourceUsageResponse extends AbstractModel
{
    /**
     * @var MonitorData binlog日志磁盘可用空间,单位GB
     */
    public $BinlogDiskAvailable;

    /**
     * @var MonitorData 磁盘可用空间,单位GB
     */
    public $DataDiskAvailable;

    /**
     * @var MonitorData CPU利用率
     */
    public $CpuUsageRate;

    /**
     * @var MonitorData 内存可用空间,单位GB
     */
    public $MemAvailable;

    /**
     * @var string 唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
     */
    public $RequestId;
    /**
     * @param MonitorData $BinlogDiskAvailable binlog日志磁盘可用空间,单位GB
     * @param MonitorData $DataDiskAvailable 磁盘可用空间,单位GB
     * @param MonitorData $CpuUsageRate CPU利用率
     * @param MonitorData $MemAvailable 内存可用空间,单位GB
     * @param string $RequestId 唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
     */
    function __construct()
    {

    }
    /**
     * 内部实现，用户禁止调用
     */
    public function deserialize($param)
    {
        if ($param === null) {
            return;
        }
        if (array_key_exists("BinlogDiskAvailable",$param) and $param["BinlogDiskAvailable"] !== null) {
            $this->BinlogDiskAvailable = new MonitorData();
            $this->BinlogDiskAvailable->deserialize($param["BinlogDiskAvailable"]);
        }

        if (array_key_exists("DataDiskAvailable",$param) and $param["DataDiskAvailable"] !== null) {
            $this->DataDiskAvailable = new MonitorData();
            $this->DataDiskAvailable->deserialize($param["DataDiskAvailable"]);
        }

        if (array_key_exists("CpuUsageRate",$param) and $param["CpuUsageRate"] !== null) {
            $this->CpuUsageRate = new MonitorData();
            $this->CpuUsageRate->deserialize($param["CpuUsageRate"]);
        }

        if (array_key_exists("MemAvailable",$param) and $param["MemAvailable"] !== null) {
            $this->MemAvailable = new MonitorData();
            $this->MemAvailable->deserialize($param["MemAvailable"]);
        }

        if (array_key_exists("RequestId",$param) and $param["RequestId"] !== null) {
            $this->RequestId = $param["RequestId"];
        }
    }
}
