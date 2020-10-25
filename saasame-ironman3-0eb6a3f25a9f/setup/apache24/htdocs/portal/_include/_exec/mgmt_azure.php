<?php
set_time_limit(0);
session_start();
require_once '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA,$TIME_OUT = 0)
{
	$URL = 'http://127.0.0.1:8080/restful/AzureWebServices';
	
	$REST_DATA['EncryptKey'] = Misc::encrypt_decrypt('encrypt',time());
	$ENCODE_DATA = json_encode($REST_DATA);
	
	$ch = curl_init($URL);
	curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt($ch, CURLOPT_HEADER, false);
	curl_setopt($ch, CURLOPT_POSTFIELDS, $ENCODE_DATA);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json','Content-Length: ' . strlen($ENCODE_DATA)));
	curl_setopt($ch, CURLOPT_TIMEOUT_MS, $TIME_OUT);
	$output = curl_exec($ch);	

	if (is_string($output) && is_array(json_decode($output, true)) == TRUE)
	{
		$output = json_decode($output);
		if (isset($output -> Msg) AND !is_object($output -> Msg))
		{
			$output -> Msg = _($output -> Msg);
		}	
		return json_encode($output);
	}
	else
	{
		return $output;
	}
	#return json_decode($output,true);
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# Verify Azure Connection
	############################
	case "VerifyAzureAccessCredential":
	
	if ($_REQUEST['AZ_ACCESS_KEY'] != '' and $_REQUEST['AZ_SECRET_KEY'] != '' )
	{
		$REST_DATA = array(
						'Action'	=> 'VerifyAzureAccessCredential',
						//'UserName' 		=> $_REQUEST['AZ_USER_NAME'],
						//'Password' 		=> $_REQUEST['AZ_PASSWORD'],
						'AccessKey' 	=> $_REQUEST['AZ_ACCESS_KEY'],
						'SecretKey' 	=> $_REQUEST['AZ_SECRET_KEY'],
						'TenantId' 		=> $_REQUEST['AZ_TENANT_ID'],
						'Endpoint'		=> $_REQUEST['AZ_ENDPOINT']
						//SubscriptionId'=> $_REQUEST['AZ_SUBSCRIPTION_ID'],
								
					);
		
		$REST_DATA["ConnectionString"] = (isset( $_REQUEST["AZ_STORAGE_CONNCTION_STRING"]))?$_REQUEST["AZ_STORAGE_CONNCTION_STRING"]:"";
		
		$output = CurlPostToRemote($REST_DATA);
	
		if (json_decode($output) == false)
		{
			$response = array('msg' => _('Cloud connection could not be verified.<br>Please check that the information provided is correct.'),'status' => false);
		}
		else
		{
			$response = array('msg' => $output,'status' => true);
		}
	}
	else
	{
		$response = array('msg' => _('Please fill in all of the required fields.'),'status' => false);
	}
	
	print_r(json_encode($response));
	break;
			
	############################
	# Initialize New Azure Connection
	############################
	case "InitializeNewAzureConnection":
	
	$REST_DATA = array(
					'Action'			=> 'InitializeNewAzureConnection',
					'AcctUUID'  		=> $_REQUEST['ACCT_UUID'],
					'RegnUUID'  		=> $_REQUEST['REGN_UUID'],
					//'UserName' 			=> $_REQUEST['AZ_USER_NAME'],
					//'Password' 			=> $_REQUEST['AZ_PASSWORD'],
					'AccessKey' 		=> $_REQUEST['AZ_ACCESS_KEY'],
					'SecretKey' 		=> $_REQUEST['AZ_SECRET_KEY'],					
					'TenantId' 			=> $_REQUEST['AZ_TENANT_ID'],
					'SubscriptionId' 	=> $_REQUEST['AZ_SUBSCRIPTION_ID'],
					'Endpoint' 			=> $_REQUEST['AZ_ENDPOINT']
				);
				
	$REST_DATA["ConnectionString"] = (isset( $_REQUEST["AZ_STORAGE_CONNCTION_STRING"]))?$_REQUEST["AZ_STORAGE_CONNCTION_STRING"]:"";
	
	$REST_DATA["SubscriptionName"] = (isset( $_REQUEST["AZ_SUBSCRIPTION_NAME"]))?$_REQUEST["AZ_SUBSCRIPTION_NAME"]:"";

	$REST_DATA["onpremisesTransport"] = (isset( $_REQUEST["AZ_ON_PREMISES_TRANSPORT"]))?$_REQUEST["AZ_ON_PREMISES_TRANSPORT"]:"";
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	
	############################
	# Query Access Credential Information
	############################
	case "QueryAccessCredentialInformation":
		$ACTION    = 'QueryAccessCredentialInformation';
		$REST_DATA = array(
					'Action' 			=> $ACTION,
					'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
				);
		$output = CurlPostToRemote($REST_DATA);
	
		echo $output;	
	break;
	
		
	############################
	# Update Azure Connection
	############################
	case "UpdateAzureConnection":
	
	$REST_DATA = array(
					'Action'			=> 'UpdateAzureConnection',
					'ClusterUUID'  		=> $_REQUEST['CLUSTER_UUID'],					
					'AccessKey' 		=> $_REQUEST['AZ_ACCESS_KEY'],
					'SecretKey' 		=> $_REQUEST['AZ_SECRET_KEY'],					
					'TenantId' 			=> $_REQUEST['AZ_TENANT_ID'],
					'SubscriptionId' 	=> $_REQUEST['AZ_SUBSCRIPTION_ID'],
					'Endpoint' 			=> $_REQUEST['AZ_ENDPOINT']				
				);
				
	$REST_DATA["ConnectionString"] = (isset( $_REQUEST["AZ_STORAGE_CONNCTION_STRING"]))?$_REQUEST["AZ_STORAGE_CONNCTION_STRING"]:"";
	
	$REST_DATA["SubscriptionName"] = (isset( $_REQUEST["AZ_SUBSCRIPTION_NAME"]))?$_REQUEST["AZ_SUBSCRIPTION_NAME"]:"";
	
	$REST_DATA["onpremisesTransport"] = (isset( $_REQUEST["AZ_ON_PREMISES_TRANSPORT"]))?$_REQUEST["AZ_ON_PREMISES_TRANSPORT"]:"";
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# List Installed Instances
	############################
	case "ListInstalledInstances":
	$ACTION    = 'ListInstalledInstances';
	$REST_DATA = array(
					'Action' 			=> $ACTION,
					'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;
	break;
	
	case "ListInstancesInResourceGroup":
	$ACTION    = 'ListInstancesInResourceGroup';
	$REST_DATA = array(
					'Action' 			=> $ACTION,
					'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID'],
					'ServerId'			=> $_REQUEST['ServerId'],
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;
	break;
	
	############################
	# Query Selected Host Information
	############################
	case "QuerySelectedHostInformation":
	$ACTION    = 'QuerySelectedHostInformation';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'HostName'		=> $_REQUEST['HOST_NAME'],
					'HostREGN'		=> $_REQUEST['HOST_REGN'],
					'HostRG'		=> $_REQUEST['HOST_RG']
				);
				
	if( isset($_REQUEST['SERV_UUID']) )
		$REST_DATA['SERV_UUID']	= $_REQUEST['SERV_UUID'];
				
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	############################
	# Query Transport Information
	############################	
	case "QueryTransportInformation":
	$ACTION    = 'QueryTransportInformation';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'VMName'		=> $_REQUEST['VM_NAME'],
					'VMREGN'		=> substr($_REQUEST['VM_REGN'],0,-1)
				);
				
	if( isset($_REQUEST['SERV_UUID']) )
		$REST_DATA['SERV_UUID']	= $_REQUEST['SERV_UUID'];
				
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# List Available Ebs Snapshot
	############################
	case "ListAvailableDiskSnapshot":
	$ACTION    = 'ListAvailableDiskSnapshot';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'ServZone'		=> $_REQUEST['SERVER_ZONE'],
					'DiskID'		=> $_REQUEST['DISK_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	if ($output != 'false')
	{
		#RE-FORMAT SNAPSHOT ARRAY LIST
		//$snapshots = array_reverse(json_decode($output, true));
		$snapshots = json_decode($output, true);
		for ($i=0; $i<count($snapshots); $i++)
		{
			#SET DEFAULT TAG VALUES
			$description = 'No description';
			$name = 'No Name';
	
			if( isset($snapshots[$i]['tags']) ) {
				#BEGIN TO GET TAG VALUES
				$snapshot_tags = $snapshots[$i]['tags'];

				$description = Misc::convert_snapshot_time_with_zone($snapshot_tags['Description'], $snapshots[$i]['properties']['timeCreated']);					
			}
				
			$name = $snapshots[$i]['name'];
			
			$FormatOutput[] = array(
								'status' 		=> $snapshots[$i]['properties']['provisioningState'],								
								'name'			=> $name,
								//'volume_id'		=> $snapshots[$i] -> VolumeId,
								//'created_at'	=> $snapshots[$i] -> StartTime,
								//'size'			=> $snapshots[$i] -> VolumeSize,								
								//'progress'		=> $snapshots[$i] -> Progress,
								'id'			=> $snapshots[$i]['id'],
								'description'	=> $description
							);
		}
		
		$output = json_encode($FormatOutput, JSON_UNESCAPED_SLASHES);
	}
	
	print_r($output);
	
	break;
	
	############################
	# List Instance Flavors
	############################
	case "ListInstanceFlavors":
	$ACTION    = 'ListInstanceFlavors';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'SERV_REGN'		=> $_REQUEST['SERV_REGN'],
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# List Available Network
	############################
	case "ListAvailableNetwork":
	$ACTION    = 'ListAvailableNetwork';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# List Security Group
	############################
	case "ListSecurityGroup":
	$ACTION    = 'ListSecurityGroup';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'ServerZONE'	=> $_REQUEST['SERVER_ZONE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	############################
	# Query Flavor Information
	############################
	case "QueryFlavorInformation":
	$ACTION    = 'QueryFlavorInformation';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SELECT_ZONE'	=> $_REQUEST['SELECT_ZONE'],
					'FlavorId'		=> $_REQUEST['FLAVOR_ID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# Query Network Information
	############################
	case "QueryNetworkInformation":
	$ACTION    = 'QueryNetworkInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'NetworkUUID'	=> $_REQUEST['NETWORK_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# Query Security Group Information
	############################
	case "QuerySecurityGroupInformation":
	$ACTION    = 'QuerySecurityGroupInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'SGroupUUID'	=> $_REQUEST['SGROUP_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	############################
	# Query Snapshot Information
	############################
	case "QuerySnapshotInformation":
	$ACTION    = 'QuerySnapshotInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'SnapshotID'	=> $_REQUEST['SNAPSHOT_ID'],
					'ReplicaUUID'	=> $_REQUEST['REPLICA_UUID'],
					'ReplicaDisk'	=> $_REQUEST['REPLICA_DISK'],
					'BlobMode'		=> $_REQUEST['BLOB_MODE']
				);
	$output = CurlPostToRemote($REST_DATA);
	if ($output != FALSE)
	{
		#RE-FORMAT SNAPSHOT ARRAY LIST
		$snapshots = json_decode($output, true);
	
		$description = 'No description';
	
		foreach( $snapshots as $snapshot) {
			
			if( isset( $snapshot['tags'] ) )
			{
				foreach( $snapshot['tags'] as $key => $tag) {
					if( $key == 'Description' )
						$description = Misc::convert_snapshot_time_with_zone($tag, $snapshot['properties']['timeCreated']);
					
				}
			}
					
			$FormatOutput[] = array(
				'created_at'	=> $snapshot['properties']['timeCreated'],
				'size'			=> $snapshot['properties']['diskSizeGB'],								
				'description'	=> $description
			);
		}
		$output = json_encode($FormatOutput);
	}	
	print_r($output);
	break;
	
	case 'QueryEndpoint':
		
		$ACTION    = 'QueryEndpoint';
		
		$REST_DATA = array(
			'Action' 	=> $ACTION
		);
		
		$output = CurlPostToRemote($REST_DATA);
		
		print_r( $output);
	break;
	
	case 'CheckPrivateIp':
		
		$ACTION    = 'CheckPrivateIp';
		
		$REST_DATA = array(
			'Action' 		=> $ACTION,
			'CLUSTER_UUID' 	=> $_REQUEST["CLUSTER_UUID"],
			'NETWORK_UUID' 	=> $_REQUEST["NETWORK_UUID"],
			'PRIVATE_IP'   	=> $_REQUEST["PRIVATE_IP"]
		);
		
		$output = CurlPostToRemote($REST_DATA);
		
		print_r( $output);
	break;
	
	case 'ListAzureBlobSnapshot':
		
		$ACTION    = 'ListAzureBlobSnapshot';
		
		$REST_DATA = array(
			'Action' 		=> $ACTION,
			'CLUSTER_UUID' 	=> $_REQUEST["CLUSTER_UUID"],
			'CONTAINER' 	=> $_REQUEST["CONTAINER"],
			'DISK_ID'   	=> $_REQUEST["DISK_ID"]
		);
		
		$output = CurlPostToRemote($REST_DATA);
		$snapshots = json_decode($output, true);
		
		foreach( $snapshots as $snapshot) {
			
			$description = Misc::convert_snapshot_time_with_zone($snapshot['description'],$snapshot['datetime']);
			
			$FormatOutput[] = array(
				'id'			=> $snapshot['id'],
				'description'	=> $description
			);
		}
		$output = json_encode($FormatOutput);
				
		print_r( $output);
	break;
	
	case 'setPublicIpStatic':
		
		$ACTION    = 'setPublicIpStatic';
		
		$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'HostName'		=> $_REQUEST['HOST_NAME'],
					'HostREGN'		=> $_REQUEST['HOST_REGN'],
					'HostRG'		=> $_REQUEST['HOST_RG']
				);
				
		if( isset($_REQUEST['SERV_UUID']) )
			$REST_DATA['SERV_UUID']	= $_REQUEST['SERV_UUID'];

		$output = CurlPostToRemote($REST_DATA);
		
		echo $output;
		
	break;
	
	case 'ListAvailabilitySet':
		
		$ACTION    = 'ListAvailabilitySet';
		
		$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'RESOURCE_GROUP'=> $_REQUEST['RESOURCE_GROUP'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE']
				);
				
		$output = CurlPostToRemote($REST_DATA);
		
		echo $output;
		
	break;
	
	############################
	# QUERY VOLUME INFORMATION
	############################
	case "QueryVolumeDetailInfo":
	
	$ACTION    = 'QueryVolumeDetailInfo';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'DiskUUID' 	 => $_REQUEST['DISK_UUID'],
					   'ServerId'	 => $_REQUEST['ServerId']
				);	
	
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	
	break;
	
	case "ListDataModeInstances":
	$ACTION    = 'ListDataModeInstances';
	$REST_DATA = array(
					'Action' 			=> $ACTION,
					'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID'],
					'FilterUUID'		=> $_REQUEST['FILTER_UUID']
				);

	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;
	break;
	
	default:
	break;
}