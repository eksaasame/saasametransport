<?php
###################################
#
#	SERVER MANAGEMENT
#
###################################
class Server_Class extends Db_Connection
{
	###########################
	#CHECK SERVER EXIST ON DATABASE
	###########################
	public function check_server_exist($ACCT_UUID,$CHECK_KEY,$SERV_TYPE,$LINK_UUID)
	{
		if ($SERV_TYPE == 'Virtual Packer')
		{
			$CHECK_SERVER_EXIST = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_UUID = '".$LINK_UUID."' AND _SERV_TYPE = 'Virtual Packer' AND _SERV_INFO LIKE '%".$CHECK_KEY."%' AND _STATUS = 'Y'";
		}
		else
		{
			$CHECK_SERVER_EXIST = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_INFO LIKE '%".$CHECK_KEY."%' AND _SERV_TYPE = '".$SERV_TYPE."' AND _STATUS = 'Y'";
		}

		$QUERY = $this -> DBCON -> prepare($CHECK_SERVER_EXIST);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{
			return true;
		}
		else
		{
			Misc_Class::function_debug('_mgmt',__FUNCTION__,array('reason' => 'duplicate machine id', 'parameter' => func_get_args()));
			
			foreach($QUERY as $QueryResult)
			{
				return $QueryResult['_SERV_UUID'];
			}			
		}
	}
	
