<?php
###################################
#
#	REPLICA MANAGEMENT
#
###################################
class Replica_Class extends Db_Connection
{
	###########################
	#QUERY REPLICA
	###########################
	public function query_replica($REPL_UUID)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA WHERE _REPL_UUID = '".strtoupper($REPL_UUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			$ServerMgmt  = new Server_Class();
			
			foreach($QUERY as $QueryResult)
			{
				#GET HOST INFORMATION
				$HOST_INFO = $ServerMgmt -> query_host_info($QueryResult['_PACK_UUID']);			

				$UUID_ARRAY = explode('-',$REPL_UUID);			
				
				if ($QueryResult['_CONN_UUID'] == '')
				{
					$CONN_UUID = false;
				}
				else
				{
					$CONN_UUID = $QueryResult['_CONN_UUID'];
				}		
				
				$REPL_DATA = array(
								"ACCT_UUID" 	 => $QueryResult['_ACCT_UUID'],
								"REGN_UUID" 	 => $QueryResult['_REGN_UUID'],
								"REPL_UUID" 	 => $QueryResult['_REPL_UUID'],
								"CLUSTER_UUID"   => $QueryResult['_CLUSTER_UUID'],

								"MGMT_ADDR" 	 => $QueryResult['_MGMT_ADDR'],
								"PACK_UUID" 	 => $QueryResult['_PACK_UUID'],
								"HOST_NAME" 	 => $HOST_INFO['HOST_NAME'],
								"HOST_ADDR"		 => $HOST_INFO['HOST_ADDR'],
								"CONN_UUID" 	 => $CONN_UUID,
								"REPL_TYPE"		 => $QueryResult['_REPL_TYPE'],
								"LOG_LOCATION"	 => $HOST_INFO['HOST_NAME'].'-'.end($UUID_ARRAY),

								"JOBS_JSON" 	 => $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON'],false)),
								"WINPE_JOB"		 => $QueryResult['_WINPE_JOB'],
								"OS_TYPE"		 => $HOST_INFO['OS_TYPE'],								
								"REPL_HIST_JSON" => $QueryResult['_REPL_HIST_JSON'],
								"HOST_TYPE"		 => $HOST_INFO['HOST_TYPE']
							);	
			}
			
