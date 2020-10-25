<?php

include_once 'Integrate_Interface.php';
include_once 'aliyun-openapi-php-sdk-master/aliyun-php-sdk-core/Config.php';
require_once 'aliyun-oss-php-sdk-2.2.3.phar';
use Ecs\Request\V20140526 as Ecs;
use Ram\Request\V20150501 as Ram;

class Aliyun_Controller implements SaasameInterface
{
    private $accessKey;

    private $accessSecret;

    private $RegionId;

    private $client;

    private $url;

    private $signature;

    private $ossClient;

    private $LogJobId;

    public function __construct() {
	}

    public function SetKey( $accessKey, $accessSecret, $region = "ap-southeast-1" ) {
        $this->accessKey = $accessKey;
        $this->accessSecret = $accessSecret;
        $iClientProfile = DefaultProfile::getProfile( $region, $this->accessKey, $this->accessSecret);
        $this->client = new DefaultAcsClient($iClientProfile);
    }

    private function getkey() {
        return $this->accessKey;
    }

    private function getSecret() {
        return $this->accessSecret;
    }

    public function setRegionId( $RegionId ) {
        $this->RegionId = $RegionId;
    }

    public function CreateInstance( $ImageId, $InstanceType ) {

    }

    public function CreateInstanceI( $param ) {
        $request = new Ecs\CreateInstanceRequest();
        
        $request->setMethod("POST"); 

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["zone"] ) )
            $request->setZoneId( $param["zone"] );
       
       if( isset( $param["imageId"] ) )
            $request->setImageId( $param["imageId"] );

        if( isset( $param["instanceType"] ) )
            $request->setInstanceType( $param["instanceType"] );
        
        if( isset( $param["securityGroup"] ) )
            $request->setSecurityGroupId( $param["securityGroup"] );

        if( isset( $param["switch"] ) )
            $request->setVSwitchId( $param["switch"] );

        if( isset( $param["password"] ) )
            $request->setPassword( $param["password"] );

        if( isset( $param["instanceName"] ) )
            $request->setInstanceName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $param["instanceName"] ) );
        
        if( isset( $param["internetMaxBandwidthIn"] ) )
            $request->setInternetMaxBandwidthIn( $param["internetMaxBandwidthIn"] );
            
        if( isset( $param["internetMaxBandwidthOut"] ) )
            $request->setInternetMaxBandwidthOut( $param["internetMaxBandwidthOut"] );

        if( isset( $param["description"] ) )
            $request->setDescription( $param["description"] );

        if( isset( $param["hostName"] ) )
            $request->setHostName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9.-]/", "", $param["hostName"] ) );

        if( isset( $param["systemDiskDiskName"] ) )
            $request->setSystemDiskDiskName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $param["systemDiskDiskName"] ) );

        if( isset( $param["privateIp"] ) )
            $request->setPrivateIpAddress( $param["privateIp"] );

        if( isset( $param["diskType"] ) )
            $request->setSystemDiskCategory( $param["diskType"] );
        //$request->setDataDisk1Category('cloud_efficiency');

        $response = $this->SendRequest($request, "CreateInstanceRequest", $param, true);

        return $response;
    }

    public function StartInstance( $param ) {
        $request = new Ecs\StartInstanceRequest();
        
        $request->setMethod("POST"); 

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

       $response = $this->SendRequest($request, "StartInstanceRequest", $param);

        return $response;
    }

    public function GetInstanceStatus( $param ) {
        $request = new Ecs\DescribeInstanceStatusRequest();
        
        $request->setMethod("POST"); 

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        $request->setPageSize( 100 );

        $response = $this->SendRequest($request, "DescribeInstanceStatusRequest", $param);

        return $response;
    }
    
    public function DeleteInstance( $param ) {

        $request = new Ecs\DeleteInstanceRequest();

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

        $response = $this->SendRequest($request, "DeleteInstanceRequest", $param);

        return $response;
    }

    public function StopInstance( $param ) {

        $request = new Ecs\StopInstanceRequest();

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );
        
        $response = $this->SendRequest($request, "StopInstanceRequest", $param);

        return $response;
    }

    public function ListInstances( $instanceId = null ) {

        $request = new Ecs\DescribeInstancesRequest();
        $request->setRegionId( $this->RegionId );

        if( $instanceId ) {
            $request->setInstanceIds( json_encode(array( $instanceId )) );
        }

        $request->setPageSize( 100 );

        $response = $this->SendRequest($request, "DescribeInstancesRequest", $instanceId);

        return $response;
    }

    public function GetInstanceDetail() {
        $req = new Ecs\DescribeInstanceAttributeRequest();
        $req->setInstanceId("<InstanceId>");
        try {
            $resp = $this->client->getAcsResponse($req);
            if(!isset($resp->Code))
            {
                // 查询成功
                // 查看实例信息相关代码
                // ......
                echo($resp->RequestId);
                print_r($resp);
            }
            else
            {
                // 查询失败
                $code = $resp->Code;
                $message = $resp->Message;
            }
        }
        catch (Exception $e)
        {
            // TODO: handle exception
        }
    }

    public function CreateVolume() {}

    public function DeleteVolume( $vol ) {}

    public function ListVolume() {}

    public function GetDiskInfo( $param ) {

        $request = new Ecs\DescribeDisksRequest();

        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["zone"] ) )
            $request->setZoneId( $param["zone"] );

        if( isset( $param["diskId"] ) )
            $request->setDiskIds( $param["diskId"] );

        if( isset( $param["diskType"] ) )
            $request->setDiskType( $param["diskType"] );

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

        $request->setPageSize( 100 );

        $response = $this->SendRequest($request, "DescribeDisksRequest", $param);

        return $response;
    }

    public function CreateSnapshot( $param ) {

        $request = new Ecs\CreateSnapshotRequest();

        $request->setMethod("GET");

        if( isset( $param["diskId"] ) )
            $request->setDiskId( $param["diskId"] );

        if( isset( $param["snapshotName"] ) )
            $request->setSnapshotName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $param["snapshotName"] ) );

        if( isset( $param["description"] ) )
            $request->setDescription( $param["description"] );
        
        $response = $this->SendRequest($request, "CreateSnapshotRequest", $param);

        return $response;
    }

    public function DeleteSnapshotI( $SnapshotId ) {

        $request = new Ecs\DeleteSnapshotRequest();

        $request->setSnapshotId( $SnapshotId );

        $response = $this->SendRequest($request, "DeleteSnapshotRequest", $SnapshotId);

        return $response;
    }
    
    public function CreateImage( $param ) {

        $DiskDeviceMappings = array(array(
            "DiskType" => "system",
            "Size" => $param["size"],
            "SnapshotId" => $param["snapshotId"],
            "Device" => ""
        ));

        $request = new Ecs\CreateImageRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );
   
        if( isset( $param["snapshotId"] ) )
            $request->setSnapshotId( $param["snapshotId"] );
        
        if( isset( $param["arch"] ) )
            $request->setArchitecture( $param["arch"] );
        
        if( isset( $param["platfrom"] ) )
            $request->setPlatform( $param["platfrom"] );
 
        $request->setDiskDeviceMappings( $DiskDeviceMappings );

        $response = $this->SendRequest($request, "CreateImageRequest", $param);

        return $response;
    }

    public function GetImage( $param ) {

        $request = new Ecs\DescribeImagesRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );
   
        if( isset( $param["snapshotId"] ) )
            $request->setSnapshotId( $param["snapshotId"] );

        if( isset( $param["imageId"] ) )
            $request->setImageId( $param["imageId"] );

        $request->setPageSize( 100 );

        $response = $this->SendRequest($request, "DescribeImagesRequest", $param);

        return $response;
    }

    public function DeleteImage( $param ) {

        $request = new Ecs\DeleteImageRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );
   
        if( isset( $param["imageId"] ) )
            $request->setImageId( $param["imageId"] );

        $request->setForce( true );

        $response = $this->SendRequest($request, "DeleteImageRequest", $param);

        return $response;
    }

    public function ImportImage( $param ) {

        $request = new Ecs\ImportImageRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );
   
        if( isset( $param["imageName"] ) )
            $request->setImageName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $param["imageName"] ) );

        if( isset( $param["platform"] ) )
            $request->setPlatform( $param["platform"] );

        if( isset( $param["oSType"] ) )
            $request->setOSType( $param["oSType"] );

        if( isset( $param["architecture"] ) )
            $request->setArchitecture( $param["architecture"] );

        $DiskDeviceMappings = array(array(
            "Format" => $param["format"],
            "OSSBucket" => $param["bucket"],
            "OSSObject" => $param["object"],
            "DiskImSize" => $param["imSize"],
            "Device" => "",
            "DiskImageSize" => $param["imageSize"]
        ));

        $request->setDiskDeviceMappings( $DiskDeviceMappings );

        $response = $this->SendRequest($request, "ImportImageRequest", $param);

        return $response;
    }

    public function getTaskAttribute( $param ) {

        $request = new Ecs\DescribeTaskAttributeRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["taskId"] ) )
            $request->setTaskId( $param["taskId"] );

        $response = $this->SendRequest($request, "DescribeTaskAttributeRequest", $param);

        return $response;
    }

    public function getTaskList( $param ) {

        $request = new Ecs\DescribeTasksRequest();

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        $response = $this->SendRequest($request, "DescribeTasksRequest", $param);

        return $response;
    }

    public function CreateVolFromSnapshot() {}

    public function DeleteSanpshot() {}

    public function ListSnapshot() {}

    public function AttachVolToInstance() {}

    public function DetachVolFromInstance() {}

    public function AttachDiskToInstance( $param ) {

        $request = new Ecs\AttachDiskRequest();

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

        if( isset( $param["diskId"] ) )
            $request->setDiskId( $param["diskId"] );

        $response = $this->SendRequest($request, "AttachDiskRequest", $param);

        return $response;
    }

    public function DettachDiskToInstance( $param ) {

        $request = new Ecs\DetachDiskRequest();

        if( !isset( $param["instanceId"] ) || !isset( $param["diskId"] ) || $param["instanceId"] == '')
            return null;

        $request->setInstanceId( $param["instanceId"] );

        $request->setDiskId( $param["diskId"] );

        $response = $this->SendRequest($request, "DetachDiskRequest", $param);

        return $response;
    }

    public function CheckConnect() {
        return $this->GetRegion();
    }

    public function GetRegion() {
        $request = new Ecs\DescribeRegionsRequest();
        $request->setMethod("GET");
        $response = $this->SendRequest($request, "DescribeRegionsRequest");
        return $response;
    }

    public function GetSecurityGroup( $param ) {

        $request = new Ecs\DescribeSecurityGroupsRequest();

        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["sgId"] ) )
            $request->setSecurityGroupIds( $param["sgId"] );

        $request->setPageSize( 50 );
        
        $response = $this->SendRequest($request, "DescribeSecurityGroupsRequest", $param);

        return $response;
    }

    public function GetSnapshotList( ) {
    }

    public function DescribeAvailableResource( $param ) {
        $request = new Ecs\DescribeAvailableResourceRequest();
        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["destinationResource"] ) )
            $request->setDestinationResource( $param["destinationResource"] );

        if( isset( $param["zoneId"] ) )
            $request->setZoneId( $param["zoneId"] );

        if( isset( $param["instanceChargeType"] ) )
            $request->setInstanceChargeType( $param["instanceChargeType"] );

        if( isset( $param["ioOptimized"] ) )
            $request->setIoOptimized( $param["ioOptimized"] );

        $response = $this->SendRequest($request, "DescribeAvailableResource", $param);
        return $response;
    }

    public function GetSnapshotListI( $param ) {
        $request = new Ecs\DescribeSnapshotsRequest();
        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["diskId"] ) )
            $request->setDiskId( $param["diskId"] );

        if( isset( $param["snapshotId"] ) )
            $request->setSnapshotIds( $param["snapshotId"] );

        if( isset( $param["snapshotName"] ) )
            $request->setSnapshotName( $param["snapshotName"] );

        $request->setPageSize( 100 );

        $response = $this->SendRequest($request, "DescribeSnapshotsRequest", $param);
        return $response;
    }

    public function CreateMachine() {}

    public function DescribeRegionsRequest() {
        $request = new Ecs\DescribeRegionsRequest();
        $request->setMethod("GET");
        $response = $this->client->getAcsResponse($request);
       // return $response;
        echo '<pre>';
        echo json_encode($response);
        echo '</pre>';
    }

    public function getImagesList(){

        $request = new Ecs\DescribeImagesRequest();
        $request->setMethod("GET");
        $request->setRegionId( $this->RegionId );
        $response = $this->SendRequest($request, "DescribeImagesRequest");
        return $response;
    }

    public function replaceSystemDisk( $InstanceId, $ImageId ) {

        $request = new Ecs\ReplaceSystemDiskRequest();
        $request->setMethod("GET");
        $request->setInstanceId( $InstanceId );
        $request->setImageId( $ImageId );
        $response = $this->SendRequest($request, "replaceSystemDisk");
        return $response;
    }

    public function getInstanceTypeList() {
        $request = new Ecs\DescribeInstanceTypesRequest();
        $request->setMethod("GET");
        $response = $this->SendRequest($request, "DescribeInstanceTypesRequest");
        return $response;
    }

    public function getVpcList( $param ) {

        $request = new Ecs\DescribeVpcsRequest();

        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["vpcId"] ) )
            $request->setVpcId( $param["vpcId"] );

        $request->setPageSize( 50 );

        $response = $this->SendRequest($request, "DescribeVpcsRequest");

        return $response;
    }

    public function getVSwitchList( $param ) {

        $request = new Ecs\DescribeVSwitchesRequest();

        $request->setMethod("GET");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["vpcId"] ) )
            $request->setVpcId( $param["vpcId"] );

        $request->setPageSize( 50 );

        $response = $this->SendRequest($request, "DescribeVSwitchesRequest");

        return $response;
    }

    public function CreateDiskI( $param ) {

        $request = new Ecs\CreateDiskRequest();
        $request->setMethod("POST");

        if( isset( $param["region"] ) )
            $request->setRegionId( $param["region"] );

        if( isset( $param["zone"] ) )
            $request->setZoneId( $param["zone"] );

         if( isset( $param["size"] ) )
            $request->setSize( $param["size"] );
        
        if( isset( $param["description"] ) ) {
            $request->setDescription( $param["description"] );
        }

        if( isset( $param["snapId"] ) ) {
            $request->setSnapshotId( $param["snapId"] );
        }

        if( isset( $param["diskName"] ) ) {
            $request->setDiskName( preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $param["diskName"] ) );
        }

        if( isset( $param["diskType"] ) ) {
            $request->setDiskCategory($param["diskType"]);
        }
        else{
            $request->setDiskCategory('cloud_efficiency');
        }

        $response = $this->SendRequest($request, "CreateDiskRequest", $param);

        return $response;
    }

    public function DeleteDiskI( $diskId ) {

        $request = new Ecs\DeleteDiskRequest();

        $request->setMethod("POST");

        $request->setDiskId( $diskId );

        $response = $this->SendRequest($request, "DeleteDiskRequest", $diskId);

        return $response;
    }

    public function getZones( $Region = "ap-southeast-1" ) {
        $request = new Ecs\DescribeZonesRequest();
        $request->setMethod("GET");
        $request->setRegionId( $Region );
        $response = $this->SendRequest($request, "DescribeZonesRequest");
        return $response;
    }

    public function GenSignatureAndUrl( $param_array ) {
        array_push($param_array,"Version=2014-05-26");
        array_push($param_array,"AccessKeyId=".$this->accessKey);
        array_push($param_array,"SignatureMethod=HMAC-SHA1");
        array_push($param_array,"Format=XML");
        array_push($param_array,"Timestamp=".urlencode(gmdate("Y-m-d\TH:i:s\Z")));
        array_push($param_array,"SignatureVersion=1.0");
        array_push($param_array,"SignatureNonce=".uniqid());
        
        asort( $param_array );
        $url = '';
        foreach( $param_array as $param )
            $url .= $param.'&';
        $url = rtrim($url,'&');

        $this->signature = base64_encode( hash_hmac( 'sha1','GET&%2F&'.urlencode ($url), $this->accessSecret.'&', true ));
        $this->url = 'http://ecs.aliyuncs.com/?'.$url.'&Signature='.$this->signature;

    }

    public function AllocatePublicIpAddress( $param ) {

        $request = new Ecs\AllocatePublicIpAddressRequest();

        $request->setMethod("GET");

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

        $response = $this->SendRequest($request, "AllocatePublicIpAddressRequest",$param);

        return $response;
    }

    public function ModifyInstanceAttribute( $param ) {

        $request = new Ecs\ModifyInstanceAttributeRequest();

        $request->setMethod("GET");

        if( isset( $param["instanceId"] ) )
            $request->setInstanceId( $param["instanceId"] );

        if( isset( $param["password"] ) )
            $request->setPassword( $param["password"] );

        $response = $this->SendRequest($request, "ModifyInstanceAttributeRequest",$param);

        return $response;
    }

