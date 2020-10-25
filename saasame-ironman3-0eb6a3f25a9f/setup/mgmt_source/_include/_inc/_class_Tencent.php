<?php

require_once 'qcloudapi-sdk-php-master/src/QcloudApi/QcloudApi.php';
require(__DIR__ . DIRECTORY_SEPARATOR . 'cos-php-sdk-v5-master/cos-autoloader.php');

require_once 'tencentcloud-sdk-php-master/TCloudAutoLoader.php';

use TencentCloud\Cbs\V20170312\CbsClient;
use TencentCloud\Cbs\V20170312\Models\CreateDisksRequest;
use TencentCloud\Cbs\V20170312\Models\TerminateDisksRequest;
use TencentCloud\Common\Exception\TencentCloudSDKException;
use TencentCloud\Common\Credential;

class Tencent_Controller 
{
private $test;

    private $cvm;

    private $cbs;

    private $snapshot;

    private $vpc;

    private $dfw;

    private $image;

    private $cosClient;

    private $SecretId;

    private $SecretKey;

    private $AppId;

    private $RegionId;
    
    private $LogJobId;

    public function __construct() {
$this->test = false;
    }

    public function SetAppId( $AppId ) {
        $this->AppId = $AppId;
    }

    public function SetKey( $SecretId, $SecretKey, $region = "bj" ) {
        $this->SecretId = $SecretId;
        $this->SecretKey = $SecretKey;
        $config = array(
                'SecretId'       => $this->SecretId,
                'SecretKey'      => $this->SecretKey,
                'RequestMethod'  => 'GET',
                'DefaultRegion'  => $region
            );

        //for new sdk
        $this->cred = new Credential( $SecretId, $SecretKey );

        $this->cvm = QcloudApi::load(QcloudApi::MODULE_CVM, $config);

        $this->cbs = QcloudApi::load(QcloudApi::MODULE_CBS, $config);

        $this->snapshot = QcloudApi::load(QcloudApi::MODULE_SNAPSHOT, $config);

        $this->vpc = QcloudApi::load(QcloudApi::MODULE_VPC, $config);

        $this->dfw = QcloudApi::load(QcloudApi::MODULE_DFW, $config);

        $this->image = QcloudApi::load(QcloudApi::MODULE_IMAGE, $config);

        $this->cosClient = new Qcloud\Cos\Client(array('region' => $region,
            'credentials'=> array(
            'appId'     => $this->AppId,
            'secretId'  => $this->SecretId,
            'secretKey' => $this->SecretKey
        )));
    }

    private function sendRequest( $action, $config , $client) {

        $response = $client->$action($config);
        
        //echo "\nRequest :" . $client->getLastRequest();
        //echo "\nResponse :" . $client->getLastResponse();
        //echo "\n";
        if ($response === false ) { 
            
            $this->writeErrorLog( $action, $client );

            return false;
        }
        else if(  isset( $response["Response"]["Error"] ) ) {

            $param = array(
                "config" => $config,
                "response" => $response,
                "callstack" => debug_backtrace()
            );

            Misc_Class::function_debug('Tencent', $action, $param);	

            return false;
        }

        return $response;
    }

    private function writeErrorLog( $action, $client, $isShowError = null ) {

        $error = $client->getError();

        $param = array(
            "Error code" => $error->getCode(),
            "Msg"       => $error->getMessage(),
            "ext"       => $error->getExt(),
            "Request"   => $client->getLastRequest(),
            "Response"  => $client->getLastResponse()
        );

        Misc_Class::function_debug('Tencent', $action, $param);	

        if( $isShowError ) {
            $replica  = new Replica_Class();
            $mesage = $replica->job_msg($param["Msg"]);
            $replica->update_job_msg( $this->LogJobId , $mesage, 'Service');
        }

        return $param;
    }

    public function CheckConnect( ) {

        $region = $this->cvm->DescribeRegions();

        if( $region === false) {

            $this->writeErrorLog( "DescribeRegions" );
            return false;
        }

        return true;
    }

    public function GetRegion() {

        $region = $this->cvm->DescribeRegions();

        if( $region === false)
            return false;

        return $region;
    }

    public function CreateInstance( $param ) {

        $config = array(
            "Version" => "2017-03-12",
            "InstanceChargeType" => "POSTPAID_BY_HOUR"
        );

        if( isset( $param["Zone"] ) ){
            $config["Placement.Zone"] = $param["Zone"];
            $config["Region"] = $this->getCurrectRegion( $param["Zone"] );
        }

        if( isset( $param["InstanceType"] ) )
            $config["InstanceType"] = $param["InstanceType"];
        
        if( isset( $param["ImageId"] ) )
            $config["ImageId"] = $param["ImageId"];

        if( isset( $param["InstanceName"] ) )
            $config["InstanceName"] = $param["InstanceName"];

        if( isset( $param["VirtualPrivateCloud.VpcId"] ) )
            $config["VirtualPrivateCloud.VpcId"] = $param["VirtualPrivateCloud.VpcId"];

        if( isset( $param["VirtualPrivateCloud.SubnetId"] ) )
            $config["VirtualPrivateCloud.SubnetId"] = $param["VirtualPrivateCloud.SubnetId"];

        if( isset( $param["VirtualPrivateCloud.PrivateIpAddresses"] ) )
            $config["VirtualPrivateCloud.PrivateIpAddresses.0"] = $param["VirtualPrivateCloud.PrivateIpAddresses"];

        if( isset( $param["SecurityGroupIds.1"] ) )
            $config["SecurityGroupIds.1"] = $param["SecurityGroupIds.1"];
        
        if( isset( $param["InternetAccessible"] ) ) {
            $config["InternetAccessible.InternetMaxBandwidthOut"] = $param["InternetAccessible"];

            if( $param["InternetAccessible"] > 0 )
                $config["InternetAccessible.PublicIpAssigned"] = "true";
        }

        if( isset( $param["LoginSettings.Password"] ) )
            $config["LoginSettings.Password"] = $param["LoginSettings.Password"];

        if( isset( $param["SystemDisk.DiskType"] ) )
            $config["SystemDisk.DiskType"] = $param["SystemDisk.DiskType"];

        $instances = $this->sendRequest( "RunInstances", $config, $this->cvm );

        return $instances;
    }

