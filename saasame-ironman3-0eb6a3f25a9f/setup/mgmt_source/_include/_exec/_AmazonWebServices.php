<?php
$GetPostData = file_get_contents("php://input"); 
$PostData = json_decode($GetPostData,true);

/* Secure Key Identification */
if (isset($PostData['EncryptKey']))
{
	$EncryptKey = $PostData['EncryptKey'];
	$KeyDecrypt = Misc_Class::encrypt_decrypt('decrypt', $EncryptKey);
	$DefineTime = time();
	$MinAllowed = $DefineTime - 15;
	$MaxAllowed = $DefineTime + 15;
	if ($MinAllowed < $KeyDecrypt AND $KeyDecrypt < $MaxAllowed)
	{
		$SelectAction = $PostData['Action'];
		
		$AwsWebActMgmt = new Aws_Action_Class();
		$AwsWebSevMgmt = new Aws_Query_Class();
	}
	else
	{
		header($_SERVER["SERVER_PROTOCOL"]." 401 Unauthorized");
		print_r(json_encode(false));
		exit;
	}
}
else
{
	header($_SERVER["SERVER_PROTOCOL"]." 401 Unauthorized");
	print_r(json_encode(false));
	exit;
}

if (isset($SelectAction))
{
	switch ($SelectAction)
	{
		############################
		# Verify Aws Access Credential
		############################
		case "VerifyAwsAccessCredential":
			$AWS_REGION = $PostData['AwsRegion'];
			$ACCESS_KEY = $PostData['AccessKey'];
			$SECRET_KEY = $PostData['SecretKey'];
			$Result = $AwsWebActMgmt -> verify_aws_credential($AWS_REGION,$ACCESS_KEY,$SECRET_KEY);
			
			if ($Result != FALSE)
			{
				echo json_encode(array('Code' => true, 'Msg' => ''));
			}
			else
			{
				echo json_encode(array('Code' => false, 'Msg' => 'Cloud connection could not be verified.<br>Please check that the information provided is correct.'));
			}
		break;

		
		############################
		# Initialize New Aws Connection
		############################
		case "InitializeNewAwsConnection":
			$ACCT_UUID  = $PostData['AcctUUID'];
			$REGN_UUID  = $PostData['RegnUUID'];
			$AWS_REGION = $PostData['AwsRegion'];
			$ACCESS_KEY = $PostData['AccessKey'];
			$SECRET_KEY = $PostData['SecretKey'];
			//$USER_NAME  = $AwsWebActMgmt -> verify_aws_credential($AWS_REGION,$ACCESS_KEY,$SECRET_KEY)['User']['UserName'];
				
			$AWS_TYPE = $AwsWebSevMgmt -> create_aws_connection($ACCT_UUID,$REGN_UUID,$AWS_REGION,$ACCESS_KEY,$SECRET_KEY);			

			print_r(json_encode(array('Code' => true, 'Msg' => 'New '.$AWS_TYPE.' connection added.')));		
		break;
		
		
		############################
		# Query Access Credential Information
		############################
		case "QueryAccessCredentialInformation":
		
			$CloudConnection = $AwsWebSevMgmt -> query_aws_connection_information($PostData['ClusterUUID']);
		
			print_r(json_encode($CloudConnection));		
		break;
		
		
		############################
		# Update Aws Connection
		############################
		case "UpdateAwsConnection":
			$CLUSTER_UUID  	= $PostData['ClusterUUID'];
			$AWS_REGION 	= $PostData['AwsRegion'];
			$ACCESS_KEY 	= $PostData['AccessKey'];
			$SECRET_KEY 	= $PostData['SecretKey'];
			
			$AWS_TYPE = $AwsWebSevMgmt -> update_aws_connection($CLUSTER_UUID,$AWS_REGION,$ACCESS_KEY,$SECRET_KEY);			
			
			print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update '.$AWS_TYPE.' connection.')));		
		break;
		
		
		############################
		# List Installed Instances
		############################
		case "ListInstalledInstances":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$ListAllInstances = $AwsWebActMgmt -> describe_all_instances($CLOUD_UUID);
			
			print_r(json_encode($ListAllInstances));
		break;
		
		
		############################
		# Query Selected Host Information
		############################
		case "QuerySelectedHostInformation":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_UUID  = $PostData['HostUUID'];
			$HOST_REGN  = $PostData['HostREGN'];
			
			$ListInstance = $AwsWebActMgmt -> describe_instance($CLOUD_UUID,$HOST_REGN,$HOST_UUID);
			
			print_r(json_encode($ListInstance));
		break;

		
		############################
		# List Available Ebs Snapshot
		############################
		case "ListAvailableEbsSnapshot":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_REGN  = $PostData['ServZone'];
			$VOLUME_ID  = $PostData['DiskID'];
			
			$ListSnapshots = $AwsWebActMgmt -> describe_snapshots($CLOUD_UUID,$HOST_REGN,$VOLUME_ID);
			
			print_r(json_encode($ListSnapshots));
		break;
		
		
		############################
		# List Availability Zones
		############################
		case "ListAvailabilityZone":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
				
			$ListSnapshots = $AwsWebActMgmt -> describe_availability_zones($CLOUD_UUID,$SERVER_ZONE);
			
			print_r(json_encode($ListSnapshots));
		break;
		
		
		############################
		# List Instance Flavors
		############################
		case "ListInstanceFlavors":
				
			$ListFlavors = $AwsWebActMgmt -> describe_instance_types();
			
			print_r(json_encode($ListFlavors));
		break;
		
		
		############################
		# List Available Network
		############################
		case "ListAvailableNetwork":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
							
			$ListNetworks = $AwsWebActMgmt -> describe_available_network($CLOUD_UUID,$SELECT_ZONE);
			
			print_r(json_encode($ListNetworks));
		break;
		
		
		############################
		# Describe Network Interfaces
		############################
		case "DescribeNetworkInterfaces":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
			$VPC_UUID	  = $PostData['VpcUUID'];
		
			$ListNetworkInterface = $AwsWebActMgmt -> describe_network_interfaces($CLOUD_UUID,$SERVER_ZONE,$VPC_UUID);
			
			print_r(json_encode($ListNetworkInterface));
		break;
		
		
		############################
		# List Security Group
		############################
		case "ListSecurityGroup":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
			$VPC_UUID	  = $PostData['VpcUUID'];
			
			$SecurityGroup = $AwsWebActMgmt -> list_security_groups($CLOUD_UUID,$SERVER_ZONE,$VPC_UUID);
			
			print_r(json_encode($SecurityGroup));
		break;
		
		
		############################
		# List Key Pairs
		############################
		case "ListAvailableKeys":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
				
			$SecurityGroup = $AwsWebActMgmt -> describe_key_pairs($CLOUD_UUID,$SERVER_ZONE);
			
			print_r(json_encode($SecurityGroup));
		break;
		
		
		############################
		# Query Flavor Information
		############################
		case "QueryFlavorInformation":
			$FLAVOR_ID  = $PostData['FlavorId'];
				
			$InstanceTypeInfo = $AwsWebActMgmt -> describe_instance_types($FLAVOR_ID);
			
			print_r(json_encode($InstanceTypeInfo));
		break;
		
		
		############################
		# Query Network Information
		############################
		case "QueryNetworkInformation":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SELECT_ZONE  = $PostData['SelectZONE'];
			$NETWORK_UUID = $PostData['NetworkUUID'];
				
			$SelectNetwork = $AwsWebActMgmt -> describe_available_network($CLOUD_UUID,$SELECT_ZONE,$NETWORK_UUID);
			
			print_r(json_encode($SelectNetwork));
		break;
		
		
		############################
		# Query Security Group Information
		############################
		case "QuerySecurityGroupInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SGROUP_UUID = $PostData['SGroupUUID'];
						
			$SelectSecurityGroup = $AwsWebActMgmt -> describe_security_group($CLOUD_UUID,$SELECT_ZONE,$SGROUP_UUID);
			
			print_r(json_encode($SelectSecurityGroup));
		break;
		
		
		############################
		# Query Volume Detail Info
		############################
		case "QueryVolumeDetailInfo":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SELECT_ZONE  = $PostData['SelectZONE'];
			$DISK_UUID 	  = $PostData['DiskUUID'];
				
			$QueryVolumeInformation = $AwsWebActMgmt -> describe_volumes($CLOUD_UUID,$SELECT_ZONE,$DISK_UUID);
			
			print_r(json_encode($QueryVolumeInformation));
		break;
		
		
		############################
		# Query Snapshot Information
		############################
		case "QuerySnapshotInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SNAPSHOT_ID = $PostData['SnapshotID'];
			$ReplicaDisk = $PostData['ReplicaDisk'];
			
			if ($SNAPSHOT_ID != 'UseLastSnapshot')
			{
				$SNAP_ID = explode(',',$SNAPSHOT_ID);
				for ($i=0; $i<count($SNAP_ID); $i++)
				{
					if ($SNAP_ID[$i] != 'false')
					{					
						$SelectSnapshotInfo[] = $AwsWebActMgmt -> describe_snapshots($CLOUD_UUID,$SELECT_ZONE,null,$SNAP_ID[$i]);
					}
				}					
			}
			else
			{
				$REPLICA_DISK = explode(',',$ReplicaDisk);
				for ($i=0; $i<count($REPLICA_DISK); $i++)
				{
					$GET_SNAPSHOT_LIST = $AwsWebActMgmt -> describe_snapshots($CLOUD_UUID,$SELECT_ZONE,$REPLICA_DISK[$i],null);
					$SelectSnapshotInfo[] = array_slice($GET_SNAPSHOT_LIST, -1);		
				}		
			}
			print_r(json_encode($SelectSnapshotInfo));
		break;
		
		
		############################
		# Query Volume Information
		############################
		case "QueryVolumeInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
				
			$ReplMgmt = new Replica_Class();
			
			$RECOVER_DISK = $ReplMgmt -> query_replica_disk($PostData['ReplUUID']);
		
			$TO_BE_FILTER = array_values(array_diff(explode(',',$PostData['VolumeId']),array('false')));
		
			#FILTER OUT RECOVERY DISK
			foreach ($TO_BE_FILTER as $PM_FILTER_UUID)
			{
				for ($f=0; $f<count($RECOVER_DISK); $f++)
				{
					if ($RECOVER_DISK[$f]['OPEN_DISK'] == $PM_FILTER_UUID)
					{
						unset($RECOVER_DISK[$f]);			
					}	
				}
				$RECOVER_DISK = array_values($RECOVER_DISK); #RE-INDEX
			}

			foreach ($RECOVER_DISK as $OPEN_DISK)
			{
				$SelectVolumeInfo[] = $AwsWebActMgmt -> describe_volumes($CLOUD_UUID,$SELECT_ZONE,$OPEN_DISK['OPEN_DISK']);
			}
			
			print_r(json_encode($SelectVolumeInfo));
		break;
		
		
		############################
		# List Elastic IP addresses
		############################
		case "ListElasticAddresses":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			
			$ListElasticAddress = $AwsWebActMgmt -> describe_addresses($CLOUD_UUID,$SELECT_ZONE,null);
			
			print_r(json_encode($ListElasticAddress));			
		break;
		
		############################
		# Query Elastic IP addresses
		############################
		case "QueryElasticAddresses":
			$CLOUD_UUID  	= $PostData['ClusterUUID'];
			$SELECT_ZONE 	= $PostData['SelectZONE'];
			$PUBLIC_ADDR_ID = $PostData['PUBLIC_ADDR_ID'];
			
			$ListElasticAddress = $AwsWebActMgmt -> describe_addresses($CLOUD_UUID,$SELECT_ZONE,$PUBLIC_ADDR_ID);
			
			print_r(json_encode($ListElasticAddress));	
		break;
		
		############################
		# List Region Transport
		############################
		case "ListRegionInstances":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SELECT_ZONE  = $PostData['SelectZONE'];
			$TRANSPORT_ID = $PostData['TransportId'];
			
			$ListElasticAddress = $AwsWebActMgmt -> describe_region_instances($CLOUD_UUID,$SELECT_ZONE,$TRANSPORT_ID);
			
			print_r(json_encode($ListElasticAddress));			
		break;
	}
}