/**
 * @brief  send request throught sdk and write log when get exception
 * @param[in]   request             the sdk request object
 * @param[in]   action              the action for sdk
 * @param[in]   p                   reserv parameter, write info to log
 * @return  result from aliyun ( object )
 */

    public function SendRequest( $request , $action, $p = null, $isShowError = false ) {

        $param = array( "action" => $action );

        if( isset( $p ) )
            $param["param"] = $p;

        $response = false;

        try{
            $response = $this->client->getAcsResponse($request);

            if( isset( $response->Code ) ) {
                $param["Code"] = $response->Code;
                Misc_Class::function_debug('Aliyun', $action, $response->Code);
            }

            return $response;
        }
        catch (Exception $e) {

            $error = $e->getMessage();
            switch( $action ){
                case "DetachDiskRequest":
                case "DeleteDiskRequest":

                if( strpos( $error, "InvalidOperation.Conflict") !== false )
                    throw new Exception($error."\n");
                break;

                default:
                break;
            };

            $param["Msg"] = $error;
            $param["CallStack"] = debug_backtrace();
            Misc_Class::function_debug('Aliyun', $action, $param);	
            if( $isShowError ) {
                $replica  = new Replica_Class();
                $mesage = $replica->job_msg($param["Msg"]);
                $replica->update_job_msg( $this->LogJobId , $mesage, 'Service');
            }
        }

        return $response;
    }
    
    public function GetSignature() {
        return $this->signature;
    }

    public function GetUrl() {
        return $this->url;
    }