    public function DeleteInstance( $param ) {

        $config = array(
            "Version" => "2017-03-12"
        );

        if( isset( $param["zone"] ) ) 
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        if( isset( $param["InstanceIds"] ) ) {
            foreach( $param["InstanceIds"] as $key=>$instanceId ) 
                $config["InstanceIds.".$key] = $instanceId;
        }

        $instances = $this->sendRequest( "TerminateInstances", $config, $this->cvm );

        return $instances;
    }

    public function ListInstances( $instanceIds = null, $zone = null ) {

        $config = array("Version"=>"2017-03-12");

        if( isset( $instanceIds ) && isset( $zone ) ){

            $config["Region"] = $this->getCurrectRegion( $zone );

            if( $instanceIds )
                foreach( $instanceIds as $key=>$instanceId ) {
                    $config["InstanceIds.".$key] = $instanceId;
                }

            $instances = $this->sendRequest( "DescribeInstances", $config, $this->cvm );

            return $instances;
        }

        $regions = $this->GetRegion();

        $all_instances = array();
        $all_instances["Response"] = array();
        $all_instances["Response"]["InstanceSet"] = array();

        foreach( $regions["regionSet"] as $region ){
            $config["Region"] = $region["regionCode"];
        
            $instances = $this->sendRequest( "DescribeInstances", $config, $this->cvm );

            if($instances["Response"]["TotalCount"] > 0)
                $all_instances["Response"]["InstanceSet"] = array_merge($all_instances["Response"]["InstanceSet"], $instances["Response"]["InstanceSet"]);
            
        }

        return $all_instances;
    }

    public function ListInstancesStatus( $instanceIds = null, $zone = null ) {

        $config = array("Version"=>"2017-03-12");

        if( isset( $instanceIds ) && isset( $zone ) ){

            $config["Region"] = $this->getCurrectRegion( $zone );

            if( $instanceIds )
                foreach( $instanceIds as $key=>$instanceId ) {
                    $config["InstanceIds.".$key] = $instanceId;
                }

            $instances = $this->sendRequest( "DescribeInstancesStatus", $config, $this->cvm );

            return $instances;
        }

        $regions = $this->GetRegion();

        $all_instances = array();
        $all_instances["Response"] = array();
        $all_instances["Response"]["InstanceStatusSet"] = array();

        foreach( $regions["regionSet"] as $region ){
            $config["Region"] = $region["regionCode"];

            $instances = $this->sendRequest( "DescribeInstancesStatus", $config, $this->cvm );

            if($instances["Response"]["TotalCount"] > 0)
                $all_instances["Response"]["InstanceStatusSet"] = array_merge($all_instances["Response"]["InstanceStatusSet"], $instances["Response"]["InstanceStatusSet"]);
            
        }

        return $all_instances;
    }

    public function CreateDisk( $param ) {

        $config = array(
            "storageType"   => "cloudBasic",
            "goodsNum"      => 1,
            "payMode"       => "POSTPAID_BY_HOUR",
            "period"        => 1
        );

        if( isset( $param["zone"] ) ) {
            $config["zone"] = $param["zone"];
        }

        if( isset( $param["size"] ) ) {
            $config["storageSize"] = $param["size"];
        }

        if( isset( $param["snapId"] ) ) {
            $config["snapshotId"] = $param["snapId"];
        }

        $disk = $this->sendRequest( "CreateCbsStorages", $config, $this->cbs );

        return $disk;
    }

    public function CreateDiskV2( $param ) {
    
        $region = "";

        if( isset( $param["zone"] ) ) {
            $region = $this->getCurrectRegion($param["zone"]);
        }

        $CbsClient = new CbsClient($this->cred, $region);
        
        $config = new CreateDisksRequest();

        $config->DiskChargeType = "POSTPAID_BY_HOUR";

        if( isset( $param["zone"] ) ) {
            $config->Placement = array( 
                "Zone" => $param["zone"]
            );
        }

        if( isset( $param["size"] ) ) {
            $config->DiskSize = $param["size"];
        }

        if( isset( $param["snapId"] ) ) {
            $config->SnapshotId = $param["snapId"];
        }

        if( isset( $param["diskType"] ) ) {
            $config->DiskType = $param["diskType"];
        }

        try{
            $disk = $CbsClient->CreateDisks($config);
        }
        catch(TencentCloudSDKException $e) {
            Misc_Class::function_debug('Tencent', "CreateDiskV2", $e);
        }
        //$disk = $this->sendRequest( "CreateDisks", $config, $CbsClient );

        return json_decode( $disk->toJsonString(), true );
    }

    public function DeleteDiskV2( $param ) {

        $CbsClient = new CbsClient($this->cred, $this->getCurrectRegion( $param["zone"] ));
        
        $config = new TerminateDisksRequest();

        if( isset( $param["diskId"]) )
        $config->DiskIds = array( $param["diskId"] );

        try{
            $disk = $CbsClient->TerminateDisks($config);
        }
        catch(TencentCloudSDKException $e) {
            Misc_Class::function_debug('Tencent', "DeleteDiskV2", $e);
            return false;
        }
        //$disk = $this->sendRequest( "CreateDisks", $config, $CbsClient );

        return json_decode( $disk->toJsonString(), true );
    }

    public function GetDiskInfo( $param ) {

        $config = array( );

        if( isset( $param["diskIds"] ) ) {
            foreach( $param["diskIds"] as $key=>$diskId ) 
                $config["storageIds.".$key] = $diskId;
        }

        if( isset( $param["zone"] ) ) {
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );
        }

        $ret = $this->sendRequest( "DescribeCbsStorages", $config, $this->cbs );

