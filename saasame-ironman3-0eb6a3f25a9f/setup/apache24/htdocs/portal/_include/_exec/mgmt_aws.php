<?php
set_time_limit(0);
session_start();
include '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA)
{
	$URL = 'http://127.0.0.1:8080/restful/AmazonWebServices';
	
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
	case "VerifyAwsAccessCredential":
	
	if ($_REQUEST['AWS_ACCESS_KEY'] != '' and $_REQUEST['AWS_SECRET_KEY'] != '')
	{
		$REST_DATA = array(
						'Action'	=> 'VerifyAwsAccessCredential',
						'AwsRegion'	=> $_REQUEST['AWS_REGION'],
						'AccessKey' => $_REQUEST['AWS_ACCESS_KEY'],
						'SecretKey' => $_REQUEST['AWS_SECRET_KEY']				
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$output = json_encode(array('Code' => false, 'Msg' =>  _('Please fill in all of the required fields.')));
	}
	print_r($output);
	break;
			
	############################
	# Initialize New Aws Connection
	############################
	case "InitializeNewAwsConnection":
	
	$REST_DATA = array(
					'Action'	=> 'InitializeNewAwsConnection',
					'AcctUUID'  => $_REQUEST['ACCT_UUID'],
					'RegnUUID'  => $_REQUEST['REGN_UUID'],
					'AwsRegion' => $_REQUEST['AWS_REGION'],
					'AccessKey' => $_REQUEST['AWS_ACCESS_KEY'],
					'SecretKey' => $_REQUEST['AWS_SECRET_KEY']				
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
	# Update Aws Connection
	############################
	case "UpdateAwsConnection":
	
	$REST_DATA = array(
					'Action'		=> 'UpdateAwsConnection',
					'ClusterUUID'  	=> $_REQUEST['CLUSTER_UUID'],
					'AwsRegion' 	=> $_REQUEST['AWS_REGION'],
					'AccessKey' 	=> $_REQUEST['AWS_ACCESS_KEY'],
					'SecretKey'		=> $_REQUEST['AWS_SECRET_KEY']				
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
					'HostUUID'		=> $_REQUEST['HOST_UUID'],
					'HostREGN'		=> substr($_REQUEST['HOST_REGN'],0,-1)
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# List Available Ebs Snapshot
	############################
	case "ListAvailableEbsSnapshot":
	$ACTION    = 'ListAvailableEbsSnapshot';	
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
		$snapshots = json_decode($output);
		for ($i=0; $i<count($snapshots); $i++)
		{
			#SET DEFAULT TAG VALUES
			$description = 'No description';
			$name = 'No Name';
			
			#BEGIN TO GET TAG VALUES
			$snapshot_tags = $snapshots[$i] -> Tags;
			for ($x=0; $x<count($snapshot_tags); $x++)
			{
				if ($snapshot_tags[$x] -> Key == 'Description')
				{
					$description = Misc::convert_snapshot_time_with_zone($snapshot_tags[$x] -> Value, $snapshots[$i] -> StartTime);					
				}
				
				if ($snapshot_tags[$x] -> Key == 'Name')
				{
					$name = $snapshot_tags[$x] -> Value;
				}	
			}
			
			$FormatOutput[] = array(
								'status' 		=> $snapshots[$i] -> State,								
								'name'			=> $name,
								'volume_id'		=> $snapshots[$i] -> VolumeId,
								'created_at'	=> $snapshots[$i] -> StartTime,
								'size'			=> $snapshots[$i] -> VolumeSize,								
								'progress'		=> $snapshots[$i] -> Progress,
								'id'			=> $snapshots[$i] -> SnapshotId,
								'description'	=> $description
							);
		}
		$output = json_encode($FormatOutput);
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
					'Action' => $ACTION
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
	# Describe Network Interfaces
	############################
	case "DescribeNetworkInterfaces":
	$ACTION    = 'DescribeNetworkInterfaces';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'ServerZONE'	=> $_REQUEST['SERVER_ZONE'],
					'VpcUUID'		=> $_REQUEST['VPC_UUID']
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
					'ServerZONE'	=> $_REQUEST['SERVER_ZONE'],
					'VpcUUID'		=> $_REQUEST['VPC_UUID']
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
					'FlavorId'	=> $_REQUEST['FLAVOR_ID']
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
	# Query Volume Detail Info
	############################
	case "QueryVolumeDetailInfo":
	$ACTION    = 'QueryVolumeDetailInfo';	
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SERVER_ZONE'],
					'DiskUUID'		=> $_REQUEST['DISK_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	$volume_detail = json_decode($output)[0];

	foreach ($volume_detail as $volume_key => $volume_value)
	{
		if ($volume_key == 'Tags')
		{
			for ($i=0; $i<count($volume_value); $i++)
			{
				if ($volume_value[$i] -> Key == 'Description')
				{
					$volume_info = explode('@',$volume_value[$i] -> Value);
					
					$volume_detail -> volume_name = Misc::convert_snapshot_time_with_zone($volume_info[0], $volume_info[1]);
				}
			}
		}
	}
	
	print_r(json_encode($volume_detail));
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
					'SnapshotID'  	=> $_REQUEST['SNAPSHOT_ID'],
					'ReplicaDisk' 	=> $_REQUEST['REPLICA_DISK']
				);
	$output = CurlPostToRemote($REST_DATA);
	if ($output != FALSE)
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
	}	
	print_r($output);
	break;
	
	############################
	# Query Volume Information
	############################
	case "QueryVolumeInformation":
		$ACTION    = 'QueryVolumeInformation';	
		$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'ReplUUID'		=> $_REQUEST['REPL_UUID'],
					'VolumeId'  	=> $_REQUEST['VOLUME_ID']
				);
		$output = CurlPostToRemote($REST_DATA);
		if ($output != FALSE)
		{
			#RE-FORMAT VOLUME ARRAY LIST
			$volumes = json_decode($output);
			for ($i=0; $i<count($volumes); $i++)
			{
				#SET DEFAULT TAG VALUES
				$description = 'No description';
				$name = 'No Name';
				
				#BEGIN TO GET TAG VALUES
				$volume_tags = $volumes[$i][0] -> Tags;			
				for ($x=0; $x<count($volume_tags); $x++)
				{					
					if ($volume_tags[$x] -> Key == 'Name')
					{
						$name = $volume_tags[$x] -> Value;
					}				
				}
				
				$FormatOutput[] = array(
									'status' 		=> $volumes[$i][0] -> State,								
									'name'			=> $name,
									'id'			=> $volumes[$i][0] -> VolumeId,
									'created_at'	=> Misc::convert_volume_time_with_zone($volumes[$i][0] -> CreateTime),
									'size'			=> $volumes[$i][0] -> Size
								);
			}
			$output = json_encode($FormatOutput);
		}
		print_r($output);
	break;
		
	############################
	# Query Elastic IP addresses
	############################
	case "ListElasticAddresses":
	$ACTION    = 'ListElasticAddresses';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);	
	break;
	
	############################
	# Query Elastic IP addresses
	############################
	case "QueryElasticAddresses":
	$ACTION    = 'QueryElasticAddresses';

	if ($_REQUEST['PUBLIC_ADDR_ID'] == 'DynamicAssign')
	{
		$output = json_encode(array(array('PublicIp' => 'Dynamic Assign')));
	}
	else
	{	
		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
						'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
						'PUBLIC_ADDR_ID'=> $_REQUEST['PUBLIC_ADDR_ID']					
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	print_r($output);	
	break;
	
	############################
	# List Region Transport
	############################
	case "ListRegionInstances":
	$ACTION    = 'ListRegionInstances';
	$REST_DATA = array(
					'Action' 		=> $ACTION,
					'ClusterUUID'	=> $_REQUEST['CLUSTER_UUID'],
					'SelectZONE'	=> $_REQUEST['SELECT_ZONE'],
					'TransportId'	=> $_REQUEST['TRANSPORT_ID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;
	break;
}