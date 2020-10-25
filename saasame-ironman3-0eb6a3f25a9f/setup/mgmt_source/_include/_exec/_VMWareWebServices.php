<?php
$GetPostData = file_get_contents("php://input");
$PostData = json_decode($GetPostData,true);

use saasame\transport\invalid_operation;

function cancelPromoteTransport( $cloudId ){

	$model = new Common_Model();

	$server = $model->getVMwarePromoteTransport( $cloudId );

	if( count($server) == 0 )
		return;

	$serverInfo = json_decode( $server[0]["serverInfo"], true );

	$model->updateServerInfo( "", $serverInfo["machine_id"], false, "LOCAL_HOST" );

	$model->updateServerConn( "", $server[0]['serverId'] );
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

		$VMWareClient = new VM_Ware();

		$CommonModel = new Common_Model();
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

function getEsxInformation( $tansportIp, $EsxIp, $username, $password, $serverId ){
	
	$VMWareClient = new VM_Ware();

	
	$CommonModel = new Common_Model();
	
	$TransportInfo = $CommonModel->getTransportInfo( $serverId );
	
	$hosts_info = (array)$VMWareClient->getVirtualHosts( $TransportInfo["ConnectAddr"], $EsxIp, $username, $password );

	if( isset( $hosts_info["success"] ) && $hosts_info["success"] == false )
		return json_encode( $hosts_info );
	
	foreach( $hosts_info as $key => $host_info ){
		
		$tmp = json_decode( json_encode($host_info), true );
		
		$license = reset( $tmp["lic_features"] );
		
		$keywords = array('vSphere API','Storage APIs','vStorage APIs');
	
		if( $host_info->virtual_center_version == "" && count( array_intersect($license,$keywords) ) == 0 ){
			
			array_splice($hosts_info, $key, 1);
			
			continue;
		}
		
		$folder_path = (array)$VMWareClient->getDatacenterFolderList( $TransportInfo["ConnectAddr"], $EsxIp, $username, $password, $host_info->datacenter_name );
		
		$host_info->folder_path = $folder_path;
	}

	if( count( $hosts_info ) == 0 ){
		$ret = array(
			"success" 	=> false,
			"format" 	=> "The ESX license API does not meet the requirement.",
			"arguments" => array(""),
			"why" 		=> ""
		);
		
		return json_encode( $ret );
	}
				
	$ret = array("success"=>true, "EsxInfo" => $hosts_info);

	return json_encode($ret);
}

if (isset($SelectAction))
{
	switch ($SelectAction)
	{		
		############################
		# Initialize New Azure Connection
		############################
		case "InitializeNewConnection":
				 
			$ACCT_UUID  		= $PostData['AcctUUID'];
			$REGN_UUID  		= $PostData['RegnUUID'];
			$INPUT_HOST_IP		= trim($PostData['INPUT_HOST_IP']);
			$INPUT_HOST_USER 	= trim($PostData['INPUT_HOST_USER']);
			$INPUT_HOST_PASS 	= trim($PostData['INPUT_HOST_PASS']);
			$TRANSPORT_UUID		= trim($PostData['TRANSPORT_UUID']);
			
			$auth_data = array(
							"INPUT_HOST_USER" => $INPUT_HOST_USER
			);
			
			$secret_data = array(
							"INPUT_HOST_PASS" => $INPUT_HOST_PASS
			);
			
			$cloudId = $CommonModel->create_cloud_connection($ACCT_UUID,$REGN_UUID,json_encode( $auth_data ),json_encode( $secret_data ),"", $INPUT_HOST_IP, $TRANSPORT_UUID, 7);

			$a = $CommonModel->getMachineId( $TRANSPORT_UUID );
			
			$CommonModel->updateServerInfo( $cloudId, $a["machineId"] );
			
			$CommonModel->updateServerConn( $cloudId, $TRANSPORT_UUID );
	
			print_r(json_encode(array('Code' => true, 'Msg' => 'New VMWare connection added.')));

		break;
		
		case "TestEsxConnection":
			$EsxInfo = getEsxInformation( $PostData['TRANSPORT_IP'], $PostData['ESX_ADDR'], $PostData['ESX_USER'], $PostData['ESX_PASS'], $PostData['SERVER_ID'] );				
			
			print_r( $EsxInfo );
		break;
		
		case "getVMWareStorage":
		
			$AuthInfo = $VMWareClient->getESXAuthInfo( $PostData['LAUN_UUID'] );

			$EsxInfo = getEsxInformation( $AuthInfo["ConnectAddr"], $AuthInfo["EsxIp"], $AuthInfo["Username"], $AuthInfo["Password"], $PostData['LAUN_UUID'] );
			
			print_r($EsxInfo);

		break;
		
		case "ListSnapshot":
		
			$ReplicaInfo = $CommonModel->getReplicatInfo( $PostData["ReplicatId"] );

			$machineInfo = $VMWareClient->getVirtualMachineInfo( $PostData["CLUSTER_UUID"], $PostData["ReplicatId"], $PostData["ReplicatId"] );
			
			$snapshotList = array();
			
			foreach( $machineInfo->root_snapshot_list as $snapshots ){
				
				while(true){
					
					$temp = array(
						"name" => $snapshots->name,
						"time" => explode(',', $snapshots->description)[1]
					);
					
					array_push( $snapshotList, $temp );
                    
                    if( !isset( $snapshots->child_snapshot_list[0] ) )
                        break;
                    
                    $snapshots = $snapshots->child_snapshot_list[0];
				}
            }
			
			print_r( json_encode( $snapshotList ) );
		break;
		
		case "getVMWareSettingConfig":
			
			$AuthInfo = $VMWareClient->getESXAuthInfo( $PostData['SERV_UUID'] );
			
			$ReplicaInfo = $CommonModel->getReplicatInfo( $PostData["ReplicatId"] );
			
			$replica_job = json_decode( $ReplicaInfo[0]["rep_job_json"],true );

			$settingRange = $VMWareClient->getVMWareSettingConfig( $AuthInfo["ConnectAddr"], $AuthInfo["EsxIp"], $AuthInfo["Username"], $AuthInfo["Password"], $replica_job["VMWARE_ESX"] );

			if(isset( $ReplicaInfo[0]["network_adapters"] ))
				$settingRange["network_adapters"] = $ReplicaInfo[0]["network_adapters"];
			else
				$settingRange["network_adapters"] = $ReplicaInfo[0]["network_infos"];
				
			$configOtion = $VMWareClient->getConfigOption( $settingRange["EsxInfo"], $ReplicaInfo[0] );

			if(isset( $ReplicaInfo[0]["disks"] ))
				$settingRange["SCSIType"] = $VMWareClient->scsiTypeMappingInt( json_decode($ReplicaInfo[0]["disks"],true)[0]["controller_type"] );

			$settingRange["configOtion"] = $configOtion;
			
			$settingRange["ESXStorage"] = $replica_job["VMWARE_STORAGE"];
			
			$settingRange["memory"] = isset( $ReplicaInfo[0]["vm_memory"] )?$ReplicaInfo[0]["vm_memory"]:$ReplicaInfo[0]["physical_memory"];

			$settingRange["cpu"] = isset( $ReplicaInfo[0]["vm_cpu"] )?$ReplicaInfo[0]["vm_cpu"]:((isset($ReplicaInfo[0]["logical_processors"]) AND $ReplicaInfo[0]["logical_processors"] != 0)?$ReplicaInfo[0]["logical_processors"]:$ReplicaInfo[0]["physical_cpu"]);

			print_r( json_encode( $settingRange ) );
		break;
		
		case "getConfigDefault":
			
			$AuthInfo = $VMWareClient->getESXAuthInfo( $PostData['SERV_UUID'] );
			
			$ReplicaInfo = $CommonModel->getReplicatInfo( $PostData["ReplicatId"] );
			
			$replica_job = json_decode( $ReplicaInfo[0]["rep_job_json"],true );

			$settingRange = $VMWareClient->getVMWareSettingConfig( $AuthInfo["ConnectAddr"], $AuthInfo["EsxIp"], $AuthInfo["Username"], $AuthInfo["Password"], $replica_job["VMWARE_ESX"] );

			if(isset( $ReplicaInfo[0]["network_adapters"] ))
				$settingRange["network_adapters"] = $ReplicaInfo[0]["network_adapters"];
			else
				$settingRange["network_adapters"] = $ReplicaInfo[0]["network_infos"];
			
			$osinfo = $VMWareClient->getOSFromOSType( $PostData['os'] );
			
			$configOtion = $VMWareClient->getConfigOption( $settingRange["EsxInfo"], $ReplicaInfo[0], $osinfo );
			
			$settingRange["configOtion"] = $configOtion;
	
			print_r( json_encode( $settingRange ) );
		break;
		
		case "QueryAccessCredentialInformation":
		
			$VMWareInfo = $CommonModel->query_cloud_connection_information( $PostData["ClusterUUID"] );
			
			$name = json_decode( $VMWareInfo["ACCESS_KEY"], true )["INPUT_HOST_USER"];
			$pass = json_decode( $VMWareInfo["SECRET_KEY"], true )["INPUT_HOST_PASS"];
			$endpoint = $VMWareInfo["DEFAULT_ADDR"];
			
			$ret = array(
				"name" 		=> $name,
				"pass" 		=> $pass, 
				"endpoint" 		=> $endpoint
			);
			print_r(json_encode($ret));		
		break;

		case "UpdateConnection":
			$CLUSTER_UUID  		= $PostData['CLUSTER_UUID'];
			$INPUT_HOST_IP		= trim($PostData['INPUT_HOST_IP']);
			$INPUT_HOST_USER 	= trim($PostData['INPUT_HOST_USER']);
			$INPUT_HOST_PASS 	= trim($PostData['INPUT_HOST_PASS']);
			$TRANSPORT_UUID		= trim($PostData['TRANSPORT_UUID']);
			
			$auth_data = array(
							"INPUT_HOST_USER" => $INPUT_HOST_USER
			);
			
			$secret_data = array(
							"INPUT_HOST_PASS" => $INPUT_HOST_PASS
			);
			
			cancelPromoteTransport( $CLUSTER_UUID );

			$CommonModel -> update_cloud_connection($CLUSTER_UUID, json_encode( $auth_data ), json_encode( $secret_data ), "");

			$a = $CommonModel->getMachineId( $TRANSPORT_UUID );
			
			$CommonModel->updateServerInfo( $CLUSTER_UUID, $a["machineId"] );
			
			$CommonModel->updateServerConn( $CLUSTER_UUID, $TRANSPORT_UUID );
	
			print_r(json_encode(array('Code' => true, 'Msg' => 'Successful update VMware connection.')));
		break;
		
	}
}