        return $ret;
    }

    public function AttachDiskToInstance( $param ) {

        $config = array( );

        if( isset( $param["zone"] ) ) {
            $region = $this->getCurrectRegion($param["zone"]);
        }
        
        if( isset( $param["diskIds"] ) ) {
            foreach( $param["diskIds"] as $key=>$diskId ) 
                $config["storageIds.".$key] = $diskId;
        }

        if( isset( $param["instanceId"] ) ) {
            $config["uInstanceId"] = $param["instanceId"];
        }

        if( isset( $region ) ) {
            $config["Region"] = $region;
        }

        $ret = $this->sendRequest( "AttachCbsStorages", $config, $this->cbs );

        return $ret;
    }
    
    public function DettachDiskFromInstance( $param ) {

        $config = array( );

        if( isset( $param["diskIds"] ) ) {
            foreach( $param["diskIds"] as $key=>$diskId ) 
                $config["storageIds.".$key] = $diskId;
        }

        if( isset( $param["zone"] ) )
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        $ret = $this->sendRequest( "DetachCbsStorages", $config, $this->cbs );

        return $ret;
    }

    public function CreateSnapshot( $param ) {

        $config = array();

        if( isset( $param["diskId"] ) ) {
            $config["storageId"] = $param["diskId"];
        }

        if( isset( $param["snapshotName"] ) ) {
            $config["snapshotName"] = $param["snapshotName"];
        }

        if( isset( $param["region"] ) ) {
            $config["Region"] = $param["region"];
        }

        $snapshot = $this->sendRequest( "CreateSnapshot", $config, $this->snapshot );

        return $snapshot;
    }

    public function GetSnapshotList( $param ) {

        $config = array( "limit" => 100 );

        if( isset( $param["diskIds"] ) )
        foreach( $param["diskIds"] as $key=>$diskId ) {
            $config["storageIds.".$key] = $diskId;
        }

        if( isset( $param["snapshotIds"] ) )
        foreach( $param["snapshotIds"] as $key=>$snapshotId ) {
            $config["snapshotIds.".$key] = $snapshotId;
        }

        if( isset( $param["zone"] ) )
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        $ret = $this->sendRequest( "DescribeSnapshots", $config, $this->snapshot );

        return $ret;

    }

    public function GetNetworkInterface( $param ) {

        $config = array( "limit" => 50 );

        if( isset( $param["instanceId"] ) )
            $config["instanceId"] = $param["instanceId"];

        if( isset( $param["zone"] ) )
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        $ret = $this->sendRequest( "DescribeNetworkInterfaces", $config, $this->vpc );

        return $ret;
    }

    public function DeleteSnapshot( $param ) {

        $config = array( );

        if( $param["snapshotIds"] )
        foreach( $param["snapshotIds"] as $key=>$snapshotId ) {
            $config["snapshotIds.".$key] = $snapshotId;
        }

        if( isset( $param["zone"] ) )
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        $snapshot = $this->sendRequest( "DeleteSnapshot", $config, $this->snapshot );

        return $snapshot;
    }

    public function GetInstanceTypes( $param ) {

        $config = array(
            "Version"=>"2017-03-12"
        );

        if( $param["zone"] ) {
            $config["Filters.1.Name"] = "zone";
            $config["Filters.1.Values.1"] = $param["zone"];
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );
        }

        $types = $this->sendRequest( "DescribeInstanceTypeConfigs", $config, $this->cvm );

        return $types;
    }

    public function GetVPCList( $param ) {

        $config = array(
            "limit"=>100
        );

        if( isset( $param["vpcId"] ) )
            $config["vpcId"] = $param["vpcId"];

        if( isset( $param["zone"] ) )
            $config["Region"] = $param["zone"];

        $VPCs = $this->sendRequest( "DescribeVpcEx", $config, $this->vpc );

        return $VPCs;
    }

    public function GetSubnetList( $param ) {

        $config = array(
            "limit"=>100
        );

        if( isset( $param["zone"] ) )
            $config["Region"] = $param["zone"];

        $subnets = $this->sendRequest( "DescribeSubnetEx", $config, $this->vpc );

        return $subnets["data"];
    }

    public function GetSecurityGroups( $param ) {

        $config = array(
            "limit"=>100
        );

        if( isset( $param["zone"] ) )
            $config["Region"] = $param["zone"];

        $securityGroups = $this->sendRequest( "DescribeSecurityGroupEx", $config, $this->dfw );

        return $securityGroups;
    }

    public function ImportImage( $param ) {

        $config = array(
            "Version" => "2017-3-12",
            "Force" => "true"
        );

        if( isset( $param["OsType"] ) )
            $config["OsType"] = $param["OsType"];

        if( isset( $param["OsVersion"] ) )
            $config["OsVersion"] = $param["OsVersion"];

        if( isset( $param["ImageUrl"] ) )
            $config["ImageUrl"] = $param["ImageUrl"];
        
        if( isset( $param["ImageName"] ) )
            $config["ImageName"] = $param["ImageName"];

        if( isset( $param["ImageDescription"] ) )
            $config["ImageDescription"] = $param["ImageDescription"];

        if( isset( $param["Architecture"] ) )
            $config["Architecture"] = $param["Architecture"];

        $securityGroups = $this->sendRequest( "ImportImage", $config, $this->image );

        return $securityGroups;
    }

    public function DeleteImage( $param ) {

        $config = array(
            "Version" => "2017-3-12"
        );

        foreach( $param["ImageIds"] as $key=>$imageId ) {
            $config["ImageIds.".$key] = $imageId;
        }

        if( isset( $param["zone"] ) )
            $config["Region"] = $this->getCurrectRegion( $param["zone"] );

        $ret = $this->sendRequest( "DeleteImages", $config, $this->image );

        return $ret;
    }

    public function GetImages( $param ) {

        $config = array(
            "Version" => "2017-3-12",
            "Limit" => 100
        );

        if( isset( $param["ImageName"] ) ) {
            $config["Filters.1.Name"] = "image-name";
            $config["Filters.1.Values.1"] = $param["ImageName"];
        }

        $Images = $this->sendRequest( "DescribeImages", $config, $this->image );

        return $Images;
    }

    public function GetObjectUrl( $bucket, $object, $region) {

        try {
            $bucket =  $bucket;
            $key = $object;
            $region = $region;
            $url = "/{$key}";
            $request = $this->cosClient->get($url);
            $signedUrl = $this->cosClient->getObjectUrl($bucket, $key, '+10 minutes');
            return $signedUrl;

        } catch (\Exception $e) {
            Misc_Class::function_debug("Tencent", "GetObjectUrl", $e->getMessage());
            return false;
        }

    }
