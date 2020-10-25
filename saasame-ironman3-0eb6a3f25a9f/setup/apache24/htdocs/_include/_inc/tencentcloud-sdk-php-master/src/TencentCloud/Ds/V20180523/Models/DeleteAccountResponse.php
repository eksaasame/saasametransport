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
namespace TencentCloud\Ds\V20180523\Models;
use TencentCloud\Common\AbstractModel;

/**
 * @method array getDelSuccessList() 获取删除成功帐号ID列表
 * @method void setDelSuccessList(array $DelSuccessList) 设置删除成功帐号ID列表
 * @method array getDelFailedList() 获取删除失败帐号ID列表
 * @method void setDelFailedList(array $DelFailedList) 设置删除失败帐号ID列表
 * @method string getRequestId() 获取唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
 * @method void setRequestId(string $RequestId) 设置唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
 */

/**
 *DeleteAccount返回参数结构体
 */
class DeleteAccountResponse extends AbstractModel
{
    /**
     * @var array 删除成功帐号ID列表
     */
    public $DelSuccessList;

    /**
     * @var array 删除失败帐号ID列表
     */
    public $DelFailedList;

    /**
     * @var string 唯一请求ID，每次请求都会返回。定位问题时需要提供该次请求的RequestId。
     */
    public $RequestId;
    /**
     * @param array $DelSuccessList 删除成功帐号ID列表
     * @param array $DelFailedList 删除失败帐号ID列表
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
        if (array_key_exists("DelSuccessList",$param) and $param["DelSuccessList"] !== null) {
            $this->DelSuccessList = $param["DelSuccessList"];
        }

        if (array_key_exists("DelFailedList",$param) and $param["DelFailedList"] !== null) {
            $this->DelFailedList = $param["DelFailedList"];
        }

        if (array_key_exists("RequestId",$param) and $param["RequestId"] !== null) {
            $this->RequestId = $param["RequestId"];
        }
    }
}