/**
 * @brief  get cloud key from database and init to used for API
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @return  none
 */

    public function AuthAli( $CloudId ) {

        $AliModel = new Aliyun_Model();

        $cloud_info = $AliModel->query_cloud_connection_information( $CloudId );

        $this->SetKey( $cloud_info['ACCESS_KEY'], $cloud_info['SECRET_KEY'] );
    }

/**
 * @brief  describe instance in all regions
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @return  instance information like name, ip, id...
 */

    public function describe_all_instances( $cloud_uuid ) {

        $this->AuthAli( $cloud_uuid );

        $regions = $this->GetRegion();

        $instances = array();

        foreach( $regions->{"Regions"}->{"Region"} as $region ) {
            $this->setRegionId( $region->{"RegionId"} );
            $listInstance = $this->ListInstances();

           // if( $listInstance )
            foreach( $listInstance->{"Instances"}->{"Instance"} as $Instance ) {
                $temp = array(
                    "InstanceName" => $Instance->{"InstanceName"},
                    "PrivateIpAddress" => $Instance->{"VpcAttributes"}->{"PrivateIpAddress"}->{"IpAddress"}[0],
                    "PublicIpAddress" => isset($Instance->{"PublicIpAddress"}->{"IpAddress"}[0])? $Instance->{"PublicIpAddress"}->{"IpAddress"}[0]:"N/A",
                    "Region" => $region->{"RegionId"},
                    "Zone" => $Instance->{"ZoneId"},
                    "InstanceType" => $Instance->{"InstanceTypeFamily"},
                    "InstanceId" => $Instance->{"InstanceId"},
                    "Status" => $Instance->{"Status"}
                );

                if( $Instance->{"EipAddress"}->{"IpAddress"} != '' )
                    $temp["PublicIpAddress"] = $Instance->{"EipAddress"}->{"IpAddress"};

                array_push( $instances, $temp);
            }
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

        $this->AuthAli( $cloud_uuid );

        $this->setRegionId( $this->getCurrectRegion( $region ) );

        $Instance = $this->ListInstances( $instanceId )->{"Instances"}->{"Instance"}[0];
        
        $publicIp = "N/A";
        if( isset( $Instance->{"PublicIpAddress"}->{"IpAddress"}[0] ) )
            $publicIp = $Instance->{"PublicIpAddress"}->{"IpAddress"}[0];

        if( $Instance->{"EipAddress"}->{"IpAddress"} != '' )
            $publicIp = $Instance->{"EipAddress"}->{"IpAddress"};

        $Instance_Info = array(
            "InstanceName" => $Instance->{"InstanceName"},
            "PrivateIpAddress" => $Instance->{"VpcAttributes"}->{"PrivateIpAddress"}->{"IpAddress"}[0],
            "PublicIpAddress" => $publicIp,
            "Region" => $this->getCurrectRegion( $region ),
            "Zone" => $Instance->{"ZoneId"},
            "InstanceType" => $Instance->{"InstanceTypeFamily"},
            "InstanceId" => $Instance->{"InstanceId"},
            "Status" => $Instance->{"Status"},
            "SecurityGroup" => $Instance->{"SecurityGroupIds"}->{"SecurityGroupId"}[0],
            "NetworkInterfaces" => $Instance->{"NetworkInterfaces"}->{"NetworkInterface"},
            "CPU" => $Instance->{"Cpu"},
            "Memory" => $Instance->{"Memory"}
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

    public function begin_volume_for_loader($cloud_uuid, $region, $instanceId, $size, $hostName) {

        $this->AuthAli( $cloud_uuid );

        $now = Misc_Class::current_utc_time();	

        $hostName = $hostName.'-'.date('ymdHis',strtotime($now));

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "size" => ( $size >= 20 )? $size : 20,
            "description" => "Disk Created By Saasame Transport Service@".$now,
            "diskName" => $hostName
        );

        $diskRet = $this->CreateDiskI( $param );

        if( !isset( $diskRet->{"DiskId"} ) )
            return false;
            
        $param = array(
            "instanceId" => $instanceId,
            "diskId" => $diskRet->{"DiskId"}
        );

        $ret = $this->AttachDiskToInstance( $param );
        
        if( $ret == false )
            return false;
        
		$VOLUMD_INFO = (object)array(
								'serverId' 	 => $instanceId.'|'.$region,							
								'volumeId'	 => $diskRet->{"DiskId"},
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

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "diskId" => $diskId,
            "snapshotName" => $InstanceName.'-'.strtotime($time),
            "description" => "Snapshot Created By Saasame Transport Service@".$time
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

    public function snapshot_control( $cloud_uuid, $region ,$diskId, $numSnapshot ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "diskId" => $diskId
        );

        $snapshots = $this->GetSnapshotListI( $param );

        $sort_array = (array)$snapshots->{"Snapshots"}->{"Snapshot"};
        
        usort( $sort_array, array("Aliyun_Controller", "snapshot_cmp"));

		if ($sort_array != FALSE)
        {
		    $SLICE_SNAPSHOT = array_slice( $sort_array, $numSnapshot );

		    for ($i=0; $i<count($SLICE_SNAPSHOT); $i++)
			    $this->DeleteSnapshotI( $SLICE_SNAPSHOT[$i]->{"SnapshotId"} );				
	    }

        return true;
    }

/**
 * @brief  for sort snapshot by time
 * @return  snapshot
 */

    static function snapshot_cmp( $a, $b) {

        if( strtotime( $a->{"CreationTime"} ) == strtotime( $b->{"CreationTime"} ) )
            return 0;

        return (strtotime( $a->{"CreationTime"} ) > strtotime( $b->{"CreationTime"} )) ? -1 : +1;
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

        $this->AuthAli( $cloud_uuid );

        $rm_disk_array = array();

        foreach( $diskArray as $diskInfo ) {

            $diskId = $diskInfo['OPEN_DISK_UUID'];

            $param = array(
                "instanceId" => $instanceId,
                "diskId" => $diskId
            );

            if( $this->DettachDiskToInstance( $param ) == false ){

                for($i = 0; $i< 5; $i++){

                    sleep(10);

                    if( $this->DettachDiskToInstance( $param ) != false )
                        break;
                }
            }

            sleep( 10 );

            $param = array(
                "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
                "diskId" => $diskId
            );

            $snapshots = $this->GetSnapshotListI( $param );

            foreach( $snapshots->{"Snapshots"}->{"Snapshot"} as $snap ) {

                if( $this->DeleteSnapshotI( $snap->{"SnapshotId"} ) == false ){

                    for($i = 0; $i< 5; $i++){

                        sleep(10);
    
                        if( $this->DeleteSnapshotI( $snap->{"SnapshotId"} ) != false )
                            break;
                    }
                }
            }

            if( $this->DeleteDiskI( $diskId ) == false ){

                for($i = 0; $i< 5; $i++){

                    sleep(10);

                    if( $this->DeleteDiskI( $diskId ) != false )
                        break;
                }

            }
        }
    }

/**
 * @brief  describe snapshots whitch create from specified disk
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region whitch snapshot in
 * @param[in]   diskId              disk unique id
 * @return  snapshots information 
 */

    public function describe_snapshots($cloud_uuid, $region, $diskId) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "diskId" => $diskId
        );

        $snapshots = $this->GetSnapshotListI( $param );

        $snapshotsArray = array();

        foreach( $snapshots->{"Snapshots"}->{"Snapshot"} as $snap ) {

            $tmp = array(
			    'status' 		=> $snap->{"Status"},								
			    'name'			=> $snap->{"SnapshotName"},
			    'volume_id'		=> $snap->{"SourceDiskId"},
			    'created_at'	=> $snap->{"CreationTime"},
			    'size'			=> $snap->{"SourceDiskSize"},								
			    'progress'		=> $snap->{"Progress"},
			    'id'			=> $snap->{"SnapshotId"},
			    'description'	=> $snap->{"Description"}
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

    public function describe_instance_types( $cloud_uuid, $region, $type = null ) {

        $this->AuthAli( $cloud_uuid );

        $types = $this->getInstanceTypeList()->{"InstanceTypes"}->{"InstanceType"};

        if( isset($type) ){
            foreach( $types as $t ) {
                if( $t->{"InstanceTypeId"} == $type )
                    return $t;
            }
        }

        return $types;
    }

    static function type_cmp( $a, $b) {

        if( $a->CpuCoreCount == $b->CpuCoreCount ) {
            if( $a->MemorySize == $b->MemorySize )
                return 0;
            return ( $a->MemorySize > $b->MemorySize ) ? +1 : -1;
        }

        return ( $a->CpuCoreCount > $b->CpuCoreCount ) ? +1 : -1;
    }

/**
 * @brief  describe instance types
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in,option]   type         if null, describe all types, otherwise describe specified type
 * @return  instance type information 
 */

    public function describe_zone_instance_types( $cloud_uuid, $region) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" =>  $this->getCurrectRegion( substr($region, 0, -1) ),
            "destinationResource" => "InstanceType",
            "zoneId" => $region,
            "instanceChargeType" => "PostPaid",
            "ioOptimized" => "optimized"
        );

        $AvailableZones = $this->DescribeAvailableResource( $param );

        $param["ioOptimized"] = "none";
        $AvailableZones2 = $this->DescribeAvailableResource( $param );

        $types_info = $this->getInstanceTypeList()->{"InstanceTypes"}->{"InstanceType"};

        $typs = array();

        if( isset( $AvailableZones->AvailableZones->AvailableZone ) ) {

            $key = array_search( $region, array_column( $AvailableZones->AvailableZones->AvailableZone, 'ZoneId') );

            $AvailableZone = $AvailableZones->AvailableZones->AvailableZone[ $key ];

            $AvailableResource = $AvailableZone->AvailableResources->AvailableResource[0];

            foreach( $AvailableResource->SupportedResources->SupportedResource as $instanceType ){

                if( $instanceType->Status != "Available" )
                    continue;

                $key = array_search( $instanceType->Value, array_column( $types_info, 'InstanceTypeId') );

                array_push( $typs, $types_info[$key]);
            }
        }

        if( isset( $AvailableZones2->AvailableZones->AvailableZone ) ) {

            $key = array_search( $region, array_column( $AvailableZones2->AvailableZones->AvailableZone, 'ZoneId') );

            $AvailableZone = $AvailableZones2->AvailableZones->AvailableZone[ $key ];

            $AvailableResource = $AvailableZone->AvailableResources->AvailableResource[0];

            foreach( $AvailableResource->SupportedResources->SupportedResource as $instanceType ){

                if( $instanceType->Status != "Available" )
                    continue;
                        
                $key = array_search( $instanceType->Value, array_column( $types_info, 'InstanceTypeId') );

                array_push( $typs, $types_info[$key]);
            }
        }

        usort( $typs, array("Aliyun_Controller", "type_cmp"));

        return $typs;
    }
/**
 * @brief  describe vpc and switch information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of vpc in
 * @param[in,option]   netId        vpc id.if null, describe all vpc and switch, otherwise describe specified type
 * @return  vpc and switch information 
 */

    public function describe_internal_network($cloud_uuid, $region, $netId = null) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) )
        );

        if( isset( $netId ) )
            $param["vpcId"] = $netId;

        $vpc = $this->getVpcList( $param )->{"Vpcs"}->{"Vpc"};

        $switch = $this->getVSwitchList( $param )->{"VSwitches"}->{"VSwitch"};

        $vpc["switch_detail"] = $switch;

        return $vpc;
    }