/**
 * @brief  get cloud key from database and init to used for API
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @return  none
 */

    public function AuthTencent( $CloudId, $region = "bj" ) {

        $TencentModel = new Tencent_Model();

        $cloud_info = $TencentModel->query_cloud_connection_information( $CloudId );

        $this->SetAppId( $cloud_info['USER_UUID'] );
        
        $this->SetKey( $cloud_info['ACCESS_KEY'], $cloud_info['SECRET_KEY'], $this->getCurrectRegion( $region ) );
    }

    public function ImportImageFromOss( $cloud_uuid, $region , $bucket, $object, $service_satus ) {

        $this->AuthTencent( $cloud_uuid, $region );

        $p = json_encode( array(
            "guest_os_name" => $service_satus->platform . (($service_satus->architecture == "amd64")?" 64-bit":" 32-bit"),
        ) );

        $os = $this->enumOS( $p );

        $arch = $os["arch"];

        $platfrom = $os["platfrom"];

        if( strpos( $platfrom, "Windows" ) === false ){
            $osType = 'Linux';
            $osinfo = explode(' ', $service_satus->platform );
            $platfrom = $osinfo[0];
            $version = explode('.', $osinfo[1])[0];
            if( $service_satus->architecture == " amd64" )
                $arch = "x86_64";
            else
                $arch = "i386";
        }
        else {
            $osType = 'Windows';
            $version = '-';
        }

        $objectURL = $this->GetObjectUrl( $bucket, $object, $region);

        if( $objectURL === false )
            return false;
            //$platfrom = "Windows";
            //$arch = "i386";

        $now = Misc_Class::current_utc_time();

        $param = array(
            "OsType" => $platfrom,
            "OsVersion" => $version,
            "ImageUrl" => $objectURL,
            "ImageName" => "SaaSaMe_".strtotime($now),
            "ImageDescription" => "Created by SaaSaMe",
            "Architecture" => $arch
        );

        $ret = $this->ImportImage( $param );

        if( isset( $ret["Response"]["Error"] ) )
            return false;

        $ret["ImageId"] = $param["ImageName"];

        $ret["TaskId"] = $param["ImageName"];

        return (object)$ret;
    }

    public function describe_image_detail( $cloud_uuid, $zone, $imageName ) {
        
        $this->AuthTencent( $cloud_uuid, $zone );

        $param = array( 
            "ImageName" => $imageName
         );

        $ret = $this->GetImages( $param );

        $ret["name"] = $imageName;

        if( isset( $ret["Response"]["ImageSet"][0]["ImageState"] ) &&
            $ret["Response"]["ImageSet"][0]["ImageState"] == "NORMAL" ) 
            return "100%";
        
        return 0;
    }

    public function enumOS( $param ) {

        $platfrom = 'Windows Server 2012';

        $arch = 'x86_64';

        $hostInfo = json_decode( $param, true );

        if( isset( $hostInfo["os_name"] ) ) {
            if( strpos( $hostInfo["os_name"], "Windows Server 2012" ) )
                $platfrom = 'Windows Server 2012';
            else if( strpos( $hostInfo["os_name"], "Windows Server 2008" ) )
                $platfrom = 'Windows Server 2008';
            else if( strpos( $hostInfo["os_name"], "Windows Server 2003" ) )
                $platfrom = 'Windows Server 2003';
            else if( strpos( $hostInfo["os_name"], "Windows 7" ) )
                $platfrom = 'Windows 7';
        }

        if( isset($hostInfo["os_name"]) ) {
            if( strpos( $hostInfo["os_name"], "64-bit" ) )
                $arch = 'x86_64';
            else if( strpos( $hostInfo["os_name"], "32-bit" ) )
                $arch = 'i386';
        }

        if( isset( $hostInfo["guest_os_name"] ) ) {
            if( strpos( $hostInfo["guest_os_name"], "CentOS" ) !== false )
                $platfrom = 'CentOS';
            else if( strpos( $hostInfo["guest_os_name"], "Red Hat" ) !== false )
                $platfrom = 'RedHat';
            else if( strpos( $hostInfo["guest_os_name"], "Ubuntu Linux" ) !== false )
                $platfrom = 'Ubuntu';
            else if( strpos( $hostInfo["guest_os_name"], "SUSE Linux" ) !== false )
                $platfrom = 'SUSE';
            else if( strpos( $hostInfo["guest_os_name"], "SUSE openSUSE" ) !== false )
                $platfrom = 'OpenSUSE';
            else if( strpos( $hostInfo["guest_os_name"], "Debian" ) !== false )
                $platfrom = 'Debian';
            else if( strpos( $hostInfo["guest_os_name"], "CoreOS" ) !== false )
                $platfrom = 'CoreOS';
            else if( strpos( $hostInfo["guest_os_name"], "FreeBSD" ) !== false )
                $platfrom = 'FreeBSD';
            else if( strpos( $hostInfo["guest_os_name"], "Windows Server 2012" ) !== false )
                $platfrom = 'Windows Server 2012';
            else if( strpos( $hostInfo["guest_os_name"], "Windows Server 2008" ) !== false )
                $platfrom = 'Windows Server 2008';
            else if( strpos( $hostInfo["guest_os_name"], "Windows Server 2003" ) !== false )
                $platfrom = 'Windows Server 2003';
            else if( strpos( $hostInfo["guest_os_name"], "Windows Server 7" ) !== false )
                $platfrom = 'Windows Server 7';
            else if( strpos( $hostInfo["guest_os_name"], "Other Linux" ) !== false )
                $platfrom = 'Others Linux';
            else
                $platfrom = 'Others Linux';
        }

        if( isset( $hostInfo["guest_os_name"] ) ) {
            if( strpos( $hostInfo["guest_os_name"], "32-bit" ) !== false )
                $arch = 'i386';
            else
                $arch = 'x86_64';
        }

        $ret = array( "arch" => $arch,
                      "platfrom" => $platfrom 
                    );

        return $ret;
    }

