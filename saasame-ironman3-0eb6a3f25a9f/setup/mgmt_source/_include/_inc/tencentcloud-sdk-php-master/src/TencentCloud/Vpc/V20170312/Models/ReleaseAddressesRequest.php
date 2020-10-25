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
namespace TencentCloud\Vpc\V20170312\Models;
use TencentCloud\Common\AbstractModel;

/**
 * @method array getAddressIds() 获取标识 EIP 的唯一 ID 列表。EIP 唯一 ID 形如：`eip-11112222`。
 * @method void setAddressIds(array $AddressIds) 设置标识 EIP 的唯一 ID 列表。EIP 唯一 ID 形如：`eip-11112222`。
 */

/**
 *ReleaseAddresses请求参数结构体
 */
class ReleaseAddressesRequest extends AbstractModel
{
    /**
     * @var array 标识 EIP 的唯一 ID 列表。EIP 唯一 ID 形如：`eip-11112222`。
     */
    public $AddressIds;
    /**
     * @param array $AddressIds 标识 EIP 的唯一 ID 列表。EIP 唯一 ID 形如：`eip-11112222`。
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
        if (array_key_exists("AddressIds",$param) and $param["AddressIds"] !== null) {
            $this->AddressIds = $param["AddressIds"];
        }
    }
}
