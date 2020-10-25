<?php
$GetPostData = file_get_contents("php://input");
$PostData = json_decode($GetPostData,true);

function procPromoteTransport($ConnectionString, $PostData, $AzureClient, $cloudId ){

	if( isset( $PostData['onpremisesTransport'] ) && ($PostData['onpremisesTransport'] != "0") ){

		$AzureModel = new Common_Model();

		$pre = strpos( $ConnectionString, "AccountName=");
		$post = strpos( $ConnectionString, ";", $pre);
		$accNmae = substr( $ConnectionString, $pre+strlen("AccountName="),$post - $pre - strlen("AccountName=") );

		$accDetail = $AzureClient->GetStorageAccountDetail( $accNmae );

		$a = $AzureModel->getMachineId( $PostData['onpremisesTransport'] );

		$serverInfo = json_decode( $a["serverInfo"], true );

		$id = explode( '/', $accDetail["id"] );
		$serverInfo["rg"] = $id[4];
		$serverInfo["location"] = $accDetail["location"];
		$serverInfo["is_promote"] = true;

		$AzureModel->updateServerInfo( $cloudId, $a["machineId"], json_encode( $serverInfo ) );

		$AzureModel->updateServerConn( $cloudId, $PostData['onpremisesTransport'], json_encode( $serverInfo ) );

	}
}