/**
 * @brief  describe instance in all regions
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @return  instance information like name, ip, id...
 */

    public function describe_all_instances( $cloud_uuid ) {

        $this->AuthTencent( $cloud_uuid );

        $instances = array();

        $listInstance = $this->ListInstances();

        $listInstanceStatus = $this->ListInstancesStatus();

        foreach( $listInstance["Response"]["InstanceSet"] as $instance ) {

            $statusIndex = array_search($instance["InstanceId"],array_column($listInstanceStatus["Response"]["InstanceStatusSet"],"InstanceId"));

            $temp = array(
                "InstanceName" => $instance["InstanceName"],
                "PrivateIpAddress" => $instance["PrivateIpAddresses"][0],
                "PublicIpAddress" => isset($instance["PublicIpAddresses"][0])? $instance["PublicIpAddresses"][0]:"N/A",
                "Region" => $this->getCurrectRegion( $instance["Placement"]["Zone"] ),
                "Zone" => $instance["Placement"]["Zone"],
                "InstanceType" => $instance["InstanceType"],
                "InstanceId" => $instance["InstanceId"],
                "Status" => $listInstanceStatus["Response"]["InstanceStatusSet"][$statusIndex]["InstanceState"]
            );

            array_push( $instances, $temp);
        }

       return $instances;
    }

    /**
 * @brief  describe specified instance
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch instance in
 * @param[in]   instanceId          instance unique id
 * @return  instance information like name, ip, id...
 */

    public function describe_instance( $cloud_uuid, $region, $instanceId) {

        $this->AuthTencent( $cloud_uuid );

        $networkRet = $this->GetNetworkInterface( array( 
            "instanceId" => $instanceId,
            "zone" => $region
         ) );

        if( $networkRet["codeDesc"] != "Success" )
            return false;
        
        $networkInterfaces = array( );

        foreach( $networkRet["data"]["data"] as $network ) {

            $temp = (object) array(
                "PrimaryIpAddress" => $network["privateIpAddressesSet"][0]["privateIpAddress"],
                "MacAddress" => $network["macAddress"],
                "NetworkInterfaceId" => $network["networkInterfaceId"]
            );

            array_push( $networkInterfaces, $temp );
        }

        $instance = $this->ListInstances( array($instanceId), $region )["Response"]["InstanceSet"][0];

        $listInstanceStatus = $this->ListInstancesStatus( array($instanceId), $region );

        $statusIndex = array_search($instance["InstanceId"],array_column($listInstanceStatus["Response"]["InstanceStatusSet"],"InstanceId"));

        $publicIp = "N/A";
        if( isset( $instance["PublicIpAddresses"][0] ) )
            $publicIp = $instance["PublicIpAddresses"][0];

        $privateIp = '';
        foreach( $instance["PrivateIpAddresses"] as $pIp ){
            $privateIp .= $pIp;
        }

        $securityId = '';
        
        if( isset($instance["SecurityGroupIds"]) ) {
            foreach( $instance["SecurityGroupIds"] as $sgId ){
                $securityId .= $sgId;
            }
        }

        $Instance_Info = array(
            "InstanceName" => $instance["InstanceName"],
            "PrivateIpAddress" => $privateIp,
            "PublicIpAddress" => $publicIp,
            "Region" => $this->getCurrectRegion( $instance["Placement"]["Zone"] ),
            "Zone" => $instance["Placement"]["Zone"],
            "InstanceType" => $instance["InstanceType"],
            "InstanceId" => $instance["InstanceId"],
            "Status" => $listInstanceStatus["Response"]["InstanceStatusSet"][$statusIndex]["InstanceState"],
            "SecurityGroup" => $securityId,
            "NetworkInterfaces" => $networkInterfaces,
            "VPC" => $instance["VirtualPrivateCloud"]["VpcId"],
            "CPU" => $instance["CPU"],
            "Memory" => $instance["Memory"]
        );

        return $Instance_Info;
    }

    /**
 * @brief  create disk and attach it to transport server
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch disk in
 * @param[in]   instanceId          instance unique id, disk will attach the instance
 * @param[in]   size                disk size ( if less than 20g, will assigned 20g automatically )
 * @param[in]   hostName            instance name ( for assigned a name to disk )
 * @return  disk information
 */

    public function begin_volume_for_loader($cloud_uuid, $zone, $instanceId, $size, $hostName) {

        $this->AuthTencent( $cloud_uuid );

        $now = Misc_Class::current_utc_time();	

        $hostName = $hostName.'-'.strtotime($now);

        $diskSize = (ceil( $size / 10 )* 10 );

        $param = array(
            //"region" => $this->getCurrectRegion( substr($zone, 0, -1) ),
            "zone" => $zone,
            "size" => ( $diskSize >= 10 )? $diskSize : 10,
            "description" => "Disk Created By Saasame Transport Service@".$now,
            "diskType" => "CLOUD_BASIC"
            //"diskName" => $hostName
        );

        $diskRet = $this->CreateDiskV2( $param );

        if( !isset( $diskRet["DiskIdSet"][0] ) )
            return false;

        $count = 0;

        do{
            $count ++;
            $param = array(
                "instanceId" => $instanceId,
                "diskIds" => array( $diskRet["DiskIdSet"][0] ),
                "zone" => $zone
            );

            sleep(10);

            $ret = $this->AttachDiskToInstance( $param );

            if( $ret["detail"][$diskRet["DiskIdSet"][0]] == 0 )
                break;

            if( $count >= 10 )
                $ret = false;

        }while($count >= 10);

        if( $ret == false )
            return false;
        
		$VOLUMD_INFO = (object)array(
								'serverId' 	 => $instanceId.'|'.$zone,							
								'volumeId'	 => $diskRet["DiskIdSet"][0],
								'volumeSize' => $size,
								'volumePath' => 'None'
						);
		
		return $VOLUMD_INFO;
    }
    