			return $REPL_DATA;
		}
		else
		{
			return false;
		}		
	}

	###########################
	#	QUERY REPLICA JOB INFO
	###########################
	public function query_replica_job_info($REPL_UUID)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA WHERE _REPL_UUID = '".strtoupper($REPL_UUID)."'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{			
			foreach($QUERY as $QueryResult)
			{
				$JOBS_JSON = $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON'],false));
			}			
			return $JOBS_JSON;
		}
		else
		{
			return false;
		}
	}	
	
	###########################
	#UPGRADE REPLICA INFORMATION
	###########################
	private function upgrade_replica_info($JOB_JSON)
	{
		if (!isset($JOB_JSON -> init_carrier))
		{
			$JOB_JSON -> init_carrier = false;
		}
		
		if (!isset($JOB_JSON -> is_continuous_data_replication))
		{
			$JOB_JSON -> is_continuous_data_replication = false;
		}
		
		if (!isset($JOB_JSON -> skip_disk))
		{
			$JOB_JSON -> skip_disk = 'KeepThisDisk';
		}
		
		if (!isset($JOB_JSON -> pre_snapshot_script))
		{
			$JOB_JSON -> pre_snapshot_script = '';
		}
		else
		{
			$JOB_JSON -> pre_snapshot_script = str_replace('\"','"',$JOB_JSON -> pre_snapshot_script);
		}
		
		if (!isset($JOB_JSON -> post_snapshot_script))
		{
			$JOB_JSON -> post_snapshot_script = '';
		}
		else
		{
			$JOB_JSON -> post_snapshot_script = str_replace('\"','"',$JOB_JSON -> post_snapshot_script);
		}
		
		if (!isset($JOB_JSON -> webdav_priority_addr))
		{
			$JOB_JSON -> webdav_priority_addr = '';
		}
		
		if (!isset($JOB_JSON -> direct_mode))
		{
			$JOB_JSON -> direct_mode = true;
		}
		
		if (!isset($JOB_JSON -> last_sync))
		{
			$JOB_JSON -> last_sync = time();
		}
		
		if (!isset($JOB_JSON -> delete_time))
		{
			$JOB_JSON -> delete_time = time();
		}
		
		if (!isset($JOB_JSON -> mark_delete))
		{
			$JOB_JSON -> mark_delete = true;
		}
		
		if (!isset($JOB_JSON -> is_azure_blob_mode))
		{
			$JOB_JSON -> is_azure_blob_mode = false;
		}
		
		if (!isset($JOB_JSON -> is_repaired))
		{
			$JOB_JSON -> is_repaired = false;
		}
		
		if (!isset($JOB_JSON -> multi_layer_protection))
		{
			$JOB_JSON -> multi_layer_protection = 'NoSeries';
		}
		
		if (!isset($JOB_JSON -> source_transport_uuid))
		{
			$JOB_JSON -> source_transport_uuid = "";
		}
		
		if (!isset($JOB_JSON -> target_transport_uuid))
		{
			$JOB_JSON -> target_transport_uuid = "";
		}
		
		if (!isset($JOB_JSON -> job_order))
		{
			$JOB_JSON -> job_order = 0;
		}
		
		if (!isset($JOB_JSON -> cascaded_carrier))
		{
			$JOB_JSON -> cascaded_carrier = false;
		}

		if (!isset($JOB_JSON -> is_error))
		{
			$JOB_JSON -> is_error = false;
		}
		
		#CONTROL SYNC TIMEOUT
		if ((time() - $JOB_JSON -> last_sync) > 60)
		{
			$JOB_JSON -> sync_control = true;
		}
		else
		{
			$JOB_JSON -> sync_control = false;
		}
		
		#CONTROL DELETE TIMEOUT
		if ((time() - $JOB_JSON -> delete_time) > 60)
		{
			$JOB_JSON -> delete_control = true;
		}
		else
		{
			$JOB_JSON -> delete_control = false;
		}
		
		return $JOB_JSON;
	}
	
	
	###########################
	#CREATE NEW REPLICA
	###########################
	public function create_replica($ACCT_UUID,$REGN_UUID,$CLUSTER_UUID,$MGMT_ADDR,$PACK_UUID,$REPL_CONF,$IS_WINPE,$REPL_UUID)
	{
		#REPLICA JOB TYPE
		$REGN_TYPE	= 'IronManSync';
		
		#JOB CONFIGURATION
		#MGMT COMMUNICATION PORT
		$MGMT_COMM = Misc_Class::mgmt_comm_type('scheduler');
		
		$TRIGGER_INFO = array(
								'type' 					=> null,
								'triggers' 				=> array(new saasame\transport\job_trigger(array('type'		=> 1, 
																										 'start' 	=> $REPL_CONF['UTC_START_TIME'],
																										 'finish' 	=> '',
																										 'interval' => $REPL_CONF['IntervalMinutes']))),
								'management_id' 		 	 		=> $REPL_UUID,
								'mgmt_addr'				 	 		=> $MGMT_ADDR,
								'mgmt_port'				 	 		=> $MGMT_COMM['mgmt_port'],
								'is_ssl'				 	 		=> $MGMT_COMM['is_ssl'],
								'worker_thread_number'   	 		=> $REPL_CONF['WorkerThreadNumber'],
								'block_mode_enable'  	 	 		=> $REPL_CONF['UseBlockMode'],
								'snapshot_rotation' 	 	 		=> $REPL_CONF['SnapshotsNumber'],
								'loader_thread_number'   	 		=> $REPL_CONF['LoaderThreadNumber'],
								'loader_trigger_percentage'  		=> $REPL_CONF['LoaderTriggerPercentage'],
								'export_type'  				 		=> $REPL_CONF['ExportType'],
								'export_path'  				 		=> $REPL_CONF['ExportPath'],
								'buffer_size'				 		=> $REPL_CONF['BufferSize'],
								'sync_control'				 		=> false,
								'last_sync'							=> time(),
								'set_disk_customized_id'	 		=> $REPL_CONF['SetDiskCustomizedId'],
								'checksum_verify'			 		=> $REPL_CONF['ChecksumVerify'],
								'schedule_pause'			 		=> $REPL_CONF['SchedulePause'],
								'is_compressed'			 	 		=> $REPL_CONF['DataCompressed'],
								'is_checksum'			 	 		=> $REPL_CONF['DataChecksum'],
								'file_system_filter'		 		=> $REPL_CONF['FileSystemFilter'],
								'create_by_partition'		 		=> $REPL_CONF['CreateByPartition'],
								'is_continuous_data_replication'	=> $REPL_CONF['EnableCDR'],
								'extra_gb'					 		=> $REPL_CONF['ExtraGB'],
								'skip_disk'							=> $REPL_CONF['SkipDisk'],
								'cloud_mapping_disk'				=> $REPL_CONF['CloudMappingDisk'],
								'pre_snapshot_script'				=> $REPL_CONF['PreSnapshotScript'],
								'post_snapshot_script'				=> $REPL_CONF['PostSnapshotScript'],
								'job_version'			 	 		=> $REPL_CONF['CreateByVersion'],
								'edit_lock'					 		=> true,
								'is_executing'				 		=> true,
								'init_carrier'						=> false,
								'init_loader'				 		=> false,
								'loader_keep_alive'			 		=> $REPL_CONF['loader_keep_alive'],
								'is_resume'			 		 		=> true,
								'priority_addr'		 		 		=> $REPL_CONF['PriorityAddr'],
								'webdav_priority_addr' 		 		=> $REPL_CONF['WebDavPriorityAddr'],
								'timezone'					 		=> $REPL_CONF['TimeZone'],
								'cloud_type'						=> $REPL_CONF['CloudType'],
								'is_azure_blob_mode'		 		=> $REPL_CONF['IsAzureBlobMode'],
								'is_packer_data_compressed'	 		=> $REPL_CONF['IsPackerDataCompressed'],
								'always_retry'						=> $REPL_CONF['ReplicationRetry'],
								'is_encrypted'						=> $REPL_CONF['PackerEncryption'],
								'excluded_paths'					=> $REPL_CONF['ExcludedPaths'],
								'post_loader_script'				=> $REPL_CONF['PostLoaderScript'],
								'multi_layer_protection'			=> $REPL_CONF['MultiLayerProtection'],
								'source_transport_uuid'				=> '',
								'target_transport_uuid'				=> '',
								'job_order'							=> 0,
								'cascaded_carrier'					=> $REPL_CONF['CascadedCarrier'],
								'delete_time'						=> time(),
								'mark_delete'						=> false,
								'is_error'							=> false
							);
		
		if( isset($REPL_CONF['VMWARE_STORAGE']) ){
			$TRIGGER_INFO["VMWARE_STORAGE"] = $REPL_CONF['VMWARE_STORAGE'];
			$TRIGGER_INFO["VMWARE_ESX"] = $REPL_CONF['VMWARE_ESX'];
			$TRIGGER_INFO["VMWARE_FOLDER"] = $REPL_CONF['VMWARE_FOLDER'];
			$TRIGGER_INFO["VMWARE_THIN_PROVISIONED"] = $REPL_CONF['VMWARE_THIN_PROVISIONED'];
		}
		$JOB_CONFIG = str_replace('\\','\\\\',json_encode($TRIGGER_INFO));
		
		#SET WINPE STATUS
		if ($IS_WINPE == TRUE)
		{
			$WINPE_JOB = 'Y';
		}
		else
		{
			$WINPE_JOB = 'N';
		}		
		
		$INSERT_EXEC = "INSERT 
							INTO _REPLICA(
								_ID,
							
								_ACCT_UUID,
								_REGN_UUID,
								_REPL_UUID,
								_CLUSTER_UUID,
								
								_JOBS_JSON,
								_MGMT_ADDR,
								_PACK_UUID,
								
								_REPL_TYPE,
								_REPL_CREATE_TIME,
								_WINPE_JOB,
								_STATUS)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$REPL_UUID."',
								'".$CLUSTER_UUID."',
								
								'".$JOB_CONFIG."',
								'".$MGMT_ADDR."',
								'".$PACK_UUID."',
								
								'".$REGN_TYPE."',
								'".Misc_Class::current_utc_time()."',
								'".$WINPE_JOB."',
								'Y')";
	
		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		$HIST_MSG = $this -> job_msg('Replication process started.');	
		$this -> update_job_msg($REPL_UUID,$HIST_MSG,'Replica');
			
		return $REPL_UUID;
	}
	
	
	###########################
	#UPDATE REPLICA CONNECTION
	###########################
	public function update_replica_connection($REPL_UUID,$CONN_JSON)
	{
		$UPDATE_EXEC = "UPDATE _REPLICA
						SET
							_CONN_UUID = '".$CONN_JSON."'
						WHERE
							_REPL_UUID = '".$REPL_UUID."'";
							
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
	}
	
	
	###########################
	#UPDATE JOBS MESSAGE AND HISTORY
	###########################
	public function job_msg($FORMAT,$ARGUMENTS=array())
	{
		$NowTime = Misc_Class::current_utc_time();
		
		if (count($ARGUMENTS) != 0)
		{
			for ($i=0; $i<count($ARGUMENTS); $i++){$ARGUMENTS_ID[] = '%'.($i+1).'%';}
		}
		else
		{
			$ARGUMENTS_ID = array();
		}
		
		$MESSAGE = str_replace($ARGUMENTS_ID,$ARGUMENTS,$FORMAT);
		
		$JOB_MESSAGE = (object)array(
								'time' 			=> $NowTime,
								'state' 		=> 0,
								'error' 		=> 0,
								'format'		=> $FORMAT,
								'arguments'		=> $ARGUMENTS,
								'description' 	=> $MESSAGE
							);
						
		return $JOB_MESSAGE;
	}
	
	
	###########################
	#UPDATE JOBS MESSAGE AND HISTORY
	###########################
	public function update_job_msg($JOBS_UUID,$HIST_JSON,$TYPE,$SYNC_TIME = null)
	{
		if ($SYNC_TIME == null)
		{
			$SYNC_TIME = Misc_Class::current_utc_time();
		}		
		
		if (!isset($HIST_JSON -> is_display))
		{
			$HIST_JSON -> is_display = true;
			$IS_DISPLAY = 'Y';
		}
		elseif($HIST_JSON -> is_display == false)
		{
			$IS_DISPLAY = 'N';
		}
		else
		{
			$IS_DISPLAY = 'Y';
		}
		
		$HIST_JSON -> sync_time = $SYNC_TIME;
		$HIST_TIME = $HIST_JSON -> time;
		
		/*
		$CHECK_DUPLICATE = "SELECT * FROM _JOBS_HISTORY WHERE _JOBS_UUID = '".$JOBS_UUID."' AND _HIST_TIME = '".$HIST_TIME."'";
		$QUERY = $this -> DBCON -> prepare($CHECK_DUPLICATE);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{
			
		}
		*/
		$HIST_JSON = json_encode($HIST_JSON);
		$HIST_JSON = str_replace("\\","\\\\",$HIST_JSON);
		$HIST_JSON = str_replace("'","\'",$HIST_JSON);	
		
		if ($TYPE == 'Replica')
		{
			$UPDATE_EXEC = "UPDATE _REPLICA
							SET 
								_REPL_HIST_JSON = '".$HIST_JSON."'
							WHERE
								_REPL_UUID = '".$JOBS_UUID."'";
							
			$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		}
		else
		{
			$UPDATE_EXEC = "UPDATE _SERVICE
							SET 
								_SERV_HIST_JSON = '".$HIST_JSON."'
							WHERE
								_SERV_UUID = '".$JOBS_UUID."'";
							
			$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		}
		
		#INSERT OR UPDATE JOBS MESSAGE
		$COMBINE_ID = $this -> check_combine_history_msg($JOBS_UUID,$HIST_JSON);

		if ($COMBINE_ID == FALSE)
		{			
			$JOB_HISTORY = "INSERT 
								INTO _JOBS_HISTORY(
									_ID,
									_JOBS_UUID,
									_HIST_JSON,
									_HIST_TIME,
									_SYNC_TIME,
									_IS_DISPLAY
								)
							VALUE(
								'',
								'".$JOBS_UUID."',
								'".$HIST_JSON."',
								'".$HIST_TIME."',
								'".$SYNC_TIME."',
								'".$IS_DISPLAY."'
							)";
		}
		else
		{
			$JOB_HISTORY = "UPDATE 
								_JOBS_HISTORY
							SET
								_HIST_JSON  = '".$HIST_JSON."',
								_HIST_TIME  = '".$HIST_TIME."',
								_SYNC_TIME  = '".$SYNC_TIME."',
								_IS_DISPLAY = '".$IS_DISPLAY."'
							WHERE
								_ID = '".$COMBINE_ID."' AND
								_JOBS_UUID = '".$JOBS_UUID."'
							";
		}
		
		$this -> DBCON -> prepare($JOB_HISTORY) -> execute();
		
		sleep(1);			
	}

	###########################
	#CHECK COMBINE HISTORY MSG
	###########################
	private function check_combine_history_msg($JOB_UUID,$HIST_JSON)
	{
		$KEYWORD = array('xyzzy','Progress : %1%');
		
		$Response = false;
		
		for ($i=0; $i<count($KEYWORD); $i++)
		{
			if (json_decode($HIST_JSON, true)['description'] == $KEYWORD[$i])
			{
				$MATCH_KEYWORD = implode("|",$KEYWORD);
				$QUERY_MSG = "SELECT * FROM _JOBS_HISTORY WHERE _JOBS_UUID = '".$JOB_UUID."' AND _HIST_JSON REGEXP '".$MATCH_KEYWORD."' ORDER BY _ID DESC LIMIT 1";
				$QUERY = $this -> DBCON -> prepare($QUERY_MSG);
				$QUERY -> execute();
				$COUNT_ROWS = $QUERY -> rowCount();
				
				if ($COUNT_ROWS == 0)
				{
					$Response = false;
				}
				else
				{
					$Response = $QUERY -> fetch(PDO::FETCH_ASSOC)['_ID'];
				}
				break;
			}
		}
		return $Response;
	}

	###########################
	#LIST AND QUERY REPLICA
	###########################
	public function list_replica($ACCT_UUID = null,$REPL_UUID = null)
	{
		if ($ACCT_UUID == null)
		{
			$GET_EXEC 	= "SELECT * FROM _REPLICA WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y'";
		}
		elseif ($REPL_UUID == null)
		{
			$GET_EXEC 	= "SELECT * FROM _REPLICA WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		}
	
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			$ServerMgmt  = new Server_Class();
			
			foreach($QUERY as $QueryResult)
			{
				#GET REPLICA HISTORY MESSAGE
				$HISTORY_MSG = $this -> list_job_history($QueryResult['_REPL_UUID'],10);
					
				$REPL_MESG 	 	= $HISTORY_MSG[0]['description'];					
				$SYNC_TIME 	 	= $HISTORY_MSG[0]['time'];
				$REPL_TIME 	 	= $HISTORY_MSG[0]['unsync_time'];
				$REPL_FORMAT 	= $HISTORY_MSG[0]['format'];
				$REPL_ARGUMENTS = $HISTORY_MSG[0]['arguments'];
				
				#GET HOST INFORMATION
				$HOST_INFO = $ServerMgmt -> query_host_info($QueryResult['_PACK_UUID']);
				
				#SET REPL PROGRESS INFO
				$REPL_DISK        = $this -> query_replica_disk($QueryResult['_REPL_UUID']);
				$BACKUP_SIZE      = 0;
				$PROGRESS_SIZE	  = 0;
				$PROGRESS_PRECENT = 0;
				$PROGRESS_SPEED   = 0;
				$PROGRESS_TIME    = time() - strtotime($QueryResult['_REPL_CREATE_TIME']);
				
				$LOADER_PROGRESS_SIZE = 0;
				$LOADER_PROGRESS_PRECENT = 0;
				$LOADER_PROGRESS_SPEED = 0;
				
				#SET DEFAULT
				$NUM_SNAPSHOT = null;
				$IDLE_COUNT = null;
				
				#GET JOB EXECUTING STATUS
				$JOB_INFO = $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON']));
				$IS_EXECUTING = $JOB_INFO -> is_executing;
				$MULTI_LAYER = $JOB_INFO -> multi_layer_protection;				
			
				if ($REPL_DISK != false)
				{
					for ($DISK=0; $DISK<count($REPL_DISK); $DISK++)
					{
						$REPL_UUID = $REPL_DISK[$DISK]['REPL_UUID'];
						$DISK_UUID = $REPL_DISK[$DISK]['DISK_UUID'];
						$SNAPSHOT_INFO = $this -> query_replica_snapshot($REPL_UUID,$DISK_UUID);
			
						if ($SNAPSHOT_INFO != FALSE)
						{
							#RESET IDLE SNAPSHOT COUNT
							$IDLE_COUNT = 0;
							
							$NUM_SNAPSHOT = count($SNAPSHOT_INFO);
							for ($SNAP=0; $SNAP<$NUM_SNAPSHOT; $SNAP++)
							{
								if ($SNAPSHOT_INFO[$SNAP]['SNAP_OPEN'] == 'N')
								{
									$BACKUP_SIZE 			 = $BACKUP_SIZE + $SNAPSHOT_INFO[$SNAP]['BACKUP_SIZE'];
									$PROGRESS_SIZE 			 = $PROGRESS_SIZE + $SNAPSHOT_INFO[$SNAP]['PROGRESS_SIZE'];
									
									$LOADER_PROGRESS_SIZE	 = $LOADER_PROGRESS_SIZE + $SNAPSHOT_INFO[$SNAP]['LOADER_DATA'];
									$LOADER_PROGRESS_TIME 	 = time() - strtotime($SNAPSHOT_INFO[$SNAP]['SNAP_TIME']);

									if ($PROGRESS_SIZE != 0)
									{
										$PROGRESS_PRECENT 		 = number_format(($PROGRESS_SIZE / $BACKUP_SIZE)*100,1);
										$LOADER_PROGRESS_PRECENT = number_format(($LOADER_PROGRESS_SIZE / $BACKUP_SIZE)*100,1);
										
										$PROGRESS_SPEED 	     = number_format($PROGRESS_SIZE / $PROGRESS_TIME / 1024 / 1024, 1);
										$LOADER_PROGRESS_SPEED   = number_format($LOADER_PROGRESS_SIZE / $LOADER_PROGRESS_TIME / 1024 / 1024, 1);
									}
									else
									{
										#VALUE REFERENCE FROM LINE 237 ~ 247
									}
								}
								else
								{
									$PROGRESS_PRECENT = number_format(100,1);
									$PROGRESS_SPEED   = '0.0';
									
									$LOADER_PROGRESS_PRECENT = number_format(100,1);
									$LOADER_PROGRESS_SPEED 	 = '0.0';
									
									$IDLE_COUNT = $IDLE_COUNT + 1;
								}
							}
						}					
					}
				}
								
				#TO DISPLAY PRRCENTAGE
				if ($NUM_SNAPSHOT == null)
				{
					$PROGRESS_PRECENT = number_format(0,1);
					$PROGRESS_SPEED   = '0.0';
						
					$LOADER_PROGRESS_PRECENT = number_format(0,1);
					$LOADER_PROGRESS_SPEED 	 = '0.0';				
				}
				else
				{
					if ($NUM_SNAPSHOT == $IDLE_COUNT AND $IS_EXECUTING != TRUE)
					{
						$PROGRESS_PRECENT = number_format(100,1);
						$PROGRESS_SPEED   = '0.0';
						
						$LOADER_PROGRESS_PRECENT = number_format(100,1);
						$LOADER_PROGRESS_SPEED 	 = '0.0';
					}
					else
					{
						if ($PROGRESS_PRECENT > 98)
						{
							$PROGRESS_PRECENT = number_format(100,1);
							$PROGRESS_SPEED   = '0.0';
						}
						else
						{
							#VALUE REFERENCE FROM LINE 507 ~ 508
						}
					
						if ($LOADER_PROGRESS_PRECENT > 97)
						{
							$LOADER_PROGRESS_PRECENT = number_format(100,1);
							$LOADER_PROGRESS_SPEED 	 = '0.0';
						}
						else
						{
							#VALUE REFERENCE FROM LINE 510 ~ 511
						}
					}
				}

				#EXPORT JOB
				$EXPORT_PATH = json_decode($QueryResult['_JOBS_JSON']) -> export_path;
				if ($EXPORT_PATH != '')
				{
					$EXPORT_JOB = 'Y';
				}
				else
				{
					$EXPORT_JOB = 'N';
				}
				
				#LIST REPLICA JOB INFORMATION
				if ($ACCT_UUID == null)
				{
					$REPL_DATA = array(
									"ACCT_UUID" 				=> $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 				=> $QueryResult['_REGN_UUID'],
									"REPL_UUID" 				=> $QueryResult['_REPL_UUID'],
									"PACK_UUID" 	 			=> $QueryResult['_PACK_UUID'],
									"CLUSTER_UUID" 				=> $QueryResult['_CLUSTER_UUID'],
									"CONN_UUID" 				=> $QueryResult['_CONN_UUID'],
									"JOBS_JSON" 				=> $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON'])),
									"MGMT_ADDR" 				=> $QueryResult['_MGMT_ADDR'],
									"EXPORT_JOB"				=> $EXPORT_JOB,
									"WINPE_JOB"					=> $QueryResult['_WINPE_JOB'],
									"HOST_SERV"					=> $HOST_INFO['HOST_SERV']['SERV_MISC']['ADDR'],
									"HOST_NAME" 				=> $HOST_INFO['HOST_NAME'],
									"HOST_ADDR" 				=> $HOST_INFO['HOST_ADDR'],
									"BACKUP_SIZE" 				=> $BACKUP_SIZE,
									"PROGRESS_SIZE" 			=> $PROGRESS_SIZE,
									"PROGRESS_PRECENT" 			=> $PROGRESS_PRECENT,
									"PROGRESS_SPEED"			=> $PROGRESS_SPEED,
									"LOADER_PROGRESS_PRECENT" 	=> $LOADER_PROGRESS_PRECENT,
									"LOADER_PROGRESS_SPEED"		=> $LOADER_PROGRESS_SPEED,
									"REPL_MESG"					=> $REPL_MESG,
									"REPL_FORMAT"				=> $REPL_FORMAT,
									"REPL_ARGUMENTS"			=> $REPL_ARGUMENTS,
									"SYNC_TIME"					=> $SYNC_TIME,
									"REPL_TIME"					=> $REPL_TIME,
									"MULTI_LAYER"				=> $MULTI_LAYER
								);
				}
				else
				{	
					$REPL_DATA[] = array(
									"ACCT_UUID" 				=> $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 				=> $QueryResult['_REGN_UUID'],
									"REPL_UUID" 				=> $QueryResult['_REPL_UUID'],
									"PACK_UUID" 	 			=> $QueryResult['_PACK_UUID'],
									"CLUSTER_UUID" 				=> $QueryResult['_CLUSTER_UUID'],
									"CONN_UUID" 				=> $QueryResult['_CONN_UUID'],
									"JOBS_JSON" 				=> $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON'])),
									"MGMT_ADDR" 				=> $QueryResult['_MGMT_ADDR'],
									"EXPORT_JOB"				=> $EXPORT_JOB,
									"WINPE_JOB"					=> $QueryResult['_WINPE_JOB'],
									"HOST_SERV"					=> $HOST_INFO['HOST_SERV']['SERV_MISC']['ADDR'],
									"HOST_NAME" 				=> $HOST_INFO['HOST_NAME'],
									"HOST_ADDR" 				=> $HOST_INFO['HOST_ADDR'],
									"BACKUP_SIZE" 				=> $BACKUP_SIZE,
									"PROGRESS_SIZE" 			=> $PROGRESS_SIZE,
									"PROGRESS_PRECENT" 			=> $PROGRESS_PRECENT,
									"PROGRESS_SPEED"			=> $PROGRESS_SPEED,
									"LOADER_PROGRESS_PRECENT" 	=> $LOADER_PROGRESS_PRECENT,
									"LOADER_PROGRESS_SPEED"		=> $LOADER_PROGRESS_SPEED,
									"REPL_MESG" 				=> $REPL_MESG,
									"REPL_FORMAT"				=> $REPL_FORMAT,
									"REPL_ARGUMENTS"			=> $REPL_ARGUMENTS,
									"SYNC_TIME"					=> $SYNC_TIME,
									"REPL_TIME"					=> $REPL_TIME,
									"MULTI_LAYER"				=> $MULTI_LAYER
									
								);				
				}
			}
			return $REPL_DATA;
		}
		else
		{
			return false;
		}		
	}
	
	
	###########################
	#LIST AND QUERY REPLICA WITH PLAN
	###########################
	public function list_replica_with_plan($ACCT_UUID)
	{
		#QUERY REPLICA
		$GET_REPLICA = "SELECT * FROM _REPLICA WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		
		$QUERY_REPLICA = $this -> DBCON -> prepare($GET_REPLICA);
		$QUERY_REPLICA -> execute();
		$REPLICA_ROWS = $QUERY_REPLICA -> rowCount();
	
		$REPL_DATA = false;
		$ServiceMgmt = new Service_Class();
		
		if ($REPLICA_ROWS != 0)
		{
			foreach($QUERY_REPLICA as $ReplicaResult)
			{
				$GET_REPL = $this -> query_replica($ReplicaResult['_REPL_UUID']);
			
				#GET REPLICA HISTORY MESSAGE
				$HISTORY_MSG = $this -> list_job_history($GET_REPL['REPL_UUID'],10);

				$CT_SEARCH = array('OPENSTACK','AWS','Azure','Aliyun','天翼云');
				$CT_REPLACE = array('OpenStack','AWS','Azure','Alibaba Cloud','Ctyun');
				$CLOUD_TYPE = str_replace($CT_SEARCH, $CT_REPLACE, $GET_REPL['JOBS_JSON'] -> cloud_type);

				$REPL_DATA[] = array(
									'ACCT_UUID'				=> $GET_REPL['ACCT_UUID'],
									'REGN_UUID' 			=> $GET_REPL['REGN_UUID'],
									'REPL_UUID' 			=> $GET_REPL['REPL_UUID'],
									'HOST_NAME'				=> $GET_REPL['HOST_NAME'],
									'PLAN_UUID'				=> '00000000-0000-0000-0000-000000000000',
									'MSG_FORMAT'			=> $HISTORY_MSG[0]['format'],
									'MSG_ARGUMENTS'			=> $HISTORY_MSG[0]['arguments'],
									'REPL_MESG'				=> $HISTORY_MSG[0]['description'],
									'SYNC_TIME'				=> $HISTORY_MSG[0]['time'],
									'REPL_TIME'				=> $HISTORY_MSG[0]['unsync_time'],
									'IS_RCD_JOB'			=> $GET_REPL['WINPE_JOB'] == 'N' ? false : true,
									'IS_EXPORT_JOB'			=> $GET_REPL['JOBS_JSON'] -> export_path == '' ? false : true,
									'IS_TEMPLATE'			=> false,
									'HAS_RECOVERY_RUNNING'	=> $ServiceMgmt -> check_running_service($GET_REPL['REPL_UUID'],false) ? false : true,
									'CLOUD_TYPE'			=> $CLOUD_TYPE,
									'MULTI_LAYER'			=> $GET_REPL['JOBS_JSON'] -> multi_layer_protection
								);				
			}
		}
		
		#QUERY PLAN
		$GET_PLAN = "SELECT * FROM _SERVICE_PLAN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		
		$QUERY_PLAN = $this -> DBCON -> prepare($GET_PLAN);
		$QUERY_PLAN -> execute();
		$PLAN_ROWS = $QUERY_PLAN -> rowCount();
		if ($PLAN_ROWS != 0)
		{
			foreach($QUERY_PLAN as $PlanResult)
			{	
				$GET_REPL = $this -> query_replica($PlanResult['_REPL_UUID']);
						
				$PLAN_INFO = json_decode($PlanResult['_PLAN_JSON']);
			
				$CT_SEARCH = array('OPENSTACK','AWS','Azure','Aliyun','天翼云');
				$CT_REPLACE = array('OpenStack','AWS','Azure','Alibaba Cloud','Ctyun');
				$CLOUD_TYPE = str_replace($CT_SEARCH, $CT_REPLACE, $PLAN_INFO -> VendorName);

				$RE_SEARCH = array('RECOVERY_PM','RECOVERY_DR','RECOVERY_DT');
				$RE_REPLACE = array('Planned Migration','Disaster Recovery','Development Testing');
				$RECOVERY_TYPE = str_replace($RE_SEARCH, $RE_REPLACE, $PLAN_INFO -> RecoverType);
				
				$REPL_FORMAT 	= $RECOVERY_TYPE.' (%1%)';
				$REPL_ARGUMENTS = array($CLOUD_TYPE);
				$REPL_MESG 		= $RECOVERY_TYPE.' ('.$CLOUD_TYPE.')';
							
				$REPL_DATA[] = array(
									'ACCT_UUID' 			=> $GET_REPL['ACCT_UUID'],
									'REGN_UUID' 			=> $GET_REPL['REGN_UUID'],
									'REPL_UUID' 			=> $GET_REPL['REPL_UUID'],
									'HOST_NAME'				=> $GET_REPL['HOST_NAME'],
									'PLAN_UUID' 			=> $PlanResult['_PLAN_UUID'],
									'MSG_FORMAT'			=> $REPL_FORMAT,
									'MSG_ARGUMENTS'			=> $REPL_ARGUMENTS,
									'REPL_MESG'				=> $REPL_MESG,
									'SYNC_TIME'				=> strtotime($PlanResult['_TIMESTAMP']),
									'REPL_TIME'				=> strtotime($PlanResult['_TIMESTAMP']),
									'IS_RCD_JOB'			=> $GET_REPL['WINPE_JOB'] == 'N' ? false : true,
									'IS_EXPORT_JOB'			=> $GET_REPL['JOBS_JSON'] -> export_path == '' ? false : true,
									'IS_TEMPLATE'			=> true,
									'HAS_RECOVERY_RUNNING'	=> $ServiceMgmt -> check_running_service($GET_REPL['REPL_UUID'],false) ? false : true,
									'CLOUD_TYPE'			=> $CLOUD_TYPE,
									'MULTI_LAYER'			=> $GET_REPL['JOBS_JSON'] -> multi_layer_protection
								);				
			}		
		}
		
		return $REPL_DATA;
	}
	
	
	###########################
	#DELETE REPLICA
	###########################
	public function delete_replica($REPL_UUID)
	{
		$DELETE_EXEC = "UPDATE _REPLICA
						SET
							_REPL_DELETE_TIME 	= '".Misc_Class::current_utc_time()."',
							_STATUS				= 'X'
						WHERE
							_REPL_UUID 			= '".$REPL_UUID."'";
		
		$this -> DBCON -> prepare($DELETE_EXEC) -> execute();
		
		$this -> delete_replica_disk($REPL_UUID);
		
		$this -> delete_replica_disk_snapshot($REPL_UUID);
		
		return true;
	}
	
	###########################
	#DELETE REPLICA DISK
	###########################
	private function delete_replica_disk($REPL_UUID)
	{
		$DELETE_EXEC = "UPDATE _REPLICA_DISK
						SET
							_TIMESTAMP 		= '".Misc_Class::current_utc_time()."',
							_STATUS			= 'X'
						WHERE
							_REPL_UUID 		= '".$REPL_UUID."'";
		
		$this -> DBCON -> prepare($DELETE_EXEC) -> execute();
	}
	
	###########################
	#DELETE REPLICA DISK SNAPSHOT
	###########################
	private function delete_replica_disk_snapshot($REPL_UUID)
	{
		$DELETE_EXEC = "UPDATE _REPLICA_SNAP
						SET
							_TIMESTAMP 		= '".Misc_Class::current_utc_time()."',
							_STATUS			= 'X'
						WHERE
							_REPL_UUID 		= '".$REPL_UUID."'";
		
		$this -> DBCON -> prepare($DELETE_EXEC) -> execute();
	}
	
	###########################
	#QUERY REPLICA DISK
	###########################
	public function query_replica_disk($REPL_UUID)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA_DISK WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS != 'X' ORDER BY _ID ASC";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			$REPL_QUERY = $this -> query_replica($REPL_UUID);
			
			#GET HOST BOOT DISK
			$ServerMgmt = new Server_Class();
			$HOST_INFO = $ServerMgmt -> query_host_info($REPL_QUERY['PACK_UUID']);

			$BOOT_DISK_INDEX = array();
			$SYSTEM_DISK = array();
			
			#FOR PHYSICAL PACKER
			if ($HOST_INFO['HOST_TYPE'] == 'Physical')
			{
				if (isset($HOST_INFO['HOST_INFO']['disk_infos']))
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_infos'];
				
					for ($i=0; $i<count($HOST_DISK); $i++)
					{
						if ($HOST_DISK[$i]['boot_from_disk'] == TRUE)
						{
							$BOOT_DISK_INDEX[] = strtoupper($HOST_DISK[$i]['uri']);
						}
						
						if ($HOST_DISK[$i]['is_boot'] == TRUE)
						{
							$SYSTEM_DISK[] = strtoupper($HOST_DISK[$i]['uri']);
						}
					}
				}
				else
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_infos'];
					$BOOT_DISK_INDEX[] = strtoupper($HOST_DISK[0]['uri']);
					$SYSTEM_DISK[] = strtoupper($HOST_DISK[0]['uri']);
				}
			}
	
			#FOR VIRTUAL PACKER
			if ($HOST_INFO['HOST_TYPE'] == 'Virtual')
			{			
				if (isset($HOST_INFO['HOST_INFO']['disks'][0]['boot_from_disk']))
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disks'];
					for ($i=0; $i<count($HOST_DISK); $i++)
					{
						if ($HOST_DISK[$i]['boot_from_disk'] == TRUE)
						{
							$BOOT_DISK_INDEX[] = strtoupper($HOST_DISK[$i]['id']);
						}
					
						if ($HOST_DISK[$i]['is_boot'] == TRUE)
						{
							$SYSTEM_DISK[] = strtoupper($HOST_DISK[$i]['id']);
						}
					}
				}
				else
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disks'];
					$BOOT_DISK_INDEX[] = strtoupper($HOST_DISK[0]['id']);
					$SYSTEM_DISK[] = strtoupper($HOST_DISK[0]['id']);
				}
			}

			foreach($QUERY as $QueryResult)
			{
				$REPL_UUID 		= $QueryResult['_REPL_UUID'];
				$DISK_UUID 		= $QueryResult['_DISK_UUID'];
				$PACK_URI  		= $QueryResult['_PACK_URI'];
		
				if (in_array(strtoupper($PACK_URI),$BOOT_DISK_INDEX))
				{				
					$IS_BOOT = true;
				}
				else
				{
					$IS_BOOT = false;
				}					
				
				if (in_array(strtoupper($PACK_URI),$SYSTEM_DISK))
				{
					$IS_SYSTEM_DISK = true;
				}
				else
				{
					$IS_SYSTEM_DISK = false;
				}
								
				$SNAP_MAPS = $this -> get_snapshot_mapping($REPL_UUID,$DISK_UUID);
				
				$REPL_DISK_DATA[] = array(
									"ID"					=> $QueryResult['_ID'],
									"DISK_UUID" 	 		=> $DISK_UUID,
									"REPL_UUID" 	 		=> $REPL_UUID,
									"HOST_UUID" 	 		=> $QueryResult['_HOST_UUID'],
									"PACK_URI"		 		=> $PACK_URI,
									"SNAPSHOT_MAPPING"		=> $SNAP_MAPS,
									"DISK_SIZE" 	 		=> $QueryResult['_DISK_SIZE'],
									"SCSI_ADDR"				=> $QueryResult['_SCSI_ADDR'],
									"PURGE_DATA"			=> $QueryResult['_PURGE_DATA'],
									"OPEN_DISK"				=> $QueryResult['_OPEN_DISK'],
									"TIMESTAMP" 	 		=> $QueryResult['_TIMESTAMP'],
									"IS_BOOT"				=> $IS_BOOT,
									"SYSTEM_DISK"			=> $IS_SYSTEM_DISK,
									"STATUS" 	 			=> $QueryResult['_STATUS']									
								);
			}
			return $REPL_DISK_DATA;			
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	#	DATAMODE AGENT DISK MAPPING
	###########################
	public function datamode_agent_disk_mapping($REPL_UUID,$DISK_FILTER,$RECOVERY_MODE)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		$GET_EXEC = "SELECT * FROM _REPLICA_DISK WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS != 'X' ORDER BY _ID ASC";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			$REPL_QUERY = $this -> query_replica($REPL_UUID);
			
			#GET HOST BOOT DISK
			$ServerMgmt = new Server_Class();
			$HOST_INFO = $ServerMgmt -> query_host_info($REPL_QUERY['PACK_UUID']);

			$BOOT_DISK_INDEX = array();
			$SYSTEM_DISK = array();
			
			#FOR PHYSICAL PACKER
			if ($HOST_INFO['HOST_TYPE'] == 'Physical')
			{
				if (isset($HOST_INFO['HOST_INFO']['disk_infos']))
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_infos'];
					$PARTITION_INFO = $HOST_INFO['HOST_INFO']['partition_infos'];
			
					for ($i=0; $i<count($HOST_DISK); $i++)
					{
						for ($p=0; $p<count($PARTITION_INFO); $p++)
						{
							if ($HOST_DISK[$i]['number'] == $PARTITION_INFO[$p]['disk_number'])
							{
								$HOST_DISK[$i]['partitions'][] = $PARTITION_INFO[$p];
							}
						}
					}
				}
			}
			
		
			#FOR VIRTUAL PACKER
			if ($HOST_INFO['HOST_TYPE'] == 'Virtual')
			{			
				if (isset($HOST_INFO['HOST_INFO']['disk_partition_infos']))
				{
					$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_partition_infos'];
					for ($i=0; $i<count($HOST_DISK); $i++)
					{
						$HOST_DISK[$i]['uri'] = $HOST_DISK[$i]['id'];
						unset($HOST_DISK[$i]['id']);
					}
					
				}
				else
				{
					return false;
				}
			}

			#FILTER OUT UN-SELECTED DISK
			$DISK_FILTER = array_keys(explode(',',$DISK_FILTER),'false');
		
			if ($RECOVERY_MODE == 'RECOVERY_PM' OR $RECOVERY_MODE == 'RECOVERY_PLAN')
			{
				foreach ($DISK_FILTER as $FILTER_KEY)
				{
					$PM_HOST_DISK[$FILTER_KEY] = $HOST_DISK[$FILTER_KEY];
				}
				unset($HOST_DISK);
				$HOST_DISK = array_values($PM_HOST_DISK); #RE-INDEX
			}
			else
			{			
				foreach ($DISK_FILTER as $FILTER_KEY)
				{
					unset($HOST_DISK[$FILTER_KEY]);
				}
				$HOST_DISK = array_values($HOST_DISK);
			}
		
			foreach($QUERY as $QueryResult)
			{
				for ($x=0; $x<count($HOST_DISK); $x++)
				{
					if ($QueryResult['_PACK_URI'] == $HOST_DISK[$x]['uri'])
					{
						for ($p=0; $p<count($HOST_DISK[$x]['partitions']); $p++)
						{
							$DRIVE_LETTER = isset($HOST_DISK[$x]['partitions'][$p]['drive_letter']) ? $HOST_DISK[$x]['partitions'][$p]['drive_letter'] : "Not Applicable";

							$PARTATION_MAPPING[] = array(
														'volume'				=> $x,
														'guid' 					=> $HOST_DISK[$x]['guid'],
														'is_system_disk' 		=> $HOST_DISK[$x]['is_system'],
														'partition_offset' 		=> $HOST_DISK[$x]['partitions'][$p]['offset'],
														'partition_size' 		=> $HOST_DISK[$x]['partitions'][$p]['size'],
														'partition_number' 		=> $HOST_DISK[$x]['partitions'][$p]['partition_number'],
														'partition_style' 		=> $HOST_DISK[$x]['partition_style'],
														'partition_style_name' 	=> $this -> partition_style($HOST_DISK[$x]['partition_style']),
														'signature' 			=> $HOST_DISK[$x]['signature'],
														'drive_letter'			=> $DRIVE_LETTER,
														'path'					=> ''
													);
						}
					}			
				}
			}
			
			return array('partitions' => $PARTATION_MAPPING);
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#	DECODE PARTATION STYLE NAME
	###########################
	private function partition_style($NUMBER)
	{
		switch ($NUMBER)
		{
			case 1:
				return 'MBR';
			break;
			
			case 2:
				return 'GPT';
			break;
			
			default:
				return 'Unknown';
		}
	}
	
	
	###########################
	#	GET REPLICA DISK INFO
	###########################
	public function get_replica_disk_info($REPL_UUID)
	{
		$GET_REPL_DISK = "SELECT * FROM _REPLICA_DISK WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS != 'X'";
	
		$QUERY = $this -> DBCON -> prepare($GET_REPL_DISK);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$DISK_INFO[] = array(
									"ID"			=> $QueryResult['_ID'],
									"DISK_UUID" 	=> $QueryResult['_DISK_UUID'],
									"REPL_UUID" 	=> $QueryResult['_REPL_UUID'],
									"HOST_UUID" 	=> $QueryResult['_HOST_UUID'],
									"DISK_SIZE" 	=> $QueryResult['_DISK_SIZE'],
									"SCSI_ADDR"		=> $QueryResult['_SCSI_ADDR'],
									"PACK_URI"  	=> $QueryResult['_PACK_URI'],
									"PURGE_DATA"  	=> $QueryResult['_PURGE_DATA'],
									"OPEN_DISK"		=> $QueryResult['_OPEN_DISK'],
									"CBT_INFO"		=> $QueryResult['_CBT_INFO'],									
									"TIMESTAMP" 	=> $QueryResult['_TIMESTAMP']
				);
			}
			return $DISK_INFO;
		}
		else
		{
			return false;
		}
	}
		
	###########################
	#QUERY REPLICA DISK BY DISK UUID
	###########################
	public function query_replica_disk_by_uuid($REPL_UUID,$DISK_UUID)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA_DISK WHERE _REPL_UUID = '".$REPL_UUID."' AND _DISK_UUID = '".$DISK_UUID."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$SNAP_MAPS = $this -> get_snapshot_mapping($REPL_UUID,$DISK_UUID);
				$PACK_URI = $QueryResult['_PACK_URI'];
				
				#IS BOOT DISK
				$REPL_QUERY = $this -> query_replica($REPL_UUID);
				$GET_BOOT_DISK = json_encode($REPL_QUERY['JOBS_JSON'] -> boot_disk);
				if ($PACK_URI == $GET_BOOT_DISK)
				{
					$IS_BOOT = true;
				}
				else
				{
					$IS_BOOT = false;
				}
				
				$REPL_DISK_DATA = array(
									"REPL_UUID" 	 	=> $REPL_UUID,
									"DISK_UUID" 	 	=> $DISK_UUID,									
									"HOST_UUID" 	 	=> $QueryResult['_HOST_UUID'],
									"PACK_URI"		 	=> $PACK_URI,
									"SNAPSHOT_MAPPING"	=> $SNAP_MAPS,
									"BACKUP_SIZE" 	 	=> $QueryResult['_DISK_SIZE'],							
									"SCSI_ADDR"			=> $QueryResult['_SCSI_ADDR'],
									"OPEN_DISK"			=> $QueryResult['_OPEN_DISK'],									
									"PURGE_DATA"		=> $QueryResult['_PURGE_DATA'],
									"CBT_INFO"			=> $QueryResult['_CBT_INFO'],
									"TIMESTAMP" 	 	=> $QueryResult['_TIMESTAMP'],
									"IS_BOOT"			=> $IS_BOOT,
									"STATUS" 	 		=> $QueryResult['_STATUS']
								);	
			}
			return $REPL_DISK_DATA;			
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#IS REPLICA DISK IN USE
	###########################
	public function is_replica_disk_in_use($SCSI_ADDR)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA_DISK WHERE _SCSI_ADDR = '".$SCSI_ADDR."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{
			return false;		
		}
		else
		{
			return true;
		}
	}
	
	###########################
	#GET SNAPSHOT MAPPING VALUE
	###########################
	public function get_snapshot_mapping($REPL_UUID,$DISK_UUID)
	{
		$GET_SNAPSHOT_VALUE = "SELECT * FROM _REPLICA_SNAP WHERE _REPL_UUID = '".$REPL_UUID."' AND _DISK_UUID = '".$DISK_UUID."' AND _STATUS = 'Y' ORDER BY _TIMESTAMP ASC LIMIT 1";
		$QUERY = $this -> DBCON -> prepare($GET_SNAPSHOT_VALUE);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
	
		if ($COUNT_ROWS == 1)
		{
			$QueryResult = $QUERY -> fetch(PDO::FETCH_ASSOC);		
			return $QueryResult['_SNAP_NAME'];
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# QUERY REPLICA DISK SNAPSHOT
	###########################
	public function query_replica_snapshot($REPL_UUID,$DISK_UUID=null)
	{
		if ($DISK_UUID == null)
		{
			$QUERY_REPL_SNAPSHOT = "SELECT * FROM _REPLICA_SNAP WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y'";
		}
		else
		{
			$QUERY_REPL_SNAPSHOT = "SELECT * FROM _REPLICA_SNAP WHERE _REPL_UUID = '".$REPL_UUID."' AND _DISK_UUID = '".$DISK_UUID."' AND _STATUS = 'Y'";
		}
		$QUERY = $this -> DBCON -> prepare($QUERY_REPL_SNAPSHOT);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$REPL_SNAP_DATA[] = array(
									"REPL_UUID" 		=> $QueryResult['_REPL_UUID'],
									"DISK_UUID" 		=> $QueryResult['_DISK_UUID'],
									"SNAP_UUID" 		=> $QueryResult['_SNAP_UUID'],
									"SNAP_NAME"			=> $QueryResult['_SNAP_NAME'],
									"ORIGINAL_SIZE"		=> $QueryResult['_ORIGINAL_SIZE'],
									"BACKUP_SIZE" 		=> $QueryResult['_BACKUP_SIZE'],
									"PROGRESS_SIZE"		=> $QueryResult['_PROGRESS_SIZE'],
									"OFFSET_SIZE"   	=> $QueryResult['_OFFSET_SIZE'],
									"LOADER_DATA"		=> $QueryResult['_LOADER_DATA'],
									"TRANSPORT_DATA"	=> $QueryResult['_TRANSPORT_DATA'],
									"LOADER_TRIG"		=> $QueryResult['_LOADER_TRIG'],
									"SNAP_TIME"			=> $QueryResult['_SNAP_TIME'],
									"SNAP_OPEN"			=> $QueryResult['_SNAP_OPEN'],
									"TIMESTAMP" 		=> $QueryResult['_TIMESTAMP'],
									"STATUS" 	 		=> $QueryResult['_STATUS']
								);	
			}
			return $REPL_SNAP_DATA;			
		}
		else
		{
			return false;
		}	
	}
	
	###########################
	# QUERY REPLICA DISK SNAPSHOT BY UUID (TO BE DELETE)
	###########################
	public function query_replica_snapshot_by_uuid($SNAP_UUID)
	{
		$QUERY_REPL_SNAPSHOT = "SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = '".$SNAP_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($QUERY_REPL_SNAPSHOT);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$REPL_SNAP_DATA[] = array(
										"REPL_UUID" 	=> $QueryResult['_REPL_UUID'],
										"DISK_UUID" 	=> $QueryResult['_DISK_UUID'],
										"SNAP_UUID" 	=> $QueryResult['_SNAP_UUID'],
										"SNAP_NAME"		=> $QueryResult['_SNAP_NAME'],
										"ORIGINAL_SIZE"	=> $QueryResult['_ORIGINAL_SIZE'],
										"BACKUP_SIZE" 	=> $QueryResult['_BACKUP_SIZE'],
										"PROGRESS_SIZE"	=> $QueryResult['_PROGRESS_SIZE'],
										"OFFSET_SIZE"   => $QueryResult['_OFFSET_SIZE'],
										"LOADER_DATA"	=> $QueryResult['_LOADER_DATA'],
										"LOADER_TRIG"	=> $QueryResult['_LOADER_TRIG'],
										"SNAP_TIME"		=> $QueryResult['_SNAP_TIME'],
										"SNAP_OPEN"		=> $QueryResult['_SNAP_OPEN'],
										"SNAP_OPTIONS"	=> json_decode($QueryResult['_SNAP_OPTIONS']),
										"TIMESTAMP" 	=> $QueryResult['_TIMESTAMP'],
										"STATUS" 	 	=> $QueryResult['_STATUS']
									);	
			}
			return $REPL_SNAP_DATA;			
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# QUERY REPLICA DISK SNAPSHOT WITH MACHINE ID
	###########################
	public function query_replica_snapshot_with_machine_id($MACHINE_ID,$SNAP_UUID)
	{
		$QUERY_REPL_SNAPSHOT = 'SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = "'.$SNAP_UUID.'" AND JSON_CONTAINS(_SNAP_OPTIONS, \'"'.$MACHINE_ID.'"\', \'$.machine_id\') AND _STATUS = "Y"';
		
		$QUERY = $this -> DBCON -> prepare($QUERY_REPL_SNAPSHOT);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
	
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$REPL_SNAP_DATA[] = array(
										"REPL_UUID" 	=> $QueryResult['_REPL_UUID'],
										"DISK_UUID" 	=> $QueryResult['_DISK_UUID'],
										"SNAP_UUID" 	=> $QueryResult['_SNAP_UUID'],
										"SNAP_NAME"		=> $QueryResult['_SNAP_NAME'],
										"ORIGINAL_SIZE"	=> $QueryResult['_ORIGINAL_SIZE'],
										"BACKUP_SIZE" 	=> $QueryResult['_BACKUP_SIZE'],
										"PROGRESS_SIZE"	=> $QueryResult['_PROGRESS_SIZE'],
										"OFFSET_SIZE"   => $QueryResult['_OFFSET_SIZE'],
										"LOADER_DATA"	=> $QueryResult['_LOADER_DATA'],
										"LOADER_TRIG"	=> $QueryResult['_LOADER_TRIG'],
										"SNAP_TIME"		=> $QueryResult['_SNAP_TIME'],
										"SNAP_OPEN"		=> $QueryResult['_SNAP_OPEN'],
										"SNAP_OPTIONS"	=> json_decode($QueryResult['_SNAP_OPTIONS']),
										"TIMESTAMP" 	=> $QueryResult['_TIMESTAMP'],
										"STATUS" 	 	=> $QueryResult['_STATUS']
									);	
			}
			return $REPL_SNAP_DATA;			
		}
		else
		{
			return false;
			//return $this -> query_replica_snapshot($REPL_UUID, $REPL_DISK_UUID);
		}
	}
			
	###########################
	# LIST JOB HISTORY
	###########################
	public function list_job_history($JOBS_UUID,$ITEM_LIMIT)
	{
		$GET_REPL_HIST = "(SELECT * FROM _JOBS_HISTORY WHERE _JOBS_UUID = '".$JOBS_UUID."' AND _IS_DISPLAY = 'Y' ORDER BY _ID DESC LIMIT ".$ITEM_LIMIT.")ORDER BY _SYNC_TIME DESC";

		$QUERY = $this -> DBCON -> prepare($GET_REPL_HIST);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$JOBS_MSG = json_decode($QueryResult['_HIST_JSON'],true);
				
				$JOB_TIME   	= strtotime($JOBS_MSG['time']);
				$JOB_SYNC_TIME 	= strtotime($JOBS_MSG['sync_time']);
				$JOB_STATUS 	= $JOBS_MSG['state'];
				$JOB_ERROR 		= $JOBS_MSG['error'];
				$JOB_TEXT		= $JOBS_MSG['description'];
				$JOB_FORMAT		= null;
				$JOB_ARGUMENTS	= null;
				
				if(isset($JOBS_MSG['format'])){$JOB_FORMAT = $JOBS_MSG['format'];}
				if(isset($JOBS_MSG['arguments'])){$JOB_ARGUMENTS = $JOBS_MSG['arguments'];}
								
				$REPL_HIST[] = array(
									'time' 			=> $JOB_SYNC_TIME,
									'unsync_time'  	=> $JOB_TIME,
									'state'			=> $JOB_STATUS,
									'error'			=> $JOB_ERROR,
									'description' 	=> $JOB_TEXT,
									'format'		=> $JOB_FORMAT,
									'arguments'		=> $JOB_ARGUMENTS	
							);
			}
			return $REPL_HIST;
		}
	}
	
	
	###########################
	# LIST SNAPSHOT JOB HISTORY
	###########################
	public function list_snapshot_job_history($JOBS_UUID)
	{
		$GET_REPL_HIST = "(SELECT * FROM _JOBS_HISTORY WHERE _JOBS_UUID = '".$JOBS_UUID."' ORDER BY _ID DESC LIMIT 20)ORDER BY _SYNC_TIME ASC";
		$QUERY = $this -> DBCON -> prepare($GET_REPL_HIST);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
	
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$JOBS_MSG = json_decode($QueryResult['_HIST_JSON'],true);
				
				$JOB_TIME   = strtotime($JOBS_MSG['time']);
				$JOB_STATUS = $JOBS_MSG['status'];
				$JOB_ERROR 	= $JOBS_MSG['error'];
				$JOB_TEXT	= $JOBS_MSG['description'];
				
				$REPL_HIST[] = array('time' 		=> strtotime($QueryResult['_SYNC_TIME']),
 									 'unsync_time'  => $JOB_TIME,
									 'state'		=> $JOB_STATUS,
									 'error'		=> $JOB_ERROR,
									 'description' 	=> $JOB_TEXT);
				
			}
			return $REPL_HIST;
		}
	}
	
	###########################
	# QUERY REPLICA BY HOST
	###########################
	public function query_replica_by_host($HOST_UUID)
	{
		$QUERY_REPL_PACKER = "SELECT * FROM _REPLICA WHERE _PACK_UUID = '".$HOST_UUID."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($QUERY_REPL_PACKER);
		
		$QUERY -> execute();
		
		return $QUERY -> fetchAll(PDO::FETCH_ASSOC);		
	}
	
	###########################
	#QUERY REPLICA BY LIKE TARGET CONNECTION UUID
	###########################
	public function query_replica_by_target_connection_id($TARGET_CONN_UUID)
	{
		$TARGET_STRING = '"TARGET":"'.$TARGET_CONN_UUID.'"';
		$GET_EXEC = "SELECT * FROM _REPLICA WHERE _CONN_UUID LIKE '%".$TARGET_STRING."%' AND _STATUS = 'Y' LIMIT 1";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			$QueryResult = $QUERY -> fetchAll(PDO::FETCH_ASSOC)[0];
			
			$REPL_DATA = array(
							"ACCT_UUID" 	 => $QueryResult['_ACCT_UUID'],
							"REGN_UUID" 	 => $QueryResult['_REGN_UUID'],
							"REPL_UUID" 	 => $QueryResult['_REPL_UUID'],
							"CLUSTER_UUID"   => $QueryResult['_CLUSTER_UUID'],

							"MGMT_ADDR" 	 => $QueryResult['_MGMT_ADDR'],
							"PACK_UUID" 	 => $QueryResult['_PACK_UUID'],							
							"CONN_UUID" 	 => $QueryResult['_CONN_UUID'],
							"REPL_TYPE"		 => $QueryResult['_REPL_TYPE'],					
								
							"JOBS_JSON" 	 => $this -> upgrade_replica_info(json_decode($QueryResult['_JOBS_JSON'],false)),
							"WINPE_JOB"		 => $QueryResult['_WINPE_JOB'],
							"REPL_HIST_JSON" => $QueryResult['_REPL_HIST_JSON']
						);	
			return $REPL_DATA;
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	# QUERY REPLICA REPORT
	###########################
	public function query_replica_report($ACCT_UUID,$INTERVAL_HOURS)
	{
		$LIST_REPLICA = "
						SELECT 
							_REPLICA._CLUSTER_UUID AS CLUSTER_UUID,
							_REPLICA._JOBS_JSON AS JOBS_JSON,
							_REPLICA._REPL_CREATE_TIME AS REPLICA_CREATE_TIME,
							_REPLICA._REPL_DELETE_TIME AS REPLICA_DELETE_TIME,
							_REPLICA._STATUS AS REPLICA_STATUS,
							Count(_REPLICA_SNAP._ID) AS SNAP_COUNT,
							_SERVER_HOST._HOST_NAME AS PACKER_NAME,
							JSON_UNQUOTE(JSON_EXTRACT(_REPLICA._CONN_UUID, '$.TARGET')) AS REPL_CONN
						FROM 
							_REPLICA
						LEFT JOIN 
							_REPLICA_SNAP ON _REPLICA._REPL_UUID = _REPLICA_SNAP._REPL_UUID
						LEFT JOIN
							_SERVER_HOST ON _REPLICA._PACK_UUID = _SERVER_HOST._HOST_UUID
						
						WHERE
							_REPLICA._ACCT_UUID = '".$ACCT_UUID."' AND (_REPLICA._REPL_CREATE_TIME >= NOW() - INTERVAL ".$INTERVAL_HOURS." HOUR OR _REPLICA._STATUS != 'X')
						GROUP BY 
							_REPLICA._REPL_UUID
						ORDER BY 
							_REPLICA._ID";
	
		$QUERY_REPLICA = $this -> DBCON -> prepare($LIST_REPLICA);
		$QUERY_REPLICA -> execute();
		
		if ($COUNT_ROWS = $QUERY_REPLICA -> rowCount() != 0)
		{		
			$QueryResult = $QUERY_REPLICA -> fetchAll(PDO::FETCH_ASSOC);
			
			$ServerMgmt = new Server_Class();
			
			for ($i=0; $i<count($QueryResult); $i++)
			{
				$CONN_INFO = $ServerMgmt -> query_connection_info($QueryResult[$i]['REPL_CONN']);				
				$QueryResult[$i]['SOURCE_SERVER'] 		= $CONN_INFO['SCHD_HOST'];
				$QueryResult[$i]['SOURCE_SERVER_ADDR'] 	= json_encode($CONN_INFO['SCHD_ADDR']);
				$QueryResult[$i]['TARGET_SERVER'] 		= $CONN_INFO['LAUN_HOST'];
				$QueryResult[$i]['TARGET_SERVER_ADDR'] 	= json_encode($CONN_INFO['LAUN_ADDR']);
				$QueryResult[$i]['CLOUD_TYPE'] 			= $CONN_INFO['CLOUD_TYPE'];			
			}
			return $QueryResult;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# QUERY SERVICE REPORT
	###########################
	public function query_service_report($ACCT_UUID,$INTERVAL_HOURS)
	{
		$LIST_SERVICE = "
						SELECT 
							_SERVICE._CONN_UUID AS CONN_UUID,
							_SERVER_HOST._HOST_NAME AS PACKER_NAME,							
							_SERVICE._OS_TYPE AS OS_TYPE,
							SUM(_SERVICE_DISK._DISK_SIZE) AS DISK_SIZE,
							JSON_UNQUOTE(JSON_EXTRACT(_SERVICE._JOBS_JSON, '$.recovery_type')) AS SERVICE_TYPE,
							JSON_UNQUOTE(JSON_EXTRACT(_SERVICE._JOBS_JSON, '$.job_status')) AS JOB_STATUS,
							_SERVICE._TIMESTAMP AS SERVICE_TIME,
							_SERVICE._STATUS AS STATUS
						FROM 
							_SERVICE						
						LEFT JOIN
							_SERVER_HOST ON _SERVICE._PACK_UUID = _SERVER_HOST._HOST_UUID
						LEFT JOIN
							_SERVICE_DISK ON _SERVICE._SERV_UUID = _SERVICE_DISK._SERV_UUID	
						WHERE
							_SERVICE._ACCT_UUID = '".$ACCT_UUID."' AND (_SERVICE._TIMESTAMP >= NOW() - INTERVAL ".$INTERVAL_HOURS." HOUR OR _SERVICE._STATUS != 'X')
						GROUP BY 
							_SERVICE._SERV_UUID
						ORDER BY 
							_SERVICE._ID";
							
		$QUERY_SERVICE = $this -> DBCON -> prepare($LIST_SERVICE);
		$QUERY_SERVICE -> execute();
		if ($COUNT_ROWS = $QUERY_SERVICE -> rowCount() != 0)
		{		
			$QueryResult = $QUERY_SERVICE -> fetchAll(PDO::FETCH_ASSOC);
			
			$ServerMgmt = new Server_Class();
			
			for ($i=0; $i<count($QueryResult); $i++)
			{
				$CONN_INFO = $ServerMgmt -> query_connection_info($QueryResult[$i]['CONN_UUID']);				
				$QueryResult[$i]['CLOUD_TYPE'] = $CONN_INFO['CLOUD_TYPE'];			
			}
			return $QueryResult;
		}
		else
		{
			return false;
		}		
	}
	
	###########################
	# SNAPSHOT TRANSPORT DATA
	###########################
	public function snapshot_transport_data($REPL_UUID)
	{
		$TOTAL_TRANSPORT_DATA = 0;
	
		$TOTAL_SNAPSHOT_DATA = "SELECT SUM(_TRANSPORT_DATA) AS TOTAL_TRANSPORT_DATA FROM _REPLICA_SNAP WHERE _REPL_UUID = '".$REPL_UUID."'";
		$TOTAL_DATA = $this -> DBCON -> prepare($TOTAL_SNAPSHOT_DATA);
		$TOTAL_DATA -> execute();
		$TOTAL_TRANSPORT_DATA = $TOTAL_DATA -> fetch(PDO::FETCH_ASSOC)['TOTAL_TRANSPORT_DATA'];
		
		$SNAPSHOT_SIZE = "SELECT
							_ID,
							_REPL_UUID,
							_SNAP_UUID,
							SUM(_ORIGINAL_SIZE) AS _ORIGINAL_SIZE,
							SUM(_BACKUP_SIZE ) AS _BACKUP_SIZE,
							SUM(_PROGRESS_SIZE) AS _PROGRESS_SIZE,
							SUM(_OFFSET_SIZE) AS _OFFSET_SIZE,
							SUM(_LOADER_DATA) AS _LOADER_DATA,
							_STATUS 
						FROM 
							_REPLICA_SNAP 
						WHERE 
							_REPL_UUID = '".$REPL_UUID."' 
						GROUP BY 
							_SNAP_UUID
						ORDER BY
							_ID DESC LIMIT 1";
		
		$SNAPSHOT_SIZE_DATA = $this -> DBCON -> prepare($SNAPSHOT_SIZE);
		$SNAPSHOT_SIZE_DATA -> execute();
		
		if ($SNAPSHOT_SIZE_DATA -> rowCount() == 1)
		{		
			$FetchData = $SNAPSHOT_SIZE_DATA -> fetchAll(PDO::FETCH_ASSOC)[0];
			for ($i=0; $i<count($FetchData); $i++)
			{
				$QueryResult = array(
								'total_transport_data' => $TOTAL_TRANSPORT_DATA,
								'backup_size' => $FetchData['_BACKUP_SIZE'],
								'progress_size' => $FetchData['_PROGRESS_SIZE'],
								'offset_size' => $FetchData['_OFFSET_SIZE'],
								'loader_data' => $FetchData['_LOADER_DATA']
							);
			}
		}
		else
		{
			$QueryResult = array(
								'total_transport_data' => $TOTAL_TRANSPORT_DATA,
								'backup_size' => 0,
								'progress_size' => 0,
								'offset_size' => 0,
								'loader_data' => 0
							);
		}
		
		return $QueryResult;
	}
	
	###########################
	# CASCADING CONNECTION
	###########################
	public function cascading_connection($PACKER_UUID)
	{
		$GET_EXEC = "SELECT * FROM _REPLICA WHERE _PACK_UUID = '".strtoupper($PACKER_UUID)."' AND _STATUS = 'Y'";
	
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS >= 1)
		{
			$ServerMgmt  = new Server_Class();
			
			foreach($QUERY as $QueryResult)
			{
				$SOURCE_MACHINE_UUID = json_decode($QueryResult['_JOBS_JSON']) -> source_transport_uuid;
				$TARGET_MACHINE_UUID = json_decode($QueryResult['_JOBS_JSON']) -> target_transport_uuid;
				$JOB_ORDER = json_decode($QueryResult['_JOBS_JSON']) -> job_order;
				
				$SOURCE_INFO = $ServerMgmt -> list_match_service_id($SOURCE_MACHINE_UUID);
				$TARGET_INFO = $ServerMgmt -> list_match_service_id($TARGET_MACHINE_UUID);
				
				$CASCADING_TYPE[] = json_decode($QueryResult['_JOBS_JSON']) -> multi_layer_protection;
				
				$SNAPSHOT_COUNT = ($this -> query_replica_snapshot($QueryResult['_REPL_UUID']) != false)?count($this -> query_replica_snapshot($QueryResult['_REPL_UUID'])):0;
				
				$REPL_QUERY = $this -> query_replica($QueryResult['_REPL_UUID']);
				
				$MACHINE_ID[$QueryResult['_REPL_UUID']] = array(
																'source_machine_id'  => $SOURCE_MACHINE_UUID, 
																'target_machine_id'  => $TARGET_MACHINE_UUID,
															
																'carrier_uuid' 		 => ($SOURCE_INFO != false)?$SOURCE_INFO['ServiceId']['Carrier']:false, 
																'loader_uuid' 		 => ($TARGET_INFO != false)?$TARGET_INFO['ServiceId']['Loader']:false,
																
																'souce_conn_uuid'	 => ($SOURCE_INFO != false)?json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE:false,
																'target_conn_uuid'	 => ($TARGET_INFO != false)?json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET:false,
																
																'souce_direct'		 => ($SOURCE_INFO != false)?$SOURCE_INFO['IsDirect']:false,
																'target_direct'		 => ($TARGET_INFO != false)?$TARGET_INFO['IsDirect']:false,
																
																'snapshot_count'	 => $SNAPSHOT_COUNT,
																'job_order' 		 => $JOB_ORDER
															);
				$REPL_UUID[] = $QueryResult['_REPL_UUID'];
				
				$CASCADING_DATA = array('transport' => $MACHINE_ID, 'replica' => $REPL_UUID[0], 'cascading_type' => end($CASCADING_TYPE), 'count' => $COUNT_ROWS);
			}
		
			#CHECK IS SOURCE TRANSPORT DIRECT
			foreach ($CASCADING_DATA['transport'] as $JOB_UUID => $JOB_DATA)
			{
				if ($JOB_DATA['souce_direct'] == false)
				{
					$IS_SOUCE_HTTPS = false; 
					break;
				}
				else
				{
					$IS_SOUCE_HTTPS = true;
				}
			}		
		
			#CHECK IS CASCADING READY
			if ($COUNT_ROWS > 1)
			{
				foreach ($CASCADING_DATA['transport'] as $JOB_UUID => $JOB_DATA)
				{
					if ($JOB_DATA['snapshot_count'] == 0)
					{
						$IS_CASCADING_READY = false; 
						break;
					}
					else
					{
						$IS_CASCADING_READY = true;
					}
				}
			}
			else
			{
				$IS_CASCADING_READY = false;
			}
			
			#ADD PACKER INFORMATION
			//$HOST_INFO = $ServerMgmt -> query_host_info($PACKER_UUID);
			$CASCADING_DATA['packer_uuid'] = $PACKER_UUID;
			$CASCADING_DATA['is_souce_https'] = $IS_SOUCE_HTTPS;
			$CASCADING_DATA['is_cascading_ready'] = $IS_CASCADING_READY;
			
			return $CASCADING_DATA;
		}
		else
		{
			return array('transport' => false, 'replica' => false, 'cascading_type' => 'NoSeries', 'count' => 0, 'packer' => false, 'is_cascading_ready' => false);
		}
	}
		
	###########################
	# NETWORK TOPOLOGY FOR VIS.JS
	###########################
	public function network_topology($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'Replication')
		{
			$PACK_UUID = $this -> query_replica($JOB_UUID)['PACK_UUID'];
		}
		else
		{
			$ServiceMgmt = new Service_Class();
			$SERV_INFO = $ServiceMgmt -> query_service($JOB_UUID);
			$PACK_UUID = $SERV_INFO['PACK_UUID'];
		}
		
		$LINK_INFO = $this -> cascading_connection($PACK_UUID);

		if ($LINK_INFO['transport'] != false)
		{
			$ServerMgmt  = new Server_Class();
			
			#BASE64 PNGS
			$SOURCE_HOST_PNG = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAJEAAAB9CAYAAACiXoHaAAAABmJLR0QAAAAAAAD5Q7t/AAAACXBIWXMAAC4jAAAuIwF4pT92AAAAB3RJTUUH4wQMCToLXvjmigAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAANASURBVHja7dtPSBRhGIDxx8kKqkN1aIvytBGRpFlEhIJ0CDKPXoK6tYeshCQipHMhdhECKzIIigqCqEuYlxAkCiQSooKwS1jZrWNm2sEVlmVX98/lm9nnByK7OysfLw/jzsxOXWpkkCIiYBDoBupRrZoGLgGPlgulkD7gH9BjQDVvO/AQmAGaSo3oJXDN2SnPFmASOJX/Qv5e5g1wyHlpGfezvx8U2hPdMSCVEVJzfkTNQMbZqAyj+RE9diaq4DPSyaWIGoDdzkQVuL4U0XFnoQptA9bVAx0lvqFnqi1zw7nVhvT48BjQXsKmTRGwp4QNXxlQbZlqy7SXuOnWCEiXsOFzx6oiUhHFL33kmnVWKiKKnIGqrsgRyIhkRDIiyYhkRDIiGZFkRDIiGZGMSDIiGZGMSEYkGZGMSEYkI5KMSEYkI5IRSUYkI5IRyYgkI5IRyYhkRJIRyYhkRDIiyYhkRDIiGZFkRDIiGZGMSDIiGZGMSEYkGZGMSEYkI5KMSEYkI5IRSUYkI5IRyYgAVjkqVRvRMUelaiPqTI8Pn3BctSM9Pvys1G3rUiODC2X87Xlg2hEn2hogVcb2Z+sr2HM1OGd5dCYjkhHJiCQjkhHJiGREkhHJiGREMiLJiGREMiIl0UIE/HIOqsLvCPjkHFSF7xHw1jmoCt8i4IlzUIXmgK8RMJF9IJXrZu7R2XnnoQpczI3oNjDjTFSGPuBvbkQA+1i8r0xayQugf+lBbkQ/gf2GpBW8Bjpzn8g/Yz0J7MATkCpsCGjNf7LQZY8fLN5G2+1Rm7LeA43AuUIvLnft7BawGjgIDABj7qFqwhzwGXgKZLL/mVqAj8XeUMq9+BPZnzhZCHRde4EPSasuiVfx1wa8to1J3HUlMaKdAa+t0YjioTXgtXUZUTycCXhtR40oHloCX99hIwpbdwzWeNWIwjYQgzUeATYZUZjagQ0xWWuvEYXpbozWesWIwvywmo7Z3HuNKCz3/PxmRNU4AOyK4brrgdNGFIbLHk0aUbU6Yrz2zUYUhncxXvu8EYUhzmeA+40oDKMsfgsvbr6QkPNFSTnE7wIuEJ/vhA/F9IiyoLrUyCAJEwHrA13bLPAnaQP/D1fPY75N5/c7AAAAAElFTkSuQmCC';
			$TARGET_HOST_PNG = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAJEAAAB/CAYAAADvliDRAAAABmJLR0QAAAAAAAD5Q7t/AAAACXBIWXMAAC4jAAAuIwF4pT92AAAAB3RJTUUH4wQMCTcsTlwtrAAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAAUmSURBVHja7d1taFV1HMDx7z1OjFzSC7MhyFiNIgIHFcTuRH2l0n1jgcYsjR4kU5KSsAzMHiBDejDU2RMRZZlW5Julrjc5chakMN8EoY2xF87yRdg1CnPrxT3GVe62e+7OuOfcfT+wN2e7547f/8u9595ztpthXxcVmAssAFqBO4EGYAZKm/PAIHAcOAYcAU5G3UldhJ+9CXgRWOnsa8aM8OsWoL1o+yfAFqCvnJ0EZfzMtcBh4LQBTRorgV+Bg8C08Ua0HLgALHKuk9IS4G/gvkoj2gXsc44CvgLejnpMtB9Y5uxUZD0wE3ignEeiXQakEawA3horouXAWmelUTwFLB0pomkeA6lMXwNTS0V0wNkoYkhXRNQUvpyTypUDGosj2uJMVIEXADLhubPhCnYwBKw7f0/rg0Em0+Y802loePjojG+OfQrspLwzGFfLBBROpkZ1tmPOrKZ8LrvbgNItyGTa8rlsR8ecWU3A2Qp2cXsAzI8a77ypU1pWzW3udwlqx6q5zf3zpk5pCZ9hopgfANmIN9pwaNHdg4699oTruiHizbIBheuBynZqfsvPjrt2VbC+dwUULigrW8N10w876tpVwfo21DEBVyTWd/bMAZ6kcA6uyaWpuj4KJ9V35nPZgZj3fX0wAQFtAgaAjQaUGE3As8BAfWfPc7G/wos5oI3AVtcs0V4L1yl5EdV39jQA21yjVNi2pOvHhsRFxJUXeivhvr94qT2JES10aVJlQVw7qovxl7qh1MZ8LutyVVl9Z0+pzTMTeWCtVAmMSLVXo4xIMiIZkYxIRiQZkYxIRiQjkoxIRiQjkhFJRiQjkhFJRiQjkhHJiCQjkhHJiGREkhHJiGREMiLJiGREMiIZkWREMiIZkYxIMiIZkYxIRiQZkYxIRiQjkoxIBUNx7SjOD837vdTGET6wTdV3LomPREdcl1Q5ksSIPnNdUmVv4iLK57KDwPOuTSpsCtcreQfW+Vz2VWCza5Rom/O57NZEvzrL57KvAI3AG0C/a5YI/cDrQGO4PrGqm4jfOJ/LGk+yNALPhF+x830iGZGMSEYkGZFiiui8Y9A4/BEAkd65HPzzwmLnVrsqWN/BADge5RbN3b23Oura1dzde1vEm/wUAFGv1djee+Zci+OuPR+fPNUIvBnxZj0B0B31OKrtxC+HfFqrLb1nzrWsHfjthwpebHVn2NcFMFzB/f4LrMvnsu+6BOlW39mzBthJZafBMpcj+gh4yHEqog+BRy8/dL3kPFSBlyl6/usDDjkTRdBJeKlP8UHUUueiCO79/5VW0cZ/gPudjcoM6GKpiAD2Ax3OSKPYDhwo3lDqPYF1wBfOSiXsBZ6+euNIbywtB3z/R8V2ACtKfWO0dyfXAO3OTsAyYP1I3xzrLe7PgenAt85xUuoCrgG+HO2HyjlP8hewCGgG9jjXSWFPuN6Lw1fto7p82iOqFmAh0AbcAdwI1Dv71MkDZ4ETwFHgO6A36k4qjSgNHgPeL7G9NZ/LHpuoO63v7GkFSu1/NfBBLQ7aa6xlRDIiGZFkRDIiGZGMSDIiGZGMSDUiKefOGihcUdkO3OyyjOk0hasMdxHxH3LUakRP4HXd453fO5P56ewRAxq33eEcJ+UjUQBcsoHYTCHGTw5KyyPRw657bcyzmhEtdN1jtcCX+Eqtakbk56PFq7tad+yBtQfWqX4kGgIed+1jsbpaASXhmOg9RvnLSpVlPVX+K5IkHFjvAGYDWyn8sy2NrS+c1+xwflX1H7gf9YPKMpCKAAAAAElFTkSuQmCC';
			$SOURCE_SERVER_PNG = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAH0AAABiCAYAAAB9ANxrAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAC4jAAAuIwF4pT92AAAAB3RJTUUH4wQMAjYYeqYHuQAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAARISURBVHja7Z3ba1xVFId/+5wzE6eW1BalmaZJo89eq1RRS8VaiIJ3fFAQUSIiVAu2IvigifHSB9MKWq2IUAtp0dLghcZCalAJgqUo/Q9EmzRRSTPJaDBn5hwfRhED59Y4MTvzfW8hh0xY315rr31mzhozd0UxFDQUDiFAOiAdkA5IBzvxCIGFuK6CLdsUtrbLnJuU+eZLmclfkb4saV6lyqtvKdh4vWTMv35lxkflde+SOX0q8c+YxHO64yi8ZC0BjwpgeUb6rVz31wnbOuR/8KlUKMRcFMrr65FztH9h0sO2DvkfDWE3KidGhuU9+0Tdy/nc8ZPSyuYUqyNU7smHYjOeRs4CKs+8kE64JBmjSvfrdO+2E2y9I9tW0NKqcM3FSLe9gcvcA9x4C9Lt7hZNdumr1yDd7vpezb5ORn9EutWJPn42Y5qHcr4aQrrNuO+/me0Y+d23UjW6OiTfkZv7Y1FuPlibhWfP1P9ewOCAzL0PKrz86uSLZ2flPb89/n/mkzOW4Djy3/0wXnx5WrnH7pf56QfO6cujmQuUe/wBeb3PyYyd+ae5C0OpNCVn4JDynZsShZPpjVo0CAHSAemAdEA62IlX7XqaKDQYplwuc2SjvAPSAemAdEA6IB2QDksDL+tHccB+eD+d8g5IB6QD0gHpYM2RjRDYRdB5t6pdOxS2rJNcVwoCabokd+gzuW+8UvuZI9tyqcmO/HcOKbzy2uhrZkrKPXpf7MOLqaSHa4vyDx8n6BG4xwbk9vXU/XX8/YcVXnVd8oWzvyt/502xj6Ill/d8k1RYgd2opGhZtyglPZVwSSqsUGX3PnlPPUIjZzPVrh3ZFsnGG2r7PdItribFjNXEcRRs3op0u5s4N/tCaetAut2pnv2AZUpTSLea6VJ26SPDSLe6ug9/nk34xFjsgGCkW4DX1yOVZ1JvBV73rvhFkWogcGs7kY/cO8+dV/nNvK1vuEz+gY+lC+IHArt7X5Z75OACpcPS4aLV8ne/XbsVO3/098/j8rp3ynx/MnmhIt1C8k0Kbu2sDfmfmpT5+oTMLxPpqxPSG7AxJARIB6QD0gHpYOm9BcaPkOmAdEA6IB2QDkgHpAPS4f/Dcw/uJwoNBu+nU94B6YB0QDogHaw5shECuwhuv+evmTPF2tOsf8+cOXFM7t5eZs4sr5r8333BbrqZM/2DBD0Cd3BA7p7eur+O/96R1F+lnb/rZqk8vYDynm+SLlyJ3aikKK5flJKeSrgkFQq1mTPbH6aRs5nMM2eu2cTMGeurSUsx8/4fbNmGdLubuPOYORPzeDnSrUh1Zs40HsycacDq/kW2I7MZH2XmjO14e16KPXfP3wqYOVNnFm3mTPul8g98IhXiZ854fT1yjvYvUDosHZpXqfLavto5fP7MmYkxeS/ulDl9KnmhIt1CcjkFm29TuH5DbebMyHDsHo50oJFDOiAdkA5IB1v5E38hXnD9rztgAAAAAElFTkSuQmCC';
			$TARGET_SERVER_PNG = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAH0AAABiCAYAAAB9ANxrAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAC4jAAAuIwF4pT92AAAAB3RJTUUH4wQMAjUwZD78gAAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAASdSURBVHja7Z1daFtlGMf/70mWNClpm310tE1W2o12VDtBGLTQD1FQccWBK8yLdirtlHmhN11niZPdiBCr4hxtx4aCLgXFqZVKP1Zh0mkpZSjRTrRK2do5FrqmJ2mSdm49XgiCGznJSU3tu/x/t3lyDjw/zpP3zUn+Ryi1HRpIRqGwBZROKJ1QOqF0IidmtkBCaSYFDdXlKCnIw5waxdDEbwgEI5R+L+J02PCRZx9qdxVDiH+/NhNQ0ertw9jkTMLjiET7dEUIFG52sONxUCPLCEeX036e7UUb8e2JVtizNsSt0TSgrWsQp/ovru5KLyl04vvTh2g3DgPjU9h/7JO0j/Pz7z6nKxwAhAA6X3wcP00HdK94LuQkwHvoUeRmZyVVKwRwun0vV++y81RthaF6d34u8p3ZlC77As4oj+3eQekyc+dKPRk259opXWZurxi/ETp9bYHSZWY2oBqq1zSgf+wXSpeZN3yjhupH/Zdx6/ZK6vv05Zu31uTLB1m5cn0h7efoHfGjZc+D2L2zKGFtdOlPNL9+Vn+NwF/OyIEiBM69/YyueDWyhIde/gC/X53XPZZJFNceY0vXPxqAD4d+wOXrKipLt8Jht0IRApoGBMMx+Eb8aDjiww01mng3wCs9A6cGW0DphNIJpRNKJ3Ji7miqYxcyDLG4uMgtG8c7oXRC6YTSCaUTSieUTtYHZqM/xSHyw/vpHO+E0gmlE0onlE6k2bKxBXKx/+H74Wmuh2tLDswmBSuahmA4hk/PT+JIzzmsaIk3Y9yyyTKShcBg5wFUVbji1iwsLqH+pfcxfS24OumuLTmYOPkCux4H34gfbV1DaT/PUOcBVN/nTlgXid1EWdNx3b+iJRzvVosZ2TYL7cbBnZ+7JiM9GeEAkG2zwHe0EU92+LiQkxlPc72h+roHimE2KZSeSdNEEQJPVJVRusyYFOP5I9sLnZQuM1oK+6v5cIzSZSaoIzAeA+NTlC4zn4/+bKh+NhDSDQimdAk43D2EUCS5CBhNAw6+2adbk1QgcInOoiDTmQ/FUhq/Rilzb8Loey2wWfUDgdt7hnHyy4nVSSfrh005dvS+1oiqCvddgYJX50I46O3DhR+vJDwOpUtIlsWMvTU7UVrgxJwaxVdjv+KPG+Gk30/pGQgXcpROKJ1QOqF0IimMH+GVTiidUDqhdELphNIJpRNKJ/8j5rc+/o5dyDB4P53jnVA6oXRC6YTSiTRbNrZALp5+pBKepjq48nNhUsQ/mTNnv7mE9u5hZs7cUyP5P3zAblKZM+M9z7Prcegd8eNw93Daz/P1O88m/Sjt8qbjUCNLqY93q8UMh91Ku3HYtjVvTUZ6MsIBwJ61Ab6jjWh45QwXcjLjMfjsvJpd25g5IzuuFDJnGqrLKV1mUsmcKSnIo3SZYeZMBsLMmQzks9FLhupnAiozZ2SnvXtYd99950dBq5eZM2llrTJndhRtxIUTrbBn6WfOtHUN4lT/xdVJJ+sHp8OGM6/uQ01l8V2ZM7OBEFq8X2BscibhcShdQixmE/ZUl6G08O/MmYHxKd3PcEonXMhROqF0QumE0oms/AVGBXEiZmXuywAAAABJRU5ErkJggg==';
			
			#DEFINE AXIS
			$X_AXIS = 0;
			$Y_AXIS = 0;
			
			#GET SOURCE HOST INFORMATION
			$HOST_INFO = $ServerMgmt -> query_host_info($PACK_UUID);						
			$HOST_LABEL = $HOST_INFO['HOST_NAME'].'\n'.$HOST_INFO['HOST_ADDR'].'\n'.ucfirst(strtolower($HOST_INFO['OS_TYPE'])).' ('.$HOST_INFO['HOST_TYPE'].')';			
			$NODES[$PACK_UUID] = array('id' => $PACK_UUID, 'label' => $HOST_LABEL, 'type' => 'host', 'shape' => 'image', 'image' => $SOURCE_HOST_PNG, 'size' => 40, 'is_direct' => $HOST_INFO['IS_DIRECT'], 'x' => 0, 'y' => 0);
			
			#GET RECOVERY HOST INFORMATION
			if ($TYPE == 'Recovery')
			{
				foreach ($LINK_INFO['transport'] as $REPLICA_ID => $LINK_DATA)
				{		
					$RECOVERY_HOST[$LINK_DATA['target_machine_id']] = $LINK_DATA['target_conn_uuid'];					
				}
				
				$CLOUD_NAME = ($SERV_INFO['CLOUD_NAME'] != "UnknownCloudName")?$SERV_INFO['CLOUD_NAME']:null;				
				$CONN_UUID = $SERV_INFO['CONN_UUID'];				
				$OS_TYPE = ($SERV_INFO['OS_TYPE'] == "MS")?"Windows":"Linux";
				$HOST_LABEL = $SERV_INFO['HOST_NAME'].'\n'.$OS_TYPE.'\n'.$CLOUD_NAME;
				$NODES[$JOB_UUID] = array('id' => $JOB_UUID,'label' => $HOST_LABEL, 'type' => 'recovery_host', 'shape' => 'image', 'image' => $TARGET_HOST_PNG, 'size' => 40, 'is_direct' => $HOST_INFO['IS_DIRECT']);
			}
			
			#DEFINE NAME CHANGE
			$DEFINE_NAME = ["On-Premises","Local Recover Kit"];
			$CHANGE_NAME = ["General Purpose","Recover Kit"];
			
			switch ($LINK_INFO['cascading_type'])
			{
				case 'NoSeries':
				case 'parallel':
					#NODES INFORMATION
					foreach ($LINK_INFO['transport'] as $REPLICA_ID => $LINK_DATA)					
					{	
						$SOURCE_INFO = $ServerMgmt -> list_match_service_id($LINK_DATA['source_machine_id']);						
						$SOURCE_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $SOURCE_INFO['CloudType']);

						$SOURCE_LABEL = $SOURCE_INFO['Hostname'].'\n'.implode(",",json_decode($SOURCE_INFO['Address'])).'\n'.$SOURCE_TYPE;
						$SOURCE_PNG = ($SOURCE_INFO['CloudType'] == 'On-Premises')? $SOURCE_SERVER_PNG:$TARGET_SERVER_PNG;
						$NODES[$LINK_DATA['source_machine_id']] = array('id' => $LINK_DATA['source_machine_id'], 'label' => $SOURCE_LABEL, 'type' => 'source', 'shape' => 'image', 'image' => $SOURCE_PNG, 'size' => 40, 'is_direct' => $SOURCE_INFO['IsDirect'], 'x' => 220, 'y' => 0);
					
						$Y_AXIS = ($LINK_INFO['cascading_type'] == 'parallel')? $Y_AXIS - 150:$Y_AXIS;
						$TARGET_INFO = $ServerMgmt -> list_match_service_id($LINK_DATA['target_machine_id']);
						$TARGET_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $TARGET_INFO['CloudType']);
						
						$TARGET_LABEL = $TARGET_INFO['Hostname'].'\n'.implode(",",json_decode($TARGET_INFO['Address'])).'\n'.$TARGET_TYPE;
						$NODES[$LINK_DATA['target_machine_id']] = array('id' => $LINK_DATA['target_machine_id'], 'label' => $TARGET_LABEL, 'type' => 'target', 'shape' => 'image', 'image' => $TARGET_SERVER_PNG, 'size' => 40, 'is_direct' => $TARGET_INFO['IsDirect'], 'x' => 430, 'y' => $Y_AXIS);						
						$Y_AXIS = ($LINK_INFO['cascading_type'] == 'parallel')? $Y_AXIS + 450:$Y_AXIS + 300;
					}
					
					#ADD RECOVERY POSITION
					if ($TYPE == 'Recovery')
					{
						foreach ($RECOVERY_HOST as $TARGET_CLOUD_UUID => $TARGET_CONNECTION_UUID)
						{
							if ($TARGET_CONNECTION_UUID == $CONN_UUID)
							{
								$NODES[$JOB_UUID]['x'] = 650;
								$NODES[$JOB_UUID]['y'] = $NODES[$TARGET_CLOUD_UUID]['y'];
							}
						}
					}
					
					#REMOVE DUPLICATE ITEMS
					$NODES = array_values($NODES);					
					
					#OVERWRITE TARGET AS SOURCE FOR ONLY HOST TO TRANSPORT
					if (count($NODES) == 2){$NODES[1]['type'] = 'source';}				
				
					#EDGES DIRECTION
					for($i=0; $i<count($NODES); $i++)
					{
						if ($NODES[$i]['type'] == 'source')
						{
							$EDGES[] = array("from" => $PACK_UUID, "to" => $LINK_DATA['source_machine_id'], "arrows" => "to", "length" => 200);
						}
						if ($NODES[$i]['type'] == 'target')
						{
							$TARGET_LINK_ID = ($LINK_DATA['source_machine_id'] == $NODES[$i]['id'])?$PACK_UUID:$LINK_DATA['source_machine_id']; # FOR ONE LAYER HOST -> TARGET
							$EDGES[] = array("from" => $TARGET_LINK_ID, "to" => $NODES[$i]['id'], "arrows" => ($NODES[$i]['id'] == TRUE)?"to":"from", "length" => 200);							
						}						
					}
					
					
					#RECOVERY HOST
					if ($TYPE == 'Recovery')
					{
						foreach ($RECOVERY_HOST as $TARGET_CLOUD_UUID => $TARGET_CONNECTION_UUID)
						{
							if ($TARGET_CONNECTION_UUID == $CONN_UUID)
							{
								$EDGES[] = array("from" => $TARGET_CLOUD_UUID, "to" => $JOB_UUID, "arrows" => "to", "length" => 200);
							}
						}
					}
					
					return array('node' => stripslashes(json_encode($NODES)), 'edge' => json_encode($EDGES));
				break;
				
				case 'cascaded':				
					#NODES INFORMATION
					foreach ($LINK_INFO['transport'] as $REPLICA_ID => $LINK_DATA)
					{		
						$MACHINE_UUID[] = $LINK_DATA['source_machine_id'];
						$MACHINE_UUID[] = $LINK_DATA['target_machine_id'];
					}
					$MACHINE_UUID = array_values(array_unique($MACHINE_UUID));
				
					for ($i=0; $i<count($MACHINE_UUID); $i++)
					{
						$X_AXIS = $X_AXIS + 200;
						$PROTECT_TYPE = ($i == 0)? "source" : "target";
						$LINK_FROM = ($i == 0)? $MACHINE_UUID[$i]: $MACHINE_UUID[$i-1];
							
						$SERVER_INFO = $ServerMgmt -> list_match_service_id($MACHINE_UUID[$i]);			
						$SERVER_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $SERVER_INFO['CloudType']);
						
						$SERVER_LABEL = $SERVER_INFO['Hostname'].'\n'.implode(",",json_decode($SERVER_INFO['Address'])).'\n'.$SERVER_TYPE;
						$SERVER_PNG = ($SERVER_INFO['CloudType'] == 'On-Premises')? $SOURCE_SERVER_PNG:$TARGET_SERVER_PNG;
												
						$NODES[$MACHINE_UUID[$i]] = array('id' => $MACHINE_UUID[$i], 'from' => $LINK_FROM, 'label' => $SERVER_LABEL, 'type' => $PROTECT_TYPE, 'shape' => 'image', 'image' => $SERVER_PNG, 'size' => 40, 'is_direct' => $SERVER_INFO['IsDirect'], 'x' => $X_AXIS, 'y' => $Y_AXIS);
					}
					
					#ADD RECOVERY POSITION
					if ($TYPE == 'Recovery')
					{
						foreach ($RECOVERY_HOST as $TARGET_CLOUD_UUID => $TARGET_CONNECTION_UUID)
						{
							if ($TARGET_CONNECTION_UUID == $CONN_UUID)
							{
								$NODES[$JOB_UUID]['x'] = $NODES[$TARGET_CLOUD_UUID]['x'];
								$NODES[$JOB_UUID]['y'] = 170;
							}
						}
					}
					
					#REMOVE DUPLICATE ITEMS
					$NODES = array_values($NODES);
		
					#EDGES DIRECTION
					for($i=0; $i<count($NODES); $i++)
					{
						if ($NODES[$i]['type'] == 'source')
						{
							$EDGES[] = array("from" => $PACK_UUID, "to" => $NODES[$i]['id'], "arrows" => ($NODES[$i]['id'] == TRUE)?"to":"from", "length" => 200);
						}						
						if ($NODES[$i]['type'] == 'target')
						{							
							$EDGES[] = array("from" => $NODES[$i]['from'], "to" => $NODES[$i]['id'], "arrows" => ($NODES[$i]['id'] == TRUE)?"to":"from", "length" => 250);
						}						
					}
					
					#RECOVERY HOST
					if ($TYPE == 'Recovery')
					{
						foreach ($RECOVERY_HOST as $TARGET_CLOUD_UUID => $TARGET_CONNECTION_UUID)
						{
							if ($TARGET_CONNECTION_UUID == $CONN_UUID)
							{
								$EDGES[] = array("from" => $TARGET_CLOUD_UUID, "to" => $JOB_UUID, "arrows" => "to", "length" => 200);
							}
						}
					}
					
					return array('node' => stripslashes(json_encode($NODES)), 'edge' => json_encode($EDGES));
				break;
				
				default:
					return false;
			}
		}
		else
		{
			return false;
		}
	}
}