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
		
		$CtyunActMgmt = new Ctyun_Action_Class();
		$CtyunSevMgmt = new Ctyun_Query_Class();
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
		# Verify Ctyun Access Credential
		############################
		case "VerifyCtyunAccessCredential":
					
			$Result = $CtyunActMgmt -> verify_ctyun_credential($PostData);
			
			if ($Result != FALSE)
			{
				echo json_encode(array('Code' => true, 'Msg' => '', 'Regions' => $Result));
			}
			else
			{
				echo json_encode(array('Code' => false, 'Msg' => 'Cloud connection could not be verified.<br>Please check that the information provided is correct.', 'Regions' => false));
			}
		break;

		
		############################
		# Initialize New Ctyun Connection
		############################
		case "InitializeNewCtyunConnection":
		
			$CtyunSevMgmt -> create_ctyun_connection($PostData);			
			
			print_r(json_encode(array('Code' => true, 'Msg' => 'New 天翼云 connection added.')));		
		break;
		
		
		############################
		# Query Access Credential Information
		############################
		case "QueryAccessCredentialInformation":
		
			$CloudConnection = $CtyunSevMgmt -> query_ctyun_connection_information($PostData['ClusterUUID']);
		
			print_r(json_encode($CloudConnection));		
		break;
		
		
		############################
		# Update Aws Connection
		############################
		case "UpdateCtyunConnection":
			
			$CtyunSevMgmt -> update_ctyun_connection($PostData);			
			
			print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update 天翼云 connection.')));		
		break;
		
		
		############################
		# List Installed Instances
		############################
		case "ListInstalledInstances":
	
			$ListAllInstances = $CtyunActMgmt -> describe_all_instances($PostData['ClusterUUID'],null);
			
			print_r(json_encode($ListAllInstances));
		break;
		
		
		############################
		# Query Selected Host Information
		############################
		case "QuerySelectedHostInformation":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_UUID = $PostData['HostUUID'];
			
			$QueryInstance = $CtyunActMgmt -> describe_instance($CLOUD_UUID,$HOST_UUID);
			
			print_r(json_encode($QueryInstance));
		break;
		
		############################
		# Query Volume Detail Info
		############################
		case "QueryVolumeDetailInfo":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$VOLUME_ID  = $PostData['DiskID'];
			
			$VolumeInfo = $CtyunActMgmt -> describe_volumes($CLOUD_UUID,$VOLUME_ID);
			
			print_r(json_encode($VolumeInfo));
		break;
		
		############################
		# List Available Snapshot
		############################
		case "ListAvailableSnapshot":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$VOLUME_ID  = $PostData['DiskID'];
			
			$ListSnapshots = $CtyunActMgmt -> list_available_snapshot($CLOUD_UUID,$VOLUME_ID);
			
			print_r(json_encode($ListSnapshots));
		break;
				
		############################
		# List Instance Flavors
		############################
		case "ListInstanceFlavors":			
			$CLOUD_UUID = $PostData['ClusterUUID'];
			
			$ListFlavors = $CtyunActMgmt -> get_flavor_list_detail($CLOUD_UUID);
			
			print_r(json_encode($ListFlavors));
		break;
		
		
		############################
		# List Available Network
		############################
		case "ListAvailableNetwork":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			
			$ListNetworks = $CtyunActMgmt -> describe_available_network($CLOUD_UUID);
			
			print_r(json_encode($ListNetworks));
		break;
		
		
		############################
		# Query Network Information
		############################
		case "QuerySubnetInformation":
			$CLOUD_UUID   = $PostData['ClusterUUID'];			
			$NETWORK_UUID = $PostData['NetworkUUID'];
				
			$SelectNetwork = $CtyunActMgmt -> describe_subnet($CLOUD_UUID,$NETWORK_UUID);
			
			print_r(json_encode($SelectNetwork));
		break;
		
		
		############################
		# List Security Group
		############################
		case "ListSecurityGroup":
			$CLOUD_UUID    = $PostData['ClusterUUID'];
			
			$SecurityGroup = $CtyunActMgmt -> describe_available_security_groups($CLOUD_UUID);
			
			print_r(json_encode($SecurityGroup));
		break;
		
		
		############################
		# List Public IP addresses
		############################
		case "ListAvailablePublicIP":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			
			$ListPublicAddress = $CtyunActMgmt -> describe_available_public_address($CLOUD_UUID);
			
			print_r(json_encode($ListPublicAddress));			
		break;
		
		############################
		# List DataMode Instances
		############################
		case "ListDataModeInstances":
	
			$ListAllInstances = $CtyunActMgmt -> describe_all_instances($PostData['ClusterUUID'],$PostData['FilterUUID']);
			
			print_r(json_encode($ListAllInstances));
		break;
		
		############################
		# Query Flavor Information
		############################
		case "QueryFlavorInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$FLAVOR_ID  = $PostData['FlavorId'];
				
			$InstanceTypeInfo = $CtyunActMgmt -> describe_instance_types($CLOUD_UUID,$FLAVOR_ID);
			
			print_r(json_encode($InstanceTypeInfo));
		break;
			
		############################
		# Query Public IP addresses
		############################
		case "QueryPublicAddresses":
			$CLOUD_UUID  	= $PostData['ClusterUUID'];			
			$PUBLIC_ADDR_ID = $PostData['PUBLIC_ADDR_ID'];
			
			$QueryAddress = $CtyunActMgmt -> describe_public_address($CLOUD_UUID,$PUBLIC_ADDR_ID);
			
			print_r(json_encode($QueryAddress));	
		break;		
		
		############################
		# Query Security Group Information
		############################
		case "QuerySecurityGroupInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];			
			$SGROUP_UUID = $PostData['SGroupUUID'];
				
			$SelectSecurityGroup = $CtyunActMgmt -> describe_security_group($CLOUD_UUID,$SGROUP_UUID);
			
			print_r(json_encode($SelectSecurityGroup));
		break;
		
		############################
		# Query Snapshot Information
		############################
		case "QuerySnapshotInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];			
			$SNAPSHOT_ID = $PostData['SnapshotID'];
						
			if ($SNAPSHOT_ID != 'UseLastSnapshot')
			{
				$SNAP_ID = explode(',',$SNAPSHOT_ID);
				for ($i=0; $i<count($SNAP_ID); $i++)
				{
					$SelectSnapshotInfo[] = $CtyunActMgmt -> describe_snapshot($CLOUD_UUID,$SNAP_ID[$i]);
				}
			}
			else
			{
				/*$REPLICA_DISK = explode(',',$ReplicaDisk);
				for ($i=0; $i<count($REPLICA_DISK); $i++)
				{
					$GET_SNAPSHOT_LIST = $AwsWebActMgmt -> describe_snapshots($CLOUD_UUID,$SELECT_ZONE,$REPLICA_DISK[$i],null);
					$SelectSnapshotInfo[] = array_slice($GET_SNAPSHOT_LIST, -1);		
				}*/		
			}
			print_r(json_encode($SelectSnapshotInfo));
		break;
		
		############################
		# Describe Volumes
		############################
		case "DescribeVolumes":
			$CLOUD_UUID  = $PostData['CloudUUID'];
			
			$DescribeVolumes = $CtyunActMgmt -> describe_volumes($CLOUD_UUID,null);
			print_r(json_encode($DescribeVolumes));
		break;
	}
}