/**
 * @brief  create snapshot from disk
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch snapshot in
 * @param[in]   diskId              disk unique id
 * @param[in]   InstanceName        for named snapshot
 * @param[in]   time                for named snapshot
 * @return  snapshot information
 */

    public function create_disk_snapshot( $cloud_uuid, $region ,$diskId, $InstanceName, $time ) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            "diskId" => $diskId,
            "snapshotName" => $InstanceName.'-'.strtotime($time),
            "region" => $this->getCurrectRegion( $region )
        );

		$result = $this->CreateSnapshot( $param );
		
		return $result;
    }

/**
 * @brief  control the number of snapshots
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch snapshot in
 * @param[in]   diskId              disk unique id
 * @param[in]   numSnapshot         max number of snapshot in cloud
 * @return  true 
 */

    public function snapshot_control( $cloud_uuid, $zone ,$diskId, $numSnapshot ) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            //"region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "diskIds" => array( $diskId ),
            "zone" => $zone
        );

        $snapshots = $this->GetSnapshotList( $param );
 
        $sort_array = (array)$snapshots["snapshotSet"];
        
        usort( $sort_array, array("Tencent_Controller", "snapshot_cmp_Tencent"));

		if ($sort_array != FALSE)
        {
		    $excess_snapshots = array_slice( $sort_array, $numSnapshot );

            $param["snapshotIds"] = array();

		    for ($i=0; $i<count($excess_snapshots); $i++) 
                array_push( $param["snapshotIds"], $excess_snapshots[$i]["snapshotId"] );
            
            if( count( $param["snapshotIds"] ) > 0 )
			    $this->DeleteSnapshot( $param );				
	    }

        return true;
    }

/**
 * @brief  for sort snapshot by time
 * @return  snapshot
 */

    static function snapshot_cmp_Tencent( $a, $b) {

        if( strtotime( $a["createTime"] ) == strtotime( $b["createTime"] ) )
            return 0;

        return (strtotime( $a["createTime"] ) > strtotime( $b["createTime"] )) ? -1 : +1;
    }

/**
 * @brief  describe snapshots whitch create from specified disk
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch snapshot in
 * @param[in]   diskId              disk unique id
 * @return  snapshots information 
 */

    public function describe_snapshots($cloud_uuid, $zone, $diskId) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            "diskIds" => array( $diskId ),
            "zone" => $zone
        );

        $snapshots = $this->GetSnapshotList( $param );

        $snapshotsArray = array();

        foreach( $snapshots["snapshotSet"] as $snap ) {

            $tmp = array(
			    'status' 		=> $snap["snapshotStatus"],								
			    'name'			=> $snap["snapshotName"],
			    'volume_id'		=> $snap["storageId"],
			    'created_at'	=> $snap["createTime"],
			    'size'			=> $snap["storageSize"],
			    'progress'		=> $snap["percent"],
			    'id'			=> $snap["snapshotId"],
			    'description'	=> $snap["snapshotName"]
		    );

            array_push( $snapshotsArray, $tmp );
        }

        return $snapshotsArray;

    }

/**
 * @brief  describe specified snapshot information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of snapshot in
 * @param[in]   snapId              specified snapshot ids
 * @return  snapshots information 
 */

    public function describe_snapshot_detail( $cloud_uuid, $zone, $snapId ) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            //"snapshotIds" => array( $snapId ),
            "zone" => $zone
        );

        if( $snapId != "UseLastSnapshot")
            $param["snapshotIds"] = array( $snapId );

        $snapshots = $this->GetSnapshotList( $param );

        $snapshotsArray = array();

        foreach( $snapshots["snapshotSet"] as $snap ) {

            $tmp = array(
			    'status' 		=> $snap["snapshotStatus"],								
			    'name'			=> $snap["snapshotName"],
			    'volume_id'		=> $snap["storageId"],
			    'created_at'	=> $snap["createTime"],
			    'size'			=> $snap["storageSize"],
			    'progress'		=> $snap["percent"],
			    'id'			=> $snap["snapshotId"],
			    'description'	=> $snap["snapshotName"]
		    );

            array_push( $snapshotsArray, $tmp );
        }

        return $snapshotsArray;
    }

/**
 * @brief  describe instance types
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in,option]   type         if null, describe all types, otherwise describe specified type
 * @return  instance type information 
 */

    public function describe_zone_instance_types( $cloud_uuid, $region) {

        $this->AuthTencent( $cloud_uuid );

        $param = array( "zone" => $region );

        $types = $this->GetInstanceTypes( $param )["Response"];

        $typesArray = array();

        foreach($types["InstanceTypeConfigSet"] as $type) {

            $tmp = array(
			    'Zone' 		        => $type["Zone"],								
			    'InstanceFamily'    => $type["InstanceFamily"],
			    'InstanceType'		=> $type["InstanceType"],
			    'CPU'	            => $type["CPU"],
			    'Memory'			=> $type["Memory"],
			    'GPU'		        => $type["GPU"],
			    'FPGA'			    => $type["FPGA"]
		    );

            array_push( $typesArray, $tmp );
        }

        return $typesArray;
    }

/**
 * @brief  create disk from snapshot
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of disk in
 * @param[in]   snapId              snapshot unique id
 * @param[in]   instanceId          for named snapshot
 * @return  disk unique id 
 */

    public function create_volume_from_snapshot( $cloud_uuid, $zone, $snapId, $instanceId, $diskType, $diskSize) {

        $this->AuthTencent( $cloud_uuid );

        $now = Misc_Class::current_utc_time();

        $param = array(
            //"region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $zone,
            "tag.1" => array( "key"=>"description",
                "value" => "Disk Created By Saasame Transport Service@".$now ),
            "snapId" => $snapId,
            "diskType" => "CLOUD_BASIC"
            //"diskName" => "snap-".$instanceId."_".strtotime($now)
        );

        if( $diskType == "SSD" ){
            $param["diskType"] = "CLOUD_SSD";
            $param["size"] = ($diskSize >= 100 )?$diskSize:100;
        }

        $diskRet = $this->CreateDiskV2( $param );

        if( !isset( $diskRet["DiskIdSet"][0] ) )
            return false;

        return $diskRet["DiskIdSet"][0];
    }

