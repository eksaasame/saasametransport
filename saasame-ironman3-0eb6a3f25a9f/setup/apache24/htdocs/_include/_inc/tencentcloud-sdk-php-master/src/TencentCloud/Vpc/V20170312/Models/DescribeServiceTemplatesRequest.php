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
 * @method array getFilters() 获取过滤条件。
<li>service-template-name - String - （过滤条件）协议端口模板名称。</li>
<li>service-template-id - String - （过滤条件）协议端口模板实例ID，例如：ppm-e6dy460g。</li>
 * @method void setFilters(array $Filters) 设置过滤条件。
<li>service-template-name - String - （过滤条件）协议端口模板名称。</li>
<li>service-template-id - String - （过滤条件）协议端口模板实例ID，例如：ppm-e6dy460g。</li>
 * @method string getOffset() 获取偏移量，默认为0。
 * @method void setOffset(string $Offset) 设置偏移量，默认为0。
 * @method string getLimit() 获取返回数量，默认为20，最大值为100。
 * @method void setLimit(string $Limit) 设置返回数量，默认为20，最大值为100。
 */

/**
 *DescribeServiceTemplates请求参数结构体
 */
class DescribeServiceTemplatesRequest extends AbstractModel
{
    /**
     * @var array 过滤条件。
<li>service-template-name - String - （过滤条件）协议端口模板名称。</li>
<li>service-template-id - String - （过滤条件）协议端口模板实例ID，例如：ppm-e6dy460g。</li>
     */
    public $Filters;

    /**
     * @var string 偏移量，默认为0。
     */
    public $Offset;

    /**
     * @var string 返回数量，默认为20，最大值为100。
     */
    public $Limit;
    /**
     * @param array $Filters 过滤条件。
<li>service-template-name - String - （过滤条件）协议端口模板名称。</li>
<li>service-template-id - String - （过滤条件）协议端口模板实例ID，例如：ppm-e6dy460g。</li>
     * @param string $Offset 偏移量，默认为0。
     * @param string $Limit 返回数量，默认为20，最大值为100。
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
        if (array_key_exists("Filters",$param) and $param["Filters"] !== null) {
            $this->Filters = [];
            foreach ($param["Filters"] as $key => $value){
                $obj = new Filter();
                $obj->deserialize($value);
                array_push($this->Filters, $obj);
            }
        }

        if (array_key_exists("Offset",$param) and $param["Offset"] !== null) {
            $this->Offset = $param["Offset"];
        }

        if (array_key_exists("Limit",$param) and $param["Limit"] !== null) {
            $this->Limit = $param["Limit"];
        }
    }
}