function cancelPromoteTransport( $cloudId ){

	$AzureModel = new Common_Model();

	$server = $AzureModel->getAzurePromoteTransport( $cloudId );

	if( count($server) == 0 )
		return;

	$serverInfo = json_decode( $server[0]["serverInfo"], true );

	unset($serverInfo["rg"]);

	unset($serverInfo["location"]);

	unset($serverInfo["is_promote"]);

	$AzureModel->updateServerInfo( "", $serverInfo["machine_id"], json_encode( $serverInfo ), "LOCAL_HOST" );

	$AzureModel->updateServerConn( "", $server[0]['serverId'], json_encode( $serverInfo ) );
}

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
		$AzureClient = new Azure_Controller();
		$AzureModel = new Common_Model();
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
		# Verify Azure Access Credential
		############################
		case "VerifyAzureAccessCredential":
		
			//$USER_NAME 			= $PostData['UserName'];
			//$PASSWORD 			= $PostData['Password'];
			$TENANT_ID 			= $PostData['TenantId'];
			//$SUBSCRIPTION_ID 	= $PostData['SubscriptionId'];
			$ACCESS_KEY 		= $PostData['AccessKey'];
			$SECRET_KEY 		= $PostData['SecretKey'];
			$EndpointType 		= $PostData['Endpoint'];
			$ConnectionString	= $PostData['ConnectionString'];

			$AzureClient->DefaultConfig( $TENANT_ID, '' , $ACCESS_KEY, $SECRET_KEY );
				
			$AzureClient->SetEndpointType( $EndpointType );
			try{
				$Result = $AzureClient -> ListSubscription();
	
				if ($Result != false)
				{
					if( $ConnectionString != "" )
					{
						$ServiceMgmt = new Service_Class();
						$BlobConnCheck = $ServiceMgmt -> verify_connection_string($ConnectionString);
					}
					else
					{
						$BlobConnCheck = true;
					}
					
					if ($BlobConnCheck == TRUE)
					{					
						print_r( $Result );
					}
					else
					{
						echo json_encode(false);
					}
				}
				else
				{
					echo json_encode(false);
				}
			} catch (Exception $e) {
				echo json_encode(false);
			}
		break;

		
		############################
		# Initialize New Azure Connection
		############################
		case "InitializeNewAzureConnection":
			$ACCT_UUID  		= $PostData['AcctUUID'];
			$REGN_UUID  		= $PostData['RegnUUID'];
			//$USER_NAME 			= $PostData['UserName'];
			//$PASSWORD 			= $PostData['Password'];
			$TENANT_ID 			= trim($PostData['TenantId']);
			$SUBSCRIPTION_ID 	= trim($PostData['SubscriptionId']);
			$ACCESS_KEY 		= trim($PostData['AccessKey']);
			$SECRET_KEY 		= trim($PostData['SecretKey']);
			$EndpointType 		= trim($PostData['Endpoint']);
			$ConnectionString	= trim($PostData['ConnectionString']);
			$SUBSCRIPTION_NAME  = "";
			
			if( $PostData['SubscriptionId'] )
				$SUBSCRIPTION_NAME 	= trim($PostData['SubscriptionName']);
			
			$AzureClient->DefaultConfig( $TENANT_ID, $SUBSCRIPTION_ID ,$ACCESS_KEY, $SECRET_KEY );
			
			$AzureClient->SetEndpointType( $EndpointType );
			try{
				if ( $AzureClient -> ListLocation() != false)	{
					
					$auth_data = array(
									//"USER_NAME" => $USER_NAME,
									//"PASSWORD"  => $PASSWORD,
									"TENANT_ID" => $TENANT_ID,
									"SUBSCRIPTION_ID" => $SUBSCRIPTION_ID,
									"CONNECTION_STRING" => $ConnectionString,
									"SUBSCRIPTION_NAME" => $SUBSCRIPTION_NAME
					);
					
					$secret_data = array(
									"ACCESS_KEY" => $ACCESS_KEY,
									"SECRET_KEY" => $SECRET_KEY
					);
					
					$cloudId = $AzureModel -> create_cloud_connection($ACCT_UUID,$REGN_UUID,json_encode( $auth_data ),json_encode( $secret_data ),$AzureClient->GetTokenInfo(), $EndpointType);			
	
					procPromoteTransport( $ConnectionString, $PostData, $AzureClient, $cloudId );

					print_r(json_encode(array('Code' => true, 'Msg' => 'New Azure connection added.')));
				}
				else
					echo json_encode(false);
			} catch (Exception $e) {
				return '';
			}
		break;
		
		
		############################
		# Query Access Credential Information
		############################
		case "QueryAccessCredentialInformation":
		
			$CloudConnection = $AzureModel -> query_cloud_connection_information($PostData['ClusterUUID']);
			
			$data1 = json_decode( $CloudConnection["ACCESS_KEY"], true );
			
			$data2 = json_decode( $CloudConnection["SECRET_KEY"], true );
			
			$ret = array(
				"ACCESS_KEY" 		=> $data2["ACCESS_KEY"],
				"SECRET_KEY" 		=> $data2["SECRET_KEY"], 
				"TENANT_ID" 		=> $data1["TENANT_ID"],
				"SUBSCRIPTION_ID" 	=> $data1["SUBSCRIPTION_ID"],
				"SUBSCRIPTION_NAME"	=> ( isset( $data1["SUBSCRIPTION_NAME"] ))?$data1["SUBSCRIPTION_NAME"]:"",
				"CONNECTION_STRING"	=> ( isset( $data1["CONNECTION_STRING"] ))?$data1["CONNECTION_STRING"]:"",
				"ENDPOINT" 			=> $CloudConnection["DEFAULT_ADDR"]
			);
			
			print_r(json_encode($ret));		
		break;
		
		
		############################
		# Update Azure Connection
		############################
		case "UpdateAzureConnection":
			$CLUSTER_UUID  		= $PostData['ClusterUUID'];
			$ACCESS_KEY 		= trim($PostData['AccessKey']);
			$SECRET_KEY 		= trim($PostData['SecretKey']);
			$TENANT_ID 			= trim($PostData['TenantId']);
			$SUBSCRIPTION_ID 	= trim($PostData['SubscriptionId']);
			$EndpointType 		= trim($PostData['Endpoint']);
			$ConnectionString	= trim($PostData['ConnectionString']);
			$SUBSCRIPTION_NAME  = "";

			if( $PostData['SubscriptionId'] )
				$SUBSCRIPTION_NAME 	= trim($PostData['SubscriptionName']);
			
			$auth_data = array(
				//"USER_NAME" => $USER_NAME,
				//"PASSWORD"  => $PASSWORD,
				"TENANT_ID" => $TENANT_ID,
				"SUBSCRIPTION_ID" => $SUBSCRIPTION_ID,
				"CONNECTION_STRING" => $ConnectionString,
				"SUBSCRIPTION_NAME" => $SUBSCRIPTION_NAME
			);
			
			$secret_data = array(
				"ACCESS_KEY" => $ACCESS_KEY,
				"SECRET_KEY" => $SECRET_KEY
			);
			
			$AzureClient->SetEndpointType( $EndpointType );
			
			$TokenInfo = $AzureClient -> GetNewOAuthTokenInfo( $TENANT_ID, $ACCESS_KEY, $SECRET_KEY, $SUBSCRIPTION_ID);
			
			if ( $AzureClient -> ListLocation() != false)	{
				
				$AzureModel -> update_cloud_connection($CLUSTER_UUID,json_encode( $auth_data ),json_encode( $secret_data ),$TokenInfo);
				
				cancelPromoteTransport( $CLUSTER_UUID );
				
				procPromoteTransport( $ConnectionString, $PostData, $AzureClient, $CLUSTER_UUID );

				print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update Azure connection.')));
			}
			else
				echo json_encode(false);
				
		break;
		
		
		############################
		# List Installed Instances
		############################
		case "ListInstalledInstances":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			
			$ListAllInstances = $AzureClient -> describe_all_instances($CLOUD_UUID);
			
			echo json_encode( $ListAllInstances );
		break;
		
		case "ListInstancesInResourceGroup":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$ServerId = $PostData['ServerId'];
			
			$AzureClient->getVMByServUUID( $ServerId, $CLOUD_UUID );
			
			$ListAllInstances = $AzureClient->getVMInResourceGroup( $CLOUD_UUID );
			
			echo json_encode( $ListAllInstances );
		break;
		
		
		############################
		# Query Selected Host Information
		############################
		case "QuerySelectedHostInformation":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_NAME  = $PostData['HostName'];
			$HOST_REGN  = $PostData['HostREGN'];
			$HOST_RG    = $PostData['HostRG'  ];
			if( isset( $PostData['SERV_UUID'] ))
				$SERV_UUID	= $PostData['SERV_UUID'];

			if( $HOST_RG == "" ) {

				$vm = $AzureClient->getVMByServUUID( $SERV_UUID, $CLOUD_UUID);

				$HOST_RG = $vm["ret"]['resource_group'];
			}

			$ListInstance = $AzureClient->describe_instance($CLOUD_UUID, $HOST_REGN, $HOST_NAME, $HOST_RG);
			
			print_r(json_encode($ListInstance));
		break;
		
		############################
		# Query Transport Information
		############################
		case "QueryTransportInformation":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$VMName  = $PostData['VMName'];
			$VMREGN  = $PostData['VMREGN'];
			if( isset( $PostData['SERV_UUID'] ))
				$SERV_UUID	= $PostData['SERV_UUID'];

			if( !isset( $HOST_RG ) || $HOST_RG == "" ) {

				$vm = $AzureClient->getVMByServUUID( $SERV_UUID, $CLOUD_UUID);

				$HOST_RG = $AzureClient->GetResourceGroup();
			}
			
			$ret = array(
				"success" => true,
				"Resource_group" => $HOST_RG);
			//$ListInstance = $AzureClient->describe_instance($CLOUD_UUID, $HOST_REGN, $HOST_NAME, $HOST_RG);

			print_r(json_encode($ret));
		break;

		
		############################
		# List Available Ebs Snapshot
		############################
		case "ListAvailableDiskSnapshot":
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_REGN  = $PostData['ServZone'];
			$VOLUME_ID  = $PostData['DiskID'];
			
			$ListSnapshots = $AzureClient -> describe_snapshots($CLOUD_UUID,$HOST_REGN,$VOLUME_ID);
		
			print_r(json_encode($ListSnapshots));
		break;
			
		############################
		# List Instance Flavors
		############################
		case "ListInstanceFlavors":
				
			$CLOUD_UUID   	= $PostData['ClusterUUID'];
			$ZONE 			=  $PostData['SERV_REGN'];
			
			$ListFlavors = $AzureClient -> describe_instance_types( $CLOUD_UUID, $ZONE );
			
			print_r(json_encode($ListFlavors));
		break;
		
		
		############################
		# List Available Network
		############################
		case "ListAvailableNetwork":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
				
			//$ListNetworks = $AzureClient -> describe_available_network($CLOUD_UUID);
			$ListNetworks = $AzureClient -> describe_internal_network($CLOUD_UUID, $SELECT_ZONE);
			
			print_r(json_encode($ListNetworks, JSON_UNESCAPED_SLASHES));
		break;
		
		
		############################
		# List Security Group
		############################
		case "ListSecurityGroup":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SERVER_ZONE  = $PostData['ServerZONE'];
				
			$SecurityGroup = $AzureClient -> describe_security_groups($CLOUD_UUID, $SERVER_ZONE);
			
			print_r(json_encode($SecurityGroup));
		break;
		
		############################
		# Query Flavor Information
		############################
		case "QueryFlavorInformation":
			$FLAVOR_ID  = $PostData['FlavorId'];
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$ZONE = $PostData['SELECT_ZONE'];
				
			$InstanceTypeInfo = $AzureClient -> describe_instance_types( $CLOUD_UUID, $ZONE, $FLAVOR_ID );
			
			print_r(json_encode($InstanceTypeInfo));
		break;
		
		
		############################
		# Query Network Information
		############################
		case "QueryNetworkInformation":
			$CLOUD_UUID   = $PostData['ClusterUUID'];
			$SELECT_ZONE  = $PostData['SelectZONE'];
			$NETWORK_UUID = $PostData['NetworkUUID'];
				
			//$SelectNetwork = $AzureClient -> describe_available_network($CLOUD_UUID,$NETWORK_UUID);
			$SelectNetwork = $AzureClient -> describe_internal_network($CLOUD_UUID, $SELECT_ZONE, $NETWORK_UUID);
			
			print_r(json_encode($SelectNetwork, JSON_UNESCAPED_SLASHES));
		break;
		
		
		############################
		# Query Security Group Information
		############################
		case "QuerySecurityGroupInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SGROUP_UUID = $PostData['SGroupUUID'];
				
			$SelectSecurityGroup = $AzureClient -> describe_security_groups($CLOUD_UUID, $SELECT_ZONE, $SGROUP_UUID);
			
			print_r(json_encode($SelectSecurityGroup));
		break;
		
		############################
		# Query Snapshot Information
		############################
		case "QuerySnapshotInformation":
			$CLOUD_UUID  = $PostData['ClusterUUID'];
			$SELECT_ZONE = $PostData['SelectZONE'];
			$SNAPSHOT_ID = $PostData['SnapshotID'];
			$ReplicaUUID = $PostData['ReplicaUUID'];
			$ReplicaDisk = $PostData['ReplicaDisk'];
			$BlobMode	 = $PostData['BlobMode'];
			
			if ($BlobMode == 'true')
			{
				$AzureBlobMgmt = new Azure_Blob_Action_Class();
				
				$SelectSnapshotInfo = $AzureBlobMgmt -> get_snapshot_info($ReplicaUUID,$SNAPSHOT_ID);
			}
			else
			{	
				if ($SNAPSHOT_ID == 'UseLastSnapshot')
				{
					$REPLICA_DISK = explode(',',$ReplicaDisk);
					for ($i=0; $i<count($REPLICA_DISK); $i++)
					{
						$SelectSnapshotInfo[] = $AzureClient->describe_snapshots($CLOUD_UUID, $SELECT_ZONE, $REPLICA_DISK[$i])[0];						
					}	
					print_r(json_encode($SelectSnapshotInfo, JSON_UNESCAPED_SLASHES));
					return;
				}
				
				$SNAP_ID = explode(',',$SNAPSHOT_ID);
				for ($i=0; $i<count($SNAP_ID); $i++)
				{
					$SelectSnapshotInfo[] = $AzureClient -> describe_snapshot_detail($CLOUD_UUID,$SELECT_ZONE,$SNAP_ID[$i]);
				}
			}			
			print_r(json_encode($SelectSnapshotInfo, JSON_UNESCAPED_SLASHES));
		break;
		
		case "QueryEndpoint":
			
			$azureInfo = Misc_Class::define_mgmt_setting();

			print_r(json_encode($azureInfo->azure_enpoint));
		break;
		
		case "CheckPrivateIp":
			
			$CLOUD_UUID   = $PostData['CLUSTER_UUID'];
			$NETWORK_UUID = $PostData['NETWORK_UUID'];
			$PRIVATE_IP   = $PostData['PRIVATE_IP'];
			
			$ret = explode( '|', $NETWORK_UUID );
			
			$virtualNetwork = $ret[0];
			
			$resourceGroup = $ret[2];
			
			$AzureClient->SetResourceGroup( $resourceGroup );
			
			$result = $AzureClient->CheckPrivateIp( $CLOUD_UUID, $virtualNetwork, $PRIVATE_IP );
			
			print_r( $result );
		break;
		
		case "ListAzureBlobSnapshot":
			
			$CLOUD_UUID		= $PostData['CLUSTER_UUID'];
			$CONTAINER 		= $PostData['CONTAINER'];
			$DISK_ID		= $PostData['DISK_ID'];
			
			$AzureBlobCient = new Azure_Blob_Action_Class();
			
			$DISK_SNAPSHOT = $AzureBlobCient -> list_snapshot($CONTAINER,$DISK_ID);
			
			print_r(json_encode($DISK_SNAPSHOT));			
		break;
		
		case "setPublicIpStatic":
		
			$CLOUD_UUID = $PostData['ClusterUUID'];
			$HOST_NAME  = $PostData['HostName'];
			$HOST_REGN  = $PostData['HostREGN'];
			$HOST_RG    = $PostData['HostRG'  ];
			
			if( isset( $PostData['SERV_UUID'] ))
				$SERV_UUID	= $PostData['SERV_UUID'];

			if( $HOST_RG == "" ) {

				$vm = $AzureClient->getVMByServUUID( $SERV_UUID, $CLOUD_UUID);

				$HOST_RG = $vm["ret"]['resource_group'];
			}
			$ListInstance = $AzureClient->setPublicIpStatic($CLOUD_UUID, $HOST_REGN, $HOST_NAME, $HOST_RG);
			
			print_r(json_encode($ListInstance));
		break;
		
		case "ListAvailabilitySet":
		
			$CLOUD_UUID 	= $PostData['ClusterUUID'];
			$Region  		= $PostData['SelectZONE'];
			$RESOURCE_GROUP	= $PostData['RESOURCE_GROUP'];

			$ListInstance = $AzureClient->ListAvailabilitySet( $CLOUD_UUID, $RESOURCE_GROUP );
			
			print_r(json_encode($ListInstance));
		break;
		
		case "QueryVolumeDetailInfo":
		
			$vm = $AzureClient->getVMByServUUID( $PostData['ServerId'], $PostData['ClusterUUID']);
			
			$diskInfo = $AzureClient->GetDisksDetail( $PostData['DiskUUID'] );
	
			echo ($diskInfo);
		break;
		
		case "ListDataModeInstances":
		
			$GetVmDetailList = $AzureClient->listDataModeInstance($PostData['ClusterUUID'],$PostData['FilterUUID']);
			
			print_r(json_encode($GetVmDetailList));
		break;
	}
}

