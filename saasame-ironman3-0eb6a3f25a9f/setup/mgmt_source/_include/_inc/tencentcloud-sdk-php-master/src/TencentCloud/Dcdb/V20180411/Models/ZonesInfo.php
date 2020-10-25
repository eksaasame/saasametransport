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
namespace TencentCloud\Dcdb\V20180411\Models;
use TencentCloud\Common\AbstractModel;

/**
 * @method string getZone() 获取可用区英文ID
 * @method void setZone(string $Zone) 设置可用区英文ID
 * @method integer getZoneId() 获取可用区数字ID
 * @method void setZoneId(integer $ZoneId) 设置可用区数字ID
 * @method string getZoneName() 获取可用区中文名
 * @method void setZoneName(string $ZoneName) 设置可用区中文名
 */

/**
 *可用区信息
 */
class ZonesInfo extends AbstractModel
{
    /**
     * @var string 可用区英文ID
     */
    public $Zone;

    /**
     * @var integer 可用区数字ID
     */
    public $ZoneId;

    /**
     * @var string 可用区中文名
     */
    public $ZoneName;
    /**
     * @param string $Zone 可用区英文ID
     * @param integer $ZoneId 可用区数字ID
     * @param string $ZoneName 可用区中文名
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
        if (array_key_exists("Zone",$param) and $param["Zone"] !== null) {
            $this->Zone = $param["Zone"];
        }

        if (array_key_exists("ZoneId",$param) and $param["ZoneId"] !== null) {
            $this->ZoneId = $param["ZoneId"];
        }

        if (array_key_exists("ZoneName",$param) and $param["ZoneName"] !== null) {
            $this->ZoneName = $param["ZoneName"];
        }
    }
}
