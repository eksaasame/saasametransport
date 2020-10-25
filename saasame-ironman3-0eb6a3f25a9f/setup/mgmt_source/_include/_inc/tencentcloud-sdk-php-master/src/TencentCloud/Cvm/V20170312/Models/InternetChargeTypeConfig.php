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
namespace TencentCloud\Cvm\V20170312\Models;
use TencentCloud\Common\AbstractModel;

/**
 * @method string getInternetChargeType() 获取网络计费模式。
 * @method void setInternetChargeType(string $InternetChargeType) 设置网络计费模式。
 * @method string getDescription() 获取网络计费模式描述信息。
 * @method void setDescription(string $Description) 设置网络计费模式描述信息。
 */

/**
 *描述了网络计费
 */
class InternetChargeTypeConfig extends AbstractModel
{
    /**
     * @var string 网络计费模式。
     */
    public $InternetChargeType;

    /**
     * @var string 网络计费模式描述信息。
     */
    public $Description;
    /**
     * @param string $InternetChargeType 网络计费模式。
     * @param string $Description 网络计费模式描述信息。
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
        if (array_key_exists("InternetChargeType",$param) and $param["InternetChargeType"] !== null) {
            $this->InternetChargeType = $param["InternetChargeType"];
        }

        if (array_key_exists("Description",$param) and $param["Description"] !== null) {
            $this->Description = $param["Description"];
        }
    }
}
