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

namespace TencentCloud\Cbs\V20170312;
use TencentCloud\Common\AbstractClient;
use TencentCloud\Common\Profile\ClientProfile;
use TencentCloud\Common\Credential;
use TencentCloud\Cbs\V20170312\Models as Models;

/**
* @method Models\ApplySnapshotResponse ApplySnapshot(Models\ApplySnapshotRequest $req) 本接口（ApplySnapshot）用于回滚快照到原云硬盘。

* 仅支持回滚到原云硬盘上。对于数据盘快照，如果您需要复制快照数据到其它云硬盘上，请使用[CreateDisks](/document/product/362/16312)接口创建新的弹性云盘，将快照数据复制到新购云盘上。 
* 用于回滚的快照必须处于NORMAL状态。快照状态可以通过[DescribeSnapshots](/document/product/362/15647)接口查询，见输出参数中SnapshotState字段解释。
* 如果是弹性云盘，则云盘必须处于未挂载状态，云硬盘挂载状态可以通过[DescribeDisks](/document/product/362/16315)接口查询，见Attached字段解释；如果是随实例一起购买的非弹性云盘，则实例必须处于关机状态，实例状态可以通过[DescribeInstancesStatus](/document/product/213/15738)接口查询。
* @method Models\AttachDisksResponse AttachDisks(Models\AttachDisksRequest $req) 本接口（AttachDisks）用于挂载云硬盘。

* 支持批量操作，将多块云盘挂载到同一云主机。如果多个云盘存在不允许挂载的云盘，则操作不执行，以返回特定的错误码返回。
* 本接口为异步接口，当挂载云盘的请求成功返回时，表示后台已发起挂载云盘的操作，可通过接口[DescribeDisks](/document/product/362/16315)来查询对应云盘的状态，如果云盘的状态由“ATTACHING”变为“ATTACHED”，则为挂载成功。
* @method Models\CreateDisksResponse CreateDisks(Models\CreateDisksRequest $req) 本接口（CreateDisks）用于创建云硬盘。

* 预付费云盘的购买会预先扣除本次云盘购买所需金额，在调用本接口前请确保账户余额充足。
* 本接口支持传入数据盘快照来创建云盘，实现将快照数据复制到新购云盘上。
* 本接口为异步接口，当创建请求下发成功后会返回一个新建的云盘ID列表，此时云盘的创建并未立即完成。可以通过调用[DescribeDisks](/document/product/362/16315)接口根据DiskId查询对应云盘，如果能查到云盘，且状态为'UNATTACHED'或'ATTACHED'，则表示创建成功。
* @method Models\CreateSnapshotResponse CreateSnapshot(Models\CreateSnapshotRequest $req) 本接口（CreateSnapshot）用于对指定云盘创建快照。

* 只有具有快照能力的云硬盘才能创建快照。云硬盘是否具有快照能力可由[DescribeDisks](/document/product/362/16315)接口查询，见SnapshotAbility字段。
* 可创建快照数量限制见[产品使用限制](https://cloud.tencent.com/doc/product/362/5145)。
* @method Models\DeleteSnapshotsResponse DeleteSnapshots(Models\DeleteSnapshotsRequest $req) 本接口（DeleteSnapshots）用于删除快照。

* 快照必须处于NORMAL状态，快照状态可以通过[DescribeSnapshots](/document/product/362/15647)接口查询，见输出参数中SnapshotState字段解释。
* 支持批量操作。如果多个快照存在无法删除的快照，则操作不执行，以返回特定的错误码返回。
* @method Models\DescribeDiskConfigQuotaResponse DescribeDiskConfigQuota(Models\DescribeDiskConfigQuotaRequest $req) 本接口（DescribeDiskConfigQuota）用于查询云硬盘配额。
* @method Models\DescribeDisksResponse DescribeDisks(Models\DescribeDisksRequest $req) 本接口（DescribeDisks）用于查询云硬盘列表。

* 可以根据云硬盘ID、云硬盘类型或者云硬盘状态等信息来查询云硬盘的详细信息，不同条件之间为与(AND)的关系，过滤信息详细请见过滤器`Filter`。
* 如果参数为空，返回当前用户一定数量（`Limit`所指定的数量，默认为20）的云硬盘列表。
* @method Models\DescribeInstancesDiskNumResponse DescribeInstancesDiskNum(Models\DescribeInstancesDiskNumRequest $req) 本接口（DescribeInstancesDiskNum）用于查询实例已挂载云硬盘数量。

* 支持批量操作，当传入多个云服务器实例ID，返回结果会分别列出每个云服务器挂载的云硬盘数量。
* @method Models\DescribeSnapshotsResponse DescribeSnapshots(Models\DescribeSnapshotsRequest $req) 本接口（DescribeSnapshots）用于查询快照的详细信息。

* 根据快照ID、创建快照的云硬盘ID、创建快照的云硬盘类型等对结果进行过滤，不同条件之间为与(AND)的关系，过滤信息详细请见过滤器`Filter`。
*  如果参数为空，返回当前用户一定数量（`Limit`所指定的数量，默认为20）的快照列表。
* @method Models\DetachDisksResponse DetachDisks(Models\DetachDisksRequest $req) 本接口（DetachDisks）用于解挂云硬盘。

* 支持批量操作，解挂挂载在同一主机上的多块云盘。如果多块云盘存在不允许解挂载的云盘，则操作不执行，以返回特定的错误码返回。
* 本接口为异步接口，当请求成功返回时，云盘并未立即从主机解挂载，可通过接口[DescribeDisks](/document/product/362/16315)来查询对应云盘的状态，如果云盘的状态由“ATTACHED”变为“UNATTACHED”，则为解挂载成功。
* @method Models\InquiryPriceCreateDisksResponse InquiryPriceCreateDisks(Models\InquiryPriceCreateDisksRequest $req) 本接口（InquiryPriceCreateDisks）用于创建云硬盘询价。

* 支持查询创建多块云硬盘的价格，此时返回结果为总价格。
* @method Models\InquiryPriceRenewDisksResponse InquiryPriceRenewDisks(Models\InquiryPriceRenewDisksRequest $req) 本接口（InquiryPriceRenewDisks）用于续费云硬盘询价。

* 只支持查询预付费模式的弹性云盘续费价格。
* 支持与挂载实例一起续费的场景，需要在[DiskChargePrepaid](/document/product/362/15669#DiskChargePrepaid)参数中指定CurInstanceDeadline，此时会按对齐到实例续费后的到期时间来续费询价。
* 支持为多块云盘指定不同的续费时长，此时返回的价格为多块云盘续费的总价格。
* @method Models\InquiryPriceResizeDiskResponse InquiryPriceResizeDisk(Models\InquiryPriceResizeDiskRequest $req) 本接口（InquiryPriceResizeDisk）用于扩容云硬盘询价。

* 只支持预付费模式的云硬盘扩容询价。
* @method Models\ModifyDiskAttributesResponse ModifyDiskAttributes(Models\ModifyDiskAttributesRequest $req) 本接口（ModifyDiskAttributes）用于修改云硬盘属性。

* 只支持修改弹性云盘的项目ID。随云主机创建的云硬盘项目ID与云主机联动。可以通过[DescribeDisks](/document/product/362/16315)接口查询，见输出参数中Portable字段解释。
* “云硬盘名称”仅为方便用户自己管理之用，腾讯云并不以此名称作为提交工单或是进行云盘管理操作的依据。
* 支持批量操作，如果传入多个云盘ID，则所有云盘修改为同一属性。如果存在不允许操作的云盘，则操作不执行，以特定错误码返回。
* @method Models\ModifyDisksRenewFlagResponse ModifyDisksRenewFlag(Models\ModifyDisksRenewFlagRequest $req) 本接口（ModifyDisksRenewFlag）用于修改云硬盘续费标识，支持批量修改。
* @method Models\ModifySnapshotAttributeResponse ModifySnapshotAttribute(Models\ModifySnapshotAttributeRequest $req) 本接口（ModifySnapshotAttribute）用于修改指定快照的属性。

* 当前仅支持修改快照名称及将非永久快照修改为永久快照。
* “快照名称”仅为方便用户自己管理之用，腾讯云并不以此名称作为提交工单或是进行快照管理操作的依据。
* @method Models\RenewDiskResponse RenewDisk(Models\RenewDiskRequest $req) 本接口（RenewDisk）用于续费云硬盘。

* 只支持预付费的云硬盘。云硬盘类型可以通过[DescribeDisks](/document/product/362/16315)接口查询，见输出参数中DiskChargeType字段解释。
* 支持与挂载实例一起续费的场景，需要在[DiskChargePrepaid](/document/product/362/15669#DiskChargePrepaid)参数中指定CurInstanceDeadline，此时会按对齐到子机续费后的到期时间来续费。
* 续费时请确保账户余额充足。可通过[DescribeAccountBalance](/document/product/378/4397)接口查询账户余额。
* @method Models\ResizeDiskResponse ResizeDisk(Models\ResizeDiskRequest $req) 本接口（ResizeDisk）用于扩容云硬盘。

* 只支持扩容弹性云盘。云硬盘类型可以通过[DescribeDisks](/document/product/362/16315)接口查询，见输出参数中Portable字段解释。随云主机创建的云硬盘需通过[ResizeInstanceDisks](/document/product/213/15731)接口扩容。
* 本接口为异步接口，接口成功返回时，云盘并未立即扩容到指定大小，可通过接口[DescribeDisks](/document/product/362/16315)来查询对应云盘的状态，如果云盘的状态为“EXPANDING”，表示正在扩容中，当状态变为“UNATTACHED”，表示扩容完成。 
* @method Models\TerminateDisksResponse TerminateDisks(Models\TerminateDisksRequest $req) 本接口（TerminateDisks）用于退还云硬盘。

* 不再使用的云盘，可通过本接口主动退还。
* 本接口支持退还预付费云盘和按小时后付费云盘。按小时后付费云盘可直接退还，预付费云盘需符合退还规则。
* 支持批量操作，每次请求批量云硬盘的上限为50。如果批量云盘存在不允许操作的，请求会以特定错误码返回。
 */

class CbsClient extends AbstractClient
{
    /**
     * @var string 产品默认域名
     */
    protected $endpoint = "cbs.tencentcloudapi.com";

    /**
     * @var string api版本号
     */
    protected $version = "2017-03-12";

    /**
     * CvmClient constructor.
     * @param Credential $credential 认证类实例
     * @param string $region 地域
     * @param ClientProfile $profile client配置
     */
    function __construct($credential, $region, $profile=null)
    {
        parent::__construct($this->endpoint, $this->version, $credential, $region, $profile);
    }

    public function returnResponse($action, $response)
    {
        $respClass = "TencentCloud"."\\".ucfirst("cbs")."\\"."V20170312\\Models"."\\".ucfirst($action)."Response";
        $obj = new $respClass();
        $obj->deserialize($response);
        return $obj;
    }
}
