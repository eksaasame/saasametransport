<?php
$GetPostData = file_get_contents("php://input");
$PostData = json_decode($GetPostData,true);

/* Secure Key Identification */
$EncryptKey = $PostData['EncryptKey'];
$KeyDecrypt = Misc_Class::encrypt_decrypt('decrypt', $EncryptKey);
$DefineTime = time();
$MinAllowed = $DefineTime - 15;
$MaxAllowed = $DefineTime + 15;
if ($MinAllowed < $KeyDecrypt AND $KeyDecrypt < $MaxAllowed)
{
	$SelectAction = $PostData['Action'];
	$ProviderClient = new Tencent_Controller();
	$ProviderModel = new Tencent_Model();
}
else
{
	print_r(json_encode(false));
	exit;	
}

if (isset($SelectAction))
{
	switch ($SelectAction)
	{
		############################
		# Verify Tencent Access Credential
		############################
		case "VerifyTencentAccessCredential":
			$ACCESS_KEY 		= $PostData['AccessKey'];
			$SECRET_KEY 		= $PostData['SecretKey'];
			$APP_ID 			= $PostData['AppId'];
			
			$ProviderClient->SetAppId( $APP_ID );
			$ProviderClient->SetKey( $ACCESS_KEY, $SECRET_KEY );
			$Result = $ProviderClient->CheckConnect();
			
			$ret = json_encode( $Result );
			if ($Result != false)
			{
				echo json_encode(true);
			}
			else
			{
				echo json_encode(false);
			}
		break;

		
		############################
		# Initialize New Tencent Connection
		############################
		case "InitializeNewTencentConnection":
		
			$ACCT_UUID  		= $PostData['AcctUUID'];
			$REGN_UUID  		= $PostData['RegnUUID'];
		
			$ACCESS_KEY 		= $PostData['AccessKey'];
			$SECRET_KEY 		= $PostData['SecretKey'];
			$APP_ID 			= $PostData['AppId'];
			
			$ProviderClient->SetAppId( $APP_ID );
			
			$ProviderClient->SetKey( $ACCESS_KEY, $SECRET_KEY );

			if ( $ProviderClient->CheckConnect() != false)	{

				$auth_data = $ACCESS_KEY;
				
				$secret_data = $SECRET_KEY;

				$ProviderModel->create_cloud_connection($ACCT_UUID,$REGN_UUID, $auth_data , $secret_data , $APP_ID);			

				print_r(json_encode(array('Code' => true, 'Msg' => 'New Tencent cloud connection added.')));
			}
			else
				echo json_encode(false);
		break;
		
		
		############################
		# Query Access Credential Information
		############################
		case "QueryAccessCredentialInformation":
		
			$CloudConnection = $ProviderModel -> query_cloud_connection_information($PostData['ClusterUUID']);
			
			$ret = array(
				"ACCESS_KEY" => $CloudConnection["ACCESS_KEY"],
				"SECRET_KEY" => $CloudConnection["SECRET_KEY"],
				"APP_ID"	 => $CloudConnection["USER_UUID"]
			);
			
			print_r(json_encode($ret));		
		break;
		
		
		############################
		# Update Tencent Connection
		############################
		case "UpdateTencentConnection":
			$CLUSTER_UUID  		= $PostData['ClusterUUID'];
			$ACCESS_KEY 		= $PostData['AccessKey'];
			$SECRET_KEY 		= $PostData['SecretKey'];
			$APP_ID 			= $PostData['AppId'];
			
			$ProviderClient->SetAppId( $APP_ID );
			
			$ProviderClient->SetKey( $ACCESS_KEY, $SECRET_KEY );

			if ( $ProviderClient -> CheckConnect() != false)	{
				
				$ProviderModel -> update_cloud_connection($CLUSTER_UUID, $ACCESS_KEY, $SECRET_KEY, $APP_ID);
				
				print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update Tencent connection.')));
			}
			else
				echo json_encode(false);
				
		break;
		
		
		############################
		# List Installed Instances
		############################
		case "ListInstalledInstances":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			
			$ListAllInstances = $ProviderClient->describe_all_instances($CLOUD_UUID);
			
			echo json_encode( $ListAllInstances, JSON_UNESCAPED_SLASHES );
		break;
		
		
		############################
		# Query Selected Host Information
		############################
		case "QuerySelectedHostInformation":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_UUID  = $PostData['HostUUID'];
			$HOST_REGN  = $PostData['HostREGN'];
			//$SERV_UUID	= $PostData['SERV_UUID'];

			$ListInstance = $ProviderClient->describe_instance($CLOUD_UUID, $HOST_REGN, $HOST_UUID);
			
			print_r(json_encode($ListInstance));
		break;

		
		############################
		# List Available Ebs Snapshot
		############################
		case "ListAvailableDiskSnapshot":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_REGN  = $PostData['ServZone'];
			$VOLUME_ID  = $PostData['DiskID'];
			$_SESSION['timezone']  = $PostData['TimeZone'];

			$ListSnapshots = $ProviderClient -> describe_snapshots($CLOUD_UUID,$HOST_REGN,$VOLUME_ID);
		
			print_r(json_encode($ListSnapshots));
		break;
			
		############################
		# List Instance Flavors
		############################
		case "ListInstanceFlavors":
				
			$CLOUD_UUID   	= $PostData['ClusterUUID'];
			$ZONE 			=  $PostData['SERV_REGN'];

			$ListFlavors = $ProviderClient -> describe_zone_instance_types( $CLOUD_UUID, $ZONE );

			print_r(json_encode($ListFlavors));
		break;
		
		
		############################
		# List Available Network
		############################
		case "ListAvailableNetwork":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];

			//$ListNetworks = $ProviderClient -> describe_available_network($CLOUD_UUID);
			$ListNetworks = $ProviderClient -> describe_internal_network($CLOUD_UUID, $SELECT_ZONE);

			print_r(json_encode($ListNetworks, JSON_UNESCAPED_SLASHES));
		break;
		
		
		############################
		# List Security Group
		############################
		case "ListSecurityGroup":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
				
			$SecurityGroup = $ProviderClient -> describe_security_groups($CLOUD_UUID, $SERVER_ZONE);
			
			print_r(json_encode($SecurityGroup));
		break;
		
		############################
		# Query Flavor Information
		############################
		case "QueryFlavorInformation":
			$FLAVOR_ID  = $PostData['FlavorId'];
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$ZONE = $PostData['SELECT_ZONE'];
				
			$InstanceTypeInfo = $ProviderClient -> describe_instance_types( $CLOUD_UUID, $ZONE, $FLAVOR_ID );
			
			print_r(json_encode($InstanceTypeInfo));
		break;
		
		
		############################
		# Query Network Information
		############################
		case "QueryNetworkInformation":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SELECT_ZONE  = $PostData['SelectZONE'];
			$NETWORK_UUID = $PostData['NetworkUUID'];
				
			//$SelectNetwork = $ProviderClient -> describe_available_network($CLOUD_UUID,$NETWORK_UUID);
			$SelectNetwork = $ProviderClient -> describe_internal_network($CLOUD_UUID, $SELECT_ZONE, $NETWORK_UUID);
			
			print_r(json_encode($SelectNetwork, JSON_UNESCAPED_SLASHES));
		break;
		
		
		############################
		# Query Security Group Information
		############################
		case "QuerySecurityGroupInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SGROUP_UUID = $PostData['SGroupUUID'];
				
			$SelectSecurityGroup = $ProviderClient -> describe_security_groups($CLOUD_UUID, $SELECT_ZONE, $SGROUP_UUID);
			
			print_r(json_encode($SelectSecurityGroup));
		break;
		
		############################
		# Query Snapshot Information
		############################
		case "QuerySnapshotInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SNAPSHOT_ID = $PostData['SnapshotID'];
			
			$SNAP_ID = explode(',',$SNAPSHOT_ID);
			for ($i=0; $i<count($SNAP_ID); $i++)
			{
				$SelectSnapshotInfo[] = $ProviderClient -> describe_snapshot_detail($CLOUD_UUID,$SELECT_ZONE,$SNAP_ID[$i]);
			}
			print_r(json_encode($SelectSnapshotInfo, JSON_UNESCAPED_SLASHES));
		break;
	}
}