/**
 * @brief  describe instance types
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in,option]   type         if null, describe all types, otherwise describe specified type
 * @return  instance type information 
 */

    public function describe_instance_types( $cloud_uuid, $region, $instanceType = null ) {

        $this->AuthTencent( $cloud_uuid );

        $param = array( "zone" => $region );

        $types = $this->GetInstanceTypes( $param )["Response"];

        $typesArray = array();

        foreach($types["InstanceTypeConfigSet"] as $type) {

            $tmp = array(
			    'Zone' 		        => $type["Zone"],								
			    'InstanceFamily'    => $type["InstanceFamily"],
			    'InstanceType'		=> $type["InstanceType"],
			    'CPU'	            => $type["CPU"],
			    'Memory'			=> $type["Memory"],
			    'GPU'		        => $type["GPU"],
			    'FPGA'			    => $type["FPGA"]
		    );

            if( isset( $instanceType ) ) {
                if( $type["InstanceType"] == $instanceType )
                    return $tmp;
            }
            else
                array_push( $typesArray, $tmp );   
        }

        return $typesArray;
    }

/**
 * @brief  describe vpc and switch information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of vpc in
 * @param[in,option]   netId        vpc id.if null, describe all vpc and switch, otherwise describe specified type
 * @return  vpc information 
 */

    public function describe_internal_network($cloud_uuid, $zone, $netId = null) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            "zone" => $this->getCurrectRegion( $zone )
        );

        if( isset( $netId ) )
            $param["vpcId"] = $netId;

        $vpcs = $this->GetVPCList( $param );

        $retvpc = array();

        foreach ( $vpcs["data"] as $vpc ) {

            $temp = array();
            $temp["VpcId"]      = $vpc["vpcId"];
            $temp["unVpcId"]      = $vpc["unVpcId"];
            $temp["VpcName"]    = $vpc["vpcName"];
            $temp["CidrBlock"]  = $vpc["cidrBlock"];
            $temp["CreateTime"]  = $vpc["createTime"];

            array_push( $retvpc, $temp );
        }

        $retvpc["subnets"] = $this->GetSubnetList( $param );

        return $retvpc;
    }

/**
 * @brief  describe security groups information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of security group in
 * @param[in,option]   sgId         security id. if null, describe all security groups, otherwise describe specified one
 * @return  security groups information 
 */

    public function describe_security_groups($cloud_uuid, $zone, $sgId = null ) {

        $this->AuthTencent( $cloud_uuid );

        if( isset( $sgId ) )
            $param["sgId"] = json_encode( array( $sgId ) );

        $param = array(
            "zone" => $this->getCurrectRegion( $zone )
        );

        $sgs = $this->GetSecurityGroups( $param )["data"]["detail"];

        $retsg = array();

        foreach ( $sgs as $sg ) {

            $temp = array();
        
            $temp["SecurityGroupId"]   = $sg["sgId"];
            $temp["SecurityGroupName"] = $sg["sgName"];
            $temp["sgRemark"]          = $sg["sgRemark"];
            $temp["createTime"]        = $sg["createTime"];

            array_push( $retsg, $temp );
        }

        return $retsg;
    }

    /**
 * @brief  attach disk to instance
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   instanceId          instance unique id
 * @param[in]   diskId              disk unique id
 * @return  result from alyun 
 */

    public function attach_volume($cloud_uuid, $zone, $instanceId, $diskId) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            "instanceId" => $instanceId,
            "diskIds" => array( $diskId ),
            "zone" => $zone
        );

        $ret = $this->AttachDiskToInstance( $param );

        return $ret;

    }

    public function VerifyOss( $cloud_uuid, $region , $bucket ) {

        $this->AuthTencent( $cloud_uuid, $region );

        try {

            $result = $this->cosClient->listBuckets();

            $hit = array_search( $bucket.'-'.$this->AppId, array_column($result["Buckets"], 'Name'));
            
            if( $hit === false ) {
                $result = $this->cosClient->createBucket(array('Bucket' => $bucket));
            }
            
        } catch (\Exception $e) {
            Misc_Class::function_debug("Tencent", "VerifyOss", $e->getMessage());
            return false;
        }
            
        return true;
    }

    public function DeleteObjectFromStorage( $cloud_uuid, $region , $bucket, $object, $isInternal ) {

        $this->AuthTencent( $cloud_uuid, $region );

        try {

            $result = $this->cosClient->deleteObject(array(
                'Bucket' => $bucket.'-'.$this->AppId,
                'Key' => $object));
            
                return true;

        } catch (\Exception $e) {
            Misc_Class::function_debug("Tencent", "DeleteObjectFromStorage", $e->getMessage());
            return false;
        }
            
        return true;
    }

/**
 * @brief  delete disk, if there is any snapshot create from the disk, it will be delete too.
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              region of snapshot and image
 * @param[in]   diskId              disk unique id
 * @return  none
 */

    public function delete_volume( $cloud_uuid, $zone, $diskId ) {

        $this->AuthTencent( $cloud_uuid );

        $param = array(
            "diskIds" => array( $diskId ),
            "zone" => $zone
        );

        $snapshots = $this->GetSnapshotList( $param );

        $param = array(
            "diskIds" => array( $diskId ),
            "zone" => $zone
        );

        $diskInfo = $this->GetDiskInfo( $param );

        if( isset( $diskInfo["storageSet"][0]["uInstanceId"] ) && 
            $diskInfo["storageSet"][0]["uInstanceId"] != "" ) {

            $param = array(
                "diskIds" => array( $diskId ),
                "zone" => $zone
            );
            
            $ret = $this->DettachDiskFromInstance( $param );

        }

        $param["snapshotIds"] = array();

        foreach( $snapshots["snapshotSet"] as $snap ) 
            array_push( $param["snapshotIds"], $snap["snapshotId"] );
        
        if( count( $param["snapshotIds"] ) > 0 )
            $this->DeleteSnapshot( $param );
                
        sleep( 20 );

        $param = array(
            "diskId" => $diskId,
            "zone" => $zone
        );

        $this->DeleteDiskV2( $param );
    }

