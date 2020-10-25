<?php
set_time_limit(0);
session_start();
include '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA)
{
	$URL = 'http://127.0.0.1:8080/restful/OpenStackManagement';
	
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
	#return json_decode($output,true);
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# List OpenStack Connection
	############################
	case "ListOpenStackConnection":
		$ACTION    = 'ListOpenStackConnection';
		$REST_DATA = array(
					'Action' 			=> $ACTION,
					'AcctUUID'			=> $_REQUEST['ACCT_UUID']				
				);
	
	$output = CurlPostToRemote($REST_DATA);
	
	$JSON_DECODE = json_decode($output);
	
	if ($JSON_DECODE != FALSE)
	{
		for ($i=0; $i<count($JSON_DECODE); $i++)
		{
			$LIST_CLOUD[] = array(
							'CLUSTER_UUID' 	=> $JSON_DECODE[$i] -> CLUSTER_UUID,
							'PROJECT_NAME' 	=> $JSON_DECODE[$i] -> PROJECT_NAME,
							'CLUSTER_ADDR' 	=> $JSON_DECODE[$i] -> CLUSTER_ADDR,
							'CLUSTER_USER' 	=> $JSON_DECODE[$i] -> CLUSTER_USER,
							'TIMESTAMP' 	=> Misc::time_convert_with_zone($JSON_DECODE[$i] -> TIMESTAMP),
							'CLOUD_TYPE'	=> $JSON_DECODE[$i] -> CLOUD_TYPE,
							'VENDOR_TYPE'	=> $JSON_DECODE[$i] -> VENDOR,
							'REGION_NAME'	=> $JSON_DECODE[$i] -> REGION
							);	
		}
		$Response = $LIST_CLOUD;
	}
	else
	{
		$Response = false;
	}
	print_r(json_encode($Response));
	break;
		
		
	############################
	# Verify OpenStack Connection
	############################
	case "VerifyOpenStackConnection":
	
	if ($_REQUEST['PROJECT_NAME'] != '' and $_REQUEST['CLUSTER_USER'] != '' and $_REQUEST['CLUSTER_PASS'] != '' and $_REQUEST['CLUSTER_VIP_ADDR'] != '')
	{	
		$ACTION    = 'VerifyOpenStackConnection';
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ProjectName' 		=> $_REQUEST['PROJECT_NAME'],
						'ClusterUsername' 	=> $_REQUEST['CLUSTER_USER'],
						'ClusterPassword' 	=> $_REQUEST['CLUSTER_PASS'],
						'IdentityProtocol' 	=> $_REQUEST['IDENTITY_PROTOCOL'],
						'ClusterVipAddr' 	=> $_REQUEST['CLUSTER_VIP_ADDR'],
						'IdentityPort' 		=> $_REQUEST['IDENTITY_PORT']
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$output = json_encode(array('Code' => false, 'Msg' => _('Please fill in all of the required fields.')));
	}	
	print_r($output);	
	break;
	
	############################
	# Query Endpoint Information
	############################
	case "QueryEndpointInformation":
		$ACTION    = 'QueryEndpointInformation';
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID'],
						'ProjectName' 		=> $_REQUEST['PROJECT_NAME'],
						'ClusterUsername' 	=> $_REQUEST['CLUSTER_USER'],
						'ClusterPassword' 	=> $_REQUEST['CLUSTER_PASS'],
						'IdentityProtocol' 	=> $_REQUEST['IDENTITY_PROTOCOL'],
						'ClusterVipAddr' 	=> $_REQUEST['CLUSTER_VIP_ADDR'],
						'IdentityPort' 		=> $_REQUEST['IDENTITY_PORT'],
						'ProjectId'			=> $_REQUEST['PROJECT_ID']
					);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);
	break;
	
	############################
	# Edit OpenStack Endpoints
	############################
	case "EditApiEndpoints":
		$ENDPOINT_REF = json_decode($_REQUEST['ENDPOINT_REF_ADDR']);
				
		$ENDPOINT_TABLE = '<style>#container{padding-top:6px;!important; padding-left:11px;!important;}</style>';
		for ($i=0; $i<count($ENDPOINT_REF); $i++)
		{
			$REF_INFO = explode(",",$ENDPOINT_REF[$i]);
			
			$ENDPOINT_TABLE .= '<table id="EndpointTable" style="width:770px;">';
			$ENDPOINT_TABLE .= '<tr>';
			$ENDPOINT_TABLE .= '<td width="133px">'.$REF_INFO[0].'</td>';
			$ENDPOINT_TABLE .= '<td><input id="'.$REF_INFO[0].'" class="form-control" style="height:30px;" value="'.$REF_INFO[1].'"></td>';
			$ENDPOINT_TABLE .= '</tr>';
			$ENDPOINT_TABLE .= '</table>';
		}
		print_r($ENDPOINT_TABLE);
	break;	
	
	############################
	# Initialize New OpenStack Connection
	############################
	case "InitializeNewOpenStackConnection":
	
	$ACTION    = 'InitializeNewOpenStackConnection';
	$REST_DATA = array(
					'Action' 			=> $ACTION,
					'AcctUUID'			=> $_REQUEST['ACCT_UUID'],
					'RegnUUID'			=> $_REQUEST['REGN_UUID'],
					'ProjectName' 		=> $_REQUEST['PROJECT_NAME'],
					'ProjectId' 		=> $_REQUEST['PROJECT_ID'],
					'ClusterUsername' 	=> $_REQUEST['CLUSTER_USER'],
					'ClusterPassword' 	=> $_REQUEST['CLUSTER_PASS'],
					'IdentityProtocol' 	=> $_REQUEST['IDENTITY_PROTOCOL'],
					'ClusterVipAddr' 	=> $_REQUEST['CLUSTER_VIP_ADDR'],
					'IdentityPort' 		=> $_REQUEST['IDENTITY_PORT'],
					'EndpointRefAddr'	=> $_REQUEST['ENDPOINT_REF_ADDR']
				);

	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
	
	
	############################
	# Edit Select Connection
	############################
	case "EditSelectConnection":
	
	$ACTION = 'EditSelectConnection';	
	if (isset($_REQUEST['CLUSTER_UUID']))
	{	
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
					);
	
		$output = CurlPostToRemote($REST_DATA);		
		if ($output == false)
		{
			$Response = array('Code' => false, 'Msg' => _('Cannot get connection information.'));
		}
		else
		{
			$_SESSION['CLUSTER_UUID'] = $_REQUEST['CLUSTER_UUID'];
			
			$CLUSTER_ADDR = json_decode($output, false) -> CLUSTER_ADDR;
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{
				$REDIRECT_TYPE = 'HuaweiCloud';
			}
			else
			{			
				$REDIRECT_TYPE = json_decode($output, false) -> CLOUD_TYPE;
			}
			$Response = array('Code' => true, 'Msg' => $REDIRECT_TYPE);
		}
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a cloud connection.'));
	}
	echo json_encode($Response);	
	break;
	
	
	############################
	# Delete Select Connection
	############################
	case "DeleteSelectConnection":
	
	$ACTION    = 'DeleteSelectConnection';	
	if (isset($_REQUEST['CLUSTER_UUID']))
	{		
	
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
					);
	
		$Response = json_decode(CurlPostToRemote($REST_DATA));
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a cloud connection.'));
	}
	echo json_encode($Response);	
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
	# Update OpenStack Connection
	############################
	case "UpdateOpenStackConnection":
	
	$REST_DATA = array(
					'Action'			=> 'UpdateOpenStackConnection',
					'ClusterUUID'  		=> $_REQUEST['CLUSTER_UUID'],
					'ProjectName' 		=> $_REQUEST['PROJECT_NAME'],
					'ProjectId' 		=> $_REQUEST['PROJECT_ID'],
					'ClusterUsername' 	=> $_REQUEST['CLUSTER_USER'],
					'ClusterPassword' 	=> $_REQUEST['CLUSTER_PASS'],
					'IdentityProtocol' 	=> $_REQUEST['IDENTITY_PROTOCOL'],
					'ClusterVipAddr' 	=> $_REQUEST['CLUSTER_VIP_ADDR'],
					'IdentityPort' 		=> $_REQUEST['IDENTITY_PORT'],
					'EndpointRefAddr'	=> $_REQUEST['ENDPOINT_REF_ADDR']
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
					'HostUUID'		=> $_REQUEST['HOST_UUID']									
				);

	$output = CurlPostToRemote($REST_DATA);
	
	echo $output;	
	break;
			
	
	############################
	# LIST AVAILABLE OPENSTACK DISK
	############################
	case "ListAvailableOpenStackDisk":
	
	$ACTION    = 'ListAvailableOpenStackDisk';
	$REST_DATA = array('Action'   => $ACTION,
					   'ReplUUID' => $_REQUEST['REPL_UUID']
				);
	
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);	
	break;
	
	############################
	# QUERY VOLUME INFORMATION
	############################
	case "QueryVolumeDetailInfo":
	
	$ACTION    = 'QueryVolumeDetailInfo';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'DiskUUID' 	 => $_REQUEST['DISK_UUID']
				);	
	
	$output = CurlPostToRemote($REST_DATA);
	
	$volume_detail = json_decode($output);
	
	$volume_info = explode('@',$volume_detail -> volume_name);
	
	$volume_detail -> volume_name = Misc::convert_snapshot_time_with_zone($volume_info[0], $volume_info[1]);
	
	print_r(json_encode($volume_detail));
	
	break;	
	
	############################
	# LIST AVAILABLE SNAPSHOT
	############################
	case "ListAvailableSnapshot":
	
	$ACTION    = 'ListAvailableSnapshot';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'DiskUUID' 	 => $_REQUEST['DISK_UUID']
				);	

	$output = CurlPostToRemote($REST_DATA);
	
	if ($output != 'false')
	{
		#RE-FORMAT SNAPSHOT ARRAY LIST
		$snapshots = array_reverse(json_decode($output));
		for ($i=0; $i<count($snapshots); $i++)
		{
			$description = Misc::convert_snapshot_time_with_zone($snapshots[$i] -> description, $snapshots[$i] -> created_at);
			
			$FormatOutput[] = array(
								'status' 		=> $snapshots[$i] -> status,
								'metadata' 		=> $snapshots[$i] -> metadata,							
								'name'			=> $snapshots[$i] -> name,
								'volume_id'		=> $snapshots[$i] -> volume_id,
								'created_at'	=> $snapshots[$i] -> created_at,
								'size'			=> $snapshots[$i] -> size,
								'id'			=> $snapshots[$i] -> id,
								'description'	=> $description
							);			
		}
		$output = json_encode($FormatOutput);
	}

	print_r($output);	
	break;
	
	
	############################
	# QUERY SNAPSHOT INFORMATION
	############################
	case "QuerySnapshotInformation":
	$ACTION    = 'QuerySnapshotInformation';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'SnapUUID' 	 => $_REQUEST['SNAP_UUID'],
					   'ReplicaDisk' => $_REQUEST['REPLICA_DISK']
				);
	
	$output = CurlPostToRemote($REST_DATA);

	if ($output != FALSE)
	{
		#RE-FORMAT SNAPSHOT ARRAY
		$snapshots = json_decode($output);
		for ($i=0; $i<count($snapshots); $i++)
		{
			$description = Misc::convert_snapshot_time_with_zone($snapshots[$i] -> snapshot -> description, $snapshots[$i] -> snapshot -> created_at);
				
			$FormatOutput[]['snapshot'] = (object)array(
											'status' 		=> $snapshots[$i] -> snapshot -> status,
											'metadata' 		=> $snapshots[$i] -> snapshot -> metadata,
											'name'			=> $snapshots[$i] -> snapshot -> name,
											'progress'		=> $snapshots[$i] -> snapshot -> {'os-extended-snapshot-attributes:progress'},
											'volume_id'		=> $snapshots[$i] -> snapshot -> volume_id,
											'created_at'	=> $snapshots[$i] -> snapshot -> created_at,
											'size'			=> $snapshots[$i] -> snapshot -> size,
											'id'			=> $snapshots[$i] -> snapshot -> id,
											'description'	=> $description
										);			
		}
		$output = json_encode($FormatOutput);
	}
	print_r($output);	
	break;
	
	############################
	# QUERY VOLUME INFORMATION
	############################
	case "QueryVolumeInformation":
	$ACTION    = 'QueryVolumeInformation';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'ReplUUID'	 => $_REQUEST['REPL_UUID'],
					   'VolumeUUID'  => $_REQUEST['VOLUME_UUID']
				);
	
	$output = CurlPostToRemote($REST_DATA);

	if ($output != FALSE)
	{
		#RE-FORMAT SNAPSHOT ARRAY
		$volume = json_decode($output);
		for ($i=0; $i<count($volume); $i++)
		{
			$FormatOutput[]['volume'] = (object)array(
											'cloud_disk_id'	  => $volume[$i] -> volume_id,
											'cloud_disk_name' => $volume[$i] -> volume_name,
											'created_at'	  => Misc::convert_volume_time_with_zone(explode("@",$volume[$i] -> volume_name)[1]),
											'size'			  => $volume[$i] -> volume_size
										);			
		}
		$output = json_encode($FormatOutput);
	}
	print_r($output);	
	break;
	
	
	############################
	# List Instance Flavors
	############################
	case "ListInstanceFlavors":
	
	$ACTION = 'ListInstanceFlavors';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# List Instance Network
	############################
	case "ListInternalNetwork":
	
	$ACTION = 'ListInternalNetwork';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# List Subnet Network
	############################
	case "ListSubnetNetworks":
	
	$ACTION = 'ListSubnetNetworks';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
		
	############################
	# List Available Network
	############################
	case "ListAvailableFloatingIP":
	
	$ACTION = 'ListAvailableFloatingIP';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# List Security Group
	############################
	case "ListSecurityGroup":
	
	$ACTION = 'ListSecurityGroup';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# List DataMode Instances
	############################
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
	
	############################
	# Query Flavor Information
	############################
	case "QueryFlavorInformation":
	
	$ACTION = 'QueryFlavorInformation';
	$REST_DATA = array('Action'   => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'FlavorID' => $_REQUEST['FLAVOR_ID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# Query Network Information
	############################
	case "QueryNetworkInformation":
	
	$ACTION = 'QueryNetworkInformation';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'NetworkUUID' => $_REQUEST['NETWORK_UUID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# Query Floating Ip Information
	############################
	case "QueryFloatingIpInformation":
	
	$ACTION = 'QueryFloatingIpInformation';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'FloatingID'  => $_REQUEST['FLOATING_ID']);
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);	
	break;
	
	############################
	# Query Subnet Information
	############################
	case "QuerySubnetInformation":
	
	$ACTION = 'QuerySubnetInformation';
	$REST_DATA = array('Action'   	=> $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'SubnetUUID' => $_REQUEST['SUBNET_UUID']);

	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# Query Security Group Information
	############################
	case "QuerySecurityGroupInformation":
	
	$ACTION = 'QuerySecurityGroupInformation';
	$REST_DATA = array('Action'   	=> $ACTION,
					   'ClusterUUID' => $_REQUEST['CLUSTER_UUID'],
					   'SgroupUUID' => $_REQUEST['SGROUP_UUID']);

	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
}