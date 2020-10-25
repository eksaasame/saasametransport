<?php
set_time_limit(0);
session_start();
include '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA)
{
	$URL = 'http://127.0.0.1:8080/restful/TencentWebServices';
	
	$REST_DATA['EncryptKey'] = Misc::encrypt_decrypt('encrypt',time());
	$ENCODE_DATA = json_encode($REST_DATA);
	
	$ch = curl_init($URL);
	curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt($ch, CURLOPT_HEADER, false);
	curl_setopt($ch, CURLOPT_POSTFIELDS, $ENCODE_DATA);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);	
	curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json','Content-Length: ' . strlen($ENCODE_DATA)));
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
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# Verify Aws Connection
	############################
	case "VerifyTencentAccessCredential":
	
	if ($_REQUEST['ACCESS_KEY'] != '' and $_REQUEST['SECRET_KEY'] != '')
	{
		$REST_DATA = array(
						'Action'	=> 'VerifyTencentAccessCredential',
						'AccessKey' => $_REQUEST['ACCESS_KEY'],
						'SecretKey' => $_REQUEST['SECRET_KEY'],
						'AppId' 	=> $_REQUEST['APP_ID']						
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$output = json_encode('Please fill in all of the required fields.');
	}
	print_r($output);
	break;
			
	############################
	# Initialize New Aws Connection
	############################
	case "InitializeNewTencentConnection":
	
	$REST_DATA = array(
					'Action'	=> 'InitializeNewTencentConnection',
					'AcctUUID'  => $_REQUEST['ACCT_UUID'],
					'RegnUUID'  => $_REQUEST['REGN_UUID'],					
					'AccessKey' => $_REQUEST['ACCESS_KEY'],
					'SecretKey' => $_REQUEST['SECRET_KEY'],
					'AppId' 	=> $_REQUEST['APP_ID']					
				);
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
	# Update Tencent Connection
	############################
	case "UpdateTencentConnection":
	
	$REST_DATA = array(
					'Action'		=> 'UpdateTencentConnection',
					'ClusterUUID'  	=> $_REQUEST['CLUSTER_UUID'],					
					'AccessKey' 	=> $_REQUEST['ACCESS_KEY'],
					'SecretKey'		=> $_REQUEST['SECRET_KEY'],
					'AppId'			=> $_REQUEST['APP_ID']					
				);
				
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
	
	
	############################
	# Query Selected Host Information
	############################
	case "QuerySelectedHostInformation":
	$ACTION    = 'QuerySelectedHostInformation';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'HostUUID'		=> $_REQUEST['HOST_ID'],
					'HostREGN'		=> substr($_REQUEST['HOST_REGN'],0,-1)
				);
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
					'DiskID'		=> $_REQUEST['DISK_UUID'],
					'TimeZone'		=> $_SESSION['timezone']
				);

	$output = CurlPostToRemote($REST_DATA);
	
	if ($output != 'false')
	{
		#RE-FORMAT SNAPSHOT ARRAY LIST
		$snapshots = array_reverse(json_decode($output));
		for ($i=0; $i<count($snapshots); $i++)
		{
			#SET DEFAULT TAG VALUES
			$description = 'No description';
			$name = 'No Name';
			
			$description = Misc::convert_snapshot_time_with_zone($snapshots[$i] ->{"description"}, $snapshots[$i] -> {"created_at"});
			
			$snapshots[$i] ->{"description"} = $description;
			
		}
		$output = json_encode($snapshots);
	}
	
	print_r($output);	
	break;
	
	
	############################
	# List Available Network
	############################
	case "ListAvailabilityZone":
	$ACTION    = 'ListAvailabilityZone';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'ServerZONE'	=> $_REQUEST['SERVER_ZONE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
		
	
	############################
	# List Instance Flavors
	############################
	case "ListInstanceFlavors":
	$ACTION    = 'ListInstanceFlavors';	
	$REST_DATA = array(
					'Action' => $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SERV_REGN'	=> $_REQUEST['SERV_REGN']
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
	# List Available Keys
	############################
	/*case "ListAvailableKeys":
	$ACTION    = 'ListAvailableKeys';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'ServerZONE'	=> $_REQUEST['SERVER_ZONE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;*/
	
	
	############################
	# Query Flavor Information
	############################
	case "QueryFlavorInformation":
	$ACTION    = 'QueryFlavorInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'FlavorId'	=> $_REQUEST['FLAVOR_ID'],
					'SELECT_ZONE'	=> $_REQUEST['SELECT_ZONE']
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
	# Query Key Pair Information
	############################
	/*case "QueryKeyPairInformation":
	$ACTION    = 'QueryKeyPairInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'KeyPairName'	=> $_REQUEST['KEYPAIR_NAME']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;*/
	
	
	############################
	# Query Snapshot Information
	############################
	case "QuerySnapshotInformation":
	$ACTION    = 'QuerySnapshotInformation';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'SnapshotID'	=> $_REQUEST['SNAPSHOT_ID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	/*if ($output != FALSE)
	{
		#RE-FORMAT SNAPSHOT ARRAY LIST
		$snapshots = json_decode($output);
		for ($i=0; $i<count($snapshots); $i++)
		{
			#SET DEFAULT TAG VALUES
			$description = 'No description';
			$name = 'No Name';
			
			#BEGIN TO GET TAG VALUES
			$snapshot_tags = $snapshots[$i][0] -> Tags;			
			for ($x=0; $x<count($snapshot_tags); $x++)
			{
				if ($snapshot_tags[$x] -> Key == 'Description')
				{
					$description = Misc::convert_snapshot_time_with_zone($snapshot_tags[$x] -> Value, $snapshots[$i][0] -> StartTime);					
				}
				
				if ($snapshot_tags[$x] -> Key == 'Name')
				{
					$name = $snapshot_tags[$x] -> Value;
				}				
			}
			
			$FormatOutput[] = array(
								'status' 		=> $snapshots[$i][0] -> State,								
								'name'			=> $name,
								'volume_id'		=> $snapshots[$i][0] -> VolumeId,
								'created_at'	=> $snapshots[$i][0] -> StartTime,
								'size'			=> $snapshots[$i][0] -> VolumeSize,								
								'progress'		=> $snapshots[$i][0] -> Progress,
								'id'			=> $snapshots[$i][0] -> SnapshotId,
								'description'	=> $description
							);
		}
		$output = json_encode($FormatOutput);
	}	*/
	print_r($output);
	break;
}