/**
 * @brief  describe security groups information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of security group in
 * @param[in,option]   sgId         security id. if null, describe all security groups, otherwise describe specified one
 * @return  security groups information 
 */

    public function describe_security_groups($cloud_uuid, $region, $sgId = null ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) )
        );

        if( isset( $sgId ) )
            $param["sgId"] = json_encode( array( $sgId ) );

        $sg = $this->GetSecurityGroup( $param )->{"SecurityGroups"}->{"SecurityGroup"};

        return $sg;
    }

/**
 * @brief  describe specified snapshot information
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of snapshot in
 * @param[in]   snapId              specified snapshot ids
 * @return  snapshots information 
 */

    public function describe_snapshot_detail( $cloud_uuid, $region, $snapId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "snapshotId" => json_encode( array( $snapId ) )
        );

        $snapshots = $this->GetSnapshotListI( $param );

        $snapshotsArray = array();

        foreach( $snapshots->{"Snapshots"}->{"Snapshot"} as $snap ) {

            $tmp = array(
			    'status' 		=> $snap->{"Status"},								
			    'name'			=> $snap->{"SnapshotName"},
			    'volume_id'		=> $snap->{"SourceDiskId"},
			    'created_at'	=> $snap->{"CreationTime"},
			    'size'			=> $snap->{"SourceDiskSize"},								
			    'progress'		=> $snap->{"Progress"},
			    'id'			=> $snap->{"SnapshotId"},
			    'description'	=> $snap->{"Description"}
		    );

            array_push( $snapshotsArray, $tmp );
        }

        return $snapshotsArray;
    }

/**
 * @brief  create disk from snapshot
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              the region of disk in
 * @param[in]   snapId              snapshot unique id
 * @param[in]   instanceId          for named snapshot
 * @return  disk unique id 
 */

    public function create_volume_from_snapshot( $cloud_uuid, $region, $snapId, $instanceId, $diskType = "cloud_efficiency") {

        $this->AuthAli( $cloud_uuid );

        $now = Misc_Class::current_utc_time();

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            //"size" => ( $size >= 20 )? $size : 20,
            "description" => "Disk Created By Saasame Transport Service@".$now,
            "snapId" => $snapId,
            "diskName" => "snap-".$instanceId."_".date('ymdHis',strtotime($now)),
            "diskType" => $diskType
        );

        $diskRet = $this->CreateDiskI( $param );

        return $diskRet->{"DiskId"};
    }

/**
 * @brief  attach disk to instance
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   instanceId          instance unique id
 * @param[in]   diskId              disk unique id
 * @return  result from alyun 
 */

    public function attach_volume($cloud_uuid, $region, $instanceId, $diskId) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "instanceId" => $instanceId,
            "diskId" => $diskId
        );

        $ret = $this->AttachDiskToInstance( $param );

        return $ret;

    }

/**
 * @brief  detach disk from instance
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   instanceId          instance unique id
 * @param[in]   diskId              disk unique id
 * @return  result from alyun 
 */

    public function detach_volume($cloud_uuid, $region, $instanceId, $diskId) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "instanceId" => $instanceId,
            "diskId" => $diskId
        );


        for( $i=0;$i<5;$i++){

            try{
                return $this->DettachDiskToInstance( $param );
            }
            catch( Exception $e ){

                $errorMsg = $e->getMessage();

                if( strpos( $errorMsg, "InvalidOperation.Conflict") !== false )
                    sleep(60);
            }
        }
               
        

    }

/**
 * @brief  wait snapshot process finish
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   snapId              snapshot unique id
 * @return  snapshot information
 */

    public function waitSnapFinish( $cloud_uuid, $region, $snapId) {
        do{
            sleep(30);

            $snapInfo = $this->describe_snapshot_detail( $cloud_uuid, $region, $snapId );

        }while( (strcmp( $snapInfo[0]['status'], "accomplished" ) != 0) );

        return $snapInfo;
    }