/**
 * @brief  detach disks, delete the snapshot from the disks and delete the disks
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch snapshot in
 * @param[in]   instanceId          instance unique id, for detach disk
 * @param[in]   diskArray           disk array, OPEN_DISK_UUID is the disk unique id
 * @return  none 
 */

    public function delete_replica_job( $cloud_uuid, $region, $instanceId, $diskArray ) {

        foreach( $diskArray as $diskInfo ) {

            $diskId = $diskInfo['OPEN_DISK_UUID'];

            $this->delete_volume($cloud_uuid, $region, $diskId);

        }

        return true;
    }
/**
 * @brief  delete instance, it will hook until instance really terminated
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   InstanceId          instance unique id
 * @return  none
 */

    public function terminate_instances($cloud_uuid, $zone ,$InstanceId ) {

       $this->AuthTencent( $cloud_uuid );

        $param = array(
            "InstanceIds" => array( $InstanceId ),
            "zone" => $zone
        );

        /*$this->StopInstance( $param );

        do{
            sleep(5);

            $Instances = $this->ListInstances( $InstanceId )->{"Instances"};

            if( count( $Instances->{"Instance"} ) == 0 )
                break;

        }while( strcmp( $Instances->{"Instance"}[0]->{'Status'}, "Stopped" ) != 0 );
*/
        //if( count( $Instances->{"Instance"} ) != 0 ) {	
            $this->DeleteInstance( $param );	
            sleep(5);
        //}

       /* $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "imageId" => $Instances->{"Instance"}[0]->{"ImageId"}
        );
        
        $this->DeleteImage( $param );*/

    }

/**
 * @brief  transform a os disk to snapshot then to image, and then create instance from the image
 * @param[in]   servInfo            service information, include cloud uuid, packer uuid...
 * @param[in]   zone              not used
 * @param[in]   diskIds             disk(id) array, first one is the os disk, the others is data disk
 * @param[in]   InstanceId          for named snapshot
 * @return  instance id whitch created
 */

    public function run_instance_from_image($servInfo, $zone, $diskIds, $imageName) {

        sleep(180);

       $this->AuthTencent( $servInfo['CLUSTER_UUID'], $zone );

       $net = explode( '|', $servInfo['SGROUP_UUID']);

        $AliModel = new Aliyun_Model();
        $server   = new Server_Class();
        $service  = new Service_Class();
        $replica  = new Replica_Class();
        
        $diskType = "CLOUD_BASIC";

        $serviceConfig = json_decode($servInfo['JOBS_JSON'], true);

        $allowPublicIP = $serviceConfig["elastic_address_id"];
        
        $specifiedPrivateIP = $serviceConfig["private_address_id"];

        if( $serviceConfig["disk_type"] == "SSD" )
            $diskType = "CLOUD_SSD";

        $this->LogJobId = $servInfo["SERV_UUID"];

        $hostInfo = $AliModel->getServerHostInfo( $servInfo['PACK_UUID'] );

        $sg = $net[0];

        $subnet = $net[1];

        $now = Misc_Class::current_utc_time();

        $param = array( "ImageName" => $imageName );

        $image = $this->GetImages( $param );

        $param = array(
            "Zone" => $zone,
            "InstanceType" => $servInfo['FLAVOR_ID'],
            "ImageId" => $image["Response"]["ImageSet"][0]["ImageId"],
            "InstanceName" => $hostInfo[0]["_HOST_NAME"].strtotime( $now ),
            "VirtualPrivateCloud.VpcId" => $servInfo["NETWORK_UUID"],
            "VirtualPrivateCloud.SubnetId" => $subnet,
            "SecurityGroupIds.1" => $sg,
            "InternetAccessible" => ($allowPublicIP != "No")?1:0,
            "LoginSettings.Password" => $servInfo["Password"],
            "SystemDisk.DiskType" => $diskType
        );

        if( $specifiedPrivateIP != "DynamicAssign" )
            $param["VirtualPrivateCloud.PrivateIpAddresses"] = $specifiedPrivateIP;

        $MESSAGE = $replica->job_msg('Creating a new instance from custom image %1%.',array($imageId));
        $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $instanceResponse = $this->CreateInstance( $param );

        if( !isset( $instanceResponse ) ) 
            return false;

        $MESSAGE = $replica->job_msg('Instance %1% created.',array($instanceResponse["Response"]["InstanceIdSet"][0]));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        sleep(10);

        $param = array(
            "instanceId" => $instanceResponse["Response"]["InstanceIdSet"][0],
            "zone" => $zone
        );

        $param["diskIds"] = array();

        foreach( $diskIds as $k => $d ){

            if( $k == 0 ) {

                $this->delete_volume( $servInfo['CLUSTER_UUID'], $zone, $d );

                continue;
            }

            array_push( $param["diskIds"], $d);

            $MESSAGE = $replica->job_msg('Attaching disk %1% to instance.',array($d));
		    $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	
        }
        
        if( count( $param["diskIds"] ) > 0 ){

            $flag = true;
        
            while( $flag ){

                $ret = $this->AttachDiskToInstance( $param );

                if( $ret !== false )
                    break;

                sleep( 60 );

                $MESSAGE = $replica->job_msg('Trying to attach data disk.');
		        $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');
            }
        }

        $MESSAGE = $replica->job_msg('Recover Workload process completed.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $param = array(
            "ImageIds" => array( $image["Response"]["ImageSet"][0]["ImageId"] ),
            "zone" => $zone
        );

        $ret = $this->DeleteImage( $param );

        //$this->DeleteAutoSnapshotFromImage($servInfo, $zone, "saasame-".$servInfo["SERV_UUID"]);

        return $instanceResponse["Response"]["InstanceIdSet"][0];

    }
    
    public function getCurrectRegion( $zone ) {

        $r = explode( '-', $zone );

        if( isset($r[1]))
            return $r[0].'-'.$r[1];
        else
        return $r[0];
    }
}

?>