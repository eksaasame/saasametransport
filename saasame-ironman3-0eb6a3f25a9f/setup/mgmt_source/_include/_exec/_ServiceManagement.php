<?php

require_once '_ServiceManagementFunction.php';

############################
# TRIGGER ASYNCHRONOUS CALL
############################
function asynchronous_call($POST_DATA,$TIMEOUT)
{
	$POST_DATA['EncryptKey'] = Misc_Class::encrypt_decrypt('encrypt',time());
	
	$ch = curl_init();
	curl_setopt($ch, CURLOPT_URL, 'http://127.0.0.1:8080/restful/ServiceManagement');
	curl_setopt($ch, CURLOPT_FRESH_CONNECT, true);
	curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($POST_DATA));
	curl_setopt($ch, CURLOPT_TIMEOUT_MS, $TIMEOUT);
	
	curl_exec($ch);	
	curl_close($ch);
}

set_time_limit(0);
session_write_close();
ob_end_flush();

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

		$ReplMgmt    	= new Replica_Class();
		$ServerMgmt  	= new Server_Class();
		$ServiceMgmt 	= new Service_Class();
		$MailerMgmt		= new Mailer_Class();
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
	$CALLBACK_FILTER = array_filter(explode("?", $_SERVER['REQUEST_URI']));	
	if (isset($CALLBACK_FILTER[1]))
	{	
		$ACTION_FILTER = array_filter(explode("=", $CALLBACK_FILTER[1]));
		if ($ACTION_FILTER[0] == 'ServiceCallback' AND isset($ACTION_FILTER[1]))
		{
			$ReplMgmt	  = new Replica_Class();
			$ServiceMgmt  = new Service_Class();
			$MailerMgmt	  = new Mailer_Class();
			$SelectAction = $ACTION_FILTER[0];
			$PostData 	  = array('ServUUID' => $ACTION_FILTER[1]);
		}
		elseif ($ACTION_FILTER[0] == 'DownloadFile' AND isset($ACTION_FILTER[1]))
		{
			$FILE_LOCATION = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/upload_script/'.$ACTION_FILTER[1]; #FILE LOCATION
			if (file_exists($FILE_LOCATION)) {
				header('Content-Description: File Transfer');
				header('Content-Type: application/octet-stream');
				header('Content-Disposition: attachment; filename="'.basename($FILE_LOCATION).'"');
				header('Expires: 0');
				header('Cache-Control: must-revalidate');
				header('Pragma: public');
				header('Content-Length: ' . filesize($FILE_LOCATION));
				readfile($FILE_LOCATION);
				exit;
			}
			else
			{
				header($_SERVER["SERVER_PROTOCOL"]." 404 page not found");
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
	}
	else
	{	
		header($_SERVER["SERVER_PROTOCOL"]." 401 Unauthorized");
		print_r(json_encode(false));
		exit;	
	}
}

if (isset($SelectAction))
{
	switch ($SelectAction)
	{
		############################
		# List Cloud
		############################
		case "ListCloud":		
			$QueryCloud = $ServiceMgmt -> list_cloud($PostData['AcctUUID']);
			
			print_r(json_encode($QueryCloud));	
		break;
		
		############################
		# Edit Select Cloud
		############################
		case "EditSelectCloud":		
			$QueryCloud = $ServiceMgmt -> query_cloud_info($PostData['ClusterUUID']);
			
			print_r(json_encode($QueryCloud));	
		break;
		
		############################
		# Delete Select Cloud
		############################
		case "DeleteSelectCloud":
			
			$DeleteCloud = $ServiceMgmt -> delete_cloud($PostData['ClusterUUID']);
			
			if ($DeleteCloud == TRUE)
			{
				$Msg = 'Cloud connection deleted.';
			}
			else
			{
				$Msg = 'Associated transport still registered with select cloud connection.';
			}
			print_r(json_encode(array('Code' => $DeleteCloud, 'Msg' => $Msg)));			
		break;
		
		############################
		# List Cloud Disk
		############################
		case "ListCloudDisk":
		
			$OPENSTACK_DISK = $ServiceMgmt -> query_cloud_disk($PostData['ReplUUID']);
			
			echo json_encode($OPENSTACK_DISK);
		break;
		
		############################
		# Get Server List
		############################
		case "GetServiceTypes":		
			echo json_encode($ServerMgmt -> list_server_type(),true);
		break;
		
		
		############################
		# Query Available Service
		############################
		case "QueryAvailableService":		
			$AcctUUID = $PostData['AcctUUID'];
			$ServTYPE = $PostData['ServTYPE'];
			$SystTYPE = $PostData['SystTYPE'];
			#$WithAllIPAddresses = $PostData['WithAllIPAddresses'];
		
			echo json_encode($ServerMgmt -> list_server_with_type($AcctUUID,$ServTYPE,$SystTYPE),true);
		break;
		
		
		############################
		# Query Select Transport Info
		############################
		case "QuerySelectTransportInfo":					
			$ServUUID = $PostData['ServUUID'];
				
			echo json_encode($ServiceMgmt -> query_select_transport_info($ServUUID),true);
		break;
		
		
		############################
		# Query Available Host
		############################
		case "QueryAvailableHost":		
			$AcctUUID = $PostData['AcctUUID'];
			$ServTYPE = $PostData['ServTYPE'];
			
			echo json_encode($ServerMgmt -> list_host_with_type($AcctUUID,$ServTYPE),true);
		break;
		
		############################
		# Test Service Connection (For-Packer-Type)
		############################
		case "TestServiceConnection":
			$AcctUUID = $PostData['AcctUUID'];
			$ServUUID = $PostData['ServUUID'];
			$HostADDR = $PostData['HostADDR'];
			$HostUSER = $PostData['HostUSER'];
			$HostPASS = $PostData['HostPASS'];
			$MgmtADDR = null;
			$HostUUID = $PostData['HostUUID'];			
			$ServTYPE = $PostData['ServTYPE'];
		
			echo json_encode($ServiceMgmt -> test_service_connection($AcctUUID,$ServUUID,$HostADDR,$HostUSER,$HostPASS,$ServTYPE,$MgmtADDR,$HostUUID),true);
		break;
	
		############################
		# Test Service Connection (All-In-One)
		############################
		case "TestServicesConnection":		
			$ServADDR = $PostData['ServADDR'];
			$SeltSERV = $PostData['SeltSERV'];
			$MgmtADDR = ($PostData['MgmtADDR'] == '127.0.0.1' OR $PostData['MgmtADDR'] == 'localhost')?getHostByName(getHostName()):$PostData['MgmtADDR'];
		
			echo json_encode($ServiceMgmt -> test_services_connection($ServADDR,$SeltSERV,$MgmtADDR),true);
		break;
		
		############################
		# Query Virtual Host List
		############################
		case "QueryVirtualHostList":
			$AcctUUID = $PostData['AcctUUID'];
			$ServUUID = $PostData['ServUUID'];
			$HostADDR = $PostData['HostADDR'];
			$HostUSER = $PostData['HostUSER'];
			$HostPASS = $PostData['HostPASS'];
			
			$SERV_INFO = $ServerMgmt -> query_server_info($ServUUID);
			if ($SERV_INFO == FALSE)
			{
				return false;
			}
			if ($SERV_INFO['SERV_INFO']['direct_mode'] == TRUE)
			{
				#SERVER ADDRESS ARRAY
				$ServADDR = $SERV_INFO['SERV_ADDR'];
			}
			else
			{
				#SERVER ADDRESS ARRAY
				$ServADDR = array($SERV_INFO['SERV_INFO']['machine_id']);
			}		
			
			for ($i=0; $i<count($ServADDR); $i++)
			{
				$VirtualHostList = $ServiceMgmt -> virtual_host_info($ServADDR[$i],$HostADDR,$HostUSER,$HostPASS);
			
				if ($VirtualHostList != FALSE)
				{
					$VirtualHostList = $VirtualHostList -> vms;
					break;
				}
			}
			
			if ($VirtualHostList != FALSE)
			{
				foreach($VirtualHostList as $VmUUID => $HostName)
				{
					/* Query Host Information */
					if ($VmUUID != '')
					{
						$CheckHostExist = $ServerMgmt -> query_host_info($VmUUID);
						if ($CheckHostExist == FALSE)
						{
							$check_host_exist = FALSE;
						}
						else
						{
							if ($CheckHostExist['ACCT_UUID'] == $AcctUUID)
							{							
								$check_host_exist = TRUE;
							}
							else
							{
								$check_host_exist = FALSE;
							}
						}
						
						$QUERY_VM_LIST[] = array(
											'vm_uuid' => $VmUUID,
											'name' => $HostName,
											'in_db_list' => $check_host_exist
											);
					}
				}
				print_r(json_encode($QUERY_VM_LIST));
			}
			else
			{
				echo json_encode(array('Msg' => 'Failed to get virtual machine host list.', 'Status' => false));
			}			
		break;
		
		############################
		# Query Select VM Host Info
		############################
		case "QuerySelectVmHostInfo":
			$ServUUID = $PostData['ServUUID'];
			$HostADDR = $PostData['HostADDR'];
			$HostUSER = $PostData['HostUSER'];
			$HostPASS = $PostData['HostPASS'];
			$SelectVM = $PostData['SelectVM'];
			
			$SERV_INFO = $ServerMgmt -> query_server_info($ServUUID);
			
			if ($SERV_INFO == FALSE)
			{
				return false;
			}
						
			#SERVER ADDRESS ARRAY
			if($SERV_INFO['SERV_INFO']['direct_mode'] == TRUE)
			{
				$ServADDR = $SERV_INFO['SERV_ADDR'];
			}
			else
			{
				$ServADDR = array($SERV_INFO['SERV_INFO']['machine_id']);
			}
		
			for($i=0; $i<count($ServADDR); $i++)
			{
				$QUERY_VM_INFO = $ServiceMgmt -> virtual_vms_info($ServADDR[$i],$HostADDR,$HostUSER,$HostPASS,$SelectVM);
				if ($QUERY_VM_INFO != FALSE)
				{
					break;
				}
			}
			print_r(json_encode($QUERY_VM_INFO));		
		break;
		
		############################
		# Initialize New Service
		############################
		case "InitializeNewService":		
			$AcctUUID 		= $PostData['AcctUUID'];
			$RegnUUID 		= $PostData['RegnUUID'];
			$OpenUUID 		= $PostData['OpenUUID'];
			$ServUUID 		= $PostData['ServUUID'];
			$HostADDR 		= $PostData['HostADDR'];
			$HostUSER 		= $PostData['HostUSER'];
			$HostPASS 		= $PostData['HostPASS'];
			$ServTYPE 		= $PostData['ServTYPE'];
			$SystTYPE 		= $PostData['SystTYPE'];
			$PriorityAddr	= $PostData['PriorityAddr'];
			$SelectVMHost 	= $PostData['SelectVMHost'];

			addHost( $AcctUUID, $RegnUUID, $OpenUUID, $ServUUID, $HostADDR, $HostUSER, $HostPASS, $ServTYPE, $SystTYPE, $PriorityAddr, $SelectVMHost );
			
			break;

		############################
		# Initialize New Services (All-In-One)
		############################
		case "InitializeNewServices":
			$AcctUUID = $PostData['AcctUUID'];
			$RegnUUID = $PostData['RegnUUID'];			
			$OpenUUID = $PostData['OpenUUID'];
			$HostUUID = $PostData['HostUUID'];			
			$ServADDR = $PostData['ServADDR'];
			$SeltSERV = $PostData['SeltSERV'];
			$SystTYPE = $PostData['SystTYPE'];			
			$MgmtADDR = ($PostData['MgmtADDR'] == '127.0.0.1' OR $PostData['MgmtADDR'] == 'localhost')?getHostByName(getHostName()):$PostData['MgmtADDR'];
			$ConnDEST = $PostData['ConnDEST'];
			$MgmtDisk = $PostData['MgmtDisk'];
			
			
			$InitTYPE = 'Add';
			if ($ConnDEST != 'NA')
			{
				$ConnPath = array('LocalPath' => $ConnDEST, 'WebDavPath' => null);
				$InitializeConnection = $ServiceMgmt -> verify_folder_connection(explode(',',$ServADDR),$ConnPath,$InitTYPE,'LocalFolder');
			}
			else
			{
				$InitializeConnection = true;
			}
			
			if ($InitializeConnection != FALSE)
			{
				$ServUUUD = $ServerMgmt -> initialize_servers($AcctUUID,$RegnUUID,$OpenUUID,$HostUUID,$ServADDR,$SeltSERV,$SystTYPE,$MgmtADDR,$MgmtDisk);

				if ($ServUUUD != false)
				{
					if ($ConnDEST != 'NA')
					{
						$ConnectionDate = array_merge(array('ACCT_UUID' => $AcctUUID,'CONN_TYPE' => 'LocalFolder'),$ServUUUD,$InitializeConnection,array('MGMT_ADDR' => $MgmtADDR),array('CLUSTER_UUID' => $OpenUUID));
					
						$SaveConnection = $ServerMgmt -> save_connection($ConnectionDate);
						echo json_encode($SaveConnection);
					}
					else
					{
						echo json_encode(true);
					}
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
		break;
		
		############################
		# Update Host Information
		############################
		case "UpdateHostInformation":
			$AcctUUID 	  = $PostData['AcctUUID'];
			$RegnUUID 	  = $PostData['RegnUUID'];
			$OpenUUID 	  = $PostData['OpenUUID'];
			$ServUUID 	  = $PostData['ServUUID'];
			$HostUUID 	  = $PostData['HostUUID'];
			$HostADDR 	  = $PostData['HostADDR'];
			$HostUSER 	  = $PostData['HostUSER'];
			$HostPASS 	  = $PostData['HostPASS'];
			$ServTYPE 	  = $PostData['ServTYPE'];
			$PriorityAddr = $PostData['PriorityAddr'];
			$SelectVMHost = $PostData['SelectVMHost'];
			
			$SERV_INFO = $ServerMgmt -> query_server_info($ServUUID);
			
			if ($SERV_INFO['SERV_INFO']['direct_mode'] == TRUE)
			{
				$ServADDR = $SERV_INFO['SERV_ADDR'];
			}
			else
			{
				$ServADDR = array($SERV_INFO['SERV_INFO']['machine_id']);				
			}
			
			$HostADDR = explode(',', $HostADDR);
	
			switch ($ServTYPE)
			{
				case 'Virtual Packer':
					for ($s=0; $s<count($ServADDR); $s++)
					{
						try{
							$ServINFO = $ServiceMgmt -> get_connection($ServADDR[$s],$ServTYPE);
							if ($ServINFO != false)
							{
								for ($i=0; $i<count($HostADDR); $i++)
								{
									$VmsINFO  = $ServiceMgmt -> virtual_host_info($ServADDR[$s],$HostADDR[$i],$HostUSER,$HostPASS);
									$PackerAddress = $HostADDR[$i];	
								}
								break 2;
							}
						}
						catch(Throwable $e){
							//return false;
						}
					}					
				break;
				
				case 'Physical Packer':
					for ($s=0; $s<count($ServADDR); $s++)
					{
						for ($i=0; $i<count($HostADDR); $i++)
						{
							$VmsINFO  = $ServiceMgmt -> physical_host_info($ServADDR[$s],$HostADDR[$i]);
							if ($VmsINFO != '')
							{
								$PackerAddress = $HostADDR[$i];
								break 2;
							}
						}
					}
					
					#UPDATE CARRIER PRIORITY ADDRESS
					$VmsINFO -> priority_addr = $PriorityAddr;
					
					$ServINFO = $VmsINFO;
				break;
				
				case 'Offline Packer':
					for ($s=0; $s<count($ServADDR); $s++)
					{
						for ($i=0; $i<count($HostADDR); $i++)
						{
							$VmsINFO  = $ServiceMgmt -> physical_host_info($ServADDR[$s],$HostADDR[$i]);
							if ($VmsINFO != '')
							{
								$PackerAddress = $HostADDR[$i];
								break 2;
							}
						}
					}			
					
					#UPDATE CARRIER PRIORITY ADDRESS
					$VmsINFO -> priority_addr = $PriorityAddr;
					
					$ServINFO = $VmsINFO;
				break;
			}
		
			print_r($ServerMgmt -> update_host($AcctUUID,$RegnUUID,$OpenUUID,$HostUUID,$PackerAddress,$HostUSER,$HostPASS,$ServTYPE,$ServINFO,$VmsINFO,$ServUUID,$ServADDR,$SelectVMHost));
		break;
		
		
		############################
		# Query Server By Account ID
		############################
		case "QueryServiceByAcctUUID":
			$AcctUUID = $PostData['AcctUUID'];
					
			print_r($ServerMgmt->list_server_with_type($AcctUUID));	
		break;
		
		
		############################
		# Update Server By Service ID
		############################
		case "UpdateTrnasportServices":
			$SERV_UUID = $PostData['ServUUID'];
			$SERV_ADDR = $PostData['ServADDR'];
			$MGMT_ADDR = ($PostData['MgmtADDR'] == '127.0.0.1' OR $PostData['MgmtADDR'] == 'localhost')?getHostByName(getHostName()):$PostData['MgmtADDR'];
						
			print_r($ServerMgmt -> update_server($SERV_UUID,$SERV_ADDR,$MGMT_ADDR));	
		break;
		
		
		############################
		# Select Edit Server By Server ID
		############################
		case "SelectEditServiceByUUID":
			$ServUUID = $PostData['ServUUID'];
			
			$LauncherInfo = $ServerMgmt -> query_server_info($ServUUID);
			
			$OPEN_UUID  = $LauncherInfo['OPEN_UUID'];
			$OPEN_HOST  = $LauncherInfo['OPEN_HOST'];
			$SYST_TYPE  = $LauncherInfo['SYST_TYPE'];
			$IS_WINPE   = $LauncherInfo['SERV_INFO']['is_winpe'];
			$IS_PROMOTE = $LauncherInfo['SERV_INFO']['is_promote'];
			
			$SERV_INFO = array(
							'SERV_UUID'  => $ServUUID,
							'OPEN_UUID'  => $OPEN_UUID,
							'OPEN_HOST'  => $OPEN_HOST,
							'SYST_TYPE'  => $SYST_TYPE							
						);
			
			if ($OPEN_UUID == 'ONPREMISE-00000-LOCK-00000-PREMISEON')
			{
				if ($IS_WINPE == false)
				{
					$SERV_INFO['RDIR_TYPE'] = 'OnPremise';
				}
				else
				{
					$SERV_INFO['RDIR_TYPE'] = 'RecoveryKit';
				}
			}
			else
			{
				$HOST_INFO = explode('|',$OPEN_HOST);
				if(count( $HOST_INFO ) > 2)
				{
					$SERV_INFO['RDIR_TYPE'] = $HOST_INFO[2];
				}
				elseif(isset($HOST_INFO[1]))
				{
					$SERV_INFO['RDIR_TYPE'] = 'AWS';
				}
				elseif ($LauncherInfo['VENDOR_NAME'] == 'Ctyun')
				{
					$SERV_INFO['RDIR_TYPE'] = 'Ctyun';
				}
				elseif ($LauncherInfo['VENDOR_NAME'] == 'VMWare')
				{
					$SERV_INFO['RDIR_TYPE'] = 'VMWare';
				}
				else
				{
					$SERV_INFO['RDIR_TYPE'] = 'OpenStack';
				}			
			}
			
			if($SERV_INFO['RDIR_TYPE'] == "Azure" OR $LauncherInfo['VENDOR_NAME'] == "AzureBlob")
			{
				$SERV_INFO["SERV_ADDR"] = $LauncherInfo['SERV_ADDR'][0];
					
				if ($IS_PROMOTE == TRUE)
				{
					$SERV_INFO['RDIR_TYPE'] = 'OnPremise';
				}
			}
			
			print_r(json_encode($SERV_INFO));
		break;		
		
		
		############################
		# Query Transport Server Information
		############################
		case "QueryTransportServerInformation":
			$ServUUID = $PostData['ServUUID'];
			$OnTheFly = $PostData['OnTheFly'];

			if ($OnTheFly == FALSE)
			{
				$LauncherInfo = $ServerMgmt -> query_server_info($ServUUID);
			}
			else
			{
				$LauncherInfo = $ServiceMgmt -> query_select_transport_info($ServUUID);
			}
			print_r(json_encode($LauncherInfo));
		break;
		
		
		############################
		# Check Running Connection
		############################
		case "CheckRunningConnection":
			$ServUUID = $PostData['ServUUID'];
			$Is_Running = $ServerMgmt -> check_running_connection($ServUUID);			
			print_r(json_encode($Is_Running));
		break;		
		
		
		############################
		# Delete Service
		############################
		case "DeleteServiceByUUID":
			$ServUUID = $PostData['ServUUID'];
		
			print_r($ServerMgmt -> delete_server($ServUUID,TRUE));	
		break;
		

		############################
		# Query Packer By Address
		############################
		case "QueryPackerInfoByAddress":
			$ServADDR = $PostData['ServADDR'];
					
			print_r($ServiceMgmt -> get_packer_info($ServADDR));	
		break;


		############################
		# Create Job Connection
		############################
		case "CreateJobConnection":
			$ServADDR   = $PostData['ServADDR'];
			$ConnUUID   = $PostData['ConnUUID'];
			$DestFolder = $PostData['DestFolder'];
			
			print_r($ServiceMgmt->add_connection($ServADDR,$ConnUUID,$DestFolder));	
		break;
		
		
		############################
		# Modify Job Connection
		############################
		case "ModifyJobConnection":
			$ServADDR   = $PostData['ServADDR'];
			$ConnUUID   = $PostData['ConnUUID'];
			$DestFolder = $PostData['DestFolder'];
				
			print_r($ServiceMgmt->modify_connection($ServADDR,$ConnUUID,$DestFolder));	
		break;

		
		############################
		# Remove Job Connection
		############################
		case "RemoveJobConnection":
			$ServADDR   = $PostData['ServADDR'];
			$ConnUUID   = $PostData['ConnUUID'];
				
			print_r($ServiceMgmt->remove_connection($ServADDR,$ConnUUID));	
		break;
		
		
		############################
		# Enumerate Job Connection
		############################
		case "EnumerateJobConnection":
			$ServADDR   = $PostData['ServADDR'];
				
			print_r($ServiceMgmt->enumerate_connection($ServADDR));	
		break;
		
		
		############################
		# Query Server Information
		############################
		case "QueryServerInformation":
			$ServUUID = $PostData['ServUUID'];
				
			echo json_encode($ServerMgmt->query_server_info($ServUUID),true);			
		break;

		
		############################
		# Query Connection Information
		############################
		case "QueryConnectionInformation":
			$ConnUUID = $PostData['ConnUUID'];
				
			echo json_encode($ServerMgmt->query_connection_info($ConnUUID),true);	
		break;
		
		
		############################
		# Query Connection Information
		############################
		case "QueryConnectionInformationByServUUID":
			$HostUUID = $PostData['HostUUID'];
			$LoadUUID = $PostData['LoadUUID'];
			$LaunUUID = $PostData['LaunUUID'];
			
			$CONNECTION = $ServerMgmt->query_connection_by_serv_uuid($HostUUID,$LoadUUID,$LaunUUID);
			
			echo json_encode($CONNECTION,true);		
		break;
		
		
		############################
		# Query Host Information
		############################
		case "QueryHostInformation":
			$HostUUID = $PostData['HostUUID'];
				
			echo json_encode($ServerMgmt->query_host_info($HostUUID),true);	
		break;
		
		
		############################
		# Query Select Host Information
		############################
		case "QuerySelectHostInfo":
			$HostUUID = $PostData['HostUUID'];
			
			echo json_encode($ServiceMgmt->query_select_host_info($HostUUID),true);	
		break;
		
		
		############################
		# Query Available Connection
		############################
		case "QueryAvailableConnection":
			$AcctUUID = $PostData['AcctUUID'];
				
			echo json_encode($ServerMgmt->query_available_connection($AcctUUID),true);	
		break;
			
		
		############################
		# Test Connection
		############################
		case "TestConnection":
			$SchdUUID = $PostData['SchdUUID'];
			$CarrUUID = $PostData['CarrUUID'];
			$LoadUUID = $PostData['LoadUUID'];
			$LaunUUID = $PostData['LaunUUID'];
			$ConnDEST = $PostData['ConnDEST'];
			
			$ConnOutput = $ServiceMgmt->test_add_connections($SchdUUID,$CarrUUID,$LoadUUID,$LaunUUID,$ConnDEST,'Test','LocalFolder');
		
			if ($ConnOutput != FALSE)
			{
				echo true;				
			}
			else
			{
				echo false;
			}
		break;
		
		
		############################
		# Add Connection
		############################
		case "AddConnection":
			$SchdUUID = $PostData['SchdUUID'];
			$CarrUUID = $PostData['CarrUUID'];
			$LoadUUID = $PostData['LoadUUID'];
			$LaunUUID = $PostData['LaunUUID'];
			$ConnDEST = $PostData['ConnDEST'];
			
			$ConnOutput = $ServiceMgmt -> test_add_connections($SchdUUID,$CarrUUID,$LoadUUID,$LaunUUID,$ConnDEST,'Add','LocalFolder');
	
			if ($ConnOutput != FALSE)
			{
				$Output = $ServerMgmt -> save_connection($ConnOutput);
				
				echo $Output;				
			}
			else
			{
				echo false;
			}
		break;
		
		############################
		# Select Edit Host
		############################
		case "SelectEditHost":
			$HostUUID = $PostData['HostUUID'];
		
			$HostInfo = $ServerMgmt -> query_host_info($HostUUID);
				
			$HOST_TYPE = $HostInfo['HOST_TYPE'];
			$MANUFACTURER = $HostInfo['HOST_INFO']['manufacturer'];
		
			if ($HOST_TYPE == 'Physical')
			{
				if ($MANUFACTURER == 'Xen')
				{
					$HostType = 'AWS';
				}
				elseif ($MANUFACTURER == 'OpenStack Foundation')
				{
					$HostType = 'OpenStack';
				}
				else
				{
					$HostType = 'Physical';
				}
			}
			elseif ($HOST_TYPE == 'Virtual')
			{
				$HostType = 'Virtual';
			}
			elseif ($HOST_TYPE == 'Offline')
			{
				$HostType = 'Offline';
			}
			else
			{
				$HostType = false;
			}		
			
			print_r($HostType);
		break;
		
		
		############################
		# Query Host Information
		############################
		case "QueryHostInfo":
			$HostUUID = $PostData['HostUUID'];
		
			$HostInfo = $ServerMgmt -> query_host_info($HostUUID);
			
			print_r(json_encode($HostInfo));
		break;
			
		
		############################
		# Delete Packer Host
		############################
		case "DeletePackerHost":
			$AcctUUID = $PostData['AcctUUID'];
			$HostUUID = $PostData['HostUUID'];
		
			$Output = $ServerMgmt -> delete_packer_host($AcctUUID,$HostUUID);
			
			if ($Output == TRUE)
			{
				$Response = array('Code' => $Output, 'Msg' => 'Host deleted.');
			}
			else
			{
				$Response = array('Code' => false, 'Msg' => 'Cannot delete host; host links with running replica.');
			}			
			print_r(json_encode($Response));
		break;

		
		############################
		# Reflush Packer Information
		############################
		case "ReflushPacker":
			$AcctUUID = $PostData['AcctUUID'];
			$HostUUID = $PostData['HostUUID'];
		
			$ServiceMgmt -> reflush_physical_packer($AcctUUID,$HostUUID);
			$ServiceMgmt -> reflush_virtual_packer($AcctUUID,$HostUUID);
		break;
		
		
		############################
		# Reflush Packer Async
		############################
		case "ReflushPackerAsync":
			$PostData['Action'] = 'ReflushPacker';
					
			echo json_encode($PostData['ReplJobUUID']);
					
			asynchronous_call($PostData,1000);
		break;		
		
		############################
		# List Available Replica
		############################
		case "ListAvailableReplica":
			$AcctUUID = $PostData['AcctUUID'];
			
			echo json_encode($ReplMgmt -> list_replica($AcctUUID,null));	
		break;
				
		############################
		# List Available Replica With Plan
		############################
		case "ListAvailableReplicaWithPlan":
			$AcctUUID = $PostData['AcctUUID'];
			
			echo json_encode($ReplMgmt -> list_replica_with_plan($AcctUUID));	
		break;
		
		############################
		# Display Job History
		############################
		case "DisplayJobHistory":
			$JobsUUID  = $PostData['JobsUUID'];
			$ItemLimit = $PostData['ItemLimit'];
			
			echo json_encode($ReplMgmt -> list_job_history($JobsUUID,$ItemLimit));	
		break;
		
		
		############################
		# Display Service Job Information
		############################
		case "DisplayPrepareJobInfo":
			$JobsUUID  = $PostData['JobsUUID'];
		
		try{
			echo json_encode($ServiceMgmt -> query_prepare_workload($JobsUUID));	
		}
		catch( Exception $e ){
			echo $e->getMessage();
		}
		break;
		
		
		############################
		# Display Service Job Information
		############################
		case "DisplayRecoveryJobInfo":
			$JobsUUID  = $PostData['JobsUUID'];
		
			echo json_encode($ServiceMgmt -> query_recovery_workload($JobsUUID));	
		break;
		
		
		############################
		# Display Job Snapshot History
		############################
		case "DisplayJobSnapshotHistory":
			$JobsUUID = $PostData['JobsUUID'];
			
			echo json_encode($ReplMgmt -> list_snapshot_job_history($JobsUUID));	
		break;
		
		
		############################
		# Begin To Run Replica
		############################
		case "BeginToRunReplica":
			#JOB CONF
			$ReplicaSettings = json_decode($PostData['ReplicaSettings'],false);
				
			if (isset($ReplicaSettings -> UseBlockMode) AND $ReplicaSettings -> UseBlockMode = 'off'){$UseBlockMode = false;}else{$UseBlockMode = true;} #ReverseWithOthers
			if (isset($ReplicaSettings -> CreateByPartition) AND $ReplicaSettings -> CreateByPartition = 'on'){$CreateByPartition = true;}else{$CreateByPartition = false;}
			if (isset($ReplicaSettings -> EnableCDR) AND $ReplicaSettings -> EnableCDR = 'on'){$EnableCDR = true;}else{$EnableCDR = false;}
			if (isset($ReplicaSettings -> ChecksumVerify) AND $ReplicaSettings -> ChecksumVerify = 'on'){$ChecksumVerify = true;}else{$ChecksumVerify = false;}
			if (isset($ReplicaSettings -> SchedulePause) AND $ReplicaSettings -> SchedulePause = 'on'){$SchedulePause = true;}else{$SchedulePause = false;}
			
			if (isset($ReplicaSettings -> SetDiskCustomizedId) AND $ReplicaSettings -> SetDiskCustomizedId = 'on'){$SetDiskCustomizedId = true;}else{$SetDiskCustomizedId = false;}			
			if (isset($ReplicaSettings -> DataCompressed) AND $ReplicaSettings -> DataCompressed = 'on'){$DataCompressed = true;}else{$DataCompressed = false;}
			if (isset($ReplicaSettings -> DataChecksum) AND $ReplicaSettings -> DataChecksum = 'on'){$DataChecksum = true;}else{$DataChecksum = false;}
			if (isset($ReplicaSettings -> FileSystemFilter) AND $ReplicaSettings -> FileSystemFilter = 'on'){$FileSystemFilter = true;}else{$FileSystemFilter = false;}
			if (isset($ReplicaSettings -> ExtraGB) AND $ReplicaSettings -> ExtraGB = 'on'){$ExtraGB = true;}else{$ExtraGB = false;}
			if (isset($ReplicaSettings -> IsAzureBlobMode) AND $ReplicaSettings -> IsAzureBlobMode = 'on'){$IsAzureBlobMode = true;}else{$IsAzureBlobMode = false;}
			if (isset($ReplicaSettings -> IsPackerDataCompressed) AND $ReplicaSettings -> IsPackerDataCompressed = 'on'){$IsPackerDataCompressed = true;}else{$IsPackerDataCompressed = false;}
			if (isset($ReplicaSettings -> ReplicationRetry) AND $ReplicaSettings -> ReplicationRetry = 'on'){$ReplicationRetry = true;}else{$ReplicationRetry = false;}
			if (isset($ReplicaSettings -> PackerEncryption) AND $ReplicaSettings -> PackerEncryption = 'on'){$PackerEncryption = true;}else{$PackerEncryption = false;}
			
			if (!isset($ReplicaSettings -> CloudMappingDisk)){$CloudMappingDisk = false;}else{$CloudMappingDisk = $ReplicaSettings -> CloudMappingDisk;}
			if (isset($ReplicaSettings -> MultiLayerProtection)){$MultiLayerProtection = $ReplicaSettings -> MultiLayerProtection;}else{$MultiLayerProtection = 'NoSeries';}
			if (isset($ReplicaSettings -> CascadedCarrier)){$CascadedCarrier = $ReplicaSettings -> CascadedCarrier;}else{$CascadedCarrier = false;}
			
			$REPLICA_CONFIG = array(
									'IntervalMinutes' 	 	   	=> $ReplicaSettings -> IntervalMinutes,
									'SnapshotsNumber' 	 	   	=> $ReplicaSettings -> SnapshotsNumber,
									'BufferSize'  	 	   		=> $ReplicaSettings -> BufferSize,
									'StartTime'					=> $ReplicaSettings -> StartTime,									
									'WorkerThreadNumber' 	   	=> $ReplicaSettings -> WorkerThreadNumber,
									'LoaderThreadNumber' 	   	=> $ReplicaSettings -> LoaderThreadNumber,
									'LoaderTriggerPercentage' 	=> $ReplicaSettings -> LoaderTriggerPercentage,
									'ExportType'				=> $ReplicaSettings -> ExportType,
									'ExportPath'				=> $ReplicaSettings -> ExportPath,
									'CreateByPartition'			=> $CreateByPartition,
									'EnableCDR'					=> $EnableCDR,
									'ChecksumVerify' 	   		=> $ChecksumVerify,
									'UseBlockMode'  	 	   	=> $UseBlockMode,
									'SchedulePause'				=> $SchedulePause,
									'SetDiskCustomizedId'		=> $SetDiskCustomizedId,
									'DataCompressed'			=> $DataCompressed,
									'DataChecksum'				=> $DataChecksum,
									'FileSystemFilter'			=> $FileSystemFilter,
									'ExtraGB'					=> $ExtraGB,
									'IsAzureBlobMode'			=> $IsAzureBlobMode,
									'IsPackerDataCompressed'	=> $IsPackerDataCompressed,
									'ReplicationRetry'			=> $ReplicationRetry,
									'PackerEncryption'			=> $PackerEncryption,
									'SkipDisk'					=> $ReplicaSettings -> SkipDisk,
									'CloudMappingDisk'			=> $CloudMappingDisk,
									'PreSnapshotScript'			=> $ReplicaSettings -> PreSnapshotScript,
									'PostSnapshotScript'		=> $ReplicaSettings -> PostSnapshotScript,									
									'PriorityAddr'				=> $ReplicaSettings -> PriorityAddr,
									'WebDavPriorityAddr'		=> $ReplicaSettings -> WebDavPriorityAddr,
									'TimeZone'					=> $ReplicaSettings -> TimeZone,
									'CloudType'					=> $ReplicaSettings -> CloudType,
									'ExcludedPaths'				=> $ReplicaSettings -> ExcludedPaths,
									'PostLoaderScript'			=> $ReplicaSettings -> PostLoaderScript,
									'ApiRefererFrom'			=> $_SERVER['REMOTE_ADDR'],
									'MultiLayerProtection'		=> $MultiLayerProtection,
									'CascadedCarrier'			=> $CascadedCarrier
									);
									
			$AcctUUID  = $PostData['AcctUUID'];
			$RegnUUID  = $PostData['RegnUUID'];
			$CarrUUID  = $PostData['CarrUUID'];
			$LoadUUID  = $PostData['LoadUUID'];
			$LaunUUID  = $PostData['LaunUUID'];
			$HostUUID  = $PostData['HostUUID'];
			$ReplCONF  = $REPLICA_CONFIG;
		
			if( isset($ReplicaSettings->VMWARE_STORAGE) ){
				$ReplCONF["VMWARE_STORAGE"] = $ReplicaSettings->VMWARE_STORAGE;
				$ReplCONF["VMWARE_ESX"] = $ReplicaSettings->VMWARE_ESX;
				$ReplCONF["VMWARE_FOLDER"] = $ReplicaSettings->VMWARE_FOLDER;
				$ReplCONF["VMWARE_THIN_PROVISIONED"] = $ReplicaSettings->VMWARE_THIN_PROVISIONED;
			}
			
			$MgmtADDR = ($PostData['MgmtADDR'] == '127.0.0.1' OR $PostData['MgmtADDR'] == 'localhost')?getHostByName(getHostName()):$PostData['MgmtADDR'];
		
			if (isset($PostData['ReplJobUUID']))
			{
				$ReplJobUUID = $PostData['ReplJobUUID'];
			}
			else
			{
				$ReplJobUUID = Misc_Class::guid_v4();
				echo json_encode($PostData['ReplJobUUID']);
			}
			
			$ServiceMgmt -> pre_create_replica_job($AcctUUID,$RegnUUID,$CarrUUID,$LoadUUID,$LaunUUID,$HostUUID,$ReplCONF,$MgmtADDR,$ReplJobUUID);
		break;
		
		############################
		# Async Begin To Run Replica
		############################
		case "BeginToRunReplicaAsync":
			$PostData['Action'] 	 = 'BeginToRunReplica';
			$PostData['ReplJobUUID'] = Misc_Class::guid_v4();
			
			echo json_encode($PostData['ReplJobUUID']);
					
			asynchronous_call($PostData,2500);
		break;
				
		############################
		# Sync Select Replica
		############################
		case "SyncSelectReplica":
			$ReplUUID 		= $PostData['ReplUUID'];
			$SyncType 		= $PostData['SyncTYPE'];
			$DebugSnapshot	= $PostData['DebugSnapshot'];
			
			$MESSAGE = $ReplMgmt -> job_msg('Trigger job synchronization.');
			$ReplMgmt -> update_job_msg($ReplUUID,$MESSAGE,'Replica');
			
			$HOST_UUID = $ReplMgmt -> query_replica($ReplUUID)['PACK_UUID'];
			$REPL_UUID = $ReplMgmt -> cascading_connection($HOST_UUID)['replica'];
			
			#WITH CASCADING SUPPORT
			$REPL_QUERY = $ReplMgmt -> query_replica($REPL_UUID);		
			$JOB_INFO = $REPL_QUERY['JOBS_JSON'];			
			$JOB_INFO -> sync_control = false;
			$JOB_INFO -> last_sync = time();
			$ServiceMgmt -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');	
			
			$Sync_Status = $ServiceMgmt -> resume_service_job($REPL_UUID,'SCHEDULER');
			
			if ($Sync_Status == TRUE)
			{
				if ($SyncType == 'FullSync')
				{
					$JOB_INFO -> is_full_replica = true;
					$ServiceMgmt -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');						
				}			
				
				$MESSAGE = $ReplMgmt -> job_msg('Successful submitted synchronization task.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
			else
			{
				$JOB_INFO -> is_executing = false;
				$JOB_INFO -> sync_control = true;
				$JOB_INFO -> last_sync = time();
				$ServiceMgmt -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');	
				
				$MESSAGE = $ReplMgmt -> job_msg('Failed to submit synchronization task.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
			echo json_encode(TRUE);
		break;
		
		############################
		# Async Sync Select Replica
		############################
		case "SyncSelectReplicaAsync":
			$PostData['Action'] = 'SyncSelectReplica';
	
			echo json_encode(TRUE);
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# Delete Select Replica
		############################
		case "DeleteSelectReplica":
			$ReplUUID = $PostData['ReplUUID'];
			
			$ServiceMgmt -> delete_replica_job($ReplUUID,'N');
			
			echo json_encode(TRUE);
		break;	
		
		############################
		# Async Delete Select Replica
		############################
		case "DeleteSelectReplicaAsync":
			$PostData['Action'] = 'DeleteSelectReplica';
	
			echo json_encode(TRUE);
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# Query Select Replica Snapshot Script
		############################
		case "QueryReplicaSnapshotScript":
			$ReplUUID = $PostData['ReplUUID'];
			
			$REPLICA_CONFIG_INFO = $ReplMgmt -> query_replica($ReplUUID)['JOBS_JSON'];
			
			$SNAPSHOT_SCRIPT = array('pre_snapshot_script' => $REPLICA_CONFIG_INFO -> pre_snapshot_script, 'post_snapshot_script' => $REPLICA_CONFIG_INFO -> post_snapshot_script);			
			
			print_r(json_encode($SNAPSHOT_SCRIPT));	
		break;
		
		############################
		# Query Select Replica Excluded Paths
		############################
		case "QueryReplicaExcludedPaths":
			//$ReplUUID = $PostData['ReplUUID'];
			
			//$REPLICA_CONFIG_INFO = $ReplMgmt -> query_replica($ReplUUID)['JOBS_JSON'];
			
			//$SNAPSHOT_SCRIPT = array('pre_snapshot_script' => $REPLICA_CONFIG_INFO -> pre_snapshot_script, 'post_snapshot_script' => $REPLICA_CONFIG_INFO -> post_snapshot_script);			
			
			//print_r(json_encode($SNAPSHOT_SCRIPT));	
		break;
		
		############################
		# Query Select Replica Configuration
		############################
		case "QueryReplicaConfiguration":
			$ReplUUID = $PostData['ReplUUID'];
			
			$QUERY_REPLICA = $ReplMgmt -> query_replica($ReplUUID);
			
			$REPLICA_CONFIG_INFO = $QUERY_REPLICA['JOBS_JSON'];
			
			$REPLICA_CONFIG_INFO -> host_uuid = $QUERY_REPLICA['PACK_UUID'];
			
			$MIGRATION_JOBS = $ServiceMgmt -> check_running_service($ReplUUID,true);
			
			#INVERT BOOLENA
			$MIGRATION_JOBS = !$MIGRATION_JOBS;
			
			#ADD MIGRATION EXECUTING STATUS
			$REPLICA_CONFIG_INFO -> is_migration_executing = $MIGRATION_JOBS;
			
			if( $REPLICA_CONFIG_INFO->cloud_type == "VMWare"){
				$model = new Common_Model();
				$DRcount = $model->getDRCount($ReplUUID);
				$REPLICA_CONFIG_INFO->DRcount = $DRcount;
				$DRexit = $model->getDRCount($ReplUUID, true);
				$REPLICA_CONFIG_INFO->DRexit = $DRexit;
			}

			print_r(json_encode($REPLICA_CONFIG_INFO));		
		break;
		
		
		############################
		# Update Replica Configuration
		############################
		case "UpdateReplicaConfiguration":
			$ReplUUID 		 = $PostData['ReplUUID'];
			$ReplicaSettings = json_decode($PostData['ReplicaSettings'],false);
			
			if (isset($ReplicaSettings -> UseBlockMode) AND $ReplicaSettings -> UseBlockMode = 'off'){$UseBlockMode = false;}else{$UseBlockMode = true;} #ReverseWithOthers
			if (isset($ReplicaSettings -> CreateByPartition) AND $ReplicaSettings -> CreateByPartition = 'on'){$CreateByPartition = true;}else{$CreateByPartition = false;}
			if (isset($ReplicaSettings -> EnableCDR) AND $ReplicaSettings -> EnableCDR = 'on'){$EnableCDR = true;}else{$EnableCDR = false;}
			if (isset($ReplicaSettings -> ChecksumVerify) AND $ReplicaSettings -> ChecksumVerify = 'on'){$ChecksumVerify = true;}else{$ChecksumVerify = false;}
			if (isset($ReplicaSettings -> SchedulePause) AND $ReplicaSettings -> SchedulePause = 'on'){$SchedulePause = true;}else{$SchedulePause = false;}
			
			if (isset($ReplicaSettings -> DataCompressed) AND $ReplicaSettings -> DataCompressed = 'on'){$DataCompressed = true;}else{$DataCompressed = false;}
			if (isset($ReplicaSettings -> DataChecksum) AND $ReplicaSettings -> DataChecksum = 'on'){$DataChecksum = true;}else{$DataChecksum = false;}
			if (isset($ReplicaSettings -> FileSystemFilter) AND $ReplicaSettings -> FileSystemFilter = 'on'){$FileSystemFilter = true;}else{$FileSystemFilter = false;}
			if (isset($ReplicaSettings -> ExtraGB) AND $ReplicaSettings -> ExtraGB = 'on'){$ExtraGB = true;}else{$ExtraGB = false;}
			if (isset($ReplicaSettings -> IsPackerDataCompressed) AND $ReplicaSettings -> IsPackerDataCompressed = 'on'){$IsPackerDataCompressed = true;}else{$IsPackerDataCompressed = false;}
			if (isset($ReplicaSettings -> ReplicationRetry) AND $ReplicaSettings -> ReplicationRetry = 'on'){$ReplicationRetry = true;}else{$ReplicationRetry = false;}
						
			$REPLICA_CONFIG = array(
									'IntervalMinutes' 	 	   	=> $ReplicaSettings -> IntervalMinutes,
									'SnapshotsNumber' 	 	   	=> $ReplicaSettings -> SnapshotsNumber,
									'BufferSize'  	 	   		=> $ReplicaSettings -> BufferSize,
									'StartTime'					=> $ReplicaSettings -> StartTime,									
									'WorkerThreadNumber' 	   	=> $ReplicaSettings -> WorkerThreadNumber,
									'LoaderThreadNumber' 	   	=> $ReplicaSettings -> LoaderThreadNumber,
									'LoaderTriggerPercentage' 	=> $ReplicaSettings -> LoaderTriggerPercentage,
									'ExportType'				=> $ReplicaSettings -> ExportType,
									'ExportPath'				=> $ReplicaSettings -> ExportPath,
									'CreateByPartition'			=> $CreateByPartition,
									'EnableCDR'					=> $EnableCDR,
									'ChecksumVerify' 	   		=> $ChecksumVerify,
									'UseBlockMode'  	 	   	=> $UseBlockMode,
									'SchedulePause'				=> $SchedulePause,
									'DataCompressed'			=> $DataCompressed,
									'DataChecksum'				=> $DataChecksum,
									'FileSystemFilter'			=> $FileSystemFilter,
									'ExtraGB'					=> $ExtraGB,
									'IsPackerDataCompressed'	=> $IsPackerDataCompressed,
									'ReplicationRetry'			=> $ReplicationRetry,
									'PreSnapshotScript'			=> $ReplicaSettings -> PreSnapshotScript,
									'PostSnapshotScript'		=> $ReplicaSettings -> PostSnapshotScript,
									'ExcludedPaths'				=> $ReplicaSettings -> ExcludedPaths,
									'TimeZone'					=> $ReplicaSettings -> TimeZone					
									);
			
			$UPDATE_REPLICA_JOB = $ServiceMgmt -> update_replica_job_configuration($ReplUUID,$REPLICA_CONFIG);
			print_r($UPDATE_REPLICA_JOB);
		break;
		
		############################
		# Un Initialized Disks
		############################
		case "GetUninitializedDisksSCSI":
			$ConnUUID = $PostData['ConnUUID'];
			
			$CONN_INFO = $ServerMgmt -> query_connection_info($ConnUUID);
			$LOADER_ADDR = $CONN_INFO['LOAD_ADDR'];
			
			$SCSI_INFO = $ServiceMgmt -> enumerate_disks($LOADER_ADDR);
			if (empty($SCSI_INFO))
			{
				$SCSI_ADDR = 'No Uninitialized Disk';
			}
			else
			{
				$SCSI_INFO = end($SCSI_INFO);
				$SCSI_ADDR = $SCSI_INFO -> scsi_port.':'.$SCSI_INFO -> scsi_bus.':'.$SCSI_INFO -> scsi_target_id.':'.$SCSI_INFO -> scsi_logical_unit;
			}
			
			echo $SCSI_ADDR;
		break;

		############################
		# Query Replica Information
		############################
		case "QueryReplicaInformation":
			$ReplUUID = $PostData['ReplUUID'];	
			
			$REPL_INFO = $ReplMgmt -> query_replica($ReplUUID);
			$CASCADE_INFO = $ReplMgmt -> cascading_connection($REPL_INFO['PACK_UUID']);
			$ALL_INIT = ($CASCADE_INFO['cascading_type'] != 'NoSeries')?$CASCADE_INFO['is_cascading_ready']:true;
			$REPL_INFO['JOBS_JSON'] -> all_init = $ALL_INIT;
			echo json_encode($REPL_INFO);	
		break;		
		
		############################
		# Query Replica Disk Information
		############################
		case "QueryReplicaDiskInformation":
			$ReplUUID = $PostData['ReplUUID'];	
			
			#print_r($ReplMgmt->query_replica_disk($ReplUUID));
			echo json_encode($ReplMgmt->query_replica_disk($ReplUUID));	
		break;

		############################
		# List Available Service
		############################
		case "ListAvailableService":
			$AcctUUID = $PostData['AcctUUID'];
			echo json_encode($SERV_QUERY = $ServiceMgmt -> list_service($AcctUUID,null));
		break;
		
		############################
		# Select Service Replica
		############################
		case "SelectServiceReplica":
			$SelectReplica = explode('|',$PostData['ReplUUID']);
	
			if (isset($SelectReplica[1]) and $SelectReplica[1] == 'true')
			{
				$PLAN_UUID  = $SelectReplica[0];
				$PLAN_QUERY = $ServerMgmt -> query_recover_plan($PLAN_UUID);
				$REPL_UUID  = $PLAN_QUERY['_REPL_UUID'];
				$PLAN_JSON	= $PLAN_QUERY['_PLAN_JSON'];
				$IS_TEMP	= true;
			}
			else
			{
				$PLAN_UUID	= false;
				$REPL_UUID	= $SelectReplica[0];
				$PLAN_JSON	= false;
				$IS_TEMP	= false;
			}
			
			$REPL_QUERY   = $ReplMgmt -> query_replica($REPL_UUID);			
			$TARGET_CONN  = json_decode($REPL_QUERY['CONN_UUID']) -> TARGET;
			$CONN_INFO    = $ServerMgmt -> query_connection_info($TARGET_CONN);
			$LAUNCER_INFO = explode("|",$CONN_INFO['LAUN_OPEN']);
		
			$LAUN_UUID = $LAUNCER_INFO[0];
			if (isset($LAUNCER_INFO[1]))
			{
				$LAUN_REGN = $LAUNCER_INFO[1];
			}
			else
			{
				$LAUN_REGN = 'Neptune';
			}
			
			if ($REPL_QUERY['WINPE_JOB'] == 'N')
			{
				$IS_RCD_JOB = false;
			}
			else
			{
				$IS_RCD_JOB = true;
			}
			
			if ($REPL_QUERY['JOBS_JSON'] -> export_path == '')
			{
				$IS_EXPORT_JOB = false;
			}
			else
			{
				$IS_EXPORT_JOB = true;
			}
			
			$SERV_INFO = array(
							'PLAN_UUID'		=> $PLAN_UUID,
							'SERV_UUID' 	=> $LAUN_UUID,
							'SERV_REGN' 	=> $LAUN_REGN,
							'CLOUD_TYPE' 	=> $CONN_INFO['CLOUD_TYPE'],
							'VENDOR_NAME'	=> $CONN_INFO['VENDOR_NAME'],
							'SERVER_UUID' 	=> $CONN_INFO['LAUN_UUID'],
							'PLAN_JSON' 	=> $PLAN_JSON,							
							'IS_EXPORT_JOB' => $IS_EXPORT_JOB,
							'IS_RCD_JOB'	=> $IS_RCD_JOB,
							'IS_TEMPLATE'	=> $IS_TEMP
						);
			$result = array_merge($REPL_QUERY, $SERV_INFO);
					
			echo json_encode($result);
		break;

		############################
		# Verify Folder Connection
		############################
		case "VerifyFolderConnection":
			$ServADDR = $PostData['ServADDR'];
			$ConnDEST = $PostData['ConnDEST'];
			$InitTYPE = 'Test';
			
			$ConnPath = array('LocalPath' => $ConnDEST, 'WebDavPath' => null);
			$output = $ServiceMgmt -> verify_folder_connection(explode(',',$ServADDR),$ConnPath,$InitTYPE,'LocalFolder');
			
			echo json_encode($output);
		break;
					
		############################
		# Begin To Run Service
		############################
		case "BeginToRunService":
			$AcctUUID 	     = $PostData['AcctUUID'];
			$RegnUUID 	     = $PostData['RegnUUID'];			
			$PlanUUID	     = $PostData['PlanUUID'];
			$RpelUUID 	     = $PostData['ReplUUID'];
			$RecyType	     = $PostData['RecyType'];
			
			if(isset($PostData['ServJobUUID']))
			{
				$ServUUID = $PostData['ServJobUUID'];
			}
			else
			{
				$ServUUID = Misc_Class::guid_v4();
			}			
			
			#REFORMAT SERVICE SETTINGS
			$ServiceSettings = json_decode($PostData['ServiceSettings']);
		
			$output = $ServiceMgmt -> pre_create_launcher_job($AcctUUID,$RegnUUID,$RpelUUID,$ServUUID,$PlanUUID,$RecyType,$ServiceSettings);		

			echo json_encode($output);
		break;
		
		############################
		# Async Begin To Run Service
		############################
		case "BeginToRunServiceAsync":
			$PostData['Action'] 		= 'BeginToRunService';
			$PostData['ServJobUUID'] 	= Misc_Class::guid_v4();
			
			echo json_encode($PostData['ServJobUUID']);

			asynchronous_call($PostData,2500);			
		break;
		
		############################
		# Begin To Run VMware Recovery
		############################
		case "BeginToRunVMWareService":
			$AcctUUID 	     = $PostData['AcctUUID'];
			$RegnUUID 	     = $PostData['RegnUUID'];			
			$PlanUUID	     = $PostData['PlanUUID'];
			$RpelUUID 	     = $PostData['ReplUUID'];
			$RecyType	     = $PostData['RecyType'];
			
			if(isset($PostData['ServJobUUID']))
			{
				$ServUUID = $PostData['ServJobUUID'];
			}
			else
			{
				$ServUUID = Misc_Class::guid_v4();
			}			
			
			#REFORMAT SERVICE SETTINGS
			$ServiceSettings = json_decode($PostData['ServiceSettings']);
		
			$output = $ServiceMgmt -> pre_create_vmware_launcher_job($AcctUUID,$RegnUUID,$RpelUUID,$ServUUID,$PlanUUID,$RecyType,$ServiceSettings);		

			echo json_encode($output);
		break;
		
		############################
		# Async Begin To Run VMware Recovery
		############################
		case "BeginToRunVMWareServiceAsync":
			$PostData['Action'] 		= 'BeginToRunVMWareService';
			$PostData['ServJobUUID'] 	= Misc_Class::guid_v4();
			
			echo json_encode($PostData['ServJobUUID']);

			asynchronous_call($PostData,2500);		
		break;
		
		############################
		# Query Service Information
		############################
		case "QueryServiceInformation":
			$JobUUID = $PostData['JobUUID'];
			
			echo json_encode($ServiceMgmt -> query_service($JobUUID),true);	
		break;
			
		############################
		# Delete Select Recover Job
		############################
		case "DeleteSelectRecover":
			$ServUUID 	 	= $PostData['ServUUID'];
			$InstanceAction	= $PostData['InstanceAction'];
			
			if (!isset($PostData['DeleteSnapshot']))
			{
				$DeleteSnapshot	= 'off';
			}
			else
			{
				$DeleteSnapshot	= $PostData['DeleteSnapshot'];
			}
			
			$output = $ServiceMgmt -> delete_service_job($ServUUID,$InstanceAction,$DeleteSnapshot);
			
			//echo json_encode($output);		
		break;
		
		############################
		# Async Delete Select Recover Job
		############################
		case "DeleteSelectRecoverAsync":
			$PostData['Action'] = 'DeleteSelectRecover';
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# Begin To Run Recover Kit Service
		############################
		case "BeginToRunRecoverKitService":			
			$AcctUUID    = $PostData['AcctUUID'];
			$RegnUUID 	 = $PostData['RegnUUID'];
			$RpelUUID 	 = $PostData['ReplUUID'];
			$ConnUUID	 = $PostData['ConnUUID'];
			$TriggerSync = $PostData['TriggerSync'];
			$ConvertType = $PostData['ConvertType'];
			$AutoReboot  = $PostData['AutoReboot'];
			
			if (isset($TriggerSync) and $TriggerSync == 'on')
			{
				$TriggerSync = true;
			}
			else
			{
				$TriggerSync = false;
			}

			if (isset($AutoReboot) and $AutoReboot == 'on')
			{
				$AutoReboot = true;
			}
			else
			{
				$AutoReboot = false;
			}
			
			if(isset($PostData['ServJobUUID']))
			{
				$ServJobUUID = $PostData['ServJobUUID'];
			}
			else
			{
				$ServJobUUID = Misc_Class::guid_v4();
				echo json_encode($ServJobUUID);
			}			
			
			$ServiceMgmt->pre_create_recover_kit_launcher_job($AcctUUID,$RegnUUID,$RpelUUID,$ConnUUID,$TriggerSync,$AutoReboot,$ConvertType,$ServJobUUID);			
		break;
		
		############################
		# Async Begin To Run Recover Kit Service
		############################
		case "BeginToRunRecoverKitServiceAsync":
			$PostData['Action'] 	 = 'BeginToRunRecoverKitService';
			$PostData['ServJobUUID'] = Misc_Class::guid_v4();
			
			echo json_encode($PostData['ServJobUUID']);
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# Begin To Run Image Export Service
		############################
		case "BeginToRunRecoverImageExportService":			
			$AcctUUID    = $PostData['AcctUUID'];
			$RegnUUID 	 = $PostData['RegnUUID'];
			$RpelUUID 	 = $PostData['ReplUUID'];
			$ConnUUID	 = $PostData['ConnUUID'];
			$ConvertType = $PostData['ConvertType'];
			$TriggerSync = $PostData['TriggerSync'];
			
			if (isset($TriggerSync) and $TriggerSync == 'on')
			{
				$TriggerSync = true;
			}
			else
			{
				$TriggerSync = false;
			}
		
			if(isset($PostData['ServJobUUID']))
			{
				$ServJobUUID = $PostData['ServJobUUID'];
			}
			else
			{
				$ServJobUUID = Misc_Class::guid_v4();
				echo json_encode($ServJobUUID);
			}
		
			$ServiceMgmt -> pre_create_image_export_launcher_job($AcctUUID,$RegnUUID,$RpelUUID,$ConnUUID,$TriggerSync,$ConvertType,$ServJobUUID);
		break;
		
		############################
		# Async Begin To Run Image Export Service
		############################
		case "BeginToRunRecoverImageExportServiceAsync":
			$PostData['Action'] 	 = 'BeginToRunRecoverImageExportService';
			$PostData['ServJobUUID'] = Misc_Class::guid_v4();
			
			echo json_encode($PostData['ServJobUUID']);
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# LIST RECOVER PLAN
		############################
		case "ListAvailableRecoverPlan":
			$AcctUUID = $PostData['AcctUUID'];
			$output = $ServerMgmt -> list_recover_plan($AcctUUID);
			print_r(json_encode($output));
		break;
		
		############################
		# SAVE RECOVER PLAN
		############################
		case "SaveRecoverPlan":
			#$REPL_DISK = $ReplMgmt -> query_replica_disk($PostData['RecoverPlan']['ReplUUID']);
			
			$DISK_STRING = isset( $PostData['RecoverPlan']['volume_uuid'] )?$PostData['RecoverPlan']['volume_uuid']:"";
	
			$REPL_DISK = array_diff(explode(',',$DISK_STRING),array('false'));
			$REPL_DISK = array_values($REPL_DISK);

			for($i=0; $i<count($REPL_DISK); $i++)
			{
				$OPEN_DISK[] = $REPL_DISK[$i];
			}
			
			$PostData['RecoverPlan']['CloudDisk'] = $OPEN_DISK;
		
			$output = $ServerMgmt -> save_recover_plan($PostData);
			print_r(json_encode($output));
		break;
		
		############################
		# UPDATE RECOVER PLAN
		############################
		case "UpdateRecoverPlan":
			#$REPL_DISK = $ReplMgmt -> query_replica_disk($PostData['RecoverPlan']['ReplUUID']);
			
			$DISK_STRING = isset($PostData['RecoverPlan']['volume_uuid'])?$PostData['RecoverPlan']['volume_uuid']:"";
	
			$REPL_DISK = array_diff(explode(',',$DISK_STRING),array('false'));
			$REPL_DISK = array_values($REPL_DISK);

			for($i=0; $i<count($REPL_DISK); $i++)
			{
				$OPEN_DISK[] = $REPL_DISK[$i];
			}
			
			$PostData['RecoverPlan']['CloudDisk'] = $OPEN_DISK;
			
			$output = $ServerMgmt -> update_recover_plan($PostData);
			print_r(json_encode($PostData));
		break;		
		
		############################
		# DELETE RECOVER PLAN
		############################
		case "DeleteRecoverPlan":
			$PlanUUID = $PostData['PlanUUID'];
			$output = $ServerMgmt -> delete_recover_plan($PlanUUID);
			print_r(json_encode($output));
		break;

		############################
		# EDIT RECOVER PLAN
		############################
		case "EditRecoverPlan":
			$PlanUUID = $PostData['PlanUUID'];
			$output = $ServerMgmt -> query_recover_plan($PlanUUID);			
			$PLAN_JSON = json_decode($output['_PLAN_JSON']);			
			print_r(json_encode($PLAN_JSON));
		break;	
				
		############################
		# Query RECOVER PLAN
		############################
		case "QueryRecoverPlan":
			$AcctUUID = $PostData['AcctUUID'];
			$PlanUUID = $PostData['PlanUUID'];
			$output = $ServiceMgmt -> query_service_plan($AcctUUID,$PlanUUID);
			print_r(json_encode($output));
		break;		
		
		############################
		# SERVICE CALLBACK
		############################
		case "ServiceCallback":		
			$ServUUID = $PostData['ServUUID'];
			
			$MESSAGE = $ReplMgmt -> job_msg('Instance is ready.');
			$ReplMgmt -> update_job_msg($ServUUID,$MESSAGE,'Service');
			
			$AcctUUID = $ServiceMgmt -> query_service($ServUUID)['ACCT_UUID'];
			$MailerMgmt -> gen_recovery_notification($AcctUUID,$ServUUID);
		break;
				
		############################
		# TAKE XRAY
		############################
		case "TakeXray":		
			$AcctUUID   = $PostData['AcctUUID'];
			$XrayPATH	= $PostData['XrayPATH'];
			$XrayTYPE	= $PostData['XrayTYPE'];
			$ServUUID	= $PostData['ServUUID'];

			$output = $ServiceMgmt -> take_xray($AcctUUID,$XrayPATH,$XrayTYPE,$ServUUID);
			print_r(json_encode($output));			
		break;
		
		############################
		# QUERY LICENSE
		############################
		case "QueryLicense":		
				
			$output = $ServiceMgmt -> query_license();
			print_r(json_encode($output));			
		break;
		
		############################
		# GET PACKAGE INFO
		############################
		case "GetPackageInfo":		
			$LicenseName	= $PostData['LicenseName'];
			$LicenseEmail	= $PostData['LicenseEmail'];
			$LicenseKey		= $PostData['LicenseKey'];
			
			$output = $ServiceMgmt -> get_package_info($LicenseName,$LicenseEmail,$LicenseKey);
			print_r($output);
		break;
		
		############################
		# QUERY PACKAGE INFO
		############################
		case "QueryPackageInfo":		
			$LicenseKey	= $PostData['LicenseKey'];
			
			$output = $ServiceMgmt -> query_package_info($LicenseKey);
			print_r($output);
		break;
		
		############################
		# REMOVE LICENSE
		############################
		case "RemoveLicense":		
			$LicenseKey	= $PostData['LicenseKey'];
			
			$output = $ServiceMgmt -> remove_license($LicenseKey);
			print_r($output);
		break;
		
		############################
		# ONLINE LICENSE ACTIVATION
		############################
		case "OnlineLicenseActivation":		
			$LicenseName	= $PostData['LicenseName'];
			$LicenseEmail	= $PostData['LicenseEmail'];
			$LicenseKey		= $PostData['LicenseKey'];
						
			$output = $ServiceMgmt -> online_active_license($LicenseName,$LicenseEmail,$LicenseKey);
			print_r(json_encode($output));
		break;
		
		############################
		# OFFLINE LICENSE ACTIVATION
		############################
		case "OfflineLicenseActivation":		
			$LicenseKey  = $PostData['LicenseKey'];
			$LicenseText = $PostData['LicenseText'];
			
			$output = $ServiceMgmt -> offline_active_license($LicenseKey,$LicenseText);
			print_r(json_encode($output));
		break;
		
		############################
		# TEST SMTP
		############################
		case "TestSMTP":
			
			if (count(array_filter($PostData)) == 10)
			{
				$output = $MailerMgmt -> send_verify_email($PostData);
			
				if ($output['status'] == TRUE)
				{
					$output['reason'] = 'SMTP settings verified.';
				}
				else
				{
					$output = array('status' => false, 'reason' => 'Failed to verify SMTP settings.');
				}
			}
			else
			{
				$output = array('status' => false, 'reason' => 'Please fill in all of the required fields.');
			}
			print_r(json_encode($output));
		break;
		
		############################
		# REPORT MGMT
		############################
		case "ReportMgmt":			
			Misc_Class::transport_report_async($PostData);
		break;
		
		############################
		# ASYNC REPORT MGMT
		############################
		case "ReportMgmtAsync":
			$PostData['Action'] = 'ReportMgmt';
	
			asynchronous_call($PostData,2500);
		break;
				
		############################
		# LIST RECOVERY SCRIPT
		############################
		case "MgmtRecoveryScript":
		case "ListRecoveryScript":
			$SCRIPT_LOCATION = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/upload_script/';
			
			#CHECK AND CREATE PATH EXISTS
			if(!file_exists($SCRIPT_LOCATION))
			{
				mkdir($SCRIPT_LOCATION);
			}
			
			$FILE_LIST = array_values(array_diff(scandir($SCRIPT_LOCATION), array('..', '.')));
			$LIST_FILE = null;
			
			if (isset($PostData['FilterType']))
			{
				$FILTE_TYPE = $PostData['FilterType'];
			}
			else
			{
				$FILTE_TYPE = null;
			}
			
			if ($FILTE_TYPE == 'WINDOWS')
			{
				$DEFINE_EXTION = array('zip');
			}
			else
			{
				$DEFINE_EXTION = array('zip','tar','tgz','gz');
			}			
			
			for ($i=0; $i<count($FILE_LIST); $i++)
			{
				$FILE_SIZE = filesize($SCRIPT_LOCATION.$FILE_LIST[$i]);
				$FILE_TIME = filemtime($SCRIPT_LOCATION.$FILE_LIST[$i]);
				$FILE_EXT = explode('.', $FILE_LIST[$i]);
				$FILE_EXT = strtolower(end($FILE_EXT));
	
				if (in_array($FILE_EXT, $DEFINE_EXTION))
				{	
					$LIST_FILE[$FILE_LIST[$i]] = $FILE_SIZE.'@'.$FILE_TIME;
				}
			}				
			print_r(json_encode($LIST_FILE));
		break;
		
		############################
		# FILE UPLOAD
		############################
		case "FileUpload":
			
			#DEFINE SCRIPT PATH
			$SCRIPT_LOCATION = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/upload_script';
		
			#CHECK AND CREATE PATH EXISTS
			if(!file_exists($SCRIPT_LOCATION))
			{
				mkdir($SCRIPT_LOCATION);
			}
	
			$MAX_POST_SIZE = $ServiceMgmt -> convert_to_bytes(ini_get("upload_max_filesize"));			
			if ($MAX_POST_SIZE > $PostData['FileInfo']['size'])
			{		
				$DEFINE_SUPPORT_EXT = array('zip','tar','tgz','gz');	
				$FILE_EXT = explode('.', $PostData['FileInfo']['name']);																#FILE NAME
				$FILE_EXT = strtolower(end($FILE_EXT));																					#FILE EXTION
				
				if (in_array($FILE_EXT, $DEFINE_SUPPORT_EXT))
				{					
					$FILE_NAME = str_replace(' ','', basename($PostData["FileInfo"]["name"],'.'.$FILE_EXT).'-'.time().'.'.$FILE_EXT);	#FILE NAME WITH NO SPACE AND ADD TIMESTAMP
					$SCRIPT_LOCATION = $SCRIPT_LOCATION.'/'.$FILE_NAME;							#SCRIPT LOCATION
					copy($PostData['FileInfo']['tmp_name'],$SCRIPT_LOCATION);															#MOVE TO NEW LOCATION
					$UPLOAD_STATUS = array('status' => true, 'reason' => null);
				}
				else
				{
					$UPLOAD_STATUS = array('status' => false, 'reason' => 'upload file extension must be zip, tar, tgz or gz.');
				}
			}
			else
			{
				$UPLOAD_STATUS = array('status' => false, 'reason' => 'upload file max size is '.ini_get("upload_max_filesize"));
			}
			
			print_r(json_encode($UPLOAD_STATUS));			
		break;
		
		############################
		# DELETE FILE
		############################
		case "DeleteFiles":
			$SCRIPT_LOCATION = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/upload_script/';
			
			#CHECK AND CREATE PATH EXISTS
			if(!file_exists($SCRIPT_LOCATION))
			{
				mkdir($SCRIPT_LOCATION);
			}
			
			if (count($PostData['SelectFiles']) != 0)
			{
				for ($i=0; $i<count($PostData['SelectFiles']); $i++)
				{
					$UNLINK_FILE = $SCRIPT_LOCATION.$PostData['SelectFiles'][$i];
					unlink($UNLINK_FILE);
				}
			}
		break;
		
		
		############################
		# LIST RESTORE BACKUP
		############################
		case "ListRestoreBackup":
			$BACKUP_LOCATION = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/';
		
			#CHECK AND CREATE PATH EXISTS
			if(!file_exists($BACKUP_LOCATION))
			{
				mkdir($BACKUP_LOCATION);
			}
			
			$FILE_LIST = array_values(array_diff(scandir($BACKUP_LOCATION), array('..', '.')));
			$LIST_FILE = null;
		
			$DEFINE_EXTION = array('sql','zip');
			
			for ($i=0; $i<count($FILE_LIST); $i++)
			{
				$FILE_SIZE = filesize($BACKUP_LOCATION.$FILE_LIST[$i]);
				$FILE_TIME = filemtime($BACKUP_LOCATION.$FILE_LIST[$i]);
				$FILE_EXT = explode('.', $FILE_LIST[$i]);
				$FILE_EXT = strtolower(end($FILE_EXT));
	
				if (in_array($FILE_EXT, $DEFINE_EXTION))
				{	
					$LIST_FILE[$FILE_LIST[$i]] = $FILE_SIZE.'@'.$FILE_TIME;
				}
			}				
			print_r(json_encode($LIST_FILE));
		break;
		
		############################
		# RESTORE DATABASE
		############################
		case "RestoreDatabase":
			$RESTORE_FILE = $PostData['RestoreFile'];
			$SECURITY_CODE = $PostData['SecurityCode'];
			
			$FILE_EXT = explode('.', $RESTORE_FILE);
			$FILE_EXT = strtolower(end($FILE_EXT));
			
			switch ($FILE_EXT)
			{
				case "zip":
					$RESPONSE = $MailerMgmt -> extract_archive($RESTORE_FILE,$SECURITY_CODE);
					if ($RESPONSE['Status'] == TRUE)
					{
						$RESTORE_OUTPUT = Misc_Class::retore_database($RESPONSE['SqlFile']);
						unlink($_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/'.$RESTORE_FILE);
						unlink($_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/'.$RESPONSE['SqlFile']);
					}
					else
					{
						$RESTORE_OUTPUT = $RESPONSE;
					}
				break;
				
				case "sql":		
					if ($ServiceMgmt -> check_security_code($SECURITY_CODE) == TRUE)
					{	
						$RESTORE_OUTPUT = Misc_Class::retore_database($RESTORE_FILE);						
						unlink($_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/'.$RESTORE_FILE);
					}
					else
					{
						$RESTORE_OUTPUT = array('Status' => false, 'Msg' => 'Invalid Security code.');
					}
				break;
				
				default:
					$RESTORE_OUTPUT = array('Status' => false, 'Msg' => 'Unsupported file type.');
			}
			
			print_r(json_encode($RESTORE_OUTPUT));
		break;
		
		############################
		# QUERY REPAIR TRANSPORT
		############################
		case "QueryRepairTransport":
			$QueryReplica = $ReplMgmt -> query_replica($PostData['ReplUUID']);
			$CarrierUUID = $ServerMgmt -> query_connection_info(json_decode($QueryReplica['CONN_UUID'],false) -> SOURCE)['CONN_DATA']['CARR_UUID'];
			$ListTransport = $ServerMgmt -> list_server_with_type($QueryReplica['ACCT_UUID'],'Carrier');
			
			$Response = array('SourceTransport' => $CarrierUUID, 'ListTransport' => $ListTransport);
			
			print_r(json_encode($Response));
		break;
		
		############################
		# DEBUG REPAIR REPLICA JOB
		############################
		case "DebugRepairReplica":
			$REPL_UUID 			= $PostData['ReplUUID'];
			$TRANSPORT_UUID 	= $PostData['TargetTransport'];
			$SYNC_MODE 			= $PostData['RepairSyncMode'];
			$SECURITY_CODE 		= $PostData['SecurityCode'];
				
			if ($ServiceMgmt -> check_security_code($SECURITY_CODE) == TRUE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Get the Scheduler job status.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				$JOB_STATUS = $ServiceMgmt -> get_service_job_status($REPL_UUID,'SCHEDULER');
				
				/*
				if ($JOB_STATUS == TRUE)
				{
					#REMOVE SCHEDULER JOB
					$MESSAGE = $ReplMgmt -> job_msg('Remove the Scheduler job.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$ServiceMgmt -> remove_service_job($REPL_UUID,'SCHEDULER');
				}
				*/
				
				if ($JOB_STATUS == 4099)
				{
					$MESSAGE = $ReplMgmt -> job_msg('Begin to repair replication job.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$ServiceMgmt -> repair_replica_job($REPL_UUID,$TRANSPORT_UUID,$SYNC_MODE);
				}
				else
				{
					$MESSAGE = $ReplMgmt -> job_msg('Replication job still exists.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				}
			}
			else
			{
				$MESSAGE = $ReplMgmt -> job_msg('Invalid Security code.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}	
		break;
		
		############################
		# DEBUG REPAIR REPLICA ASYNC JOB
		############################
		case "DebugRepairReplicaAsync":
			$PostData['Action'] = 'DebugRepairReplica';
			
			asynchronous_call($PostData,2500);
		break;
		
		############################
		# DEBUG DELETE REPLICA JOB
		############################
		case "DebugDeleteReplica":
			$REPL_UUID		= $PostData['ReplUUID'];
			$SECURITY_CODE	= $PostData['SecurityCode'];
		
			if ($ServiceMgmt -> check_security_code($SECURITY_CODE) == TRUE)
			{		
				#CHECK RUNNING SERVICE
				$IS_RUNNING = $ServiceMgmt -> check_running_service($REPL_UUID,false);
				if ($IS_RUNNING == FALSE)
				{
					$MESSAGE = $ReplMgmt -> job_msg('Cannot remove the replication process due to the active recovery process.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				}
				else
				{	
					#REMOVE REPLICA JOB MESSAGE
					$MESSAGE = $ReplMgmt -> job_msg('Fource to removed the Replication.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$ReplMgmt -> delete_replica($REPL_UUID);
				}
			}
			else
			{
				$MESSAGE = $ReplMgmt -> job_msg('Invalid Security code.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
		break;
		
		############################
		# DEBUG DELETE SERVICE JOB
		############################
		case "DebugDeleteService":
			$SERV_UUID		= $PostData['ServUUID'];
			$SECURITY_CODE	= $PostData['SecurityCode'];
		
			if ($ServiceMgmt -> check_security_code($SECURITY_CODE) == TRUE)
			{		
				#REMOVE RECOVERY JOB MESSAGE
				$MESSAGE = $ReplMgmt -> job_msg('Fource to removed the Recovery.');
				$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				$ServiceMgmt -> update_service_info($SERV_UUID,'X');
			}
			else
			{
				$MESSAGE = $ReplMgmt -> job_msg('Invalid Security code.');
				$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			}		
		break;
				
		############################
		# CONFIGURE DATAMODE AGENT
		############################
		case "ConfigureDataModeAgent":
			$REPL_UUID     = $PostData['ReplUUID'];
			$DISK_FILTER   = $PostData['DiskFilter'];
			$RECOVERY_MODE = $PostData['RecoveryMode'];
			
			$REPL_DISK_PARTATION = $ReplMgmt -> datamode_agent_disk_mapping($REPL_UUID,$DISK_FILTER,$RECOVERY_MODE);

			print_r(json_encode($REPL_DISK_PARTATION));	
		break;
		
		############################
		# GENERATE DATAMODE AGENT PACKAGE
		############################
		case "GenerateDataModeAgentPackage":
			$AGENT_PATH 	= $PostData['AgentPath'];
			$PARTITION_REF	= $PostData['PartitionRef'];
			$OS_TYPE		= $PostData['OSType'];

			$output = $ServiceMgmt -> generate_datemode_agent($AGENT_PATH,$PARTITION_REF,$OS_TYPE);

			print_r(json_encode($output));	
		break;

		############################
		# GET REPLICA SERVICE NUM
		############################
		case "getReplicaServiceNum":
			$serverId 	= $PostData['serverId'];
			$AcctUUID	= $PostData['AcctUUID'];

			$model = new Common_Model();

			$rNum = $model->getReplicaServiceNum( $serverId );
			
			$output = array("success"=>true, "replicaNum"=>$rNum);

			print_r(json_encode($output));	
		break;
		
		############################
		# GET CASCADED REPLICA INFOMATION
		############################
		case "GetCascadedReplicaInfo":
			$PACKER_UUID = $PostData['PackerUUID'];
			
			$output = $ReplMgmt -> cascading_connection($PACKER_UUID);
			
			print_r(json_encode($output));	
		break;
		
		############################
		# GET TRANSPORT INFO BY MACHINEID
		############################
		case "GetTransportInfoByMachineId":
			$MACHINE_ID = $PostData['MachineId'];
			
			$output = $ServerMgmt -> list_match_service_id($MACHINE_ID);
			
			print_r(json_encode($output));	
		break;
		
		############################
		# DISPLAY NETWORK TOPOLOGY
		############################
		case "NetworkTopology":			
			$JOB_UUID = $PostData['JobUUID'];
			$TYPE	  = $PostData['Type'];
			
			$output = $ReplMgmt -> network_topology($JOB_UUID,$TYPE);
			  
			print_r(json_encode($output));
		break;
	}
}