/**
 * @brief  transform a os disk to snapshot then to image, and then create instance from the image
 * @param[in]   servInfo            service information, include cloud uuid, packer uuid...
 * @param[in]   region              not used
 * @param[in]   diskIds             disk(id) array, first one is the os disk, the others is data disk
 * @param[in]   InstanceId          for named snapshot
 * @return  instance id whitch created
 */

    public function run_instance_from_disk($servInfo, $region, $diskIds, $InstanceId) {

       $this->AuthAli( $servInfo['CLUSTER_UUID'] );

        $net = explode( '|', $servInfo['SGROUP_UUID']);

        $AliModel = new Aliyun_Model();
        $server   = new Server_Class();
        $service  = new Service_Class();
        $replica  = new Replica_Class();

        $serviceConfig = json_decode($servInfo['JOBS_JSON'], true);

        $specifiedPrivateIP = $serviceConfig["private_address_id"];

        $hostInfo = $AliModel->getServerHostInfo( $servInfo['PACK_UUID'] );

        $os = $this->enumOS( $hostInfo[0]["_HOST_INFO"] );

        $arch = $os["arch"];

        $platfrom = $os["platfrom"];

        $sg = $net[0];

        $switch = $net[1];

        $now = Misc_Class::current_utc_time();

        $param = array(
                "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
                "diskId" => $diskIds[0]
            );

        $snapshots = $this->GetSnapshotListI( $param );
        
        foreach( $snapshots->{"Snapshots"}->{"Snapshot"} as $snap ) {

            if( strcmp( $snap->{"Status"}, "accomplished" ) != 0 ) {
                Misc_Class::function_debug('',__FUNCTION__,$snap);
                $this->DeleteSnapshotI( $snap->{"SnapshotId"} );
            }
        }

        $MESSAGE = $replica->job_msg('Creating temporary snapshot from converted system volume.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $snap = $this->create_disk_snapshot( $servInfo['CLUSTER_UUID'], $region,$diskIds[0], $InstanceId, $now );
						
		$snapInfo = $this->waitSnapFinish( $servInfo['CLUSTER_UUID'], $region, $snap->{"SnapshotId"} );
        
        $MESSAGE = $replica->job_msg('Temporary snapshot %1% created.',array($snap->{"SnapshotId"}));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	
        
        $this->DeleteDiskI( $diskIds[0] );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "snapshotId" => $snap->{"SnapshotId"},
            "arch" => $arch,
            "platfrom" => $platfrom,
            "size" => $snapInfo[0]["size"]
        );

        $MESSAGE = $replica->job_msg('Creating custom image from snapshot %1%.',array($snap->{"SnapshotId"}));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

		$image = $this->CreateImage( $param );

        $MESSAGE = $replica->job_msg('Custom image %1% created.',array($image->{"ImageId"}));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "imageId" => $image->{"ImageId"},
            "instanceType" => $servInfo['FLAVOR_ID'],
            "securityGroup" => $sg,
            "switch" => $switch
        );
        
        if( $specifiedPrivateIP != "DynamicAssign") {
            $param["privateIp"] = $specifiedPrivateIP;
        }

        $MESSAGE = $replica->job_msg('Creating a new instance from custom image %1%.',array($image->{"ImageId"}));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	
        
        $id = $this->CreateInstanceI( $param )->{"InstanceId"};

        $MESSAGE = $replica->job_msg('Instance %1% created.',array($id));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        foreach( $diskIds as $k => $d ){

            if( $k != 0 ) {
                $param = array(
                    "instanceId" => $id,
                    "diskId" => $d
                );

                $MESSAGE = $replica->job_msg('Attaching disk %1% to instance.',array($d));
		        $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

                $ret = $this->AttachDiskToInstance( $param );

            }
        }

        $param = array(
            "instanceId" => $id
        );

        do{
            sleep(3);

            $status = $this->ListInstances( $id )->{"Instances"}->{"Instance"};

        }while( (strcmp( $status[0]->{'Status'}, "Pending" ) == 0) );
        
        $MESSAGE = $replica->job_msg('Starting instance.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $this->StartInstance( $param );
        
        $MESSAGE = $replica->job_msg('Recover Workload process completed.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        return $id;

    }

    /**
 * @brief  transform a os disk to snapshot then to image, and then create instance from the image
 * @param[in]   servInfo            service information, include cloud uuid, packer uuid...
 * @param[in]   region              not used
 * @param[in]   diskIds             disk(id) array, first one is the os disk, the others is data disk
 * @param[in]   InstanceId          for named snapshot
 * @return  instance id whitch created
 */

    public function run_instance_from_image($servInfo, $region, $diskIds, $imageId, $osHostName) {

        sleep(180);

       $this->AuthAli( $servInfo['CLUSTER_UUID'] );

        $net = explode( '|', $servInfo['SGROUP_UUID']);

        $AliModel = new Aliyun_Model();
        $server   = new Server_Class();
        $service  = new Service_Class();
        $replica  = new Replica_Class();
		
        $allowPublicIP = json_decode($servInfo['JOBS_JSON'])->elastic_address_id;
        
        $serviceConfig = json_decode($servInfo['JOBS_JSON'], true);

        $specifiedPrivateIP = $serviceConfig["private_address_id"];

        $instanceName = $serviceConfig["hostname_tag"];

        $this->LogJobId = $servInfo["SERV_UUID"];

        $hostInfo = $AliModel->getServerHostInfo( $servInfo['PACK_UUID'] );

        $sg = $net[0];

        $switch = $net[1];

        $now = Misc_Class::current_utc_time();

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "imageId" => $imageId,
            "instanceType" => $servInfo['FLAVOR_ID'],
            "securityGroup" => $sg,
            "switch" => $switch,
            "instanceName" => "RI-".$hostInfo[0]["_HOST_NAME"].'-'.date('ymdHis',strtotime($now)),
            "password" => $servInfo["Password"],
            "internetMaxBandwidthOut" => 100,
            "internetMaxBandwidthIn" => 100,
            "systemDiskDiskName" => $hostInfo[0]["_HOST_NAME"].'_0-'.date('ymdHis',strtotime($now)),
            "description" => "Instance Created By Saasame Transport Service@".$now
        );

        if( isset($instanceName) )
            $param["instanceName"] = $instanceName;

        if( isset($serviceConfig["disk_type"]) )
            $param["diskType"] = $serviceConfig["disk_type"];
        
        /*if( $servInfo['hostName'] != "" ){
            if($servInfo["OS_TYPE"] == "LX" )
                $param["hostName"] = substr( $servInfo['hostName'], 0, 128 );
            else
                $param["hostName"] = substr( $servInfo['hostName'], 0, 15 );
        }*/

        if( $osHostName != "" ){
            if($servInfo["OS_TYPE"] == "LX" )
                $param["hostName"] = substr( $osHostName, 0, 128 );
            else
                $param["hostName"] = substr( $osHostName, 0, 15 );
        }

        if( $specifiedPrivateIP != "DynamicAssign") {
            $param["privateIp"] = $specifiedPrivateIP;
        }
        
        $MESSAGE = $replica->job_msg('Creating a new instance from custom image %1%.',array($imageId));
        $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $id = $this->CreateInstanceI( $param )->{"InstanceId"};

        if( !isset( $id ) ) 
            return false;

        $MESSAGE = $replica->job_msg('Instance %1% created.',array($id));
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        sleep(10);

        foreach( $diskIds as $k => $d ){

            if( $k != 0 ) {
                $param = array(
                    "instanceId" => $id,
                    "diskId" => $d
                );

                $MESSAGE = $replica->job_msg('Attaching disk %1% to instance.',array($d));
		        $replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

                $ret = $this->AttachDiskToInstance( $param );

            }
        }

        $param = array(
            "instanceId" => $id,
            "password" => $servInfo["Password"]
        );

        $this->ModifyInstanceAttribute( $param );

        sleep(3);
        
        $param = array(
            "instanceId" => $id
        );

        do{
            sleep(3);

            $status = $this->ListInstances( $id )->{"Instances"}->{"Instance"};

        }while( (strcmp( $status[0]->{'Status'}, "Pending" ) == 0) );
        
        if( $allowPublicIP != "No") {
            $this->AllocatePublicIpAddress( $param );
        }
 
        $MESSAGE = $replica->job_msg('Starting instance.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $this->StartInstance( $param );
        
        $MESSAGE = $replica->job_msg('Recover Workload process completed.');
		$replica->update_job_msg( $servInfo["SERV_UUID"], $MESSAGE, 'Service');	

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "imageId" => $imageId
        );
        
        if( $serviceConfig["recovery_type"] != "PlannedMigration")
            $this->DeleteDiskI( $diskIds[0] );

        $this->DeleteImage( $param );

        $hostName = preg_replace( "/^[^a-zA-Z]+|[^a-zA-Z0-9_.-]/", "", $servInfo["HOST_NAME"] );

        $this->DeleteAutoSnapshotFromImage($servInfo, $region, $hostName.$servInfo["SERV_UUID"]);
        
        return $id;

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

            if( strpos( $hostInfo["os_name"], "64-bit" ) )
                $arch = 'x86_64';
            else if( strpos( $hostInfo["os_name"], "32-bit" ) )
                $arch = 'i386';
        }

        $hostInfo["guest_os_name"] = strtolower( $hostInfo["guest_os_name"] );

        if( isset( $hostInfo["guest_os_name"] ) ) {
            if( strpos( $hostInfo["guest_os_name"], "centos" ) !== false )
                $platfrom = 'CentOS';
            else if( strpos( $hostInfo["guest_os_name"], "red" ) !== false )
                $platfrom = 'RedHat';
            else if( strpos( $hostInfo["guest_os_name"], "ubuntu" ) !== false )
                $platfrom = 'Ubuntu';
            else if( strpos( $hostInfo["guest_os_name"], "suse" ) !== false )
                $platfrom = 'SUSE';
            //else if( strpos( $hostInfo["guest_os_name"], "SUSE openSUSE" ) !== false )
                //$platfrom = 'OpenSUSE';
            else if( strpos( $hostInfo["guest_os_name"], "debian" ) !== false )
                $platfrom = 'Debian';
            else if( strpos( $hostInfo["guest_os_name"], "coreos" ) !== false )
                $platfrom = 'CoreOS';
            else if( strpos( $hostInfo["guest_os_name"], "windows" ) !== false ){

                if( strpos( $hostInfo["guest_os_name"], "2016" ) !== false )
                    $platfrom = 'Windows Server 2016';
                else if( strpos( $hostInfo["guest_os_name"], "2012" ) !== false )
                    $platfrom = 'Windows Server 2012';
                else if( strpos( $hostInfo["guest_os_name"], "2008" ) !== false )
                    $platfrom = 'Windows Server 2008';
                else if( strpos( $hostInfo["guest_os_name"], "2003" ) !== false )
                    $platfrom = 'Windows Server 2003';
                else if( strpos( $hostInfo["guest_os_name"], "7" ) !== false )
                    $platfrom = 'Windows Server 2016';
                else
                    $platfrom = 'Windows Server 2012';
            }
            else if( strpos( $hostInfo["guest_os_name"], "other" ) !== false )
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
 * @brief  delete instance, it will hook until instance really terminated
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              not used
 * @param[in]   InstanceId          instance unique id
 * @return  none
 */

    public function terminate_instances($cloud_uuid, $region ,$InstanceId ) {

       $this->AuthAli( $cloud_uuid );

        $param = array(
            "instanceId" => $InstanceId
        );

        $this->StopInstance( $param );

        do{
            sleep(5);

            $Instances = $this->ListInstances( $InstanceId )->{"Instances"};

            if( count( $Instances->{"Instance"} ) == 0 )
                break;

        }while( strcmp( $Instances->{"Instance"}[0]->{'Status'}, "Stopped" ) != 0 );

        if( count( $Instances->{"Instance"} ) != 0 ) {	
            $this->DeleteInstance( $param );	
            sleep(5);
        }

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "imageId" => $Instances->{"Instance"}[0]->{"ImageId"}
        );
        
        $this->DeleteImage( $param );

    }