	###########################
	#CHECK HOST EXIST ON DATABASE
	###########################
	public function check_host_exist($ACCT_UUID,$CHECK_KEY)
	{
		$CHECK_HOST_EXIST = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_INFO LIKE '%".$CHECK_KEY."%' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($CHECK_HOST_EXIST);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS == 0)
		{
			return true;
		}
		else
		{
			return false;			
		}
	}
	
	###########################
	#Report Transport Server
	###########################	
	private function report_transport_server($ACCT_UUID,$REGN_UUID,$SERV_UUID,$OPEN_UUID,$HOST_NAME,$ACTION_TYPE)
	{
		$GET_SCHD_UUID = "SELECT * FROM _SERVER WHERE _HOST_NAME = '".$HOST_NAME."' AND _OPEN_UUID = '".$OPEN_UUID."' AND _SERV_TYPE = 'Scheduler' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_SCHD_UUID);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 1)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_UUID = $QueryResult['_SERV_UUID'];
				$SERV_ADDR = $QueryResult['_SERV_ADDR'];
				$SERV_VERS = json_decode($QueryResult['_SERV_INFO'],false) -> version;
			}
		}
		else
		{
			$SERV_UUID = 'NA';
			$SERV_ADDR = 'NA';
			$SERV_VERS = 'NA';
		}
		
		$TRANSPORT_SERVER = array(
									'user_id' 				=> $ACCT_UUID,
									'mgmt_id' 				=> $REGN_UUID,
									'reported_from_mgmt'	=> gmdate('Y-m-d G:i:s', time()),
									'collection'			=> '_SERVER',
									'transport_uuid'		=> $SERV_UUID,
									'transport_open_uuid'	=> $OPEN_UUID,
									'transport_address'		=> $SERV_ADDR,
									'transport_name'		=> $HOST_NAME,
									'transport_version'	    => $SERV_VERS,
									'Usage_Type'			=> $ACTION_TYPE,
									'LogLocation'			=> '_mgmt'
								);
									
		return $TRANSPORT_SERVER;
	}
	
	###########################
	#ADD NEW INITIALIZE DISK
	###########################
	public function new_initialize_disk($HOST_UUID,$DISK_INFO,$TYPE)
	{
		if ($TYPE == 'Physical Packer' or $TYPE == 'Offline Packer')
		{
			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				$DISK_NAME 		= $DISK_INFO[$i] -> friendly_name;
				$DISK_UUID 		= Misc_Class::guid_v4();
				$DISK_SIZE 		= $DISK_INFO[$i] -> size;
				$DISK_URI  		= $DISK_INFO[$i] -> uri;
				$DISK_BUS_TYPE  = $DISK_INFO[$i] -> bus_type;
								
				if ($DISK_BUS_TYPE == 1 or $DISK_BUS_TYPE == 2 or $DISK_BUS_TYPE == 3 or $DISK_BUS_TYPE == 6 or $DISK_BUS_TYPE == 8 or $DISK_BUS_TYPE == 9 or $DISK_BUS_TYPE == 10 or $DISK_BUS_TYPE == 11 or $DISK_BUS_TYPE == 15 or $DISK_BUS_TYPE == 16 or $DISK_BUS_TYPE == 17)
				{				
					$NEW_INIT_DISK = "INSERT
										INTO _SERVER_DISK(
											_ID,
											_HOST_UUID,
											_DISK_UUID,
											_DISK_NAME,
											_DISK_SIZE,
											_DISK_URI,
											_TIMESTAMP,
											_STATUS
										)VALUE(
											'',
											'".$HOST_UUID."',
											'".$DISK_UUID."',
											'".$DISK_NAME."',
											'".$DISK_SIZE."',
											'".$DISK_URI."',
											'".Misc_Class::current_utc_time()."',
											'Y')";
					
					$QUERY = $this -> DBCON -> prepare($NEW_INIT_DISK) -> execute();
				}
			}
		}
		elseif ($TYPE == 'Virtual Packer')
		{
			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				$DISK_NAME = str_replace("'","\'",$DISK_INFO[$i] -> name);
				$DISK_UUID = strtoupper($DISK_INFO[$i] -> id);
				$DISK_SIZE = $DISK_INFO[$i] -> size;
				$DISK_URI  = $DISK_NAME;
				
				$NEW_INIT_DISK = "INSERT
									INTO _SERVER_DISK(
										_ID,
										_HOST_UUID,
										_DISK_UUID,
										_DISK_NAME,
										_DISK_SIZE,
										_DISK_URI,
										_TIMESTAMP,
										_STATUS
									)VALUE(
										'',
										'".$HOST_UUID."',
										'".$DISK_UUID."',
										'".$DISK_NAME."',
										'".$DISK_SIZE."',
										'".$DISK_UUID."',
										'".Misc_Class::current_utc_time()."',
										'Y')";
										
				$QUERY = $this -> DBCON -> prepare($NEW_INIT_DISK) -> execute();
			}
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#	ADD NEW SERVER
	###########################
	public function initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$SERV_INFO,$VMS_INFO,$SYST_TYPE,$SELECTED_VMS,$MGMT_DISK)
	{
		#HOSTNAME
		$HOST_NAME = $SERV_INFO -> client_name;
		
		########################
		#	Initialize Type: 
		#	scheduler; carrier; loader; launcher; esx
		########################
		if ($SERV_TYPE != 'Physical Packer' and $SERV_TYPE != 'Offline Packer')
		{
			#ENCRYPT LOGIN INFOMATION
			$SERV_MISC = array('ADDR' => $HOST_ADDR,'USER' => $HOST_USER,'PASS' => $HOST_PASS);
			$ENCRYPT_SERV_MISC = Misc_Class::encrypt_decrypt('encrypt',json_encode($SERV_MISC));
			
			$INIT_SERV_ADDR = $SERV_ADDR;
			
			if ($SERV_TYPE == 'Virtual Packer')
			{
				$INIT_SERV_ADDR = array($HOST_ADDR); #INITIALIZATION ADDRESS
				$VERIFY_KEY = $VMS_INFO -> uuid;
				#$SERV_UUID = $VMS_INFO -> uuid;
				$SERV_INFO -> hypervisor_uuid = $VMS_INFO -> uuid;
				
				#NO VM HOST SELECTED
				if ($SELECTED_VMS == FALSE)
				{
					return false;
				}
			}
			else
			{
				$INIT_SERV_ADDR = $SERV_ADDR; #INITIALIZATION ADDRESS
				$VERIFY_KEY = $SERV_INFO -> machine_id;				
				//$VERIFY_KEY = json_encode($SERV_ADDR);
			}
			
			#SET AS AZURE MGMT DISK
			$SERV_INFO -> is_azure_mgmt_disk = filter_var($MGMT_DISK,FILTER_VALIDATE_BOOLEAN);		
			
			#CHECK DUPLICATE TRANSPORT SERVICES			
			$CheckServExist = $this -> check_server_exist($ACCT_UUID,$VERIFY_KEY,$SERV_TYPE,$HOST_UUID);
	
			if ($CheckServExist === TRUE)
			{
				#NEW SERVICE UUID
				$SERV_UUID = Misc_Class::guid_v4();
				
				#TRANSPORT SERVER ADDRESS JSON
				$ADDR_JSON = json_encode($INIT_SERV_ADDR);
						
				$SERV_INFO = str_replace('\\','\\\\',json_encode($SERV_INFO,true));
				
				if ($OPEN_UUID != 'ONPREMISE-00000-LOCK-00000-PREMISEON')
				{
					$LOCATION = 'REMOTE_HOST';
				}
				else
				{
					$LOCATION = 'LOCAL_HOST';
				}			
				
				$INSERT_EXEC = "INSERT 
									INTO _SERVER(
										_ID,
										_ACCT_UUID,
										_REGN_UUID,
										_OPEN_UUID,
										_HOST_UUID,
										
										_SERV_UUID,
										
										_SERV_ADDR,
										_HOST_NAME,
										_SERV_INFO,
										_SERV_MISC,
										
										_SERV_TYPE,
										_SYST_TYPE,
										_LOCATION,
										_TIMESTAMP,
										_STATUS)
									VALUE(
										'',
										'".$ACCT_UUID."',
										'".$REGN_UUID."',
										'".$OPEN_UUID."',
										'".$HOST_UUID."',
										
										'".$SERV_UUID."',
										
										'".$ADDR_JSON."',
										'".$HOST_NAME."',
										'".$SERV_INFO."',
										
										'".$ENCRYPT_SERV_MISC."',
										'".$SERV_TYPE."',
										'".$SYST_TYPE."',
										'".$LOCATION."',
										'".Misc_Class::current_utc_time()."',
										'Y')";
				
				$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
				if ($SERV_TYPE == 'Scheduler')
				{
					$REPORT_DATA = $this -> report_transport_server($ACCT_UUID,$REGN_UUID,$SERV_UUID,$OPEN_UUID,$HOST_NAME,'new');
					$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);				
				}
				
				if ($SERV_TYPE != 'Virtual Packer')
				{
					return $SERV_UUID;
				}				
			}
			else
			{
				if ($SERV_TYPE == 'Virtual Packer')
				{				
					$SERV_UUID = $CheckServExist;
				}				
				else
				{
					return false;
				}
			}
		}
		
		########################
		#	Initialize Type: 
		#	physical; offline; virtual Packer
		########################
		if (strpos($SERV_TYPE, 'Packer') == true)
		{
			if ($SERV_TYPE == 'Physical Packer')
			{
				#CHECK DUPLICATE PACKER HOST
				$VERIFY_KEY = $SERV_INFO -> machine_id;
				
				$CheckHostExist = $this -> check_host_exist($ACCT_UUID,$VERIFY_KEY);
				if ($CheckHostExist == TRUE)
				{
				
					#USE ON CLOUD SELECT CLIENT
					if ($HOST_USER != '')
					{					
						$MISC_INFO = explode('|',$HOST_USER);
						$INSTANCE_ID = $MISC_INFO[0];
						$AVAILABILITY_ZONE = $MISC_INFO[1];
						$REG_CLOUD_UUID = $HOST_PASS;
					}
					else
					{
						$INSTANCE_ID = null;
						$AVAILABILITY_ZONE = null;
						$REG_CLOUD_UUID = null;
					}
					
					$INSTANCE_INFO = array('instance_id' => $INSTANCE_ID,'availability_zone' => $AVAILABILITY_ZONE,'reg_cloud_uuid' => $REG_CLOUD_UUID);
					
					$SERV_INFO = (object)array_merge((array)$VMS_INFO,$INSTANCE_INFO);
					
					$SERV_INFO = str_replace('\\','\\\\',json_encode($SERV_INFO,true));
															
					$PACK_UUID = Misc_Class::guid_v4();
					
					$LINK_UUID = '00000000-0000-0000-0000-000000000000';
							
					$INSERT_HOST_EXEC = "INSERT 
											INTO _SERVER_HOST(
												_ID,
												_ACCT_UUID,
												_REGN_UUID,
												
												_HOST_UUID,
												_HOST_ADDR,
												_HOST_NAME,
												_HOST_INFO,
												
												_SERV_UUID,
												_SERV_TYPE,
												_TIMESTAMP,
												_STATUS)
											VALUE(
												'',
												'".$ACCT_UUID."',
												'".$REGN_UUID."',
												
												'".$PACK_UUID."',
												'".$HOST_ADDR."',
												'".$HOST_NAME."',
												'".$SERV_INFO."',
												
												'".$HOST_UUID."',
												'Physical',
												'".Misc_Class::current_utc_time()."',
												'Y')";

					$this -> DBCON -> prepare($INSERT_HOST_EXEC) -> execute();					
					
					#Misc_Class::function_debug('_mgmt',__FUNCTION__,$INSERT_HOST_EXEC);
					
					$DISK_INFO = $VMS_INFO -> disk_infos;
					$this -> new_initialize_disk($PACK_UUID,$DISK_INFO,$SERV_TYPE);
					
					return true;
				}
				else
				{
					return false;
				}
			}
						
			if ($SERV_TYPE == 'Virtual Packer')
			{
				$ServiceMgmt = new Service_Class();
				foreach ($SELECTED_VMS as $VM_INDEX => $VM_KEY)
				{
					$CheckHostExist = $this -> check_host_exist($ACCT_UUID,$VM_KEY);
					
					if ($CheckHostExist == TRUE)
					{
						/* Add New VM Host*/
						for ($i=0; $i<count($SERV_ADDR); $i++)
						{						
							$GET_VMS_INFO = $ServiceMgmt -> virtual_vms_info($SERV_ADDR[$i],$HOST_ADDR,$HOST_USER,$HOST_PASS,$VM_KEY);
														
							if ($GET_VMS_INFO != false)
							{
								break;
							}						
						}
						
						if ($GET_VMS_INFO -> guest_ip != '')
						{
							$GUEST_ADDR = $GET_VMS_INFO -> guest_ip; 
						}
						else
						{
							$GUEST_ADDR = 'None';
						}
						
						#GET HOSTNAME
						$HOST_NAME = $GET_VMS_INFO -> name;
						
						$GET_VMS_INFO -> priority_addr = '127.0.0.1';
						$GET_VMS_INFO -> direct_mode = true;
						$GET_VMS_INFO -> manufacturer = 'VMware, Inc.';
						
						#REMOVE ANNOTATION OBJECT
						$ANNOTATION = $GET_VMS_INFO -> annotation;
						unset($GET_VMS_INFO -> annotation);
							
						$HOST_INFO = addslashes(json_encode($GET_VMS_INFO,JSON_UNESCAPED_UNICODE));
						
						#$PACK_HOST = Misc_Class::guid_v4();
						$PACK_HOST = $GET_VMS_INFO -> uuid;
						
						Misc_Class::function_debug('_mgmt',__FUNCTION__,$PACK_HOST);
						
						$INSERT_HOST_EXEC = "INSERT 
											INTO _SERVER_HOST(
												_ID,
												_ACCT_UUID,
												_REGN_UUID,
												
												_HOST_UUID,
												_HOST_ADDR,
												_HOST_NAME,
												_HOST_INFO,
												
												_SERV_UUID,
												_SERV_TYPE,
												_TIMESTAMP,
												_STATUS)
											VALUE(
												'',
												'".$ACCT_UUID."',
												'".$REGN_UUID."',
												
												'".$PACK_HOST."',
												'".$GUEST_ADDR."',
												'".addslashes($HOST_NAME)."',
												'".$HOST_INFO."',
											
												'".$SERV_UUID."',
												'Virtual',
												'".Misc_Class::current_utc_time()."',
												'Y')";
											
						$this -> DBCON -> prepare($INSERT_HOST_EXEC) -> execute();
								
						$DISK_INFO = $GET_VMS_INFO -> disks;						
						usort($DISK_INFO, array($this,"usort_disk_by_key"));						
						$this -> new_initialize_disk($PACK_HOST,$DISK_INFO,$SERV_TYPE);						
					}
				}
				return $SERV_UUID;
			}
			
			if ($SERV_TYPE == 'Offline Packer')
			{
				$OS_DETECTION = $this -> detect_packer_os_type($VMS_INFO -> volume_infos);
				
				if ($OS_DETECTION == 'LX')
				{
					//$VMS_INFO -> os_name = str_replace('Microsoft Windows','SaaSaMe Linux', $VMS_INFO -> os_name);
					$VMS_INFO -> os_name = 'unknown OS type';
				}
		
				$SERV_INFO = str_replace('\\','\\\\',json_encode($VMS_INFO,true));
			
				$PACK_HOST = Misc_Class::guid_v4();
				
				$LINK_UUID = '00000000-0000-0000-0000-000000000000';
							
				$INSERT_HOST_EXEC = "INSERT 
										INTO _SERVER_HOST(
											_ID,
											_ACCT_UUID,
											_REGN_UUID,
											
											_HOST_UUID,
											_HOST_ADDR,
											_HOST_NAME,
											_HOST_INFO,
											
											_SERV_UUID,
											_SERV_TYPE,
											_TIMESTAMP,
											_STATUS)
										VALUE(
											'',
											'".$ACCT_UUID."',
											'".$REGN_UUID."',
											
											'".$PACK_HOST."',
											'".$HOST_ADDR."',
											'".$HOST_NAME."',
											'".$SERV_INFO."',
											
											'".$HOST_UUID."',
											'Offline',
											'".Misc_Class::current_utc_time()."',
											'Y')";
			
				$this -> DBCON -> prepare($INSERT_HOST_EXEC) -> execute();					
				
				$DISK_INFO = $VMS_INFO -> disk_infos;
				$this -> new_initialize_disk($PACK_HOST,$DISK_INFO,$SERV_TYPE);
				
				return true;
			}				
		}
		else
		{
			return $SERV_UUID;
		}
	}
	
	###########################
	#SORT DISK BY KEY
	###########################
	private function usort_disk_by_key($KEY_A,$KEY_B)
	{
		return strcmp($KEY_A->key, $KEY_B->key);
	}
	
	###########################
	#DETECT PACKER OS TYPE
	###########################
	private function detect_packer_os_type($VOLUME_INFOS)
	{
		for ($i=0; $i<count($VOLUME_INFOS); $i++)
		{
			if ($VOLUME_INFOS[$i] -> file_system == 'NTFS')
			{
				return 'WIN';
				break;
			}
		}
		return 'LX';
	}
		
	###########################
	#DELETE HOST PACKER DISK
	###########################
	private function delete_packer_disk($HOST_UUID)
	{
		$DELETE_HOST_DISK = "UPDATE _SERVER_DISK
							 SET
								_TIMESTAMP = '".Misc_Class::current_utc_time()."',
								_STATUS    = 'X'
							WHERE
								_HOST_UUID = '".$HOST_UUID."'";
								
		$this -> DBCON -> prepare($DELETE_HOST_DISK) -> execute();
	}
	
	###########################
	#DELETE SELECT PACKER HOST
	###########################
	public function delete_packer_host($ACCT_UUID,$HOST_UUID)
	{
		#CHECK RUNNING REPLICA
		$COUNT_RUNNING_REPLICA = $this -> check_prepare_wordload($ACCT_UUID,$HOST_UUID)['job_count'];
		
		if ($COUNT_RUNNING_REPLICA == 0)
		{
			$HOST_INFO = $this -> query_host_info($HOST_UUID);
			$HOST_TYPE = $HOST_INFO['HOST_TYPE'];
					
			#DELETE HOST DISK
			$this -> delete_packer_disk($HOST_UUID);
			
			$DELETE_HOST = "UPDATE _SERVER_HOST
							SET
								_TIMESTAMP = '".Misc_Class::current_utc_time()."',
								_STATUS    = 'X'
							WHERE
								_ACCT_UUID = '".$ACCT_UUID."' AND
								_HOST_UUID = '".$HOST_UUID."'";
										
			$this -> DBCON -> prepare($DELETE_HOST) -> execute();
						
			if ($HOST_TYPE == 'Virtual')
			{
				$SERV_UUID[] = $HOST_INFO['HOST_SERV']['SERV_UUID'];
				$HOST_COUNT = $this -> check_register_packer($SERV_UUID);
				if ($HOST_COUNT == TRUE)
				{
					$DELETE_ESX_SERV = "UPDATE _SERVER 
									    SET
											_TIMESTAMP = '".Misc_Class::current_utc_time()."',
											_STATUS    = 'X'
										WHERE
											_SYST_TYPE = 'ESX' AND
											_ACCT_UUID = '".$ACCT_UUID."' AND
										    _SERV_UUID = '".$SERV_UUID[0]."'";
					$this -> DBCON -> prepare($DELETE_ESX_SERV) -> execute();
				}		
			}
			else
			{
				#DELETE HOST IN-DIRECT MODE
				if ($HOST_INFO['HOST_INFO']['direct_mode'] == FALSE)
				{
					$MACHINE_UUID = $HOST_INFO['HOST_INFO']['machine_id'];
				
					$ServiceMgmt = new Service_Class();
				
					$ServiceMgmt -> unregister_packer($MACHINE_UUID);
				}
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	
	###########################	
	#ADD ALL-IN-ONE SERVICES
	###########################
	public function initialize_servers($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERV_ADDR,$SELT_SERV,$SYST_TYPE,$MGMT_ADDR,$MGMT_DISK)
	{
		#Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		if ($HOST_UUID == 'ONPREMISE-00000-LOCK-00000-PREMISEON')
		{
			$HOST_UUID = Misc_Class::guid_v4();
		}
		
		$ServiceMgmt = new Service_Class();
		#$SERV_ADDR = array_reverse(array_unique(array_reverse(explode(',', $SERV_ADDR))));
		$SERV_ADDR = array_unique(explode(',', $SERV_ADDR));
		$SERV_TYPE = array_unique(explode(',', $SELT_SERV));

		#GET SERV INFO
		$StatusCode = false;
		for ($x=0; $x<count($SERV_ADDR); $x++)
		{
			try{
				$SERV_INFO = $ServiceMgmt -> get_connection($SERV_ADDR[$x],'Loader',0,$MGMT_ADDR);
		
				$NETWORK_INFO = $SERV_INFO -> network_infos;
				
				for($n=0; $n<count($NETWORK_INFO); $n++)
				{
					$NET_IP_ADDR[] = $NETWORK_INFO[$n] -> ip_addresses;
				}
					
				$IP_ADDRESS = call_user_func_array('array_merge', $NET_IP_ADDR);
				
				$SERVER_ADDRESS = array_values(array_unique(array_merge($SERV_ADDR,$IP_ADDRESS)));	
				
				if ($SERV_INFO != FALSE)
				{
					#SET AS DIRECT MODE
					$SERV_INFO -> direct_mode = true;
					
					for ($i=0; $i<count($SERV_TYPE); $i++)
					{
						$ADD_SERVICE = $this -> initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERVER_ADDRESS,null,null,null,$SERV_TYPE[$i],$SERV_INFO,null,$SYST_TYPE,false,$MGMT_DISK);
						if ($ADD_SERVICE != FALSE)
						{
							$DATA_TYPE = array('Scheduler','Carrier','Loader','Launcher');
							$UUID_TYPE = array('SCHD_UUID','CARR_UUID','LOAD_UUID','LAUN_UUID');
							$SASA_TYPE = str_replace($DATA_TYPE,$UUID_TYPE,$SERV_TYPE[$i]);
						
							$ADD_SERV[$SASA_TYPE] = $ADD_SERVICE;
							$StatusCode = true;
						}
					}
					break;
				}	
			}
			catch (Throwable $e){
					
			}
		}
/*
		for ($i=0; $i<count($SERV_TYPE); $i++)
		{
			$DATA_TYPE = array('Scheduler','Carrier','Loader','Launcher');
			$UUID_TYPE = array('SCHD_UUID','CARR_UUID','LOAD_UUID','LAUN_UUID');
			$SASA_TYPE = str_replace($DATA_TYPE,$UUID_TYPE,$SERV_TYPE[$i]);
			
			#GET SERV INFO
			$StatusCode = false;
			for ($x=0; $x<count($SERV_ADDR); $x++)
			{
				try{
					$SERV_INFO = $ServiceMgmt -> get_connection($SERV_ADDR[$x],$SERV_TYPE[$i],0,$MGMT_ADDR);
						
					$NETWORK_INFO = $SERV_INFO -> network_infos;
					
					for($n=0; $n<count($NETWORK_INFO); $n++)
					{
						$NET_IP_ADDR[] = $NETWORK_INFO[$n] -> ip_addresses;
					}
					
					$IP_ADDRESS = call_user_func_array('array_merge', $NET_IP_ADDR);
				
					$SERVER_ADDRESS = array_values(array_unique(array_merge($SERV_ADDR,$IP_ADDRESS)));	
				
					if ($SERV_INFO == FALSE)
					{
						break 2;
					}
					else
					{
						#SET AS DIRECT MODE
						$SERV_INFO -> direct_mode = true;
						
						$ADD_SERVICE = $this -> initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERVER_ADDRESS,null,null,null,$SERV_TYPE[$i],$SERV_INFO,null,$SYST_TYPE,false,$MGMT_DISK);
						if ($ADD_SERVICE != FALSE)
						{						
							$ADD_SERV[$SASA_TYPE] = $ADD_SERVICE;
							$StatusCode = true;
							break;
						}
					}	
				}
				catch (Throwable $e){
					
				}
			}
		}
*/
		if ($StatusCode == TRUE)
		{	
			return $ADD_SERV;
		}
		else
		{
			return false;
		}
	}
	
	private function select_self_emulator_launcher($ACCT_UUID,$SERV_QUERY)
	{
		$MACHINE_ID = json_decode( $SERV_QUERY["_SERV_INFO"], true)["machine_id"];
		
		$SELF_PAIR_LAUNCHER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_INFO LIKE '%".$MACHINE_ID."%' AND _SERV_ADDR = '".$SERV_QUERY["_SERV_ADDR"]."' AND _SERV_TYPE = 'Launcher' AND _SYST_TYPE = 'WINDOWS' AND _STATUS = 'Y' LIMIT 1";
		$QUERY = $this -> DBCON -> prepare($SELF_PAIR_LAUNCHER);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$LAUNCHER_UUID = $QueryResult['_SERV_UUID'];
			}
		}
		else
		{
			$LAUNCHER_UUID = false;
		}		
		return $LAUNCHER_UUID;
	}
	
	
	###########################
	#SMART QUERY AVAILABLE TARGET
	###########################
	private function smart_select_pair_launcher($ACCT_UUID,$OPEN_UUID,$HOST_UUID,$SYST_TYPE,$HOST_NAME,$IS_WINPE,$HOST_REGION,$SERV_QUERY)
	{		
		if ($IS_WINPE == FALSE)
		{
			if ($SYST_TYPE == 'WINDOWS')
			{
				$PAIR_LAUNCHER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_UUID LIKE '%".$HOST_UUID."%' AND _SERV_TYPE = 'Launcher' AND _SYST_TYPE = '".$SYST_TYPE."' AND _STATUS = 'Y' LIMIT 1";
			}
			else
			{
				if ($HOST_REGION == 'NO_REGION')
				{
					$PAIR_LAUNCHER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _OPEN_UUID = '".$OPEN_UUID."' AND _SERV_TYPE = 'Launcher' AND _SYST_TYPE = '".$SYST_TYPE."' AND _STATUS = 'Y' LIMIT 1";
				}
				else
				{
					$PAIR_LAUNCHER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _OPEN_UUID = '".$OPEN_UUID."' AND _HOST_UUID LIKE '%".$HOST_REGION."%' AND _SERV_TYPE = 'Launcher' AND _SYST_TYPE = '".$SYST_TYPE."' AND _STATUS = 'Y' LIMIT 1";
				}			
			}			
		}		
		else
		{
			$PAIR_LAUNCHER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_NAME = '".$HOST_NAME."' AND _SERV_TYPE = 'Launcher' AND _STATUS = 'Y' LIMIT 1";
		}
	
		$QUERY = $this -> DBCON -> prepare($PAIR_LAUNCHER);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$LAUNCHER_UUID = $QueryResult['_SERV_UUID'];
			}
			
			if ($IS_WINPE == TRUE)
			{
				$CHECK_CONNECTION = "SELECT * FROM _SERVER_CONN WHERE _LAUN_UUID = '".$LAUNCHER_UUID."' AND _STATUS = 'Y'";
				$QUERY_WINPE = $this -> DBCON -> prepare($CHECK_CONNECTION);
				$QUERY_WINPE -> execute();
				$WINPE_COUNT_ROWS = $QUERY_WINPE -> rowCount();
				if ($WINPE_COUNT_ROWS != 0)
				{
					foreach($QUERY_WINPE as $WinpeQueryResult)
					{
						$CONN_UUID = $WinpeQueryResult['_CONN_UUID'];
					}
					
					$LIST_IN_USE_CONN = $this -> check_in_use_connection($ACCT_UUID);

					if ($LIST_IN_USE_CONN != FALSE)
					{
						for ($i=0; $i<count($LIST_IN_USE_CONN); $i++)
						{
							foreach ($LIST_IN_USE_CONN[$i] as $LIST_SERV_UUID)
							{	
								if ($LIST_SERV_UUID == $LAUNCHER_UUID)
								{
									return false;
									break 2;
								}
							}
						}						
						return $LAUNCHER_UUID;
					}
					else
					{
						return $LAUNCHER_UUID;
					}				
				}
				else
				{
					return $LAUNCHER_UUID;
				}				
			}
			else
			{
				return $LAUNCHER_UUID;
			}
		}
		else
		{
			return $this -> select_self_emulator_launcher($ACCT_UUID,$SERV_QUERY);
		}
	}
	
	###########################
	#LIST SERVERS WITH TYPE
	###########################
	public function list_server_with_type($ACCT_UUID,$SERV_TYPE=null,$SYST_TYPE=null)
	{
		if ($SERV_TYPE == null)
		{
			$GET_EXEC = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		}		
		else
		{
			$GET_EXEC = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = '".$SERV_TYPE."' AND _STATUS = 'Y'";
		}

		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			$COUNT = 1;
			foreach($QUERY as $QueryResult)
			{
				$HOSTNAME = $QueryResult['_HOST_NAME'];

				$SERV_INFO = $this -> upgrade_server_info(json_decode($QueryResult['_SERV_INFO']));
		
				$WINPE = $SERV_INFO -> is_winpe;
				
				if ($QueryResult['_LOCATION'] == 'REMOTE_HOST')
				{
					$LOCATION = 'Cloud';
				}
				else
				{					
					if ($WINPE == FALSE)
					{
						$LOCATION = 'On Premise';
					}
					else
					{
						$LOCATION = 'Local Recover Kit';
					}					
				}
				
				#CLOUD UUID
				$OPEN_UUID = $QueryResult['_OPEN_UUID'];
				
				#VENDOR NAME
				$VENDOR = $this -> query_cloud_type($OPEN_UUID) -> vendor_type;
				
				#GET SERVER VERSION
				$SERV_VERSION = $SERV_INFO -> version;

				$HOST_INFO = explode('|',$QueryResult['_HOST_UUID']);
				$HOST_UUID = $HOST_INFO[0];
				if (isset($HOST_INFO[1]))
				{
					$HOST_REGN = $HOST_INFO[1];
					$HOST_TYPE = 'AWS';
				}				
				elseif(end($HOST_INFO) == 'Azure')
				{
					$HOST_REGN = $HOST_INFO[1];
					$HOST_TYPE = $HOST_INFO[2];
				}
				elseif($VENDOR == 'AzureBlob')
				{
					if ($SERV_INFO -> is_promote == true)
					{
						$HOST_REGN = $SERV_INFO -> location;
						$HOST_TYPE = 'Azure On-Premises';
					}
					else
					{
						$HOST_REGN = $HOST_INFO[1];
						$HOST_TYPE = $HOST_INFO[2];
					}
				}
				elseif ($VENDOR == 'Ctyun')
				{
					$HOST_REGN = 'NO_REGION';
					$HOST_TYPE = 'Ctyun';
				}
				else
				{
					$HOST_REGN = 'NO_REGION';
					if ($LOCATION != 'Cloud')
					{
						$HOST_TYPE = 'ONPREMISE';
					}
					else
					{
						$HOST_TYPE = 'OPENSTACK';
					}
				}
				
				if( end( $HOST_INFO ) == 'Azure' || end( $HOST_INFO ) == 'Aliyun' || end( $HOST_INFO ) == 'Tencent' )
				{
					$HOST_TYPE = $HOST_INFO[2];
				}
				
				
				#GET LISTING REPLICA
				$LIST_REPLICA = $this -> query_listing_replica_by_server_id($QueryResult['_SERV_UUID'],'LOADER');
				
				#SMART PAIR LAUNCHER				
				$PAIR_LAUNCHER = $this -> smart_select_pair_launcher($ACCT_UUID,$OPEN_UUID,$HOST_UUID,$SYST_TYPE,$HOSTNAME,$WINPE,$HOST_REGN,$QueryResult);
				
				$SERV_JSON[] = array(
					'ID'		   => $COUNT,
					'ACCT_UUID'    => $QueryResult['_ACCT_UUID'],
					'REGN_UUID'    => $QueryResult['_REGN_UUID'],
					'OPEN_UUID'    => $OPEN_UUID,
					'HOST_UUID'    => $HOST_UUID,
					'HOST_REGN'    => $HOST_REGN,
					'HOST_TYPE'    => $HOST_TYPE,
					'SERV_UUID'    => $QueryResult['_SERV_UUID'],
					'HOST_NAME'    => $QueryResult['_HOST_NAME'],
					'SERV_ADDR'    => json_decode($QueryResult['_SERV_ADDR']),
					'SERV_LOCA'    => $LOCATION,
					'SERV_TYPE'    => $QueryResult['_SERV_TYPE'],
					'SERV_VERN'    => $SERV_VERSION,
					'SYST_TYPE'	   => $QueryResult['_SYST_TYPE'],
					'SERV_INFO'    => $SERV_INFO,
					'LIST_REPLICA' => $LIST_REPLICA,
					'LAUN_UUID'    => $PAIR_LAUNCHER,
					'VENDOR_NAME'  => $VENDOR
				);									
				$COUNT = $COUNT + 1;
			}
			return $SERV_JSON;
		}
		else
		{
			return false;
		}
	}
	
	
	###########################
	#QUERY LISTING REPLICA BY SERVER ID
	###########################
	public function query_listing_replica_by_server_id($SERVICE_UUID,$SERVER_TYPE)
	{
		if ($SERVER_TYPE == 'LOADER')
		{
			$SERVER_TYPE = '_LOAD_UUID';
		}
		else
		{
			$SERVER_TYPE = '_LAUN_UUID';
		}	
		
		$QUERY_RUNNING_REPLICA = "SELECT 
										_REPLICA._REPL_UUID,
										_REPLICA._PACK_UUID
								  FROM 
										_REPLICA 
								  WHERE 
										EXISTS
											(
												SELECT 
													_SERVER_CONN._CONN_UUID 
												FROM 
													_SERVER_CONN
												WHERE
													_SERVER_CONN.".$SERVER_TYPE." = '".$SERVICE_UUID."' 
												AND 
													_SERVER_CONN._STATUS = 'Y' AND _REPLICA._CONN_UUID LIKE CONCAT('%', _SERVER_CONN._CONN_UUID, '%')
											)
										AND 
											_REPLICA._STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($QUERY_RUNNING_REPLICA);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$LIST_REPLICA[$QueryResult['_REPL_UUID']] = $QueryResult['_PACK_UUID'];
			}		
			return $LIST_REPLICA;
		}
		else
		{
			return false;
		}
	}		
	
	
	###########################
	#QUERY REPLICA WITH HOST
	###########################
	private function check_prepare_wordload($ACCT_UUID,$PACK_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _REPLICA WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _PACK_UUID = '".$PACK_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{
			return array('init_carrier' => false, 'job_count' => 0);
		}
		else
		{
			foreach($QUERY as $QueryResult)
			{
				$INIT_CARRIER = (isset(json_decode($QueryResult['_JOBS_JSON']) -> init_carrier))?json_decode($QueryResult['_JOBS_JSON']) -> init_carrier:false;
				
				if ($INIT_CARRIER == TRUE)
				{
					break;
				}			
			}
			return array('init_carrier' => $INIT_CARRIER, 'job_count' => $COUNT_ROWS);
		}
	}
	
	###########################
	#LIST HOST WITH TYPE
	###########################
	public function list_host_with_type($ACCT_UUID,$SERV_TYPE=null)
	{
		if ($SERV_TYPE == null)
		{
			$GET_EXEC = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		}		
		else
		{
			if ($SERV_TYPE == 'Physical Packer')
			{
				$GET_EXEC = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = 'Physical' AND _STATUS = 'Y'";
			}
			else
			{
				$GET_EXEC = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = 'Virtual' AND _STATUS = 'Y'";
			}
		}

		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			$COUNT = 1;
			$IS_DIRECT = true;
			$PRIORITY_ADDR = '0.0.0.0';
			foreach($QUERY as $QueryResult)
			{
				if ($QueryResult['_SERV_TYPE'] == 'Physical')
				{
					$HOST_INFO = json_decode($QueryResult['_HOST_INFO'],true);
				
					$PACK_NAME 		= $QueryResult['_HOST_NAME'];
					$PACK_ADDR 		= $QueryResult['_HOST_ADDR'];
					$SERV_MISC 		= $this -> upgrade_host_info($HOST_INFO);
					$REAL_NAME 		= $SERV_MISC['client_name'];
					$IS_DIRECT 		= $SERV_MISC['direct_mode'];
								
					if (strpos($HOST_INFO['os_name'],'Microsoft') !== FALSE)
					{
						$OS_TYPE = 'WINDOWS';
					}
					else
					{
						$OS_TYPE = 'LINUX';
					}
					
					if (isset($SERV_MISC['priority_addr']))
					{
						$PRIORITY_ADDR	= $SERV_MISC['priority_addr'];
					}
					else
					{
						$PRIORITY_ADDR = '0.0.0.0';
					}
				}
				else
				{
					$PACK_INFO = $this -> query_server_info($QueryResult['_SERV_UUID']);
					$PACK_NAME = $PACK_INFO['HOST_NAME'];
					$PACK_ADDR = $PACK_INFO['SERV_ADDR'];
					$SERV_MISC = $PACK_INFO['SERV_MISC'];
			
					if (isset(json_decode($QueryResult['_HOST_INFO'],false) -> guest_os_name))
					{	
						$OS_NAME = json_decode($QueryResult['_HOST_INFO'],false) -> guest_os_name;
						$REAL_NAME = json_decode($QueryResult['_HOST_INFO'],false) -> name;
					}
					else
					{
						if (isset(json_decode($QueryResult['_HOST_INFO'],false) -> os_name))
						{
							$OS_NAME = json_decode($QueryResult['_HOST_INFO'],false) -> os_name;
						}
						else
						{
							$OS_NAME = 'LINUX';
						}
						$REAL_NAME = json_decode($QueryResult['_HOST_INFO'],false) -> client_name;
					}
					
					if (strpos($OS_NAME,'Microsoft') !== FALSE)
					{
						$OS_TYPE = 'WINDOWS';
					}
					else
					{
						$OS_TYPE = 'LINUX';
					}
					
					$IS_DIRECT 		= json_decode($QueryResult['_HOST_INFO'],false) -> direct_mode;
					$PRIORITY_ADDR 	= json_decode($QueryResult['_HOST_INFO'],false) -> priority_addr;
				}
		
				$DISK_INFO = $this -> query_host_disk($QueryResult['_HOST_UUID'],null);
				$DISK_SIZE = 0;
				for ($i=0; $i<count($DISK_INFO); $i++)
				{
					$DISK_SIZE = $DISK_SIZE + $DISK_INFO[$i]['DISK_SIZE'];
				}
				
				#CHECK RUNNING WORKLOAD
				$RUNNING_CHECK = $this -> check_prepare_wordload($QueryResult['_ACCT_UUID'],$QueryResult['_HOST_UUID']);
	
				$SERV_JSON[] = array(
									'ID'			=> $COUNT,
									'ACCT_UUID' 	=> $QueryResult['_ACCT_UUID'],
									'REGN_UUID' 	=> $QueryResult['_REGN_UUID'],
									'HOST_UUID' 	=> $QueryResult['_HOST_UUID'],
									'HOST_NAME' 	=> $QueryResult['_HOST_NAME'],
									'HOST_ADDR' 	=> $QueryResult['_HOST_ADDR'],
									'SERV_UUID' 	=> $QueryResult['_SERV_UUID'],
									'OS_TYPE'   	=> $OS_TYPE,
									'PACK_NAME' 	=> $PACK_NAME,
									'PACK_ADDR' 	=> $PACK_ADDR,
									'SERV_MISC' 	=> $SERV_MISC,
									'DISK_SIZE' 	=> $DISK_SIZE,
									'HOST_TYPE' 	=> $QueryResult['_SERV_TYPE'],
									'REAL_NAME'		=> $REAL_NAME,
									'PROTECTED' 	=> ($RUNNING_CHECK['job_count'] == 0)?false:true,
									'PROTECTED_NUM'	=> $RUNNING_CHECK['job_count'],
									'INIT_CARRIER'	=> $RUNNING_CHECK['init_carrier'],
									'PRIORITY_ADDR'	=> $PRIORITY_ADDR,
									'IS_DIRECT' 	=> $IS_DIRECT
								);									
				$COUNT = $COUNT + 1;
			}
			return $SERV_JSON;
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	#QUERY SERVICE INFORMATION
	###########################
	public function query_server_info($SERV_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVER WHERE _SERV_UUID = '".$SERV_UUID."' AND _STATUS = 'Y'";
	
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();		
		if ($COUNT_ROWS == 1)
		{
			$QUERY_LOCAL_FOLDER_CONN = "SELECT * FROM _SERVER_CONN WHERE _LAUN_UUID = '".$SERV_UUID."' AND _CONN_TYPE = 'LocalFolder' AND _STATUS = 'Y'";
			$QUERY_CONN = $this -> DBCON -> prepare($QUERY_LOCAL_FOLDER_CONN);
			$QUERY_CONN -> execute();
			$COUNT_CONN_ROWS = $QUERY_CONN -> rowCount();	
			
			if ($COUNT_CONN_ROWS == 1)
			{
				$LOCAL_CONN_UUID = $QUERY_CONN -> fetch()['_CONN_UUID'];
			}
			else
			{
				$LOCAL_CONN_UUID = false;
			}		
			
			foreach($QUERY as $QueryResult)
			{
				$LIST_ADDR = json_decode($QueryResult['_SERV_ADDR'],false);
				for ($i=0; $i<count($LIST_ADDR); $i++)
				{
					$LIST_ADDRESS[$LIST_ADDR[$i]] = $i;
				}
				
				/* FOR UPGRADE BELOW 800 */
				$SERV_INFO = $this -> upgrade_server_info(json_decode($QueryResult['_SERV_INFO'],true));
			
				#VENDOR NAME
				$VENDOR = $this -> query_cloud_type($QueryResult['_OPEN_UUID']) -> vendor_type;
			
				$SERV_SUMMARY = array(
									  'ACCT_UUID'   => $QueryResult['_ACCT_UUID'],
									  'REGN_UUID'   => $QueryResult['_REGN_UUID'],
									  'SERV_UUID'   => $QueryResult['_SERV_UUID'],
									  'OPEN_UUID'   => $QueryResult['_OPEN_UUID'],
									  'OPEN_HOST'   => (isset($SERV_INFO['location']))?$QueryResult['_HOST_UUID'].'|'.$SERV_INFO['location']:$QueryResult['_HOST_UUID'],
									  'HOST_NAME'   => $QueryResult['_HOST_NAME'],
									  'SERV_ADDR'   => json_decode($QueryResult['_SERV_ADDR'],false),
									  'SERV_INFO'   => $SERV_INFO,
									  'SERV_MISC'   => json_decode(Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_SERV_MISC']),true),
									  'SERV_TYPE'   => $QueryResult['_SERV_TYPE'],
									  'LIST_ADDR'   => $LIST_ADDRESS,
									  'LOCAL_CONN'  => $LOCAL_CONN_UUID, 
									  'SYST_TYPE'   => $QueryResult['_SYST_TYPE'],
									  'VENDOR_NAME' => $VENDOR,
									);
			}
			return $SERV_SUMMARY;
		}
		else
		{
			return false;
		}	
	}
	
	###########################
	#UPGRADE SERVER INFO
	###########################
	private function upgrade_server_info($SERV_INFO)
	{
		if (is_array($SERV_INFO))
		{
			if(!isset($SERV_INFO['direct_mode']))
			{
				$SERV_INFO['direct_mode'] = true;
			}
			
			if(!isset($SERV_INFO['is_azure_mgmt_disk']))
			{
				$SERV_INFO['is_azure_mgmt_disk'] = true;
			}
			
			if(!isset($SERV_INFO['is_promote']))
			{
				$SERV_INFO['is_promote'] = false;
			}
		}
		
		if (is_object($SERV_INFO))
		{
			if (!isset($SERV_INFO -> direct_mode))
			{
				$SERV_INFO -> direct_mode = true;
			}
			
			if (!isset($SERV_INFO -> is_azure_mgmt_disk))
			{
				$SERV_INFO -> is_azure_mgmt_disk = true;
			}
			
			if (!isset($SERV_INFO -> is_promote))
			{
				$SERV_INFO -> is_promote = false;
			}
		}
		
		return $SERV_INFO;
	}
	
	###########################
	#QUERY DISK INFORMATION
	###########################
	public function query_host_disk($HOST_UUID,$PARTITION_INFO)
	{
		$QUERY_DISK = "SELECT * FROM _SERVER_DISK WHERE _HOST_UUID = '".$HOST_UUID."' AND _STATUS = 'Y'";
		$DISK_QUERY = $this -> DBCON -> prepare($QUERY_DISK);
		$DISK_QUERY -> execute();
		$DISK_COUNT = $DISK_QUERY -> rowCount();
		if ($DISK_COUNT != 0)
		{
			$COUNT = 0;
			foreach ($DISK_QUERY as $QueryDisk)
			{
				$PARTATION_BOUNDARY  = array(1*1024*1024*1024);
				if ($PARTITION_INFO != null)
				{
					for ($i=0; $i<count($PARTITION_INFO); $i++)
					{
						if ($PARTITION_INFO[$i]['disk_number'] == $COUNT)
						{
							$PARTATION_BOUNDARY[] = $PARTITION_INFO[$i]['offset'] + $PARTITION_INFO[$i]['size'];
						}	
					}				
				}
				
				$HOST_DISK[] = array(
								'HOST_UUID'  	=> $QueryDisk['_HOST_UUID'],
								'DISK_UUID'  	=> $QueryDisk['_DISK_UUID'],
								'DISK_NAME'  	=> $QueryDisk['_DISK_NAME'],
								'DISK_SIZE'  	=> $QueryDisk['_DISK_SIZE'],
								"DISK_BOUNDARY"	=> max($PARTATION_BOUNDARY),
								'DISK_URI'   	=> $QueryDisk['_DISK_URI'],
								'TIMESTAMP'  	=> $QueryDisk['_TIMESTAMP']);
								
				$COUNT++;
			}
			return $HOST_DISK;
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	#QUERY HOST INFORMATION
	###########################
	public function query_host_info($HOST_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVER_HOST WHERE _HOST_UUID = '".$HOST_UUID."' AND _STATUS = 'Y'";

		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS == 1)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_UUID = $QueryResult['_SERV_UUID']; 
				$HOST_SERV = $this -> query_server_info($SERV_UUID);
				
				#QUERY DISK INFORMATION
				if (isset(json_decode($QueryResult['_HOST_INFO'],TRUE)['partition_infos']))
				{
					$HOST_PARTATION = json_decode($QueryResult['_HOST_INFO'],TRUE)['partition_infos'];
				}
				else
				{
					$HOST_PARTATION = null;
				}
				$HOST_DISK = $this -> query_host_disk($HOST_UUID,$HOST_PARTATION);
				
				/* FOR UPGRADE BELOW 800 */
				$HOST_INFO = $this -> upgrade_host_info(json_decode($QueryResult['_HOST_INFO'],TRUE));
				
				#GET OS TYPE
				if (isset($HOST_INFO['os_name']))
				{
					if (strpos($HOST_INFO['os_name'],'Microsoft') !== FALSE)
					{
						$OS_TYPE = 'WINDOWS';
					}
					else
					{
						$OS_TYPE = 'LINUX';
					}
				}
				else
				{
					if (strpos($HOST_INFO['guest_os_name'],'Microsoft') !== FALSE)
					{
						$OS_TYPE = 'WINDOWS';
					}
					else
					{
						$OS_TYPE = 'LINUX';
					}
				}
						
				$HOST_SUMMARY = array(
									  'ACCT_UUID' => $QueryResult['_ACCT_UUID'],
									  'REGN_UUID' => $QueryResult['_REGN_UUID'],
									  'HOST_UUID' => $QueryResult['_HOST_UUID'],
									  'HOST_NAME' => $QueryResult['_HOST_NAME'],
									  'HOST_ADDR' => $QueryResult['_HOST_ADDR'],
									  'HOST_DISK' => $HOST_DISK,
									  'HOST_INFO' => $HOST_INFO,
									  'HOST_SERV' => $HOST_SERV,
									  'OS_TYPE'	  => $OS_TYPE,
									  'HOST_TYPE' => $QueryResult['_SERV_TYPE'],
									  'IS_DIRECT' => $HOST_INFO['direct_mode']
									);				
			}
			return $HOST_SUMMARY;
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	#UPGRADE HOST INFO
	###########################
	private function upgrade_host_info($HOST_INFO)
	{
		if (is_array($HOST_INFO))
		{
			if (!isset($HOST_INFO['direct_mode']))
			{
				$HOST_INFO['direct_mode'] = true;
			}
		}		
		
		if (is_object($HOST_INFO))
		{
			if (!isset($HOST_INFO -> direct_mode))
			{
				$HOST_INFO -> direct_mode = true;
			}
		}
		return $HOST_INFO;
	}
	
	###########################
	#QUERY CONNECTION INFORMATION
	###########################
	public function query_connection_info($CONN_UUID)
	{
		$QUERY_EXEC = "SELECT 
							_SERVER_CONN.*, _SYS_CLOUD_TYPE._CLOUD_TYPE 
					    FROM 
							_SERVER_CONN 
						LEFT JOIN 
							_CLOUD_MGMT 
						ON 
							_SERVER_CONN._CLUSTER_UUID = _CLOUD_MGMT._CLUSTER_UUID
						LEFT JOIN 
							_SYS_CLOUD_TYPE 
						ON 
							_CLOUD_MGMT._CLOUD_TYPE = _SYS_CLOUD_TYPE._ID
						WHERE 
							_CONN_UUID = '".$CONN_UUID."' AND _SERVER_CONN._STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 1)
		{
			foreach($QUERY as $QueryResult)
			{
				$SCHD_INFO = $this -> query_server_info($QueryResult['_SCHD_UUID']);
				$CARR_INFO = $this -> query_server_info($QueryResult['_CARR_UUID']);
				$LOAD_INFO = $this -> query_server_info($QueryResult['_LOAD_UUID']);
				$LAUN_INFO = $this -> query_server_info($QueryResult['_LAUN_UUID']);
				
				$CLOUD_INFO = $this -> query_cloud_type($QueryResult['_CLUSTER_UUID']);
				
				$CONN_SUMMARY = array(
									  'ACCT_UUID' => $QueryResult['_ACCT_UUID'],
									  
									  'SCHD_UUID' 	 => $QueryResult['_SCHD_UUID'],
									  'SCHD_HOST' 	 => $SCHD_INFO['HOST_NAME'],
									  'SCHD_ADDR' 	 => $SCHD_INFO['SERV_ADDR'],
									  'SCHD_OPEN' 	 => $SCHD_INFO['OPEN_HOST'],
									  'SCHD_SYST' 	 => $SCHD_INFO['SYST_TYPE'],
									  'SCHD_DIRECT'	 => $SCHD_INFO['SERV_INFO']['direct_mode'],
									  'SCHD_ID'		 => array_flip(array($SCHD_INFO['SERV_INFO']['machine_id'])),
									  'LIST_SCHD' 	 => $SCHD_INFO['LIST_ADDR'],
									  'SCHD_PROMOTE' => $SCHD_INFO['SERV_INFO']['is_promote'],
														
									  'CARR_UUID' 	 => $QueryResult['_LOAD_UUID'],
									  'CARR_HOST' 	 => $CARR_INFO['HOST_NAME'],
									  'CARR_ADDR' 	 => $CARR_INFO['SERV_ADDR'],
									  'CARR_OPEN' 	 => $CARR_INFO['OPEN_HOST'],
									  'CARR_SYST' 	 => $CARR_INFO['SYST_TYPE'],
									  'CARR_DIRECT'	 => $CARR_INFO['SERV_INFO']['direct_mode'],
									  'CARR_ID'		 => array_flip(array($CARR_INFO['SERV_INFO']['machine_id'])),
									  'LIST_CARR' 	 => $CARR_INFO['LIST_ADDR'],
									  'CARR_PROMOTE' => $CARR_INFO['SERV_INFO']['is_promote'],
									  
									  'LOAD_UUID' 	 => $QueryResult['_LOAD_UUID'],
									  'LOAD_HOST' 	 => $LOAD_INFO['HOST_NAME'],
									  'LOAD_ADDR' 	 => $LOAD_INFO['SERV_ADDR'],
									  'LOAD_OPEN' 	 => $LOAD_INFO['OPEN_HOST'],
									  'LOAD_SYST' 	 => $LOAD_INFO['SYST_TYPE'],
									  'LOAD_DIRECT'	 => $LOAD_INFO['SERV_INFO']['direct_mode'],
									  'LOAD_ID'		 => array_flip(array($LOAD_INFO['SERV_INFO']['machine_id'])),
									  'LIST_LOAD' 	 => $LOAD_INFO['LIST_ADDR'],
									  'LOAD_PROMOTE' => $LOAD_INFO['SERV_INFO']['is_promote'],
									  
									  'LAUN_UUID' 	 => $QueryResult['_LAUN_UUID'],
									  'LAUN_HOST' 	 => $LAUN_INFO['HOST_NAME'],
									  'LAUN_ADDR' 	 => $LAUN_INFO['SERV_ADDR'],
									  'LAUN_OPEN' 	 => $LAUN_INFO['OPEN_HOST'],
									  'LAUN_SYST' 	 => $LAUN_INFO['SYST_TYPE'],
									  'LAUN_DIRECT'	 => $LAUN_INFO['SERV_INFO']['direct_mode'],
									  'LAUN_ID'		 => array_flip(array($LAUN_INFO['SERV_INFO']['machine_id'])),									  
									  'LIST_LAUN' 	 => $LAUN_INFO['LIST_ADDR'],
									  'LAUN_PROMOTE' => $LAUN_INFO['SERV_INFO']['is_promote'],
							
									  'HOST_UUID' 	 => $LAUN_INFO['OPEN_HOST'],
									  'CONN_UUID' 	 => $QueryResult['_CONN_UUID'],
									  'CONN_DATA' 	 => json_decode($QueryResult['_CONN_DATA'],true),
									  'CONN_TYPE' 	 => $QueryResult['_CONN_TYPE'],
									  
									  'MGMT_ADDR' 	 => $QueryResult['_MGMT_ADDR'],
									  
									  'AZURE_MGMT_DISK' => $LAUN_INFO['SERV_INFO']['is_azure_mgmt_disk'],
									  'CLUSTER_UUID' 	=> $QueryResult['_CLUSTER_UUID'],
									  'CLOUD_TYPE' 		=> $CLOUD_INFO -> cloud_type,
									  'CLOUD_NAME' 		=> $CLOUD_INFO -> cloud_name,
									  'VENDOR_NAME'	 	=> $CLOUD_INFO -> vendor_type
									);
			}
			return $CONN_SUMMARY;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#QUERY CONNECTION INFORMATION BY SERV UUID
	###########################
	public function query_connection_by_serv_uuid($HOST_UUID,$LOAD_UUID,$LAUN_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVER_CONN WHERE _CARR_UUID = '".$HOST_UUID."' AND _LOAD_UUID = '".$LOAD_UUID."' AND _LAUN_UUID = '".$LAUN_UUID."' AND _CONN_TYPE = 'LocalFolder' AND _STATUS = 'Y'";

		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();	
		
		#ALL-IN-ONE LOCAL FOLDER CONNECTION
		if ($COUNT_ROWS == 1)
		{
			#ALL-IN-ONE LOCAL FOLDER CONNECTION
			foreach($QUERY as $QueryResult)
			{
				$CARR_INFO = $this -> query_server_info($HOST_UUID);
				$LOAD_INFO = $this -> query_server_info($QueryResult['_LOAD_UUID']);
				
				$CONN_SUMMARY = array
								(
									'ACCT_UUID' 	=> $QueryResult['_ACCT_UUID'],
									'SOURCE_CONN' 	=> $QueryResult['_CONN_UUID'],
									'TARGET_CONN' 	=> $QueryResult['_CONN_UUID'],
									'CONN_DATA' 	=> json_decode($QueryResult['_CONN_DATA'],true),
									'CONN_TYPE' 	=> $QueryResult['_CONN_TYPE'],
									'CARR_INFO'		=> $CARR_INFO,
									'SERV_INFO' 	=> $LOAD_INFO,
									'MGMT_ADDR' 	=> $QueryResult['_MGMT_ADDR'],
									'CLUSTER_UUID' 	=> $QueryResult['_CLUSTER_UUID'],
									'CLOUD_TYPE'	=> $this -> query_cloud_type($QueryResult['_CLUSTER_UUID']) -> cloud_type,
									'VENDOR_NAME'	=> $this -> query_cloud_type($QueryResult['_CLUSTER_UUID']) -> vendor_type,
									'LAUN_UUID' 	=> $LAUN_UUID
								);
			}
			return $CONN_SUMMARY;
		}
		else
		{
			$QUERY_SELECT_DIFF = "SELECT * FROM _SERVER_CONN WHERE _CARR_UUID = '".$HOST_UUID."' AND _LOAD_UUID = '".$LOAD_UUID."' AND _CONN_TYPE = 'LocalFolder' AND _STATUS = 'Y'";
		
			$QUERY_DIFF = $this -> DBCON -> prepare($QUERY_SELECT_DIFF);
			$QUERY_DIFF -> execute();
			$COUNT_DIFF_ROWS = $QUERY_DIFF -> rowCount();
						
			if ($COUNT_DIFF_ROWS == 1)
			{
				foreach($QUERY_DIFF as $QueryResult)
				{
					$CARR_INFO = $this -> query_server_info($HOST_UUID);
					$LOAD_INFO = $this -> query_server_info($QueryResult['_LOAD_UUID']);
				
					$CONN_SUMMARY = array
									(
										'ACCT_UUID' 	=> $QueryResult['_ACCT_UUID'],
										'SOURCE_CONN' 	=> $QueryResult['_CONN_UUID'],
										'TARGET_CONN' 	=> $QueryResult['_CONN_UUID'],
										'CONN_DATA' 	=> json_decode($QueryResult['_CONN_DATA'],true),
										'CONN_TYPE' 	=> $QueryResult['_CONN_TYPE'],
										'CARR_INFO'		=> $CARR_INFO,
										'SERV_INFO' 	=> $LOAD_INFO,
										'MGMT_ADDR' 	=> $QueryResult['_MGMT_ADDR'],
										'CLUSTER_UUID' 	=> $QueryResult['_CLUSTER_UUID'],
										'CLOUD_TYPE'	=> $this -> query_cloud_type($QueryResult['_CLUSTER_UUID']) -> cloud_type,
										'VENDOR_NAME'	=> $this -> query_cloud_type($QueryResult['_CLUSTER_UUID']) -> vendor_type,
										'LAUN_UUID' 	=> ''
									);
				}
				return $CONN_SUMMARY;
			}
			else
			{			
				#WEB DAV CONNECTION
				$QUERY_WEBDAV = "SELECT * FROM _SERVER_CONN WHERE _CARR_UUID = '".$HOST_UUID."' AND _CONN_TYPE = 'LocalFolder' AND _STATUS = 'Y'";
			
				$QUERY_WEBDAV = $this -> DBCON -> prepare($QUERY_WEBDAV);
				$QUERY_WEBDAV -> execute();				
				$COUNT_QUERY_WEBDAV = $QUERY_WEBDAV -> rowCount();
				
				if ($COUNT_QUERY_WEBDAV != 0)
				{
					foreach($QUERY_WEBDAV as $ResultQuery)
					{
						$SOURCE_CONN = $ResultQuery['_CONN_UUID'];
						$SCHD_UUID   = $ResultQuery['_SCHD_UUID'];
					}
				}
				else
				{
					$SOURCE_CONN = null;
					$SCHD_UUID = null;
				}
				
				$CARR_INFO = $this -> query_server_info($HOST_UUID);
				$LOAD_INFO = $this -> query_server_info($LOAD_UUID);
		
				$CARR_MACHINE_ID = $CARR_INFO['SERV_INFO']['machine_id'];
				$LOAD_MACHINE_ID = $LOAD_INFO['SERV_INFO']['machine_id'];
		
				if ($CARR_MACHINE_ID == $LOAD_MACHINE_ID)
				{
					$CONN_TYPE = 'LocalFolder';
					
					#TEMP CONN DATA
					$CONN_DATA = array('CONN_TYPE' => $CONN_TYPE,
									   'SCHD_UUID' => $SCHD_UUID);
									   
					$TARGET_CONN = $SOURCE_CONN;
				}
				else
				{
					$CONN_TYPE = 'Remote';
					
					#TEMP CONN DATA
					$CONN_DATA = array('CONN_TYPE' => $CONN_TYPE,
									   'SCHD_UUID' => $SCHD_UUID,
								       'options'   => array('folder' => 'WebDav'));
					$TARGET_CONN = ''; 
				}
			
				$CONN_SUMMARY = array
									(
										'ACCT_UUID' 	=> $CARR_INFO['ACCT_UUID'],
										'SOURCE_CONN'	=> $SOURCE_CONN,
										'TARGET_CONN' 	=> $TARGET_CONN,
										'CONN_DATA' 	=> $CONN_DATA,
										'CONN_TYPE' 	=> $CONN_TYPE,
										'CARR_INFO'		=> $CARR_INFO,
										'SERV_INFO' 	=> $LOAD_INFO,
										'MGMT_ADDR' 	=> $CARR_INFO['SERV_ADDR'],			#FOR CREATE WEBDAV USE ONLY
										'CLUSTER_UUID' 	=> $LOAD_INFO['OPEN_UUID'],
										'CLOUD_TYPE'	=> $this -> query_cloud_type($LOAD_INFO['OPEN_UUID']) -> cloud_type,
										'VENDOR_NAME'	=> $this -> query_cloud_type($LOAD_INFO['OPEN_UUID']) -> vendor_type,
										'LAUN_UUID' 	=> ''
									);
									
				return $CONN_SUMMARY;
			}
		}
	}
	
	###########################	
	#QUERY CLOUD TYPE
	###########################
	private function query_cloud_type($CLUSTER_UUID)
	{
		$CLOUD_MGMT = "SELECT * FROM _CLOUD_MGMT as TA JOIN _SYS_CLOUD_TYPE as TB on TA._CLOUD_TYPE = TB._ID WHERE _CLUSTER_UUID = '".$CLUSTER_UUID."' AND _STATUS = 'Y'";
		$QUERY_CLOUD_MGMT = $this -> DBCON -> prepare($CLOUD_MGMT);
		$QUERY_CLOUD_MGMT -> execute();				
		$COUNT_CLOUD_MGMT = $QUERY_CLOUD_MGMT -> rowCount();

		if ($COUNT_CLOUD_MGMT == 1)
		{			
			$CLOUD_MGMT_RESULT = $QUERY_CLOUD_MGMT -> fetch(PDO::FETCH_OBJ);
			
			$CLOUD_TYPE = $CLOUD_MGMT_RESULT -> _CLOUD_TYPE;
			
			if (isset(json_decode($CLOUD_MGMT_RESULT -> _AUTH_TOKEN) -> vendor_name))
			{
				$VENDOR_NAME = json_decode($CLOUD_MGMT_RESULT -> _AUTH_TOKEN) -> vendor_name;
			}
			else
			{
				$VENDOR_NAME = $CLOUD_TYPE;
			}
			
			if ($CLOUD_MGMT_RESULT -> _CLOUD_TYPE == 'Azure')
			{
				$DECODE_STRING = json_decode(Misc_Class::encrypt_decrypt('decrypt',$CLOUD_MGMT_RESULT -> _CLUSTER_USER),false);
				if ($DECODE_STRING -> CONNECTION_STRING != '')
				{
					$VENDOR_NAME = 'AzureBlob';
				}					
			}
			
			$DEFINE_NAME = ["OPENSTACK", "Aliyun", "Ctyun", "VMWare"];
			$CHANGE_NAME = ["OpenStack", "Alibaba Cloud", "天翼云", "VMware"];
			$CLOUD_NAME = str_replace($DEFINE_NAME, $CHANGE_NAME, $CLOUD_TYPE);
		}
		else
		{
			$CLOUD_TYPE = 'UnknownCloudType';
			$CLOUD_NAME = 'UnknownCloudName';
			$VENDOR_NAME  = 'UnknownVendorType';
		}
		
		return (object)array('cloud_type' => $CLOUD_TYPE, 'cloud_name' => $CLOUD_NAME, 'vendor_type' => $VENDOR_NAME);
	}
	
	###########################	
	#UPDATE SERVER
	###########################
	public function update_server($SERV_UUID,$INPUT_ADDR,$MGMT_ADDR)
	{
		#QUERY SERVER INFORMATION
		$SERV_INFO = $this -> query_server_info($SERV_UUID);
		$ACCT_UUID  = $SERV_INFO['ACCT_UUID'];
		$MACHINE_ID = $SERV_INFO['SERV_INFO']['machine_id'];
		$LIST_SERVICE_UUID = $this -> list_match_service_id($MACHINE_ID)['ServiceId'];

		$ServiceMgmt = new Service_Class();
				
		#DIRECT/IN-DIRECT MODE
		if (isset($SERV_INFO['SERV_INFO']['direct_mode'])){$DIRECT_MODE = $SERV_INFO['SERV_INFO']['direct_mode'];}else{$DIRECT_MODE = true;}
		
		if ($DIRECT_MODE == TRUE)
		{
			$VERIFY_ADDR = array_values(array_unique(array_merge(explode(',', $INPUT_ADDR),$SERV_INFO['SERV_ADDR'])));
		}
		else
		{
			$VERIFY_ADDR = array($MACHINE_ID);
		}
			
		#IS AZURE BLOB
		if (isset($SERV_INFO['SERV_INFO']['is_azure_mgmt_disk'])){$IS_AZURE_MGMT_DISK = $SERV_INFO['SERV_INFO']['is_azure_mgmt_disk'];}else{$IS_AZURE_MGMT_DISK = true;}
		
		#GET TRANSPORT SERVER METADATA
		for ($i=0; $i<count($VERIFY_ADDR); $i++)
		{
			try{
				$QUERY_INFO = $ServiceMgmt -> get_connection($VERIFY_ADDR[$i],'Launcher',0,$MGMT_ADDR);
				
				$NETWORK_INFO = $QUERY_INFO -> network_infos;
						
				for($n=0; $n<count($NETWORK_INFO); $n++)
				{
					$NET_IP_ADDR[] = $NETWORK_INFO[$n] -> ip_addresses;
				}
				$IP_ADDRESS = call_user_func_array('array_merge', $NET_IP_ADDR);
				
				break;
			}
			catch (Throwable $e){
				//return false;
			}
		}
	
		#MERGE/UNIQUE/RE-INDEX AND JSON THE SERVE ADDRESS
		if (Misc_Class::is_valid_uuid($INPUT_ADDR) === TRUE)
		{
			$SERV_ADDR = json_encode($IP_ADDRESS);
		}
		else
		{
			$SERV_ADDR = json_encode(array_values(array_unique(array_merge(explode(',', $INPUT_ADDR),$IP_ADDRESS))));
		}

		$ACCT_UUID = $SERV_INFO['ACCT_UUID'];
		$HOST_NAME = $SERV_INFO['HOST_NAME'];
		$HOST_UUID = $SERV_INFO['OPEN_HOST'];
		
		#UPDATE SERVE METADATA
		$QUERY_INFO -> direct_mode = $DIRECT_MODE;
		$QUERY_INFO -> is_azure_mgmt_disk = $IS_AZURE_MGMT_DISK;

		foreach ($LIST_SERVICE_UUID as $SERVICE_TYPE => $SERVICE_UUID)
		{
			$UPDATE_EXEC = "UPDATE _SERVER
						SET
							_SERV_INFO = '".str_replace('\\','\\\\',json_encode($QUERY_INFO,true))."',
							_SERV_ADDR = '".$SERV_ADDR."',
							_TIMESTAMP = '".Misc_Class::current_utc_time()."'
						WHERE
							_ACCT_UUID = '".$ACCT_UUID."' AND
							_SERV_UUID = '".$SERVICE_UUID."'";
			$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		}
				
		#RECREATE LOCAL FOLDER CONNECTION
		if (isset($SERV_INFO['LOCAL_CONN']) AND $SERV_INFO['LOCAL_CONN'] != FALSE)
		{
			$ServiceMgmt -> re_create_connection($SERV_INFO['LOCAL_CONN'],'CARRIER');
		}
		
		return true;
	}
	
	###########################	
	#UPDATE METADATA
	###########################
	public function update_server_metadata($SERV_UUID,$METADATA)
	{
		$UPDATE_EXEC = "UPDATE
							_SERVER
						SET
							_HOST_NAME = '".$METADATA -> client_name."',
							_SERV_INFO = '".str_replace('\\','\\\\',json_encode($METADATA,true))."',
							_TIMESTAMP = '".Misc_Class::current_utc_time()."'
						WHERE
							_SERV_UUID = '".$SERV_UUID."'";
							
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
	}
	
	###########################
	#CHECK IN USE CONNECTION
	###########################
	private function check_in_use_connection($ACCT_UUID)
	{
		$GET_REPL_CONN = "SELECT * FROM _REPLICA WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_REPL_CONN);
		$QUERY -> execute();
	
		$QueryFetch = $QUERY -> fetchAll(PDO::FETCH_ASSOC);
		$COUNT_ROWS = count($QueryFetch);
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QueryFetch as $QueryResult)
			{
				$SOURCE_UUID = json_decode($QueryResult['_CONN_UUID'], FALSE) -> SOURCE;
				$TARGET_UUID = json_decode($QueryResult['_CONN_UUID'], FALSE) -> TARGET;

				$LIST_USED_UUID[] = $this -> check_register_connection($SOURCE_UUID);
				$LIST_USED_UUID[] = $this -> check_register_connection($TARGET_UUID);				
			}
			
			return $LIST_USED_UUID;
		}
		else
		{
			return false;
		}		
	}
	
	
	###########################
	#RETURN SELECT CONNECTION SERV UUID
	###########################
	private function check_register_connection($CONN_UUID)
	{
		$LIST_CONN = "SELECT * FROM _SERVER_CONN WHERE _CONN_UUID = '".$CONN_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($LIST_CONN);
		$QUERY -> execute();
	
		$QueryFetch = $QUERY -> fetchAll(PDO::FETCH_ASSOC);
		$COUNT_ROWS = count($QueryFetch);
	
		if ($COUNT_ROWS != 0)
		{
			foreach($QueryFetch as $QueryResult)
			{
				$LIST_UUID['SCHD_UUID'] = $QueryResult['_SCHD_UUID'];
				$LIST_UUID['CARR_UUID'] = $QueryResult['_CARR_UUID'];
				$LIST_UUID['LOAD_UUID'] = $QueryResult['_LOAD_UUID'];
				$LIST_UUID['LAUN_UUID'] = $QueryResult['_LAUN_UUID'];
			}
			return $LIST_UUID;
		}
		else
		{
			return false;
		}
	}
	
	
	###########################
	#CHECK ASSOCIATED PACKER
	###########################
	private function check_register_packer($MATCH_SERV_UUID)
	{
		foreach ($MATCH_SERV_UUID as $SERV_TYPE => $SERV_UUID)
		{
			$CHECK_REG_HOST = "SELECT * FROM _SERVER_HOST WHERE _SERV_UUID = '".$SERV_UUID."' AND _STATUS = 'Y'";
		
			$QUERY = $this -> DBCON -> prepare($CHECK_REG_HOST);
			$QUERY -> execute();
		
			$COUNT_ROWS = $QUERY -> rowCount();
			if ($COUNT_ROWS != 0)
			{
				return false;
			}
		}
		
		return true;
	}
	
	
	###########################
	#LIST ALL RELATED SERVICE UUID
	###########################
	public function list_match_service_id($MACHINE_ID)
	{
		$MATCH_SERV_UUID = array();
		$IS_DIRECT = true;
		
		$SELECT_ALL_SERVER = "SELECT _SERV_ADDR, _HOST_NAME, _SERV_UUID, _SERV_INFO, _SERV_TYPE, _OPEN_UUID FROM _SERVER WHERE _STATUS = 'Y'";
		$QUERY_SERVER = $this -> DBCON -> prepare($SELECT_ALL_SERVER);
		$QUERY_SERVER -> execute();
		
		$COUNT_ROWS = $QUERY_SERVER -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY_SERVER as $QueryResult)
			{
				$SERV_INFO = $this -> upgrade_server_info(json_decode($QueryResult['_SERV_INFO'],true));

				$GET_MACHINE_ID = isset($SERV_INFO['machine_id'])?$SERV_INFO['machine_id']:'00000000-0000-0000-0000-000000000000';
								
				if ($GET_MACHINE_ID == $MACHINE_ID)
				{
					$CLOUD_TYPE = ($this -> query_cloud_type($QueryResult['_OPEN_UUID']) -> cloud_type == 'UnknownCloudType')?"On-Premises":$this -> query_cloud_type($QueryResult['_OPEN_UUID']) -> cloud_type;
					if ($CLOUD_TYPE == 'Azure')
					{
						if ($SERV_INFO['is_azure_mgmt_disk'] == TRUE)
						{
							$CLOUD_TYPE = 'AzureBlob';
						}
						elseif ($SERV_INFO['is_promote'] == TRUE)
						{
							$CLOUD_TYPE = 'Azure On-Premises';
						}					
					}

					if ($SERV_INFO['is_winpe'] == TRUE)
					{
						$CLOUD_TYPE = 'Recover Kit';
					}
					
					$DEFINE_NAME = ["OPENSTACK", "Aliyun", "Ctyun", "VMWare"];
					$CHANGE_NAME = ["OpenStack", "Alibaba Cloud", "天翼云", "VMware"];
					$CLOUD_NAME = str_replace($DEFINE_NAME, $CHANGE_NAME, $CLOUD_TYPE);
					
					$SERV_TYPE = $QueryResult['_SERV_TYPE'];
					if ($SERV_TYPE == 'Scheduler'){$IS_DIRECT = $SERV_INFO['direct_mode'];}
					$MATCH_SERV_UUID['ServiceId'][$SERV_TYPE] = $QueryResult['_SERV_UUID'];
					$MATCH_SERV_UUID['Address'] = $QueryResult['_SERV_ADDR'];
					$MATCH_SERV_UUID['Hostname'] = $QueryResult['_HOST_NAME'];
					$MATCH_SERV_UUID['CloudType'] = $CLOUD_NAME;
					$MATCH_SERV_UUID['IsDirect'] = $IS_DIRECT;
				}
			}
		}
		return $MATCH_SERV_UUID;		
	}	
		
	###########################
	#DELETE SERVER
	###########################
	public function delete_server($SERV_UUID,$CHECK_DEPENDENCE)
	{
		$SERV_INFO   = $this -> query_server_info($SERV_UUID);
		$ACCT_UUID   = $SERV_INFO['ACCT_UUID'];
		$REGN_UUID   = $SERV_INFO['REGN_UUID'];
		$HOST_NAME   = $SERV_INFO['HOST_NAME'];
		$OPEN_UUID   = $SERV_INFO['OPEN_UUID'];
		$IS_WINPE	 = $SERV_INFO['SERV_INFO']['is_winpe'];
		$MACHINE_ID  = $SERV_INFO['SERV_INFO']['machine_id'];
		$DIRECT_MODE = $SERV_INFO['SERV_INFO']['direct_mode'];
		
		$LIST_SERVICE_UUID = $this -> list_match_service_id($MACHINE_ID)['ServiceId'];
	
		$REG_HOST_CHECK = $this -> check_register_packer($LIST_SERVICE_UUID);
		
		if ($REG_HOST_CHECK == FALSE)
		{
			return false;
		}
		else
		{
			if ($CHECK_DEPENDENCE == TRUE)
			{
				$LIST_IN_USE_CONN = $this -> check_in_use_connection($ACCT_UUID);
		
				if ($LIST_IN_USE_CONN != FALSE)
				{
					for ($i=0; $i<count($LIST_IN_USE_CONN); $i++)
					{
						foreach ($LIST_IN_USE_CONN[$i] as $LIST_SERV_UUID)
						{				
							foreach($LIST_SERVICE_UUID as $SERV_TYPE => $SERV_UUID)
							{
								if ($LIST_SERV_UUID == $SERV_UUID)
								{
									return false;
									break 3;
								}
							}
						}
					}			
				}
			}
			
			#GET SERVER REPORT DATA
			$REPORT_DATA = $this -> report_transport_server($ACCT_UUID,$REGN_UUID,$SERV_UUID,$OPEN_UUID,$HOST_NAME,'delete');
						
			foreach ($LIST_SERVICE_UUID as $SERVICE_TYPE => $SERVICE_UUID)
			{		
				#DELETE ASSOCIATED SERVICE			
				$DELETE_SERV = "UPDATE _SERVER
								SET
									_TIMESTAMP = '".Misc_Class::current_utc_time()."',
									_STATUS	   = 'X'
								WHERE
									_SERV_UUID = '".$SERVICE_UUID."'";
									
				$this -> DBCON -> prepare($DELETE_SERV) -> execute();
				
				#DELETE ASSOCIATED CONNECTION
				if ($SERVICE_TYPE == 'Scheduler')
				{
					$DELETE_CONN = "UPDATE _SERVER_CONN
									SET
										_TIMESTAMP = '".Misc_Class::current_utc_time()."',
										_STATUS	   = 'X'
									WHERE
										_SCHD_UUID = '".$SERVICE_UUID."'";
										
					$this -> DBCON -> prepare($DELETE_CONN) -> execute();					
				}
			}
			
			if( $SERV_INFO['VENDOR_NAME'] == "VMWare" ){
				$ServiceMgmt = new Service_Class();
				$ServiceMgmt -> delete_cloud($OPEN_UUID);
			}

			#UNREGISTER SERVER
			if ($DIRECT_MODE == FALSE AND $IS_WINPE == FALSE)
			{
				$ServiceMgmt = new Service_Class();				
				$ServiceMgmt -> unregister_server($MACHINE_ID);
			}		
			
			#UPDATE REPORT			
			$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);
			
			return true;
		}
	}
	
	###########################
	#UPDATE HOST
	###########################
	public function update_host($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$SERV_INFO,$VMS_INFO,$SERV_UUID,$SERV_ADDR,$SELECT_VMS)
	{
		$ServiceMgmt = new Service_Class();
		
		if ($SERV_TYPE == 'Physical Packer' or $SERV_TYPE == 'Offline Packer')
		{
			$HOST_NAME  = $SERV_INFO -> client_name;
			$DISK_INFO  = $SERV_INFO -> disk_infos;
			
			if ($HOST_USER != '')
			{
				$MISC_INFO = explode('|',$HOST_USER);
				$INSTANCE_ID = $MISC_INFO[0];
				$AVAILABILITY_ZONE = $MISC_INFO[1];
				$REG_CLOUD_UUID = $HOST_PASS;
			}
			else
			{
				$INSTANCE_ID = '';
				$AVAILABILITY_ZONE = '';
				$REG_CLOUD_UUID = '';
			}
			
			if (Misc_Class::is_valid_uuid($HOST_ADDR) == true)
			{
				$NETWORK_INFO = $SERV_INFO -> network_infos;
				for($i=0; $i<count($NETWORK_INFO); $i++)
				{
					$HOST_ADDRESS[] = $NETWORK_INFO[$i] -> ip_addresses;
				}					
				$HOST_ADDRESS = call_user_func_array('array_merge', $HOST_ADDRESS);
				$HOST_ADDRESS = implode(',',$HOST_ADDRESS);
			}
			else
			{
				$HOST_ADDRESS = $HOST_ADDR;
			}
			
			#READ FROM QUERY
			$QUERY_HOST_INFO = $this -> query_host_info($HOST_UUID);
			$SERV_INFO -> manufacturer = $QUERY_HOST_INFO['HOST_INFO']['manufacturer'];
			$SERV_INFO -> direct_mode  = $QUERY_HOST_INFO['HOST_INFO']['direct_mode'];
			
			$INSTANCE_INFO = array('instance_id' => $INSTANCE_ID,'availability_zone' => $AVAILABILITY_ZONE,'reg_cloud_uuid' => $REG_CLOUD_UUID);
			$SERV_INFO = (object)array_merge((array)$SERV_INFO,$INSTANCE_INFO);
			
			$HOST_JSON = str_replace('\\','\\\\',json_encode($SERV_INFO,true));
				
			$UPDATE_HOST = "UPDATE
								_SERVER_HOST
							SET
								_HOST_ADDR = '".$HOST_ADDRESS."',
								_HOST_NAME = '".$HOST_NAME."',
								_HOST_INFO = '".$HOST_JSON."',
								_SERV_UUID = '".$SERV_UUID."',
								_TIMESTAMP = '".Misc_Class::current_utc_time()."'
							WHERE
								_HOST_UUID = '".$HOST_UUID."'";
			
			$this -> DBCON -> prepare($UPDATE_HOST) -> execute();
			
			#reflush_physical_packer
			#$ServiceMgmt -> reflush_physical_packer($ACCT_UUID);
			
			return true;
		}
		elseif ($SERV_TYPE == 'Virtual Packer')
		{
			#ADD NEW ESX AND VMS
			$ESX_SERV_UUID = $this -> initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$SERV_UUID,$SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$SERV_INFO,$VMS_INFO,'ESX',$SELECT_VMS,true);
	
			#UPDATE SELECT HOST INFORMATION
			//$GET_HOST_INFO = $this -> query_host_info($HOST_UUID);
			//$ESX_SERV_UUID = $GET_HOST_INFO['HOST_SERV']['SERV_UUID'];
			
			$SERV_INFO = str_replace('\\','\\\\',json_encode($SERV_INFO,true));
						
			$SERV_MISC = array('ADDR' => $HOST_ADDR,
							   'USER' => $HOST_USER,
							   'PASS' => $HOST_PASS);
						
			$UPDATE_ESX = "UPDATE
								_SERVER
							SET
								_SERV_ADDR = '".json_encode(array($HOST_ADDR))."',
								_SERV_INFO = '".$SERV_INFO."',
								_SERV_MISC = '".Misc_Class::encrypt_decrypt('encrypt',json_encode($SERV_MISC))."',
								_TIMESTAMP = '".Misc_Class::current_utc_time()."'
							WHERE
								_SERV_UUID = '".$ESX_SERV_UUID."'";

			$this -> DBCON -> prepare($UPDATE_ESX) -> execute();
			
			
			$UPDATE_ESX_VM = "UPDATE
								_SERVER_HOST
							  SET
								_SERV_UUID = '".$ESX_SERV_UUID."'
							  WHERE
								_HOST_UUID = '".$HOST_UUID."'";
		
			$this -> DBCON -> prepare($UPDATE_ESX_VM) -> execute();
			
			#reflush_virtual_packer
			#$ServiceMgmt -> reflush_virtual_packer($ACCT_UUID);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#LIST ALL CONNECTION TYPE
	###########################
	public static function list_target_type()
	{
		$CONNECTION_TYPE = array(
								'AWS' 		=> '8c7a225f349d',
								'ESX' 		=> '27ae3ca1aeac',
								'KVM' 		=> 'c572f9de69a2',
								'OpenStack' => '85430b46b0d2');
		return $CONNECTION_TYPE;
	}
	
	###########################
	#TAEGET ARRAY TO JSON
	###########################
	public function target_json($CONN_ADDR,$CONN_TYPE,$USER_NAME,$PASS_WORD)
	{
		$TARGET_ARRAY = array(
							'ADDR' => $CONN_ADDR,
							'TYPE' => $CONN_TYPE,
							'USER' => Misc_Class::encrypt_decrypt('encrypt',$USER_NAME),
							'PASS' => Misc_Class::encrypt_decrypt('encrypt',$PASS_WORD));
							
		$TARGET_JSON = json_encode($TARGET_ARRAY);
		return $TARGET_JSON;
	}
		
	###########################
	#QUERY AVAILABLE CONNECTION
	###########################
	public function query_available_connection($ACCT_UUID)
	{
		$GET_EXEC = "SELECT * FROM _SERVER_CONN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
				
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_NAME = $this -> query_server_info($QueryResult['_SCHD_UUID']);
				$CONN_ARRAY[] = array(
								'ACCT_UUID' => $QueryResult['_ACCT_UUID'],
								'CONN_UUID' => $QueryResult['_CONN_UUID'],
								'CONN_DATA' => json_decode($QueryResult['_CONN_DATA'],true),
								'CONN_TYPE' => $QueryResult['_CONN_TYPE'],
								'CONN_DATE' => $QueryResult['_TIMESTAMP'],
								'SERV_NAME' => $SERV_NAME['HOST_NAME']
							);									
			}
			return $CONN_ARRAY;
		}
		else
		{
			return false;
		}	
	}
		
	###########################		
	#SAVE CONNECTION
	###########################
	public function save_connection($CONN_DATA)
	{	
		$ACCT_UUID 	  = $CONN_DATA['ACCT_UUID'];
		$SCHD_UUID 	  = $CONN_DATA['SCHD_UUID'];
		$CARR_UUID 	  = $CONN_DATA['CARR_UUID'];
		$LOAD_UUID 	  = $CONN_DATA['LOAD_UUID'];
		$LAUN_UUID 	  = $CONN_DATA['LAUN_UUID'];
		$CLUSTER_UUID = $CONN_DATA['CLUSTER_UUID'];
		
		$CONN_UUID 	  = $CONN_DATA['id'];
		$CONN_TYPE 	  = $CONN_DATA['CONN_TYPE'];
		$MGMT_ADDR 	  = $CONN_DATA['MGMT_ADDR'];
		$CONN_DATA 	  = urldecode(addslashes(json_encode($CONN_DATA,true)));
		
		
		$INSERT_EXEC = "INSERT
							INTO _SERVER_CONN(
								_ID,
								_ACCT_UUID,
								
								_SCHD_UUID,
								_CARR_UUID,
								_LOAD_UUID,
								_LAUN_UUID,
								_CLUSTER_UUID,
								
								_CONN_UUID,
								_CONN_DATA,
								_CONN_TYPE,
								
								_MGMT_ADDR,
								_TIMESTAMP,
								_STATUS)
							VALUE(
								'',
								'".$ACCT_UUID."',
								
								'".$SCHD_UUID."',
								'".$CARR_UUID."',
								'".$LOAD_UUID."',								
								'".$LAUN_UUID."',
								'".$CLUSTER_UUID."',
								
								'".$CONN_UUID."',
								'".$CONN_DATA."',
								'".$CONN_TYPE."',
								
								'".$MGMT_ADDR."',
								'".Misc_Class::current_utc_time()."',
								'Y')";
		
		$QUERY = $this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		return true;
	}
	
	###########################
	#LIST TARGET CONNECTION
	###########################
	public function list_target_connection($CONN_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _SERVER_DEST WHERE _CONN_UUID = '".$CONN_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				#DECODE JSON STRING TO ARRAY
				$CONN_DATA = json_decode($QueryResult['_CONN_DATA'],true);
				$CONN_JSON[] = array(
									'ADDR' => $CONN_DATA['ADDR'],
									'TYPE' => $CONN_DATA['TYPE'],
									'USER' => Misc_Class::encrypt_decrypt('decrypt',$CONN_DATA['USER']),
									'PASS' => Misc_Class::encrypt_decrypt('decrypt',$CONN_DATA['PASS'])
								);
			}
			return $CONN_JSON;
		}
		else
		{
			return false;
		}		
	}
	
	###########################	
	#UPDATE TARGET CONNECTION
	###########################
	public function update_target_connection($TARG_UUID,$CONN_UUID,$CONN_DATA)
	{
		$UPDATE_EXEC = "UPDATE _SERVER_DEST
						SET
							_CONN_UUID = '".$CONN_UUID."',
							_CONN_DATA = '".$CONN_DATA."',
							_TIMESTAMP = '".Misc_Class::current_utc_time()."'
						WHERE
							_TARG_UUID = '".$TARG_UUID."'";
							
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC);
		$QUERY -> execute();
		
		return array('UPDATE_TARGET_CONNECTION' => 'Successful');
	}
	
	###########################
	#DELETE TARGET CONNECTION
	###########################
	public function delete_target_connection($TARG_UUID)
	{
		$DELETE_EXEC = "UPDATE _SERVER_DEST
						SET
							_TIMESTAMP 	= '".Misc_Class::current_utc_time()."',
							_STATUS		= 'X'
						WHERE
							_TARG_UUID = '".$TARG_UUID."'";
							
		$QUERY = $this -> DBCON -> prepare($DELETE_EXEC);
		$QUERY -> execute();
		
		return array('DELETE_TARGET_CONNECTION' => 'Successful');
	}
	
	###########################
	#QUERY LIST CONNECTION
	###########################
	public function server_register_connection($SERVICE_UUID,$TYPE)
	{
		$QUERY_CONNECTION_UUID = "SELECT * FROM _SERVER_CONN WHERE _".$TYPE." = '".$SERVICE_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($QUERY_CONNECTION_UUID);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				#DECODE JSON STRING TO ARRAY
				$CONN_DATA[] = $QueryResult['_CONN_UUID'];
			}
			return $CONN_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#CHECK RUNNING CONNECTION
	###########################
	public function check_running_connection($LAUNCHER_UUID)
	{
		$DEFAULT_RESPONSE = TRUE;
		
		$CONN_UUID = $this -> server_register_connection($LAUNCHER_UUID,'LAUN_UUID');
		
		if ($CONN_UUID != FALSE)
		{	
			$QUERY_REPLICA = "SELECT * FROM _REPLICA WHERE _STATUS = 'Y'";
			$QUERY = $this -> DBCON -> prepare($QUERY_REPLICA);
			$QUERY -> execute();
			$COUNT_ROWS = $QUERY -> rowCount();
			
			if ($COUNT_ROWS != 0)
			{
				foreach($QUERY as $QueryResult)
				{
					#DECODE JSON STRING TO ARRAY
					$CONN_DATA = json_decode($QueryResult['_CONN_UUID']);
					for ($i=0; $i<count($CONN_UUID); $i++)
					{
						if ($CONN_DATA -> SOURCE == $CONN_UUID[$i])
						{
							$DEFAULT_RESPONSE = FALSE;
							break 2;
						}
						
						if ($CONN_DATA -> TARGET == $CONN_UUID[$i])
						{
							$DEFAULT_RESPONSE = FALSE;
							break 2;
						}
					}
				}			
			}			
		}
		return $DEFAULT_RESPONSE;
	}
	
	###########################
	#QUERY SERVER CONNECTION
	###########################
	public function query_server_connection($SERV_UUID,$SERV_TYPE,$CONN_TYPE)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVER_CONN WHERE _".$SERV_TYPE." = '".$SERV_UUID."' AND _CONN_TYPE = '".$CONN_TYPE."' AND _STATUS = 'Y' LIMIT 1";
		
		$QUERY_CONN = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY_CONN -> execute();
		$COUNT_ROWS = $QUERY_CONN -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY_CONN as $QueryResult)
			{
				#DECODE JSON STRING TO ARRAY
				$CONN_DATA[] = $QueryResult['_CONN_UUID'];
			}
			return $CONN_DATA[0];
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#LIST RECOVER PLAN
	###########################
	public function list_recover_plan($ACCT_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVICE_PLAN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		
		$QUERY_TEMP = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY_TEMP -> execute();
		$COUNT_ROWS = $QUERY_TEMP -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			$ReplMgmt = new Replica_Class();
			
			foreach($QUERY_TEMP as $QueryResult)
			{
				$QUERY_REPLICA = $ReplMgmt -> query_replica($QueryResult['_REPL_UUID']);
				
				$PLAN_JSON = json_decode($QueryResult['_PLAN_JSON']);
				$CLOUD_TYPE = $PLAN_JSON -> CloudType;
				$VENDOR_NAME = $PLAN_JSON -> VendorName;
				$RECOVER_TYPE = $PLAN_JSON -> RecoverType;
				
				$RECOVER_PLAN[] = array(
										'PLAN_UUID' 	=> $QueryResult['_PLAN_UUID'],
										'HOST_NAME' 	=> $QUERY_REPLICA['HOST_NAME'],
										'CLOUD_TYPE' 	=> $CLOUD_TYPE,
										'VENDOR_NAME'	=> $VENDOR_NAME,
										'RECOVER_TYPE'	=> $RECOVER_TYPE,
										'TIMESTAMP'		=> strtotime($QueryResult['_TIMESTAMP'])
									);
			}			
			return $RECOVER_PLAN;
		}
		else
		{
			return false;
		}
	}
		
	
	###########################
	#SAVE RECOVER PLAN
	###########################
	public function save_recover_plan($POST_DATA)
	{
		$ACCT_UUID = $POST_DATA['AcctUUID'];
		$REGN_UUID = $POST_DATA['RegnUUID'];
		$REPL_UUID = $POST_DATA['RecoverPlan']['ReplUUID'];
		$PLAN_UUID = Misc_Class::guid_v4();
		
		$POST_DATA['RecoverPlan']['PlanUUID'] = $PLAN_UUID;
		
		if (!isset($POST_DATA['RecoverPlan']['elastic_address_id']))
		{
			$POST_DATA['RecoverPlan']['elastic_address_id'] = "DynamicAssign";
		}
		
		if (!isset($POST_DATA['RecoverPlan']['private_address_id']))
		{
			$POST_DATA['RecoverPlan']['private_address_id'] = "DynamicAssign";
		}
		
		$RECOVER_PLAN = json_encode($POST_DATA['RecoverPlan'],JSON_UNESCAPED_UNICODE);
				
		$INSERT_TEMPLATE = "INSERT
								INTO _SERVICE_PLAN(
									_ID,
									_ACCT_UUID,								
									_REGN_UUID,
									_REPL_UUID,
									
									_PLAN_UUID,
									_PLAN_JSON,
									
									_TIMESTAMP,
									_STATUS
								)
							VALUE(
								'',
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$REPL_UUID."',
								
								'".$PLAN_UUID."',								
								'".$RECOVER_PLAN."',
										
								'".Misc_Class::current_utc_time()."',
								'Y')";
		
		$this -> DBCON -> prepare($INSERT_TEMPLATE) -> execute();
		return true;
	}
	
	
	###########################
	#UPDATE RECOVER PLAN
	###########################
	public function update_recover_plan($POST_DATA)
	{
		$PLAN_UUID = $POST_DATA['PlanUUID'];
		$POST_DATA['RecoverPlan']['PlanUUID'] = $PLAN_UUID;
		
		if (!isset($POST_DATA['RecoverPlan']['PublicAddrID']))
		{
			$POST_DATA['RecoverPlan']['PublicAddrID'] = "DynamicAssign";
		}
		
		if (!isset($POST_DATA['RecoverPlan']['PrivateAddrID']))
		{
			$POST_DATA['RecoverPlan']['PrivateAddrID'] = "DynamicAssign";
		}		
		
		$RECOVER_PLAN = json_encode($POST_DATA['RecoverPlan'],JSON_UNESCAPED_UNICODE);		
		
		$UPDATE_EXEC = "UPDATE _SERVICE_PLAN
						SET
							_PLAN_JSON	= '".$RECOVER_PLAN."',
							_TIMESTAMP 	= '".Misc_Class::current_utc_time()."'						
						WHERE
							_PLAN_UUID = '".$PLAN_UUID."'";
							
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		
		return true;
	}
	
	###########################
	#DELETE RECOVER PLAN
	###########################
	public function delete_recover_plan($PLAN_UUID)
	{
		$DELETE_EXEC = "UPDATE _SERVICE_PLAN
						SET
							_TIMESTAMP 	= '".Misc_Class::current_utc_time()."',
							_STATUS		= 'X'
						WHERE
							_PLAN_UUID = '".$PLAN_UUID."'";
							
		$this -> DBCON -> prepare($DELETE_EXEC) -> execute();
		
		return true;
	}
	
	
	###########################
	#DELETE RECOVER PLAN BY REPLICA
	###########################
	public function delete_recover_plan_with_replica($REPL_UUID)
	{
		$DELETE_EXEC = "UPDATE _SERVICE_PLAN
						SET
							_TIMESTAMP 	= '".Misc_Class::current_utc_time()."',
							_STATUS		= 'X'
						WHERE
							_STATUS		= 'Y' AND
							_REPL_UUID = '".$REPL_UUID."'";
							
		$QUERY = $this -> DBCON -> prepare($DELETE_EXEC) -> execute();
		
		return true;
	}
	
	
	###########################
	#QUERY RECOVER PLAN
	###########################
	public function query_recover_plan($PLAN_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _SERVICE_PLAN WHERE _PLAN_UUID = '".$PLAN_UUID."' AND _STATUS = 'Y'";
		
		$QUERY_TEMP = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY_TEMP -> execute();
		$COUNT_ROWS = $QUERY_TEMP -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			$TEMP_QUERY = $QUERY_TEMP -> fetch(PDO::FETCH_ASSOC);
			return $TEMP_QUERY;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# SELECT SELF TRANSPORT INFO
	# FOR PASSIVE PACKER REGISTRATION PAIRED WITH THE MANAGEMENT SERVER
	###########################
	public function select_self_transport_info($ACCT_UUID,$MACHINE_ID)
	{
		$SELF_PAIR_CARRIER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_INFO LIKE '%".$MACHINE_ID."%' AND _SERV_TYPE = 'Carrier' AND _STATUS = 'Y' LIMIT 1";
		$QUERY = $this -> DBCON -> prepare($SELF_PAIR_CARRIER);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$CARRIER_UUID = $QueryResult['_SERV_UUID'];
			}
		}
		else
		{
			$SELECT_FIRST_AVAILABLE = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = 'Carrier' AND _LOCATION = 'REMOTE_HOST' AND _STATUS = 'Y' LIMIT 1";
			$QUERY = $this -> DBCON -> prepare($SELECT_FIRST_AVAILABLE);
			$QUERY -> execute();
			$COUNT_ROWS = $QUERY -> rowCount();
			if ($COUNT_ROWS != 0)
			{
				foreach($QUERY as $QueryResult)
				{
					$CARRIER_UUID = $QueryResult['_SERV_UUID'];
				}
			}
			else
			{
				$CARRIER_UUID = false;
			}
		}			
		return $CARRIER_UUID;
	}
	
	
	###########################
	# UPDATE VIRTUAL PACKER BOOT DISK INFORMATION
	###########################
	public function update_virtual_packer_boot_disk_info($HOST_UUID,$BOOT_DISK,$SYSTEM_DISKS)
	{
		$HOST_INFO = $this -> query_host_info($HOST_UUID)['HOST_INFO'];
		
		$DISK_INFO = $HOST_INFO['disks'];
		
		for ($i=0; $i<count($DISK_INFO); $i++)
		{
			if (strtoupper($DISK_INFO[$i]['id']) == strtoupper($BOOT_DISK))
			{
				$DISK_INFO[$i]['boot_from_disk'] = true;
			}
			else
			{
				$DISK_INFO[$i]['boot_from_disk'] = false;
			}
			
			$DISK_INFO[$i]['is_boot'] = false;
		}
		
		if ($SYSTEM_DISKS != '')
		{
			foreach ($SYSTEM_DISKS as $INDEX => $SYSTEM_DISK_UUID)
			{
				for ($x=0; $x<count($DISK_INFO); $x++)
				{
					if (strtoupper($DISK_INFO[$x]['id']) == strtoupper($SYSTEM_DISK_UUID))
					{
						$DISK_INFO[$x]['is_boot'] = true;
					}		
				}		
			}
		}
		
		$HOST_INFO['disks'] = $DISK_INFO;
		$HOST_JSON = str_replace('\\','\\\\',json_encode($HOST_INFO,true));

		$UPDATE_HOST = "UPDATE
								_SERVER_HOST
							SET
								_HOST_INFO = '".$HOST_JSON."',
								_TIMESTAMP = '".Misc_Class::current_utc_time()."'
							WHERE
								_HOST_UUID = '".$HOST_UUID."'";
		
		$this -> DBCON -> prepare($UPDATE_HOST) -> execute();
		
		return true;
	}
	
	###########################
	# UPDATE VIRTUAL PACKER_DISK PARTITION INFO
	###########################
	public function update_virtual_packer_disk_partition_info($HOST_UUID,$DISK_PARTITION_INFO)
	{
		if ($DISK_PARTITION_INFO != '')
		{
			$HOST_INFO = $this -> query_host_info($HOST_UUID)['HOST_INFO'];
			
			$HOST_INFO['disk_partition_infos'] = $DISK_PARTITION_INFO;
			
			$HOST_JSON = str_replace('\\','\\\\',json_encode($HOST_INFO,true));

			$UPDATE_HOST = "UPDATE
									_SERVER_HOST
								SET
									_HOST_INFO = '".$HOST_JSON."',
									_TIMESTAMP = '".Misc_Class::current_utc_time()."'
								WHERE
									_HOST_UUID = '".$HOST_UUID."'";
			
			$this -> DBCON -> prepare($UPDATE_HOST) -> execute();
		}
	}
}
