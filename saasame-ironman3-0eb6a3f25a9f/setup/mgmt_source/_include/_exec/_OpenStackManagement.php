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
		$OpenStackActionMgmt = new OpenStack_Action_Class();		
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
		# List OpenStack Connection
		############################
		case "ListOpenStackConnection":
			
			$QueryConnection = $OpenStackActionMgmt -> list_openstack_connection($PostData['AcctUUID']);
			
			print_r(json_encode($QueryConnection));		
		break;		
		
		############################
		# Verify OpenStack Connection
		############################
		case "VerifyOpenStackConnection":
			$AUTH_INFO = $OpenStackActionMgmt -> generate_auth_token($PostData);
		
			if (isset($AUTH_INFO -> AUTH_TOKEN) AND strlen($AUTH_INFO -> AUTH_TOKEN) > 13)
			{
				$LIST_REGION = $OpenStackActionMgmt -> list_regions($AUTH_INFO);
				
				$LIST_PROJECT = $OpenStackActionMgmt -> list_projects($AUTH_INFO);

				if ($AUTH_INFO -> ENDPOINT_REF == '')
				{
					echo json_encode(array('Code' => false, 'Msg' => 'Please associate a default project to the account.', 'Regions' => $LIST_REGION, 'Project' => $LIST_PROJECT, 'AuthVersion' => $AUTH_INFO -> auth_version));
				}
				else
				{				
					echo json_encode(array('Code' => true, 'Msg' => '', 'Regions' => $LIST_REGION, 'Project' => $LIST_PROJECT, 'AuthVersion' => $AUTH_INFO -> auth_version));
				}			
			}
			else
			{
				echo json_encode(array('Code' => false, 'Msg' => 'Cloud connection could not be verified.<br>Please check that the information provided is correct.', 'Regions' => false , 'Project' => false, 'AuthVersion' => false));
			}
		break;
		
		############################
		# Query Endpoint Information
		############################
		case "QueryEndpointInformation":
			$AUTH_INFO = $OpenStackActionMgmt -> generate_auth_token($PostData);
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$AUTH_PATH = $AUTH_INFO -> ENDPOINT_REF;
			
			$OpenStackInfo = $OpenStackActionMgmt -> query_openstack_connection_information($CLOUD_UUID);
			if ($OpenStackInfo == false OR $OpenStackInfo['AUTH_PROJECT_ID'] != $PostData['ProjectId'])
			{
				$CLOUD_UUID = false;
			}
			
			$ENDPOINT_REF = $OpenStackActionMgmt -> QueryEndPointReference($CLOUD_UUID,$AUTH_PATH);
			
			echo json_encode($ENDPOINT_REF);
		break;
		
		############################
		# Initialize New OpenStack Connection
		############################
		case "InitializeNewOpenStackConnection":
			#DEFINE API ADDRESS
			$CLUSTER_ADDR = $PostData['ClusterVipAddr'];
			
			#DEFINE PROJECT ID
			$PROJECT_ID = $PostData['ProjectId'];
			
			#AUTH REGION
			$AUTH_REGION = explode(".",$CLUSTER_ADDR)[1];
			
			#AUTH INIT
			$AUTH_INIT = $OpenStackActionMgmt -> generate_auth_token($PostData);
			
			#GET PROJECT ID FOR HUAWEI CLOUD
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{			
				$PROJECT_ID = $OpenStackActionMgmt -> generate_project_id($AUTH_INIT,$AUTH_REGION);
				$PostData['ProjectId'] = $PROJECT_ID;
			}
			else
			{
				$AUTH_REGION = $OpenStackActionMgmt -> query_project_name($AUTH_INIT,$PROJECT_ID);
			}
		
			#GET TOKEN
			$AUTH_INFO = $OpenStackActionMgmt -> generate_auth_token($PostData);
			$AUTH_TOKEN = $AUTH_INFO -> AUTH_TOKEN;
			
			$ACCT_UUID    = $PostData['AcctUUID'];
			$REGN_UUID    = $PostData['RegnUUID'];
			$PROJECT_NAME = $PostData['ProjectName'];			
			$CLUSTER_USER = $PostData['ClusterUsername'];
			$CLUSTER_PASS = $PostData['ClusterPassword'];
			$CLUSTER_ADDR = $PostData['ClusterVipAddr'];
			
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{
				$VENDOR_NAME = 'Huawei Cloud';			
			}
			else
			{
				$VENDOR_NAME = 'OpenStack';
			}
			
			$AUTH_DATA = array('auth_token' => $AUTH_TOKEN, 'domain_name' => $PROJECT_NAME, 'project_region' => $AUTH_REGION, 'project_id' => $PROJECT_ID, 'vendor_name' => $VENDOR_NAME, 'identity_protocol' => $PostData['IdentityProtocol'], 'identity_port' => $PostData['IdentityPort']);
			
			$CLUSTER_UUID = $OpenStackActionMgmt -> create_openstack_connection($ACCT_UUID,$REGN_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA);

			#UPDATE ENDPOINT REFERENCE
			if (isset($PostData['EndpointRefAddr']) AND $PostData['EndpointRefAddr'] != '')
			{
				$ENDPOINT_REF = $PostData['EndpointRefAddr'];
				$OpenStackActionMgmt -> GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,'JSON');
			}
			else
			{
				$REFERENCE_INFO = $PostData['EndpointRefAddr'];
				$ENDPOINT_REF = $AUTH_INFO -> ENDPOINT_REF;
				$OpenStackActionMgmt -> GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,'New');
			}		
			
			#$OpenStackActionMgmt -> create_null_imgage($CLUSTER_UUID);

			print_r(json_encode(array('Code' => true, 'Msg' => 'New '.$VENDOR_NAME.' connection added.')));
		break;
		
		
		############################
		# Edit Select Connection
		############################
		case "EditSelectConnection":
		
			$CloudConnection = $OpenStackActionMgmt -> query_openstack_connection_information($PostData['ClusterUUID']);
		
			print_r(json_encode($CloudConnection));		
		break;
		
		
		############################
		# Delete Select Connection
		############################
		case "DeleteSelectConnection":
			
			$DeleteConnection = $OpenStackActionMgmt -> delete_connection($PostData['ClusterUUID']);
			
			if ($DeleteConnection == TRUE)
			{
				$Msg = 'Cloud connection deleted.';
			}
			else
			{
				$Msg = 'Associated transport still registered with select cloud connection.';
			}
			print_r(json_encode(array('Code' => $DeleteConnection, 'Msg' => $Msg)));			
		break;
		
		
		############################
		# Query Access Credential Information
		############################
		case "QueryAccessCredentialInformation":
		
			$CloudConnection = $OpenStackActionMgmt -> query_openstack_connection_information($PostData['ClusterUUID']);
			#$EndpointReference = $OpenStackActionMgmt -> QueryEndPointReference($PostData['ClusterUUID']);
			
			#$CloudConnection['ENDPOINTS'] = $EndpointReference;
		
			print_r(json_encode($CloudConnection));		
		break;
		
		
		############################
		# Update OpenStack Connection
		############################
		case "UpdateOpenStackConnection":
			#DEFINE API ADDRESS
			$CLUSTER_ADDR = $PostData['ClusterVipAddr'];
			
			#DEFINE PROJECT ID
			$PROJECT_ID = $PostData['ProjectId'];
			
			#AUTH REGION
			$AUTH_REGION = explode(".",$CLUSTER_ADDR)[1];			
			
			#AUTH INIT
			$AUTH_INIT = $OpenStackActionMgmt -> generate_auth_token($PostData);
			
			#GET PROJECT ID FOR HUAWEI CLOUD
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{			
				$PROJECT_ID = $OpenStackActionMgmt -> generate_project_id($AUTH_INIT,$AUTH_REGION);
				$PostData['ProjectId'] = $PROJECT_ID;
			}
			else
			{
				$AUTH_REGION = $OpenStackActionMgmt -> query_project_name($AUTH_INIT,$PROJECT_ID);
			}
			
			#GET TOKEN
			$AUTH_INFO = $OpenStackActionMgmt -> generate_auth_token($PostData);
			$AUTH_TOKEN = $AUTH_INFO -> AUTH_TOKEN;
			
			$CLUSTER_UUID = $PostData['ClusterUUID'];
			$PROJECT_NAME = $PostData['ProjectName'];
			$CLUSTER_USER = $PostData['ClusterUsername'];
			$CLUSTER_PASS = $PostData['ClusterPassword'];			
			
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{
				$VENDOR_NAME = 'Huawei Cloud';			
			}
			else
			{
				$VENDOR_NAME = 'OpenStack';
			}
			$AUTH_DATA = array('auth_token' => $AUTH_TOKEN, 'domain_name' => $PROJECT_NAME, 'project_region' => $AUTH_REGION, 'project_id' => $PROJECT_ID, 'vendor_name' => $VENDOR_NAME, 'identity_protocol' => $PostData['IdentityProtocol'], 'identity_port' => $PostData['IdentityPort']);
			
			$OpenStackActionMgmt -> update_openstack_connection($CLUSTER_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA);
			
			#UPDATE ENDPOINT REF
			if (isset($PostData['EndpointRefAddr']) AND $PostData['EndpointRefAddr'] != '')
			{
				$ENDPOINT_REF = $PostData['EndpointRefAddr'];
				$OpenStackActionMgmt -> GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,'JSON');
			}
			else
			{			
				$ENDPOINT_REF = $AUTH_INFO -> ENDPOINT_REF;			
				$OpenStackActionMgmt -> GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,'New');
			}
			print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update '.$VENDOR_NAME.' connection.')));	
		break;
		
		
		############################
		# List Installed Instances
		############################
		case "ListInstalledInstances":
		
			$GetVmDetailList = $OpenStackActionMgmt -> get_vm_detail_list($PostData['ClusterUUID'],null);
			
			print_r(json_encode($GetVmDetailList));
		break;

		
		############################
		# Query Selected Host Information
		############################
		case "QuerySelectedHostInformation":
		
			$QueryHostInformation = $OpenStackActionMgmt -> query_vm_detail_info($PostData['ClusterUUID'],$PostData['HostUUID']);
						
			print_r(json_encode($QueryHostInformation));
		break;	


		############################
		# List Available OpenStack Disk
		############################
		case "ListAvailableOpenStackDisk":
		
			$OPENSTACK_DISK = $OpenStackActionMgmt -> get_openstack_disk($PostData['ReplUUID']);
			
			echo json_encode($OPENSTACK_DISK);
		break;
				
		############################
		# Query Volume Information
		############################
		case "QueryVolumeDetailInfo":
			$VOLUME_INFO = $OpenStackActionMgmt -> get_volume_detail_info($PostData['ClusterUUID'],$PostData['DiskUUID']);
	
			echo json_encode($VOLUME_INFO);
		break;
		
		############################
		# List Available Snapshot
		############################
		case "ListAvailableSnapshot":
			$LIST_MATCH = $OpenStackActionMgmt -> list_available_snapshot($PostData['ClusterUUID'],$PostData['DiskUUID']);
	
			echo str_replace('.000000','',json_encode($LIST_MATCH));
		break;

		
		############################
		# Query Snapshot Information
		############################
		case "QuerySnapshotInformation":

			$SNAP_UUID = array_values(array_diff(explode(',',$PostData['SnapUUID']),array('false')));
			
			if ($PostData['SnapUUID'] != 'UseLastSnapshot')
			{
				for ($i=0; $i<count($SNAP_UUID); $i++)
				{
					$SNAPSHOT_DETAIL[] = $OpenStackActionMgmt -> get_snapshot_detail_list($PostData['ClusterUUID'],$SNAP_UUID[$i]);
				}
				
				echo str_replace('.000000','',json_encode($SNAPSHOT_DETAIL));	
			}
			else
			{
				$REPLICA_DISK = array_values(explode(',',$PostData['ReplicaDisk']));
				for ($i=0; $i<count($REPLICA_DISK); $i++)
				{
					$LAST_SNAPSHOT = $OpenStackActionMgmt -> list_available_snapshot($PostData['ClusterUUID'],$REPLICA_DISK[$i])[0];
					$LAST_SNAPSHOT -> {'os-extended-snapshot-attributes:progress'} = '100%';
					
					$SNAPSHOT_DETAIL[] = array('snapshot' => $LAST_SNAPSHOT);					
				}
				echo str_replace('.000000','',json_encode($SNAPSHOT_DETAIL));
			}
		break;
		
		
		############################
		# Query Volume Information
		############################
		case "QueryVolumeInformation":

			$ReplMgmt = new Replica_Class();

			$TO_BE_FILTER = array_values(array_diff(explode(',',$PostData['VolumeUUID']),array('false')));
			
			$RECOVER_DISK = $ReplMgmt -> query_replica_disk($PostData['ReplUUID']);
		
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
				$VOLUME_INFO[] = $OpenStackActionMgmt -> get_volume_detail_info($PostData['ClusterUUID'],$OPEN_DISK['OPEN_DISK']);				
			}
			
			echo json_encode($VOLUME_INFO);
		break;
		
		############################
		# List Instance Flavors
		############################
		case "ListInstanceFlavors":
						
			$GetFlavorList = $OpenStackActionMgmt -> get_flavor_list_detail($PostData['ClusterUUID']);
			
			echo json_encode($GetFlavorList);
		break;
		
		
		############################
		# List Internal Network
		############################
		case "ListInternalNetwork":
						
			$GetNetworkList = $OpenStackActionMgmt -> get_network_list_detail($PostData['ClusterUUID']);
			
			echo json_encode($GetNetworkList);
		break;
		
		
		############################
		# List Subnet Network
		############################
		case "ListSubnetNetworks":
		
			$GetSubnetList = $OpenStackActionMgmt -> get_subnet_list_detail($PostData['ClusterUUID']);
			
			echo json_encode($GetSubnetList);
		break;
		
		
		############################
		# List Available Floating IP
		############################
		case "ListAvailableFloatingIP":
		
			$GetAddressList = $OpenStackActionMgmt -> get_available_network_address($PostData['ClusterUUID']);
			
			echo json_encode($GetAddressList);
		break;
			
		
		############################
		# List Security Group
		############################
		case "ListSecurityGroup":
						
			$GetSecurityGroupList = $OpenStackActionMgmt -> get_security_group_list_detail($PostData['ClusterUUID']);
			
			echo json_encode($GetSecurityGroupList);
		break;
		
		############################
		# List DataMode Instances
		############################
		case "ListDataModeInstances":
		
			$GetVmDetailList = $OpenStackActionMgmt -> get_vm_detail_list($PostData['ClusterUUID'],$PostData['FilterUUID']);
			
			print_r(json_encode($GetVmDetailList));
		break;
		
		
		############################
		# Query Flavor Information
		############################
		case "QueryFlavorInformation":
					
			$GetFlavorDetailInfo = $OpenStackActionMgmt -> get_flavor_detail_info($PostData['ClusterUUID'],$PostData['FlavorID']);
			
			echo json_encode($GetFlavorDetailInfo);
		break;
		
		
		############################
		# Query Network Information
		############################
		case "QueryNetworkInformation":
					
			$GetNetworkDetailInfo = $OpenStackActionMgmt -> get_network_detail_info($PostData['ClusterUUID'],$PostData['NetworkUUID']);
			
			echo json_encode($GetNetworkDetailInfo);
		break;
		
		
		############################
		# Query Floating IP Information
		############################
		case "QueryFloatingIpInformation":
					
			$GetFloatingID = $OpenStackActionMgmt -> get_floating_ip_details($PostData['ClusterUUID'],$PostData['FloatingID']);
			
			echo json_encode($GetFloatingID);
		break;
		
		
		############################
		# Query Subnet Information
		############################
		case "QuerySubnetInformation":
					
			$GetSubnetDetailInfo = $OpenStackActionMgmt -> get_subnet_detail_info($PostData['ClusterUUID'],$PostData['SubnetUUID']);
			
			echo json_encode($GetSubnetDetailInfo);
		break;
		
		############################
		# Query Security Group Information
		############################
		case "QuerySecurityGroupInformation":
					
			$GetSecurityGroupDetailInfo = $OpenStackActionMgmt -> get_security_group_detail_info($PostData['ClusterUUID'],$PostData['SgroupUUID']);
			
			echo json_encode($GetSecurityGroupDetailInfo);
		break;
	}
}