/**
 * @brief  delete snapshot, if there is any image create from the snapshot, it will be delete too.
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              region of snapshot and image
 * @param[in]   snapId              snapshot unique id
 * @return  none
 */

    public function delete_snapshot( $cloud_uuid, $region, $snapId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "snapshotId" => $snapId
        );
        
        $images = $this->GetImage( $param );

        foreach( $images->{"Images"}->{"Image"} as $image ) {
        
            $param = array(
                "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
                "imageId" => $image->{"ImageId"}
            );
            
            $this->DeleteImage( $param );
        }

        $this->DeleteSnapshotI( $snapId );
    }

/**
 * @brief  delete disk, if there is any snapshot create from the disk, it will be delete too.
 * @param[in]   cloud_uuid          _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   region              region of snapshot and image
 * @param[in]   diskId              disk unique id
 * @return  none
 */

    public function delete_volume( $cloud_uuid, $region, $diskId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "diskId" => $diskId
        );

        $snapshots = $this->GetSnapshotListI( $param );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "diskId" => json_encode( array($diskId) )
        );

        
        $diskInfo = $this->GetDiskInfo( $param );
        
        $d = $diskInfo->{"Disks"}->{"Disk"}[0];

        if( isset( $d->{"InstanceId"} ) ) {
            $param = array(
                "instanceId" => $d->{"InstanceId"},
                "diskId" => $diskId
            );

            $this->DettachDiskToInstance( $param );
        }

        foreach( $snapshots->{"Snapshots"}->{"Snapshot"} as $snap ) 
            $this->delete_snapshot( $cloud_uuid, $region, $snap->{"SnapshotId"} );

        sleep( 5 );

        $this->DeleteDiskI( $diskId );
    }

    public function create_Image( $cloud_uuid, $region, $diskId, $InstanceName, $time, $data ){

        $server     = new Server_Class();
        $service    = new Service_Class();
        $replica   = new Replica_Class();	

		$snap = $this->create_disk_snapshot( $cloud_uuid, $region, $diskId, $InstanceName, $time);
		
        $mesage = $replica->job_msg('Creating snapshot %1%',array($snap->{"SnapshotId"}));
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

		$snapInfo = $this->waitSnapFinish( $cloud_uuid, $region, $snap->{"SnapshotId"} );
        
        $mesage = $replica->job_msg('Snapshot created.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

        $mesage = $replica->job_msg('Starting system conversion process.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

		$param = array(
			"region" => $this->getCurrectRegion( substr($region, 0, -1) ),
			"zone" => $region,
			"size" => $snapInfo[0]["size"],
            "snapId" => $snap->{"SnapshotId"},
			"description" => "Disk Created By Saasame Transport Service@".$time
		);
		
        $mesage = $replica->job_msg('Creating temporary system disk from last snapshot.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

		$diskRet = $this->CreateDiskI( $param );

        $mesage = $replica->job_msg('Temporary system disk created.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

        $server_uuid = explode( '|', $data["SERVER_UUID"]);

        $param = array(
            "instanceId" => $server_uuid[0],
            "diskId" => $diskRet->{"DiskId"}
        );

        $AliModel = new Aliyun_Model();

        $replcaInfo = $AliModel->getReplicaInfo( $data["REPL_UUID"] );

		$JOB_INFO = json_decode( $replcaInfo[0]['_JOBS_JSON'], true );

        $JOB_INFO["ali_diskId"] = $diskRet->{"DiskId"};

        $JOB_JSON = json_encode($JOB_INFO, JSON_UNESCAPED_SLASHES);

		$AliModel->updateReplicaInfo( $JOB_JSON, $data["REPL_UUID"] );
        
        $FI_SCSI_ADDR = $service->list_disk_addr_info($data["HOST_UUID"],$data["LAUNCHER_ADDR"],"Loader","MS","Physical");						

        $mesage = $replica->job_msg('Attached temporary system disk to Transport Server.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

        $ret = $this->AttachDiskToInstance( $param );

        $SCSI_ADDR = $service->CompareEnumerateDisks($data["HOST_UUID"],$data["LAUNCHER_ADDR"],"Launcher","MS","Physical",$FI_SCSI_ADDR);

        $mesage = $replica->job_msg('New volume address is %1%.',array($SCSI_ADDR));
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');
        
        $IMAGE_UUID  = Misc_Class::guid_v4();
        
        $param = array(
            "LUN_MAPS" => array( $IMAGE_UUID => $SCSI_ADDR )
        );

        $json = array(
            "replica_uuid" => $data["REPL_UUID"],
            "image_uuid" => $IMAGE_UUID,
            "disk_id" => $diskRet->{"DiskId"},
            "json" => json_encode( $param, JSON_UNESCAPED_SLASHES )
        );

        $AliModel->insertAliImage( $json );

        $mesage = $replica->job_msg('The volume is ready for conversion.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

        $mesage = $replica->job_msg('Conversion process submitted.');
		$replica->update_job_msg( $data["REPL_UUID"] , $mesage, 'Replica');

        $service->create_launcher_job( $data["REPL_UUID"], $data["CONN_UUID"], $IMAGE_UUID);
    }

    public function describe_images( $cloud_uuid, $region, $repl_uuid ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) )
        );

        $images = $this->GetImage( $param );

        $ret = array();

        foreach( $images->{"Images"}->{"Image"} as $key => $image ) {

            if( strpos( $image->{"Description"}, $repl_uuid ) === 0 )
                array_push( $ret, $image );

        }

        return $ret;
        
    }

    public function describe_image_detail( $cloud_uuid, $region, $imageId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "imageId" => $imageId
        );

        $images = $this->GetImage( $param );

        if( isset($images->{"Images"}->{"Image"}[0]) )
            return $images->{"Images"}->{"Image"}[0];

        return null;
        
    }

    public function create_instance_form_image( $cloud_uuid, $region, $param ) {

        $server     = new Server_Class();
        $service    = new Service_Class();
        $replica    = new Replica_Class();
        $AliModel   = new Aliyun_Model();

        $this->LogJobId = $param["service_uuid"];

        $MESSAGE = $replica->job_msg('Recover Workload process started.');
		$replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');

        $this->AuthAli( $cloud_uuid );

        $json = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone" => $region,
            "imageId" => $param["imageId"],
            "instanceType" => $param['typeId'],
            "securityGroup" => $param['sgId'],
            "switch" => $param['switchId'],
            "bandwidthOut" => 100,
            "bandwidthIn" => 100
        );
        
        $MESSAGE = $replica->job_msg('Creating a new instance from custom image %1%.',array($param["imageId"]));
		$replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');
        
        $id = $this->CreateInstanceI( $json )->{"InstanceId"};

        $MESSAGE = $replica->job_msg('Instance %1% created.',array($id));
		$replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');

        $in = array(
            "instanceId" => $id
        );

        $condition = array(
            "_SERV_UUID" => $param["service_uuid"]
        );

        $AliModel->updateInstanceIdInService( $in, $condition);

        do{
            sleep(3);

            $status = $this->ListInstances( $id )->{"Instances"}->{"Instance"};

        }while( (strcmp( $status[0]->{'Status'}, "Pending" ) == 0) );

        $snaps = explode( ',', $param['snapIds'] );
        
        array_shift($snaps);

        $now = Misc_Class::current_utc_time();	

        foreach( $snaps as $snap ) {

            $data = array(
                "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
                "snapshotId" => json_encode( array( $snap ) )
            );

            $snapshots = $this->GetSnapshotListI( $data );

            $s = $snapshots->{"Snapshots"}->{"Snapshot"}[0];

            $data = array(
                "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
                "zone" => $region,
                "size" => $s->{"SourceDiskSize"},
                "description" => "Disk Created By Saasame Transport Service@".$now
            );

            $diskRet = $this->CreateDiskI( $data );

            $MESSAGE = $replica->job_msg('Creating disk %1%.',array($diskRet->{"DiskId"}));
		    $replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');

            $data = array(
                "instanceId" => $id,
                "diskId" => $diskRet->{"DiskId"}
            );
            
            $ret = $this->AttachDiskToInstance( $data );

            $MESSAGE = $replica->job_msg('Attaching disk %1% to instance.',array($diskRet->{"DiskId"}));
		    $replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');
            
        }

        do{
            sleep(3);

            $status = $this->ListInstances( $id )->{"Instances"}->{"Instance"};

        }while( (count( $status[0]->{'OperationLocks'}->{"LockReason"} ) != 0) );

        $data = array(
            "instanceId" => $id
        );

        $MESSAGE = $replica->job_msg('Starting instance.');
		$replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');

        $this->StartInstance( $data );

        $MESSAGE = $replica->job_msg('Recover Workload process completed.');
		$replica->update_job_msg( $param["service_uuid"], $MESSAGE, 'Service');

    }

    public function DeleteAutoSnapshotFromImage($cloud_uuid, $region, $imageName){

        $param = array(
            "snapshotName" => "SnapshotForImage-".$imageName,
            "region" => $this->getCurrectRegion( substr($region, 0, -1) )
        );
        
        $snapshotInfo = $this->GetSnapshotListI( $param );
        
        $this->DeleteSnapshotI( $snapshotInfo->Snapshots->Snapshot[0]->SnapshotId );
    }

 /**
 *  create a OssClient
 *
 * @param string $cloud_uuid 
 * @param string $endpoint example "http://oss-cn-hangzhou.aliyuncs.com"
 * @return object OssClient
 */
    public function CreateOssClient($cloud_uuid, $endpoint)
    {
        $AliModel = new Aliyun_Model();

        $cloud_info = $AliModel->query_cloud_connection_information( $cloud_uuid );

        try {
            $this->ossClient = new OSS\OssClient($cloud_info['ACCESS_KEY'], $cloud_info['SECRET_KEY'], $endpoint);
        } catch (OssException $e) {
            return false;
        }
        if ($this->ossClient === true) {
            return true;
        } 
        else {
            return false;
        }
    }

    /**
    * 创建一存储空间
    * acl 指的是bucket的访问控制权限，有三种，私有读写，公共读私有写，公共读写。
    * 私有读写就是只有bucket的拥有者或授权用户才有权限操作
    * 三种权限分别对应OSSClient::OSS_ACL_TYPE_PRIVATE，
    *               OssClient::OSS_ACL_TYPE_PUBLIC_READ,
    *               OssClient::OSS_ACL_TYPE_PUBLIC_READ_WRITE
    *
    * @param OssClient $ossClient OSSClient实例
    * @param string    $bucket 要创建的bucket名字
    * @param type      $type privilige type
    * @return true if created success
    */

    public function createBucket($ossClient, $bucket, $type )
    {
        try {
            $this->ossClient->createBucket($bucket, $type);
        } catch (OssException $e) {
            printf(__FUNCTION__ . ": FAILED\n");
            printf($e->getMessage() . "\n");
            return false;
        }
        
        return true;
    }

 /**
 *  判断Bucket是否存在
 *
 * @param OssClient $ossClient OssClient实例
 * @param string $bucket 存储空间名称
 * @return bool if Exist=>true, else=>false
 */
    public function doesBucketExist($ossClient, $bucket)
    {
        try {
            $res = $ossClient->doesBucketExist($bucket);
        } catch (OssException $e) {
            printf(__FUNCTION__ . ": FAILED\n");
            printf($e->getMessage() . "\n");
            return;
        }
        if ($res === true) {
            return true;
        } else {
            return false;
        }
    }

    public function VerifyOss( $cloud_uuid, $region , $bucket, $isInternal ) {

        $ret = true;

        if( $isInternal )
            $endpoint = "http://oss-".$this->getCurrectRegion( substr($region, 0, -1) )."-internal.aliyuncs.com";
        else
            $endpoint = "http://oss-".$this->getCurrectRegion( substr($region, 0, -1) ).".aliyuncs.com";

        $this->CreateOssClient($cloud_uuid, $endpoint);

        try {
             if( !$this->ossClient->doesBucketExist($bucket) )
                $ret = $this->ossClient->createBucket( $bucket, OSS\OssClient::OSS_ACL_TYPE_PRIVATE);    
        }
        catch (Exception $e) {
            return $e->getMessage();
        }

        return true;
    }

    function deleteObject( $bucket, $object )
    {
        //$object = "oss-php-sdk-test/upload-test-object-name.txt";

        try{
            $this->ossClient->deleteObject($bucket, $object);
        } catch(OssException $e) {
            printf(__FUNCTION__ . ": FAILED\n");
            printf($e->getMessage() . "\n");
            return;
        }
        print(__FUNCTION__ . ": OK" . "\n");
    }

    public function DeleteObjectFromOSS( $cloud_uuid, $region , $bucket, $object, $isInternal ) {

        $ret = true;

        if( $isInternal )
            $endpoint = "http://oss-".$this->getCurrectRegion( substr($region, 0, -1) )."-internal.aliyuncs.com";
        else
            $endpoint = "http://oss-".$this->getCurrectRegion( substr($region, 0, -1) ).".aliyuncs.com";

        $this->CreateOssClient($cloud_uuid, $endpoint);

        try {
             $this->ossClient->deleteObject( $bucket, $object );
        }
        catch (Exception $e) {
            echo $e;
        }

        return $ret;
    }

    public function ImportImageFromOss( $cloud_uuid, $region , $bucket, $object, $service_satus ) {

        $this->AuthAli( $cloud_uuid );

        $osInfo = array( "guest_os_name" => $service_satus->platform .' '. ((strpos( $service_satus->architecture, "amd64" ) !== false)?"64-bit":"32-bit"));

        $os = $this->enumOS( json_encode( $osInfo ) );

        $arch = $os["arch"];

        $platfrom = $os["platfrom"];

        if( strpos( $platfrom, "Windows" ) === false )
            $osType = 'Linux';
        else
            $osType = 'Windows';

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            //"imageName" => "saasame-".$service_satus->id,
            "imageName" => $object,
            "format" => "VHD",
            "imSize" => $service_satus->size,
            "imageSize" => $service_satus->size,
            "platform" => $platfrom,
            "oSType" => $osType,
            "architecture" => $arch,
            "bucket" => $bucket,
            "object" => $object
        );

        $ret = $this->ImportImage( $param );

        return $ret;
    }

    public function describe_task_detail( $cloud_uuid, $region, $taskId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region" => $this->getCurrectRegion( substr($region, 0, -1) ),
            "taskId" => $taskId
        );

        $task = $this->getTaskAttribute( $param );

        return $task;
        
    }

/**
* Role Permission Control
*/
    public function GetLoginProfile() {
    
        $request = new Ram\GetLoginProfileRequest();
        
        $request->setUserName("rex@saasame.com");

        $response = $this->SendRequest( $request , "GetLoginProfileRequest" );
    
        return $response;
    }

    public function CreateRoleRequest( $param ) {

        $request = new Ram\CreateRoleRequest();

        if( isset( $param["roleName"] ) )
            $request->setRoleName( $param["roleName"] );
        
        if( isset( $param["description"] ) )
            $request->setDescription( $param["description"] );

        if( isset( $param["policy"] ) )
            $request->setAssumeRolePolicyDocument( $param["policy"] );

        $response = $this->SendRequest( $request , "CreatePolicyRequest" );

        return $response;
    }

    public function AttachPolicyToRoleRequest( $param ) {

        $request = new Ram\AttachPolicyToRoleRequest();

        if( isset( $param["policyType"] ) )
            $request->setPolicyType( $param["policyType"] );
        
        if( isset( $param["policyName"] ) )
            $request->setPolicyName( $param["policyName"] );

        if( isset( $param["roleName"] ) )
            $request->setRoleName( $param["roleName"] );

        $response = $this->SendRequest( $request , "CreatePolicyRequest" );

        return $response;
    }

    public function SetImageExportRoleforOss( $cloud_uuid ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "roleName" => "AliyunECSImageExportDefaultRole",
            "description" => "The ECS service will use this role to export image file.",
            "policy" => '{
                "Statement": [
                    {
                        "Action": "sts:AssumeRole",
                        "Effect": "Allow",
                        "Principal": {
                            "Service": [
                                "ecs.aliyuncs.com"
                            ]
                        }
                    }
                ],
                "Version": "1"
              }'
        );

        $this->CreateRoleRequest( $param );

        $param = array(
            "policyType" => "System",
            "policyName" => "AliyunECSImageExportRolePolicy",
            "roleName" => "AliyunECSImageExportDefaultRole"
        );

        $this->AttachPolicyToRoleRequest( $param );

    }

    public function SetImageImportRoleforOss( $cloud_uuid ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "roleName" => "AliyunECSImageImportDefaultRole",
            "description" => "The ECS service will use this role to import image file.",
            "policy" => '{
                "Statement": [
                    {
                        "Action": "sts:AssumeRole",
                        "Effect": "Allow",
                        "Principal": {
                            "Service": [
                                "ecs.aliyuncs.com"
                            ]
                        }
                    }
                ],
                "Version": "1"
            }'
        );

        $this->CreateRoleRequest( $param );

        $param = array(
            "policyType" => "System",
            "policyName" => "AliyunECSImageImportRolePolicy",
            "roleName" => "AliyunECSImageImportDefaultRole"
        );

        $this->AttachPolicyToRoleRequest( $param );
    }

    public function ListRoles() {

        $request = new Ram\ListRolesRequest();

        $response = $this->SendRequest( $request , "ListRolesRequest" );

        return $response;
    }

    public function ListPoliciesForRole( $param ) {
        
        $request = new Ram\ListPoliciesForRoleRequest();

        if( $param["roleName"] )
            $request->setRoleName( $param["roleName"] );

        $response = $this->SendRequest( $request , "ListPoliciesForRoleRequest" );

        return $response;
    }

    public function CheckOssPermission( $cloud_uuid ) {

        $ret = array();

        $this->AuthAli( $cloud_uuid );

        $roles = $this->ListRoles();

        if( $roles === false )
            return false;

        foreach( $roles->{"Roles"}->{"Role"} as $role ) {
            
            if( $role->{"RoleName"} == "AliyunECSImageImportDefaultRole" ) {

                $param = array(
                    "roleName" => $role->{"RoleName"}
                );

                $policies = $this->ListPoliciesForRole( $param );

                foreach( $policies->{"Policies"}->{"Policy"} as $policy ) {
                    if( $policy->{"PolicyName"} == "AliyunECSImageImportRolePolicy" )
                        $ret["import"] = true;
                }
            }
            else if( $role->{"RoleName"} == "AliyunECSImageExportDefaultRole" ) {
                
                $param = array(
                    "roleName" => $role->{"RoleName"}
                );

                $policies = $this->ListPoliciesForRole( $param );

                foreach( $policies->{"Policies"}->{"Policy"} as $policy ) {
                    if( $policy->{"PolicyName"} == "AliyunECSImageExportRolePolicy" )
                        $ret["export"] = true;
                }
            }
        }

        if( !isset( $ret["import"] ) ) {
            $this->SetImageImportRoleforOss( $cloud_uuid );
        }

        if( !isset( $ret["export"] ) ) {
            $this->SetImageExportRoleforOss( $cloud_uuid );
        }

        return $ret;
    }

    public function getCurrectRegion( $region ) {

        if( $region[0] == 'c' && $region[1] == 'n' ) {
            $r = explode( '-', $region );
            return $r[0].'-'.$r[1];
        }

        return $region;

    }

    public function endpointCheck( $region ) {

        $endpoint = "vpc100-oss-".$this->getCurrectRegion( substr($region, 0, -1) ).".aliyuncs.com";

        if( gethostbyname( $endpoint ) != $endpoint ) {
            return "vpc100-oss-".$this->getCurrectRegion( substr($region, 0, -1) );
        }

        $endpoint = "oss-".$this->getCurrectRegion( substr($region, 0, -1) )."-internal.aliyuncs.com";

        if( gethostbyname( $endpoint ) != $endpoint ) {
            return "oss-".$this->getCurrectRegion( substr($region, 0, -1) )."-internal";
        }
        
        return "oss-".$this->getCurrectRegion( substr($region, 0, -1) );
    }

    public function getInstanceInfoForReport( $param ) {

        $this->AuthAli( $param["cloudId"] );

        $this->setRegionId( $this->getCurrectRegion( $param["region"] ) );

        $ret = $this->ListInstances( $param["instanceId"] );

        if( count( $ret->Instances->Instance ) == 0 )
            return false;
            
        $instanceStatus = $ret->Instances->Instance[0];
        
        $instanceTypeDetail = $this->describe_instance_types( $param["cloudId"], $this->getCurrectRegion( $param["region"] ), $instanceStatus->InstanceType );

        $report = array(
            "Instance Name" => $instanceStatus->InstanceName,
            "Flavor" => $instanceStatus->InstanceType,
            "Security Group" => $instanceStatus->SecurityGroupIds->SecurityGroupId[0],
            "CPU Core" => $instanceTypeDetail->CpuCoreCount,
            "GPU" => (empty( $instanceTypeDetail->GPUSpec ))?0:$instanceTypeDetail->GPUSpec,
            "Memory" => $instanceTypeDetail->MemorySize * 1024
        );

        $report["NetworkInterfaces"] = array();

        $report["Public IP address"] = array();

        foreach( $instanceStatus->NetworkInterfaces->NetworkInterface as $NetworkInterface ) {

            $network = array(
                "MacAddress" => $NetworkInterface->MacAddress,
                "Private IP address" => $NetworkInterface->PrimaryIpAddress,
                "NetworkInterfaceId" => $NetworkInterface->NetworkInterfaceId
            );

            array_push( $report["NetworkInterfaces"], $network );
        }

        foreach( $instanceStatus->PublicIpAddress->IpAddress as $publicIP ) {

            array_push( $report["Public IP address"], $publicIP );

        }

        return $report;
    }

    public function DescribeSystemDiskInfoFromInstance( $cloud_uuid, $region, $instanceId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region"        => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone"          => $region,
            "instanceId"    => $instanceId,
            "diskType"      => "system"
        );

        $disksInfo = $this->GetDiskInfo( $param );

        $disks = array();

        foreach( $disksInfo->Disks->Disk as $disk ) {

            $temp = array(
                "diskId" => $disk->DiskId,
                "diskName" => $disk->DiskName,
                "diskType" => "system",
                "size" => $disk->Size * 1024 * 1024
            );

            array_push( $disks, $temp );
        }

        return $disks;
    }

    public function describe_volume( $cloud_uuid, $region, $diskId ) {

        $this->AuthAli( $cloud_uuid );

        $param = array(
            "region"        => $this->getCurrectRegion( substr($region, 0, -1) ),
            "zone"          => $region,
            "diskId"        => json_encode( array($diskId) )
        );

        $disksInfo = $this->GetDiskInfo( $param );

        $disks = array();

        foreach( $disksInfo->Disks->Disk as $disk ) {

            $temp = array(
                "diskId" => $disk->DiskId,
                "diskName" => $disk->DiskName,
                "size" => $disk->Size 
            );

            array_push( $disks, $temp );
        }

        return $disks;
    }

    public function listDataModeInstance( $cloudId, $serverId ){

        $vm_list = $this->describe_all_instances( $cloudId );

        $db = new Common_Model();

        $serverInfo = explode('|', $db->getTransportInfo( $serverId )["serverInfo"] );

        $vms = array();

        foreach( $vm_list as $vm ){
            if( $vm["Zone"] == $serverInfo[1] && 
                $vm["InstanceId"] != $serverInfo[0] )
                array_push( $vms, $vm);
        }
        return $vms;
    }
};

?>